#include "SmartTune.h"

SmartTune::SmartTune()
    : rng (std::random_device{}()), dist (-1.0f, 1.0f)
{
}

void SmartTune::prepare (double sampleRate, int /*maxBlock*/, int numChannels)
{
    sr = sampleRate;

    // 150 ms delay line for pitch shifting (bigger than old 100ms for better quality)
    dlSize = juce::nextPowerOfTwo ((int) (sr * 0.150));
    delayLines.assign ((size_t) juce::jmax (1, numChannels),
                       std::vector<float> ((size_t) dlSize, 0.0f));

    // 40 ms grains for higher quality overlap-add
    grainSamples = (float) (sr * 0.040);

    // Initialize 4 grains per channel
    grains.assign ((size_t) juce::jmax (1, numChannels),
                   std::vector<Grain> ((size_t) numGrains));
    for (size_t ch = 0; ch < grains.size(); ++ch)
    {
        for (size_t g = 0; g < (size_t) numGrains; ++g)
        {
            grains[ch][g].phase = (float) g / (float) numGrains;
            grains[ch][g].readOffset = 0.0f;
            grains[ch][g].active = true;
        }
    }

    // LPC states for formant preservation
    lpcStates.assign ((size_t) juce::jmax (1, numChannels),
                      std::vector<float> ((size_t) lpcOrder, 0.0f));

    reset();
}

void SmartTune::reset()
{
    for (auto& dl : delayLines)
        std::fill (dl.begin(), dl.end(), 0.0f);
    writePos = 0;
    smoothedRatio = 1.0f;
    currentCents = 0.0f;
    f0HistoryWrite = 0;
    std::fill (std::begin (f0History), std::end (f0History), 0.0f);
    vibratoActive = false;
    vibratoDepth = 0.0f;
    vibratoRate = 0.0f;
    lastEnv = 0.0f;
    transientFactor = 0.0f;
    humanizePhase = 0.0f;

    for (auto& chGrains : grains)
    {
        for (size_t g = 0; g < chGrains.size(); ++g)
        {
            chGrains[g].phase = (float) g / (float) chGrains.size();
            chGrains[g].readOffset = 0.0f;
            chGrains[g].active = true;
        }
    }

    for (auto& st : lpcStates)
        std::fill (st.begin(), st.end(), 0.0f);
}

void SmartTune::setParameters (bool enabled, int keyIndex, Scale scaleV,
                             float retuneSpeed01, float strength01,
                             bool formantPreserve, bool vibratoPreserve,
                             float humanize01, Profile profileV)
{
    on = enabled;
    key = ((keyIndex % 12) + 12) % 12;
    scale = scaleV;
    retune = juce::jlimit (0.0f, 1.0f, retuneSpeed01);
    strength = juce::jlimit (0.0f, 1.0f, strength01);
    formant = formantPreserve;
    vibrato = vibratoPreserve;
    humanize = juce::jlimit (0.0f, 1.0f, humanize01);
    profile = profileV;
}

//==============================================================================
// Scale / Key logic
//==============================================================================

