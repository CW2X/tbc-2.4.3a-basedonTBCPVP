#include "WorldPacket.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "Log.h"
#include "World.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "UpdateMask.h"
#include "AuctionHouseMgr.h"
#include "Util.h"
#include "Chat.h"
#include "LogsDatabaseAccessor.h"
#include "Mail.h"
#include "CharacterCache.h"
#include "GameTime.h"

//please DO NOT use iterator++, because it is slower than ++iterator!!!
//post-incrementation is always slower than pre-incrementation !

//void called when player click on auctioneer npc
void WorldSession::HandleAuctionHelloOpcode( WorldPacket & recvData )
{
    ObjectGuid guid;                                            //NPC guid
    recvData >> guid;

    Creature *unit = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_AUCTIONEER);
    if (!unit)
    {
        TC_LOG_ERROR("auction","WORLD: HandleAuctionHelloOpcode - Unit (GUID: %u) not found or you can't interact with him.", uint32(guid.GetCounter()) );
        return;
    }

    // remove fake death
    if(GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    SendAuctionHello(guid, unit);
}

//this void causes that auction window is opened
void WorldSession::SendAuctionHello( ObjectGuid guid, Creature* unit )
{
    AuctionHouseEntry const* ahEntry = AuctionHouseMgr::GetAuctionHouseEntry(unit->GetFaction());
    if(!ahEntry)
        return;

    WorldPacket data( MSG_AUCTION_HELLO, 12 );
    data << (uint64) guid;
    data << (uint32) ahEntry->houseId;
    SendPacket( &data );
}

//call this method when player bids, creates, or deletes auction
void WorldSession::SendAuctionCommandResult(uint32 auctionId, uint32 Action, uint32 ErrorCode, uint32 bidError )
{
    WorldPacket data( SMSG_AUCTION_COMMAND_RESULT, 16 );
    data << auctionId;
    data << Action;
    data << ErrorCode;
    if ( !ErrorCode && Action )
        data << bidError;                                   //when bid, then send 0, once...
    SendPacket(&data);
}

//this function sends notification, if bidder is online
void WorldSession::SendAuctionBidderNotification( uint32 location, uint32 auctionId, uint64 bidder, uint32 bidSum, uint32 diff, uint32 itemEntry)
{
    WorldPacket data(SMSG_AUCTION_BIDDER_NOTIFICATION, (8*4));
    data << uint32(location);
    data << uint32(auctionId);
    data << uint64(bidder);
    data << uint32(bidSum);
    data << uint32(diff);
    data << uint32(itemEntry);
    data << uint32(0);
    SendPacket(&data);
}

//this void causes on client to display: "Your auction sold"
void WorldSession::SendAuctionOwnerNotification( AuctionEntry* auction)
{
    WorldPacket data(SMSG_AUCTION_OWNER_NOTIFICATION, (7*4));
    data << auction->Id;
    data << auction->bid;
    //from mac leak : next fields = public ItemInstance Item. These unknown fields should be item related
    data << (uint32) 0;                                     //unk
    data << (uint32) 0;                                     //unk
    data << (uint32) 0;                                     //unk
    data << auction->itemEntry;
    data << (uint32) 0;                                     //unk
    SendPacket(&data);
}

//this function sends mail to old bidder
void AuctionHouseMgr::SendAuctionOutbiddedMail(AuctionEntry *auction, uint32 newPrice, Player* newBidder, SQLTransaction& trans)
{
    ObjectGuid oldBidder_guid = ObjectGuid(HighGuid::Player, auction->bidder);
    Player *oldBidder = ObjectAccessor::FindPlayer(oldBidder_guid);

    uint32 oldBidder_accId = 0;
    if(!oldBidder)
        oldBidder_accId = sCharacterCache->GetCharacterAccountIdByGuid(oldBidder_guid);

    // old bidder exist
    if(oldBidder || oldBidder_accId)
    {
        if (oldBidder)
        {
            if (!newBidder)
                oldBidder->GetSession()->SendAuctionBidderNotification(auction->GetHouseId(), auction->Id, 0, newPrice, auction->GetAuctionOutBid(), auction->itemEntry);
            else
                oldBidder->GetSession()->SendAuctionBidderNotification(auction->GetHouseId(), auction->Id, newBidder->GetGUID(), newPrice, auction->GetAuctionOutBid(), auction->itemEntry);
        }


        MailDraft(auction->BuildAuctionMailSubject(AUCTION_OUTBIDDED), AuctionEntry::BuildAuctionMailBody(auction->owner, auction->bid, auction->buyout, auction->deposit, auction->GetAuctionCut()))
            .AddMoney(auction->bid)
            .SendMailTo(trans, MailReceiver(oldBidder, auction->bidder), auction, MAIL_CHECK_MASK_COPIED);
    }
}

