
#include "../../playerbot.h"
#include "UseItemAction.h"
#include "TradeData.h"

using namespace ai;

bool UseItemAction::Execute(Event event)
{
    std::string _name = event.getParam();
    if (_name.empty())
        _name = getName();

    list<Item*> items = AI_VALUE2(list<Item*>, "inventory items", _name);
    list<ObjectGuid> gos = chat->parseGameobjects(_name);

    if (gos.empty())
    {
        if (items.size() > 1)
        {
            list<Item*>::iterator i = items.begin();
            Item* itemTarget = *i++;
            Item* item = *i;
            return UseItemOnItem(item, itemTarget);
        }
        else if (!items.empty())
            return UseItemAuto(*items.begin());
    }
    else
    {
        if (items.empty())
            return UseGameObject(*gos.begin());
        else
            return UseItemOnGameObject(*items.begin(), *gos.begin());
    }

    ai->TellMaster("No items (or game objects) available");
    return false;
}

bool UseItemAction::UseGameObject(ObjectGuid guid)
{
    GameObject* go = ai->GetGameObject(guid);
    if (!go || !go->isSpawned())
        return false;

    go->Use(bot);
    std::ostringstream out; out << "Using " << chat->formatGameobject(go);
    ai->TellMasterNoFacing(out.str());
    return true;
}

bool UseItemAction::UseItemAuto(Item* item)
{
    return UseItem(item, ObjectGuid(), NULL);
}

bool UseItemAction::UseItemOnGameObject(Item* item, ObjectGuid go)
{
    return UseItem(item, go, NULL);
}

bool UseItemAction::UseItemOnItem(Item* item, Item* itemTarget)
{
    return UseItem(item, ObjectGuid(), itemTarget);
}

bool UseItemAction::UseItem(Item* item, ObjectGuid goGuid, Item* itemTarget)
{
    if (bot->CanUseItem(item) != EQUIP_ERR_OK)
        return false;

    if (bot->IsNonMeleeSpellCast(true))
        return false;

#ifdef LICH_KING
    if (bot->IsInCombat() && item->IsPotion() && bot->GetLastPotionId())
        return false;
#endif

    uint8 bagIndex = item->GetBagSlot();
    uint8 slot = item->GetSlot();
    uint8 cast_count = 1;
    ObjectGuid item_guid = item->GetGUID();
    uint32 glyphIndex = 0;
    uint8 unk_flags = 0;

    WorldPacket* const packet = new WorldPacket(CMSG_USE_ITEM, 1 + 1 + 1 + 4 + 8 + 4 + 1 + 8 + 1);
    *packet << bagIndex << slot << cast_count << uint32(0) << item_guid
        << glyphIndex << unk_flags;

    bool targetSelected = false;
    std::ostringstream out; out << "Using " << chat->formatItem(item->GetTemplate());
    if (item->GetTemplate()->Stackable)
    {
        uint32 count = item->GetCount();
        if (count > 1)
            out << " (" << count << " available) ";
        else
            out << " (the last one!)";
    }

    if (goGuid)
    {
        GameObject* go = ai->GetGameObject(goGuid);
        if (go && go->isSpawned())
        {
            uint32 targetFlag = TARGET_FLAG_UNIT_ENEMY;
            *packet << targetFlag;
            packet->appendPackGUID(goGuid.GetRawValue());
            out << " on " << chat->formatGameobject(go);
            targetSelected = true;
        }
    }

    if (itemTarget)
    {
        if (item->GetTemplate()->Class == ITEM_CLASS_GEM)
        {
            bool fit = SocketItem(itemTarget, item) || SocketItem(itemTarget, item, true);
            if (!fit)
                ai->TellMaster("Socket does not fit");

            delete packet;
            return fit;
        }
        else
        {
            uint32 targetFlag = TARGET_FLAG_ITEM;
            *packet << targetFlag;
            packet->appendPackGUID(itemTarget->GetGUID());
            out << " on " << chat->formatItem(itemTarget->GetTemplate());
            targetSelected = true;
        }
    }

    Player* master = GetMaster();
    if (!targetSelected && item->GetTemplate()->Class != ITEM_CLASS_CONSUMABLE && master)
    {
        Unit* masterSelection = master->GetSelectedUnit();
        if (masterSelection)
        {
            uint32 targetFlag = TARGET_FLAG_UNIT;
            *packet << targetFlag;
            packet->appendPackGUID(masterSelection->GetGUID());
            out << " on " << masterSelection->GetName();
            targetSelected = true;
        }
    }

    if(uint32 questid = item->GetTemplate()->StartQuest)
    {
        Quest const* qInfo = sObjectMgr->GetQuestTemplate(questid);
        if (qInfo)
        {
            WorldPacket* const _packet = new WorldPacket(CMSG_QUESTGIVER_ACCEPT_QUEST, 8+4+4);
            *_packet << item_guid;
            *_packet << questid;
            *_packet << uint32(0);
            bot->GetSession()->QueuePacket(_packet); // queue the packet to get around race condition
            std::ostringstream _out; _out << "Got quest " << chat->formatQuest(qInfo);
            ai->TellMasterNoFacing(_out.str());
            return true;
        }
    }

    MotionMaster &mm = *bot->GetMotionMaster();
    mm.Clear();
    bot->ClearUnitState( UNIT_STATE_CHASE );
    bot->ClearUnitState( UNIT_STATE_FOLLOW );

    if (bot->isMoving())
        return false;

    for (int i=0; i<MAX_ITEM_PROTO_SPELLS; i++)
    {
        uint32 spellId = item->GetTemplate()->Spells[i].SpellId;
        if (!spellId)
            continue;

        if (!ai->CanCastSpell(spellId, bot, TRIGGERED_NONE))
            continue;

        const SpellInfo* const pSpellInfo = sSpellMgr->GetSpellInfo(spellId);
        if (pSpellInfo->Targets & TARGET_FLAG_ITEM)
        {
            Item* itemForSpell = AI_VALUE2(Item*, "item for spell", spellId);
            if (!itemForSpell)
                continue;

            if (itemForSpell->GetEnchantmentId(TEMP_ENCHANTMENT_SLOT))
                continue;

            if (bot->GetTrader())
            {
                if (selfOnly)
                    return false;

                *packet << TARGET_FLAG_TRADE_ITEM << (uint8)1 << (uint64)TRADE_SLOT_NONTRADED;
                targetSelected = true;
                out << " on traded item";
            }
            else
            {
                *packet << TARGET_FLAG_ITEM;
                packet->appendPackGUID(itemForSpell->GetGUID());
                targetSelected = true;
                out << " on "<< chat->formatItem(itemForSpell->GetTemplate());
            }

            //Spell *spell = new Spell(bot, pSpellInfo, TRIGGERED_NONE, ObjectGuid::Empty, true);
            Spell *spell = new Spell(bot, pSpellInfo, TRIGGERED_NONE, ObjectGuid::Empty, nullptr, true);
            ai->WaitForSpellCast(spell);
            delete spell;
        }
        else
        {
            *packet << TARGET_FLAG_NONE;
            targetSelected = true;
            out << " on self";
        }
        break;
    }

    if (!targetSelected)
        return false;

    if (item->GetTemplate()->Class == ITEM_CLASS_CONSUMABLE && item->GetTemplate()->SubClass == ITEM_SUBCLASS_FOOD)
    {
        if (bot->IsInCombat())
            return false;

        ai->InterruptSpell();
        ai->SetNextCheckDelay(30000);
    }

    ai->TellMasterNoFacing(out.str());
    bot->GetSession()->QueuePacket(packet);
    return true;
}

