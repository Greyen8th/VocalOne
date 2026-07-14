#pragma once

#include <JuceHeader.h>
#include <array>

/**
    AIEqualizer — multiband vocal EQ with automatic problem detection.

    The analyser scans the spectrum and flags common vocal issues:
        Mud (200-400), Boxy (300-600), Nasal (900-1200), Harsh (2.5-4k),
        Thin (lack of 150-300), Boomy (60-120 excess), Air (10k+),
        Presence (3-6k).
    When auto mode is on it applies gentle corrective gains; the user can also
    drive the bands manually.
*/
class AIEqualizer
{
public:
    static constexpr int numBands = 8;

    enum Band { Boomy = 0, Mud, Boxy, Nasal, Presence, Harsh, Air, Thin };

    AIEqualizer() = default;

    void prepare (double sampleRate, int maxBlock, int numChannels);
    void reset();
    void process (juce::AudioBuffer<float>& buffer);

    void setEnabled (bool e) noexcept { on = e; }
    void setAuto (bool a)    noexcept { autoM = a; }
    void setManualGain (int band, float dB);
    void setAmount (float a01) noexcept { amount = juce::jlimit (0.0f, 1.0f, a01); }

    /** Report detected magnitude (0..1 severity) per named issue for the UI. */
    std::array<float, numBands> getDetection() const noexcept { return detection; }

private:
    struct BandDef { float freq; float q; };
    void updateCoefficients();
    void analyseSpectrum (const juce::AudioBuffer<float>& buffer);

    double sr = 44100.0;
    bool   on = true;
    bool   autoM = true;
    float  amount = 0.6f;

    std::array<BandDef, numBands> bands
    {{
        {   90.0f, 1.0f },   // Boomy
        {  300.0f, 1.0f },   // Mud
        {  450.0f, 1.2f },   // Boxy
        { 1050.0f, 1.5f },   // Nasal
        { 4500.0f, 0.8f },   // Presence (boost region)
        { 3200.0f, 2.0f },   // Harsh
        {12000.0f, 0.7f },   // Air (shelf-ish)
        {  220.0f, 0.8f }    // Thin (boost region)
    }};

    std::array<float, numBands> manualGain {};   // dB user offset
    std::array<float, numBands> autoGain {};      // dB computed
    std::array<float, numBands> detection {};     // 0..1 severity

    std::array<juce::dsp::IIR::Filter<float>, numBands> filtersL, filtersR;

    // analysis FFT
    static constexpr int fftOrder = 11;
    static constexpr int fftSize  = 1 << fftOrder;
    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window { fftSize, juce::dsp::WindowingFunction<float>::hann };
    std::vector<float> fftScratch, ring;
    int ringPos = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AIEqualizer)
};
