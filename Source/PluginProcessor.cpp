#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    using APF   = juce::AudioParameterFloat;
    using APB   = juce::AudioParameterBool;
    using APC   = juce::AudioParameterChoice;
    using NRange = juce::NormalisableRange<float>;

    auto pid (const char* s, int v = 1) { return juce::ParameterID { s, v }; }
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout VocalOneAudioProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    // ---- Global -----------------------------------------------------------
    p.push_back (std::make_unique<APB> (pid ("bypass"),   "Bypass", false));
    p.push_back (std::make_unique<APF> (pid ("in_gain"),  "Input Gain",  NRange (-24.0f, 24.0f, 0.1f), 0.0f));
    p.push_back (std::make_unique<APF> (pid ("out_gain"), "Output Gain", NRange (-24.0f, 24.0f, 0.1f), 0.0f));
    p.push_back (std::make_unique<APF> (pid ("mix"),      "Global Mix",  NRange (0.0f, 1.0f, 0.01f), 1.0f));

    // ---- Gate -------------------------------------------------------------
    p.push_back (std::make_unique<APB> (pid ("gate_on"),      "Gate On", true));
    p.push_back (std::make_unique<APB> (pid ("gate_auto"),    "Gate Auto", true));
    p.push_back (std::make_unique<APF> (pid ("gate_thresh"),  "Gate Threshold", NRange (-80.0f, 0.0f, 0.1f), -45.0f));
    p.push_back (std::make_unique<APF> (pid ("gate_range"),   "Gate Range", NRange (-80.0f, 0.0f, 0.1f), -60.0f));
    p.push_back (std::make_unique<APF> (pid ("gate_attack"),  "Gate Attack", NRange (0.1f, 50.0f, 0.1f), 1.0f));
    p.push_back (std::make_unique<APF> (pid ("gate_hold"),    "Gate Hold", NRange (0.0f, 500.0f, 1.0f), 10.0f));
    p.push_back (std::make_unique<APF> (pid ("gate_release"), "Gate Release", NRange (5.0f, 1000.0f, 1.0f), 100.0f));

    // ---- Noise removal ----------------------------------------------------
    p.push_back (std::make_unique<APB> (pid ("noise_on"),     "Noise Removal On", true));
    p.push_back (std::make_unique<APB> (pid ("noise_learn"),  "Noise Learn", true));
    p.push_back (std::make_unique<APF> (pid ("noise_amount"), "Noise Amount", NRange (0.0f, 100.0f, 1.0f), 40.0f));

    // ---- De-esser ---------------------------------------------------------
    p.push_back (std::make_unique<APB> (pid ("deess_on"),     "DeEsser On", true));
    p.push_back (std::make_unique<APB> (pid ("deess_auto"),   "DeEsser Auto", true));
    p.push_back (std::make_unique<APF> (pid ("deess_freq"),   "DeEsser Freq", NRange (3000.0f, 12000.0f, 1.0f), 6500.0f));
    p.push_back (std::make_unique<APF> (pid ("deess_thresh"), "DeEsser Threshold", NRange (-60.0f, 0.0f, 0.1f), -30.0f));
    p.push_back (std::make_unique<APF> (pid ("deess_ratio"),  "DeEsser Ratio", NRange (1.0f, 10.0f, 0.1f), 4.0f));

    // ---- Smart Tune -------------------------------------------------------
    p.push_back (std::make_unique<APB> (pid ("tune_on"),       "Tune On", false));
    p.push_back (std::make_unique<APC> (pid ("tune_key"),      "Tune Key",
        juce::StringArray { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" }, 0));
    p.push_back (std::make_unique<APC> (pid ("tune_scale"),    "Tune Scale",
        juce::StringArray { "Chromatic","Major","Minor","Major Pentatonic","Minor Pentatonic",
                             "Blues","Dorian","Phrygian","Lydian","Mixolydian",
                             "Harmonic Minor","Melodic Minor","Diminished","Whole Tone",
                             "Arabic","Japanese" }, 0));
    p.push_back (std::make_unique<APF> (pid ("tune_speed"),    "Tune Speed", NRange (0.0f, 1.0f, 0.01f), 0.2f));
    p.push_back (std::make_unique<APF> (pid ("tune_strength"), "Tune Strength", NRange (0.0f, 1.0f, 0.01f), 1.0f));
    p.push_back (std::make_unique<APB> (pid ("tune_formant"),  "Tune Formant", true));
    p.push_back (std::make_unique<APB> (pid ("tune_vibrato"),  "Tune Vibrato Preserve", true));
    p.push_back (std::make_unique<APF> (pid ("tune_humanize"), "Tune Humanize", NRange (0.0f, 1.0f, 0.01f), 0.0f));
    p.push_back (std::make_unique<APC> (pid ("tune_profile"),  "Tune Profile",
        juce::StringArray { "Tight","Natural","Loose" }, 1));

    // ---- EQ ---------------------------------------------------------------
    p.push_back (std::make_unique<APB> (pid ("eq_on"),     "EQ On", true));
    p.push_back (std::make_unique<APB> (pid ("eq_auto"),   "EQ Auto", true));
    p.push_back (std::make_unique<APF> (pid ("eq_amount"), "EQ Amount", NRange (0.0f, 1.0f, 0.01f), 0.6f));
    const char* eqNames[] = { "eq_boomy","eq_mud","eq_boxy","eq_nasal","eq_presence","eq_harsh","eq_air","eq_thin" };
    const char* eqDisp[]  = { "EQ Boomy","EQ Mud","EQ Boxy","EQ Nasal","EQ Presence","EQ Harsh","EQ Air","EQ Thin" };
    for (int i = 0; i < 8; ++i)
        p.push_back (std::make_unique<APF> (pid (eqNames[i]), eqDisp[i], NRange (-12.0f, 12.0f, 0.1f), 0.0f));

    // ---- Compressor -------------------------------------------------------
    p.push_back (std::make_unique<APB> (pid ("comp_on"),     "Comp On", true));
    p.push_back (std::make_unique<APC> (pid ("comp_mode"),   "Comp Mode",
        juce::StringArray { "Auto","Rap","Pop","Rock","Podcast","Voice Over","Manual" }, 0));
    p.push_back (std::make_unique<APF> (pid ("comp_thresh"),  "Comp Threshold", NRange (-60.0f, 0.0f, 0.1f), -20.0f));
    p.push_back (std::make_unique<APF> (pid ("comp_ratio"),   "Comp Ratio", NRange (1.0f, 20.0f, 0.1f), 3.0f));
    p.push_back (std::make_unique<APF> (pid ("comp_attack"),  "Comp Attack", NRange (0.1f, 100.0f, 0.1f), 8.0f));
    p.push_back (std::make_unique<APF> (pid ("comp_release"), "Comp Release", NRange (5.0f, 1000.0f, 1.0f), 120.0f));
    p.push_back (std::make_unique<APF> (pid ("comp_knee"),    "Comp Knee", NRange (0.0f, 24.0f, 0.1f), 6.0f));
    p.push_back (std::make_unique<APF> (pid ("comp_makeup"),  "Comp Makeup", NRange (0.0f, 24.0f, 0.1f), 3.0f));

    // ---- Saturation -------------------------------------------------------
    p.push_back (std::make_unique<APB> (pid ("sat_on"),    "Sat On", false));
    p.push_back (std::make_unique<APC> (pid ("sat_model"), "Sat Model",
        juce::StringArray { "Tube","Tape","Vintage","Warm","Digital" }, 0));
    p.push_back (std::make_unique<APF> (pid ("sat_drive"), "Sat Drive", NRange (0.0f, 1.0f, 0.01f), 0.3f));
    p.push_back (std::make_unique<APF> (pid ("sat_mix"),   "Sat Mix", NRange (0.0f, 1.0f, 0.01f), 1.0f));

    // ---- Exciter ----------------------------------------------------------
    p.push_back (std::make_unique<APB> (pid ("exc_on"),     "Exciter On", false));
    p.push_back (std::make_unique<APF> (pid ("exc_freq"),   "Exciter Freq", NRange (1000.0f, 8000.0f, 1.0f), 3000.0f));
    p.push_back (std::make_unique<APF> (pid ("exc_amount"), "Exciter Amount", NRange (0.0f, 1.0f, 0.01f), 0.3f));

    // ---- Fresh Air --------------------------------------------------------
    p.push_back (std::make_unique<APB> (pid ("air_on"),   "Air On", false));
    p.push_back (std::make_unique<APF> (pid ("air_mid"),  "Air Mid", NRange (0.0f, 1.0f, 0.01f), 0.3f));
    p.push_back (std::make_unique<APF> (pid ("air_high"), "Air High", NRange (0.0f, 1.0f, 0.01f), 0.3f));

    // ---- Delay ------------------------------------------------------------
    p.push_back (std::make_unique<APB> (pid ("dly_on"),   "Delay On", false));
    p.push_back (std::make_unique<APF> (pid ("dly_time"), "Delay Time", NRange (1.0f, 2000.0f, 1.0f), 350.0f));
    p.push_back (std::make_unique<APF> (pid ("dly_fb"),   "Delay Feedback", NRange (0.0f, 0.95f, 0.01f), 0.35f));
    p.push_back (std::make_unique<APF> (pid ("dly_mix"),  "Delay Mix", NRange (0.0f, 1.0f, 0.01f), 0.2f));
    p.push_back (std::make_unique<APB> (pid ("dly_ping"), "Delay PingPong", false));
    p.push_back (std::make_unique<APF> (pid ("dly_damp"), "Delay Damping", NRange (500.0f, 18000.0f, 1.0f), 6000.0f));

    // ---- Reverb -----------------------------------------------------------
    p.push_back (std::make_unique<APB> (pid ("rev_on"),       "Reverb On", false));
    p.push_back (std::make_unique<APF> (pid ("rev_size"),     "Reverb Size", NRange (0.0f, 1.0f, 0.01f), 0.5f));
    p.push_back (std::make_unique<APF> (pid ("rev_damp"),     "Reverb Damping", NRange (0.0f, 1.0f, 0.01f), 0.5f));
    p.push_back (std::make_unique<APF> (pid ("rev_width"),    "Reverb Width", NRange (0.0f, 1.0f, 0.01f), 1.0f));
    p.push_back (std::make_unique<APF> (pid ("rev_mix"),      "Reverb Mix", NRange (0.0f, 1.0f, 0.01f), 0.2f));
    p.push_back (std::make_unique<APF> (pid ("rev_predelay"), "Reverb PreDelay", NRange (0.0f, 200.0f, 1.0f), 20.0f));

    // ---- Doubler ----------------------------------------------------------
    p.push_back (std::make_unique<APB> (pid ("dbl_on"),     "Doubler On", false));
    p.push_back (std::make_unique<APF> (pid ("dbl_voices"), "Doubler Voices", NRange (1.0f, 4.0f, 1.0f), 2.0f));
    p.push_back (std::make_unique<APF> (pid ("dbl_detune"), "Doubler Detune", NRange (0.0f, 40.0f, 0.1f), 12.0f));
    p.push_back (std::make_unique<APF> (pid ("dbl_delay"),  "Doubler Delay", NRange (1.0f, 60.0f, 0.1f), 20.0f));
    p.push_back (std::make_unique<APF> (pid ("dbl_spread"), "Doubler Spread", NRange (0.0f, 1.0f, 0.01f), 1.0f));
    p.push_back (std::make_unique<APF> (pid ("dbl_mix"),    "Doubler Mix", NRange (0.0f, 1.0f, 0.01f), 0.4f));

    // ---- Chorus -----------------------------------------------------------
    p.push_back (std::make_unique<APB> (pid ("chor_on"),    "Chorus On", false));
    p.push_back (std::make_unique<APF> (pid ("chor_rate"),  "Chorus Rate", NRange (0.05f, 10.0f, 0.01f), 1.2f));
    p.push_back (std::make_unique<APF> (pid ("chor_depth"), "Chorus Depth", NRange (0.0f, 1.0f, 0.01f), 0.3f));
    p.push_back (std::make_unique<APF> (pid ("chor_delay"), "Chorus Delay", NRange (1.0f, 40.0f, 0.1f), 12.0f));
    p.push_back (std::make_unique<APF> (pid ("chor_fb"),    "Chorus Feedback", NRange (-0.95f, 0.95f, 0.01f), 0.0f));
    p.push_back (std::make_unique<APF> (pid ("chor_mix"),   "Chorus Mix", NRange (0.0f, 1.0f, 0.01f), 0.3f));

    // ---- Stereo width -----------------------------------------------------
    p.push_back (std::make_unique<APB> (pid ("st_on"),       "Stereo On", false));
    p.push_back (std::make_unique<APF> (pid ("st_width"),    "Stereo Width", NRange (0.0f, 2.0f, 0.01f), 1.0f));
    p.push_back (std::make_unique<APF> (pid ("st_bassmono"), "Stereo Bass Mono", NRange (20.0f, 500.0f, 1.0f), 150.0f));

    // ---- Limiter ----------------------------------------------------------
    p.push_back (std::make_unique<APB> (pid ("lim_on"),      "Limiter On", true));
    p.push_back (std::make_unique<APF> (pid ("lim_ceiling"), "Limiter Ceiling", NRange (-12.0f, 0.0f, 0.1f), -1.0f));
    p.push_back (std::make_unique<APF> (pid ("lim_release"), "Limiter Release", NRange (1.0f, 500.0f, 1.0f), 50.0f));
    p.push_back (std::make_unique<APF> (pid ("lim_input"),   "Limiter Input", NRange (-12.0f, 24.0f, 0.1f), 0.0f));

    return { p.begin(), p.end() };
}

//==============================================================================
VocalOneAudioProcessor::VocalOneAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "VocalOneState", createLayout())
{
    // Configure the assistant from environment variables if present, so the VM
    // OpenAI-compatible endpoint works out of the box.
    AIAssistant::Config cfg;
    if (auto key = juce::SystemStats::getEnvironmentVariable ("ABACUS_API_KEY", {}); key.isNotEmpty())
        cfg.apiKey = key;
    else if (auto oa = juce::SystemStats::getEnvironmentVariable ("OPENAI_API_KEY", {}); oa.isNotEmpty())
        cfg.apiKey = oa;
    if (auto base = juce::SystemStats::getEnvironmentVariable ("OPENAI_BASE_URL", {}); base.isNotEmpty())
        cfg.baseUrl = base;
    assistant.setConfig (cfg);
}

VocalOneAudioProcessor::~VocalOneAudioProcessor() = default;

//==============================================================================
void VocalOneAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    const int numCh = juce::jmax (getTotalNumInputChannels(), 2);

    analyzer.prepare      (sampleRate, samplesPerBlock, numCh);
    gate.prepare          (sampleRate, samplesPerBlock, numCh);
    noiseRemoval.prepare  (sampleRate, samplesPerBlock, numCh);
    deEsser.prepare       (sampleRate, samplesPerBlock, numCh);
    smartTune.prepare     (sampleRate, samplesPerBlock, numCh);
    eq.prepare            (sampleRate, samplesPerBlock, numCh);
    compressor.prepare    (sampleRate, samplesPerBlock, numCh);
    saturation.prepare    (sampleRate, samplesPerBlock, numCh);
    exciter.prepare       (sampleRate, samplesPerBlock, numCh);
    freshAir.prepare      (sampleRate, samplesPerBlock, numCh);
    delay.prepare         (sampleRate, samplesPerBlock, numCh);
    reverb.prepare        (sampleRate, samplesPerBlock, numCh);
    doubler.prepare       (sampleRate, samplesPerBlock, numCh);
    chorus.prepare        (sampleRate, samplesPerBlock, numCh);
    stereoWidth.prepare   (sampleRate, samplesPerBlock, numCh);
    limiter.prepare       (sampleRate, samplesPerBlock, numCh);

    inGainSmoothed.reset  (sampleRate, 0.02);
    outGainSmoothed.reset (sampleRate, 0.02);
}

