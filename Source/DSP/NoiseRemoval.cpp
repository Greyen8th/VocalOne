#include "NoiseRemoval.h"

NoiseRemoval::NoiseRemoval()
{
    noiseMag.assign (fftSize / 2 + 1, 0.0f);
}

void NoiseRemoval::prepare (double sampleRate, int /*maxBlock*/, int channels)
{
    sr = sampleRate;
    numChannels = juce::jmax (1, channels);
    chans.assign ((size_t) numChannels, {});
    for (auto& c : chans)
    {
        c.inFifo.assign (fftSize, 0.0f);
        c.outFifo.assign (fftSize, 0.0f);
        c.overlap.assign (fftSize, 0.0f);
        c.fftData.assign (fftSize * 2, 0.0f);
        c.fifoIndex = 0;
    }
    reset();
}

void NoiseRemoval::reset()
{
    for (auto& c : chans)
    {
        std::fill (c.inFifo.begin(),  c.inFifo.end(),  0.0f);
        std::fill (c.outFifo.begin(), c.outFifo.end(), 0.0f);
        std::fill (c.overlap.begin(), c.overlap.end(), 0.0f);
        c.fifoIndex = 0;
    }
}

void NoiseRemoval::resetProfile()
{
    std::fill (noiseMag.begin(), noiseMag.end(), 0.0f);
    learnedFrames = 0;
}

void NoiseRemoval::setParameters (bool enabled, float amountPct, bool learn)
{
    on = enabled;
    amount = juce::jlimit (0.0f, 1.0f, amountPct * 0.01f);
    learning = learn;
}

void NoiseRemoval::process (juce::AudioBuffer<float>& buffer)
{
    if (! on)
        return;

    const int numCh  = juce::jmin (buffer.getNumChannels(), numChannels);
    const int numSmp = buffer.getNumSamples();
    for (int ch = 0; ch < numCh; ++ch)
        processChannel (buffer.getWritePointer (ch), numSmp, ch);
}

void NoiseRemoval::processChannel (float* data, int numSamples, int ch)
{
    auto& c = chans[(size_t) ch];
    for (int n = 0; n < numSamples; ++n)
    {
        c.inFifo[(size_t) c.fifoIndex] = data[n];
        data[n] = c.outFifo[(size_t) c.fifoIndex];

        if (++c.fifoIndex >= fftSize)
            c.fifoIndex = 0;

        // Hop reached?
        if (c.fifoIndex % hopSize == 0)
            processFrame (ch);
    }
}

void NoiseRemoval::processFrame (int ch)
{
    auto& c = chans[(size_t) ch];

    // Assemble windowed frame from the input FIFO (linearised from fifoIndex).
    for (int i = 0; i < fftSize; ++i)
    {
        const int idx = (c.fifoIndex + i) % fftSize;
        c.fftData[(size_t) i] = c.inFifo[(size_t) idx];
    }
    window.multiplyWithWindowingTable (c.fftData.data(), (size_t) fftSize);

    // interleaved complex layout for performRealOnlyForwardTransform
    fft.performRealOnlyForwardTransform (c.fftData.data());

    const int numBins = fftSize / 2;

    // Compute magnitudes / phases and, if learning, update the noise profile.
    std::vector<float> mag ((size_t) numBins + 1);
    std::vector<float> phase ((size_t) numBins + 1);
    float frameEnergy = 0.0f;

    for (int b = 0; b <= numBins; ++b)
    {
        const float re = c.fftData[(size_t) (2 * b)];
        const float im = c.fftData[(size_t) (2 * b + 1)];
        mag[(size_t) b]   = std::sqrt (re * re + im * im);
        phase[(size_t) b] = std::atan2 (im, re);
        frameEnergy += mag[(size_t) b];
    }

    // Learn noise on low-energy frames only (channel 0 drives the shared profile).
    if (learning && ch == 0)
    {
        static thread_local float minEnergy = 1.0e9f;
        minEnergy = juce::jmin (minEnergy * 1.0001f, frameEnergy);
        if (frameEnergy < minEnergy * 1.5f || learnedFrames < 8)
        {
            const float a = 1.0f / (float) (learnedFrames + 1);
            for (int b = 0; b <= numBins; ++b)
                noiseMag[(size_t) b] = noiseMag[(size_t) b] * (1.0f - a) + mag[(size_t) b] * a;
            ++learnedFrames;
        }
    }

    // Spectral subtraction.
    const float overSub = 1.0f + amount * 2.0f;     // 1..3
    const float floorGain = 0.05f + (1.0f - amount) * 0.2f;
    for (int b = 0; b <= numBins; ++b)
    {
        const float noise = noiseMag[(size_t) b] * overSub;
        float outMag = mag[(size_t) b] - noise;
        outMag = juce::jmax (outMag, mag[(size_t) b] * floorGain);

        c.fftData[(size_t) (2 * b)]     = outMag * std::cos (phase[(size_t) b]);
        c.fftData[(size_t) (2 * b + 1)] = outMag * std::sin (phase[(size_t) b]);
    }

    fft.performRealOnlyInverseTransform (c.fftData.data());
    window.multiplyWithWindowingTable (c.fftData.data(), (size_t) fftSize);

    // Overlap-add with hop normalisation (Hann, 75% overlap => gain 1.5).
    const float norm = 2.0f / 3.0f;
    for (int i = 0; i < fftSize; ++i)
    {
        const int idx = (c.fifoIndex + i) % fftSize;
        c.overlap[(size_t) idx] += c.fftData[(size_t) i] * norm;
    }
    // Emit the first hop of the overlap buffer into outFifo, then shift.
    for (int i = 0; i < hopSize; ++i)
    {
        const int idx = (c.fifoIndex + i) % fftSize;
        c.outFifo[(size_t) idx] = c.overlap[(size_t) idx];
        c.overlap[(size_t) idx] = 0.0f;
    }
}
