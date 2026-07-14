#include "AIAnalyzer.h"

AIAnalyzer::AIAnalyzer()
{
    fftScratch.assign (fftSize * 2, 0.0f);
    monoAccum.assign (fftSize, 0.0f);
}

void AIAnalyzer::prepare (double sampleRate, int maximumBlockSize, int /*numChannels*/)
{
    sr = sampleRate;
    blockSize = maximumBlockSize;
    reset();
}

void AIAnalyzer::reset()
{
    runningRms = runningPeak = 0.0f;
    noiseFloor = 1.0e-6f;
    centroid = 0.5f;
    f0MinRunning = 0.0f;
    f0MaxRunning = 0.0f;
    onsetEnv.fill (0.0f);
    onsetWrite = 0;
    prevFluxMag = 0.0f;
    chroma.fill (0.0f);
    std::fill (monoAccum.begin(), monoAccum.end(), 0.0f);
    monoAccumWrite = 0;

    f0Hz.store (0.0f);
    sibDb.store (-90.0f);
    rmsDbAtomic.store (-60.0f);
    noiseDb.store (-90.0f);
}

void AIAnalyzer::analyze (const juce::AudioBuffer<float>& buffer)
{
    const int numCh  = buffer.getNumChannels();
    const int numSmp = buffer.getNumSamples();
    if (numSmp <= 0 || numCh <= 0)
        return;

    // Build a mono view.
    std::vector<float> mono ((size_t) numSmp);
    for (int i = 0; i < numSmp; ++i)
    {
        float s = 0.0f;
        for (int ch = 0; ch < numCh; ++ch)
            s += buffer.getReadPointer (ch)[i];
        mono[(size_t) i] = s / (float) numCh;
    }

    // ---- Level statistics --------------------------------------------------
    float sumSq = 0.0f, peak = 0.0f;
    for (float s : mono)
    {
        sumSq += s * s;
        peak = juce::jmax (peak, std::abs (s));
    }
    const float rms = std::sqrt (sumSq / (float) numSmp + 1.0e-12f);

    const float smooth = 0.2f;
    runningRms  = runningRms  * (1.0f - smooth) + rms  * smooth;
    runningPeak = juce::jmax (runningPeak * 0.95f, peak);

    // Noise floor tracks the minimum RMS over time (slow attack, slow release).
    if (rms < noiseFloor)  noiseFloor = noiseFloor * 0.99f + rms * 0.01f;
    else                   noiseFloor = noiseFloor * 0.9995f + rms * 0.0005f;

    const float rmsDb   = juce::Decibels::gainToDecibels (runningRms,  -100.0f);
    const float peakDb  = juce::Decibels::gainToDecibels (runningPeak, -100.0f);
    const float noiseDbV= juce::Decibels::gainToDecibels (noiseFloor,  -120.0f);

    rmsDbAtomic.store (rmsDb);
    noiseDb.store (noiseDbV);

    // ---- Feed the FFT ring buffer -----------------------------------------
    for (int i = 0; i < numSmp; ++i)
    {
        monoAccum[(size_t) monoAccumWrite] = mono[(size_t) i];
        if (++monoAccumWrite >= fftSize)
        {
            monoAccumWrite = 0;
            updateBandEnergies (monoAccum.data(), fftSize);
            updateChroma       (monoAccum.data(), fftSize);
        }
    }

    // ---- Pitch (YIN on the block) -----------------------------------------
    const float f0 = detectPitchYIN (mono.data(), numSmp);
    if (f0 > 50.0f && f0 < 1200.0f && rmsDb > -45.0f)
    {
        f0Hz.store (f0);
        if (f0MinRunning <= 0.0f || f0 < f0MinRunning) f0MinRunning = f0;
        if (f0 > f0MaxRunning)                         f0MaxRunning = f0;
    }

    // ---- Tempo (spectral flux -> onset envelope -> autocorrelation) -------
    float fluxMag = rms;
    float flux = juce::jmax (0.0f, fluxMag - prevFluxMag);
    prevFluxMag = fluxMag;
    onsetEnv[(size_t) onsetWrite] = flux;
    onsetWrite = (onsetWrite + 1) % (int) onsetEnv.size();

    float bpm = 120.0f;
    {
        // autocorrelation over the onset envelope for lags 0.3..1.0s
        const float hopSec = (float) blockSize / (float) sr;
        int minLag = juce::jlimit (1, (int) onsetEnv.size() - 1, (int) (0.3f / juce::jmax (hopSec, 1.0e-4f)));
        int maxLag = juce::jlimit (1, (int) onsetEnv.size() - 1, (int) (1.0f / juce::jmax (hopSec, 1.0e-4f)));
        float best = 0.0f; int bestLag = minLag;
        for (int lag = minLag; lag <= maxLag; ++lag)
        {
            float acc = 0.0f;
            for (int n = lag; n < (int) onsetEnv.size(); ++n)
                acc += onsetEnv[(size_t) n] * onsetEnv[(size_t) (n - lag)];
            if (acc > best) { best = acc; bestLag = lag; }
        }
        if (best > 0.0f)
            bpm = 60.0f / (bestLag * juce::jmax (hopSec, 1.0e-4f));
        bpm = juce::jlimit (60.0f, 200.0f, bpm);
    }

    // ---- Derive key from chroma -------------------------------------------
    int   keyIdx = 0; float bestChroma = -1.0f;
    for (int k = 0; k < 12; ++k)
        if (chroma[(size_t) k] > bestChroma) { bestChroma = chroma[(size_t) k]; keyIdx = k; }

    // crude major/minor: compare 3rd degree energy
    const float majorThird = chroma[(size_t) ((keyIdx + 4) % 12)];
    const float minorThird = chroma[(size_t) ((keyIdx + 3) % 12)];
    const bool  minor = minorThird > majorThird;

    // ---- Gender / genre heuristics ----------------------------------------
    int gender = 0;
    if (f0 > 60.0f)
        gender = (f0 < 165.0f) ? 1 : 2;   // < ~E3 male, else female

    int genre = 0;
    const float crest = peakDb - rmsDb;
    if (bpm > 140.0f && crest > 14.0f)      genre = 1; // Rap/Hip-hop
    else if (bpm > 110.0f)                  genre = 2; // Pop
    else if (crest > 16.0f)                 genre = 3; // Rock
    else if (rmsDb > -18.0f && bpm < 100.0f)genre = 4; // Podcast / VO

    // Sibilance already updated in updateBandEnergies -> sibDb
    // Breathiness from HF/LF ratio proxy via brightness.
    const float breath = juce::jlimit (0.0f, 1.0f, (centroid - 0.35f) * 1.5f);

    // Mic distance: more LF proximity (lower centroid, higher level) => close.
    const float micDist = juce::jlimit (0.0f, 1.0f, centroid * 0.8f + (float) (rmsDb < -30.0f ? 0.3f : 0.0f));

    // ---- Publish snapshot --------------------------------------------------
    Snapshot snap;
    snap.keyIndex     = keyIdx;
    snap.isMinor      = minor;
    snap.bpm          = bpm;
    snap.rmsDb        = rmsDb;
    snap.peakDb       = peakDb;
    snap.crestDb      = crest;
    snap.noiseFloorDb = noiseDbV;
    snap.sibilanceDb  = sibDb.load();
    snap.breathiness  = breath;
    snap.f0           = f0Hz.load();
    snap.f0Min        = f0MinRunning;
    snap.f0Max        = f0MaxRunning;
    snap.micDistance  = micDist;
    snap.brightness   = centroid;
    snap.genderGuess  = gender;
    snap.genreGuess   = genre;

    const juce::SpinLock::ScopedLockType sl (snapshotLock);
    current = snap;
}