void VocalOneAudioProcessor::releaseResources() {}

bool VocalOneAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainOut = layouts.getMainOutputChannelSet();
    const auto& mainIn  = layouts.getMainInputChannelSet();
    if (mainOut != juce::AudioChannelSet::mono() && mainOut != juce::AudioChannelSet::stereo())
        return false;
    if (mainIn != mainOut)
        return false;
    return true;
}

//==============================================================================
static float getP (juce::AudioProcessorValueTreeState& a, const char* id)
{
    if (auto* v = a.getRawParameterValue (id))
        return v->load();
    return 0.0f;
}

void VocalOneAudioProcessor::syncParametersToModules()
{
    auto& a = apvts;
    auto snap = analyzer.getSnapshot();

    // Gate
    gate.setParameters (getP (a, "gate_on") > 0.5f, getP (a, "gate_thresh"),
                        getP (a, "gate_range"), getP (a, "gate_attack"),
                        getP (a, "gate_hold"), getP (a, "gate_release"),
                        getP (a, "gate_auto") > 0.5f);
    gate.setNoiseFloorDb (snap.noiseFloorDb);

    // Noise removal
    noiseRemoval.setParameters (getP (a, "noise_on") > 0.5f, getP (a, "noise_amount"),
                                getP (a, "noise_learn") > 0.5f);

    // De-esser
    deEsser.setParameters (getP (a, "deess_on") > 0.5f, getP (a, "deess_freq"),
                           getP (a, "deess_thresh"), getP (a, "deess_ratio"),
                           getP (a, "deess_auto") > 0.5f);
    deEsser.setSibilanceDb (snap.sibilanceDb);

    // Smart tune
    {
        const int keyChoice = (int) getP (a, "tune_key");
        const int scaleChoice = (int) getP (a, "tune_scale");
        smartTune.setParameters (getP (a, "tune_on") > 0.5f, keyChoice,
                                 (SmartTune::Scale) scaleChoice,
                                 getP (a, "tune_speed"), getP (a, "tune_strength"),
                                 getP (a, "tune_formant") > 0.5f,
                                 getP (a, "tune_vibrato") > 0.5f,
                                 getP (a, "tune_humanize"),
                                 (SmartTune::Profile) (int) getP (a, "tune_profile"));
        smartTune.setDetectedF0 (analyzer.getCurrentF0());
    }

    // EQ
    eq.setEnabled (getP (a, "eq_on") > 0.5f);
    eq.setAuto    (getP (a, "eq_auto") > 0.5f);
    eq.setAmount  (getP (a, "eq_amount"));
    const char* eqNames[] = { "eq_boomy","eq_mud","eq_boxy","eq_nasal","eq_presence","eq_harsh","eq_air","eq_thin" };
    for (int i = 0; i < 8; ++i)
        eq.setManualGain (i, getP (a, eqNames[i]));

    // Compressor
    compressor.setEnabled (getP (a, "comp_on") > 0.5f);
    compressor.setMode ((AICompressor::Mode) (int) getP (a, "comp_mode"));
    compressor.setManual (getP (a, "comp_thresh"), getP (a, "comp_ratio"),
                          getP (a, "comp_attack"), getP (a, "comp_release"),
                          getP (a, "comp_knee"), getP (a, "comp_makeup"));
    compressor.setHints (snap.crestDb, snap.bpm);

    // Saturation
    saturation.setParameters (getP (a, "sat_on") > 0.5f,
                              (Saturation::Model) (int) getP (a, "sat_model"),
                              getP (a, "sat_drive"), getP (a, "sat_mix"));

    // Exciter
    exciter.setParameters (getP (a, "exc_on") > 0.5f, getP (a, "exc_freq"), getP (a, "exc_amount"));

    // Fresh Air
    freshAir.setParameters (getP (a, "air_on") > 0.5f, getP (a, "air_mid"), getP (a, "air_high"));

    // Delay
    delay.setParameters (getP (a, "dly_on") > 0.5f, getP (a, "dly_time"),
                         getP (a, "dly_fb"), getP (a, "dly_mix"),
                         getP (a, "dly_ping") > 0.5f, getP (a, "dly_damp"));

    // Reverb
    reverb.setParameters (getP (a, "rev_on") > 0.5f, getP (a, "rev_size"),
                          getP (a, "rev_damp"), getP (a, "rev_width"),
                          getP (a, "rev_mix"), getP (a, "rev_predelay"));

    // Doubler
    doubler.setParameters (getP (a, "dbl_on") > 0.5f, (int) getP (a, "dbl_voices"),
                           getP (a, "dbl_detune"), getP (a, "dbl_delay"),
                           getP (a, "dbl_spread"), getP (a, "dbl_mix"));

    // Chorus
    chorus.setParameters (getP (a, "chor_on") > 0.5f, getP (a, "chor_rate"),
                          getP (a, "chor_depth"), getP (a, "chor_delay"),
                          getP (a, "chor_fb"), getP (a, "chor_mix"));

    // Stereo width
    stereoWidth.setParameters (getP (a, "st_on") > 0.5f, getP (a, "st_width"), getP (a, "st_bassmono"));

    // Limiter
    limiter.setParameters (getP (a, "lim_on") > 0.5f, getP (a, "lim_ceiling"),
                           getP (a, "lim_release"), getP (a, "lim_input"));
}

void VocalOneAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                           juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int totalIn  = getTotalNumInputChannels();
    const int totalOut = getTotalNumOutputChannels();
    for (int ch = totalIn; ch < totalOut; ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    if (getP (apvts, "bypass") > 0.5f)
        return;

    // Keep a dry copy for the global wet/dry mix.
    juce::AudioBuffer<float> dry;
    dry.makeCopyOf (buffer);

    // Input gain.
    inGainSmoothed.setTargetValue (juce::Decibels::decibelsToGain (getP (apvts, "in_gain")));
    buffer.applyGainRamp (0, buffer.getNumSamples(),
                          inGainSmoothed.getCurrentValue(),
                          inGainSmoothed.getNextValue());

    // 1. Analyse (read-only), then push parameters to every module.
    analyzer.analyze (buffer);
    syncParametersToModules();

    // ---- The chain, strictly in spec order --------------------------------
    gate.process         (buffer);   // 2. AI Gate
    noiseRemoval.process (buffer);   // 3. Noise removal
    deEsser.process      (buffer);   // 4. AI DeEsser
    smartTune.process    (buffer);   // 5. Smart Tune
    eq.process           (buffer);   // 6. AI EQ
    compressor.process   (buffer);   // 7. AI Compressor
    saturation.process   (buffer);   // 8. Saturation
    exciter.process      (buffer);   // 9. Harmonic Exciter
    freshAir.process     (buffer);   // 10. Fresh Air
    delay.process        (buffer);   // 11. Delay
    reverb.process       (buffer);   // 12. Reverb
    doubler.process      (buffer);   // 13. Doubler
    chorus.process       (buffer);   // 14. Chorus
    stereoWidth.process  (buffer);   // 15. Stereo / Width
    limiter.process      (buffer);   // 16. Final Limiter

    // Output gain.
    outGainSmoothed.setTargetValue (juce::Decibels::decibelsToGain (getP (apvts, "out_gain")));
    buffer.applyGainRamp (0, buffer.getNumSamples(),
                          outGainSmoothed.getCurrentValue(),
                          outGainSmoothed.getNextValue());

    // Global wet/dry mix.
    const float mix = getP (apvts, "mix");
    if (mix < 0.999f)
    {
        const int n = buffer.getNumSamples();
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* w = buffer.getWritePointer (ch);
            const auto* d = dry.getReadPointer (juce::jmin (ch, dry.getNumChannels() - 1));
            for (int i = 0; i < n; ++i)
                w[i] = d[i] * (1.0f - mix) + w[i] * mix;
        }
    }

    // Feed the visualisers.
    pushToScope (buffer);

    // Keep the assistant's context fresh for the UI thread.
    assistant.setAnalysisContext (buildAnalysisContext());
}

