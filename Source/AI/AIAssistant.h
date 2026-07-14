#pragma once

#include <JuceHeader.h>
#include <functional>
#include <vector>

/**
    AIAssistant — a chat interface backed by an OpenAI-compatible LLM endpoint.

    The assistant receives the analyser snapshot as context and can answer
    mixing questions or return structured suggestions (as JSON) that the host
    can apply to the parameter tree. Networking runs on a background thread so
    the UI never blocks.
*/
class AIAssistant : private juce::Thread
{
public:
    struct Message
    {
        juce::String role;      // "system" | "user" | "assistant"
        juce::String content;
    };

    struct Config
    {
        juce::String baseUrl = "https://api.openai.com/v1";
        juce::String apiKey;
        juce::String model   = "gpt-4o-mini";
    };

    AIAssistant();
    ~AIAssistant() override;

    void setConfig (const Config& c);
    Config getConfig() const;

    /** Context string built from the analyser snapshot (thread-safe). */
    void setAnalysisContext (const juce::String& ctx);

    /** Send a user prompt; the reply arrives via onResponse on the message thread. */
    void sendMessage (const juce::String& userText);

    void clearHistory();
    std::vector<Message> getHistory() const;

    std::function<void (juce::String reply)> onResponse;
    std::function<void (juce::String error)> onError;

private:
    void run() override;
    juce::String buildRequestBody (const juce::String& userText) const;
    juce::String performRequest (const juce::String& body, juce::String& errorOut) const;

    mutable juce::CriticalSection lock;
    Config config;
    juce::String analysisContext;
    std::vector<Message> history;

    juce::String pendingUser;
    std::atomic<bool> hasPending { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AIAssistant)
};
