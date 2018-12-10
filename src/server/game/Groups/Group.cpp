#include "Common.h"
#include "Opcodes.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Player.h"
#include "World.h"
#include "ObjectMgr.h"
#include "Group.h"
#include "Formulas.h"
#include "ObjectAccessor.h"
#include "BattleGround.h"
#include "MapManager.h"
#include "InstanceSaveMgr.h"
#include "MapInstanced.h"
#include "Util.h"
#include "Containers.h"
#include "CharacterCache.h"
#include "UpdateFieldFlags.h"
#include "GroupMgr.h"
#include "GameTime.h"

Group::Group() :
    m_masterLooterGuid(ObjectGuid::Empty),
    m_looterGuid(ObjectGuid::Empty),
    m_mainAssistant(ObjectGuid::Empty),
    m_leaderGuid(ObjectGuid::Empty),
    m_mainTank(ObjectGuid::Empty),
    m_bgGroup(nullptr),
    m_lootMethod((LootMethod)0),
    m_groupType((GroupType)0),
    m_lootThreshold(ITEM_QUALITY_UNCOMMON),
    m_subGroupsCounts(nullptr),
    m_leaderLogoutTime(0),
    m_raidDifficulty(RAID_DIFFICULTY_NORMAL),
    m_dungeonDifficulty(DUNGEON_DIFFICULTY_NORMAL), 
    m_dbStoreId(0),
    m_guid()
{
    for(ObjectGuid& m_targetIcon : m_targetIcons)
        m_targetIcon = ObjectGuid::Empty;
}

Group::~Group()
{
    if(m_bgGroup)
    {
        if(m_bgGroup->GetBgRaid(ALLIANCE) == this) m_bgGroup->SetBgRaid(ALLIANCE, nullptr);
        else if(m_bgGroup->GetBgRaid(HORDE) == this) m_bgGroup->SetBgRaid(HORDE, nullptr);
        else TC_LOG_ERROR("FIXME","Group::~Group: battleground group is not linked to the correct battleground.");
    }
    Rolls::iterator itr;
    while(!RollId.empty())
    {
        itr = RollId.begin();
        Roll *r = *itr;
        RollId.erase(itr);
        delete(r);
    }

    // it is undefined whether objectmgr (which stores the groups) or instancesavemgr
    // will be unloaded first so we must be prepared for both cases
    // this may unload some instance saves
    for(auto & m_boundInstance : m_boundInstances)
        for(auto _itr : m_boundInstance)
            _itr.second.save->RemoveGroup(this);

    // Sub group counters clean up
    delete[] m_subGroupsCounts;
}

bool Group::Create(Player* leader, SQLTransaction trans)
{
    ObjectGuid leaderGuid = leader->GetGUID();
    ObjectGuid::LowType lowguid = sGroupMgr->GenerateGroupId();

    m_guid = ObjectGuid(HighGuid::Group, lowguid);
    m_leaderGuid = leaderGuid;
    m_leaderName = leader->GetName();
    leader->SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_GROUP_LEADER);

    m_groupType  = isBGGroup() ? GROUPTYPE_RAID : GROUPTYPE_NORMAL;

    if (m_groupType == GROUPTYPE_RAID)
        _initRaidSubGroupsCounter();

    m_lootMethod = GROUP_LOOT;
    m_lootThreshold = ITEM_QUALITY_UNCOMMON;
    m_looterGuid = leaderGuid;
    m_masterLooterGuid.Clear();

    m_dungeonDifficulty = REGULAR_DIFFICULTY;
    m_raidDifficulty = REGULAR_DIFFICULTY;

    if(!isBGGroup())
    {
        m_dungeonDifficulty = leader->GetDifficulty();

        m_dbStoreId = sGroupMgr->GenerateNewGroupDbStoreId();

        sGroupMgr->RegisterGroupDbStoreId(m_dbStoreId, this);

        if (!leader->IsTestingBot())
        {
            // Store group in database
            PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_GROUP);

            uint8 index = 0;

            stmt->setUInt32(index++, m_dbStoreId);
            stmt->setUInt32(index++, m_leaderGuid.GetCounter());
            stmt->setUInt8(index++, uint8(m_lootMethod));
            stmt->setUInt32(index++, m_looterGuid.GetCounter());
            stmt->setUInt8(index++, uint8(m_lootThreshold));
            stmt->setUInt64(index++, m_targetIcons[0].GetRawValue());
            stmt->setUInt64(index++, m_targetIcons[1].GetRawValue());
            stmt->setUInt64(index++, m_targetIcons[2].GetRawValue());
            stmt->setUInt64(index++, m_targetIcons[3].GetRawValue());
            stmt->setUInt64(index++, m_targetIcons[4].GetRawValue());
            stmt->setUInt64(index++, m_targetIcons[5].GetRawValue());
            stmt->setUInt64(index++, m_targetIcons[6].GetRawValue());
            stmt->setUInt64(index++, m_targetIcons[7].GetRawValue());
            stmt->setUInt8(index++, uint8(m_groupType));
            stmt->setUInt32(index++, uint8(m_dungeonDifficulty));
            stmt->setUInt32(index++, uint8(m_raidDifficulty));
            stmt->setUInt32(index++, m_masterLooterGuid.GetCounter());

            trans->Append(stmt);
        }

        Group::ConvertLeaderInstancesToGroup(leader, this, false);

        bool addMemberResult = AddMember(leader, trans);
        ASSERT(addMemberResult); // If the leader can't be added to a new group because it appears full, something is clearly wrong.
    } else if (!AddMember(leader, trans))
        return false;

    return true;
}

void Group::LoadGroupFromDB(Field* fields)
{
    if (isBGGroup())
        return;

    m_dbStoreId = fields[16].GetUInt32();
    m_guid = ObjectGuid(HighGuid::Group, sGroupMgr->GenerateGroupId());
    m_leaderGuid = ObjectGuid(HighGuid::Player, fields[0].GetUInt32());
    m_leaderLogoutTime = WorldGameTime::GetGameTime(); // Give the leader a chance to keep his position after a server crash

    // group leader not exist
    if (!sCharacterCache->GetCharacterNameByGuid(m_leaderGuid, m_leaderName))
        return;

    for (uint8 i = 0; i < TARGETICONCOUNT; ++i)
        m_targetIcons[i].Set(fields[4 + i].GetUInt64());

    m_groupType = GroupType(fields[12].GetUInt8());
    if (m_groupType & GROUPTYPE_RAID)
        _initRaidSubGroupsCounter();

    uint32 diff = fields[13].GetUInt8();
    if (diff >= MAX_DUNGEON_DIFFICULTY)
        m_dungeonDifficulty = DUNGEON_DIFFICULTY_NORMAL;
    else
        m_dungeonDifficulty = Difficulty(diff);

#ifdef LICH_KING
    uint32 r_diff = fields[14].GetUInt8();
    if (r_diff >= MAX_RAID_DIFFICULTY)
        m_raidDifficulty = RAID_DIFFICULTY_10MAN_NORMAL;
    else
        m_raidDifficulty = Difficulty(r_diff);

    if (m_groupType & GROUPTYPE_LFG)
        sLFGMgr->_LoadFromDB(fields, GetGUID());
#endif

    m_masterLooterGuid = ObjectGuid(HighGuid::Player, fields[15].GetUInt32());
}

void Group::LoadMemberFromDB(ObjectGuid::LowType guidLow, uint8 memberFlags, uint8 subgroup, uint8 roles)
{
    MemberSlot member;
    member.guid = ObjectGuid(HighGuid::Player, guidLow);

    // skip non-existed member
    if (!sCharacterCache->GetCharacterNameByGuid(member.guid, member.name))
    {
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GROUP_MEMBER);
        stmt->setUInt32(0, guidLow);
        CharacterDatabase.Execute(stmt);
        return;
    }

    member.group = subgroup;
    member.flags = memberFlags;
    member.roles = roles;

    m_memberSlots.push_back(member);

    SubGroupCounterIncrease(subgroup);

#ifdef LICH_KING
    sLFGMgr->SetupGroupMember(member.guid, GetGUID());
#endif
}

void Group::ConvertToRaid()
{
    m_groupType = GroupType(m_groupType | GROUPTYPE_RAID);

    _initRaidSubGroupsCounter();

    if (!isBGGroup() && !isBFGroup())
    {
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GROUP_TYPE);

        stmt->setUInt8(0, uint8(m_groupType));
        stmt->setUInt32(1, m_dbStoreId);

        CharacterDatabase.Execute(stmt);
    }

    SendUpdate();

    // update quest related GO states (quest activity dependent from raid membership)
    for (member_citerator citr = m_memberSlots.begin(); citr != m_memberSlots.end(); ++citr)
        if (Player* player = ObjectAccessor::FindPlayer(citr->guid))
            player->UpdateForQuestWorldObjects();
}

bool Group::AddInvite(Player *player)
{
    if(!player || player->GetGroupInvite())
        return false;
        
    Group* group = player->GetGroup();
    if (group && group->isBGGroup())
        group = player->GetOriginalGroup();
    if (group)
        return false;

    RemoveInvite(player);

    m_invitees.insert(player);

    player->SetGroupInvite(this);

    //sScriptMgr->OnGroupInviteMember(this, player->GetGUID());
    return true;
}

bool Group::AddLeaderInvite(Player *player)
{
    if(!AddInvite(player))
        return false;

    m_leaderGuid = player->GetGUID();
    m_leaderName = player->GetName();
    return true;
}

void Group::RemoveInvite(Player *player)
{
    if (player) {
        m_invitees.erase(player);
        player->SetGroupInvite(nullptr);
    }
}

void Group::RemoveAllInvites()
{
    for(auto m_invitee : m_invitees)
        m_invitee->SetGroupInvite(nullptr);

    m_invitees.clear();
}

Player* Group::GetInvited(ObjectGuid guid) const
{
    for(auto m_invitee : m_invitees)
    {
        if(m_invitee->GetGUID() == guid)
            return m_invitee;
    }
    return nullptr;
}

Player* Group::GetInvited(const std::string& name) const
{
    for(auto m_invitee : m_invitees)
    {
        if(m_invitee->GetName() == name)
            return m_invitee;
    }
    return nullptr;
}

