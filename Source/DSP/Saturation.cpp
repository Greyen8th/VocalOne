#include "Saturation.h"

void Saturation::prepare (double sampleRate, int maxBlock, int numChannels)
{
    sr = sampleRate;
    const int ch = juce::jmax (1, numChannels);

    oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
        (size_t) ch, 1,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);
    oversampler->initProcessing ((size_t) maxBlock);

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) maxBlock, 1 };
    toneL.prepare (spec);
    toneR.prepare (spec);
    reset();
}

void Saturation::reset()
{
    if (oversampler) oversampler->reset();
    toneL.reset();
    toneR.reset();
}

void Saturation::setParameters (bool enabled, Model m, float drive01, float mix01)
{
    on = enabled;
    model = m;
    drive = juce::jlimit (0.0f, 1.0f, drive01);
    mix = juce::jlimit (0.0f, 1.0f, mix01);

    // Tone tilt per model.
    float shelfFreq = 6000.0f, shelfGainDb = 0.0f;
    switch (model)
    {
        case Model::tube:    shelfFreq = 5000.0f; shelfGainDb = 2.0f;  break;   // warm-bright
        case Model::tape:    shelfFreq = 8000.0f; shelfGainDb = -2.0f; break;   // HF roll-off
        case Model::vintage: shelfFreq = 4000.0f; shelfGainDb = -3.0f; break;
        case Model::warm:    shelfFreq = 3000.0f; shelfGainDb = -2.5f; break;
        case Model::digital: shelfFreq = 9000.0f; shelfGainDb = 1.0f;  break;
    }
    auto coeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf (
        sr, shelfFreq, 0.7f, juce::Decibels::decibelsToGain (shelfGainDb));
    *toneL.coefficients = *coeffs;
    *toneR.coefficients = *coeffs;
}

float Saturation::shape (float x) const
{
    const float d = 1.0f + drive * 9.0f;   // 1..10
    const float xd = x * d;
    switch (model)
    {
        case Model::tube:
            // asymmetric soft clip (adds even harmonics)
            return std::tanh (xd + 0.15f * xd * xd) / d;
        case Model::tape:
            // odd-harmonic soft compression
            return (xd / (1.0f + std::abs (xd))) ;
        case Model::vintage:
            // arctan drive
            return (2.0f / juce::MathConstants<float>::pi) * std::atan (xd);
        case Model::warm:
            // cubic soft clip
            {
                float y = xd;
                if (y > 1.0f) y = 1.0f; else if (y < -1.0f) y = -1.0f;
                return (y - (y * y * y) / 3.0f) * 1.5f / d;
            }
        case Model::digital:
            // hard-ish clip
            return juce::jlimit (-0.8f, 0.8f, xd) / d;
    }
    return x;
}

void Saturation::process (juce::AudioBuffer<float>& buffer)
{
    if (! on || ! oversampler)
        return;

    const int numCh  = buffer.getNumChannels();
    const int numSmp = buffer.getNumSamples();

    juce::AudioBuffer<float> dry;
    dry.makeCopyOf (buffer);

    juce::dsp::AudioBlock<float> block (buffer);
    auto osBlock = oversampler->processSamplesUp (block);

    for (size_t ch = 0; ch < osBlock.getNumChannels(); ++ch)
    {
        auto* p = osBlock.getChannelPointer (ch);
        for (size_t i = 0; i < osBlock.getNumSamples(); ++i)
            p[i] = shape (p[i]);
    }

    oversampler->processSamplesDown (block);

    // tone + wet/dry mix
    for (int ch = 0; ch < numCh; ++ch)
    {
        auto* w = buffer.getWritePointer (ch);
        const auto* d = dry.getReadPointer (ch);
        auto& tone = (ch == 0) ? toneL : toneR;
        for (int i = 0; i < numSmp; ++i)
        {
            float s = tone.processSample (w[i]);
            w[i] = d[i] * (1.0f - mix) + s * mix;
        }
    }
}
