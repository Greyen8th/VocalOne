#include "StereoWidth.h"

void StereoWidth::prepare (double sampleRate, int maxBlock, int /*numChannels*/)
{
    sr = sampleRate;
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) maxBlock, 1 };
    lpForBass.prepare (spec);
    lpForBass.setType (juce::dsp::LinkwitzRileyFilterType::lowpass);
    lpForBass.setCutoffFrequency (bassMonoHz);
    sideLow.setSize (1, maxBlock);
    reset();
}

void StereoWidth::reset()
{
    lpForBass.reset();
}

void StereoWidth::setParameters (bool enabled, float widthV, float bassMonoHzV)
{
    on = enabled;
    width = juce::jlimit (0.0f, 2.0f, widthV);
    if (! juce::approximatelyEqual (bassMonoHz, bassMonoHzV))
    {
        bassMonoHz = bassMonoHzV;
        lpForBass.setCutoffFrequency (bassMonoHz);
    }
}

void StereoWidth::process (juce::AudioBuffer<float>& buffer)
{
    if (! on || buffer.getNumChannels() < 2)
        return;

    const int numSmp = buffer.getNumSamples();
    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getWritePointer (1);

    // Extract low band of the side signal to keep bass mono.
    sideLow.setSize (1, numSmp, false, false, true);
    auto* sl = sideLow.getWritePointer (0);
    for (int i = 0; i < numSmp; ++i)
        sl[i] = 0.5f * (L[i] - R[i]);

    {
        juce::dsp::AudioBlock<float> b (sideLow);
        juce::dsp::ProcessContextReplacing<float> ctx (b);
        lpForBass.process (ctx);
    }

    for (int i = 0; i < numSmp; ++i)
    {
        const float mid  = 0.5f * (L[i] + R[i]);
        float side = 0.5f * (L[i] - R[i]);

        // widen side, but restore the mono low band
        const float sideHigh = side - sl[i];
        side = sl[i] + sideHigh * width;

        L[i] = mid + side;
        R[i] = mid - side;
    }
}