AIAnalyzer::Snapshot AIAnalyzer::getSnapshot() const noexcept
{
    const juce::SpinLock::ScopedLockType sl (snapshotLock);
    return current;
}

void AIAnalyzer::updateBandEnergies (const float* monoRing, int numSamples)
{
    // Copy + window + FFT
    std::fill (fftScratch.begin(), fftScratch.end(), 0.0f);
    for (int i = 0; i < numSamples && i < fftSize; ++i)
        fftScratch[(size_t) i] = monoRing[i];

    window.multiplyWithWindowingTable (fftScratch.data(), (size_t) fftSize);
    fft.performFrequencyOnlyForwardTransform (fftScratch.data());

    const float binHz = (float) sr / (float) fftSize;

    float total = 0.0f, weighted = 0.0f, sibEnergy = 0.0f;
    for (int bin = 1; bin < fftSize / 2; ++bin)
    {
        const float mag = fftScratch[(size_t) bin];
        const float freq = bin * binHz;
        total    += mag;
        weighted += mag * freq;
        if (freq >= 5000.0f && freq <= 9000.0f)
            sibEnergy += mag;
    }

    if (total > 1.0e-9f)
    {
        const float c = (weighted / total) / ((float) sr * 0.5f);
        centroid = centroid * 0.8f + juce::jlimit (0.0f, 1.0f, c) * 0.2f;
        const float sibRatio = sibEnergy / total;
        sibDb.store (juce::Decibels::gainToDecibels (sibRatio, -90.0f));
    }
}

