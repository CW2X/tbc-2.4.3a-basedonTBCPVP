
#include "Common.h"
#include "Database/DatabaseEnv.h"
#include "Opcodes.h"
#include "Log.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "World.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Group.h"
#include "GroupMgr.h"
#include "ObjectAccessor.h"
#include "MapManager.h"
#include "SocialMgr.h"
#include "Util.h"
#include "CharacterCache.h"

/* differeces from off:
    -you can uninvite yourself - is is useful
    -you can accept invitation even if leader went offline
*/
/* todo:
    -group_destroyed msg is sent but not shown
    -reduce xp gaining when in raid group
    -quest sharing has to be corrected
    -FIX sending PartyMemberStats
*/

void WorldSession::SendPartyResult(PartyOperation operation, const std::string& member, PartyResult res)
{
    WorldPacket data(SMSG_PARTY_COMMAND_RESULT, (8+member.size()+1));
    data << (uint32)operation;
    data << member;
    data << (uint32)res;

    SendPacket( &data );
}

void WorldSession::_HandleGroupInviteOpcode(Player* player, std::string membername)
{
    // player trying to invite himself (most likely cheating)
    if (player == GetPlayer())
    {
        SendPartyResult(PARTY_OP_INVITE, membername, PARTY_RESULT_CANT_FIND_TARGET);
        return;
    }

    // restrict invite to GMs
    if (!sWorld->getConfig(CONFIG_ALLOW_GM_GROUP) && !GetPlayer()->IsGameMaster() && player->IsGameMaster())
    {
        SendPartyResult(PARTY_OP_INVITE, membername, PARTY_RESULT_CANT_FIND_TARGET);
        return;
    }

    // can't group with
    if (!GetPlayer()->IsGameMaster() && !sWorld->getConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_GROUP) && GetPlayer()->GetTeam() != player->GetTeam())
    {
        SendPartyResult(PARTY_OP_INVITE, membername, PARTY_RESULT_TARGET_UNFRIENDLY);
        return;
    }
    if (GetPlayer()->GetInstanceId() != 0 && player->GetInstanceId() != 0 && GetPlayer()->GetInstanceId() != player->GetInstanceId() && GetPlayer()->GetMapId() == player->GetMapId())
    {
        SendPartyResult(PARTY_OP_INVITE, membername, PARTY_RESULT_NOT_IN_YOUR_INSTANCE);
        return;
    }
    // just ignore us
    if (player->GetInstanceId() != 0 && player->GetDifficulty() != GetPlayer()->GetDifficulty())
    {
        SendPartyResult(PARTY_OP_INVITE, membername, PARTY_RESULT_ALREADY_IN_GROUP);
        return;
    }

    if (player->GetSocial()->HasIgnore(GetPlayer()->GetGUID().GetCounter()))
    {
        SendPartyResult(PARTY_OP_INVITE, membername, PARTY_RESULT_TARGET_IGNORE_YOU);
        return;
    }

    Group *group = GetPlayer()->GetGroup();
    if (group && group->isBGGroup())
        group = GetPlayer()->GetOriginalGroup();

    Group *group2 = player->GetGroup();
    if (group2 && group2->isBGGroup())
        group2 = player->GetOriginalGroup();

    // player already in another group or invited
    if (group2 || player->GetGroupInvite())
    {
        SendPartyResult(PARTY_OP_INVITE, membername, PARTY_RESULT_ALREADY_IN_GROUP);
        return;
    }

    if (group)
    {
        // not have permissions for invite
        if (!group->IsLeader(GetPlayer()->GetGUID()) && !group->IsAssistant(GetPlayer()->GetGUID()))
        {
            SendPartyResult(PARTY_OP_INVITE, "", PARTY_RESULT_YOU_NOT_LEADER);
            return;
        }
        // not have place
        if (group->IsFull())
        {
            SendPartyResult(PARTY_OP_INVITE, "", PARTY_RESULT_PARTY_FULL);
            return;
        }
    }

    // ok, but group not exist, start a new group
    // but don't create and save the group to the DB until
    // at least one person joins
    if (!group)
    {
        group = new Group;
        // new group: if can't add then delete
        if (!group->AddLeaderInvite(GetPlayer()))
        {
            delete group;
            return;
        }
        if (!group->AddInvite(player))
        {
            group->RemoveAllInvites();
            delete group;
            return;
        }
    }
    else
    {
        // already existed group: if can't add then just leave
        if (!group->AddInvite(player))
        {
            return;
        }
    }

    // ok, we do it
    WorldPacket data(SMSG_GROUP_INVITE, 10);                // guess size
    data << GetPlayer()->GetName();
    player->SendDirectMessage(&data);

    SendPartyResult(PARTY_OP_INVITE, membername, PARTY_RESULT_OK);
}

