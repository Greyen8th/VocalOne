#include "AICompressor.h"

void AICompressor::prepare (double sampleRate, int /*maxBlock*/, int /*numChannels*/)
{
    sr = sampleRate;
    applyPreset (Mode::pop);
    reset();
}

void AICompressor::reset()
{
    envDb = -90.0f;
    grDb = 0.0f;
}

static void computeCoefs (float attackMs, float releaseMs, double sr, float& att, float& rel)
{
    att = std::exp (-1.0f / (juce::jmax (0.05f, attackMs)  * 0.001f * (float) sr));
    rel = std::exp (-1.0f / (juce::jmax (1.0f,  releaseMs) * 0.001f * (float) sr));
}

void AICompressor::applyPreset (Mode m)
{
    switch (m)
    {
        case Mode::rap:       threshold = -18.0f; ratio = 4.0f; knee = 4.0f; makeup = 4.0f; computeCoefs (3.0f, 80.0f,  sr, attCoef, relCoef); detectedMode = 1; break;
        case Mode::pop:       threshold = -20.0f; ratio = 3.0f; knee = 6.0f; makeup = 3.0f; computeCoefs (8.0f, 120.0f, sr, attCoef, relCoef); detectedMode = 2; break;
        case Mode::rock:      threshold = -16.0f; ratio = 5.0f; knee = 3.0f; makeup = 4.0f; computeCoefs (2.0f, 60.0f,  sr, attCoef, relCoef); detectedMode = 3; break;
        case Mode::podcast:   threshold = -24.0f; ratio = 3.5f; knee = 8.0f; makeup = 5.0f; computeCoefs (12.0f, 150.0f,sr, attCoef, relCoef); detectedMode = 4; break;
        case Mode::voiceOver: threshold = -22.0f; ratio = 2.5f; knee = 10.0f;makeup = 4.0f; computeCoefs (15.0f, 200.0f,sr, attCoef, relCoef); detectedMode = 5; break;
        case Mode::manual:    detectedMode = 0; break;
        case Mode::autoDetect:default: break;
    }
}

void AICompressor::setManual (float thresholdDb, float ratioV, float attackMs,
                              float releaseMs, float kneeDb, float makeupDb)
{
    threshold = thresholdDb;
    ratio = juce::jmax (1.0f, ratioV);
    knee = juce::jmax (0.0f, kneeDb);
    makeup = makeupDb;
    computeCoefs (attackMs, releaseMs, sr, attCoef, relCoef);
}

void AICompressor::process (juce::AudioBuffer<float>& buffer)
{
    if (! on)
    {
        grDb = 0.0f;
        return;
    }

    if (mode == Mode::autoDetect)
    {
        // Choose style from dynamics + tempo hints.
        Mode chosen = Mode::pop;
        if (hintBpm > 140.0f && hintCrest > 12.0f)      chosen = Mode::rap;
        else if (hintCrest > 16.0f)                      chosen = Mode::rock;
        else if (hintBpm < 100.0f && hintCrest < 10.0f)  chosen = Mode::podcast;
        else if (hintBpm < 90.0f)                        chosen = Mode::voiceOver;
        applyPreset (chosen);
    }
    else if (mode != Mode::manual)
    {
        applyPreset (mode);
    }

    const int numCh  = buffer.getNumChannels();
    const int numSmp = buffer.getNumSamples();
    float maxGr = 0.0f;

    for (int i = 0; i < numSmp; ++i)
    {
        float in = 0.0f;
        for (int ch = 0; ch < numCh; ++ch)
            in = juce::jmax (in, std::abs (buffer.getReadPointer (ch)[i]));

        const float inDb = juce::Decibels::gainToDecibels (in, -90.0f);
        const float coef = (inDb > envDb) ? attCoef : relCoef;
        envDb = inDb + (envDb - inDb) * coef;

        // Soft-knee static curve.
        float overDb = envDb - threshold;
        float grThisDb = 0.0f;
        if (overDb > knee * 0.5f)
            grThisDb = overDb * (1.0f - 1.0f / ratio);
        else if (overDb > -knee * 0.5f)
        {
            const float x = overDb + knee * 0.5f;
            grThisDb = (1.0f - 1.0f / ratio) * (x * x) / (2.0f * knee);
        }

        maxGr = juce::jmax (maxGr, grThisDb);
        const float g = juce::Decibels::decibelsToGain (-grThisDb + makeup);

        for (int ch = 0; ch < numCh; ++ch)
            buffer.getWritePointer (ch)[i] *= g;
    }

    grDb = maxGr;
}
