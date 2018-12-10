
#include "Common.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "World.h"
#include "ObjectAccessor.h"
#include "Log.h"
#include "Opcodes.h"
#include "Player.h"
#include "Item.h"
#include "SocialMgr.h"
#include "Language.h"
#include "LogsDatabaseAccessor.h"
#include "TradeData.h"
#include "Spell.h"
#include "RBAC.h"

void WorldSession::SendTradeStatus(TradeStatusInfo const& info)
{
    //LK OK
    WorldPacket data(SMSG_TRADE_STATUS, 13);
    data << uint32(info.Status);

    switch (info.Status)
    {
        case TRADE_STATUS_BEGIN_TRADE:
            data << uint64(info.TraderGuid);                // CGTradeInfo::m_tradingPlayer
            break;
        case TRADE_STATUS_OPEN_WINDOW:
            data << uint32(0);                              // CGTradeInfo::m_tradeID
            break;
        case TRADE_STATUS_CLOSE_WINDOW:
            data << uint32(info.Result);                    // InventoryResult
            data << uint8(info.IsTargetResult);             // bool isTargetError; used for: EQUIP_ERR_BAG_FULL, EQUIP_ERR_CANT_CARRY_MORE_OF_THIS, EQUIP_ERR_MISSING_REAGENT, EQUIP_ERR_ITEM_MAX_LIMIT_CATEGORY_COUNT_EXCEEDED
            data << uint32(info.ItemLimitCategoryId);       // ItemLimitCategory.dbc entry
            break;
        case TRADE_STATUS_ONLY_CONJURED:
            data << uint8(info.Slot);                       // Trade slot; -1 here clears CGTradeInfo::m_tradeMoney
            break;
        default:
            break;
    }

    SendPacket(&data);
}

void WorldSession::HandleIgnoreTradeOpcode(WorldPacket& /*recvPacket*/)
{
    // TODO
}

void WorldSession::HandleBusyTradeOpcode(WorldPacket& /*recvPacket*/)
{
    // TODO
}

void WorldSession::SendUpdateTrade(bool trader_data /*= true*/)
{
    TradeData* view_trade = trader_data ? _player->GetTradeData()->GetTraderData() : _player->GetTradeData();

    //LK ok
    WorldPacket data(SMSG_TRADE_STATUS_EXTENDED, (100));    // guess size
    data << uint8(trader_data);                             // can be different (only seen 0 and 1)
    data << uint32(0);                                      // added in 2.4.0, this value must be equal to value from TRADE_STATUS_OPEN_WINDOW status packet (different value for different players to block multiple trades?)
    data << uint32(TRADE_SLOT_COUNT);                       // trade slots count/number?, = next field in most cases
    data << uint32(TRADE_SLOT_COUNT);                       // trade slots count/number?, = prev field in most cases
    data << uint32(view_trade->GetMoney());                 // trader gold
    data << uint32(view_trade->GetSpell());                 // spell cast on lowest slot item

    for(uint8 i = 0; i < TRADE_SLOT_COUNT; i++)
    {
        data << (uint8) i;                                  // trade slot number, if not specified, then end of packet

        if (Item* item = view_trade->GetItem(TradeSlots(i)))
        {
            data << uint32(item->GetTemplate()->ItemId);       // entry
            data << uint32(item->GetTemplate()->DisplayInfoID);// display id
            data << uint32(item->GetCount());               // stack count
                                                            // wrapped: hide stats but show giftcreator name
            data << uint32(item->HasFlag(ITEM_FIELD_FLAGS, ITEM_FIELD_FLAG_WRAPPED) ? 1 : 0);
            data << uint64(item->GetGuidValue(ITEM_FIELD_GIFTCREATOR));
            // perm. enchantment and gems
            data << uint32(item->GetEnchantmentId(PERM_ENCHANTMENT_SLOT));
            for (uint32 enchant_slot = SOCK_ENCHANTMENT_SLOT; enchant_slot < SOCK_ENCHANTMENT_SLOT + MAX_GEM_SOCKETS; ++enchant_slot)
                data << uint32(item->GetEnchantmentId(EnchantmentSlot(enchant_slot)));
                                                            // creator
            data << uint64(item->GetGuidValue(ITEM_FIELD_CREATOR));
            data << uint32(item->GetSpellCharges());        // charges
            data << uint32(item->GetItemSuffixFactor());    // SuffixFactor
            data << uint32(item->GetItemRandomPropertyId());// random properties id
            data << uint32(item->GetTemplate()->LockID);    // lock id
                                                            // max durability
            data << uint32(item->GetUInt32Value(ITEM_FIELD_MAXDURABILITY));
                                                            // durability
            data << uint32(item->GetUInt32Value(ITEM_FIELD_DURABILITY));
        }
        else
        {
            for(uint8 j = 0; j < 18; j++)
                data << uint32(0);
        }
    }
    SendPacket(&data);
}

