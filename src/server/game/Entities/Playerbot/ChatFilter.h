#pragma once

using namespace std;

namespace ai
{
    class ChatFilter : public PlayerbotAIAware
    {
    public:
        ChatFilter(PlayerbotAI* ai) : PlayerbotAIAware(ai) {}
        virtual std::string Filter(std::string message);
        virtual ~ChatFilter() {}
    };

    class CompositeChatFilter : public ChatFilter
    {
    public:
        CompositeChatFilter(PlayerbotAI* ai);
        virtual ~CompositeChatFilter();
        std::string Filter(std::string message) override;

    private:
        list<ChatFilter*> filters;
    };
};
