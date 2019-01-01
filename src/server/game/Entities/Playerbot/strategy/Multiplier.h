#pragma once
#include "Action.h"

namespace ai
{
    class Multiplier : public AiNamedObject
    {
    public:
        Multiplier(PlayerbotAI* ai, std::string name) : AiNamedObject(ai, name) {}
        virtual ~Multiplier() = default;

    public:
        virtual float GetValue(Action* action) { return 1.0f; }
    };

}
