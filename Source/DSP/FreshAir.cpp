#include "FreshAir.h"

void FreshAir::prepare (double sampleRate, int maxBlock, int /*numChannels*/)
{
    sr = sampleRate;
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) maxBlock, 1 };
    midL.prepare (spec);  midR.prepare (spec);
    highL.prepare (spec); highR.prepare (spec);
    updateCoeffs();
    reset();
}

void FreshAir::reset()
{
    midL.reset(); midR.reset(); highL.reset(); highR.reset();
}

void FreshAir::updateCoeffs()
{
    const float midGain  = juce::Decibels::decibelsToGain (midAir  * 8.0f);
    const float highGain = juce::Decibels::decibelsToGain (highAir * 10.0f);
    auto mid  = juce::dsp::IIR::Coefficients<float>::makeHighShelf (sr, 8000.0f,  0.6f, midGain);
    auto high = juce::dsp::IIR::Coefficients<float>::makeHighShelf (sr, 15000.0f, 0.5f, highGain);
    *midL.coefficients = *mid;  *midR.coefficients = *mid;
    *highL.coefficients = *high; *highR.coefficients = *high;
}

void FreshAir::setParameters (bool enabled, float midAir01, float highAir01)
{
    on = enabled;
    midAir  = juce::jlimit (0.0f, 1.0f, midAir01);
    highAir = juce::jlimit (0.0f, 1.0f, highAir01);
    updateCoeffs();
}

void FreshAir::process (juce::AudioBuffer<float>& buffer)
{
    if (! on)
        return;

    const int numCh  = buffer.getNumChannels();
    const int numSmp = buffer.getNumSamples();

    for (int ch = 0; ch < numCh; ++ch)
    {
        auto* d = buffer.getWritePointer (ch);
        auto& m = (ch == 0) ? midL : midR;
        auto& h = (ch == 0) ? highL : highR;
        for (int i = 0; i < numSmp; ++i)
            d[i] = h.processSample (m.processSample (d[i]));
    }
}
