#include "UpdateChecker.h"

UpdateChecker::UpdateChecker() : juce::Thread ("Vibetron update check") {}

UpdateChecker::~UpdateChecker()
{
    stopThread (10000);  // at most one request, which times out on its own
}

juce::String UpdateChecker::currentVersion()
{
    return JucePlugin_VersionString;
}

bool UpdateChecker::isNewer (const juce::String& candidate, const juce::String& current)
{
    auto parts = [] (const juce::String& v)
    {
        juce::StringArray a;
        a.addTokens (v.trimCharactersAtStart ("vV"), ".", {});
        return a;
    };
    const auto a = parts (candidate), b = parts (current);
    for (int i = 0; i < juce::jmax (a.size(), b.size()); ++i)
    {
        const int x = a[i].getIntValue(), y = b[i].getIntValue();
        if (x != y)
            return x > y;
    }
    return false;
}

void UpdateChecker::checkOnceAutomatically()
{
    if (! autoChecked)
    {
        autoChecked = true;
        check();
    }
}

void UpdateChecker::check()
{
    if (status == Status::checking || isThreadRunning())
        return;
    status = Status::checking;
    sendChangeMessage();
    startThread();
}

void UpdateChecker::run()
{
    int statusCode = 0;
    const auto url = juce::URL (juce::String ("https://api.github.com/repos/") + repo + "/releases/latest");
    const auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                             .withExtraHeaders ("Accept: application/vnd.github+json\r\nUser-Agent: Vibetron/" + currentVersion())
                             .withConnectionTimeoutMs (8000)
                             .withStatusCode (&statusCode);

    const auto stream = url.createInputStream (options);
    if (stream == nullptr || statusCode != 200)
        return finish (Status::failed, {}, {});

    const auto release = juce::JSON::parse (stream->readEntireStreamAsString());
    const auto version = release["tag_name"].toString().trimCharactersAtStart ("vV");
    if (version.isEmpty())
        return finish (Status::failed, {}, {});

   #if JUCE_WINDOWS
    const char* installerSuffix = ".exe";
   #else
    const char* installerSuffix = ".pkg";
   #endif
    juce::String download = release["html_url"].toString();
    if (const auto* assets = release["assets"].getArray())
        for (const auto& asset : *assets)
            if (asset["name"].toString().endsWithIgnoreCase (installerSuffix))
                download = asset["browser_download_url"].toString();

    finish (isNewer (version, currentVersion()) ? Status::available : Status::upToDate, version, download);
}

void UpdateChecker::finish (Status result, juce::String version, juce::String url)
{
    juce::MessageManager::callAsync ([weak = juce::WeakReference<UpdateChecker> (this), result, version, url]
    {
        if (auto* self = weak.get())
        {
            self->status = result;
            if (version.isNotEmpty())
            {
                self->latestVersion = version;
                self->downloadUrl = url;
            }
            self->sendChangeMessage();
        }
    });
}

void UpdateChecker::openDownload() const
{
    const auto target = downloadUrl.isNotEmpty() ? downloadUrl
                                                 : juce::String ("https://github.com/") + repo + "/releases/latest";
    juce::URL (target).launchInDefaultBrowser();
}