void WorldSession::HandleGroupInviteOpcode( WorldPacket & recvData )
{
    std::string membername;
    recvData >> membername;

    // attempt add selected player

    // cheating
    if(!normalizePlayerName(membername))
    {
        SendPartyResult(PARTY_OP_INVITE, membername, PARTY_RESULT_CANT_FIND_TARGET);
        return;
    }

    Player *player = ObjectAccessor::FindConnectedPlayerByName(membername.c_str());

    // no player
    if(!player)
    {
        SendPartyResult(PARTY_OP_INVITE, membername, PARTY_RESULT_CANT_FIND_TARGET);
        return;
    }

    _HandleGroupInviteOpcode(player, membername);
}

void WorldSession::HandleGroupAcceptOpcode( WorldPacket & /*recvData*/ )
{
    Group *group = GetPlayer()->GetGroupInvite();
    if (!group) 
        return;

    if(group->GetLeaderGUID() == GetPlayer()->GetGUID())
    {
        TC_LOG_ERROR("network","HandleGroupAcceptOpcode: player %s(%d) tried to accept an invite to his own group", GetPlayer()->GetName().c_str(), GetPlayer()->GetGUID().GetCounter());
        return;
    }

    // remove in from ivites in any case
    group->RemoveInvite(GetPlayer());

    /** error handling **/
    /********************/

    // not have place
    if(group->IsFull())
    {
        SendPartyResult(PARTY_OP_INVITE, "", PARTY_RESULT_PARTY_FULL);
        return;
    }

    Player* leader = ObjectAccessor::FindPlayer(group->GetLeaderGUID());

    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    // forming a new group, create it
    if(!group->IsCreated())
    {
        // This can happen if the leader is zoning. To be removed once delayed actions for zoning are implemented
        if (!leader)
        {
            group->RemoveAllInvites();
            return;
        }

        ASSERT(leader);
        group->RemoveInvite(leader);
        group->Create(leader, trans);
        sGroupMgr->AddGroup(group);
    }

    // everything's fine, do it, PLAYER'S GROUP IS SET IN ADDMEMBER!!!
    if(!group->AddMember(GetPlayer(), trans))
        return;

    CharacterDatabase.CommitTransaction(trans);

    group->BroadcastGroupUpdate();
}

void WorldSession::HandleGroupDeclineOpcode( WorldPacket & /*recvData*/ )
{
    Group  *group  = GetPlayer()->GetGroupInvite();
    if (!group) return;

    Player *leader = ObjectAccessor::FindConnectedPlayer(group->GetLeaderGUID());

    /** error handling **/
    if(!leader || !leader->GetSession())
        return;
    /********************/

    // everything's fine, do it
    if(!group->IsCreated())
    {
        // note: this means that if you invite more than one person
        // and one of them declines before the first one accepts
        // all invites will be cleared
        // fixme: is that ok ?
        group->RemoveAllInvites();
        delete group;
    }

    GetPlayer()->SetGroupInvite(nullptr);

    WorldPacket data( SMSG_GROUP_DECLINE, 10 );             // guess size
    data << GetPlayer()->GetName();
    leader->SendDirectMessage( &data );
}

void WorldSession::HandleGroupUninviteGuidOpcode(WorldPacket & recvData)
{
    ObjectGuid guid;
    recvData >> guid;

    //can't uninvite yourself
    if(guid == GetPlayer()->GetGUID())
    {
        TC_LOG_ERROR("network","WorldSession::HandleGroupUninviteGuidOpcode: leader %s(%d) tried to uninvite himself from the group.", GetPlayer()->GetName().c_str(), GetPlayer()->GetGUID().GetCounter());
        return;
    }

    PartyResult res = GetPlayer()->CanUninviteFromGroup();
    if(res != PARTY_RESULT_OK)
    {
        SendPartyResult(PARTY_OP_LEAVE, "", res);
        return;
    }

    Group* grp = GetPlayer()->GetGroup();
    if(!grp)
        return;

    if(grp->IsMember(guid))
    {
        Player::RemoveFromGroup(grp, guid, GROUP_REMOVEMETHOD_KICK, GetPlayer()->GetGUID());
        return;
    }

    if(Player* plr = grp->GetInvited(guid))
    {
        plr->UninviteFromGroup();
        return;
    }

    SendPartyResult(PARTY_OP_LEAVE, "", PARTY_RESULT_NOT_IN_YOUR_PARTY);
}

