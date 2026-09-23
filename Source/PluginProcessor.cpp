#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    float modeGain (int mode)
    {
        return juce::Decibels::decibelsToGain (kModes[(size_t) juce::jlimit (0, numModes - 1, mode)].gainDb);
    }
}

VibetronProcessor::VibetronProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "VibetronState", createLayout())
{
    modeParam = apvts.getRawParameterValue (modeId);
    tameParam = apvts.getRawParameterValue (tameId);
    bypassParam = dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter (bypassId));
}

juce::AudioProcessorValueTreeState::ParameterLayout VibetronProcessor::createLayout()
{
    juce::StringArray modeNames;
    for (const auto& m : kModes)
        modeNames.add (m.name);

    return { std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { modeId, 1 }, "Mode", modeNames, 0),
             std::make_unique<juce::AudioParameterBool> (juce::ParameterID { bypassId, 1 }, "Operate Bypass", false),
             std::make_unique<juce::AudioParameterBool> (juce::ParameterID { tameId, 1 }, "Output Tame", false) };
}

bool VibetronProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void VibetronProcessor::prepareToPlay (double sampleRate, int)
{
    gain.reset (sampleRate, 0.05);
    gain.setCurrentAndTargetValue (bypassParam->get() ? 1.0f : modeGain ((int) modeParam->load()));
    detectorCoeff = (float) std::exp (-1.0 / (0.005 * sampleRate));
    detector[0] = detector[1] = 0.0f;
    meters.channels = getTotalNumOutputChannels();
    limiter.prepare (sampleRate, -0.1f);
    setLatencySamples (limiter.getLatencySamples());
}

void VibetronProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    process (buffer, false);
}

// Host-level bypass: everything off (mode gain and TAME), same path so latency and metering stay consistent.
void VibetronProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    process (buffer, true);
}

void VibetronProcessor::process (juce::AudioBuffer<float>& buffer, bool forceBypass)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int numChannels = juce::jmin (getTotalNumInputChannels(), getTotalNumOutputChannels(), 2);

    for (auto ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    // OPERATE only engages the mode gain; TAME alone decides whether the limiter runs.
    const bool gainBypassed = forceBypass || bypassParam->get();
    gain.setTargetValue (gainBypassed ? 1.0f : modeGain ((int) modeParam->load()));

    if (gain.isSmoothing())
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const auto g = gain.getNextValue();
            for (int ch = 0; ch < numChannels; ++ch)
                buffer.getWritePointer (ch)[i] *= g;
        }
    }
    else
    {
        buffer.applyGain (0, numSamples, gain.getTargetValue());
    }

    limiter.process (buffer, numChannels, ! forceBypass && tameParam->load() > 0.5f);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        const auto* x = buffer.getReadPointer (ch);
        float d = detector[ch], pk = 0.0f;
        for (int i = 0; i < numSamples; ++i)
        {
            const auto a = std::abs (x[i]);
            d = a + detectorCoeff * (d - a);
            pk = juce::jmax (pk, a);
        }
        detector[ch] = d;
        meters.rectified[ch].store (d, std::memory_order_relaxed);
        // Hold the highest peak until the editor takes it; the editor compares it with the meter reference.
        float held = meters.peak[ch].load (std::memory_order_relaxed);
        while (pk > held && ! meters.peak[ch].compare_exchange_weak (held, pk, std::memory_order_relaxed)) {}
    }
    meters.channels.store (numChannels, std::memory_order_relaxed);
    meters.blocks.fetch_add (1, std::memory_order_release);
}

MeterSettings VibetronProcessor::getMeterSettings() const
{
    MeterSettings m;
    const auto& st = apvts.state;
    m.refDbfs      = (float) st.getProperty ("vuRefDbfs", m.refDbfs);
    m.riseMs       = (float) st.getProperty ("vuRiseMs", m.riseMs);
    m.fallMs       = (float) st.getProperty ("vuFallMs", m.fallMs);
    m.overshootPct = (float) st.getProperty ("vuOvershootPct", m.overshootPct);
    m.peakMode     = (bool) st.getProperty ("vuPeakMode", m.peakMode);
    return m;
}

void VibetronProcessor::setMeterSettings (const MeterSettings& m)
{
    auto& st = apvts.state;
    st.setProperty ("vuRefDbfs", m.refDbfs, nullptr);
    st.setProperty ("vuRiseMs", m.riseMs, nullptr);
    st.setProperty ("vuFallMs", m.fallMs, nullptr);
    st.setProperty ("vuOvershootPct", m.overshootPct, nullptr);
    st.setProperty ("vuPeakMode", m.peakMode, nullptr);
}

juce::AudioProcessorEditor* VibetronProcessor::createEditor()
{
    return new VibetronEditor (*this);
}

void VibetronProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void VibetronProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VibetronProcessor();
}
