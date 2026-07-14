#include "AIAssistant.h"

AIAssistant::AIAssistant()
    : juce::Thread ("VocalOne-AIAssistant")
{
    history.push_back ({ "system",
        "You are VocalOne's built-in mixing assistant. You help the user process a "
        "vocal recording. Be concise and practical. When the user asks you to change "
        "settings, suggest concrete parameter values for the modules "
        "(Gate, DeEsser, EQ, Compressor, Saturation, Reverb, Delay, etc.)." });
}

AIAssistant::~AIAssistant()
{
    stopThread (2000);
}

void AIAssistant::setConfig (const Config& c)
{
    const juce::ScopedLock sl (lock);
    config = c;
}

AIAssistant::Config AIAssistant::getConfig() const
{
    const juce::ScopedLock sl (lock);
    return config;
}

void AIAssistant::setAnalysisContext (const juce::String& ctx)
{
    const juce::ScopedLock sl (lock);
    analysisContext = ctx;
}

void AIAssistant::sendMessage (const juce::String& userText)
{
    {
        const juce::ScopedLock sl (lock);
        history.push_back ({ "user", userText });
        pendingUser = userText;
        hasPending = true;
    }
    if (! isThreadRunning())
        startThread();
    else
        notify();
}

void AIAssistant::clearHistory()
{
    const juce::ScopedLock sl (lock);
    if (! history.empty())
        history.resize (1);   // keep the system message
}

std::vector<AIAssistant::Message> AIAssistant::getHistory() const
{
    const juce::ScopedLock sl (lock);
    return history;
}

juce::String AIAssistant::buildRequestBody (const juce::String& /*userText*/) const
{
    juce::var messages (juce::Array<juce::var>{});
    auto* arr = messages.getArray();

    juce::String ctx;
    std::vector<Message> localHistory;
    {
        const juce::ScopedLock sl (lock);
        ctx = analysisContext;
        localHistory = history;
    }

    for (size_t i = 0; i < localHistory.size(); ++i)
    {
        auto& m = localHistory[i];
        juce::DynamicObject::Ptr obj = new juce::DynamicObject();
        obj->setProperty ("role", m.role);

        // inject analysis context into the (first) system message
        if (i == 0 && m.role == "system" && ctx.isNotEmpty())
            obj->setProperty ("content", m.content + "\n\nCurrent analysis:\n" + ctx);
        else
            obj->setProperty ("content", m.content);

        arr->add (juce::var (obj.get()));
    }

    juce::DynamicObject::Ptr root = new juce::DynamicObject();
    {
        const juce::ScopedLock sl (lock);
        root->setProperty ("model", config.model);
    }
    root->setProperty ("messages", messages);
    root->setProperty ("temperature", 0.5);

    return juce::JSON::toString (juce::var (root.get()));
}

juce::String AIAssistant::performRequest (const juce::String& body, juce::String& errorOut) const
{
    Config c;
    {
        const juce::ScopedLock sl (lock);
        c = config;
    }

    if (c.apiKey.isEmpty())
    {
        errorOut = "No API key configured for the AI assistant.";
        return {};
    }

    juce::URL url (c.baseUrl + "/chat/completions");
    url = url.withPOSTData (body);

    juce::StringPairArray responseHeaders;
    int statusCode = 0;

    juce::String headers;
    headers << "Content-Type: application/json\r\n"
            << "Authorization: Bearer " << c.apiKey << "\r\n";

    auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inPostData)
                       .withExtraHeaders (headers)
                       .withConnectionTimeoutMs (15000)
                       .withResponseHeaders (&responseHeaders)
                       .withStatusCode (&statusCode)
                       .withHttpRequestCmd ("POST");

    std::unique_ptr<juce::InputStream> stream (url.createInputStream (options));
    if (stream == nullptr)
    {
        errorOut = "Could not connect to the AI endpoint.";
        return {};
    }

    const juce::String response = stream->readEntireStreamAsString();

    if (statusCode >= 400)
    {
        errorOut = "AI endpoint returned HTTP " + juce::String (statusCode) + ": " + response;
        return {};
    }

    // Parse the OpenAI-style response.
    auto parsed = juce::JSON::parse (response);
    if (auto* obj = parsed.getDynamicObject())
    {
        if (obj->hasProperty ("choices"))
        {
            auto choices = obj->getProperty ("choices");
            if (auto* carr = choices.getArray())
                if (carr->size() > 0)
                    if (auto* first = (*carr)[0].getDynamicObject())
                        if (auto* msg = first->getProperty ("message").getDynamicObject())
                            return msg->getProperty ("content").toString();
        }
        if (obj->hasProperty ("error"))
        {
            errorOut = "AI error: " + obj->getProperty ("error").toString();
            return {};
        }
    }

    errorOut = "Unexpected AI response format.";
    return {};
}

void AIAssistant::run()
{
    while (! threadShouldExit())
    {
        if (hasPending.exchange (false))
        {
            juce::String userText;
            {
                const juce::ScopedLock sl (lock);
                userText = pendingUser;
            }

            const juce::String body = buildRequestBody (userText);
            juce::String error;
            const juce::String reply = performRequest (body, error);

            if (error.isNotEmpty())
            {
                juce::MessageManager::callAsync ([this, error]
                {
                    if (onError) onError (error);
                });
            }
            else
            {
                {
                    const juce::ScopedLock sl (lock);
                    history.push_back ({ "assistant", reply });
                }
                juce::MessageManager::callAsync ([this, reply]
                {
                    if (onResponse) onResponse (reply);
                });
            }
        }

        wait (200);
    }
}
