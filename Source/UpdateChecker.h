#pragma once

#include <juce_events/juce_events.h>

// Asks GitHub for the latest published release and whether it is newer than this build. One instance is shared by
// every plug-in instance in the process, so the automatic check runs once per host session.
class UpdateChecker : public juce::ChangeBroadcaster, private juce::Thread
{
public:
    enum class Status { idle, checking, upToDate, available, failed };

    static constexpr const char* repo = "squeaksquad/Vibetron";

    UpdateChecker();
    ~UpdateChecker() override;

    void check();              // no-op while a check is already running
    void checkOnceAutomatically();

    // Message thread only.
    Status getStatus() const { return status; }
    juce::String getLatestVersion() const { return latestVersion; }
    void openDownload() const;  // this platform's installer (.pkg / .exe), or the release page if it has none

    static juce::String currentVersion();
    static bool isNewer (const juce::String& candidate, const juce::String& current);

private:
    void run() override;
    void finish (Status, juce::String version, juce::String url);

    Status status = Status::idle;
    juce::String latestVersion, downloadUrl;
    bool autoChecked = false;

    JUCE_DECLARE_WEAK_REFERENCEABLE (UpdateChecker)
};
