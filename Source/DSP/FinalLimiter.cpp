#include "FinalLimiter.h"

void FinalLimiter::prepare (double sampleRate, int /*maxBlock*/, int numChannels)
{
    sr = sampleRate;
    lookaheadSamples = juce::jmax (1, (int) (sr * 0.010));   // 10 ms look-ahead (more than old 5ms)
    delayBuf.assign ((size_t) juce::jmax (1, numChannels),
                     std::vector<float> ((size_t) lookaheadSamples, 0.0f));

    juce::dsp::ProcessSpec spec { sampleRate, 512, 1 };
    kWeightL.prepare (spec);
    kWeightR.prepare (spec);
    // approximate K-weighting with a high-shelf +4 dB @ 1.5 kHz
    auto k = juce::dsp::IIR::Coefficients<float>::makeHighShelf (sr, 1500.0f, 0.7f, juce::Decibels::decibelsToGain (4.0f));
    *kWeightL.coefficients = *k;
    *kWeightR.coefficients = *k;
    reset();
}

void FinalLimiter::reset()
{
    for (auto& d : delayBuf) std::fill (d.begin(), d.end(), 0.0f);
    writePos = 0;
    envGain = 1.0f;
    grDb = 0.0f;
    truePeakDb = -60.0f;
    kWeightL.reset(); kWeightR.reset();
    msAccum = 0.0f; msCount = 0;
    lufs = -60.0f;
}

void FinalLimiter::setParameters (bool enabled, float ceilingDb, float releaseMs,
                                float inputGainDb)
{
    on = enabled;
    ceiling = juce::Decibels::decibelsToGain (juce::jlimit (-12.0f, 0.0f, ceilingDb));
    inputGain = juce::Decibels::decibelsToGain (inputGainDb);
    relCoef = std::exp (-1.0f / (juce::jmax (1.0f, releaseMs) * 0.001f * (float) sr));
    // Attack time: 1ms (very fast)
    attCoef = std::exp (-1.0f / (1.0f * 0.001f * (float) sr));
}

float FinalLimiter::detectTruePeak (const float* samples, int n)
{
    // 4x upsampling linear interpolation for true peak detection
    float maxPeak = 0.0f;
    for (int i = 0; i < n - 1; ++i)
    {
        const float s0 = samples[i];
        const float s1 = samples[i + 1];
        maxPeak = juce::jmax (maxPeak, std::abs (s0));
        // Check interpolated points at 1/4, 1/2, 3/4
        maxPeak = juce::jmax (maxPeak, std::abs (s0 * 0.75f + s1 * 0.25f));
        maxPeak = juce::jmax (maxPeak, std::abs (s0 * 0.5f + s1 * 0.5f));
        maxPeak = juce::jmax (maxPeak, std::abs (s0 * 0.25f + s1 * 0.75f));
    }
    if (n > 0)
        maxPeak = juce::jmax (maxPeak, std::abs (samples[n - 1]));

    return maxPeak;
}

void FinalLimiter::process (juce::AudioBuffer<float>& buffer)
{
    if (! on)
        return;

    const int numCh  = buffer.getNumChannels();
    const int numSmp = buffer.getNumSamples();

    for (int i = 0; i < numSmp; ++i)
    {
        // apply input gain and find peak across channels
        float peak = 0.0f;
        for (int ch = 0; ch < numCh; ++ch)
        {
            const float v = buffer.getReadPointer (ch)[i] * inputGain;
            peak = juce::jmax (peak, std::abs (v));
        }

        // desired gain to keep peak under ceiling
        float target = 1.0f;
        if (peak > ceiling && peak > 0.0f)
            target = ceiling / peak;

        // attack instantly, release smoothly with separate attack/release behavior
        if (target < envGain)
            envGain = envGain * attCoef + target * (1.0f - attCoef); // fast attack
        else
            envGain = target + (envGain - target) * relCoef; // slow release

        grDb = juce::Decibels::gainToDecibels (juce::jmax (1.0e-4f, envGain));

        // process delayed sample with current gain
        for (int ch = 0; ch < numCh; ++ch)
        {
            auto& d = delayBuf[(size_t) ch];
            const float delayed = d[(size_t) writePos];
            d[(size_t) writePos] = buffer.getReadPointer (ch)[i] * inputGain;

            float out = delayed * envGain;
            out = juce::jlimit (-ceiling, ceiling, out);
            buffer.getWritePointer (ch)[i] = out;
        }

        // loudness integration
        float kw = 0.0f;
        for (int ch = 0; ch < numCh; ++ch)
        {
            auto& f = (ch == 0) ? kWeightL : kWeightR;
            const float w = f.processSample (buffer.getReadPointer (ch)[i]);
            kw += w * w;
        }
        msAccum += kw / (float) juce::jmax (1, numCh);
        if (++msCount >= (int) (sr * 0.4))   // 400 ms window
        {
            const float meanSq = msAccum / (float) msCount;
            lufs = -0.691f + 10.0f * std::log10 (juce::jmax (1.0e-10f, meanSq));
            msAccum = 0.0f; msCount = 0;
        }

        writePos = (writePos + 1) % lookaheadSamples;
    }

    // True peak detection on output (post-limiter)
    for (int ch = 0; ch < numCh; ++ch)
    {
        float tp = detectTruePeak (buffer.getReadPointer (ch), numSmp);
        if (tp > 0.0f)
        {
            float tpDb = juce::Decibels::gainToDecibels (tp, -60.0f);
            truePeakDb = juce::jmax (truePeakDb - 0.01f, tpDb); // smooth decay
        }
    }
}