float SmartTune::nearestScaleFreq (float f0) const
{
    if (f0 <= 0.0f)
        return f0;

    const float midi = 69.0f + 12.0f * std::log2 (f0 / 440.0f);

    // Scale interval tables (semitones relative to root)
    static const int chromatic[]      = { 0,1,2,3,4,5,6,7,8,9,10,11 };
    static const int major[]          = { 0,2,4,5,7,9,11 };
    static const int minor[]          = { 0,2,3,5,7,8,10 };
    static const int majPent[]        = { 0,2,4,7,9 };
    static const int minPent[]        = { 0,3,5,7,10 };
    static const int bluesScale[]     = { 0,3,5,6,7,10 };
    static const int dorian[]         = { 0,2,3,5,7,9,10 };
    static const int phrygian[]       = { 0,1,3,5,7,8,10 };
    static const int lydian[]         = { 0,2,4,6,7,9,11 };
    static const int mixolydian[]     = { 0,2,4,5,7,9,10 };
    static const int harmonicMinor[]  = { 0,2,3,5,7,8,11 };
    static const int melodicMinor[]   = { 0,2,3,5,7,9,11 };
    static const int diminished[]     = { 0,2,3,5,6,8,9,11 };
    static const int wholeTone[]     = { 0,2,4,6,8,10 };
    static const int arabic[]         = { 0,1,4,5,7,8,11 };
    static const int japanese[]       = { 0,1,5,7,10 };

    const int* degrees = nullptr;
    int degreeCount = 0;

    switch (scale)
    {
        case Scale::major:            degrees = major;          degreeCount = 7; break;
        case Scale::minor:            degrees = minor;          degreeCount = 7; break;
        case Scale::majorPentatonic:  degrees = majPent;        degreeCount = 5; break;
        case Scale::minorPentatonic:  degrees = minPent;        degreeCount = 5; break;
        case Scale::blues:            degrees = bluesScale;     degreeCount = 6; break;
        case Scale::dorian:           degrees = dorian;         degreeCount = 7; break;
        case Scale::phrygian:         degrees = phrygian;       degreeCount = 7; break;
        case Scale::lydian:           degrees = lydian;         degreeCount = 7; break;
        case Scale::mixolydian:       degrees = mixolydian;     degreeCount = 7; break;
        case Scale::harmonicMinor:    degrees = harmonicMinor;  degreeCount = 7; break;
        case Scale::melodicMinor:     degrees = melodicMinor;   degreeCount = 7; break;
        case Scale::diminished:       degrees = diminished;     degreeCount = 8; break;
        case Scale::wholeTone:        degrees = wholeTone;      degreeCount = 6; break;
        case Scale::arabic:           degrees = arabic;         degreeCount = 7; break;
        case Scale::japanese:         degrees = japanese;       degreeCount = 5; break;
        case Scale::chromatic:
        default:                      degrees = chromatic;      degreeCount = 12; break;
    }

    float best = midi;
    float bestDist = 1.0e9f;
    const int baseOct = (int) std::floor ((midi - key) / 12.0f) - 2;

    for (int oct = baseOct; oct <= baseOct + 3; ++oct)
    {
        for (int d = 0; d < degreeCount; ++d)
        {
            const float candidate = (float) (key + degrees[d]) + 12.0f * (float) oct;
            const float dist = std::abs (candidate - midi);
            if (dist < bestDist) { bestDist = dist; best = candidate; }
        }
    }

    return 440.0f * std::pow (2.0f, (best - 69.0f) / 12.0f);
}

//==============================================================================
// YIN Pitch Tracker (improved)
//==============================================================================

float SmartTune::trackPitchYIN (const float* mono, int n)
{
    const int minLag = juce::jmax (2, (int) (sr / 1200.0));   // up to 1200 Hz
    const int maxLag = juce::jmin (n - 1, (int) (sr / 50.0)); // down to 50 Hz
    if (maxLag <= minLag) return 0.0f;

    // Compute energy once
    float energy = 1.0e-9f;
    for (int i = 0; i < n; ++i) energy += mono[i] * mono[i];

    // Difference function d(tau)
    std::vector<float> d ((size_t) maxLag + 1, 0.0f);
    for (int tau = minLag; tau <= maxLag; ++tau)
    {
        float sum = 0.0f;
        for (int i = 0; i + tau < n; ++i)
        {
            const float diff = mono[i] - mono[i + tau];
            sum += diff * diff;
        }
        d[(size_t) tau] = sum;
    }

    // Cumulative mean normalized difference
    std::vector<float> cmnd ((size_t) maxLag + 1, 1.0f);
    float running = 0.0f;
    for (int tau = minLag; tau <= maxLag; ++tau)
    {
        running += d[(size_t) tau];
        if (running > 1.0e-9f)
            cmnd[(size_t) tau] = d[(size_t) tau] * (float) (tau - minLag + 1) / running;
    }

    // Search for first valley below threshold
    const float threshold = 0.12f; // slightly stricter than 0.15
    int bestTau = -1;
    for (int tau = minLag + 1; tau < maxLag - 1; ++tau)
    {
        if (cmnd[(size_t) tau] < threshold &&
            cmnd[(size_t) tau] < cmnd[(size_t) (tau - 1)] &&
            cmnd[(size_t) tau] < cmnd[(size_t) (tau + 1)])
        {
            bestTau = tau;
            break;
        }
    }

    // If no valley found, pick global minimum
    if (bestTau < 0)
    {
        float best = 1.0e9f;
        for (int tau = minLag; tau <= maxLag; ++tau)
        {
            if (cmnd[(size_t) tau] < best) { best = cmnd[(size_t) tau]; bestTau = tau; }
        }
    }
    if (bestTau <= 0) return 0.0f;

    // Parabolic interpolation for sub-sample accuracy
    float betterTau = (float) bestTau;
    if (bestTau > minLag && bestTau < maxLag)
    {
        const float s0 = cmnd[(size_t) (bestTau - 1)];
        const float s1 = cmnd[(size_t) bestTau];
        const float s2 = cmnd[(size_t) (bestTau + 1)];
        const float denom = 2.0f * (2.0f * s1 - s2 - s0);
        if (std::abs (denom) > 1.0e-9f)
            betterTau = (float) bestTau + (s2 - s0) / denom;
    }

    // Verify with energy ratio to reject unvoiced
    const float f0 = (float) sr / betterTau;
    if (f0 < 50.0f || f0 > 1200.0f) return 0.0f;

    // Calculate aperiodicity to reject noisy frames
    const float aperiodicity = cmnd[(size_t) bestTau] / (energy + 1.0e-9f);
    if (aperiodicity > 0.5f) return 0.0f; // too noisy, likely unvoiced

    return f0;
}

