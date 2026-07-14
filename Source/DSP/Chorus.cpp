#include "Chorus.h"

void Chorus::prepare (double sampleRate, int maxBlock, int numChannels)
{
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) maxBlock, (juce::uint32) juce::jmax (1, numChannels) };
    chorus.prepare (spec);
    reset();
}

void Chorus::reset()
{
    chorus.reset();
}

void Chorus::setParameters (bool enabled, float rateHz, float depth01,
                            float centreDelayMs, float feedback, float mix01)
{
    on = enabled;
    chorus.setRate (juce::jlimit (0.0f, 20.0f, rateHz));
    chorus.setDepth (juce::jlimit (0.0f, 1.0f, depth01));
    chorus.setCentreDelay (juce::jlimit (1.0f, 100.0f, centreDelayMs));
    chorus.setFeedback (juce::jlimit (-0.95f, 0.95f, feedback));
    chorus.setMix (juce::jlimit (0.0f, 1.0f, mix01));
}

void Chorus::process (juce::AudioBuffer<float>& buffer)
{
    if (! on)
        return;

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    chorus.process (ctx);
}
