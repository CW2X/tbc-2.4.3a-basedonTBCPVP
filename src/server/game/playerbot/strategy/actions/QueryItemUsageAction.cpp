
#include "../../playerbot.h"
#include "QueryItemUsageAction.h"
#include "../values/ItemUsageValue.h"
//#include "../../../ahbot/AhBot.h"
#include "../../RandomPlayerbotMgr.h"


using namespace ai;


bool QueryItemUsageAction::Execute(Event event)
{
    WorldPacket& data = event.getPacket();
    if (!data.empty())
    {
        data.rpos(0);

        ObjectGuid guid;
        data >> guid;
        if (guid.GetRawValue() != bot->GetGUID())
            return false;

        uint32 received, created, isShowChatMessage, notUsed, itemId,
            suffixFactor, itemRandomPropertyId, count, invCount;
        uint8 bagSlot;

        data >> received;                               // 0=looted, 1=from npc
        data >> created;                                // 0=received, 1=created
        data >> isShowChatMessage;                                      // IsShowChatMessage
        data >> bagSlot;
                                                                // item slot, but when added to stack: 0xFFFFFFFF
        data >> notUsed;
        data >> itemId;
        data >> suffixFactor;
        data >> itemRandomPropertyId;
        data >> count;
        data >> invCount;

        ItemTemplate const *item = sObjectMgr->GetItemTemplate(itemId);
        if (!item)
            return false;

        std::ostringstream _out; _out << chat->formatItem(item, count);
        if (created)
            _out << " created";
        else if (received)
            _out << " received";
        ai->TellMaster(_out);

        QueryItemUsage(item);
        QueryQuestItem(itemId);
        return true;
    }

    std::string text = event.getParam();

    ItemIds items = chat->parseItems(text);
    QueryItemsUsage(items);
    return true;
}

bool QueryItemUsageAction::QueryItemUsage(ItemTemplate const *item)
{
    std::ostringstream _out; 
    _out << item->ItemId;
    ItemUsage usage = AI_VALUE2(ItemUsage, "item usage", _out.str());
    switch (usage)
    {
    case ITEM_USAGE_EQUIP:
        ai->TellMaster("Equip");
        return true;
    case ITEM_USAGE_REPLACE:
        ai->TellMaster("Equip (replace)");
        return true;
    case ITEM_USAGE_SKILL:
        ai->TellMaster("Tradeskill");
        return true;
    case ITEM_USAGE_USE:
        ai->TellMaster("Use");
        return true;
    case ITEM_USAGE_GUILD_TASK:
        ai->TellMaster("Guild task");
        return true;
    }

    return false;
}

void QueryItemUsageAction::QueryItemPrice(ItemTemplate const *item)
{
    if (!sRandomPlayerbotMgr.IsRandomBot(bot))
        return;

    if (item->Bonding == BIND_WHEN_PICKED_UP)
        return;

    list<Item*> items = InventoryAction::parseItems(item->Name1);
    if (!items.empty())
    {
        for (list<Item*>::iterator i = items.begin(); i != items.end(); ++i)
        {
            Item* sell = *i;
            //TC int32 sellPrice = sell->GetCount() * auctionbot.GetSellPrice(sell->GetTemplate()) * sRandomPlayerbotMgr.GetSellMultiplier(bot);
            int32 sellPrice = sell->GetCount() * sell->GetTemplate()->SellPrice * sRandomPlayerbotMgr.GetSellMultiplier(bot);
            std::ostringstream _out;
            _out << "Selling " << chat->formatItem(sell->GetTemplate(), sell->GetCount()) << " for " << chat->formatMoney(sellPrice);
            ai->TellMaster(_out.str());
        }
    }

    std::ostringstream _out; _out << item->ItemId;
    ItemUsage usage = AI_VALUE2(ItemUsage, "item usage", _out.str());
    if (usage == ITEM_USAGE_NONE)
        return;

    //TC   int32 buyPrice = auctionbot.GetBuyPrice(item) * sRandomPlayerbotMgr.GetBuyMultiplier(bot);
    int32 buyPrice = item->BuyPrice * sRandomPlayerbotMgr.GetBuyMultiplier(bot);
    if (buyPrice)
    {
        std::ostringstream ___out;
        ___out << "Will buy for " << chat->formatMoney(buyPrice);
        ai->TellMaster(___out.str());
    }
}

void QueryItemUsageAction::QueryItemsUsage(ItemIds items)
{
    for (ItemIds::iterator i = items.begin(); i != items.end(); i++)
    {
        ItemTemplate const *item = sObjectMgr->GetItemTemplate(*i);
        QueryItemUsage(item);
        QueryQuestItem(*i);
        QueryItemPrice(item);
    }
}

void QueryItemUsageAction::QueryQuestItem(uint32 itemId)
{
    Player* _bot = ai->GetBot();
    QuestStatusMap const& questMap = _bot->GetQuestStatusMap();
    for (QuestStatusMap::const_iterator i = questMap.begin(); i != questMap.end(); i++)
    {
        const Quest *questTemplate = sObjectMgr->GetQuestTemplate( i->first );
        if( !questTemplate )
            continue;

        uint32 questId = questTemplate->GetQuestId();
        QuestStatus status = _bot->GetQuestStatus(questId);
        if (status == QUEST_STATUS_INCOMPLETE || (status == QUEST_STATUS_COMPLETE && !_bot->GetQuestRewardStatus(questId)))
        {
            QuestStatusData const& questStatus = i->second;
            QueryQuestItem(itemId, questTemplate, &questStatus);
        }
    }
}


void QueryItemUsageAction::QueryQuestItem(uint32 itemId, const Quest *questTemplate, const QuestStatusData *questStatus)
{
    for (int i = 0; i < QUEST_OBJECTIVES_COUNT; i++)
    {
        if (questTemplate->RequiredItemId[i] != itemId)
            continue;

        int required = questTemplate->RequiredItemCount[i];
        int available = questStatus->ItemCount[i];

        if (!required)
            continue;

        ai->TellMaster(chat->formatQuestObjective(chat->formatQuest(questTemplate), available, required));
    }
}

