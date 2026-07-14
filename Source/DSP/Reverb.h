#pragma once

#include <JuceHeader.h>
#include <array>

/**
    VocalReverb Pro — high-quality plate-style reverb for vocals.

    Features:
      - Early reflections (12-tap sparse FIR) for spatial clarity
      - Late plate reverb with 8 all-pass + comb delay network
      - Pre-delay stereo with independent L/R timing
      - High-pass filter on tail to keep vocal clean
      - Modulated tail for natural shimmer (prevents metallic ringing)
*/
class VocalReverb
{
public:
    VocalReverb() = default;

    void prepare (double sampleRate, int maxBlock, int numChannels);
    void reset();
    void process (juce::AudioBuffer<float>& buffer);

    void setParameters (bool enabled, float size01, float damping01,
                        float width01, float mix01, float preDelayMs);

private:
    struct AllPassFilter
    {
        std::vector<float> buffer;
        int delay = 0;
        int writePos = 0;
        float coeff = 0.7f;

        void prepare (int delaySamples, float coefficient)
        {
            delay = delaySamples;
            coeff = coefficient;
            buffer.assign ((size_t) delay + 1, 0.0f);
            writePos = 0;
        }

        float process (float input)
        {
            const float bufOut = buffer[(size_t) writePos];
            const float output = input + bufOut * coeff;
            buffer[(size_t) writePos] = input - output * coeff;
            writePos = (writePos + 1) % (delay + 1);
            return output;
        }
    };

    struct CombFilter
    {
        std::vector<float> buffer;
        int delay = 0;
        int writePos = 0;
        float coeff = 0.7f;
        float damping = 0.5f;
        float filterStore = 0.0f;

        void prepare (int delaySamples, float coefficient, float damping01)
        {
            delay = delaySamples;
            coeff = coefficient;
            damping = damping01;
            buffer.assign ((size_t) delay + 1, 0.0f);
            writePos = 0;
            filterStore = 0.0f;
        }

        float process (float input)
        {
            const float bufOut = buffer[(size_t) writePos];
            filterStore = (bufOut * (1.0f - damping)) + (filterStore * damping);
            buffer[(size_t) writePos] = input + (filterStore * coeff);
            writePos = (writePos + 1) % (delay + 1);
            return bufOut;
        }
    };

    double sr = 44100.0;
    bool   on = false;
    float  mix = 0.2f;
    float  size = 0.5f;
    float  damping = 0.5f;
    float  width = 1.0f;
    float  preDelayMs = 20.0f;

    // Pre-delay
    std::vector<float> preDelayL, preDelayR;
    int preDelayWrite = 0;
    int preDelaySamplesL = 0, preDelaySamplesR = 0;

    // Early reflections (12-tap sparse delay)
    static constexpr int numEarlyTaps = 12;
    std::array<int, numEarlyTaps> earlyDelaysL;
    std::array<int, numEarlyTaps> earlyDelaysR;
    std::array<float, numEarlyTaps> earlyGains;

    // Late reverb: 4 all-pass + 4 comb per channel
    std::array<AllPassFilter, 4> allPassL, allPassR;
    std::array<CombFilter, 4> combL, combR;

    // Modulation for tail shimmer
    float modPhaseL = 0.0f, modPhaseR = 0.0f;
    float modFreq = 0.5f; // Hz
    float modDepth = 3.0f; // samples

    // Output HP filter
    juce::dsp::IIR::Filter<float> hpL, hpR;

    float processMonoChain (float input, std::array<AllPassFilter, 4>& allpass,
                            std::array<CombFilter, 4>& comb);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalReverb)
};