bool Group::AddMember(Player* player, SQLTransaction trans)
{
    // Get first not-full group
    uint8 subGroup = 0;
    if (m_subGroupsCounts)
    {
        bool groupFound = false;
        for (; subGroup < MAX_RAID_SUBGROUPS; ++subGroup)
        {
            if (m_subGroupsCounts[subGroup] < MAXGROUPSIZE)
            {
                groupFound = true;
                break;
            }
        }
        // We are raid group and no one slot is free
        if (!groupFound)
            return false;
    }

    MemberSlot member;
    member.guid = player->GetGUID();
    member.name = player->GetName();
    member.group = subGroup;
    member.flags = 0;
    member.roles = 0;
    m_memberSlots.push_back(member);

    SubGroupCounterIncrease(subGroup);

    player->SetGroupInvite(nullptr);
    if (player->GetGroup())
    {
        if (isBGGroup() || isBFGroup()) // if player is in group and he is being added to BG raid group, then call SetBattlegroundRaid()
            player->SetBattlegroundOrBattlefieldRaid(this, subGroup);
        else //if player is in bg raid and we are adding him to normal group, then call SetOriginalGroup()
            player->SetOriginalGroup(this, subGroup);
    }
    else //if player is not in group, then call set group
        player->SetGroup(this, subGroup);

    // if the same group invites the player back, cancel the homebind timer
    player->m_InstanceValid = player->CheckInstanceValidity(false);

    if (!isRaidGroup())                                      // reset targetIcons for non-raid-groups
    {
        for (uint8 i = 0; i < TARGETICONCOUNT; ++i)
            m_targetIcons[i].Clear();
    }

    // insert into the table if we're not a battleground group
    if (!isBGGroup() && !isBFGroup() && !player->IsTestingBot())
    {
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_GROUP_MEMBER);

        stmt->setUInt32(0, m_dbStoreId);
        stmt->setUInt32(1, member.guid.GetCounter());
        stmt->setUInt8(2, member.flags);
        stmt->setUInt8(3, member.group);
        stmt->setUInt8(4, member.roles);

        trans->Append(stmt);
    }

    SendUpdate();
    //sScriptMgr->OnGroupAddMember(this, player->GetGUID());

    if (!IsLeader(player->GetGUID()) && !isBGGroup() && !isBFGroup())
    {
        // reset the new member's instances, unless he is currently in one of them
        // including raid/heroic instances that they are not permanently bound to!
        player->ResetInstances(INSTANCE_RESET_GROUP_JOIN, false);
#ifdef LICH_KING
        player->ResetInstances(INSTANCE_RESET_GROUP_JOIN, true);
#endif

        if(player->GetLevel() >= LEVELREQUIREMENT_HEROIC && player->GetDifficulty() != GetDifficulty() )
        {
            if (player->GetDungeonDifficulty() != GetDungeonDifficulty())
            {
                player->SetDungeonDifficulty(GetDungeonDifficulty());
                player->SendDungeonDifficulty(true);
            }
#ifdef LICH_KING
            if (player->GetRaidDifficulty() != GetRaidDifficulty())
            {
                player->SetRaidDifficulty(GetRaidDifficulty());
                player->SendRaidDifficulty(true);
            }
#endif
        }
    }
    player->SetGroupUpdateFlag(GROUP_UPDATE_FULL);
    UpdatePlayerOutOfRange(player);

    // quest related GO state dependent from raid membership
    if (isRaidGroup())
        player->UpdateForQuestWorldObjects();

    {
        //sunstrider: ... Not sure this is BC, those UF_FLAG_PARTY_MEMBER are only quest fields, why are those shared to party members?
        // Broadcast new player group member fields to rest of the group
        player->SetFieldNotifyFlag(UF_FLAG_PARTY_MEMBER);

        UpdateData groupData;
        WorldPacket groupDataPacket;

        // Broadcast group members' fields to player
        for (GroupReference* itr = GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            if (itr->GetSource() == player)
                continue;

            if (Player* existingMember = itr->GetSource())
            {
                //send group members fields to new player
                if (player->HaveAtClient(existingMember))
                {
                    existingMember->SetFieldNotifyFlag(UF_FLAG_PARTY_MEMBER);
                    existingMember->BuildValuesUpdateBlockForPlayer(&groupData, player);
                    existingMember->RemoveFieldNotifyFlag(UF_FLAG_PARTY_MEMBER);
                }

                //send new player fields to existing member
                if (existingMember->HaveAtClient(player))
                {
                    UpdateData newData;
                    WorldPacket newDataPacket;
                    player->BuildValuesUpdateBlockForPlayer(&newData, existingMember); //will build UF_FLAG_PARTY_MEMBER, see notify before loop
                    if (newData.HasData())
                    {
                        newData.BuildPacket(&newDataPacket, false);
                        existingMember->SendDirectMessage(&newDataPacket);
                    }
                }
            }
        }

        if (groupData.HasData())
        {
            groupData.BuildPacket(&groupDataPacket, false);
            player->SendDirectMessage(&groupDataPacket);
        }

        player->RemoveFieldNotifyFlag(UF_FLAG_PARTY_MEMBER);
    }

#ifdef LICH_KING
    if (m_maxEnchantingLevel < player->GetSkillValue(SKILL_ENCHANTING))
        m_maxEnchantingLevel = player->GetSkillValue(SKILL_ENCHANTING);
#endif

    return true;
}

bool Group::RemoveMember(ObjectGuid guid, RemoveMethod const& method /*= GROUP_REMOVEMETHOD_DEFAULT*/, ObjectGuid kicker /*= ObjectGuid::Empty*/, char const* reason /*= nullptr*/)
{
    BroadcastGroupUpdate();
    //sScriptMgr->OnGroupRemoveMember(this, guid, method, kicker, reason);

    Player* player = ObjectAccessor::FindConnectedPlayer(guid);
    if (player)
    {
        for (GroupReference* itr = GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            if (Player* groupMember = itr->GetSource())
            {
                if (groupMember->GetGUID() == guid)
                    continue;

                groupMember->RemoveAllGroupBuffsFromCaster(guid);
                player->RemoveAllGroupBuffsFromCaster(groupMember->GetGUID());
            }
        }
    }

#ifdef LICH_KING
    // LFG group vote kick handled in scripts
    if (isLFGGroup() && method == GROUP_REMOVEMETHOD_KICK)
        return !m_memberSlots.empty();
#endif

    // remove member and change leader (if need) only if strong more 2 members _before_ member remove (BG/BF allow 1 member group)
    if (GetMembersCount() > ((isBGGroup() || isLFGGroup() || isBFGroup()) ? 1u : 2u))
    {
        if (player)
        {
            // Battleground group handling
            if (isBGGroup() || isBFGroup())
                player->RemoveFromBattlegroundOrBattlefieldRaid();
            else
                // Regular group
            {
                if (player->GetOriginalGroup() == this)
                    player->SetOriginalGroup(nullptr);
                else
                    player->SetGroup(nullptr);

                // quest related GO state dependent from raid membership
                player->UpdateForQuestWorldObjects();
            }

            WorldPacket data;

            if (method == GROUP_REMOVEMETHOD_KICK 
#ifdef LICH_KING
                || method == GROUP_REMOVEMETHOD_KICK_LFG
#endif
                )
            {
                data.Initialize(SMSG_GROUP_UNINVITE, 0);
                player->SendDirectMessage(&data);
            }

            // Do we really need to send this opcode?
            data.Initialize(SMSG_GROUP_LIST, 1 + 1 + 1 + 1 + 8 + 4 + 4 + 8);
#ifdef LICH_KING
            data << uint8(0x10) << uint8(0) << uint8(0) << uint8(0);
            data << uint64(m_guid) << uint32(m_counter) << uint32(0) << uint64(0);
#else
            data << uint64(m_guid) << uint64(0) << uint64(0); //not sure about m_guid
#endif
            player->SendDirectMessage(&data);

            _homebindIfInstance(player);
        }

        // Remove player from group in DB
        if (!isBGGroup() && !isBFGroup())
        {
            PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GROUP_MEMBER);
            stmt->setUInt32(0, guid.GetCounter());
            CharacterDatabase.Execute(stmt);
            DelinkMember(guid);
        }

#ifdef LICH_KING
        // Reevaluate group enchanter if the leaving player had enchanting skill or the player is offline
        if (!player || player->GetSkillValue(SKILL_ENCHANTING))
            ResetMaxEnchantingLevel();
#endif
        // Remove player from loot rolls
        for (Rolls::iterator it = RollId.begin(); it != RollId.end(); ++it)
        {
            Roll* roll = *it;
            Roll::PlayerVote::iterator itr2 = roll->playerVote.find(guid);
            if (itr2 == roll->playerVote.end())
                continue;

            if (itr2->second == GREED 
#ifdef LICH_KING
                || itr2->second == DISENCHANT
#endif
                )
                --roll->totalGreed;
            else if (itr2->second == NEED)
                --roll->totalNeed;
            else if (itr2->second == PASS)
                --roll->totalPass;

            if (itr2->second != NOT_VALID)
                --roll->totalPlayersRolling;

            roll->playerVote.erase(itr2);

            CountRollVote(guid, roll->itemGUID, MAX_ROLL_TYPE);
        }
        // Update subgroups
        member_witerator slot = _getMemberWSlot(guid);
        if (slot != m_memberSlots.end())
        {
            SubGroupCounterDecrease(slot->group);
            m_memberSlots.erase(slot);
        }

        // Pick new leader if necessary
        if (m_leaderGuid == guid)
        {
            for (member_witerator itr = m_memberSlots.begin(); itr != m_memberSlots.end(); ++itr)
            {
                if (ObjectAccessor::FindConnectedPlayer(itr->guid))
                {
                    ChangeLeader(itr->guid);
                    break;
                }
            }
        }

        SendUpdate();

#ifdef LICH_KING
        if (isLFGGroup() && GetMembersCount() == 1)
        {
            Player* leader = ObjectAccessor::FindConnectedPlayer(GetLeaderGUID());
            uint32 mapId = sLFGMgr->GetDungeonMapId(GetGUID());
            if (!mapId || !leader || (leader->IsAlive() && leader->GetMapId() != mapId))
            {
                Disband();
                return false;
            }
        }
#endif

        if (m_memberMgr.getSize() < ((isLFGGroup() || isBGGroup()) ? 1u : 2u))
            Disband();

        return true;
    }
    // If group size before player removal <= 2 then disband it
    else
    {
        Disband();
        return false;
    }
}

void Group::ChangeLeader(ObjectGuid newLeaderGuid)
{
    member_witerator slot = _getMemberWSlot(newLeaderGuid);

    if (slot == m_memberSlots.end())
        return;

    Player* newLeader = ObjectAccessor::FindConnectedPlayer(slot->guid);

    // Don't allow switching leader to offline players
    if (!newLeader)
        return;

    //sScriptMgr->OnGroupChangeLeader(this, newLeaderGuid, m_leaderGuid);

    if (!isBGGroup() && !isBFGroup())
    {
        SQLTransaction trans = CharacterDatabase.BeginTransaction();

        // Remove the groups permanent instance bindings
        for (uint8 i = 0; i < MAX_DIFFICULTY; ++i)
        {
            for (BoundInstancesMap::iterator itr = m_boundInstances[i].begin(); itr != m_boundInstances[i].end();)
            {
                // Do not unbind saves of instances that already had map created (a newLeader entered)
                // forcing a new instance with another leader requires group disbanding (confirmed on retail)
                if (itr->second.perm && !sMapMgr->FindMap(itr->first, itr->second.save->GetInstanceId()))
                {
                    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GROUP_INSTANCE_PERM_BINDING);
                    stmt->setUInt32(0, m_dbStoreId);
                    stmt->setUInt32(1, itr->second.save->GetInstanceId());
                    trans->Append(stmt);

                    itr->second.save->RemoveGroup(this);
                    m_boundInstances[i].erase(itr++);
                }
                else
                    ++itr;
            }
        }

        // Copy the permanent binds from the new leader to the group
        Group::ConvertLeaderInstancesToGroup(newLeader, this, true);

        // Update the group leader
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GROUP_LEADER);

        stmt->setUInt32(0, newLeader->GetGUID().GetCounter());
        stmt->setUInt32(1, m_dbStoreId);

        trans->Append(stmt);

        CharacterDatabase.CommitTransaction(trans);
    }

    if (Player* oldLeader = ObjectAccessor::FindConnectedPlayer(m_leaderGuid))
        oldLeader->RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_GROUP_LEADER);

    newLeader->SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_GROUP_LEADER);
    m_leaderGuid = newLeader->GetGUID();
    m_leaderName = newLeader->GetName();
    ToggleGroupMemberFlag(slot, MEMBER_FLAG_ASSISTANT, false);

    WorldPacket data(SMSG_GROUP_SET_LEADER, m_leaderName.size() + 1);
    data << slot->name;
    BroadcastPacket(&data, true);
#ifndef LICH_KING
    SendUpdate(); //sun, leader not properly updated without this on BC
#endif
}

