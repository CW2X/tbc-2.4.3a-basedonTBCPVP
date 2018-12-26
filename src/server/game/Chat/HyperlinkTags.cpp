
#include "Hyperlinks.h"
#include "ObjectMgr.h"
#include "SpellInfo.h"
#include "SpellMgr.h"

static constexpr char HYPERLINK_DATA_DELIMITER = ':';

class HyperlinkDataTokenizer
{
public:
	HyperlinkDataTokenizer(char const* pos, size_t len) : _pos(pos), _len(len), _empty(false) {}

	template <typename T>
	bool TryConsumeTo(T& val)
	{
		if (_empty)
			return false;

		char const* firstPos = _pos;
		size_t thisLen = 0;
		// find next delimiter
		for (; _len && *_pos != HYPERLINK_DATA_DELIMITER; --_len, ++_pos, ++thisLen);
		if (_len)
			--_len, ++_pos; // skip the delimiter
		else
			_empty = true;

		return Trinity::Hyperlinks::LinkTags::base_tag::StoreTo(val, firstPos, thisLen);
	}

	bool IsEmpty() { return _empty; }

private:
	char const* _pos;
	size_t _len;
	bool _empty;
};

bool Trinity::Hyperlinks::LinkTags::item::StoreTo(ItemLinkData& val, char const* pos, size_t len)
{
	HyperlinkDataTokenizer t(pos, len);
	uint32 itemId, dummy;
	if (!t.TryConsumeTo(itemId))
		return false;
	val.Item = sObjectMgr->GetItemTemplate(itemId);
	return val.Item && t.TryConsumeTo(val.EnchantId) && t.TryConsumeTo(val.GemEnchantId[0]) && t.TryConsumeTo(val.GemEnchantId[1]) &&
		t.TryConsumeTo(val.GemEnchantId[2]) && t.TryConsumeTo(dummy) && t.TryConsumeTo(val.RandomPropertyId) && t.TryConsumeTo(val.RandomPropertySeed) &&
		t.IsEmpty() && !dummy;
}

bool Trinity::Hyperlinks::LinkTags::quest::StoreTo(QuestLinkData& val, char const* pos, size_t len)
{
	HyperlinkDataTokenizer t(pos, len);
	uint32 questId;
	if (!t.TryConsumeTo(questId))
		return false;
	return (val.Quest = sObjectMgr->GetQuestTemplate(questId)) && t.TryConsumeTo(val.QuestLevel) && t.IsEmpty();
}

bool Trinity::Hyperlinks::LinkTags::spell::StoreTo(SpellInfo const*& val, char const* pos, size_t len)
{
	HyperlinkDataTokenizer t(pos, len);
	uint32 spellId;
	if (!(t.TryConsumeTo(spellId) && t.IsEmpty()))
		return false;
	return !!(val = sSpellMgr->GetSpellInfo(spellId));
}