//==============================================================================
// Vibrato detection
//==============================================================================

float SmartTune::detectVibrato (float f0HistoryIn[32], int count)
{
    if (count < 16) return 0.0f;

    // Calculate mean and variance of f0
    float mean = 0.0f;
    for (int i = 0; i < count; ++i) mean += f0HistoryIn[i];
    mean /= (float) count;

    if (mean < 1.0f) return 0.0f;

    float variance = 0.0f;
    for (int i = 0; i < count; ++i)
    {
        const float d = f0HistoryIn[i] - mean;
        variance += d * d;
    }
    variance /= (float) count;

    // Vibrato depth in cents
    const float stdDevCents = 1200.0f * std::log2 (1.0f + std::sqrt (variance) / mean);
    return stdDevCents;
}

//==============================================================================
// Transient detection
//==============================================================================

float SmartTune::detectTransient (const float* mono, int n)
{
    float env = 0.0f;
    for (int i = 0; i < n; ++i)
    {
        const float absS = std::abs (mono[i]);
        env = env * 0.95f + absS * 0.05f; // fast attack envelope
    }

    const float diff = env - lastEnv;
    lastEnv = env;

    if (diff > 0.01f)
        return juce::jlimit (0.0f, 1.0f, diff * 5.0f); // strong transient

    return 0.0f;
}

//==============================================================================
// Humanize
//==============================================================================

float SmartTune::getHumanizeOffset()
{
    if (humanize <= 0.001f) return 0.0f;
    return dist (rng) * humanize * 0.5f; // +/- 0.5 cents max
}

//==============================================================================
// Delay line helpers
//==============================================================================

float SmartTune::readDelayLine (int ch, float delaySamples)
{
    float rp = (float) writePos - delaySamples;
    while (rp < 0.0f) rp += (float) dlSize;
    while (rp >= (float) dlSize) rp -= (float) dlSize;

    const int i0 = (int) rp;
    const int i1 = (i0 + 1) % dlSize;
    const float frac = rp - (float) i0;
    const auto& dl = delayLines[(size_t) ch];
    return dl[(size_t) i0] + frac * (dl[(size_t) i1] - dl[(size_t) i0]);
}

void SmartTune::writeDelayLine (int ch, float sample)
{
    delayLines[(size_t) ch][(size_t) writePos] = sample;
}

float SmartTune::hannWindow (float phase01)
{
    return 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * juce::jlimit (0.0f, 1.0f, phase01));
}

//==============================================================================
// Granular Pitch Shifter (4-grain PSOLA-like)
//==============================================================================

