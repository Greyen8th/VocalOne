#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <array>

/**
    AIAnalyzer

    Continuously inspects the incoming vocal signal and derives a set of
    high-level descriptors that the rest of the plugin (and the AI assistant)
    use to make automatic decisions:

      - musical key / tonality      (chroma based)
      - estimated tempo (BPM)       (envelope autocorrelation)
      - genre / gender guesses      (spectral heuristics)
      - vocal range (min/max f0)    (pitch tracking)
      - loudness / RMS / peak
      - noise floor estimate
      - breath activity
      - dynamic range
      - sibilance energy (5-9 kHz)
      - mic distance estimate       (LF proximity vs HF ratio)

    The analysis is intentionally lightweight so it can run in real time on the
    audio thread; results are published through lock-free atomics that the GUI
    thread reads.
*/
class AIAnalyzer
{
public:
    AIAnalyzer();
    ~AIAnalyzer() = default;

    void prepare (double sampleRate, int maximumBlockSize, int numChannels);
    void reset();

    /** Analyse a block (read-only, never modifies the audio). */
    void analyze (const juce::AudioBuffer<float>& buffer);

    // -----------------------------------------------------------------
    // Results (thread-safe getters)
    // -----------------------------------------------------------------
    struct Snapshot
    {
        int    keyIndex        = 0;      // 0..11  (C..B)
        bool   isMinor         = false;
        float  bpm             = 120.0f;
        float  rmsDb           = -60.0f;
        float  peakDb          = -60.0f;
        float  crestDb         = 0.0f;   // peak - rms (dynamics)
        float  noiseFloorDb    = -90.0f;
        float  sibilanceDb     = -90.0f;
        float  breathiness     = 0.0f;   // 0..1
        float  f0              = 0.0f;   // current pitch (Hz)
        float  f0Min           = 0.0f;
        float  f0Max           = 0.0f;
        float  micDistance     = 0.5f;   // 0 near .. 1 far
        float  brightness      = 0.5f;   // spectral centroid normalised
        int    genderGuess     = 0;      // 0 unknown, 1 male, 2 female
        int    genreGuess      = 0;      // index into genre table
    };

    Snapshot getSnapshot() const noexcept;

    static juce::String keyName (int keyIndex, bool isMinor);
    static juce::String genreName (int index);
    static juce::String genderName (int index);

    // Give other modules cheap access to a couple of frequently used values.
    float getCurrentF0()      const noexcept { return f0Hz.load(); }
    float getSibilanceDb()    const noexcept { return sibDb.load(); }
    float getRmsDb()          const noexcept { return rmsDbAtomic.load(); }
    float getNoiseFloorDb()   const noexcept { return noiseDb.load(); }

private:
    void   updateChroma (const float* mono, int numSamples);
    float  detectPitchYIN (const float* mono, int numSamples);
    void   updateBandEnergies (const float* mono, int numSamples);

    double sr = 44100.0;
    int    blockSize = 512;

    // Running statistics
    float  runningRms      = 0.0f;
    float  runningPeak     = 0.0f;
    float  noiseFloor      = 1.0e-6f;
    float  centroid        = 0.5f;
    float  f0MinRunning    = 0.0f;
    float  f0MaxRunning    = 0.0f;

    // Simple onset/tempo tracking
    std::array<float, 512> onsetEnv {};
    int    onsetWrite = 0;
    float  prevFluxMag = 0.0f;

    // Chroma accumulator (12 pitch classes)
    std::array<float, 12> chroma {};

    // FFT for spectral features
    static constexpr int fftOrder = 11;              // 2048
    static constexpr int fftSize  = 1 << fftOrder;
    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window { fftSize, juce::dsp::WindowingFunction<float>::hann };
    std::vector<float> fftScratch;                   // 2*fftSize
    std::vector<float> monoAccum;                    // ring of samples for FFT
    int monoAccumWrite = 0;

    // Published atomics
    std::atomic<float> f0Hz        { 0.0f };
    std::atomic<float> sibDb       { -90.0f };
    std::atomic<float> rmsDbAtomic { -60.0f };
    std::atomic<float> noiseDb     { -90.0f };

    // Full snapshot guarded by a spinlock-free double buffer
    mutable juce::SpinLock snapshotLock;
    Snapshot current;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AIAnalyzer)
};
