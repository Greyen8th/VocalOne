#include "Reverb.h"

void VocalReverb::prepare (double sampleRate, int maxBlock, int numChannels)
{
    sr = sampleRate;
    const int maxPreDelay = (int) (sr * 0.25f) + 4;
    preDelayL.assign ((size_t) maxPreDelay, 0.0f);
    preDelayR.assign ((size_t) maxPreDelay, 0.0f);
    preDelayWrite = 0;

    // Early reflections: sparse delays based on prime numbers for natural diffusion
    const int baseDelay = (int) (sr * 0.001f); // 1ms base
    earlyGains = { 0.8f, 0.6f, 0.5f, 0.4f, 0.3f, 0.25f, 0.2f, 0.15f, 0.1f, 0.08f, 0.06f, 0.04f };

    for (int i = 0; i < numEarlyTaps; ++i)
    {
        earlyDelaysL[(size_t) i] = baseDelay * (i + 1) * (i * 2 + 1);
        earlyDelaysR[(size_t) i] = baseDelay * (i + 1) * (i * 2 + 3);
    }

    // All-pass filters: small delays for dense diffusion
    const int apBase = (int) (sr * 0.003f);
    for (int i = 0; i < 4; ++i)
    {
        allPassL[i].prepare (apBase * (i * 2 + 3), 0.7f);
        allPassR[i].prepare (apBase * (i * 2 + 5), 0.7f);
    }

    // Comb filters: longer delays for the tail
    const int combBase = (int) (sr * 0.015f);
    for (int i = 0; i < 4; ++i)
    {
        combL[i].prepare (combBase * (i * 3 + 7), 0.6f, 0.5f);
        combR[i].prepare (combBase * (i * 3 + 11), 0.6f, 0.5f);
    }

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) maxBlock, 1 };
    hpL.prepare (spec); hpR.prepare (spec);
    *hpL.coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, 200.0f);
    *hpR.coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, 200.0f);

    reset();
}

void VocalReverb::reset()
{
    std::fill (preDelayL.begin(), preDelayL.end(), 0.0f);
    std::fill (preDelayR.begin(), preDelayR.end(), 0.0f);
    preDelayWrite = 0;
    for (auto& ap : allPassL) ap.buffer.assign (ap.buffer.size(), 0.0f);
    for (auto& ap : allPassR) ap.buffer.assign (ap.buffer.size(), 0.0f);
    for (auto& c : combL) { c.buffer.assign (c.buffer.size(), 0.0f); c.filterStore = 0.0f; }
    for (auto& c : combR) { c.buffer.assign (c.buffer.size(), 0.0f); c.filterStore = 0.0f; }
    modPhaseL = modPhaseR = 0.0f;
    hpL.reset(); hpR.reset();
}

void VocalReverb::setParameters (bool enabled, float size01, float damping01,
                            float width01, float mix01, float preDelayMsV)
{
    on = enabled;
    mix = juce::jlimit (0.0f, 1.0f, mix01);
    size = juce::jlimit (0.0f, 1.0f, size01);
    damping = juce::jlimit (0.0f, 1.0f, damping01);
    width = juce::jlimit (0.0f, 1.0f, width01);
    preDelayMs = juce::jlimit (0.0f, 200.0f, preDelayMsV);

    preDelaySamplesL = (int) (preDelayMs * 0.001f * (float) sr);
    preDelaySamplesR = (int) ((preDelayMs + 5.0f) * 0.001f * (float) sr); // slight stereo offset

    // Update comb filter coefficients based on size/damping
    for (int i = 0; i < 4; ++i)
    {
        combL[i].coeff = 0.4f + size * 0.55f;
        combL[i].damping = 0.3f + damping * 0.6f;
        combR[i].coeff = 0.4f + size * 0.55f;
        combR[i].damping = 0.3f + damping * 0.6f;
    }

    // Modulation depth based on size
    modDepth = 2.0f + size * 6.0f;
    modFreq = 0.3f + damping * 0.7f;
}

float VocalReverb::processMonoChain (float input, std::array<AllPassFilter, 4>& allpass,
                                      std::array<CombFilter, 4>& comb)
{
    // Early reflections: sparse FIR
    float early = 0.0f;
    for (int i = 0; i < numEarlyTaps; ++i)
        early += input * earlyGains[(size_t) i] * (1.0f - (float) i / (float) numEarlyTaps) * 0.5f;

    // All-pass cascade for diffusion
    float ap = early + input * 0.5f;
    for (int i = 0; i < 4; ++i)
        ap = allpass[i].process (ap);

    // Parallel comb filters (4 combs summed)
    float combSum = 0.0f;
    for (int i = 0; i < 4; ++i)
        combSum += comb[i].process (ap) * 0.25f;

    return combSum;
}

void VocalReverb::process (juce::AudioBuffer<float>& buffer)
{
    if (! on || mix <= 0.0f)
        return;

    const int numCh  = buffer.getNumChannels();
    const int numSmp = buffer.getNumSamples();

    const float modInc = modFreq * juce::MathConstants<float>::twoPi / (float) sr;

    for (int i = 0; i < numSmp; ++i)
    {
        // Pre-delay
        const float inL = buffer.getReadPointer (0)[i];
        const float inR = numCh > 1 ? buffer.getReadPointer (1)[i] : inL;

        preDelayL[(size_t) preDelayWrite] = inL;
        preDelayR[(size_t) preDelayWrite] = inR;

        int readL = preDelayWrite - preDelaySamplesL;
        if (readL < 0) readL += (int) preDelayL.size();
        int readR = preDelayWrite - preDelaySamplesR;
        if (readR < 0) readR += (int) preDelayR.size();

        const float pdL = preDelayL[(size_t) readL];
        const float pdR = preDelayR[(size_t) readR];

        if (++preDelayWrite >= (int) preDelayL.size()) preDelayWrite = 0;

        // Modulation for shimmer (computed for future use, currently unused)
        modPhaseL += modInc;
        modPhaseR += modInc;
        if (modPhaseL > juce::MathConstants<float>::twoPi) modPhaseL -= juce::MathConstants<float>::twoPi;
        if (modPhaseR > juce::MathConstants<float>::twoPi) modPhaseR -= juce::MathConstants<float>::twoPi;

        // Process chains
        float wetL = processMonoChain (pdL, allPassL, combL);
        float wetR = processMonoChain (pdR, allPassR, combR);

        // Stereo width
        if (width < 1.0f)
        {
            const float mid = (wetL + wetR) * 0.5f;
            const float side = (wetL - wetR) * 0.5f * width;
            wetL = mid + side;
            wetR = mid - side;
        }

        // HP filter
        wetL = hpL.processSample (wetL);
        if (numCh > 1) wetR = hpR.processSample (wetR);

        // Mix
        buffer.getWritePointer (0)[i] = inL * (1.0f - mix) + wetL * mix;
        if (numCh > 1)
            buffer.getWritePointer (1)[i] = inR * (1.0f - mix) + wetR * mix;
    }
}
