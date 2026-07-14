#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>

/**
    SmartTune — real-time pitch correction.

    Detects the input fundamental (fed from AIAnalyzer, or tracked internally),
    finds the nearest allowed scale note and pitch-shifts the signal toward it.
    Pitch shifting uses a dual-tap variable delay line (granular/overlap) so it
    works on monophonic vocals without an FFT.

    Parameters:
      - key / scale        (which notes are allowed)
      - retune speed       (0 = instant "hard" tune, 1 = slow/natural)
      - strength           (0..1 amount of correction applied)
      - formant preserve   (crude formant compensation)
*/
class SmartTune
{
public:
    enum class Scale { chromatic, major, minor, majorPentatonic, minorPentatonic };

    SmartTune();

    void prepare (double sampleRate, int maxBlock, int numChannels);
    void reset();
    void process (juce::AudioBuffer<float>& buffer);

    void setParameters (bool enabled, int keyIndex, Scale scale,
                        float retuneSpeed01, float strength01,
                        bool formantPreserve);

    /** Feed the analyser's pitch estimate (Hz). 0 = let SmartTune track. */
    void setDetectedF0 (float hz) noexcept { externalF0 = hz; }

    float getCurrentCents() const noexcept { return currentCents; }
    float getTargetHz()     const noexcept { return targetHz; }

private:
    float nearestScaleFreq (float f0) const;
    float trackPitch (const float* mono, int n);

    double sr = 44100.0;
    bool   on = true;
    int    key = 0;
    Scale  scale = Scale::chromatic;
    float  retune = 0.2f;
    float  strength = 1.0f;
    bool   formant = true;

    float  externalF0 = 0.0f;
    float  smoothedRatio = 1.0f;
    float  currentCents = 0.0f;
    float  targetHz = 0.0f;

    // pitch-shift delay line (per channel)
    std::vector<std::vector<float>> delayLines;
    int    writePos = 0;
    int    dlSize = 0;
    float  phase = 0.0f;             // 0..1 grain phase
    float  grainSamples = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SmartTune)
};