//==============================================================
// transfer the items to the players

void WorldSession::moveItems(std::vector<Item*> myItems, std::vector<Item*> hisItems)
{
    Player* trader = _player->GetTrader();
    if (!trader)
        return;

    for(int i=0; i<TRADE_SLOT_TRADED_COUNT; i++)
    {
        ItemPosCountVec traderDst;
        ItemPosCountVec playerDst;
        bool traderCanTrade = (myItems[i]==nullptr || trader->CanStoreItem( NULL_BAG, NULL_SLOT, traderDst, myItems[i], false ) == EQUIP_ERR_OK);
        bool playerCanTrade = (hisItems[i]==nullptr || _player->CanStoreItem( NULL_BAG, NULL_SLOT, playerDst, hisItems[i], false ) == EQUIP_ERR_OK);
        if(traderCanTrade && playerCanTrade )
        {
            // Ok, if trade item exists and can be stored
            // If we trade in both directions we had to check, if the trade will work before we actually do it
            // A roll back is not possible after we stored it
            if(myItems[i])
            {
#ifdef LICH_KING
                // adjust time (depends on /played)
                if (myItems[i]->HasFlag(ITEM_FIELD_FLAGS, ITEM_FIELD_FLAG_BOP_TRADEABLE))
                    myItems[i]->SetUInt32Value(ITEM_FIELD_CREATE_PLAYED_TIME, trader->GetTotalPlayedTime() - (_player->GetTotalPlayedTime() - myItems[i]->GetUInt32Value(ITEM_FIELD_CREATE_PLAYED_TIME)));
#endif
                // store
                trader->MoveItemToInventory(traderDst, myItems[i], true, true);
            }
            if(hisItems[i])
            {
#ifdef LICH_KING
                // adjust time (depends on /played)
                if (hisItems[i]->HasFlag(ITEM_FIELD_FLAGS, ITEM_FIELD_FLAG_BOP_TRADEABLE))
                    hisItems[i]->SetUInt32Value(ITEM_FIELD_CREATE_PLAYED_TIME, _player->GetTotalPlayedTime() - (trader->GetTotalPlayedTime() - hisItems[i]->GetUInt32Value(ITEM_FIELD_CREATE_PLAYED_TIME)));
#endif
                // store
                _player->MoveItemToInventory( playerDst, hisItems[i], true, true);
            }
        }
        else
        {
            // in case of fatal error log error message
            // return the already removed items to the original owner
            if(myItems[i])
            {
                if(!traderCanTrade)
                    TC_LOG_FATAL("network","Trader %u can't store item: %u", _player->GetGUID().GetCounter(), myItems[i]->GetGUID().GetCounter());
                if(_player->CanStoreItem( NULL_BAG, NULL_SLOT, playerDst, myItems[i], false ) == EQUIP_ERR_OK)
                    _player->MoveItemToInventory(playerDst, myItems[i], true, true);
                else
                    TC_LOG_FATAL("network","Player %u can't take item back: %u", _player->GetGUID().GetCounter(), myItems[i]->GetGUID().GetCounter());
            }
            // return the already removed items to the original owner
            if (hisItems[i])
            {
                if(!playerCanTrade)
                    TC_LOG_FATAL("network","Player %u can't store item: %u", _player->GetGUID().GetCounter(), hisItems[i]->GetGUID().GetCounter());
                if(trader->CanStoreItem( NULL_BAG, NULL_SLOT, traderDst, hisItems[i], false ) == EQUIP_ERR_OK)
                    trader->MoveItemToInventory(traderDst, hisItems[i], true, true);
                else
                    TC_LOG_FATAL("network","Trader %u can't take item back: %u", _player->GetGUID().GetCounter(), hisItems[i]->GetGUID().GetCounter());
            }
        }
    }
}