void Group::CheckLeader(ObjectGuid guid, bool isLogout)
{
    if(IsLeader(guid))
    {
        if(isLogout)
            m_leaderLogoutTime = WorldGameTime::GetGameTime();
        else
            m_leaderLogoutTime = 0;
    }
    else
    {
        //init leaderLogoutTimer when a member first connect after a boot
        if(!isLogout && m_leaderLogoutTime == 0) //normal member logins
        {
            Player* leader = nullptr;
            
            //find the leader from group members
            for(GroupReference *itr = GetFirstMember(); itr != nullptr; itr = itr->next())
            {
                if(itr->GetSource()->GetGUID() == m_leaderGuid)
                {
                    leader = itr->GetSource();
                    break;
                }
            }

            if(!leader || !leader->IsInWorld())
            {
                m_leaderLogoutTime = WorldGameTime::GetGameTime();
            }
        }
    }
}

/// convert the player's binds to the group
void Group::ConvertLeaderInstancesToGroup(Player *player, Group *group, bool switchLeader)
{
    // copy all binds to the group, when changing leader it's assumed the character
    // will not have any solo binds
    for (uint8 i = 0; i < MAX_DIFFICULTY; i++)
    {
        for (auto itr = player->m_boundInstances[i].begin(); itr != player->m_boundInstances[i].end();)
        {
            if (!switchLeader || !group->GetBoundInstance(itr->second.save->GetDifficulty(), itr->first))
                group->BindToInstance(itr->second.save, itr->second.perm, true);

            // permanent binds are not removed
            if (switchLeader && !itr->second.perm)
            {
                player->UnbindInstance(itr, Difficulty(i), true);   // increments itr
            }
            else
                ++itr;
        }
    }

    /* if group leader is in a non-raid dungeon map and nobody is actually bound to this map then the group can "take over" the instance *
    * (example: two-player group disbanded by disconnect where the player reconnects within 60 seconds and the group is reformed)       */
    if (Map* playerMap = player->GetMap())
        if (!switchLeader && playerMap->IsNonRaidDungeon())
            if (InstanceSave* save = sInstanceSaveMgr->GetInstanceSave(playerMap->GetInstanceId()))
                if (save->GetGroupCount() == 0 && save->GetPlayerCount() == 0)
                {
                    TC_LOG_DEBUG("maps", "Group::ConvertLeaderInstancesToGroup: Group for player %s is taking over unbound instance map %d with Id %d", player->GetName().c_str(), playerMap->GetId(), playerMap->GetInstanceId());
                    // if nobody is saved to this, then the save wasn't permanent
                    group->BindToInstance(save, false, false);
                }
}

bool Group::ChangeLeaderToFirstOnlineMember()
{
    for (GroupReference *itr = GetFirstMember(); itr != nullptr; itr = itr->next()) {
        Player* player = itr->GetSource();

        if (player && player->IsInWorld() && player->GetGUID() != m_leaderGuid) {
            ChangeLeader(player->GetGUID());
            return true;
        }
    }
    
    return false;
}

void Group::Disband(bool hideDestroy)
{
    //TC sScriptMgr->OnGroupDisband(this);
    Player *player;
    for(member_citerator citr = m_memberSlots.begin(); citr != m_memberSlots.end(); ++citr)
    {
        player = ObjectAccessor::FindConnectedPlayer(citr->guid);
        if (!player)
            continue;

        //sun: also remove group buff in disband case
        for (GroupReference* itr = GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            if (Player* groupMember = itr->GetSource())
            {
                if (groupMember->GetGUID() == citr->guid)
                    continue;

                groupMember->RemoveAllGroupBuffsFromCaster(citr->guid);
                player->RemoveAllGroupBuffsFromCaster(groupMember->GetGUID());
            }
        }

        // we cannot call _removeMember because it would invalidate member iterator
        // if we are removing player from battleground raid
        if (isBGGroup() || isBFGroup())
            player->RemoveFromBattlegroundOrBattlefieldRaid();
        else
        {
            // we can remove player who is in battleground from his original group
            if (player->GetOriginalGroup() == this)
                player->SetOriginalGroup(nullptr);
            else
                player->SetGroup(nullptr);
        }


        // quest related GO state dependent from raid membership
        if (isRaidGroup())
            player->UpdateForQuestWorldObjects();

        if(!player->GetSession())
            continue;

        WorldPacket data;
        if(!hideDestroy)
        {
            data.Initialize(SMSG_GROUP_DESTROYED, 0);
            player->SendDirectMessage(&data);
        }

        // we already removed player from group and in player->GetGroup() is his original group, send update
        if (Group* group = player->GetGroup())
            group->SendUpdate();
        else
        {
#ifndef LICH_KING
            data.Initialize(SMSG_GROUP_LIST, 24);
            data << uint64(m_guid) << uint64(0) << uint64(0); //not sure about m_guid
            player->SendDirectMessage(&data);
#else
            data.Initialize(SMSG_GROUP_LIST, 1 + 1 + 1 + 1 + 8 + 4 + 4 + 8);
            data << uint8(0x10) << uint8(0) << uint8(0) << uint8(0);
            data << uint64(m_guid) << uint32(m_counter) << uint32(0) << uint64(0);
            player->SendDirectMessage(&data);
#endif
        }

        _homebindIfInstance(player);

        //sun, always remove PLAYER_FLAGS_GROUP_LEADER
        player->RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_GROUP_LEADER);
    }
    RollId.clear();
    m_memberSlots.clear();

    RemoveAllInvites();

    if (!isBGGroup() && !isBFGroup())
    {
        SQLTransaction trans = CharacterDatabase.BeginTransaction();

        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GROUP);
        stmt->setUInt32(0, m_dbStoreId);
        trans->Append(stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GROUP_MEMBER_ALL);
        stmt->setUInt32(0, m_dbStoreId);
        trans->Append(stmt);

        CharacterDatabase.CommitTransaction(trans);

        ResetInstances(INSTANCE_RESET_GROUP_DISBAND, false, nullptr);
#ifdef LICH_KING
        ResetInstances(INSTANCE_RESET_GROUP_DISBAND, true, nullptr);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_LFG_DATA);
        stmt->setUInt32(0, m_dbStoreId);
        CharacterDatabase.Execute(stmt);
#endif

        sGroupMgr->FreeGroupDbStoreId(this);
    }

    sGroupMgr->RemoveGroup(this);
    delete this;
}

/*********************************************************/
/***                   LOOT SYSTEM                     ***/
/*********************************************************/

void Group::SendLootStartRoll(uint32 CountDown, uint32 /*mapid*/, const Roll &r)
{
    WorldPacket data(SMSG_LOOT_START_ROLL, (8+4+4+4+4+4));
    data << uint64(r.itemGUID);                             // guid of rolled item
#ifdef LICH_KING
    data << uint32(mapid);                                  // 3.3.3 mapid
#endif
    data << uint32(r.itemSlot);                             // itemslot
    data << uint32(r.itemid);                               // the itemEntryId for the item that shall be rolled for
    data << uint32(r.itemRandomSuffix);                     // randomSuffix
    data << uint32(r.itemRandomPropId);                     // item random property ID
#ifdef LICH_KING
    data << uint32(r.itemCount);                            // items in stack
#endif
    data << uint32(CountDown);                              // the countdown time to choose "need" or "greed"
#ifdef LICH_KING
    data << uint8(r.rollVoteMask);                          // roll type mask
#endif

    for (const auto & itr : r.playerVote)
    {
        Player *p = ObjectAccessor::FindConnectedPlayer(itr.first);
        if(!p || !p->GetSession())
            continue;

        if(itr.second == NOT_EMITED_YET)
            p->SendDirectMessage( &data );
    }
}

void Group::SendLootStartRollToPlayer(uint32 countDown, uint32 mapId, Player* p, bool canNeed, Roll const& r)
{
    if (!p || !p->GetSession())
        return;

    WorldPacket data(SMSG_LOOT_START_ROLL, (8 + 4 + 4 + 4 + 4 + 4 + 4 + 1));
    data << uint64(r.itemGUID);                             // guid of rolled ite
#ifdef LICH_KING
    data << uint32(mapId);                                  // 3.3.3 mapid
#endif
    data << uint32(r.itemSlot);                             // itemslot
    data << uint32(r.itemid);                               // the itemEntryId for the item that shall be rolled for
    data << uint32(r.itemRandomSuffix);                     // randomSuffix
    data << uint32(r.itemRandomPropId);                     // item random property ID
#ifdef LICH_KING
    data << uint32(r.itemCount);                            // items in stack
#endif
    data << uint32(countDown);                              // the countdown time to choose "need" or "greed"
#ifdef LICH_KING
    uint8 voteMask = r.rollVoteMask;
    if (!canNeed)
        voteMask &= ~ROLL_FLAG_TYPE_NEED;
    data << uint8(voteMask);                                // roll type mask
#endif

    p->SendDirectMessage(&data);
}

void Group::SendLootRoll(ObjectGuid sourceGuid, ObjectGuid targetGuid, uint8 rollNumber, uint8 rollType, const Roll &roll)
{
    WorldPacket data(SMSG_LOOT_ROLL, (8+4+8+4+4+4+1+1)); //LK ok
    data << uint64(sourceGuid);                             // guid of the item rolled
    data << uint32(roll.itemSlot);                             // itemslot
    data << uint64(targetGuid);
    data << uint32(roll.itemid);                               // the itemEntryId for the item that shall be rolled for
    data << uint32(roll.itemRandomSuffix);                     // randomSuffix
    data << uint32(roll.itemRandomPropId);                     // Item random property ID
    data << uint8(rollNumber);                              // 0: "Need for: [item name]" > 127: "you passed on: [item name]"      Roll number
    data << uint8(rollType);                                // 0: "Need for: [item name]" 0: "You have selected need for [item name] 1: need roll 2: greed roll
    data << uint8(0);                                       // 1: "You automatically passed on: %s because you cannot loot that item." - Possibly used in need befor greed

    for(const auto & itr : roll.playerVote)
    {
        Player *p = ObjectAccessor::FindConnectedPlayer(itr.first);
        if(!p || !p->GetSession())
            continue;

        if(itr.second != NOT_VALID)
            p->SendDirectMessage( &data );
    }
}

void Group::SendLootRollWon(ObjectGuid sourceGuid, ObjectGuid targetGuid, uint8 rollNumber, uint8 rollType, const Roll &roll)
{
    WorldPacket data(SMSG_LOOT_ROLL_WON, (8+4+4+4+4+8+1+1));
    data << uint64(sourceGuid);                             // guid of the item rolled
    data << uint32(roll.itemSlot);                          // slot
    data << uint32(roll.itemid);                               // the itemEntryId for the item that shall be rolled for
    data << uint32(roll.itemRandomSuffix);                     // randomSuffix
    data << uint32(roll.itemRandomPropId);                     // Item random property
    data << uint64(targetGuid);                             // guid of the player who won.
    data << uint8(rollNumber);                              // rollnumber realted to SMSG_LOOT_ROLL
    data << uint8(rollType);                                // Rolltype related to SMSG_LOOT_ROLL

    for(const auto & itr : roll.playerVote)
    {
        Player *p = ObjectAccessor::FindConnectedPlayer(itr.first);
        if(!p || !p->GetSession())
            continue;

        if(itr.second != NOT_VALID)
            p->SendDirectMessage( &data );
    }
}

void Group::SendLootAllPassed(Roll const& roll)
{
    WorldPacket data(SMSG_LOOT_ALL_PASSED, (8 + 4 + 4 + 4 + 4)); //LK ok
    data << uint64(roll.itemGUID);                             // Guid of the item rolled
    data << uint32(roll.itemSlot);                             // Item loot slot
    data << uint32(roll.itemid);                               // The itemEntryId for the item that shall be rolled for
    data << uint32(roll.itemRandomPropId);                     // Item random property ID
    data << uint32(roll.itemRandomSuffix);                     // Item random suffix ID

    for (Roll::PlayerVote::const_iterator itr = roll.playerVote.begin(); itr != roll.playerVote.end(); ++itr)
    {
        Player* player = ObjectAccessor::FindConnectedPlayer(itr->first);
        if (!player || !player->GetSession())
            continue;

        if (itr->second != NOT_VALID)
            player->SendDirectMessage(&data);
    }
}