bool UseItemAction::SocketItem(Item* item, Item* gem, bool replace)
{
    WorldPacket* const packet = new WorldPacket(CMSG_SOCKET_GEMS);
    *packet << item->GetGUID();

    bool fits = false;
    for (uint32 enchant_slot = SOCK_ENCHANTMENT_SLOT; enchant_slot < SOCK_ENCHANTMENT_SLOT + MAX_GEM_SOCKETS; ++enchant_slot)
    {
        uint8 SocketColor = item->GetTemplate()->Socket[enchant_slot-SOCK_ENCHANTMENT_SLOT].Color;
        GemPropertiesEntry const* gemProperty = sGemPropertiesStore.LookupEntry(gem->GetTemplate()->GemProperties);
        if (gemProperty && (gemProperty->color & SocketColor))
        {
            if (fits)
            {
                *packet << ObjectGuid();
                continue;
            }

            uint32 enchant_id = item->GetEnchantmentId(EnchantmentSlot(enchant_slot));
            if (!enchant_id)
            {
                *packet << gem->GetGUID();
                fits = true;
                continue;
            }

            SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
            if (!enchantEntry || !enchantEntry->GemID)
            {
                *packet << gem->GetGUID();
                fits = true;
                continue;
            }

            if (replace && enchantEntry->GemID != gem->GetTemplate()->ItemId)
            {
                *packet << gem->GetGUID();
                fits = true;
                continue;
            }

        }

        *packet << ObjectGuid();
    }

    if (fits)
    {
        std::ostringstream out; out << "Socketing " << chat->formatItem(item->GetTemplate());
        out << " with "<< chat->formatItem(gem->GetTemplate());
        ai->TellMasterNoFacing(out.str());

        bot->GetSession()->QueuePacket(packet);
    }
    return fits;
}


bool UseItemAction::isPossible()
{
    return getName() == "use" || AI_VALUE2(uint8, "item count", getName()) > 0;
}

bool UseSpellItemAction::isUseful()
{
    return AI_VALUE2(bool, "spell cast useful", getName());
}
