#include "Doubler.h"

void Doubler::prepare (double sampleRate, int /*maxBlock*/, int /*numChannels*/)
{
    sr = sampleRate;
    size = juce::nextPowerOfTwo ((int) (sr * 0.1));
    line.assign ((size_t) size, 0.0f);
    reset();
}

void Doubler::reset()
{
    std::fill (line.begin(), line.end(), 0.0f);
    writePos = 0;
    for (auto& v : vs) v.phase = 0.0f;
}

void Doubler::setParameters (bool enabled, int voicesV, float detuneCents,
                             float delayMs, float spread01, float mix01)
{
    on = enabled;
    voices = juce::jlimit (1, maxVoices, voicesV);
    detune = detuneCents;
    baseDelay = delayMs;
    spread = juce::jlimit (0.0f, 1.0f, spread01);
    mix = juce::jlimit (0.0f, 1.0f, mix01);

    for (int i = 0; i < maxVoices; ++i)
    {
        // alternate detune sign, spread delays, pan across the field
        const float sign = (i % 2 == 0) ? 1.0f : -1.0f;
        const float cents = detune * sign * (1.0f + 0.3f * (float) (i / 2));
        vs[(size_t) i].rate = std::pow (2.0f, cents / 1200.0f);
        vs[(size_t) i].delaySamp = juce::jlimit (1.0f, (float) size - 4.0f,
            (baseDelay + i * 7.0f) * 0.001f * (float) sr);
        const float pan = spread * sign * (0.4f + 0.2f * (float) (i / 2));   // -.. +
        vs[(size_t) i].panL = std::cos ((pan * 0.5f + 0.5f) * juce::MathConstants<float>::halfPi);
        vs[(size_t) i].panR = std::sin ((pan * 0.5f + 0.5f) * juce::MathConstants<float>::halfPi);
    }
}

void Doubler::process (juce::AudioBuffer<float>& buffer)
{
    if (! on || mix <= 0.0f || buffer.getNumChannels() < 1)
        return;

    const int numCh  = buffer.getNumChannels();
    const int numSmp = buffer.getNumSamples();

    auto read = [&] (float delaySamp) -> float
    {
        float rp = (float) writePos - delaySamp;
        while (rp < 0.0f) rp += (float) size;
        const int i0 = (int) rp;
        const int i1 = (i0 + 1) % size;
        const float frac = rp - (float) i0;
        return line[(size_t) i0] + frac * (line[(size_t) i1] - line[(size_t) i0]);
    };

    for (int i = 0; i < numSmp; ++i)
    {
        float mono = 0.0f;
        for (int ch = 0; ch < numCh; ++ch) mono += buffer.getReadPointer (ch)[i];
        mono /= (float) numCh;
        line[(size_t) writePos] = mono;

        float wetL = 0.0f, wetR = 0.0f;
        for (int v = 0; v < voices; ++v)
        {
            auto& vc = vs[(size_t) v];
            // modulate the delay slightly by phase to create the detune shimmer
            vc.phase += (vc.rate - 1.0f);
            const float mod = 30.0f * std::sin (vc.phase * 0.0005f);
            const float s = read (vc.delaySamp + mod);
            wetL += s * vc.panL;
            wetR += s * vc.panR;
        }
        const float norm = 1.0f / juce::jmax (1, voices);
        wetL *= norm; wetR *= norm;

        auto* L = buffer.getWritePointer (0);
        L[i] = L[i] * (1.0f - mix) + wetL * mix;
        if (numCh > 1)
        {
            auto* R = buffer.getWritePointer (1);
            R[i] = R[i] * (1.0f - mix) + wetR * mix;
        }

        writePos = (writePos + 1) % size;
    }
}