//==============================================================

static void setAcceptTradeMode(TradeData* myTrade, TradeData* hisTrade, std::vector<Item*>& myItems, std::vector<Item*>& hisItems)
{
    myTrade->SetInAcceptProcess(true);
    hisTrade->SetInAcceptProcess(true);

    // store items in local list and set 'in-trade' flag
    for (uint8 i = 0; i < TRADE_SLOT_TRADED_COUNT; ++i)
    {
        if (Item* item = myTrade->GetItem(TradeSlots(i)))
        {
            TC_LOG_DEBUG("network", "player trade item %u bag: %u slot: %u", item->GetGUID().GetCounter(), item->GetBagSlot(), item->GetSlot());
            //Can return nullptr
            myItems[i] = item;
            myItems[i]->SetInTrade();
        }

        if (Item* item = hisTrade->GetItem(TradeSlots(i)))
        {
            TC_LOG_DEBUG("network", "partner trade item %u bag: %u slot: %u", item->GetGUID().GetCounter(), item->GetBagSlot(), item->GetSlot());
            hisItems[i] = item;
            hisItems[i]->SetInTrade();
        }
    }
}

static void clearAcceptTradeMode(TradeData* myTrade, TradeData* hisTrade)
{
    myTrade->SetInAcceptProcess(false);
    hisTrade->SetInAcceptProcess(false);
}

static void clearAcceptTradeMode(std::vector<Item*>& myItems, std::vector<Item*>& hisItems)
{
    // clear 'in-trade' flag
    for (uint8 i = 0; i < TRADE_SLOT_TRADED_COUNT; ++i)
    {
        if (myItems[i])
            myItems[i]->SetInTrade(false);
        if (hisItems[i])
            hisItems[i]->SetInTrade(false);
    }
}