void WorldSession::HandleGroupUninviteOpcode(WorldPacket & recvData)
{
    std::string membername;
    recvData >> membername;

    // player not found
    if(!normalizePlayerName(membername))
        return;

    // can't uninvite yourself
    if(GetPlayer()->GetName() == membername)
    {
        TC_LOG_ERROR("network","WorldSession::HandleGroupUninviteOpcode: member %s(%d) tried to uninvite himself from the group.", GetPlayer()->GetName().c_str(), GetPlayer()->GetGUID().GetCounter());
        return;
    }

    PartyResult res = GetPlayer()->CanUninviteFromGroup();
    if(res != PARTY_RESULT_OK)
    {
        SendPartyResult(PARTY_OP_LEAVE, "", res);
        return;
    }

    Group* grp = GetPlayer()->GetGroup();
    if(!grp)
        return;

    ObjectGuid guid = grp->GetMemberGUID(membername);

    //can't uninvite leader
    if(guid == grp->GetLeaderGUID())
    {
        TC_LOG_ERROR("network","WorldSession::HandleGroupUninviteOpcode: assistant %s(%d) tried to uninvite leader from the group.", GetPlayer()->GetName().c_str(), GetPlayer()->GetGUID().GetCounter());
        return;
    }

    if(guid)
    {
        Player::RemoveFromGroup(grp, guid, GROUP_REMOVEMETHOD_KICK, GetPlayer()->GetGUID());
        return;
    }

    if(Player* plr = grp->GetInvited(membername))
    {
        plr->UninviteFromGroup();
        return;
    }

    SendPartyResult(PARTY_OP_LEAVE, membername, PARTY_RESULT_NOT_IN_YOUR_PARTY);
}

void WorldSession::HandleGroupSetLeaderOpcode( WorldPacket & recvData )
{
    Group *group = GetPlayer()->GetGroup();
    if(!group)
        return;

    ObjectGuid guid;
    recvData >> guid;

    Player *player = ObjectAccessor::FindConnectedPlayer(guid);

    /** error handling **/
    if (!player || !group->IsLeader(GetPlayer()->GetGUID()) || player->GetGroup() != group)
        return;
    /********************/

    // everything's fine, do it
    group->ChangeLeader(guid);
}

void WorldSession::HandleGroupDisbandOpcode( WorldPacket & /*recvData*/ )
{
    if(!GetPlayer()->GetGroup())
        return;

    if(_player->InBattleground())
    {
        SendPartyResult(PARTY_OP_INVITE, "", /*PARTY_RESULT_INVITE_RESTRICTED*/ PARTY_RESULT_OK); //sunstrider: changed result, PARTY_RESULT_OK shows nothing
        return;
    }

    /** error handling **/
    /********************/

    // everything's fine, do it
    SendPartyResult(PARTY_OP_LEAVE, GetPlayer()->GetName(), PARTY_RESULT_OK);

    GetPlayer()->RemoveFromGroup();
}

void WorldSession::HandleLootMethodOpcode( WorldPacket & recvData )
{
    Group *group = GetPlayer()->GetGroup();
    if(!group)
        return;

    uint32 lootMethod;
    ObjectGuid lootMaster;
    uint32 lootThreshold;
    recvData >> lootMethod >> lootMaster >> lootThreshold;

    /** error handling **/
    if(!group->IsLeader(GetPlayer()->GetGUID()))
        return;
    /********************/

    // everything's fine, do it
    group->SetLootMethod((LootMethod)lootMethod);
    group->SetMasterLooterGuid(lootMaster);
    group->SetLootThreshold((ItemQualities)lootThreshold);
    group->SendUpdate();
}

void WorldSession::HandleLootRoll( WorldPacket &recvData )
{
    if(!GetPlayer()->GetGroup())
        return;

    ObjectGuid guid;
    uint32 itemSlot;
    uint8  rollType;
    recvData >> guid;                                      //guid of the item rolled
    recvData >> itemSlot;
    recvData >> rollType;                                    //0: pass, 1: need, 2: greed

    if (rollType >= MAX_ROLL_TYPE)
        return;

    //TC_LOG_DEBUG("network.opcode","WORLD RECIEVE CMSG_LOOT_ROLL, From:%u, Numberofplayers:%u, Choise:%u", (uint32)Guid, NumberOfPlayers, Choise);

    Group* group = GetPlayer()->GetGroup();
    if(!group)
        return;

    // everything's fine, do it
    group->CountRollVote(GetPlayer()->GetGUID(), guid, rollType);

#ifdef LICH_KING

    switch (rollType)
    {
    case ROLL_NEED:
        GetPlayer()->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED, 1);
        break;
    case ROLL_GREED:
        GetPlayer()->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED, 1);
        break;
    }
#endif
}

void WorldSession::HandleMinimapPingOpcode(WorldPacket& recvData)
{
    if(!GetPlayer()->GetGroup())
        return;

    float x, y;
    recvData >> x;
    recvData >> y;

    //TC_LOG_DEBUG("network.opcode","Received opcode MSG_MINIMAP_PING X: %f, Y: %f", x, y);

    /** error handling **/
    /********************/

    // everything's fine, do it
    WorldPacket data(MSG_MINIMAP_PING, (8+4+4));
    data << GetPlayer()->GetGUID();
    data << x;
    data << y;
    GetPlayer()->GetGroup()->BroadcastPacket(&data, true, -1, GetPlayer()->GetGUID());
}

