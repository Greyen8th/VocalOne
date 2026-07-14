#pragma once

#include <JuceHeader.h>

/**
    Saturation — 5 selectable analog-style saturation models with 4x oversampling.

    Models: Tube, Tape, Vintage, Warm, Digital. Each uses a distinct waveshaper
    plus tone shaping. 4x oversampling keeps aliasing very low even at high drive.
    
    Updated with improved anti-aliasing and tone control per model.
*/
class Saturation
{
public:
    enum class Model { tube = 0, tape, vintage, warm, digital };

    Saturation() = default;

    void prepare (double sampleRate, int maxBlock, int numChannels);
    void reset();
    void process (juce::AudioBuffer<float>& buffer);

    void setParameters (bool enabled, Model model, float drive01, float mix01);

private:
    float shape (float x) const;
    float tubeShape (float x) const;
    float tapeShape (float x) const;
    float vintageShape (float x) const;
    float warmShape (float x) const;
    float digitalShape (float x) const;

    double sr = 44100.0;
    bool   on = true;
    Model  model = Model::tube;
    float  drive = 0.3f;
    float  mix = 1.0f;

    // 4x oversampling with better filter (JUCE's polyphase IIR)
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    juce::dsp::IIR::Filter<float> toneL, toneR;
    float toneGain = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Saturation)
};
