#include "EditHistory.h"

namespace
{
    constexpr double mergeWindowMs = 600.0;
    constexpr int meterKey = -2;

    class ParameterAction : public juce::UndoableAction
    {
    public:
        ParameterAction (juce::AudioProcessorParameter& p, float fromValue, float toValue, bool& applyingFlag)
            : param (p), from (fromValue), to (toValue), applying (applyingFlag) {}

        bool perform() override { set (to); return true; }
        bool undo() override    { set (from); return true; }

    private:
        void set (float v)
        {
            if (param.getValue() == v)
                return;  // the first perform(): the edit has already happened
            const juce::ScopedValueSetter<bool> svs (applying, true);
            param.beginChangeGesture();  // so hosts writing automation in touch/latch see the change
            param.setValueNotifyingHost (v);
            param.endChangeGesture();
        }

        juce::AudioProcessorParameter& param;
        float from, to;
        bool& applying;
    };

    class MeterAction : public juce::UndoableAction
    {
    public:
        MeterAction (MeterSettings fromSettings, MeterSettings toSettings, std::function<void (const MeterSettings&)>& applyFn)
            : from (fromSettings), to (toSettings), apply (applyFn) {}

        bool perform() override { apply (to); return true; }
        bool undo() override    { apply (from); return true; }

    private:
        MeterSettings from, to;
        std::function<void (const MeterSettings&)>& apply;
    };
}

EditHistory::EditHistory (juce::AudioProcessor& p, std::function<void (const MeterSettings&)> applyMeterSettings)
    : processor (p), applyMeter (std::move (applyMeterSettings))
{
    for (auto* param : processor.getParameters())
        param->addListener (this);
}

EditHistory::~EditHistory()
{
    for (auto* param : processor.getParameters())
        param->removeListener (this);
}

void EditHistory::undo()
{
    lastKey = -1;  // the next edit starts a fresh step
    undoManager.undo();
}

void EditHistory::redo()
{
    lastKey = -1;
    undoManager.redo();
}

void EditHistory::parameterGestureChanged (int index, bool starting)
{
    // Only the editor starts gestures on the message thread; anything else is ignored.
    if (applying || ! juce::MessageManager::existsAndIsCurrentThread())
        return;

    auto* param = processor.getParameters()[index];
    if (starting)
    {
        gestureStartValues[index] = param->getValue();
        return;
    }

    const auto it = gestureStartValues.find (index);
    if (it == gestureStartValues.end())
        return;
    const float from = it->second, to = param->getValue();
    gestureStartValues.erase (it);
    if (from != to)
        record (std::make_unique<ParameterAction> (*param, from, to, applying), index);
}

void EditHistory::recordMeterChange (const MeterSettings& before, const MeterSettings& after)
{
    if (before != after)
        record (std::make_unique<MeterAction> (before, after, applyMeter), meterKey);
}

void EditHistory::record (std::unique_ptr<juce::UndoableAction> action, int key)
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    if (key != lastKey || now - lastRecordMs > mergeWindowMs)
        undoManager.beginNewTransaction();
    lastKey = key;
    lastRecordMs = now;
    undoManager.perform (action.release());
}