void WorldSession::HandleRandomRollOpcode(WorldPacket& recvData)
{
    uint32 minimum, maximum, roll;
    recvData >> minimum;
    recvData >> maximum;

    /** error handling **/
    if(minimum > maximum || maximum > 10000)                // < 32768 for urand call
        return;
    //sunstrider: client sends max 1000000

    /********************/

    // everything's fine, do it
    roll = urand(minimum, maximum);

    //TC_LOG_DEBUG("network.opcode","ROLL: MIN: %u, MAX: %u, ROLL: %u", minimum, maximum, roll);

    WorldPacket data(MSG_RANDOM_ROLL, 4+4+4+8);
    data << minimum;
    data << maximum;
    data << roll;
    data << GetPlayer()->GetGUID();
    if(GetPlayer()->GetGroup())
        GetPlayer()->GetGroup()->BroadcastPacket(&data, false);
    else
        SendPacket(&data);
}

void WorldSession::HandleRaidTargetUpdateOpcode( WorldPacket & recvData )
{
    Group *group = GetPlayer()->GetGroup();
    if(!group)
        return;

    uint8 icon;
    recvData >> icon;

    /** error handling **/
    /********************/

    // everything's fine, do it
    if(icon == 0xFF)                                        // target icon request
    {
        group->SendTargetIconList(this);
    }
    else                                                    // target icon update
    {
        if (icon >= 8) //sunstrider: max valid value, reversed from client
            return;

        if(!group->IsLeader(GetPlayer()->GetGUID()) && !group->IsAssistant(GetPlayer()->GetGUID()))
            return;

        ObjectGuid guid;
        recvData >> guid;
        group->SetTargetIcon(icon, _player->GetGUID(), guid);
    }
}

void WorldSession::HandleGroupRaidConvertOpcode( WorldPacket & /*recvData*/ )
{
    Group *group = GetPlayer()->GetGroup();
    if(!group)
        return;

    if(_player->InBattleground())
        return;

    /** error handling **/
    if(!group->IsLeader(GetPlayer()->GetGUID()) || group->GetMembersCount() < 2)
        return;
    /********************/

    // everything's fine, do it (is it 0 (PARTY_OP_INVITE) correct code)
    SendPartyResult(PARTY_OP_INVITE, "", PARTY_RESULT_OK);
    group->ConvertToRaid();
}

void WorldSession::HandleGroupChangeSubGroupOpcode( WorldPacket & recvData )
{
    // we will get correct pointer for group here, so we don't have to check if group is BG raid
    Group *group = GetPlayer()->GetGroup();
    if(!group)
        return;

    std::string name;
    uint8 groupNr;
    recvData >> name;
    recvData >> groupNr;

    if (!normalizePlayerName(name))
        return;

    if (groupNr >= MAX_RAID_SUBGROUPS)
        return;

    /** error handling **/
    if(!group->IsLeader(GetPlayer()->GetGUID()) && !group->IsAssistant(GetPlayer()->GetGUID()))
        return;

    if (!group->HasFreeSlotSubGroup(groupNr))
        return;
    /********************/

    ObjectGuid guid;
    if (Player* movedPlayer = ObjectAccessor::FindConnectedPlayerByName(name))
        guid = movedPlayer->GetGUID();
    else
        guid = sCharacterCache->GetCharacterGuidByName(name);

    if (guid.IsEmpty())
        return;

    group->ChangeMembersGroup(guid, groupNr);
}

void WorldSession::HandleGroupAssistantLeaderOpcode( WorldPacket & recvData )
{
    Group *group = GetPlayer()->GetGroup();
    if(!group)
        return;

    if (!group->IsLeader(GetPlayer()->GetGUID()))
        return;

    ObjectGuid guid;
    bool apply;
    recvData >> guid;
    recvData >> apply;

    group->SetGroupMemberFlag(guid, apply, MEMBER_FLAG_ASSISTANT);
}

void WorldSession::HandlePartyAssignmentOpcode( WorldPacket & recvData )
{
    Group *group = GetPlayer()->GetGroup();
    if(!group)
        return;

    if (!group->IsLeader(GetPlayer()->GetGUID()))
        return;

    uint8 assignment;
    bool apply;
    ObjectGuid guid;
    recvData >> assignment >> apply;
    recvData >> guid;

    switch (assignment)
    {
    case GROUP_ASSIGN_MAINASSIST:
        group->RemoveUniqueGroupMemberFlag(MEMBER_FLAG_MAINASSIST);
        group->SetGroupMemberFlag(guid, apply, MEMBER_FLAG_MAINASSIST);
        break;
    case GROUP_ASSIGN_MAINTANK:
        group->RemoveUniqueGroupMemberFlag(MEMBER_FLAG_MAINTANK);           // Remove main assist flag from current if any.
        group->SetGroupMemberFlag(guid, apply, MEMBER_FLAG_MAINTANK);
    default:
        break;
    }

    group->SendUpdate();
}