void SmartTune::pitchShiftGranular (juce::AudioBuffer<float>& buffer, float ratio)
{
    const int numCh = buffer.getNumChannels();
    const int numSmp = buffer.getNumSamples();

    for (int i = 0; i < numSmp; ++i)
    {
        for (int ch = 0; ch < numCh && ch < (int) delayLines.size(); ++ch)
        {
            writeDelayLine (ch, buffer.getReadPointer (ch)[i]);
        }

        for (int ch = 0; ch < numCh && ch < (int) grains.size(); ++ch)
        {
            float out = 0.0f;
            float weightSum = 0.0f;

            for (size_t g = 0; g < grains[ch].size(); ++g)
            {
                auto& grain = grains[ch][g];
                if (! grain.active) continue;

                // Read position is behind write pointer by grain offset
                const float delay = grain.phase * grainSamples * (ratio - 1.0f);
                const float sample = readDelayLine (ch, delay);
                const float w = hannWindow (grain.phase);

                out += sample * w;
                weightSum += w;

                // Advance grain phase
                grain.phase += (ratio - 1.0f) / grainSamples;
                if (grain.phase >= 1.0f)
                {
                    grain.phase -= 1.0f;
                    grain.readOffset = 0.0f; // reset offset
                }
                if (grain.phase < 0.0f) grain.phase += 1.0f;
            }

            if (weightSum > 1.0e-6f)
                out /= weightSum;

            buffer.getWritePointer (ch)[i] = out;
        }

        writePos = (writePos + 1) % dlSize;
    }
}

//==============================================================================
// LPC Formant Preservation (lightweight)
//==============================================================================

void SmartTune::computeLPC (const float* samples, int n, float* coeffs, int order)
{
    // Levinson-Durbin recursion for LPC coefficients
    std::vector<float> r ((size_t) order + 1, 0.0f);

    // Autocorrelation
    for (int i = 0; i <= order; ++i)
    {
        float sum = 0.0f;
        for (int j = 0; j + i < n; ++j)
            sum += samples[j] * samples[j + i];
        r[(size_t) i] = sum;
    }

    // Levinson-Durbin
    std::vector<float> a ((size_t) order + 1, 0.0f);
    std::vector<float> refl ((size_t) order, 0.0f);
    a[0] = 1.0f;
    float error = r[0];

    for (int i = 0; i < order; ++i)
    {
        float k = 0.0f;
        for (int j = 0; j <= i; ++j)
            k += a[(size_t) j] * r[(size_t) (i + 1 - j)];

        if (std::abs (error) < 1.0e-12f) { k = 0.0f; }
        else { k = -k / error; }

        refl[(size_t) i] = k;

        for (int j = (i + 1) / 2; j >= 0; --j)
        {
            const float aj = a[(size_t) j];
            const float aipj = a[(size_t) (i + 1 - j)];
            a[(size_t) j] = aj + k * aipj;
            if (j != i + 1 - j)
                a[(size_t) (i + 1 - j)] = aipj + k * aj;
        }

        error = error * (1.0f - k * k);
    }

    for (int i = 0; i < order; ++i)
        coeffs[i] = a[(size_t) (i + 1)];
}

void SmartTune::lpcFilter (float* samples, int n, const float* coeffs, int order,
                           float* state, float pitchRatio)
{
    // Pitch-scaled LPC: shift formants back toward original when pitchRatio != 1
    // Simple approach: apply an all-pole filter with modified coefficients
    const float formantShift = 1.0f / std::sqrt (pitchRatio); // approximate formant scaling

    for (int i = 0; i < n; ++i)
    {
        float input = samples[i];
        float output = input;
        for (int j = 0; j < order; ++j)
        {
            output -= coeffs[j] * state[j] * formantShift;
        }
        // Shift state
        for (int j = order - 1; j > 0; --j)
            state[j] = state[j - 1];
        state[0] = output;
        samples[i] = output;
    }
}

void SmartTune::applyFormantPreserve (juce::AudioBuffer<float>& buffer, float pitchRatio)
{
    if (! formant || std::abs (pitchRatio - 1.0f) < 0.01f) return;

    const int numCh = buffer.getNumChannels();
    const int numSmp = buffer.getNumSamples();

    for (int ch = 0; ch < numCh && ch < (int) lpcStates.size(); ++ch)
    {
        auto* ptr = buffer.getWritePointer (ch);
        std::vector<float> coeffs ((size_t) lpcOrder, 0.0f);
        computeLPC (ptr, numSmp, coeffs.data(), lpcOrder);
        lpcFilter (ptr, numSmp, coeffs.data(), lpcOrder, lpcStates[(size_t) ch].data(), pitchRatio);
    }
}