// notify group members which player is the allowed looter for the given creature
void Group::SendLooter(Creature* creature, Player* groupLooter)
{
    ASSERT(creature);

    WorldPacket data(SMSG_LOOT_LIST, (8+8)); //LK ok, seems ok for bc (cmangos has the same struct 2018-02-19)
    data << uint64(creature->GetGUID());

    if (GetLootMethod() == MASTER_LOOT && creature->loot.hasOverThresholdItem())
        data << PackedGuid(GetMasterLooterGuid());
    else
        data << uint8(0);

    if (groupLooter)
        data << groupLooter->GetPackGUID();
    else
        data << uint8(0);

    BroadcastPacket(&data, false);
}

void Group::GroupLoot(Loot *loot, WorldObject* pLootedObject)
{
    std::vector<LootItem>::iterator i;
    ItemTemplate const *item;
    uint8 itemSlot = 0;

    for (i = loot->items.begin(); i != loot->items.end(); ++i, ++itemSlot)
    {
        if (i->freeforall)
            continue;

        item = sObjectMgr->GetItemTemplate(i->itemid);
        if (!item)
        {
            //TC_LOG_DEBUG("misc","Group::GroupLoot: missing item prototype for item with id: %d", i->itemid);
            continue;
        }

        //roll for over-threshold item if it's one-player loot
        if (item->Quality >= uint32(m_lootThreshold) && !i->freeforall)
        {
            ObjectGuid newitemGUID = ObjectGuid::Create<HighGuid::Item>(sObjectMgr->GetGenerator<HighGuid::Item>().Generate());

            Roll* r = new Roll(newitemGUID, *i);

            //a vector is filled with only near party members
            for (GroupReference *itr = GetFirstMember(); itr != nullptr; itr = itr->next())
            {
                Player *member = itr->GetSource();
                if(!member || !member->GetSession())
                    continue;
                if (member->IsAtGroupRewardDistance(pLootedObject) && i->AllowedForPlayer(member))
                {
                    ++r->totalPlayersRolling;
                        
                    if (member->GetPassOnGroupLoot()) {
                        r->playerVote[member->GetGUID()] = PASS;
                        r->totalPass++;
                        // can't broadcast the pass now. need to wait until all rolling players are known.
                    }
                    else
                        r->playerVote[member->GetGUID()] = NOT_EMITED_YET;
                }
            }

            if (r->totalPlayersRolling > 0)
            {
                r->setLoot(loot);
                r->itemSlot = itemSlot;
#ifdef LICH_KING
                if (item->DisenchantID && m_maxEnchantingLevel >= item->RequiredDisenchantSkill)
                    r->rollVoteMask |= ROLL_FLAG_TYPE_DISENCHANT;
#endif

                loot->items[itemSlot].is_blocked = true;

                // If there is any "auto pass", broadcast the pass now.
                if (r->totalPass)
                {
                    for (Roll::PlayerVote::const_iterator itr = r->playerVote.begin(); itr != r->playerVote.end(); ++itr)
                    {
                        Player* p = ObjectAccessor::FindConnectedPlayer(itr->first);
                        if (!p || !p->GetSession())
                            continue;

                        if (itr->second == PASS)
                            SendLootRoll(newitemGUID, p->GetGUID(), 128, ROLL_PASS, *r);
                    }
                }

                SendLootStartRoll(60000, pLootedObject->GetMapId(), *r);

                RollId.push_back(r);

                if (Creature* creature = pLootedObject->ToCreature())
                {
                    creature->m_groupLootTimer = 60000;
                    creature->lootingGroupLowGUID = GetLowGUID();
                }
                else if (GameObject* go = pLootedObject->ToGameObject())
                {
                    go->m_groupLootTimer = 60000;
                    go->lootingGroupLowGUID = GetLowGUID();
                }
            }
            else
                delete r;
        }
        else
            i->is_underthreshold = true;
    }


    for (i = loot->quest_items.begin(); i != loot->quest_items.end(); ++i, ++itemSlot)
    {
        if (!i->follow_loot_rules)
            continue;

        item = sObjectMgr->GetItemTemplate(i->itemid);
        if (!item)
        {
            //TC_LOG_DEBUG("misc", "Group::GroupLoot: missing item prototype for item with id: %d", i->itemid);
            continue;
        }

        ObjectGuid newitemGUID = ObjectGuid::Create<HighGuid::Item>(sObjectMgr->GetGenerator<HighGuid::Item>().Generate());

        Roll* r = new Roll(newitemGUID, *i);

        //a vector is filled with only near party members
        for (GroupReference* itr = GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player* member = itr->GetSource();
            if (!member || !member->GetSession())
                continue;

            if (member->IsAtGroupRewardDistance(pLootedObject) && i->AllowedForPlayer(member))
            {
                r->totalPlayersRolling++;
                r->playerVote[member->GetGUID()] = NOT_EMITED_YET;
            }
        }

        if (r->totalPlayersRolling > 0)
        {
            r->setLoot(loot);
            r->itemSlot = itemSlot;

            loot->quest_items[itemSlot - loot->items.size()].is_blocked = true;

            SendLootStartRoll(60000, pLootedObject->GetMapId(), *r);

            RollId.push_back(r);

            if (Creature* creature = pLootedObject->ToCreature())
            {
                creature->m_groupLootTimer = 60000;
                creature->lootingGroupLowGUID = GetLowGUID();
            }
            else if (GameObject* go = pLootedObject->ToGameObject())
            {
                go->m_groupLootTimer = 60000;
                go->lootingGroupLowGUID = GetLowGUID();
            }
        }
        else
            delete r;
    }
}

void Group::NeedBeforeGreed(Loot *loot, WorldObject* lootedObject)
{
    ItemTemplate const *item;
    uint8 itemSlot = 0;
    for (auto i = loot->items.begin(); i != loot->items.end(); ++i, ++itemSlot)
    {
        if (i->freeforall)
            continue;

        item = sObjectMgr->GetItemTemplate(i->itemid);

        //roll for over-threshold item if it's one-player loot
        if (item->Quality >= uint32(m_lootThreshold) && !i->freeforall)
        {
            ObjectGuid newitemGUID = ObjectGuid::Create<HighGuid::Item>(sObjectMgr->GetGenerator<HighGuid::Item>().Generate());
            Roll* r = new Roll(newitemGUID, *i);

            for(GroupReference *itr = GetFirstMember(); itr != nullptr; itr = itr->next())
            {
                Player *playerToRoll = itr->GetSource();
                if(!playerToRoll || !playerToRoll->GetSession())
                    continue;

                if (playerToRoll->CanUseItem(item) && i->AllowedForPlayer(playerToRoll) && playerToRoll->IsAtGroupRewardDistance(lootedObject))
                {
                    ++r->totalPlayersRolling;
                        
                    if (playerToRoll->GetPassOnGroupLoot()) {
                        r->playerVote[playerToRoll->GetGUID()] = PASS;
                        r->totalPass++;
                        // can't broadcast the pass now. need to wait until all rolling players are known.
                    }
                    else
                        r->playerVote[playerToRoll->GetGUID()] = NOT_EMITED_YET;
                }
            }

            if (r->totalPlayersRolling > 0)
            {
                r->setLoot(loot);
                r->itemSlot = itemSlot;
#ifdef LICH_KING
                if (item->DisenchantID && m_maxEnchantingLevel >= item->RequiredDisenchantSkill)
                    r->rollVoteMask |= ROLL_FLAG_TYPE_DISENCHANT;

                if (item->Flags2 & ITEM_FLAG2_CAN_ONLY_ROLL_GREED)
                    r->rollVoteMask &= ~ROLL_FLAG_TYPE_NEED;
#endif

                loot->items[itemSlot].is_blocked = true;

                //Broadcast Pass and Send Rollstart
                for (Roll::PlayerVote::const_iterator itr = r->playerVote.begin(); itr != r->playerVote.end(); ++itr)
                {
                    Player* p = ObjectAccessor::FindConnectedPlayer(itr->first);
                    if (!p || !p->GetSession())
                        continue;
#ifdef LICH_KING
                    bool canNeed = p->CanRollForItemInLFG(item, lootedObject) == EQUIP_ERR_OK
#else
                    bool canNeed = true;
#endif
                    if (itr->second == PASS)
                        SendLootRoll(newitemGUID, p->GetGUID(), 128, ROLL_PASS, *r);
                    else
                        SendLootStartRollToPlayer(60000, lootedObject->GetMapId(), p, canNeed, *r);
                }

                RollId.push_back(r);

                if (Creature* creature = lootedObject->ToCreature())
                {
                    creature->m_groupLootTimer = 60000;
                    creature->lootingGroupLowGUID = GetLowGUID();
                }
                else if (GameObject* go = lootedObject->ToGameObject())
                {
                    go->m_groupLootTimer = 60000;
                    go->lootingGroupLowGUID = GetLowGUID();
                }
            }
            else
            {
                delete r;
            }
        }
        else
            i->is_underthreshold = true;
    }

    for (std::vector<LootItem>::iterator i = loot->quest_items.begin(); i != loot->quest_items.end(); ++i, ++itemSlot)
    {
        if (!i->follow_loot_rules)
            continue;

        item = sObjectMgr->GetItemTemplate(i->itemid);
        ObjectGuid newitemGUID = ObjectGuid::Create<HighGuid::Item>(sObjectMgr->GetGenerator<HighGuid::Item>().Generate());

        Roll* r = new Roll(newitemGUID, *i);

        for (GroupReference* itr = GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player* playerToRoll = itr->GetSource();
            if (!playerToRoll || !playerToRoll->GetSession())
                continue;

            if (playerToRoll->IsAtGroupRewardDistance(lootedObject) && i->AllowedForPlayer(playerToRoll))
            {
                r->totalPlayersRolling++;
                r->playerVote[playerToRoll->GetGUID()] = NOT_EMITED_YET;
            }
        }

        if (r->totalPlayersRolling > 0)
        {
            r->setLoot(loot);
            r->itemSlot = itemSlot;

            loot->quest_items[itemSlot - loot->items.size()].is_blocked = true;

            //Broadcast Pass and Send Rollstart
            for (Roll::PlayerVote::const_iterator itr = r->playerVote.begin(); itr != r->playerVote.end(); ++itr)
            {
                Player* p = ObjectAccessor::FindConnectedPlayer(itr->first);
                if (!p || !p->GetSession())
                    continue;
#ifdef LICH_KING
                bool canNeed = p->CanRollForItemInLFG(item, lootedObject) == EQUIP_ERR_OK
#else
                bool canNeed = true;
#endif

                if (itr->second == PASS)
                    SendLootRoll(newitemGUID, p->GetGUID(), 128, ROLL_PASS, *r);
                else
                    SendLootStartRollToPlayer(60000, lootedObject->GetMapId(), p, canNeed, *r);
            }

            RollId.push_back(r);

            if (Creature* creature = lootedObject->ToCreature())
            {
                creature->m_groupLootTimer = 60000;
                creature->lootingGroupLowGUID = GetLowGUID();
            }
            else if (GameObject* go = lootedObject->ToGameObject())
            {
                go->m_groupLootTimer = 60000;
                go->lootingGroupLowGUID = GetLowGUID();
            }
        }
        else
            delete r;
    }
}