void WorldSession::HandleRaidReadyCheckOpcode( WorldPacket & recvData )
{
    Group *group = GetPlayer()->GetGroup();
    if(!group)
        return;

    if(recvData.empty())                                   // request
    {
        /** error handling **/
        if(!group->IsLeader(GetPlayer()->GetGUID()) && !group->IsAssistant(GetPlayer()->GetGUID()))
            return;
        /********************/

        // everything's fine, do it
        WorldPacket data(MSG_RAID_READY_CHECK, 8);
        data << GetPlayer()->GetGUID();
        group->BroadcastPacket(&data, false, -1);

        group->OfflineReadyCheck();
    }
    else                                                    // answer
    {
        uint8 state;
        recvData >> state;

        // everything's fine, do it
        WorldPacket data(MSG_RAID_READY_CHECK_CONFIRM, 9);
        data << GetPlayer()->GetGUID();
        data << state;
        group->BroadcastReadyCheck(&data);
    }
}

void WorldSession::HandleRaidReadyCheckFinishedOpcode( WorldPacket & recvData )
{
    //Group* group = GetPlayer()->GetGroup();
    //if(!group)
    //    return;

    //if(!group->IsLeader(GetPlayer()->GetGUID()) && !group->IsAssistant(GetPlayer()->GetGUID()))
    //    return;

    // Is any reaction need?
}

void WorldSession::BuildPartyMemberStatsChangedPacket(Player *player, WorldPacket *data)
{
    uint32 mask = player->GetGroupUpdateFlag();

    if (mask == GROUP_UPDATE_FLAG_NONE)
        return;

    if (mask & GROUP_UPDATE_FLAG_POWER_TYPE)                // if update power type, update current/max power also
        mask |= (GROUP_UPDATE_FLAG_CUR_POWER | GROUP_UPDATE_FLAG_MAX_POWER);

    if (mask & GROUP_UPDATE_FLAG_PET_POWER_TYPE)            // same for pets
        mask |= (GROUP_UPDATE_FLAG_PET_CUR_POWER | GROUP_UPDATE_FLAG_PET_MAX_POWER);

    uint32 byteCount = 0;
    for (int i = 1; i < GROUP_UPDATE_FLAGS_COUNT; ++i)
        if (mask & (1 << i))
            byteCount += GroupUpdateLength[i];

    data->Initialize(SMSG_PARTY_MEMBER_STATS, 8 + 4 + byteCount);
    *data << player->GetPackGUID();
    *data << (uint32) mask;

    if (mask & GROUP_UPDATE_FLAG_STATUS)
    {
        uint16 playerStatus = MEMBER_STATUS_ONLINE;
        if (player->IsPvP())
            playerStatus |= MEMBER_STATUS_PVP;

        if (!player->IsAlive())
        {
            if (player->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST))
                playerStatus |= MEMBER_STATUS_GHOST;
            else
                playerStatus |= MEMBER_STATUS_DEAD;
        }

        if (player->IsFFAPvP())
            playerStatus |= MEMBER_STATUS_PVP_FFA;

        if (player->IsAFK())
            playerStatus |= MEMBER_STATUS_AFK;

        if (player->IsDND())
            playerStatus |= MEMBER_STATUS_DND;

        *data << uint16(playerStatus);
    }

    if (mask & GROUP_UPDATE_FLAG_CUR_HP)
#ifdef LICH_KING
        *data << (uint32)player->GetHealth();
#else
        *data << (uint16) player->GetHealth();
#endif

    if (mask & GROUP_UPDATE_FLAG_MAX_HP)
#ifdef LICH_KING
        *data << (uint32)player->GetMaxHealth();
#else
        *data << (uint16) player->GetMaxHealth();
