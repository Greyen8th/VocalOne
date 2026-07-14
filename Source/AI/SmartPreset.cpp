#include "SmartPreset.h"

void SmartPreset::setParam (juce::AudioProcessorValueTreeState& apvts,
                            const juce::String& id, float value)
{
    if (auto* p = apvts.getParameter (id))
    {
        const auto norm = apvts.getParameterRange (id).convertTo0to1 (value);
        p->setValueNotifyingHost (norm);
    }
}

juce::StringArray SmartPreset::getFactoryPresetNames()
{
    return { "Init / Flat",
             "Modern Pop Vocal",
             "Rap / Trap Lead",
             "Rock Belter",
             "Podcast / Voice Over",
             "Airy Ballad",
             "Vintage Warmth" };
}

void SmartPreset::applyFactoryPreset (juce::AudioProcessorValueTreeState& apvts,
                                      const juce::String& name)
{
    // Sensible starting point for every preset.
    setParam (apvts, "gate_on", 1.0f);   setParam (apvts, "gate_auto", 1.0f);
    setParam (apvts, "noise_on", 1.0f);  setParam (apvts, "noise_amount", 40.0f);
    setParam (apvts, "deess_on", 1.0f);  setParam (apvts, "deess_auto", 1.0f);
    setParam (apvts, "tune_on", 0.0f);
    setParam (apvts, "eq_on", 1.0f);     setParam (apvts, "eq_auto", 1.0f);  setParam (apvts, "eq_amount", 0.6f);
    setParam (apvts, "comp_on", 1.0f);   setParam (apvts, "comp_mode", 0.0f);
    setParam (apvts, "sat_on", 0.0f);
    setParam (apvts, "exc_on", 0.0f);
    setParam (apvts, "air_on", 0.0f);
    setParam (apvts, "st_on", 0.0f);
    setParam (apvts, "dly_on", 0.0f);
    setParam (apvts, "rev_on", 0.0f);
    setParam (apvts, "dbl_on", 0.0f);
    setParam (apvts, "chor_on", 0.0f);
    setParam (apvts, "lim_on", 1.0f);    setParam (apvts, "lim_ceiling", -1.0f);

    if (name == "Modern Pop Vocal")
    {
        setParam (apvts, "comp_mode", 2.0f);
        setParam (apvts, "sat_on", 1.0f);  setParam (apvts, "sat_model", 0.0f); setParam (apvts, "sat_drive", 0.25f);
        setParam (apvts, "air_on", 1.0f);  setParam (apvts, "air_mid", 0.35f);  setParam (apvts, "air_high", 0.4f);
        setParam (apvts, "rev_on", 1.0f);  setParam (apvts, "rev_size", 0.4f);  setParam (apvts, "rev_mix", 0.18f);
        setParam (apvts, "dly_on", 1.0f);  setParam (apvts, "dly_mix", 0.12f);
    }
    else if (name == "Rap / Trap Lead")
    {
        setParam (apvts, "comp_mode", 1.0f);
        setParam (apvts, "sat_on", 1.0f);  setParam (apvts, "sat_model", 4.0f); setParam (apvts, "sat_drive", 0.35f);
        setParam (apvts, "dbl_on", 1.0f);  setParam (apvts, "dbl_voices", 2.0f); setParam (apvts, "dbl_mix", 0.4f);
        setParam (apvts, "dly_on", 1.0f);  setParam (apvts, "dly_ping", 1.0f);  setParam (apvts, "dly_mix", 0.15f);
    }
    else if (name == "Rock Belter")
    {
        setParam (apvts, "comp_mode", 3.0f);
        setParam (apvts, "sat_on", 1.0f);  setParam (apvts, "sat_model", 1.0f); setParam (apvts, "sat_drive", 0.45f);
        setParam (apvts, "exc_on", 1.0f);  setParam (apvts, "exc_amount", 0.4f);
        setParam (apvts, "rev_on", 1.0f);  setParam (apvts, "rev_size", 0.55f); setParam (apvts, "rev_mix", 0.2f);
    }
    else if (name == "Podcast / Voice Over")
    {
        setParam (apvts, "comp_mode", 4.0f);
        setParam (apvts, "noise_amount", 65.0f);
        setParam (apvts, "air_on", 1.0f);  setParam (apvts, "air_mid", 0.25f);
        setParam (apvts, "lim_ceiling", -1.5f);
    }
    else if (name == "Airy Ballad")
    {
        setParam (apvts, "comp_mode", 2.0f);
        setParam (apvts, "air_on", 1.0f);  setParam (apvts, "air_mid", 0.5f);  setParam (apvts, "air_high", 0.6f);
        setParam (apvts, "exc_on", 1.0f);  setParam (apvts, "exc_amount", 0.3f);
        setParam (apvts, "rev_on", 1.0f);  setParam (apvts, "rev_size", 0.65f); setParam (apvts, "rev_mix", 0.28f); setParam (apvts, "rev_predelay", 30.0f);
        setParam (apvts, "chor_on", 1.0f); setParam (apvts, "chor_mix", 0.15f);
    }
    else if (name == "Vintage Warmth")
    {
        setParam (apvts, "comp_mode", 2.0f);
        setParam (apvts, "sat_on", 1.0f);  setParam (apvts, "sat_model", 2.0f); setParam (apvts, "sat_drive", 0.5f);
        setParam (apvts, "eq_amount", 0.7f);
        setParam (apvts, "rev_on", 1.0f);  setParam (apvts, "rev_size", 0.45f); setParam (apvts, "rev_mix", 0.16f);
    }
    // "Init / Flat" leaves the baseline above.
}

