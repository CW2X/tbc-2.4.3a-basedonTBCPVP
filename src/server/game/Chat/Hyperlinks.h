

#ifndef TRINITY_HYPERLINKS_H
#define TRINITY_HYPERLINKS_H

#include "advstd.h"
#include "ObjectGuid.h"
#include <string>
#include <utility>

struct AchievementEntry;
struct GlyphPropertiesEntry;
struct GlyphSlotEntry;
struct ItemTemplate;
class SpellInfo;
class Quest;
struct TalentEntry;

namespace Trinity
{
namespace Hyperlinks
{
struct ItemLinkData
{
    ItemTemplate const* Item;
    uint32 EnchantId;
    uint32 GemEnchantId[3];
    int32 RandomPropertyId;
    int32 RandomPropertySeed;
};

struct QuestLinkData
{
    ::Quest const* Quest;
    uint8 QuestLevel;
};

namespace LinkTags {

    /************************** LINK TAGS ***************************************************\
    |* Link tags must abide by the following:                                               *|
    |* - MUST expose ::value_type typedef                                                   *|
    |*   - storage type is remove_cvref_t<value_type>                                       *|
    |* - MUST expose static ::tag method, void -> const char*                               *|
    |*   - this method SHOULD be constexpr                                                  *|
    |*   - returns identifier string for the link ("creature", "creature_entry", "item")    *|
    |* - MUST expose static ::StoreTo method, (storage&, char const*, size_t)               *|
    |*   - assign value_type& based on content of std::string(char const*, size_t)          *|
    |*   - return value indicates success/failure                                           *|
    |*   - for integral/string types this can be achieved by extending base_tag             *|
    \****************************************************************************************/
    struct base_tag
    {
        static bool StoreTo(std::string& val, char const* pos, size_t len)
        {
            val.assign(pos, len);
            return true;
        }

        template <typename T>
        static std::enable_if_t<advstd::is_integral_v<T> && advstd::is_unsigned_v<T>, bool> StoreTo(T& val, char const* pos, size_t len)
        {
            try { val = std::stoull(std::string(pos, len)); }
            catch (...) { return false; }
            return true;
        }

        template <typename T>
        static std::enable_if_t<advstd::is_integral_v<T> && advstd::is_signed_v<T>, bool> StoreTo(T& val, char const* pos, size_t len)
        {
            try { val = std::stoll(std::string(pos, len)); }
            catch (...) { return false; }
            return true;
        }

        static bool StoreTo(ObjectGuid& val, char const* pos, size_t len)
        {
            try { val.Set(std::stoul(std::string(pos, len), nullptr, 16)); }
            catch (...) { return false; }
            return true;
        }
    };

#define make_base_tag(ltag, type) struct ltag : public base_tag { using value_type = type; static constexpr char const* tag() { return #ltag; } }
    make_base_tag(area, uint32);
    make_base_tag(areatrigger, uint32);
    make_base_tag(creature, ObjectGuid::LowType);
    make_base_tag(creature_entry, uint32);
    make_base_tag(gameevent, uint32);
    make_base_tag(gameobject, ObjectGuid::LowType);
    make_base_tag(gameobject_entry, uint32);
    make_base_tag(itemset, uint32);
    make_base_tag(player, std::string const&);
    make_base_tag(skill, uint32);
    make_base_tag(taxinode, uint32);
    make_base_tag(tele, uint32);
    make_base_tag(title, uint32);
#undef make_base_tag

    struct TC_GAME_API item
    {
        using value_type = ItemLinkData const&;
        static constexpr char const* tag() { return "item"; }
        static bool StoreTo(ItemLinkData& val, char const* pos, size_t len);
    };

    struct TC_GAME_API quest
    {
        using value_type = QuestLinkData const&;
        static constexpr char const* tag() { return "quest"; }
        static bool StoreTo(QuestLinkData& val, char const* pos, size_t len);
    };

    struct TC_GAME_API spell
    {
        using value_type = SpellInfo const*;
        static constexpr char const* tag() { return "spell"; }
        static bool StoreTo(SpellInfo const*& val, char const* pos, size_t len);
    };
}

struct HyperlinkColor
{
    HyperlinkColor(uint32 c) : r(c >> 16), g(c >> 8), b(c), a(c >> 24) {}
    uint8 r, g, b, a;
    bool operator==(uint32 c) const
    {
        if ((c & 0xff) ^ b)
            return false;
        if (((c >>= 8) & 0xff) ^ g)
            return false;
        if (((c >>= 8) & 0xff) ^ r)
            return false;
        if ((c >>= 8) ^ a)
            return false;
        return true;
    }
};

struct HyperlinkInfo
{
    HyperlinkInfo(char const* n = nullptr, uint32 c = 0, char const* tS = nullptr, size_t tL = 0, char const* dS = nullptr, size_t dL = 0, char const* cS = nullptr, size_t cL = 0) :
        next(n), color(c), tag(tS, tL), data(dS, dL), text(cS, cL) {}

    explicit operator bool() { return next; }
    char const* const next;
    HyperlinkColor const color;
    std::pair<char const*, size_t> const tag;
    std::pair<char const*, size_t> const data;
    std::pair<char const*, size_t> const text;
};
HyperlinkInfo TC_GAME_API ParseHyperlink(char const* pos);
bool TC_GAME_API CheckAllLinks(std::string const&);

}
}

#endif