void VocalOneAudioProcessor::pushToScope (const juce::AudioBuffer<float>& buffer)
{
    const int numSmp = buffer.getNumSamples();
    const int numCh  = buffer.getNumChannels();
    const auto* L = buffer.getReadPointer (0);
    const auto* R = numCh > 1 ? buffer.getReadPointer (1) : L;

    int w = scope.writePos.load();
    for (int i = 0; i < numSmp; ++i)
    {
        scope.left[(size_t) w]  = L[i];
        scope.right[(size_t) w] = R[i];
        w = (w + 1) % scopeSize;
    }
    scope.writePos.store (w);
}

//==============================================================================
juce::String VocalOneAudioProcessor::buildAnalysisContext() const
{
    auto snap = const_cast<AIAnalyzer&> (analyzer).getSnapshot();
    juce::String s;
    s << "Key: " << AIAnalyzer::keyName (snap.keyIndex, snap.isMinor)
      << " | BPM: " << juce::String (snap.bpm, 1)
      << " | Genre: " << AIAnalyzer::genreName (snap.genreGuess)
      << " | Gender: " << AIAnalyzer::genderName (snap.genderGuess) << "\n"
      << "RMS: " << juce::String (snap.rmsDb, 1) << " dB"
      << " | Peak: " << juce::String (snap.peakDb, 1) << " dB"
      << " | Crest: " << juce::String (snap.crestDb, 1) << " dB\n"
      << "Noise floor: " << juce::String (snap.noiseFloorDb, 1) << " dB"
      << " | Sibilance: " << juce::String (snap.sibilanceDb, 1) << " dB"
      << " | Breathiness: " << juce::String (snap.breathiness, 2) << "\n"
      << "F0: " << juce::String (snap.f0, 1) << " Hz"
      << " (range " << juce::String (snap.f0Min, 0) << "-" << juce::String (snap.f0Max, 0) << " Hz)"
      << " | Mic distance: " << juce::String (snap.micDistance, 2);
    return s;
}

//==============================================================================
juce::AudioProcessorEditor* VocalOneAudioProcessor::createEditor()
{
    return new VocalOneAudioProcessorEditor (*this);
}

void VocalOneAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void VocalOneAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VocalOneAudioProcessor();
}