//this function sends mail, when auction is cancelled to old bidder
void AuctionHouseMgr::SendAuctionCancelledToBidderMail(AuctionEntry* auction, SQLTransaction& trans)
{
    ObjectGuid bidder_guid = ObjectGuid(HighGuid::Player, auction->bidder);
    Player *bidder = ObjectAccessor::FindPlayer(bidder_guid);

    uint32 bidder_accId = 0;
    if(!bidder)
        bidder_accId = sCharacterCache->GetCharacterAccountIdByGuid(bidder_guid);

    // bidder exist
    if(bidder || bidder_accId)
    {
        MailDraft(auction->BuildAuctionMailSubject(AUCTION_CANCELLED_TO_BIDDER), AuctionEntry::BuildAuctionMailBody(auction->owner, auction->bid, auction->buyout, auction->deposit, 0))
            .AddMoney(auction->bid)
            .SendMailTo(trans, MailReceiver(bidder, auction->bidder), auction, MAIL_CHECK_MASK_COPIED);
    }
}

//this void creates new auction and adds auction to some auctionhouse
void WorldSession::HandleAuctionSellItem( WorldPacket & recvData )
{
    ObjectGuid auctioneerGUID, itemGUID;
    uint32 etime, bid, buyout;
    recvData >> auctioneerGUID >> itemGUID;
    recvData >> bid >> buyout >> etime;
    Player *pl = GetPlayer();

    if (!itemGUID || !bid || !etime)
        return;                                             //check for cheaters

    Creature *pCreature = GetPlayer()->GetNPCIfCanInteractWith(auctioneerGUID, UNIT_NPC_FLAG_AUCTIONEER);
    if (!pCreature)
    {
        TC_LOG_ERROR( "auction", "WORLD: HandleAuctionSellItem - Unit (GUID: %u) not found or you can't interact with him.", uint32(auctioneerGUID.GetCounter()) );
        return;
    }

    AuctionHouseEntry const* auctionHouseEntry = AuctionHouseMgr::GetAuctionHouseEntry(pCreature->GetFaction());
    if(!auctionHouseEntry)
    {
        TC_LOG_ERROR( "auction", "WORLD: HandleAuctionSellItem - Unit (GUID: %u) has wrong faction.", uint32(auctioneerGUID.GetCounter()) );
        return;
    }

    AuctionHouseObject* auctionHouse = sAuctionMgr->GetAuctionsMap( pCreature->GetFaction() );
    uint32 totalcount = auctionHouse->GetAuctionsCount(_player);
    //Check for max auctions reached
    if(totalcount > MAX_AUCTIONS)
    {
        ChatHandler(_player).PSendSysMessage("Vous avez atteint le maximum d'encheres simultanees.");
        return;
    }

    // client send time in minutes, convert to common used sec time
    etime *= MINUTE;

    // client understand only 3 auction time
    switch(etime)
    {
        case 1*MIN_AUCTION_TIME:
        case 2*MIN_AUCTION_TIME:
        case 4*MIN_AUCTION_TIME:
            break;
        default:
            return;
    }

    Item *it = pl->GetItemByGuid( itemGUID );
    //do not allow to sell already auctioned items
    if(sAuctionMgr->GetAItem(itemGUID.GetCounter()))
    {
        TC_LOG_ERROR("auctionHouse","AuctionError, player %s is sending item id: %u, but item is already in another auction", pl->GetName().c_str(), itemGUID.GetCounter());
        SendAuctionCommandResult(0, AUCTION_SELL_ITEM, AUCTION_INTERNAL_ERROR);
        return;
    }
    // prevent sending bag with items (cheat: can be placed in bag after adding equiped empty bag to auction)
    if(!it)
    {
        SendAuctionCommandResult(0, AUCTION_SELL_ITEM, AUCTION_ITEM_NOT_FOUND);
        return;
    }

    if(!it->CanBeTraded())
    {
        SendAuctionCommandResult(0, AUCTION_SELL_ITEM, AUCTION_INTERNAL_ERROR);
        return;
    }

#ifdef LICH_KING
    if (item->IsBoundAccountWide() && item->IsSoulBound() && player->GetSession()->GetAccountId() != receiverAccountId)
    {
        player->SendMailResult(0, MAIL_SEND, MAIL_ERR_EQUIP_ERROR, EQUIP_ERR_ARTEFACTS_ONLY_FOR_OWN_CHARACTERS);
        return;
    }
#endif

    if ((it->GetTemplate()->Flags & ITEM_FLAG_CONJURED) || it->GetUInt32Value(ITEM_FIELD_DURATION))
    {
        SendAuctionCommandResult(0, AUCTION_SELL_ITEM, AUCTION_INTERNAL_ERROR);
        return;
    }

    //we have to take deposit :
    uint32 deposit = sAuctionMgr->GetAuctionDeposit( auctionHouseEntry, etime, it );
    if ( pl->GetMoney() < deposit )
    {
        SendAuctionCommandResult(0, AUCTION_SELL_ITEM, AUCTION_NOT_ENOUGHT_MONEY);
        return;
    }

    LogsDatabaseAccessor::CreateAuction(GetPlayer(), it->GetGUID().GetCounter(), it->GetEntry(), it->GetCount());

    pl->ModifyMoney( -int32(deposit) );

    uint32 auction_time = uint32(etime * sWorld->GetRate(RATE_AUCTION_TIME));

    auto AH = new AuctionEntry;
    AH->Id = sObjectMgr->GenerateAuctionID();
    if(sWorld->getConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_AUCTION))
        AH->auctioneer = 23442;
    else
        AH->auctioneer = auctioneerGUID.GetCounter();

    AH->itemGUIDLow = itemGUID.GetCounter();
    AH->itemEntry = it->GetEntry();
    AH->itemCount = it->GetCount();
    AH->owner = pl->GetGUID().GetCounter();
    AH->startbid = bid;
    AH->bidder = 0;
    AH->bid = 0;
    AH->buyout = buyout;
    AH->expire_time = WorldGameTime::GetGameTime() + auction_time;
    AH->deposit = deposit;
    AH->auctionHouseEntry = auctionHouseEntry;
    AH->deposit_time = WorldGameTime::GetGameTime();

    TC_LOG_DEBUG("auctionHouse","selling item %u to auctioneer %u with initial bid %u with buyout %u and with time %u (in sec) in auctionhouse %u", itemGUID.GetCounter(), AH->auctioneer, bid, buyout, auction_time, AH->GetHouseId());
    auctionHouse->AddAuction(AH);

    sAuctionMgr->AddAItem(it);
    pl->MoveItemFromInventory( it->GetBagSlot(), it->GetSlot(), true);

    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    it->DeleteFromInventoryDB(trans);
    it->SaveToDB(trans);                                         // recursive and not have transaction guard into self, not in inventiory and can be save standalone
    AH->SaveToDB(trans);
    pl->SaveInventoryAndGoldToDB(trans);
    CharacterDatabase.CommitTransaction(trans);

    SendAuctionCommandResult(AH->Id, AUCTION_SELL_ITEM, AUCTION_OK);
}

