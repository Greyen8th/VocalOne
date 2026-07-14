#include "Saturation.h"

void Saturation::prepare (double sampleRate, int maxBlock, int numChannels)
{
    sr = sampleRate;
    const int ch = juce::jmax (1, numChannels);

    // 4x oversampling for very low aliasing even at high drive
    oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
        (size_t) ch, 2,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true, true); // 4x total (2 stages of 2x)
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

    // Tone shaping per model: more sophisticated than just a shelf
    float toneFreq = 3000.0f, toneQ = 0.7f;
    toneGain = 1.0f;
    switch (model)
    {
        case Model::tube:    toneFreq = 4500.0f; toneQ = 0.8f; toneGain = 1.3f; break; // warm + slight HF boost
        case Model::tape:    toneFreq = 5000.0f; toneQ = 0.5f; toneGain = 0.85f; break; // gentle HF roll-off
        case Model::vintage: toneFreq = 3500.0f; toneQ = 0.6f; toneGain = 0.75f; break; // darker, more lo-fi
        case Model::warm:    toneFreq = 2500.0f; toneQ = 0.7f; toneGain = 0.9f; break; // very smooth, warm
        case Model::digital: toneFreq = 8000.0f; toneQ = 0.7f; toneGain = 1.15f; break; // bright, aggressive
    }
    auto coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter (sr, toneFreq, toneQ, toneGain);
    *toneL.coefficients = *coeffs;
    *toneR.coefficients = *coeffs;
}

float Saturation::tubeShape (float x) const
{
    // Asymmetric triode soft clip: adds even harmonics
    const float d = 1.0f + drive * 9.0f;   // 1..10
    const float xd = x * d;
    // Even-harmonic asymmetry
    const float asym = 0.12f * drive;
    return std::tanh (xd + asym * xd * xd) / d;
}

float Saturation::tapeShape (float x) const
{
    // Odd-harmonic compression with soft knee
    const float d = 1.0f + drive * 8.0f;
    const float xd = x * d;
    // Tape-like saturation: compressive nonlinearity
    const float y = xd / (1.0f + std::abs (xd) + 0.1f * xd * xd);
    return y * juce::jlimit (0.8f, 1.0f, 1.0f - drive * 0.1f); // slight gain reduction at high drive
}

float Saturation::vintageShape (float x) const
{
    // Arctan with drive, but with more warmth
    const float d = 1.0f + drive * 7.0f;
    const float xd = x * d;
    const float y = (2.0f / juce::MathConstants<float>::pi) * std::atan (xd * 0.8f);
    return y / d * 1.1f; // slight makeup gain
}

float Saturation::warmShape (float x) const
{
    // Cubic soft clip with very smooth transition
    const float d = 1.0f + drive * 6.0f;
    float xd = x * d;
    if (xd > 1.0f) xd = 1.0f;
    else if (xd < -1.0f) xd = -1.0f;
    return (xd - (xd * xd * xd) / 3.0f) * 1.5f / d;
}

float Saturation::digitalShape (float x) const
{
    // Hard-ish clip with aggressive drive
    const float d = 1.0f + drive * 12.0f;
    const float xd = x * d;
    float y = juce::jlimit (-0.85f, 0.85f, xd);
    // Add a bit of foldback distortion at high drive
    if (drive > 0.7f)
    {
        const float fold = (std::abs (xd) - 0.85f) * (drive - 0.7f) * 3.33f;
        if (fold > 0.0f)
            y -= juce::jlimit (-0.3f, 0.3f, fold) * ((xd >= 0.0f) ? 1.0f : -1.0f);
    }
    return y / d;
}

float Saturation::shape (float x) const
{
    switch (model)
    {
        case Model::tube:    return tubeShape (x);
        case Model::tape:    return tapeShape (x);
        case Model::vintage: return vintageShape (x);
        case Model::warm:    return warmShape (x);
        case Model::digital: return digitalShape (x);
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

    // Tone shaping + wet/dry mix
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