void WorldSession::HandleAcceptTradeOpcode(WorldPacket& /*recvPacket*/)
{
    TradeData* my_trade = _player->m_trade;
    if (!my_trade)
        return;

    Player* trader = my_trade->GetTrader();

    TradeData* his_trade = trader->m_trade;
    if (!his_trade)
        return;

    std::vector<Item*> myItems(TRADE_SLOT_TRADED_COUNT, nullptr);
    std::vector<Item*> hisItems(TRADE_SLOT_TRADED_COUNT, nullptr);

    // set before checks for propertly undo at problems (it already set in to client)
    my_trade->SetAccepted(true);

    TradeStatusInfo info;
    if (!_player->IsWithinDistInMap(trader, TRADE_DISTANCE, false))
    {
        info.Status = TRADE_STATUS_TARGET_TO_FAR;
        SendTradeStatus(info);
        my_trade->SetAccepted(false);
        return;
    }

    // not accept case incorrect money amount
    if (!_player->HasEnoughMoney(my_trade->GetMoney()))
    {
        info.Status = TRADE_STATUS_CLOSE_WINDOW;
        info.Result = EQUIP_ERR_NOT_ENOUGH_MONEY;
        SendTradeStatus(info);
        my_trade->SetAccepted(false, true);
        return;
    }

    // not accept case incorrect money amount
    if (!trader->HasEnoughMoney(his_trade->GetMoney()))
    {
        info.Status = TRADE_STATUS_CLOSE_WINDOW;
        info.Result = EQUIP_ERR_NOT_ENOUGH_MONEY;
        trader->GetSession()->SendTradeStatus(info);
        his_trade->SetAccepted(false, true);
        return;
    }

    if (_player->GetMoney() >= MAX_MONEY_AMOUNT - his_trade->GetMoney())
    {
        info.Status = TRADE_STATUS_CLOSE_WINDOW;
        info.Result = EQUIP_ERR_TOO_MUCH_GOLD;
        SendTradeStatus(info);
        my_trade->SetAccepted(false, true);
        return;
    }

    if (trader->GetMoney() >= MAX_MONEY_AMOUNT - my_trade->GetMoney())
    {
        info.Status = TRADE_STATUS_CLOSE_WINDOW;
        info.Result = EQUIP_ERR_TOO_MUCH_GOLD;
        trader->GetSession()->SendTradeStatus(info);
        his_trade->SetAccepted(false, true);
        return;
    }

    // not accept if some items now can't be trade (cheating)
    for (uint8 i = 0; i < TRADE_SLOT_TRADED_COUNT; ++i)
    {
        if (Item* item = my_trade->GetItem(TradeSlots(i)))
        {
            if (!item->CanBeTraded(false, true))
            {
                info.Status = TRADE_STATUS_TRADE_CANCELED;
                SendTradeStatus(info);
                return;
            }

            if (item->IsBindedNotWith(trader))
            {
                info.Status = TRADE_STATUS_CLOSE_WINDOW;
                info.Result = EQUIP_ERR_CANNOT_TRADE_THAT;
                SendTradeStatus(info);
                return;
            }
        }

        if (Item* item = his_trade->GetItem(TradeSlots(i)))
        {
            if (!item->CanBeTraded(false, true))
            {
                info.Status = TRADE_STATUS_TRADE_CANCELED;
                SendTradeStatus(info);
                return;
            }
            //if (item->IsBindedNotWith(_player))   // dont mark as invalid when his item isnt good (not exploitable because if item is invalid trade will fail anyway later on the same check)
            //{
            //    SendTradeStatus(TRADE_STATUS_NOT_ELIGIBLE);
            //    his_trade->SetAccepted(false, true);
            //    return;
            //}
        }
    }

    if (his_trade->IsAccepted())
    {
        setAcceptTradeMode(my_trade, his_trade, myItems, hisItems);

        Spell* my_spell = nullptr;
        SpellCastTargets my_targets;

        Spell* his_spell = nullptr;
        SpellCastTargets his_targets;

        // not accept if spell can't be cast now (cheating)
        if (uint32 my_spell_id = my_trade->GetSpell())
        {
            SpellInfo const* spellEntry = sSpellMgr->GetSpellInfo(my_spell_id);
            Item* castItem = my_trade->GetSpellCastItem();

            if (!spellEntry || !his_trade->GetItem(TRADE_SLOT_NONTRADED) ||
                (my_trade->HasSpellCastItem() && !castItem))
            {
                clearAcceptTradeMode(my_trade, his_trade);
                clearAcceptTradeMode(myItems, hisItems);

                my_trade->SetSpell(0);
                return;
            }

            my_spell = new Spell(_player, spellEntry, TRIGGERED_FULL_MASK);
            my_spell->m_CastItem = castItem;
            my_targets.SetTradeItemTarget(_player);
            my_spell->m_targets = my_targets;

            SpellCastResult res = my_spell->CheckCast(true);
            if (res != SPELL_CAST_OK)
            {
                my_spell->SendCastResult(res);

                clearAcceptTradeMode(my_trade, his_trade);
                clearAcceptTradeMode(myItems, hisItems);

                delete my_spell;
                my_trade->SetSpell(0);
                return;
            }
        }

        // not accept if spell can't be cast now (cheating)
        if (uint32 his_spell_id = his_trade->GetSpell())
        {
            SpellInfo const* spellEntry = sSpellMgr->GetSpellInfo(his_spell_id);
            Item* castItem = his_trade->GetSpellCastItem();

            if (!spellEntry || !my_trade->GetItem(TRADE_SLOT_NONTRADED) || (his_trade->HasSpellCastItem() && !castItem))
            {
                delete my_spell;
                his_trade->SetSpell(0);

                clearAcceptTradeMode(my_trade, his_trade);
                clearAcceptTradeMode(myItems, hisItems);
                return;
            }

            his_spell = new Spell(trader, spellEntry, TRIGGERED_FULL_MASK);
            his_spell->m_CastItem = castItem;
            his_targets.SetTradeItemTarget(trader);
            his_spell->m_targets = his_targets;

            SpellCastResult res = his_spell->CheckCast(true);
            if (res != SPELL_CAST_OK)
            {
                his_spell->SendCastResult(res);

                clearAcceptTradeMode(my_trade, his_trade);
                clearAcceptTradeMode(myItems, hisItems);

                delete my_spell;
                delete his_spell;

                his_trade->SetSpell(0);
                return;
            }
        }

        // inform partner client
        info.Status = TRADE_STATUS_TRADE_ACCEPT;
        trader->GetSession()->SendTradeStatus(info);

        // test if item will fit in each inventory
        TradeStatusInfo myCanCompleteInfo, hisCanCompleteInfo;
        hisCanCompleteInfo.Result = trader->CanStoreItems(myItems, TRADE_SLOT_TRADED_COUNT, &hisCanCompleteInfo.ItemLimitCategoryId);
        myCanCompleteInfo.Result = _player->CanStoreItems(hisItems, TRADE_SLOT_TRADED_COUNT, &myCanCompleteInfo.ItemLimitCategoryId);

        clearAcceptTradeMode(myItems, hisItems);

        // in case of missing space report error
        if (myCanCompleteInfo.Result != EQUIP_ERR_OK)
        {
            clearAcceptTradeMode(my_trade, his_trade);

            myCanCompleteInfo.Status = TRADE_STATUS_CLOSE_WINDOW;
            trader->GetSession()->SendTradeStatus(myCanCompleteInfo);
            myCanCompleteInfo.IsTargetResult = true;
            SendTradeStatus(myCanCompleteInfo);
            my_trade->SetAccepted(false);
            his_trade->SetAccepted(false);
            delete my_spell;
            delete his_spell;
            return;
        }
        else if (hisCanCompleteInfo.Result != EQUIP_ERR_OK)
        {
            clearAcceptTradeMode(my_trade, his_trade);

            hisCanCompleteInfo.Status = TRADE_STATUS_CLOSE_WINDOW;
            SendTradeStatus(hisCanCompleteInfo);
            hisCanCompleteInfo.IsTargetResult = true;
            trader->GetSession()->SendTradeStatus(hisCanCompleteInfo);
            my_trade->SetAccepted(false);
            his_trade->SetAccepted(false);
            delete my_spell;
            delete his_spell;
            return;
        }

        // execute trade: 1. remove
        for (uint8 i = 0; i < TRADE_SLOT_TRADED_COUNT; ++i)
        {
            if (myItems[i])
            {
                myItems[i]->SetGuidValue(ITEM_FIELD_GIFTCREATOR, _player->GetGUID());
                _player->MoveItemFromInventory(myItems[i]->GetBagSlot(), myItems[i]->GetSlot(), true);
            }
            if (hisItems[i])
            {
                hisItems[i]->SetGuidValue(ITEM_FIELD_GIFTCREATOR, trader->GetGUID());
                trader->MoveItemFromInventory(hisItems[i]->GetBagSlot(), hisItems[i]->GetSlot(), true);
            }
        }

        // execute trade: 2. store
        moveItems(myItems, hisItems);

        // logging money
        if (HasPermission(rbac::RBAC_PERM_LOG_GM_TRADE))
        {
            if (my_trade->GetMoney() > 0)
            {
                sLog->outCommand(_player->GetSession()->GetAccountId(), "GM %s (Account: %u) give money (Amount: %u) to player: %s (Account: %u)",
                    _player->GetName().c_str(), _player->GetSession()->GetAccountId(),
                    my_trade->GetMoney(),
                    trader->GetName().c_str(), trader->GetSession()->GetAccountId());
            }

            if (his_trade->GetMoney() > 0)
            {
                sLog->outCommand(trader->GetSession()->GetAccountId(), "GM %s (Account: %u) give money (Amount: %u) to player: %s (Account: %u)",
                    trader->GetName().c_str(), trader->GetSession()->GetAccountId(),
                    his_trade->GetMoney(),
                    _player->GetName().c_str(), _player->GetSession()->GetAccountId());
            }
        }

        // update money
        _player->ModifyMoney(-int32(my_trade->GetMoney()));
        _player->ModifyMoney(his_trade->GetMoney());
        trader->ModifyMoney(-int32(his_trade->GetMoney()));
        trader->ModifyMoney(my_trade->GetMoney());

        if (my_spell)
            my_spell->prepare(my_targets);

        if (his_spell)
            his_spell->prepare(his_targets);

        // cleanup
        clearAcceptTradeMode(my_trade, his_trade);
        delete _player->m_trade;
        _player->m_trade = nullptr;
        delete trader->m_trade;
        trader->m_trade = nullptr;

        // desynchronized with the other saves here (SaveInventoryAndGoldToDB() not have own transaction guards)
        SQLTransaction trans = CharacterDatabase.BeginTransaction();
        _player->SaveInventoryAndGoldToDB(trans);
        trader->SaveInventoryAndGoldToDB(trans);
        CharacterDatabase.CommitTransaction(trans);

        info.Status = TRADE_STATUS_TRADE_COMPLETE;
        trader->GetSession()->SendTradeStatus(info);
        SendTradeStatus(info);

        sLogsDatabaseAccessor->CharacterTrade(_player, trader, myItems, hisItems, my_trade->GetMoney(), his_trade->GetMoney());
    }
    else
    {
        info.Status = TRADE_STATUS_TRADE_ACCEPT;
        trader->GetSession()->SendTradeStatus(info);
    }

}