//this function is called when client bids or buys out auction
void WorldSession::HandleAuctionPlaceBid( WorldPacket & recvData )
{
    ObjectGuid auctioneerGUID;
    uint32 auctionId;
    uint32 price;
    recvData >> auctioneerGUID;
    recvData >> auctionId >> price;

    if (!auctionId || !price)
        return;                                             //check for cheaters

    Creature *pCreature = GetPlayer()->GetNPCIfCanInteractWith(auctioneerGUID, UNIT_NPC_FLAG_AUCTIONEER);
    if (!pCreature)
    {
        TC_LOG_ERROR( "auction", "WORLD: HandleAuctionPlaceBid - Unit (GUID: %u) not found or you can't interact with him.", uint32(auctioneerGUID.GetCounter()) );
        return;
    }

    AuctionHouseObject* auctionHouse = sAuctionMgr->GetAuctionsMap( pCreature->GetFaction() );

    AuctionEntry *auction = auctionHouse->GetAuction(auctionId);
    Player *pl = GetPlayer();

    if( !auction || auction->owner == pl->GetGUID().GetCounter() )
    {
        //you cannot bid your own auction:
        SendAuctionCommandResult( 0, AUCTION_PLACE_BID, CANNOT_BID_YOUR_AUCTION_ERROR );
        return;
    }

    // impossible have online own another character (use this for speedup check in case online owner)
    Player* auction_owner = ObjectAccessor::FindPlayer(ObjectGuid(HighGuid::Player, auction->owner));
    if( !auction_owner && sCharacterCache->GetCharacterAccountIdByGuid(ObjectGuid(HighGuid::Player, auction->owner)) == pl->GetSession()->GetAccountId())
    {
        //you cannot bid your another character auction:
        SendAuctionCommandResult( 0, AUCTION_PLACE_BID, CANNOT_BID_YOUR_AUCTION_ERROR );
        return;
    }
    
    // AH bot protection: in the first 10 seconds after auction deposit, only player with same ip as auction owner can buyout
    if ((auction->deposit_time + 60) > WorldGameTime::GetGameTime()) {
        if (auction_owner && auction_owner->GetSession()->GetRemoteAddress() != pl->GetSession()->GetRemoteAddress()) {
            pl->GetSession()->SendNotification("You must wait %u seconds before buying this item.", uint32((auction->deposit_time + 60) - WorldGameTime::GetGameTime()));
            return;
        }
    }

    // cheating
    if(price <= auction->bid)
        return;

    // price too low for next bid if not buyout
    if ((price < auction->buyout || auction->buyout == 0) &&
        price < auction->bid + auction->GetAuctionOutBid())
    {
        //auction has already higher bid, client tests it!
        return;
    }

    if (price > pl->GetMoney())
    {
        //you don't have enought money!, client tests!
        //SendAuctionCommandResult(auction->auctionId, AUCTION_PLACE_BID, ???);
        return;
    }

    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    if ((price < auction->buyout) || (auction->buyout == 0))
    {
        if (auction->bidder > 0)
        {
            if ( auction->bidder == pl->GetGUID().GetCounter() )
            {
                pl->ModifyMoney( -int32(price - auction->bid));
            }
            else
            {
                // mail to last bidder and return money
                sAuctionMgr->SendAuctionOutbiddedMail(auction, price, GetPlayer(), trans);
                pl->ModifyMoney(-int32(price));
            }
        }
        else
        {
            pl->ModifyMoney( -int32(price) );
        }
        auction->bidder = pl->GetGUID().GetCounter();
        auction->bid = price;

        // after this update we should save player's money ...
        trans->PAppend("UPDATE auctionhouse SET buyguid = '%u',lastbid = '%u' WHERE id = '%u'", auction->bidder, auction->bid, auction->Id);

        SendAuctionCommandResult(auction->Id, AUCTION_PLACE_BID, AUCTION_OK, 0 );
    }
    else
    {
        //buyout:
        if (pl->GetGUID().GetCounter() == auction->bidder)
            pl->ModifyMoney(-int32(auction->buyout - auction->bid));
        else
        {
            pl->ModifyMoney(-int32(auction->buyout));
            if (auction->bidder)                          //buyout for bidded auction ..
                sAuctionMgr->SendAuctionOutbiddedMail(auction, auction->buyout, GetPlayer(), trans);
        }
        auction->bidder = pl->GetGUID().GetCounter();
        auction->bid = auction->buyout;

        sAuctionMgr->SendAuctionSalePendingMail(auction, trans);
        sAuctionMgr->SendAuctionSuccessfulMail(auction, trans);
        sAuctionMgr->SendAuctionWonMail(auction, trans);

        SendAuctionCommandResult(auction->Id, AUCTION_PLACE_BID, AUCTION_OK);

        sAuctionMgr->RemoveAItem(auction->itemGUIDLow);
        auctionHouse->RemoveAuction(auction->Id);
        auction->DeleteFromDB(trans);

        delete auction;
    }

    pl->SaveInventoryAndGoldToDB(trans);
    CharacterDatabase.CommitTransaction(trans);
}