void AIAnalyzer::updateChroma (const float* monoRing, int numSamples)
{
    // Uses the same fftScratch magnitude spectrum computed just before by
    // updateBandEnergies (they are called back-to-back on the same frame).
    const float binHz = (float) sr / (float) fftSize;
    std::array<float, 12> local {};

    for (int bin = 2; bin < fftSize / 2; ++bin)
    {
        const float freq = bin * binHz;
        if (freq < 60.0f || freq > 2000.0f)
            continue;
        const float mag = fftScratch[(size_t) bin];
        const float midi = 69.0f + 12.0f * std::log2 (freq / 440.0f);
        int pc = ((int) std::lround (midi)) % 12;
        if (pc < 0) pc += 12;
        local[(size_t) pc] += mag;
    }

    for (int k = 0; k < 12; ++k)
        chroma[(size_t) k] = chroma[(size_t) k] * 0.9f + local[(size_t) k] * 0.1f;
    juce::ignoreUnused (numSamples, monoRing);
}

float AIAnalyzer::detectPitchYIN (const float* mono, int numSamples)
{
    // Classic YIN difference function on a limited lag range.
    const int maxLag = juce::jmin (numSamples / 2, (int) (sr / 60.0));   // down to 60 Hz
    const int minLag = juce::jmax (2, (int) (sr / 1200.0));              // up to 1200 Hz
    if (maxLag <= minLag)
        return 0.0f;

    std::vector<float> d ((size_t) maxLag + 1, 0.0f);
    for (int tau = minLag; tau <= maxLag; ++tau)
    {
        float sum = 0.0f;
        for (int i = 0; i + tau < numSamples; ++i)
        {
            const float diff = mono[i] - mono[i + tau];
            sum += diff * diff;
        }
        d[(size_t) tau] = sum;
    }

    // cumulative mean normalised difference
    std::vector<float> cmnd ((size_t) maxLag + 1, 1.0f);
    float running = 0.0f;
    for (int tau = minLag; tau <= maxLag; ++tau)
    {
        running += d[(size_t) tau];
        cmnd[(size_t) tau] = d[(size_t) tau] * (float) (tau - minLag + 1) / juce::jmax (running, 1.0e-9f);
    }

    const float threshold = 0.15f;
    int bestTau = -1;
    for (int tau = minLag + 1; tau < maxLag; ++tau)
    {
        if (cmnd[(size_t) tau] < threshold
            && cmnd[(size_t) tau] < cmnd[(size_t) (tau + 1)])
        {
            bestTau = tau;
            break;
        }
    }
    if (bestTau < 0)
    {
        // fall back to global minimum
        float best = 1.0e9f;
        for (int tau = minLag; tau <= maxLag; ++tau)
            if (cmnd[(size_t) tau] < best) { best = cmnd[(size_t) tau]; bestTau = tau; }
    }
    if (bestTau <= 0)
        return 0.0f;

    // parabolic interpolation around bestTau
    float betterTau = (float) bestTau;
    if (bestTau > minLag && bestTau < maxLag)
    {
        const float s0 = cmnd[(size_t) (bestTau - 1)];
        const float s1 = cmnd[(size_t) bestTau];
        const float s2 = cmnd[(size_t) (bestTau + 1)];
        const float denom = (2.0f * (2.0f * s1 - s2 - s0));
        if (std::abs (denom) > 1.0e-9f)
            betterTau = bestTau + (s2 - s0) / denom;
    }

    return (float) sr / betterTau;
}

juce::String AIAnalyzer::keyName (int keyIndex, bool isMinor)
{
    static const char* names[] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
    keyIndex = ((keyIndex % 12) + 12) % 12;
    return juce::String (names[keyIndex]) + (isMinor ? "m" : "");
}

juce::String AIAnalyzer::genreName (int index)
{
    static const char* g[] = { "Unknown", "Rap/Hip-Hop", "Pop", "Rock", "Podcast/VO" };
    if (index < 0 || index >= (int) (sizeof (g) / sizeof (g[0]))) return "Unknown";
    return g[index];
}

juce::String AIAnalyzer::genderName (int index)
{
    static const char* g[] = { "Unknown", "Male", "Female" };
    if (index < 0 || index >= 3) return "Unknown";
    return g[index];
}