void WorldSession::HandleUnacceptTradeOpcode(WorldPacket& /*recvPacket*/)
{
    TradeData* my_trade = _player->GetTradeData();
    if (!my_trade)
        return;

    my_trade->SetAccepted(false, true);
}

void WorldSession::HandleBeginTradeOpcode(WorldPacket& /*recvPacket*/)
{
    TradeData* my_trade = _player->m_trade;
    if (!my_trade)
        return;

    TradeStatusInfo info;
    info.Status = TRADE_STATUS_OPEN_WINDOW;
    my_trade->GetTrader()->GetSession()->SendTradeStatus(info);
    SendTradeStatus(info);
}

void WorldSession::SendCancelTrade()
{
    if (PlayerRecentlyLoggedOut() || PlayerLogout())
        return;

    TradeStatusInfo info;
    info.Status = TRADE_STATUS_TRADE_CANCELED;
    SendTradeStatus(info);
}

void WorldSession::HandleCancelTradeOpcode(WorldPacket& /*recvPacket*/)
{
    // sended also after LOGOUT COMPLETE
    if(_player)                                             // needed because STATUS_LOGGEDIN_OR_RECENTLY_LOGGOUT
        _player->TradeCancel(true);
}

void WorldSession::HandleInitiateTradeOpcode(WorldPacket& recvPacket)
{
    ObjectGuid ID;
    recvPacket >> ID;

    if (GetPlayer()->m_trade)
        return;

    TradeStatusInfo info;
    if (!GetPlayer()->IsAlive())
    {
        info.Status = TRADE_STATUS_YOU_DEAD;
        SendTradeStatus(info);
        return;
    }

    if (GetPlayer()->HasUnitState(UNIT_STATE_STUNNED))
    {
        info.Status = TRADE_STATUS_YOU_STUNNED;
        SendTradeStatus(info);
        return;
    }

    if (isLogingOut())
    {
        info.Status = TRADE_STATUS_YOU_LOGOUT;
        SendTradeStatus(info);
        return;
    }

    if (GetPlayer()->IsInFlight())
    {
        info.Status = TRADE_STATUS_TARGET_TO_FAR;
        SendTradeStatus(info);
        return;
    }

    /*TC
    if (GetPlayer()->GetLevel() < sWorld->getIntConfig(CONFIG_TRADE_LEVEL_REQ))
    {
        SendNotification(GetTrinityString(LANG_TRADE_REQ), sWorld->getIntConfig(CONFIG_TRADE_LEVEL_REQ));
        return;
    }*/

    Player* pOther = ObjectAccessor::FindPlayer(ID);

    if (!pOther)
    {
        info.Status = TRADE_STATUS_NO_TARGET;
        SendTradeStatus(info);
        return;
    }

    if (pOther == GetPlayer() || pOther->m_trade)
    {
        info.Status = TRADE_STATUS_BUSY;
        SendTradeStatus(info);
        return;
    }

    if (!pOther->IsAlive())
    {
        info.Status = TRADE_STATUS_TARGET_DEAD;
        SendTradeStatus(info);
        return;
    }

    if (pOther->IsInFlight())
    {
        info.Status = TRADE_STATUS_TARGET_TO_FAR;
        SendTradeStatus(info);
        return;
    }

    if (pOther->HasUnitState(UNIT_STATE_STUNNED))
    {
        info.Status = TRADE_STATUS_TARGET_STUNNED;
        SendTradeStatus(info);
        return;
    }

    if (pOther->GetSession()->isLogingOut())
    {
        info.Status = TRADE_STATUS_TARGET_LOGOUT;
        SendTradeStatus(info);
        return;
    }

    if (pOther->GetSocial()->HasIgnore(GetPlayer()->GetGUID()))
    {
        info.Status = TRADE_STATUS_IGNORE_YOU;
        SendTradeStatus(info);
        return;
    }

    if(pOther->GetTeam() != _player->GetTeam() && !sWorld->getConfig(CONFIG_ALLOW_TWO_SIDE_TRADE))
    {
        info.Status = TRADE_STATUS_WRONG_FACTION;
        SendTradeStatus(info);
        return;
    }

    if (!pOther->IsWithinDistInMap(_player, TRADE_DISTANCE, false))
    {
        info.Status = TRADE_STATUS_TARGET_TO_FAR;
        SendTradeStatus(info);
        return;
    }

    /*TC
    if (pOther->getLevel() < sWorld->getIntConfig(CONFIG_TRADE_LEVEL_REQ))
    {
        SendNotification(GetTrinityString(LANG_TRADE_OTHER_REQ), sWorld->getIntConfig(CONFIG_TRADE_LEVEL_REQ));
        return;
    }
    */

    // OK start trade
    _player->m_trade = new TradeData(_player, pOther);
    pOther->m_trade = new TradeData(pOther, _player);

    info.Status = TRADE_STATUS_BEGIN_TRADE;
    info.TraderGuid = _player->GetGUID();
    pOther->GetSession()->SendTradeStatus(info);
}