//this void is called when auction_owner cancels his auction
void WorldSession::HandleAuctionRemoveItem( WorldPacket & recvData )
{
    ObjectGuid auctioneerGUID;
    uint32 auctionId;
    recvData >> auctioneerGUID;
    recvData >> auctionId;
    //TC_LOG_DEBUG("auctionHouse", "Cancel AUCTION AuctionID: %u", auctionId);

    Creature *pCreature = GetPlayer()->GetNPCIfCanInteractWith(auctioneerGUID, UNIT_NPC_FLAG_AUCTIONEER);
    if (!pCreature)
    {
        TC_LOG_ERROR( "auction", "WORLD: HandleAuctionRemoveItem - Unit (GUID: %u) not found or you can't interact with him.", uint32(auctioneerGUID.GetCounter()) );
        return;
    }

    // remove fake death
    if(GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    AuctionHouseObject* auctionHouse = sAuctionMgr->GetAuctionsMap( pCreature->GetFaction() );

    AuctionEntry *auction = auctionHouse->GetAuction(auctionId);
    Player *pl = GetPlayer();

    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    if (auction && auction->owner == pl->GetGUID().GetCounter())
    {
        Item *pItem = sAuctionMgr->GetAItem(auction->itemGUIDLow);
        if (pItem)
        {
            if (auction->bidder > 0)                        // If we have a bidder, we have to send him the money he paid
            {
                uint32 auctionCut = auction->GetAuctionCut();
                if ( pl->GetMoney() < auctionCut )          //player doesn't have enough money, maybe message needed
                    return;
                //some auctionBidderNotification would be needed, but don't know that parts..
                sAuctionMgr->SendAuctionCancelledToBidderMail( auction, trans);
                pl->ModifyMoney( -int32(auctionCut) );
            }

            // item will deleted or added to received mail list
            MailDraft(auction->BuildAuctionMailSubject(AUCTION_CANCELED), AuctionEntry::BuildAuctionMailBody(0, 0, auction->buyout, auction->deposit, 0))
                .AddItem(pItem)
                .SendMailTo(trans, pl, auction, MAIL_CHECK_MASK_COPIED);
        }
        else
        {
            TC_LOG_ERROR("auction", "Auction id: %u has non-existed item (item guid : %u)!!!", auction->Id, auction->itemGUIDLow);
            SendAuctionCommandResult( 0, AUCTION_CANCEL, AUCTION_INTERNAL_ERROR );
            return;
        }
    }
    else
    {
        SendAuctionCommandResult( 0, AUCTION_CANCEL, AUCTION_INTERNAL_ERROR );
        //this code isn't possible... maybe there should be assert
        TC_LOG_ERROR("auction", "POSSIBLE CHEATER : %u, he tried to cancel auction (id: %u) of another player, or auction is NULL", pl->GetGUID().GetCounter(), auctionId );
        return;
    }

    //inform player, that auction is removed
    SendAuctionCommandResult( auction->Id, AUCTION_CANCEL, AUCTION_OK );

    // Now remove the auction
    pl->SaveInventoryAndGoldToDB(trans);
    auction->DeleteFromDB(trans);
    CharacterDatabase.CommitTransaction(trans);
    sAuctionMgr->RemoveAItem( auction->itemGUIDLow);
    auctionHouse->RemoveAuction( auction->Id );
    delete auction;
}

//called when player lists his bids
void WorldSession::HandleAuctionListBidderItems( WorldPacket & recvData )
{
    ObjectGuid guid;                                            //NPC guid
    uint32 listfrom;                                        //page of auctions
    uint32 outbiddedCount;                                  //count of outbidded auctions

    recvData >> guid;
    recvData >> listfrom;                                  // not used in fact (this list not have page control in client)
    recvData >> outbiddedCount;
    if (recvData.size() != (16 + outbiddedCount * 4 ))
    {
        TC_LOG_ERROR("auction", "Client sent bad opcode!!! with count: %u and size : %u (mustbe: %u", outbiddedCount, (uint32)recvData.size(), (16 + outbiddedCount * 4 ));
        outbiddedCount = 0;
    }

    Creature *pCreature = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_AUCTIONEER);
    if (!pCreature)
    {
        TC_LOG_ERROR("auction",  "WORLD: HandleAuctionListBidderItems - Unit (GUID: %u) not found or you can't interact with him.", uint32(guid.GetCounter()) );
        return;
    }

    // remove fake death
    if(GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    AuctionHouseObject* auctionHouse = sAuctionMgr->GetAuctionsMap( pCreature->GetFaction() );

    WorldPacket data( SMSG_AUCTION_BIDDER_LIST_RESULT, (4+4+4) );
    Player *pl = GetPlayer();
    data << (uint32) 0;                                     //add 0 as count
    uint32 count = 0;
    uint32 totalcount = 0;
    while ( outbiddedCount > 0)                             //add all data, which client requires
    {
        --outbiddedCount;
        uint32 outbiddedAuctionId;
        recvData >> outbiddedAuctionId;
        AuctionEntry * auction = auctionHouse->GetAuction( outbiddedAuctionId );
        if ( auction && auction->BuildAuctionInfo(data))
        {
            ++totalcount;
            ++count;
        }
    }

    auctionHouse->BuildListBidderItems(data,pl,count,totalcount);
    data.put<uint32>( 0, count );                           // add count to placeholder
    data << totalcount;
    data << (uint32)300;                                    //unk 2.3.0
    SendPacket(&data);
}

//this void sends player info about his auctions
void WorldSession::HandleAuctionListOwnerItems( WorldPacket & recvData )
{
    uint32 listfrom;
    ObjectGuid guid;

    recvData >> guid;
    recvData >> listfrom;                                  // not used in fact (this list not have page control in client)

    Creature *pCreature = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_AUCTIONEER);
    if (!pCreature)
    {
        TC_LOG_ERROR( "auction", "WORLD: HandleAuctionListOwnerItems - Unit (GUID: %u) not found or you can't interact with him.", uint32(guid.GetCounter()) );
        return;
    }

    AuctionHouseObject* auctionHouse = sAuctionMgr->GetAuctionsMap( pCreature->GetFaction() );

    WorldPacket data( SMSG_AUCTION_OWNER_LIST_RESULT, (4+4+4) );
    data << (uint32) 0;                                     // amount place holder

    uint32 count = 0;
    uint32 totalcount = 0;

    auctionHouse->BuildListOwnerItems(data,_player,count,totalcount);
    data.put<uint32>(0, count);
    data << (uint32) totalcount;
    data << (uint32) 0;
    SendPacket(&data);
}