void Group::MasterLoot(Loot* loot, WorldObject* pLootedObject)
{
    for (std::vector<LootItem>::iterator i = loot->items.begin(); i != loot->items.end(); ++i)
    {
        if (i->freeforall)
            continue;

        i->is_blocked = !i->is_underthreshold;
    }

    for (std::vector<LootItem>::iterator i = loot->quest_items.begin(); i != loot->quest_items.end(); ++i)
    {
        if (!i->follow_loot_rules)
            continue;

        i->is_blocked = !i->is_underthreshold;
    }

    uint32 real_count = 0;

    WorldPacket data(SMSG_LOOT_MASTER_LIST, 1 + GetMembersCount() * 8); //LK & BC ok
    data << uint8(GetMembersCount());

    for (GroupReference* itr = GetFirstMember(); itr != nullptr; itr = itr->next())
    {
        Player* looter = itr->GetSource();
        if (!looter->IsInWorld())
            continue;

        if (looter->IsAtGroupRewardDistance(pLootedObject))
        {
            data << uint64(looter->GetGUID());
            ++real_count;
        }
    }

    data.put<uint8>(0, real_count);

    for (GroupReference* itr = GetFirstMember(); itr != nullptr; itr = itr->next())
    {
        Player* looter = itr->GetSource();
        if (looter->IsAtGroupRewardDistance(pLootedObject))
            looter->SendDirectMessage(&data);
    }
}

void Group::CountRollVote(ObjectGuid playerGUID, ObjectGuid Guid, uint8 choice)
{
    auto rollI = GetRoll(Guid);
    if (rollI == RollId.end())
        return;
    Roll* roll = *rollI;

    auto itr = roll->playerVote.find(playerGUID);
    // this condition means that player joins to the party after roll begins
    if (itr == roll->playerVote.end())
        return;

    if (roll->getLoot())
        if (roll->getLoot()->items.empty())
            return;

    switch (choice)
    {
        case ROLL_PASS:                                             //Player choose pass
        {
            SendLootRoll(ObjectGuid::Empty, playerGUID, 128, 128, *roll);
            ++roll->totalPass;
            itr->second = PASS;
        }
        break;
        case ROLL_NEED:                                             //player choose Need
        {
            SendLootRoll(ObjectGuid::Empty, playerGUID, 0, 0, *roll);
            ++roll->totalNeed;
            itr->second = NEED;
        }
        break;
        case ROLL_GREED:                                             //player choose Greed
        {
            SendLootRoll(ObjectGuid::Empty, playerGUID, 128, 2, *roll);
            ++roll->totalGreed;
            itr->second = GREED;
        }
#ifdef LICH_KING
        case ROLL_DISENCHANT:                               // player choose Disenchant
            SendLootRoll(ObjectGuid::Empty, playerGUID, 128, ROLL_DISENCHANT, *roll);
            ++roll->totalGreed;
            itr->second = DISENCHANT;
            break;
#endif
        break;
    }
    if (roll->totalPass + roll->totalNeed + roll->totalGreed >= roll->totalPlayersRolling)
        CountTheRoll(rollI, nullptr);
}

//called when roll timer expires
void Group::EndRoll(Loot* pLoot, Map* allowedMap)
{
    for (Rolls::iterator itr = RollId.begin(); itr != RollId.end();)
    {
        if ((*itr)->getLoot() == pLoot) {
            CountTheRoll(itr, allowedMap);           //i don't have to edit player votes, who didn't vote ... he will pass
            itr = RollId.begin();
        }
        else
            ++itr;
    }
}

void Group::CountTheRoll(Rolls::iterator rollI, Map* allowedMap)
{
    Roll* roll = *rollI;
    if(!roll->isValid())                                    // is loot already deleted ?
    {
        RollId.erase(rollI);
        delete roll;
        return;
    }

    //end of the roll
    if (roll->totalNeed > 0)
    {
        if(!roll->playerVote.empty())
        {
            uint8 maxresul = 0;
            ObjectGuid maxguid  = (*roll->playerVote.begin()).first;
            Player *player = nullptr;

            for( Roll::PlayerVote::const_iterator itr=roll->playerVote.begin(); itr!=roll->playerVote.end(); ++itr)
            {
                if (itr->second != NEED)
                    continue;

                player = ObjectAccessor::FindPlayer(itr->first); //Not FindConnectedPlayer player, this is intentional (doesn't change result though)
                if (!player || (allowedMap != nullptr && player->FindMap() != allowedMap))
                {
                    --roll->totalNeed;
                    continue;
                }

                uint8 randomN = urand(1, 100);
                SendLootRoll(ObjectGuid::Empty, itr->first, randomN, ROLL_NEED, *roll);
                if (maxresul < randomN)
                {
                    maxguid  = itr->first;
                    maxresul = randomN;
                }
            }

            if (!maxguid.IsEmpty())
            {
                SendLootRollWon(ObjectGuid::Empty, maxguid, maxresul, ROLL_NEED, *roll);
                player = ObjectAccessor::FindPlayer(maxguid);

                if (player && player->GetSession())
                {
                    ItemPosCountVec dest;
                    LootItem* item = &(roll->itemSlot >= roll->getLoot()->items.size() ? roll->getLoot()->quest_items[roll->itemSlot - roll->getLoot()->items.size()] : roll->getLoot()->items[roll->itemSlot]);
                    uint8 msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, roll->itemid, item->count);
                    if (msg == EQUIP_ERR_OK)
                    {
                        item->is_looted = true;
                        roll->getLoot()->NotifyItemRemoved(roll->itemSlot);
                        roll->getLoot()->unlootedCount--;
#ifdef LICH_KING
                        player->StoreNewItem(dest, roll->itemid, true, item->randomPropertyId, item->GetAllowedLooters());
#else
                        player->StoreNewItem(dest, roll->itemid, true, item->randomPropertyId);
#endif
                    }
                    else
                    {
                        item->is_blocked = false;
                        item->rollWinnerGUID = player->GetGUID();
                        player->SendEquipError(msg, nullptr, nullptr);
                    }
                }
            }
            else
                roll->totalNeed = 0;
        }
    }

    if (roll->totalNeed == 0 && roll->totalGreed > 0) // if (roll->totalNeed == 0 && ...), not else if, because numbers can be modified above if player is on a different map
    {
        if(!roll->playerVote.empty())
        {
            uint8 maxresul = 0;
            ObjectGuid maxguid = ObjectGuid::Empty;
            Player* player = nullptr;
            RollVote rollvote = NOT_VALID;

            Roll::PlayerVote::iterator itr;
            for (itr=roll->playerVote.begin(); itr!=roll->playerVote.end(); ++itr)
            {
                if (itr->second != GREED 
#ifdef LICH_KING
                    && itr->second != DISENCHANT
#endif
                    )
                    continue;

                player = ObjectAccessor::FindPlayer(itr->first);
                if (!player || (allowedMap != nullptr && player->FindMap() != allowedMap))
                {
                    --roll->totalGreed;
                    continue;
                }

                uint8 randomN = urand(1, 100);
                SendLootRoll(ObjectGuid::Empty, itr->first, randomN, 2, *roll);
                if (maxresul < randomN)
                {
                    maxguid  = itr->first;
                    maxresul = randomN;
                    rollvote = itr->second;
                }
            }

            if (!maxguid.IsEmpty())
            {
                SendLootRollWon(ObjectGuid::Empty, maxguid, maxresul, rollvote, *roll);
                player = ObjectAccessor::FindConnectedPlayer(maxguid);

                if (player && player->GetSession())
                {
#ifdef LICH_KING
                    player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED_ON_LOOT, roll->itemid, maxresul);
#endif
                    LootItem* item = &(roll->itemSlot >= roll->getLoot()->items.size() ? roll->getLoot()->quest_items[roll->itemSlot - roll->getLoot()->items.size()] : roll->getLoot()->items[roll->itemSlot]);

                    if (rollvote == GREED)
                    {
                        ItemPosCountVec dest;
                        InventoryResult msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, roll->itemid, item->count);
                        if (msg == EQUIP_ERR_OK)
                        {
                            item->is_looted = true;
                            roll->getLoot()->NotifyItemRemoved(roll->itemSlot);
                            roll->getLoot()->unlootedCount--;
#ifdef LICH_KING
                            player->StoreNewItem(dest, roll->itemid, true, item->randomPropertyId, item->GetAllowedLooters());
#else
                            player->StoreNewItem(dest, roll->itemid, true, item->randomPropertyId);
#endif
                        }
                        else
                        {
                            item->is_blocked = false;
                            item->rollWinnerGUID = player->GetGUID();
                            player->SendEquipError(msg, nullptr, nullptr, roll->itemid);
                        }
                    }
#ifdef LICH_KING
                    else if (rollvote == DISENCHANT)
                    {
                        item->is_looted = true;
                        roll->getLoot()->NotifyItemRemoved(roll->itemSlot);
                        roll->getLoot()->unlootedCount--;
                        ItemTemplate const* pProto = sObjectMgr->GetItemTemplate(roll->itemid);
                        ASSERT(pProto);
                        player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL, 13262); // Disenchant

                        ItemPosCountVec dest;
                        InventoryResult msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, roll->itemid, item->count);
                        if (msg == EQUIP_ERR_OK)
                            player->AutoStoreLoot(pProto->DisenchantID, LootTemplates_Disenchant, true);
                        else // If the player's inventory is full, send the disenchant result in a mail.
                        {
                            Loot loot;
                            loot.FillLoot(pProto->DisenchantID, LootTemplates_Disenchant, player, true);

                            uint32 max_slot = loot.GetMaxSlotInLootFor(player);
                            for (uint32 i = 0; i < max_slot; ++i)
                            {
                                LootItem* lootItem = loot.LootItemInSlot(i, player);
                                player->SendEquipError(msg, nullptr, nullptr, lootItem->itemid);
                                player->SendItemRetrievalMail(lootItem->itemid, lootItem->count);
                            }
                        }
                    }
#endif
                }
            }
            else
                roll->totalGreed = 0;
        }
    }

    if (roll->totalNeed == 0 && roll->totalGreed == 0) // if, not else, because numbers can be modified above if player is on a different map
    {
        SendLootAllPassed(*roll);

        // remove is_blocked so that the item is lootable by all players
        LootItem* item = &(roll->itemSlot >= roll->getLoot()->items.size() ? roll->getLoot()->quest_items[roll->itemSlot - roll->getLoot()->items.size()] : roll->getLoot()->items[roll->itemSlot]);
        item->is_blocked = false;
    }

    RollId.erase(rollI);
    delete roll;
}

void Group::SetTargetIcon(uint8 id, ObjectGuid /*whoGuid*/, ObjectGuid targetGuid)
{
    if(id >= TARGETICONCOUNT)
        return;

    // clean other icons
    if(targetGuid )
        for (uint32 i = 0; i<TARGETICONCOUNT; i++)
            if (m_targetIcons[i] == targetGuid)
                SetTargetIcon(i, ObjectGuid::Empty, ObjectGuid::Empty);

    m_targetIcons[id] = targetGuid;

    WorldPacket data(MSG_RAID_TARGET_UPDATE, (2+8));
    data << uint8(0);                                  // set targets
#ifdef LICH_KING
    data << uint64(whoGuid);
#endif
    data << uint8(id);
    data << uint64(targetGuid);
    BroadcastPacket(&data, true);
}

ObjectGuid Group::GetTargetIcon(uint8 id) const
{
    if(id >= TARGETICONCOUNT)
    {
        TC_LOG_ERROR("misc", "Group::GetTargetIcon called with wrong argument %u", id);
        return ObjectGuid::Empty;
    }

    return m_targetIcons[id];
}

void Group::SendTargetIconList(WorldSession *session)
{
    if (!session)
        return;

    WorldPacket data(MSG_RAID_TARGET_UPDATE, (1 + TARGETICONCOUNT * 9)); //BC + LK ok
    data << uint8(1);                                       // list targets

    for (uint8 i = 0; i < TARGETICONCOUNT; i++)
    {
        if (m_targetIcons[i].IsEmpty())
            continue;

        data << (uint8)i;
        data << uint64(m_targetIcons[i]);
    }

    session->SendPacket(&data);
}