void WorldSession::HandleSetTradeGoldOpcode(WorldPacket& recvPacket)
{
    uint32 gold;
    recvPacket >> gold;

    TradeData* my_trade = _player->GetTradeData();
    if (!my_trade)
        return;

    my_trade->SetMoney(gold);
}

void WorldSession::HandleSetTradeItemOpcode(WorldPacket& recvPacket)
{
    // send update
    uint8 tradeSlot;
    uint8 bag;
    uint8 slot;

    //LK OK
    recvPacket >> tradeSlot;
    recvPacket >> bag;
    recvPacket >> slot;

    TradeData* my_trade = _player->GetTradeData();
    if (!my_trade)
        return;

    TradeStatusInfo info;
    // invalid slot number
    if(tradeSlot >= TRADE_SLOT_COUNT)
    {
        info.Status = TRADE_STATUS_TRADE_CANCELED;
        SendTradeStatus(info);
        return;
    }

    // check cheating, can't fail with correct client operations
    Item* item = _player->GetItemByPos(bag,slot);
    if (!item || (tradeSlot != TRADE_SLOT_NONTRADED && !item->CanBeTraded(false, true)))
    {
        info.Status = TRADE_STATUS_TRADE_CANCELED;
        SendTradeStatus(info);
        return;
    }

    ObjectGuid iGUID = item->GetGUID();
    // prevent place single item into many trade slots using cheating and client bugs
    if (my_trade->HasItem(iGUID))
    {
        // cheating attempt
        info.Status = TRADE_STATUS_TRADE_CANCELED;
        SendTradeStatus(info);
        return;
    }

#ifdef LICH_KING
    if (tradeSlot != TRADE_SLOT_NONTRADED && item->IsBindedNotWith(my_trade->GetTrader()))
    {
        info.Status = TRADE_STATUS_NOT_ON_TAPLIST;
        info.Slot = tradeSlot;
        SendTradeStatus(info);
        return;
    }
#endif

    my_trade->SetItem(TradeSlots(tradeSlot), item);
}

void WorldSession::HandleClearTradeItemOpcode(WorldPacket& recvPacket)
{
    uint8 tradeSlot;
    recvPacket >> tradeSlot;

    TradeData* my_trade = _player->m_trade;
    if (!my_trade)
        return;

    // invalid slot number
    if (tradeSlot >= TRADE_SLOT_COUNT)
        return;

    my_trade->SetItem(TradeSlots(tradeSlot), nullptr);
}