#endif

    Powers powerType = player->GetPowerType();
    if (mask & GROUP_UPDATE_FLAG_POWER_TYPE)
        *data << (uint8) powerType;

    if (mask & GROUP_UPDATE_FLAG_CUR_POWER)
        *data << (uint16) player->GetPower(powerType);

    if (mask & GROUP_UPDATE_FLAG_MAX_POWER)
        *data << (uint16) player->GetMaxPower(powerType);

    if (mask & GROUP_UPDATE_FLAG_LEVEL)
        *data << (uint16) player->GetLevel();

    if (mask & GROUP_UPDATE_FLAG_ZONE)
        *data << (uint16) player->GetZoneId();

    if (mask & GROUP_UPDATE_FLAG_POSITION)
        *data << (uint16) player->GetPositionX() << (uint16) player->GetPositionY();

    if (mask & GROUP_UPDATE_FLAG_AURAS)
    {
        uint64 auramask = player->GetAuraUpdateMask();
        *data << uint64(auramask);
        for(uint32 i = 0; i < MAX_AURAS; ++i)
        {
            if(auramask & (uint64(1) << i))
            {
                uint32 updatedAura = player->GetUInt32Value(uint16(UNIT_FIELD_AURA + i));
#ifdef LICH_KING
                *data << uint32(updatedAura);
#else
                *data << uint16(updatedAura);
#endif
                *data << uint8(1);
            }
        }
    }

    Pet *pet = player->GetPet();
    if (mask & GROUP_UPDATE_FLAG_PET_GUID)
    {
        if(pet)
            *data << (uint64) pet->GetGUID();
        else
            *data << (uint64) 0;
    }

    if (mask & GROUP_UPDATE_FLAG_PET_NAME)
    {
        if(pet)
            *data << pet->GetName();
        else
            *data << (uint8)  0;
    }

    if (mask & GROUP_UPDATE_FLAG_PET_MODEL_ID)
    {
        if(pet)
            *data << (uint16) pet->GetDisplayId();
        else
            *data << (uint16) 0;
    }

    if (mask & GROUP_UPDATE_FLAG_PET_CUR_HP)
    {
        if(pet)
            *data << (uint16) pet->GetHealth();
        else
            *data << (uint16) 0;
    }

    if (mask & GROUP_UPDATE_FLAG_PET_MAX_HP)
    {
        if(pet)
            *data << (uint16) pet->GetMaxHealth();
        else
            *data << (uint16) 0;
    }

    if (mask & GROUP_UPDATE_FLAG_PET_POWER_TYPE)
    {
        if(pet)
            *data << (uint8)  pet->GetPowerType();
        else
            *data << (uint8)  0;
    }

    if (mask & GROUP_UPDATE_FLAG_PET_CUR_POWER)
    {
        if(pet)
            *data << (uint16) pet->GetPower(pet->GetPowerType());
        else
            *data << (uint16) 0;
    }

    if (mask & GROUP_UPDATE_FLAG_PET_MAX_POWER)
    {
        if(pet)
            *data << (uint16) pet->GetMaxPower(pet->GetPowerType());
        else
            *data << (uint16) 0;
    }

    if (mask & GROUP_UPDATE_FLAG_PET_AURAS)
    {
        if(pet)
        {
            uint64 auramask = pet->GetAuraUpdateMask();
            *data << uint64(auramask);
            for(uint32 i = 0; i < MAX_AURAS; ++i)
            {
                if(auramask & (uint64(1) << i))
                {
                    uint32 updatedAura=pet->GetUInt32Value(uint16(UNIT_FIELD_AURA + i));
#ifdef LICH_KING
                    *data << uint32(updatedAura);
#else
                    *data << uint16(updatedAura);
#endif
                    *data << uint8(1);
                }
            }
        }
        else
            *data << (uint64) 0;
    }

#ifdef LICH_KING

    if (mask & GROUP_UPDATE_FLAG_VEHICLE_SEAT)
    {
        if (Vehicle* veh = player->GetVehicle())
            *data << uint32(veh->GetVehicleInfo()->m_seatID[player->m_movementInfo.transport.seat]);
        else
            *data << uint32(0);
    }
#endif
}