//this void is called when player clicks on search button
void WorldSession::HandleAuctionListItems( WorldPacket & recvData )
{
    std::string searchedname;
    uint8 levelmin, levelmax, usable;
    uint32 listfrom, auctionSlotID, auctionMainCategory, auctionSubCategory, quality;
    ObjectGuid guid;

    recvData >> guid;
    recvData >> listfrom;                                  // start, used for page control listing by 50 elements
    recvData >> searchedname;
    recvData >> levelmin >> levelmax;
    recvData >> auctionSlotID >> auctionMainCategory >> auctionSubCategory;
    recvData >> quality >> usable;

    Creature *pCreature = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_AUCTIONEER);
    if (!pCreature)
    {
        TC_LOG_ERROR( "auction", "WORLD: HandleAuctionListItems - Unit (GUID: %u) not found or you can't interact with him.", uint32(guid.GetCounter()) );
        return;
    }

    // remove fake death
    if(GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    AuctionHouseObject* auctionHouse = sAuctionMgr->GetAuctionsMap( pCreature->GetFaction() );

    //TC_LOG_DEBUG("auctionHouse","Auctionhouse search guid: " UI64FMTD ", list from: %u, searchedname: %s, levelmin: %u, levelmax: %u, auctionSlotID: %u, auctionMainCategory: %u, auctionSubCategory: %u, quality: %u, usable: %u", guid, listfrom, searchedname.c_str(), levelmin, levelmax, auctionSlotID, auctionMainCategory, auctionSubCategory, quality, usable);

    WorldPacket data( SMSG_AUCTION_LIST_RESULT, (4+4+4) );
    uint32 count = 0;
    uint32 totalcount = 0;
    data << (uint32) 0;

    // converting string that we try to find to lower case
    std::wstring wsearchedname;
    if(!Utf8toWStr(searchedname,wsearchedname))
        return;

    wstrToLower(wsearchedname);

    auctionHouse->BuildListAuctionItems(data,_player,
        wsearchedname, listfrom, levelmin, levelmax, usable,
        auctionSlotID, auctionMainCategory, auctionSubCategory, quality,
        count,totalcount);

    data.put<uint32>(0, count);
    data << (uint32) totalcount;
    data << (uint32) 300;                                   // unk 2.3.0 const?
    SendPacket(&data);
}

