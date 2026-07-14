#pragma once

#include <JuceHeader.h>

/**
    AICompressor — program-dependent vocal compressor.

    Detects the delivery style (Rap / Pop / Rock / Podcast / Voice-Over) from
    dynamics + tempo hints and selects an appropriate attack/release/ratio when
    in auto mode. Feed-forward peak/RMS hybrid detector with soft knee and
    automatic make-up gain.
*/
class AICompressor
{
public:
    enum class Mode { autoDetect, rap, pop, rock, podcast, voiceOver, manual };

    AICompressor() = default;

    void prepare (double sampleRate, int maxBlock, int numChannels);
    void reset();
    void process (juce::AudioBuffer<float>& buffer);

    void setEnabled (bool e) noexcept { on = e; }
    void setMode (Mode m) noexcept    { mode = m; }
    void setManual (float thresholdDb, float ratio, float attackMs,
                    float releaseMs, float kneeDb, float makeupDb);

    /** Hints from the analyser for auto-detection. */
    void setHints (float crestDb, float bpm) noexcept { hintCrest = crestDb; hintBpm = bpm; }

    float getGainReductionDb() const noexcept { return grDb; }
    int   getDetectedMode()    const noexcept { return detectedMode; }

private:
    void applyPreset (Mode m);

    double sr = 44100.0;
    bool   on = true;
    Mode   mode = Mode::autoDetect;
    int    detectedMode = 1;

    float  threshold = -20.0f;
    float  ratio = 3.0f;
    float  knee = 6.0f;
    float  makeup = 0.0f;
    float  attCoef = 0.0f, relCoef = 0.0f;

    float  hintCrest = 12.0f;
    float  hintBpm = 120.0f;

    float  envDb = -90.0f;
    float  grDb = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AICompressor)
};