/*this procedure handles clients CMSG_REQUEST_PARTY_MEMBER_STATS request*/
void WorldSession::HandleRequestPartyMemberStatsOpcode(WorldPacket &recvData)
{
    //TC_LOG_DEBUG("network", "WORLD: Received CMSG_REQUEST_PARTY_MEMBER_STATS");
    ObjectGuid Guid;
    recvData >> Guid;

    Player *player = ObjectAccessor::FindConnectedPlayer(Guid);
    if (!player)
    {
        //LK OK
        WorldPacket data(SMSG_PARTY_MEMBER_STATS_FULL, 3 + 4 + 2);
#ifdef LICH_KING
        data << uint8(0);  // only for SMSG_PARTY_MEMBER_STATS_FULL, probably arena/bg related
#endif
        data.appendPackGUID(Guid);
        data << (uint32)GROUP_UPDATE_FLAG_STATUS;
        data << (uint16)MEMBER_STATUS_OFFLINE;
        SendPacket(&data);
        return;
    }

    Pet* pet = player->GetPet();
    Powers powerType = player->GetPowerType();

    //LK OK
    WorldPacket data(SMSG_PARTY_MEMBER_STATS_FULL, 4 + 2 + 2 + 2 + 1 + 2 * 6 + 8 + 1 + 8);
#ifdef LICH_KING
    data << uint8(0);  // only for SMSG_PARTY_MEMBER_STATS_FULL, probably arena/bg related
#endif
    data << player->GetPackGUID();

    uint32 updateFlags = GROUP_UPDATE_FLAG_STATUS | GROUP_UPDATE_FLAG_CUR_HP | GROUP_UPDATE_FLAG_MAX_HP
        | GROUP_UPDATE_FLAG_CUR_POWER | GROUP_UPDATE_FLAG_MAX_POWER | GROUP_UPDATE_FLAG_LEVEL
        | GROUP_UPDATE_FLAG_ZONE | GROUP_UPDATE_FLAG_POSITION | GROUP_UPDATE_FLAG_AURAS
        | GROUP_UPDATE_FLAG_PET_NAME | GROUP_UPDATE_FLAG_PET_MODEL_ID | GROUP_UPDATE_FLAG_PET_AURAS;

    if (powerType != POWER_MANA)
        updateFlags |= GROUP_UPDATE_FLAG_POWER_TYPE;

    if (pet)
        updateFlags |= GROUP_UPDATE_FLAG_PET_GUID | GROUP_UPDATE_FLAG_PET_CUR_HP | GROUP_UPDATE_FLAG_PET_MAX_HP
        | GROUP_UPDATE_FLAG_PET_POWER_TYPE | GROUP_UPDATE_FLAG_PET_CUR_POWER | GROUP_UPDATE_FLAG_PET_MAX_POWER;

#ifdef LICH_KING
    if (player->GetVehicle())
        updateFlags |= GROUP_UPDATE_FLAG_VEHICLE_SEAT;
#endif

    uint16 playerStatus = MEMBER_STATUS_ONLINE;
    if (player->IsPvP())
        playerStatus |= MEMBER_STATUS_PVP;

    if (!player->IsAlive())
    {
        if (player->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST))
            playerStatus |= MEMBER_STATUS_GHOST;
        else
            playerStatus |= MEMBER_STATUS_DEAD;
    }

    if (player->IsFFAPvP())
        playerStatus |= MEMBER_STATUS_PVP_FFA;

    if (player->IsAFK())
        playerStatus |= MEMBER_STATUS_AFK;

    if (player->IsDND())
        playerStatus |= MEMBER_STATUS_DND;

    data << (uint32)updateFlags;                                 // group update mask
    data << (uint16)playerStatus;                  // member's online status
#ifdef LICH_KING
    data << (uint32)player->GetHealth();                   // GROUP_UPDATE_FLAG_CUR_HP
    data << (uint32)player->GetMaxHealth();                // GROUP_UPDATE_FLAG_MAX_HP
#else
    data << (uint16)player->GetHealth();                   // GROUP_UPDATE_FLAG_CUR_HP
    data << (uint16)player->GetMaxHealth();                // GROUP_UPDATE_FLAG_MAX_HP
#endif
    if (updateFlags & GROUP_UPDATE_FLAG_POWER_TYPE)
        data << (uint8)  powerType;

    data << (uint16) player->GetPower(powerType);           // GROUP_UPDATE_FLAG_CUR_POWER
    data << (uint16) player->GetMaxPower(powerType);        // GROUP_UPDATE_FLAG_MAX_POWER
    data << (uint16) player->GetLevel();                    // GROUP_UPDATE_FLAG_LEVEL
    data << (uint16) player->GetZoneId();                   // GROUP_UPDATE_FLAG_ZONE
    data << (uint16) player->GetPositionX();                // GROUP_UPDATE_FLAG_POSITION
    data << (uint16) player->GetPositionY();                // GROUP_UPDATE_FLAG_POSITION

    uint64 auramask = 0;
    size_t maskPos = data.wpos();
    data << (uint64) auramask;                              // placeholder
    for(uint8 i = 0; i < MAX_AURAS; ++i)
    {
        if(uint32 aura = player->GetUInt32Value(UNIT_FIELD_AURA + i))
        {
            auramask |= (uint64(1) << i);
            if (GetClientBuild() == BUILD_335)
                data << uint32(aura);
            else
                data << uint16(aura);
            data << uint8(1);
        }
    }
    data.put<uint64>(maskPos,auramask);                     // GROUP_UPDATE_FLAG_AURAS

    if (updateFlags & GROUP_UPDATE_FLAG_PET_GUID)
        data << uint64(ASSERT_NOTNULL(pet)->GetGUID());

    data << std::string(pet ? pet->GetName() : "");         // GROUP_UPDATE_FLAG_PET_NAME
    data << uint16(pet ? pet->GetDisplayId() : 0);          // GROUP_UPDATE_FLAG_PET_MODEL_ID

    if (updateFlags & GROUP_UPDATE_FLAG_PET_CUR_HP)
#ifdef LICH_KING
        data << uint32(pet ? pet->GetHealth() : 0);
#else
        data << uint16(pet ? pet->GetHealth() : 0);
