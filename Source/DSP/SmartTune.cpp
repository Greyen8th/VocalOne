#include "SmartTune.h"

SmartTune::SmartTune() = default;

void SmartTune::prepare (double sampleRate, int /*maxBlock*/, int numChannels)
{
    sr = sampleRate;
    dlSize = juce::nextPowerOfTwo ((int) (sr * 0.1));   // 100 ms
    delayLines.assign ((size_t) juce::jmax (1, numChannels),
                       std::vector<float> ((size_t) dlSize, 0.0f));
    grainSamples = (float) (sr * 0.030);                // 30 ms grains
    reset();
}

void SmartTune::reset()
{
    for (auto& dl : delayLines)
        std::fill (dl.begin(), dl.end(), 0.0f);
    writePos = 0;
    phase = 0.0f;
    smoothedRatio = 1.0f;
    currentCents = 0.0f;
}

void SmartTune::setParameters (bool enabled, int keyIndex, Scale scaleV,
                               float retuneSpeed01, float strength01,
                               bool formantPreserve)
{
    on = enabled;
    key = ((keyIndex % 12) + 12) % 12;
    scale = scaleV;
    retune = juce::jlimit (0.0f, 1.0f, retuneSpeed01);
    strength = juce::jlimit (0.0f, 1.0f, strength01);
    formant = formantPreserve;
}

float SmartTune::nearestScaleFreq (float f0) const
{
    if (f0 <= 0.0f)
        return f0;

    // note relative to A4 = 440
    const float midi = 69.0f + 12.0f * std::log2 (f0 / 440.0f);

    // Scale intervals (semitones within an octave, relative to the key root).
    static const std::array<int, 12> chrom { 0,1,2,3,4,5,6,7,8,9,10,11 };
    static const std::array<int, 7>  maj   { 0,2,4,5,7,9,11 };
    static const std::array<int, 7>  min   { 0,2,3,5,7,8,10 };
    static const std::array<int, 5>  majP  { 0,2,4,7,9 };
    static const std::array<int, 5>  minP  { 0,3,5,7,10 };

    std::vector<int> degrees;
    switch (scale)
    {
        case Scale::major:            degrees.assign (maj.begin(),  maj.end());  break;
        case Scale::minor:            degrees.assign (min.begin(),  min.end());  break;
        case Scale::majorPentatonic:  degrees.assign (majP.begin(), majP.end()); break;
        case Scale::minorPentatonic:  degrees.assign (minP.begin(), minP.end()); break;
        case Scale::chromatic:
        default:                      degrees.assign (chrom.begin(), chrom.end()); break;
    }

    // Find nearest allowed midi note.
    float best = midi;
    float bestDist = 1.0e9f;
    const int baseOct = (int) std::floor ((midi - key) / 12.0f) - 1;
    for (int oct = baseOct; oct <= baseOct + 2; ++oct)
    {
        for (int d : degrees)
        {
            const float candidate = (float) (key + d + 12 * oct) + 12.0f;  // +12 offset from A tuning frame
            const float cand = (float) (key + d) + 12.0f * (float) oct;
            juce::ignoreUnused (candidate);
            const float m = 12.0f * (float) oct + (float) (key + d);
            const float dist = std::abs (m - midi);
            if (dist < bestDist) { bestDist = dist; best = m; }
            juce::ignoreUnused (cand);
        }
    }

    return 440.0f * std::pow (2.0f, (best - 69.0f) / 12.0f);
}

float SmartTune::trackPitch (const float* mono, int n)
{
    // Lightweight autocorrelation pitch tracker (fallback when no external f0).
    const int minLag = juce::jmax (2, (int) (sr / 1000.0));
    const int maxLag = juce::jmin (n - 1, (int) (sr / 70.0));
    if (maxLag <= minLag) return 0.0f;

    float bestCorr = 0.0f; int bestLag = 0;
    float energy = 1.0e-9f;
    for (int i = 0; i < n; ++i) energy += mono[i] * mono[i];

    for (int lag = minLag; lag <= maxLag; ++lag)
    {
        float corr = 0.0f;
        for (int i = 0; i + lag < n; ++i)
            corr += mono[i] * mono[i + lag];
        corr /= energy;
        if (corr > bestCorr) { bestCorr = corr; bestLag = lag; }
    }
    if (bestLag <= 0 || bestCorr < 0.3f) return 0.0f;
    return (float) sr / (float) bestLag;
}

