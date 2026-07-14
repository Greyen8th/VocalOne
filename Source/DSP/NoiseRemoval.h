#pragma once

#include <JuceHeader.h>
#include <vector>

/**
    NoiseRemoval — spectral-subtraction broadband denoiser (RX-style).

    Works with an overlap-add STFT. A noise profile is learned automatically
    from the quietest frames, then subtracted from the magnitude spectrum with
    an over-subtraction factor and spectral floor to reduce musical noise.
*/
class NoiseRemoval
{
public:
    NoiseRemoval();

    void prepare (double sampleRate, int maxBlock, int numChannels);
    void reset();
    void process (juce::AudioBuffer<float>& buffer);

    void setParameters (bool enabled, float amountPct /*0..100*/, bool learn);
    void resetProfile();

private:
    void processChannel (float* data, int numSamples, int ch);
    void processFrame (int ch);

    static constexpr int fftOrder = 10;      // 1024
    static constexpr int fftSize  = 1 << fftOrder;
    static constexpr int hopSize  = fftSize / 4;

    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window { fftSize, juce::dsp::WindowingFunction<float>::hann };

    double sr = 44100.0;
    bool   on = true;
    float  amount = 0.5f;
    bool   learning = true;

    int    numChannels = 2;

    struct ChannelState
    {
        std::vector<float> inFifo, outFifo, overlap;
        std::vector<float> fftData;               // 2*fftSize
        int  fifoIndex = 0;
    };
    std::vector<ChannelState> chans;

    std::vector<float> noiseMag;                  // fftSize/2+1
    int   learnedFrames = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoiseRemoval)
};