#endif

    if (updateFlags & GROUP_UPDATE_FLAG_PET_MAX_HP)
#ifdef LICH_KING
        data << uint32(pet ? pet->GetMaxHealth() : 0);
#else
        data << uint16(pet? pet->GetMaxHealth() : 0);
#endif

    Powers petpowertype = pet ? pet->GetPowerType() : POWER_MANA; //not used if no pet
    if (updateFlags & GROUP_UPDATE_FLAG_PET_POWER_TYPE)
        data << (uint8)petpowertype;

    if (updateFlags & GROUP_UPDATE_FLAG_PET_CUR_POWER)
        data << uint16(pet->GetPower(petpowertype));

    if (updateFlags & GROUP_UPDATE_FLAG_PET_MAX_POWER)
        data << uint16(pet->GetMaxPower(petpowertype));

    uint64 petauramask = 0;
    size_t petMaskPos = data.wpos();
    data << (uint64)petauramask;                       // placeholder
    if (pet)
    {
        for (uint8 i = 0; i < MAX_AURAS; ++i)
        {
            if (uint32 petaura = pet->GetUInt32Value(UNIT_FIELD_AURA + i))
            {
                petauramask |= (uint64(1) << i);
#ifdef LICH_KING
                data << (uint32)petaura;
#endif
                data << (uint16)petaura;
                data << (uint8)1; //aura flags
            }
        }
    }
    data.put<uint64>(petMaskPos, petauramask);           // GROUP_UPDATE_FLAG_PET_AURAS

#ifdef LICH_KING
    if (updateFlags & GROUP_UPDATE_FLAG_VEHICLE_SEAT)
        data << uint32(player->GetVehicle()->GetVehicleInfo()->m_seatID[player->m_movementInfo.transport.seat]);
#endif

    SendPacket(&data);
}

//sun: adapted from cmangos code
void WorldSession::HandleGroupSwapSubGroupOpcode(WorldPacket& recv_data)
{
    std::string playerName1, playerName2;

    recv_data >> playerName1;
    recv_data >> playerName2;

    if (!normalizePlayerName(playerName1))
    {
        SendPartyResult(PARTY_OP_SWAP, playerName1, PARTY_RESULT_CANT_FIND_TARGET);
        return;
    }
    if (!normalizePlayerName(playerName2))
    {
        SendPartyResult(PARTY_OP_SWAP, playerName1, PARTY_RESULT_CANT_FIND_TARGET);
        return;
    }

    Group* group = GetPlayer()->GetGroup();
    if (!group || !group->isRaidGroup())
        return;

    if (!group->IsLeader(GetPlayer()->GetGUID()) &&
        !group->IsAssistant(GetPlayer()->GetGUID()))
        return;

    //get guid, member may be offline
    auto getGuid = [&group](std::string const& playerName)
    {
        if (Player* player = ObjectAccessor::FindConnectedPlayerByName(playerName.c_str()))
            return player->GetGUID();
        else
        {
            if (ObjectGuid guid = sCharacterCache->GetCharacterGuidByName(playerName))
                return guid;
            else
                return ObjectGuid::Empty;
        }
    };

    ObjectGuid guid1 = getGuid(playerName1);
    ObjectGuid guid2 = getGuid(playerName2);

    if(!guid1 || !guid2)
    {
        SendPartyResult(PARTY_OP_SWAP, playerName1, PARTY_RESULT_CANT_FIND_TARGET);
        return;
    }

    uint8 groupId1 = group->GetMemberGroup(guid1);
    uint8 groupId2 = group->GetMemberGroup(guid2);

    if (groupId1 == MAX_RAID_SUBGROUPS + 1 || groupId2 == MAX_RAID_SUBGROUPS + 1)
        return;

    if (groupId1 == groupId2)
        return;

    group->ChangeMembersGroup(guid1, groupId2);
    group->ChangeMembersGroup(guid2, groupId1);
}

/*!*/void WorldSession::HandleRequestRaidInfoOpcode( WorldPacket & /*recvData*/ )
{
    // every time the player checks the character screen
    _player->SendRaidInfo();
}

/*void WorldSession::HandleGroupCancelOpcode( WorldPacket & recvData )
{
    TC_LOG_DEBUG("network.opcode", "WORLD: got CMSG_GROUP_CANCEL." );
}*/

void WorldSession::HandleOptOutOfLootOpcode( WorldPacket & recvData )
{
    uint32 passOnLoot;
    recvData >> passOnLoot; // 1 always pass, 0 do not pass

    // ignore if player not loaded
    if (!GetPlayer()) {                                      // needed because STATUS_AUTHED
        if (passOnLoot != 0)
            TC_LOG_ERROR("network","CMSG_GROUP_PASS_ON_LOOT value<>0 for not-loaded character!");
        return;
    }

    GetPlayer()->SetPassOnGroupLoot(passOnLoot);
}

