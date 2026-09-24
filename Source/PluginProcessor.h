#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Modes.h"
#include "TruePeakLimiter.h"
#include "MeterSettings.h"
#include "EditHistory.h"
#include "UpdateChecker.h"

class VibetronProcessor : public juce::AudioProcessor
{
public:
    VibetronProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;
    using AudioProcessor::processBlockBypassed;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Lock-free meter feed for the editor.
    struct MeterFeed
    {
        std::atomic<float> rectified[2] {};  // short-term full-wave rectified average, post-processing
        std::atomic<float> peak[2] {};       // highest |sample| since the editor last took it
        std::atomic<uint32_t> blocks { 0 };
        std::atomic<int> channels { 2 };
    };

    MeterFeed meters;
    juce::AudioProcessorValueTreeState apvts;

    static constexpr const char* modeId = "mode";
    static constexpr const char* bypassId = "bypass";
    static constexpr const char* tameId = "tame";

    // Meter calibration lives in the saved state as properties (message thread only).
    MeterSettings getMeterSettings() const;
    void setMeterSettings (const MeterSettings&);

    EditHistory history { *this, [this] (const MeterSettings& m) { setMeterSettings (m); } };
    juce::SharedResourcePointer<UpdateChecker> updates;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void process (juce::AudioBuffer<float>&, bool forceBypass);

    std::atomic<float>* modeParam = nullptr;
    std::atomic<float>* tameParam = nullptr;
    juce::AudioParameterBool* bypassParam = nullptr;
    TruePeakLimiter limiter;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> gain;
    float detector[2] {}, detectorCoeff = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VibetronProcessor)
};
