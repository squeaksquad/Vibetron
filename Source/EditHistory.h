#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "MeterSettings.h"

// Undo/redo for edits made in the plug-in's own UI. Parameter edits are captured from change gestures, which hosts
// don't send for automation, so playback never lands in the history. Rapid edits to the same control (a knob swept
// across detents, a slider drag) merge into one step.
class EditHistory : private juce::AudioProcessorParameter::Listener
{
public:
    EditHistory (juce::AudioProcessor&, std::function<void (const MeterSettings&)> applyMeterSettings);
    ~EditHistory() override;

    bool canUndo() const { return undoManager.canUndo(); }
    bool canRedo() const { return undoManager.canRedo(); }
    void undo();
    void redo();

    // Call after the UI changes meter calibration (message thread).
    void recordMeterChange (const MeterSettings& before, const MeterSettings& after);

    // Fires when the history changes, including after undo/redo.
    juce::ChangeBroadcaster& changes() { return undoManager; }

private:
    void parameterValueChanged (int, float) override {}
    void parameterGestureChanged (int index, bool starting) override;
    void record (std::unique_ptr<juce::UndoableAction>, int key);

    juce::AudioProcessor& processor;
    std::function<void (const MeterSettings&)> applyMeter;
    juce::UndoManager undoManager;
    std::map<int, float> gestureStartValues;
    bool applying = false;  // set while undo/redo writes parameters, so those gestures aren't recorded
    int lastKey = -1;
    double lastRecordMs = 0.0;
};
