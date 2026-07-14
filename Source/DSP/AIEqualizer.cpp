#include "AIEqualizer.h"

void AIEqualizer::prepare (double sampleRate, int maxBlock, int numChannels)
{
    sr = sampleRate;
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) maxBlock, 1 };
    for (auto& f : filtersL) f.prepare (spec);
    for (auto& f : filtersR) f.prepare (spec);

    fftScratch.assign (fftSize * 2, 0.0f);
    ring.assign (fftSize, 0.0f);
    ringPos = 0;

    juce::ignoreUnused (numChannels);
    updateCoefficients();
    reset();
}

void AIEqualizer::reset()
{
    for (auto& f : filtersL) f.reset();
    for (auto& f : filtersR) f.reset();
}

void AIEqualizer::setManualGain (int band, float dB)
{
    if (band >= 0 && band < numBands)
        manualGain[(size_t) band] = dB;
}

void AIEqualizer::updateCoefficients()
{
    for (int b = 0; b < numBands; ++b)
    {
        const float gDb = manualGain[(size_t) b] + autoGain[(size_t) b] * amount;
        const float gain = juce::Decibels::decibelsToGain (gDb);

        juce::dsp::IIR::Coefficients<float>::Ptr coeffs;
        if (b == Air)
            coeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf (sr, bands[(size_t) b].freq, bands[(size_t) b].q, gain);
        else
            coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter (sr, bands[(size_t) b].freq, bands[(size_t) b].q, gain);

        *filtersL[(size_t) b].coefficients = *coeffs;
        *filtersR[(size_t) b].coefficients = *coeffs;
    }
}

void AIEqualizer::analyseSpectrum (const juce::AudioBuffer<float>& buffer)
{
    const int numCh  = buffer.getNumChannels();
    const int numSmp = buffer.getNumSamples();

    for (int i = 0; i < numSmp; ++i)
    {
        float s = 0.0f;
        for (int ch = 0; ch < numCh; ++ch) s += buffer.getReadPointer (ch)[i];
        ring[(size_t) ringPos] = s / (float) numCh;
        if (++ringPos >= fftSize)
        {
            ringPos = 0;

            std::fill (fftScratch.begin(), fftScratch.end(), 0.0f);
            std::copy (ring.begin(), ring.end(), fftScratch.begin());
            window.multiplyWithWindowingTable (fftScratch.data(), (size_t) fftSize);
            fft.performFrequencyOnlyForwardTransform (fftScratch.data());

            const float binHz = (float) sr / (float) fftSize;
            auto bandEnergy = [&] (float lo, float hi)
            {
                float e = 0.0f; int n = 0;
                for (int bin = 1; bin < fftSize / 2; ++bin)
                {
                    const float f = bin * binHz;
                    if (f >= lo && f <= hi) { e += fftScratch[(size_t) bin]; ++n; }
                }
                return n > 0 ? e / (float) n : 0.0f;
            };

            const float total = bandEnergy (40.0f, 16000.0f) + 1.0e-9f;
            auto rel = [&] (float lo, float hi) { return bandEnergy (lo, hi) / total; };

            // Severity heuristics normalised to 0..1.
            detection[Boomy]    = juce::jlimit (0.0f, 1.0f, (rel (60.0f, 120.0f)   - 1.3f) * 1.5f);
            detection[Mud]      = juce::jlimit (0.0f, 1.0f, (rel (200.0f, 400.0f)  - 1.2f) * 1.5f);
            detection[Boxy]     = juce::jlimit (0.0f, 1.0f, (rel (300.0f, 600.0f)  - 1.1f) * 1.5f);
            detection[Nasal]    = juce::jlimit (0.0f, 1.0f, (rel (900.0f, 1200.0f) - 1.1f) * 1.8f);
            detection[Harsh]    = juce::jlimit (0.0f, 1.0f, (rel (2500.0f, 4000.0f)- 1.0f) * 1.8f);
            detection[Air]      = juce::jlimit (0.0f, 1.0f, (0.6f - rel (10000.0f, 16000.0f)) * 1.5f); // lacking air
            detection[Presence] = juce::jlimit (0.0f, 1.0f, (0.8f - rel (3000.0f, 6000.0f)) * 1.5f);   // lacking presence
            detection[Thin]     = juce::jlimit (0.0f, 1.0f, (0.7f - rel (150.0f, 300.0f)) * 1.5f);      // lacking body

            if (autoM)
            {
                // Corrective gains: cut resonances, gently boost missing ranges.
                autoGain[Boomy]    = -6.0f * detection[Boomy];
                autoGain[Mud]      = -5.0f * detection[Mud];
                autoGain[Boxy]     = -4.0f * detection[Boxy];
                autoGain[Nasal]    = -5.0f * detection[Nasal];
                autoGain[Harsh]    = -5.0f * detection[Harsh];
                autoGain[Air]      = +4.0f * detection[Air];
                autoGain[Presence] = +3.0f * detection[Presence];
                autoGain[Thin]     = +3.0f * detection[Thin];
                updateCoefficients();
            }
        }
    }
}

void AIEqualizer::process (juce::AudioBuffer<float>& buffer)
{
    if (! on)
        return;

    analyseSpectrum (buffer);
    if (! autoM)
        updateCoefficients();   // reflect manual changes each block

    const int numCh  = buffer.getNumChannels();
    const int numSmp = buffer.getNumSamples();

    for (int ch = 0; ch < numCh; ++ch)
    {
        auto* data = buffer.getWritePointer (ch);
        auto& chain = (ch == 0) ? filtersL : filtersR;
        for (int i = 0; i < numSmp; ++i)
        {
            float s = data[i];
            for (auto& f : chain)
                s = f.processSample (s);
            data[i] = s;
        }
    }
}