void Group::SendUpdate()
{
    for (member_citerator citr = m_memberSlots.begin(); citr != m_memberSlots.end(); ++citr)
        SendUpdateToPlayer(citr->guid);
}

void Group::SendUpdateToPlayer(ObjectGuid playerGUID, MemberSlot* slot)
{
    Player* player = ObjectAccessor::FindPlayer(playerGUID);
    if (!player || !player->GetSession() || player->GetGroup() != this)
        return;

    // if MemberSlot wasn't provided
    if (!slot)
    {
        member_witerator witr = _getMemberWSlot(playerGUID);

        if (witr == m_memberSlots.end()) // if there is no MemberSlot for such a player
            return;

        slot = &(*witr);
    }

    //LK OK                                           // guess size
    WorldPacket data(SMSG_GROUP_LIST, (1 + 1 + 1 + 1 + 8 + 4 + GetMembersCount() * 20));
    data << uint8(m_groupType);                       // group type
#ifndef LICH_KING
    data << uint8(isBGGroup() ? 1 : 0);               // 2.0.x
#endif
    data << uint8(slot->group);                       // groupid
    data << uint8(slot->flags);
#ifdef LICH_KING
    data << (uint8)(slot->roles); //lfg role
#endif
#ifdef LICH_KING
    if (isLFGGroup())
    {
        data << uint8(sLFGMgr->GetState(m_guid) == lfg::LFG_STATE_FINISHED_DUNGEON ? 2 : 0); // FIXME - Dungeon save status? 2 = done
        data << uint32(sLFGMgr->GetDungeon(m_guid));
    }
    data << uint64(m_guid);
    data << uint32(m_counter++);                        // 3.3, value increases every time this packet gets sent
#else
    data << uint64(0x50000000FFFFFFFELL);               // related to voice chat?
#endif
    data << uint32(GetMembersCount() - 1);
    for (member_citerator citr2 = m_memberSlots.begin(); citr2 != m_memberSlots.end(); ++citr2)
    {
        if (slot->guid == citr2->guid)
            continue;

        Player* member = ObjectAccessor::FindConnectedPlayer(citr2->guid);

        uint8 onlineState = (member && !member->GetSession()->PlayerLogout()) ? MEMBER_STATUS_ONLINE : MEMBER_STATUS_OFFLINE;
        onlineState = onlineState | ((isBGGroup()) ? MEMBER_STATUS_PVP : 0);

        data << citr2->name;
        data << uint64(citr2->guid);
        data << uint8(onlineState);
        data << uint8(citr2->group);                  // groupid
        data << uint8(citr2->flags);                    // See enum GroupMemberFlags
#ifdef LICH_KING
        data << uint8(citr2->roles);
#endif
    }

    data << uint64(m_leaderGuid);                       // leader guid
    if (GetMembersCount() - 1)
    {
        data << uint8(m_lootMethod);                    // loot method
        if (m_lootMethod == MASTER_LOOT)
            data << uint64(m_masterLooterGuid);         // master looter guid
        else
            data << uint64(0);
        data << uint8(m_lootThreshold);                 // loot threshold
        data << uint8(m_dungeonDifficulty);             // Heroic Mod Group
#ifdef LICH_KING
        data << uint8(m_raidDifficulty);                // Raid Difficulty
        data << uint8(m_raidDifficulty >= RAID_DIFFICULTY_10MAN_HEROIC);    // 3.3 Dynamic Raid Difficulty - 0 normal/1 heroic
#endif
    }
    player->SendDirectMessage(&data);
}

void Group::GetDataForXPAtKill(Unit const* victim, uint32& count,uint32& sum_level, Player* & member_with_max_level, Player* & not_gray_member_with_max_level)
{
    for(GroupReference *itr = GetFirstMember(); itr != nullptr; itr = itr->next())
    {
        Player* member = itr->GetSource();
        if(!member || !member->IsAlive())                   // only for alive
            continue;

        if(!member->IsAtGroupRewardDistance(victim))        // at req. distance
            continue;

        ++count;
        sum_level += member->GetLevel();
        // store maximum member level
        if(!member_with_max_level || member_with_max_level->GetLevel() < member->GetLevel())
            member_with_max_level = member;

        uint32 gray_level = Trinity::XP::GetGrayLevel(member->GetLevel());
        // if the victim is higher level than the gray level of the currently examined group member,
        // then set not_gray_member_with_max_level if needed.
        if( victim->GetLevel() > gray_level && (!not_gray_member_with_max_level
           || not_gray_member_with_max_level->GetLevel() < member->GetLevel()))
            not_gray_member_with_max_level = member;
    }
}

// Automatic Update by World thread
void Group::Update(time_t diff)
{
    //change the leader if it has disconnect for a long time
    if (m_leaderLogoutTime) 
    {
        time_t thisTime = WorldGameTime::GetGameTime();

        if (thisTime > m_leaderLogoutTime + sWorld->getConfig(CONFIG_GROUPLEADER_RECONNECT_PERIOD)) {
            ChangeLeaderToFirstOnlineMember();
            m_leaderLogoutTime = 0;
        }
    }
}

void Group::UpdatePlayerOutOfRange(Player* player)
{
    if (!player || !player->IsInWorld())
        return;

    //sunstrider: Only build packet if needed
    std::list<Player const*> sendTo;
    Player const* member;
    for (GroupReference *itr = GetFirstMember(); itr != nullptr; itr = itr->next())
    {
        member = itr->GetSource();
        if (member && member != player && (!member->IsInMap(player) || !member->IsWithinDist(player, member->GetSightRange(), false))) //use HaveAtClient instead?
            sendTo.push_back(member);
    }

    if (sendTo.empty())
        return;

    WorldPacket data;
    player->GetSession()->BuildPartyMemberStatsChangedPacket(player, &data);
    for (auto itr : sendTo)
        itr->SendDirectMessage(&data);
}

void Group::BroadcastPacket(WorldPacket *packet, bool ignorePlayersInBGRaid, int group, ObjectGuid ignoredPlayer)
{
    for(GroupReference *itr = GetFirstMember(); itr != nullptr; itr = itr->next())
    {
        Player* player = itr->GetSource();
        if(!player || (!ignoredPlayer.IsEmpty() && player->GetGUID() == ignoredPlayer) || (ignorePlayersInBGRaid && player->GetGroup() != this))
            continue;

        if (player->GetSession() && (group == -1 || itr->getSubGroup() == group))
            player->SendDirectMessage(packet);
    }
}

void Group::BroadcastReadyCheck(WorldPacket *packet)
{
    for(GroupReference *itr = GetFirstMember(); itr != nullptr; itr = itr->next())
    {
        Player *pl = itr->GetSource();
        if(pl && pl->GetSession())
            if(IsLeader(pl->GetGUID()) || IsAssistant(pl->GetGUID()))
                pl->SendDirectMessage(packet);
    }
}

void Group::OfflineReadyCheck()
{
    for(member_citerator citr = m_memberSlots.begin(); citr != m_memberSlots.end(); ++citr)
    {
        Player *pl = ObjectAccessor::FindPlayer(citr->guid);
        if (!pl || !pl->GetSession())
        {
            WorldPacket data(MSG_RAID_READY_CHECK_CONFIRM, 9); //BC + LK ok
            data << citr->guid;
            data << (uint8)0;
            BroadcastReadyCheck(&data);
        }
    }
}

bool Group::_setMembersGroup(ObjectGuid guid, const uint8 &group)
{
    auto slot = _getMemberWSlot(guid);
    if (slot == m_memberSlots.end())
        return false;

    slot->group = group;

    SubGroupCounterIncrease(group);

    if (!isBGGroup() && !isBFGroup())
    {
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GROUP_MEMBER_SUBGROUP);

        stmt->setUInt8(0, group);
        stmt->setUInt32(1, guid.GetCounter());

        CharacterDatabase.Execute(stmt);
    }

    return true;
}

bool Group::SameSubGroup(Player const* member1, Player const* member2) const
{
    if (!member1 || !member2)
        return false;

    if (member1->GetGroup() != this || member2->GetGroup() != this)
        return false;
    else
        return member1->GetSubGroup() == member2->GetSubGroup();
}

// Allows setting sub groups both for online or offline members
void Group::ChangeMembersGroup(ObjectGuid guid, uint8 group)
{
    // Only raid groups have sub groups
    if (!isRaidGroup())
        return;

    // Check if player is really in the raid
    member_witerator slot = _getMemberWSlot(guid);
    if (slot == m_memberSlots.end())
        return;

    // Abort if the player is already in the target sub group
    uint8 prevSubGroup = GetMemberGroup(guid);
    if (prevSubGroup == group)
        return;

    // Update the player slot with the new sub group setting
    slot->group = group;

    // Increase the counter of the new sub group..
    SubGroupCounterIncrease(group);

    // ..and decrease the counter of the previous one
    SubGroupCounterDecrease(prevSubGroup);

    // Preserve new sub group in database for non-raid groups
    if (!isBGGroup() && !isBFGroup())
    {
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GROUP_MEMBER_SUBGROUP);

        stmt->setUInt8(0, group);
        stmt->setUInt32(1, guid.GetCounter());

        CharacterDatabase.Execute(stmt);
    }

    // In case the moved player is online, update the player object with the new sub group references
    if (Player* player = ObjectAccessor::FindConnectedPlayer(guid))
    {
        if (player->GetGroup() == this)
            player->GetGroupRef().setSubGroup(group);
        else
        {
            // If player is in BG raid, it is possible that he is also in normal raid - and that normal raid is stored in m_originalGroup reference
            prevSubGroup = player->GetOriginalSubGroup();
            player->GetOriginalGroupRef().setSubGroup(group);
        }
    }

    // Broadcast the changes to the group
    SendUpdate();
}

// Retrieve the next Round-Roubin player for the group
//
// No update done if loot method is FFA.
//
// If the RR player is not yet set for the group, the first group member becomes the round-robin player.
// If the RR player is set, the next player in group becomes the round-robin player.
//
// If ifneed is true,
//      the current RR player is checked to be near the looted object.
//      if yes, no update done.
//      if not, he loses his turn.
void Group::UpdateLooterGuid(WorldObject* pLootedObject, bool ifneed)
{
    // round robin style looting applies for all low
    // quality items in each loot method except free for all
    if (GetLootMethod() == FREE_FOR_ALL)
        return;

    ObjectGuid oldLooterGUID = GetLooterGuid();
    auto guid_itr = _getMemberCSlot(oldLooterGUID);
    if(guid_itr != m_memberSlots.end())
    {
        if (ifneed)
        {
            // not update if only update if need and ok
            Player* looter = ObjectAccessor::FindPlayer(guid_itr->guid);
            if (looter && looter->IsAtGroupRewardDistance(pLootedObject))
                return;
        }
        ++guid_itr;
    }

    // search next after current
    Player* pNewLooter = nullptr;
    for (member_citerator itr = guid_itr; itr != m_memberSlots.end(); ++itr)
    {
        if (Player* player = ObjectAccessor::FindPlayer(itr->guid))
            if (player->IsAtGroupRewardDistance(pLootedObject))
            {
                pNewLooter = player;
                break;
            }
    }

    if (!pNewLooter)
    {
        // search from start
        for (member_citerator itr = m_memberSlots.begin(); itr != guid_itr; ++itr)
        {
            if (Player* player = ObjectAccessor::FindPlayer(itr->guid))
                if (player->IsAtGroupRewardDistance(pLootedObject))
                {
                    pNewLooter = player;
                    break;
                }
        }
    }

    if (pNewLooter)
    {
        if (oldLooterGUID != pNewLooter->GetGUID())
        {
            SetLooterGuid(pNewLooter->GetGUID());
            SendUpdate();
        }
    }
    else
    {
        SetLooterGuid(ObjectGuid::Empty);
        SendUpdate();
    }
}

