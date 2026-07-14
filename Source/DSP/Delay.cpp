#include "Delay.h"

void Delay::prepare (double sampleRate, int maxBlock, int numChannels)
{
    sr = sampleRate;
    size = juce::nextPowerOfTwo ((int) (sr * 2.5));   // up to 2.5 s
    lines.assign ((size_t) juce::jmax (2, numChannels), std::vector<float> ((size_t) size, 0.0f));

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) maxBlock, 1 };
    dampL.prepare (spec);
    dampR.prepare (spec);
    *dampL.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass (sr, 6000.0f);
    *dampR.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass (sr, 6000.0f);
    reset();
}

void Delay::reset()
{
    for (auto& l : lines) std::fill (l.begin(), l.end(), 0.0f);
    writePos = 0;
    dampL.reset();
    dampR.reset();
}

void Delay::setParameters (bool enabled, float timeMs, float feedback01,
                           float mix01, bool pp, float dampingHz)
{
    on = enabled;
    delaySamples = juce::jlimit (1.0f, (float) size - 4.0f, timeMs * 0.001f * (float) sr);
    feedback = juce::jlimit (0.0f, 0.95f, feedback01);
    mix = juce::jlimit (0.0f, 1.0f, mix01);
    pingPong = pp;

    const float fc = juce::jlimit (500.0f, 18000.0f, dampingHz);
    *dampL.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass (sr, fc);
    *dampR.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass (sr, fc);
}

void Delay::process (juce::AudioBuffer<float>& buffer)
{
    if (! on || mix <= 0.0f)
        return;

    const int numCh  = juce::jmin (buffer.getNumChannels(), (int) lines.size());
    const int numSmp = buffer.getNumSamples();

    auto readLine = [&] (int ch, float delay) -> float
    {
        float rp = (float) writePos - delay;
        while (rp < 0.0f) rp += (float) size;
        const int i0 = (int) rp;
        const int i1 = (i0 + 1) % size;
        const float frac = rp - (float) i0;
        const auto& l = lines[(size_t) ch];
        return l[(size_t) i0] + frac * (l[(size_t) i1] - l[(size_t) i0]);
    };

    for (int i = 0; i < numSmp; ++i)
    {
        const float inL = buffer.getReadPointer (0)[i];
        const float inR = numCh > 1 ? buffer.getReadPointer (1)[i] : inL;

        float dL = readLine (0, delaySamples);
        float dR = numCh > 1 ? readLine (1, delaySamples) : dL;

        dL = dampL.processSample (dL);
        dR = dampR.processSample (dR);

        float wL, wR;
        if (pingPong && numCh > 1)
        {
            wL = inL + dR * feedback;
            wR = inR + dL * feedback;
        }
        else
        {
            wL = inL + dL * feedback;
            wR = inR + dR * feedback;
        }

        lines[0][(size_t) writePos] = wL;
        if (numCh > 1) lines[1][(size_t) writePos] = wR;

        buffer.getWritePointer (0)[i] = inL * (1.0f - mix) + dL * mix;
        if (numCh > 1)
            buffer.getWritePointer (1)[i] = inR * (1.0f - mix) + dR * mix;

        writePos = (writePos + 1) % size;
    }
}
