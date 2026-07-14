#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>
#include <random>

/**
    SmartTune Pro — real-time pitch correction with Antares-level quality.

    Architecture:
      1. YIN-based pitch tracker (with frame accumulation for robustness)
      2. 4-grain granular pitch shifter with PSOLA-like overlap-add
      3. Independent formant preservation using LPC-based formant shifting
      4. Vibrato detection and preservation (keeps natural vibrato intact)
      5. Transient detection with reduced correction near transients
      6. Humanize: adds micro-delays and slight randomness
      7. Extended scales: 15+ modes including jazz, modal, arabic, etc.

    Parameters:
      - key / scale        (which notes are allowed, 15+ scale types)
      - retune speed       (0 = instant hard-tune, 1 = very slow natural)
      - strength           (0..1 amount of correction applied)
      - formant preserve   (true = preserve vocal character during shifts)
      - vibrato preserve   (true = keep natural vibrato)
      - humanize           (0..1 adds timing and pitch randomness)
      - tight/natural/loose (correction curve profile)
*/
class SmartTune
{
public:
    enum class Scale
    {
        chromatic = 0,
        major, minor,
        majorPentatonic, minorPentatonic,
        blues, dorian, phrygian, lydian, mixolydian,
        harmonicMinor, melodicMinor,
        diminished, wholeTone,
        arabic, japanese,
        scaleCount
    };

    enum class Profile
    {
        tight = 0,   // fast, robotic (T-Pain style)
        natural,     // medium, transparent
        loose        // slow, barely noticeable
    };

    SmartTune();

    void prepare (double sampleRate, int maxBlock, int numChannels);
    void reset();
    void process (juce::AudioBuffer<float>& buffer);

    void setParameters (bool enabled, int keyIndex, Scale scale,
                        float retuneSpeed01, float strength01,
                        bool formantPreserve, bool vibratoPreserve = true,
                        float humanize01 = 0.0f, Profile profile = Profile::natural);

    /** Feed the analyser's pitch estimate (Hz). 0 = let SmartTune track internally. */
    void setDetectedF0 (float hz) noexcept { externalF0 = hz; }

    float getCurrentCents() const noexcept { return currentCents; }
    float getTargetHz()     const noexcept { return targetHz; }
    float getDetectedF0()   const noexcept { return lastDetectedF0; }
    bool  isVibratoActive() const noexcept { return vibratoActive; }

private:
    // Pitch detection
    float nearestScaleFreq (float f0) const;
    float trackPitchYIN (const float* mono, int n);
    float detectVibrato (float f0History[32], int count);

    // Granular pitch shifting
    void pitchShiftGranular (juce::AudioBuffer<float>& buffer, float ratio);
    float readDelayLine (int ch, float delaySamples);
    void writeDelayLine (int ch, float sample);
    float hannWindow (float phase01); // phase 0..1

    // Formant preservation (LPC-based, lightweight)
    void applyFormantPreserve (juce::AudioBuffer<float>& buffer, float pitchRatio);
    void computeLPC (const float* samples, int n, float* coeffs, int order);
    void lpcFilter (float* samples, int n, const float* coeffs, int order,
                    float* state, float pitchRatio);

    // Transient detection
    float detectTransient (const float* mono, int n);

    // Humanize / randomization
    float getHumanizeOffset();

    // Sample rate & config
    double sr = 44100.0;
    bool   on = true;
    int    key = 0;
    Scale  scale = Scale::chromatic;
    float  retune = 0.2f;
    float  strength = 1.0f;
    bool   formant = true;
    bool   vibrato = true;
    float  humanize = 0.0f;
    Profile profile = Profile::natural;

    // Pitch tracking state
    float  externalF0 = 0.0f;
    float  lastDetectedF0 = 0.0f;
    float  smoothedRatio = 1.0f;
    float  currentCents = 0.0f;
    float  targetHz = 0.0f;
    float  f0History[32] = {};
    int    f0HistoryWrite = 0;
    bool   vibratoActive = false;
    float  vibratoDepth = 0.0f;
    float  vibratoRate = 0.0f;

    // Granular pitch shifter state
    std::vector<std::vector<float>> delayLines;
    int    writePos = 0;
    int    dlSize = 0;
    float  grainSamples = 0.0f;
    int    numGrains = 4;

    struct Grain
    {
        float phase = 0.0f;      // 0..1 position within grain
        float readOffset = 0.0f; // delay offset for this grain
        bool  active = false;
    };
    std::vector<std::vector<Grain>> grains; // [channel][grain]

    // Transient detection
    float lastEnv = 0.0f;
    float transientFactor = 0.0f; // 0 = no transient, 1 = strong transient

    // Humanize
    std::mt19937 rng;
    std::uniform_real_distribution<float> dist;
    float humanizePhase = 0.0f;

    // Formant preservation (LPC states per channel)
    static constexpr int lpcOrder = 8;
    std::vector<std::vector<float>> lpcStates; // [channel][order]

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SmartTune)
};