void SmartPreset::applyAutoPreset (juce::AudioProcessorValueTreeState& apvts,
                                   const AnalysisInput& in)
{
    // Map the detected genre to the closest factory starting point.
    juce::String base;
    switch (in.genre)
    {
        case 1:  base = "Rap / Trap Lead";     break;
        case 2:  base = "Modern Pop Vocal";    break;
        case 3:  base = "Rock Belter";         break;
        case 4:  base = "Podcast / Voice Over";break;
        default: base = "Modern Pop Vocal";    break;
    }
    applyFactoryPreset (apvts, base);

    // Fine-tune from measured features.
    // Heavier de-noise when the floor is high.
    const float noiseAmt = juce::jlimit (20.0f, 80.0f, (in.noiseFloorDb + 90.0f) * 1.2f);
    setParam (apvts, "noise_amount", noiseAmt);

    // Brighter sources need less air; dull sources need more.
    setParam (apvts, "air_on", 1.0f);
    setParam (apvts, "air_mid",  juce::jlimit (0.0f, 0.8f, 0.6f - in.brightness * 0.5f));
    setParam (apvts, "air_high", juce::jlimit (0.0f, 0.8f, 0.6f - in.brightness * 0.4f));

    // Strong sibilance -> lower de-esser threshold handled automatically, but
    // ensure it is enabled.
    setParam (apvts, "deess_on", 1.0f);
    setParam (apvts, "deess_auto", 1.0f);

    // Wide crest factor -> a touch more compression via manual mode tweak.
    if (in.crestDb > 16.0f)
    {
        setParam (apvts, "comp_mode", 6.0f);   // manual
        setParam (apvts, "comp_ratio", 4.5f);
        setParam (apvts, "comp_thresh", -22.0f);
        setParam (apvts, "comp_makeup", 4.0f);
    }
}

juce::File SmartPreset::getUserPresetDirectory()
{
    auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("VocalOne")
                   .getChildFile ("Presets");
    dir.createDirectory();
    return dir;
}

bool SmartPreset::saveUserPreset (juce::AudioProcessorValueTreeState& apvts,
                                  const juce::File& file)
{
    if (auto xml = apvts.copyState().createXml())
        return xml->writeTo (file);
    return false;
}

bool SmartPreset::loadUserPreset (juce::AudioProcessorValueTreeState& apvts,
                                  const juce::File& file)
{
    if (! file.existsAsFile())
        return false;
    if (auto xml = juce::XmlDocument::parse (file))
    {
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
        return true;
    }
    return false;
}
