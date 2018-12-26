
#include "ChatCommandArgs.h"
#include "ChatCommand.h"
#include "ObjectMgr.h"

using namespace Trinity::ChatCommands;

struct GameTeleVisitor
{
    using value_type = GameTele const*;
    value_type operator()(Hyperlink<tele> tele) const { return sObjectMgr->GetGameTele(tele); }
    value_type operator()(std::string const& tele) const { return sObjectMgr->GetGameTele(tele); }
};
char const* Trinity::ChatCommands::ArgInfo<GameTele const*>::TryConsume(GameTele const*& data, char const* args)
{
    Variant<Hyperlink<tele>, std::string> val;
    if ((args = CommandArgsConsumerSingle<decltype(val)>::TryConsumeTo(val, args)))
        data = boost::apply_visitor(GameTeleVisitor(), val);
    return args;
}

struct BoolVisitor
{
    using value_type = bool;
    value_type operator()(uint32 i) const { return !!i; }
    value_type operator()(ExactSequence<'o', 'n'>) const { return true; }
    value_type operator()(ExactSequence<'o', 'f', 'f'>) const { return false; }
};
char const* Trinity::ChatCommands::ArgInfo<bool>::TryConsume(bool& data, char const* args)
{
    Variant<uint32, ExactSequence<'o', 'n'>, ExactSequence<'o', 'f', 'f'>> val;
    if ((args = CommandArgsConsumerSingle<decltype(val)>::TryConsumeTo(val, args)))
        data = boost::apply_visitor(BoolVisitor(), val);
    return args;
}