void SmartTune::process (juce::AudioBuffer<float>& buffer)
{
    const int numCh  = buffer.getNumChannels();
    const int numSmp = buffer.getNumSamples();

    // Build mono for pitch tracking.
    std::vector<float> mono ((size_t) numSmp);
    for (int i = 0; i < numSmp; ++i)
    {
        float s = 0.0f;
        for (int ch = 0; ch < numCh; ++ch)
            s += buffer.getReadPointer (ch)[i];
        mono[(size_t) i] = s / (float) numCh;
    }

    float f0 = externalF0 > 0.0f ? externalF0 : trackPitch (mono.data(), numSmp);

    if (! on || f0 <= 0.0f)
    {
        // still push through the delay line so state stays coherent
        for (int i = 0; i < numSmp; ++i)
        {
            for (int ch = 0; ch < numCh && ch < (int) delayLines.size(); ++ch)
                delayLines[(size_t) ch][(size_t) writePos] = buffer.getReadPointer (ch)[i];
            writePos = (writePos + 1) % dlSize;
        }
        currentCents = 0.0f;
        return;
    }

    targetHz = nearestScaleFreq (f0);
    float desiredRatio = targetHz / f0;

    // Apply strength (blend toward 1.0 = no shift).
    desiredRatio = 1.0f + (desiredRatio - 1.0f) * strength;

    // Retune speed -> smoothing coefficient. speed 0 = fast, 1 = slow.
    const float tauMs = 5.0f + retune * 250.0f;
    const float coef  = std::exp (-1.0f / (tauMs * 0.001f * (float) sr / (float) numSmp));
    smoothedRatio = desiredRatio + (smoothedRatio - desiredRatio) * coef;

    currentCents = 1200.0f * std::log2 (juce::jmax (1.0e-4f, smoothedRatio));

    // Two-tap granular pitch shift.
    const float pitchInc = smoothedRatio;                 // read speed
    const float grainLen = grainSamples;

    for (int i = 0; i < numSmp; ++i)
    {
        for (int ch = 0; ch < numCh && ch < (int) delayLines.size(); ++ch)
        {
            auto& dl = delayLines[(size_t) ch];
            dl[(size_t) writePos] = buffer.getReadPointer (ch)[i];
        }

        // read positions for two overlapping grains, half a grain apart
        auto readTap = [&] (int ch, float ph) -> float
        {
            const float delaySamples = ph * grainLen * (pitchInc - 1.0f);
            float rp = (float) writePos - delaySamples;
            while (rp < 0.0f) rp += (float) dlSize;
            const int i0 = (int) rp;
            const int i1 = (i0 + 1) % dlSize;
            const float frac = rp - (float) i0;
            const auto& dl = delayLines[(size_t) ch];
            return dl[(size_t) i0] + frac * (dl[(size_t) i1] - dl[(size_t) i0]);
        };

        const float ph1 = phase;
        const float ph2 = std::fmod (phase + 0.5f, 1.0f);
        const float w1 = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * ph1);
        const float w2 = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * ph2);
        const float wsum = juce::jmax (1.0e-4f, w1 + w2);

        for (int ch = 0; ch < numCh && ch < (int) delayLines.size(); ++ch)
        {
            const float shifted = (readTap (ch, ph1) * w1 + readTap (ch, ph2) * w2) / wsum;
            buffer.getWritePointer (ch)[i] = shifted;
        }

        phase += (pitchInc - 1.0f) / grainLen;
        if (phase >= 1.0f) phase -= 1.0f;
        if (phase < 0.0f)  phase += 1.0f;

        writePos = (writePos + 1) % dlSize;
    }
    juce::ignoreUnused (formant);
}
