
#include "../playerbot.h"
#include "PassiveMultiplier.h"

using namespace ai;

list<std::string> PassiveMultiplier::allowedActions;
list<std::string> PassiveMultiplier::allowedParts;

PassiveMultiplier::PassiveMultiplier(PlayerbotAI* ai) : Multiplier(ai, "passive")
{
    if (allowedActions.empty())
    {
        allowedActions.push_back("co");
        allowedActions.push_back("nc");
        allowedActions.push_back("reset ai");
        allowedActions.push_back("check mount state");
    }

    if (allowedParts.empty())
    {
        allowedParts.push_back("follow");
        allowedParts.push_back("stay");
        allowedParts.push_back("chat shortcut");
    }
}

float PassiveMultiplier::GetValue(Action* action) {
    if (!action)
        return 1.0f;

    std::string _name = action->getName();

    for (list<std::string>::iterator i = allowedActions.begin(); i != allowedActions.end(); i++)
    {
        if (_name == *i)
            return 1.0f;
    }

    for (list<std::string>::iterator i = allowedParts.begin(); i != allowedParts.end(); i++)
    {
        if (_name.find(*i) != std::string::npos)
            return 1.0f;
    }

    return 0;
}