GroupJoinBattlegroundResult Group::CanJoinBattlegroundQueue(Battleground const* bgOrTemplate, BattlegroundQueueTypeId bgQueueTypeId, uint32 MinPlayerCount, uint32 /*MaxPlayerCount*/, bool isRated, uint32 arenaSlot)
{
#ifdef LICH_KING
    // check if this group is LFG group
    if (isLFGGroup())
        return ERR_LFG_CANT_USE_BATTLEGROUND;
#endif

    BattlemasterListEntry const* bgEntry = sBattlemasterListStore.LookupEntry(bgOrTemplate->GetTypeID());
    if (!bgEntry)
        return ERR_GROUP_JOIN_BATTLEGROUND_FAIL;            // shouldn't happen

                                                            // check for min / max count
    uint32 memberscount = GetMembersCount();

#ifdef LICH_KING
    if (memberscount > bgEntry->maxGroupSize)                // no MinPlayerCount for battlegrounds
#else
    if (memberscount > bgEntry->maxplayersperteam)
#endif
        return ERR_BATTLEGROUND_NONE;                        // ERR_GROUP_JOIN_BATTLEGROUND_TOO_MANY handled on client side

    // get a player as reference, to compare other players' stats to (arena team id, queue id based on level, etc.)
    Player* reference = ASSERT_NOTNULL(GetFirstMember())->GetSource();
    // no reference found, can't join this way
    if (!reference)
#ifdef LICH_KING
        return ERR_BATTLEGROUND_JOIN_FAILED;
#else
        return ERR_BATTLEGROUND_NONE;
#endif

    PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketByLevel(bgOrTemplate->GetMapId(), reference->GetLevel());
    if (!bracketEntry)
#ifdef LICH_KING
        return ERR_BATTLEGROUND_JOIN_FAILED;
#else
        return ERR_BATTLEGROUND_NONE;
#endif

    uint32 arenaTeamId = reference->GetArenaTeamId(arenaSlot);
    uint32 team = reference->GetTeam();

#ifdef LICH_KING
    BattlegroundQueueTypeId bgQueueTypeIdRandom = BattlegroundMgr::BGQueueTypeId(BATTLEGROUND_RB, 0);
#endif

    // check every member of the group to be able to join
    memberscount = 0;
    for (GroupReference* itr = GetFirstMember(); itr != nullptr; itr = itr->next(), ++memberscount)
    {
        Player* member = itr->GetSource();
        // offline member? don't let join
        if (!member)
#ifdef LICH_KING
            return ERR_BATTLEGROUND_JOIN_FAILED;
#else
            return FAKE_ERR_BATTLEGROUND_OFFLINE_MEMBER;
#endif
        // don't allow cross-faction join as group
        if (member->GetTeam() != team)
#ifdef LICH_KING
            return ERR_BATTLEGROUND_JOIN_TIMED_OUT;
#else
            return ERR_BATTLEGROUND_MIXED_TEAM;
#endif
        // not in the same battleground level braket, don't let join
        PvPDifficultyEntry const* memberBracketEntry = GetBattlegroundBracketByLevel(bracketEntry->mapId, member->GetLevel());
        if (memberBracketEntry != bracketEntry)
#ifdef LICH_KING
            return ERR_BATTLEGROUND_JOIN_RANGE_INDEX;
#else
            return FAKE_ERR_BATTLEGROUND_MIXED_LEVELS;
#endif
        // don't let join rated matches if the arena team id doesn't match
        if (isRated && member->GetArenaTeamId(arenaSlot) != arenaTeamId)
#ifdef LICH_KING
            return ERR_BATTLEGROUND_JOIN_FAILED;
#else
            return ERR_BATTLEGROUND_NONE;
#endif
        // don't let join if someone from the group is already in that bg queue
        if (bgOrTemplate->GetTypeID() != BATTLEGROUND_AA && member->InBattlegroundQueueForBattlegroundQueueType(bgQueueTypeId))
#ifdef LICH_KING
            return ERR_BATTLEGROUND_JOIN_FAILED;  // not blizz-like
#else
            return FAKE_ERR_BATTLEGROUND_ALREADY_IN_QUEUE;  // not blizz-like
#endif           
#ifdef LICH_KING
        // don't let join if someone from the group is in bg queue random
        if (member->InBattlegroundQueueForBattlegroundQueueType(bgQueueTypeIdRandom))
            return ERR_IN_RANDOM_BG;
        // don't let join to bg queue random if someone from the group is already in bg queue
        if (bgOrTemplate->GetTypeID() == BATTLEGROUND_RB && member->InBattlegroundQueue(true))
            return ERR_IN_NON_RANDOM_BG;
#endif
        // check for deserter debuff in case not arena queue
        if (bgOrTemplate->GetTypeID() != BATTLEGROUND_AA && !member->CanJoinToBattleground(bgOrTemplate))
            return ERR_GROUP_JOIN_BATTLEGROUND_DESERTERS;
        // check if member can join any more battleground queues
        if (!member->HasFreeBattlegroundQueueId())
            return ERR_BATTLEGROUND_TOO_MANY_QUEUES;        // not blizz-like
#ifdef LICH_KING
        // check if someone in party is using dungeon system
        if (member->isUsingLfg())
            return ERR_LFG_CANT_USE_BATTLEGROUND;
#endif
        // check Freeze debuff
        if (member->HasAura(9454))
#ifdef LICH_KING
            return ERR_BATTLEGROUND_JOIN_FAILED;  // not blizz-like
#else
            return FAKE_ERR_BATTLEGROUND_FROZEN;  // not blizz-like
#endif         
    }

    // only check for MinPlayerCount since MinPlayerCount == MaxPlayerCount for arenas...
    if (bgOrTemplate->IsArena() && memberscount != MinPlayerCount)
#ifdef LICH_KING
        return ERR_ARENA_TEAM_PARTY_SIZE;
#else
        return FAKE_ERR_BATTLEGROUND_TEAM_SIZE;
#endif

    return GroupJoinBattlegroundResult(bgOrTemplate->GetTypeID());
}

//===================================================
//============== Roll ===============================
//===================================================

void Roll::targetObjectBuildLink()
{
    // called from link()
    getTarget()->addLootValidatorRef(this);
}

void Group::SetDifficulty(Difficulty difficulty)
{
    ASSERT(uint32(difficulty) < MAX_DIFFICULTY);
    m_dungeonDifficulty = difficulty;
    if (!isBGGroup() && !isBFGroup())
    {
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GROUP_DIFFICULTY);

        stmt->setUInt8(0, uint8(m_dungeonDifficulty));
        stmt->setUInt32(1, m_dbStoreId);

        CharacterDatabase.Execute(stmt);
    }

    for (GroupReference* itr = GetFirstMember(); itr != nullptr; itr = itr->next())
    {
        Player* player = itr->GetSource();
        if (!player->GetSession())
            continue;

        player->SetDungeonDifficulty(difficulty);
        player->SendDungeonDifficulty(true);
    }
}

bool Group::InCombatToInstance(uint32 instanceId)
{
    for(GroupReference *itr = GetFirstMember(); itr != nullptr; itr = itr->next())
    {
        Player *pPlayer = itr->GetSource();
        if(pPlayer && pPlayer->GetAttackers().size() && pPlayer->GetInstanceId() == instanceId && (pPlayer->GetMap()->IsRaid() || pPlayer->GetMap()->IsHeroic()))
            for(auto i : pPlayer->GetAttackers())
                if(i && i->GetTypeId() == TYPEID_UNIT && (i->ToCreature())->GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_INSTANCE_BIND)
                    return true;
    }
    return false;
}

void Group::ResetInstances(uint8 method, bool isRaid, Player* SendMsgTo)
{
    if (isBGGroup() || isBFGroup())
        return;

    // method can be INSTANCE_RESET_ALL, INSTANCE_RESET_CHANGE_DIFFICULTY, INSTANCE_RESET_GROUP_DISBAND

    // we assume that when the difficulty changes, all instances that can be reset will be
    uint8 dif = GetDifficulty();

    for(auto itr = m_boundInstances[dif].begin(); itr != m_boundInstances[dif].end();)
    {
        InstanceSave* instanceSave = itr->second.save;
        const MapEntry* entry = sMapStore.LookupEntry(itr->first);
        if (!entry || entry->IsRaid() != isRaid || (!instanceSave->CanReset() && method != INSTANCE_RESET_GROUP_DISBAND) || entry->MapID == 580) //HACK 580 = The Sunwell
        {
            ++itr;
            continue;
        }

        if(method == INSTANCE_RESET_ALL)
        {
            // the "reset all instances" method can only reset normal maps
            if(dif == DUNGEON_DIFFICULTY_HEROIC || entry->map_type == MAP_RAID)
            {
                ++itr;
                continue;
            }
        }

        bool isEmpty = true;
        // if the map is loaded, reset it
        Map* map = sMapMgr->FindMap(instanceSave->GetMapId(), instanceSave->GetInstanceId());
        if (map && map->IsDungeon() && !(method == INSTANCE_RESET_GROUP_DISBAND && !instanceSave->CanReset()))
        {
            if(instanceSave->CanReset())
                isEmpty = ((InstanceMap*)map)->Reset(method);
            else
                isEmpty = !map->HavePlayers();
        }

        if(SendMsgTo)
        {
            if (!isEmpty)
                SendMsgTo->SendResetInstanceFailed(0, instanceSave->GetMapId());
            else
            {
                if (Group* group = SendMsgTo->GetGroup())
                {
                    for (GroupReference* groupRef = group->GetFirstMember(); groupRef != nullptr; groupRef = groupRef->next())
                        if (Player* player = groupRef->GetSource())
                            player->SendResetInstanceSuccess(instanceSave->GetMapId());
                }

                else
                    SendMsgTo->SendResetInstanceSuccess(instanceSave->GetMapId());
            }
        }

        if(isEmpty || method == INSTANCE_RESET_GROUP_DISBAND || method == INSTANCE_RESET_CHANGE_DIFFICULTY)
        {
            // do not reset the instance, just unbind if others are permanently bound to it
            if (isEmpty && instanceSave->CanReset())
                instanceSave->DeleteFromDB();
            else
            {
                PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GROUP_INSTANCE_BY_INSTANCE);

                stmt->setUInt32(0, instanceSave->GetInstanceId());

                CharacterDatabase.Execute(stmt);
            }
            // i don't know for sure if hash_map iterators
            m_boundInstances[dif].erase(itr);
            itr = m_boundInstances[dif].begin();
            // this unloads the instance save unless online players are bound to it
            // (eg. permanent binds or GM solo binds)
            instanceSave->RemoveGroup(this);
        }
        else
            ++itr;
    }
}


InstanceGroupBind* Group::GetBoundInstance(Player* player)
{
    uint32 mapid = player->GetMapId();
    MapEntry const* mapEntry = sMapStore.LookupEntry(mapid);
    return GetBoundInstance(mapEntry);
}

InstanceGroupBind* Group::GetBoundInstance(Map* aMap)
{
    // Currently spawn numbering not different from map difficulty
    Difficulty difficulty = GetDifficulty(aMap->IsRaid());
    return GetBoundInstance(difficulty, aMap->GetId());
}

InstanceGroupBind* Group::GetBoundInstance(MapEntry const* mapEntry)
{
    if (!mapEntry || !mapEntry->IsDungeon())
        return nullptr;

    Difficulty difficulty = GetDifficulty(mapEntry->IsRaid());
    return GetBoundInstance(difficulty, mapEntry->MapID);
}