//==============================================================================
// Main Process
//==============================================================================

void SmartTune::process (juce::AudioBuffer<float>& buffer)
{
    const int numCh  = buffer.getNumChannels();
    const int numSmp = buffer.getNumSamples();

    if (! on || numSmp <= 0)
        return;

    // Build mono for pitch tracking and transient detection
    std::vector<float> mono ((size_t) numSmp);
    for (int i = 0; i < numSmp; ++i)
    {
        float s = 0.0f;
        for (int ch = 0; ch < numCh; ++ch)
            s += buffer.getReadPointer (ch)[i];
        mono[(size_t) i] = s / (float) numCh;
    }

    // Detect transient
    transientFactor = detectTransient (mono.data(), numSmp);

    // Get pitch
    float f0 = externalF0 > 0.0f ? externalF0 : trackPitchYIN (mono.data(), numSmp);
    lastDetectedF0 = f0;

    // Update F0 history for vibrato detection
    if (f0 > 50.0f && f0 < 1200.0f)
    {
        f0History[f0HistoryWrite] = f0;
        f0HistoryWrite = (f0HistoryWrite + 1) % 32;

        float vibratoCents = detectVibrato (f0History, 32);
        vibratoActive = vibratoCents > 15.0f && vibratoCents < 200.0f;
        vibratoDepth = vibratoCents;
    }
    else
    {
        vibratoActive = false;
    }

    // If no pitch detected or very low level, skip processing
    if (f0 <= 0.0f)
        return;

    // Find target pitch
    targetHz = nearestScaleFreq (f0);

    // Humanize: add slight random deviation to target
    targetHz *= std::pow (2.0f, getHumanizeOffset() / 1200.0f);

    float desiredRatio = targetHz / f0;

    // Apply strength (blend toward 1.0 = no shift)
    desiredRatio = 1.0f + (desiredRatio - 1.0f) * strength;

    // Profile-based correction curve
    float retuneMultiplier = 1.0f;
    switch (profile)
    {
        case Profile::tight:   retuneMultiplier = 0.3f; break;
        case Profile::natural: retuneMultiplier = 1.0f; break;
        case Profile::loose:   retuneMultiplier = 3.0f; break;
    }

    // Retune speed smoothing
    const float tauMs = (2.0f + retune * retuneMultiplier * 300.0f);
    const float coef  = std::exp (-1.0f / (tauMs * 0.001f * (float) sr / (float) numSmp));
    smoothedRatio = desiredRatio + (smoothedRatio - desiredRatio) * coef;

    // Reduce correction near transients (keeps consonants/attacks natural)
    smoothedRatio = 1.0f + (smoothedRatio - 1.0f) * (1.0f - transientFactor * 0.7f);

    // If vibrato preservation is on, reduce correction amount during vibrato
    if (vibrato && vibratoActive)
    {
        const float vibratoReduction = juce::jlimit (0.0f, 1.0f, vibratoDepth / 100.0f);
        smoothedRatio = 1.0f + (smoothedRatio - 1.0f) * (1.0f - vibratoReduction * 0.6f);
    }

    currentCents = 1200.0f * std::log2 (juce::jmax (1.0e-4f, smoothedRatio));

    // If ratio is close to 1.0, skip processing to save CPU
    if (std::abs (smoothedRatio - 1.0f) < 0.001f)
        return;

    // Preserve dry copy for formant blending
    juce::AudioBuffer<float> dryCopy;
    if (formant)
        dryCopy.makeCopyOf (buffer);

    // 4-grain granular pitch shift
    pitchShiftGranular (buffer, smoothedRatio);

    // Formant preservation
    if (formant)
    {
        applyFormantPreserve (buffer, smoothedRatio);

        // Blend with dry to preserve some naturalness
        const float formantBlend = 0.4f; // 40% preserved, 60% shifted
        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* w = buffer.getWritePointer (ch);
            const auto* d = dryCopy.getReadPointer (ch);
            for (int i = 0; i < numSmp; ++i)
                w[i] = d[i] * formantBlend + w[i] * (1.0f - formantBlend);
        }
    }
}