void Group::SetGroupMemberFlag(ObjectGuid guid, bool apply, GroupMemberFlags flag)
{
    // Assistants, main assistants and main tanks are only available in raid groups
    if (!isRaidGroup())
        return;

    // Check if player is really in the raid
    member_witerator slot = _getMemberWSlot(guid);
    if (slot == m_memberSlots.end())
        return;

    // Do flag specific actions, e.g ensure uniqueness
    switch (flag)
    {
    case MEMBER_FLAG_MAINASSIST:
        RemoveUniqueGroupMemberFlag(MEMBER_FLAG_MAINASSIST);         // Remove main assist flag from current if any.
        break;
    case MEMBER_FLAG_MAINTANK:
        RemoveUniqueGroupMemberFlag(MEMBER_FLAG_MAINTANK);           // Remove main tank flag from current if any.
        break;
    case MEMBER_FLAG_ASSISTANT:
        break;
    default:
        return;                                                      // This should never happen
    }

    // Switch the actual flag
    ToggleGroupMemberFlag(slot, flag, apply);

    // Preserve the new setting in the db
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GROUP_MEMBER_FLAG);

    stmt->setUInt8(0, slot->flags);
    stmt->setUInt32(1, guid.GetCounter());

    CharacterDatabase.Execute(stmt);

    // Broadcast the changes to the group
    SendUpdate();
}

Difficulty Group::GetDifficulty(bool isRaid) const
{
    return isRaid ? m_raidDifficulty : m_dungeonDifficulty;
}

bool Group::isRollLootActive() const
{
    return !RollId.empty();
}

Group::Rolls::iterator Group::GetRoll(ObjectGuid Guid)
{
    Rolls::iterator iter;
    for (iter = RollId.begin(); iter != RollId.end(); ++iter)
        if ((*iter)->itemGUID == Guid && (*iter)->isValid())
            return iter;
    return RollId.end();
}

void Group::LinkMember(GroupReference* pRef)
{
    m_memberMgr.insertFirst(pRef);
}

void Group::DelinkMember(ObjectGuid guid)
{
    GroupReference* ref = m_memberMgr.getFirst();
    while (ref)
    {
        GroupReference* nextRef = ref->next();
        if (ref->GetSource()->GetGUID() == guid)
        {
            ref->unlink();
            break;
        }
        ref = nextRef;
    }
}

void Group::_initRaidSubGroupsCounter()
{
    // Sub group counters initialization
    if (!m_subGroupsCounts)
        m_subGroupsCounts = new uint8[MAX_RAID_SUBGROUPS];

    memset((void*)m_subGroupsCounts, 0, (MAX_RAID_SUBGROUPS) * sizeof(uint8));

    for (member_citerator itr = m_memberSlots.begin(); itr != m_memberSlots.end(); ++itr)
        ++m_subGroupsCounts[itr->group];
}

Group::member_citerator Group::_getMemberCSlot(ObjectGuid Guid) const
{
    for (member_citerator itr = m_memberSlots.begin(); itr != m_memberSlots.end(); ++itr)
        if (itr->guid == Guid)
            return itr;
    return m_memberSlots.end();
}

Group::member_witerator Group::_getMemberWSlot(ObjectGuid Guid)
{
    for (member_witerator itr = m_memberSlots.begin(); itr != m_memberSlots.end(); ++itr)
        if (itr->guid == Guid)
            return itr;
    return m_memberSlots.end();
}

void Group::SubGroupCounterIncrease(uint8 subgroup)
{
    if (m_subGroupsCounts)
        ++m_subGroupsCounts[subgroup];
}

void Group::SubGroupCounterDecrease(uint8 subgroup)
{
    if (m_subGroupsCounts)
        --m_subGroupsCounts[subgroup];
}

void Group::RemoveUniqueGroupMemberFlag(GroupMemberFlags flag)
{
    for (member_witerator itr = m_memberSlots.begin(); itr != m_memberSlots.end(); ++itr)
        if (itr->flags & flag)
            itr->flags &= ~flag;
}

InstanceGroupBind* Group::GetBoundInstance(Difficulty difficulty, uint32 mapId)
{
    if (uint32(difficulty) >= MAX_DUNGEON_DIFFICULTY)
    {
        TC_LOG_ERROR("misc", "Group::GetBoundInstance called with invalid difficulty %u", uint32(difficulty));
        return nullptr;
    }

    // some instances only have one difficulty
    GetDownscaledMapDifficultyData(mapId, difficulty);

    auto itr = m_boundInstances[difficulty].find(mapId);
    if (itr != m_boundInstances[difficulty].end())
        return &itr->second;
    else
        return nullptr;
}

Group::BoundInstancesMap& Group::GetBoundInstances(Difficulty difficulty)
{
    return m_boundInstances[difficulty];
}

InstanceGroupBind* Group::BindToInstance(InstanceSave *save, bool permanent, bool load)
{
    if (!save || isBGGroup() || isBFGroup())
        return nullptr;

    InstanceGroupBind& bind = m_boundInstances[save->GetDifficulty()][save->GetMapId()];
    if (!load && (!bind.save || permanent != bind.perm || save != bind.save))
    {
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_REP_GROUP_INSTANCE);

        stmt->setUInt32(0, m_dbStoreId);
        stmt->setUInt32(1, save->GetInstanceId());
        stmt->setBool(2, permanent);

        CharacterDatabase.Execute(stmt);
    }

    if (bind.save != save)
    {
        if (bind.save)
            bind.save->RemoveGroup(this);
        save->AddGroup(this);
    }

    bind.save = save;
    bind.perm = permanent;
    if (!load)
        TC_LOG_DEBUG("maps", "Group::BindToInstance: %s, storage id: %u is now bound to map %d, instance %d, difficulty %d",
            GetGUID().ToString().c_str(), m_dbStoreId, save->GetMapId(), save->GetInstanceId(), save->GetDifficulty());

    return &bind;
}

void Group::UnbindInstance(uint32 mapid, uint8 difficulty, bool unload)
{
    BoundInstancesMap::iterator itr = m_boundInstances[difficulty].find(mapid);
    if (itr != m_boundInstances[difficulty].end())
    {
        if (!unload)
        {
            PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GROUP_INSTANCE_BY_GUID);

            stmt->setUInt32(0, m_dbStoreId);
            stmt->setUInt32(1, itr->second.save->GetInstanceId());

            CharacterDatabase.Execute(stmt);
        }

        itr->second.save->RemoveGroup(this);                // save can become invalid
        m_boundInstances[difficulty].erase(itr);
    }
}

void Group::_homebindIfInstance(Player *player)
{
    if(player && !player->IsGameMaster() && sMapStore.LookupEntry(player->GetMapId())->IsDungeon())
    {
        // leaving the group in an instance, the homebind timer is started
        // sun: only leave if you're not bound to this instance, else player will only have to enter again
        //      also exclude leader to avoid kicking him when a two players group disband
        InstanceSave *save = sInstanceSaveMgr->GetInstanceSave(player->GetInstanceId());
        InstancePlayerBind *playerBind = save ? player->GetBoundInstance(save->GetMapId(), save->GetDifficulty()) : NULL;
        if(GetLeaderGUID() != player->GetGUID() && (!playerBind || !playerBind->perm))
            player->m_InstanceValid = false;
    }
}

void Group::BroadcastGroupUpdate()
{
    // FG: HACK: force flags update on group leave - for values update hack
    // -- not very efficient but safe
    for(member_citerator citr = m_memberSlots.begin(); citr != m_memberSlots.end(); ++citr)
    {

        Player *pp = ObjectAccessor::FindPlayer(citr->guid);
        if(pp && pp->IsInWorld())
        {
            pp->ForceValuesUpdateAtIndex(UNIT_FIELD_BYTES_2);
            pp->ForceValuesUpdateAtIndex(UNIT_FIELD_FACTIONTEMPLATE);
            TC_LOG_DEBUG("network.opcode","-- Forced group value update for '%s'", pp->GetName().c_str());
        }
    }
}

void Group::SetLootMethod(LootMethod method)
{
    m_lootMethod = method;
}

void Group::SetLooterGuid(ObjectGuid guid)
{
    m_looterGuid = guid;
}

void Group::SetMasterLooterGuid(ObjectGuid guid)
{
    m_masterLooterGuid = guid;
}

void Group::SetLootThreshold(ItemQualities threshold)
{
    m_lootThreshold = threshold;
}

bool Group::IsFull() const
{
    return isRaidGroup() ? (m_memberSlots.size() >= MAXRAIDSIZE) : (m_memberSlots.size() >= MAXGROUPSIZE);
}

bool Group::isRaidGroup() const
{
    return (m_groupType & GROUPTYPE_RAID) != 0;
}

bool Group::isBGGroup() const
{
    return m_bgGroup != nullptr;
}

bool Group::IsCreated() const
{
    return GetMembersCount() > 0;
}

ObjectGuid Group::GetLeaderGUID() const
{
    return m_leaderGuid;
}

ObjectGuid Group::GetGUID() const
{
    return m_guid;
}

ObjectGuid::LowType Group::GetLowGUID() const
{
    return m_guid.GetCounter();
}

char const* Group::GetLeaderName() const
{
    return m_leaderName.c_str();
}

LootMethod Group::GetLootMethod() const
{
    return m_lootMethod;
}

ObjectGuid Group::GetLooterGuid() const
{
    if (GetLootMethod() == FREE_FOR_ALL)
        return ObjectGuid::Empty;
    return m_looterGuid;
}

ObjectGuid Group::GetMasterLooterGuid() const
{
    return m_masterLooterGuid;
}

ItemQualities Group::GetLootThreshold() const
{
    return m_lootThreshold;
}

bool Group::IsMember(ObjectGuid guid) const
{
    return _getMemberCSlot(guid) != m_memberSlots.end();
}

bool Group::IsLeader(ObjectGuid guid) const
{
    return (GetLeaderGUID() == guid);
}

ObjectGuid Group::GetMemberGUID(const std::string& name)
{
    for (member_citerator itr = m_memberSlots.begin(); itr != m_memberSlots.end(); ++itr)
        if (itr->name == name)
            return itr->guid;
    return ObjectGuid::Empty;
}

uint8 Group::GetMemberFlags(ObjectGuid guid) const
{
    member_citerator mslot = _getMemberCSlot(guid);
    if (mslot == m_memberSlots.end())
        return 0u;
    return mslot->flags;
}

bool Group::SameSubGroup(ObjectGuid guid1, ObjectGuid guid2) const
{
    member_citerator mslot2 = _getMemberCSlot(guid2);
    if (mslot2 == m_memberSlots.end())
        return false;
    return SameSubGroup(guid1, &*mslot2);
}

bool Group::SameSubGroup(ObjectGuid guid1, MemberSlot const* slot2) const
{
    member_citerator mslot1 = _getMemberCSlot(guid1);
    if (mslot1 == m_memberSlots.end() || !slot2)
        return false;
    return (mslot1->group == slot2->group);
}

bool Group::HasFreeSlotSubGroup(uint8 subgroup) const
{
    return (m_subGroupsCounts && m_subGroupsCounts[subgroup] < MAXGROUPSIZE);
}


uint8 Group::GetMemberGroup(ObjectGuid guid) const
{
    member_citerator mslot = _getMemberCSlot(guid);
    if (mslot == m_memberSlots.end())
        return (MAX_RAID_SUBGROUPS + 1);
    return mslot->group;
}

void Group::SetBattlegroundGroup(Battleground* bg)
{
    m_bgGroup = bg;
}

void Group::ToggleGroupMemberFlag(member_witerator slot, uint8 flag, bool apply)
{
    if (apply)
        slot->flags |= flag;
    else
        slot->flags &= ~flag;
}
