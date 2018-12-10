
#ifndef __WORLDSESSION_H
#define __WORLDSESSION_H

#include "Common.h"
#include "WorldPacket.h"
#include "ClientControl.h"
#include "SharedDefines.h"
#include "QueryHolder.h"
#include "QueryCallback.h"
#include "PlayerAntiCheat.h"
#include "World.h"

#include <string>

class PlayerAntiCheat;
class MailItemsInfo;
struct ItemTemplate;
struct AuctionEntry;
struct DeclinedName;
struct MovementInfo;
class WardenBase;
class BigNumber;
struct AddonInfo;
enum MailMessageType : uint32;
class Transaction;
typedef std::shared_ptr<Transaction> SQLTransaction;
enum WeatherState : int;
class UpdateData;
class ReplayPlayer;
class ReplayRecorder;
class Creature;
class Item;
class Object;
class Player;
class Unit;
class WorldPacket;
class WorldSocket;
class WorldSession;
class LoginQueryHolder;
class CharacterHandler;
struct TradeStatusInfo;
struct Petition;

#ifdef LICH_KING
namespace lfg
{
struct LfgJoinResultData;
struct LfgPlayerBoot;
struct LfgProposal;
struct LfgQueueStatusData;
struct LfgPlayerRewardData;
struct LfgRoleCheck;
struct LfgUpdateData;
}
#endif

namespace WorldPackets
{
    namespace Query
    {
        class QueryCreature;
        class QueryGameObject;
        class QueryItemSingle;
#ifdef LICH_KING
        class QuestPOIQuery;
#endif
    }
    namespace Quest
    {
        class QueryQuestInfo;
    }
}

enum AccountDataType
{
    GLOBAL_CONFIG_CACHE             = 0,                    // 0x01 g
    PER_CHARACTER_CONFIG_CACHE      = 1,                    // 0x02 p
    GLOBAL_BINDINGS_CACHE           = 2,                    // 0x04 g
    PER_CHARACTER_BINDINGS_CACHE    = 3,                    // 0x08 p
    GLOBAL_MACROS_CACHE             = 4,                    // 0x10 g
    PER_CHARACTER_MACROS_CACHE      = 5,                    // 0x20 p
    PER_CHARACTER_LAYOUT_CACHE      = 6,                    // 0x40 p
    PER_CHARACTER_CHAT_CACHE        = 7                     // 0x80 p
};

#define NUM_ACCOUNT_DATA_TYPES        8

#define GLOBAL_CACHE_MASK           0x15
#define PER_CHARACTER_CACHE_MASK    0xEA

struct AccountData
{
    AccountData() : Time(0), Data("") { }

    time_t Time;
    std::string Data;
};
//end LK ONLY

namespace rbac
{
class RBACData;
}

enum PartyOperation
{
    PARTY_OP_INVITE = 0,
    PARTY_OP_UNINVITE = 1,
    PARTY_OP_LEAVE = 2,
    PARTY_OP_SWAP = 4
};

enum PartyResult
{
    PARTY_RESULT_OK                   = 0,
    PARTY_RESULT_CANT_FIND_TARGET     = 1,
    PARTY_RESULT_NOT_IN_YOUR_PARTY    = 2,
    PARTY_RESULT_NOT_IN_YOUR_INSTANCE = 3,
    PARTY_RESULT_PARTY_FULL           = 4,
    PARTY_RESULT_ALREADY_IN_GROUP     = 5,
    PARTY_RESULT_YOU_NOT_IN_GROUP     = 6,
    PARTY_RESULT_YOU_NOT_LEADER       = 7,
    PARTY_RESULT_TARGET_UNFRIENDLY    = 8,
    PARTY_RESULT_TARGET_IGNORE_YOU    = 9,
    PARTY_RESULT_PENDING_MATCH        = 12,
    PARTY_RESULT_INVITE_RESTRICTED    = 13
};

enum CharterTypes
{
    CHARTER_TYPE_NONE                             = 0,
    CHARTER_TYPE_ANY                              = 10,

    GUILD_CHARTER_TYPE                            = 9,
    ARENA_TEAM_CHARTER_2v2_TYPE                   = 2,
    ARENA_TEAM_CHARTER_3v3_TYPE                   = 3,
    ARENA_TEAM_CHARTER_5v5_TYPE                   = 5
};

//class to deal with packet processing
//allows to determine if next packet is safe to be processed
class PacketFilter
{
public:
    explicit PacketFilter(WorldSession * pSession) : m_pSession(pSession) {}
    virtual ~PacketFilter() = default;

    virtual bool Process(WorldPacket* packet) { return true; }
    virtual bool ProcessLogout() const { return true; }

protected:
    WorldSession * const m_pSession;
};
//process only thread-safe packets in Map::Update()
class MapSessionFilter : public PacketFilter
{
public:
    explicit MapSessionFilter(WorldSession * pSession) : PacketFilter(pSession) {}
    ~MapSessionFilter() override = default;

    bool Process(WorldPacket * packet) override;
    //in Map::Update() we do not process player logout!
    bool ProcessLogout() const override { return false; }
};

//class used to filer only thread-unsafe packets from queue
//in order to update only be used in World::UpdateSessions()
class WorldSessionFilter : public PacketFilter
{
public:
    explicit WorldSessionFilter(WorldSession* pSession) : PacketFilter(pSession) {}
    ~WorldSessionFilter() override
    = default;

    bool Process(WorldPacket* packet) override;
};

// Proxy structure to contain data passed to callback function,
// only to prevent bloating the parameter list
class CharacterCreateInfo
{
    friend class WorldSession;
    friend class Player;

    public:

    /// User specified variables
    std::string Name;
    uint8 Race       = 0;
    uint8 Class      = 0;
    uint8 Gender     = GENDER_NONE;
    uint8 Skin       = 0;
    uint8 Face       = 0;
    uint8 HairStyle  = 0;
    uint8 HairColor  = 0;
    uint8 FacialHair = 0;
    uint8 OutfitId   = 0;
    WorldPacket Data;

    /// Server side data
    uint8 CharCount = 0;

    // Randomize appearance and gender
    void RandomizeAppearance();
};

struct CharacterRenameInfo
{
    friend class WorldSession;

    protected:
        ObjectGuid Guid;
        std::string Name;
};

struct PacketCounter
{
    time_t lastReceiveTime;
    uint32 amountCounter;
};

/// Player session in the World
class TC_GAME_API WorldSession
{
    friend class CharacterHandler;
    public:
        WorldSession(uint32 id, ClientBuild clientBuild, std::string&& name, std::shared_ptr<WorldSocket> sock, AccountTypes sec, uint8 expansion, time_t mute_time, LocaleConstant locale, uint32 recruiter, bool isARecruiter);
        ~WorldSession();

        bool PlayerLoading() const { return m_playerLoading; }
        bool PlayerLogout() const { return m_playerLogout; }
        bool PlayerLogoutWithSave() const { return m_playerLogout && m_playerSave; }
        bool PlayerRecentlyLoggedOut() const { return m_playerRecentlyLogout; }
        
        ClientBuild GetClientBuild() const;

        void ReadAddonsInfo(ByteBuffer &data);
        void SendAddonsInfo();

        void SendPacket(WorldPacket const* packet);
        void SendNotification(const char *format,...) ATTR_PRINTF(2,3);
        void SendNotification(int32 string_id,...);
        void SendPetNameInvalid(uint32 error, const std::string& name, DeclinedName *declinedName);
        void SendLfgResult(uint32 type, uint32 entry, uint8 lfg_type);
        void SendPartyResult(PartyOperation operation, const std::string& member, PartyResult res);
        void SendAreaTriggerMessage(const char* Text, ...) ATTR_PRINTF(2,3);
        void SendQueryTimeResponse();

        void SendAuthResponse(uint8 code, bool shortForm, uint32 queuePos = 0);
#ifdef LICH_KING
        void SendClientCacheVersion(uint32 version);
#endif

        void InitializeSession();
        void InitializeSessionCallback(SQLQueryHolder* realmHolder);

        //RBAC system from TC, not yet implemented
        bool HasPermission(uint32 permissionId) { return true; }
        /*
        rbac::RBACData* GetRBACData();
        void LoadPermissions();
        QueryCallback LoadPermissionsAsync();
        void InvalidateRBACData(); // Used to force LoadPermissions at next HasPermission check
        */

        AccountTypes GetSecurity() const { return _security; }
        uint32 GetAccountId() const { return _accountId; }
        Player* GetPlayer() const { return _player; }
        std::string const& GetPlayerName() const;
        std::string GetPlayerInfo() const;

        ObjectGuid::LowType GetGUIDLow() const;
        void SetSecurity(AccountTypes security) { _security = security; }
        std::string const& GetRemoteAddress() const { return m_Address; }
        void SetPlayer(Player *plr) { _player = plr; }
        uint8 Expansion() const { return m_expansion; }

     /*   uint32 GetGroupId() const { return _groupid; }
        void SetGroupId(uint32 gid) { _groupid = gid; }*/

        /// Session in auth.queue currently
        void SetInQueue(bool state) { m_inQueue = state; }

        /// Is the user engaged in a log out process?
        bool isLogingOut() const { return _logoutTime || m_playerLogout; }

        /// Engage the logout process for the user
        void LogoutRequest(time_t requestTime)
        {
            _logoutTime = requestTime;
        }
        
        void InitWarden(BigNumber *K, std::string os);

        /// Is logout cooldown expired?
        bool ShouldLogOut(time_t currTime) const
        {
            return (_logoutTime > 0 && currTime >= _logoutTime + 20);
        }

        void LogoutPlayer(bool Save);
        void KickPlayer();
        // Returns true if all contained hyperlinks are valid
        // May kick player on false depending on world config (handler should abort)
        bool ValidateHyperlinksAndMaybeKick(std::string const& str);

        void QueuePacket(WorldPacket* new_packet);
        
        bool Update(uint32 diff, PacketFilter& updater);

        /// Handle the authentication waiting queue (to be completed)
        void SendAuthWaitQue(uint32 position);

        //void SendTestCreatureQueryOpcode( uint32 entry, ObjectGuid guid, uint32 testvalue );
        void SendNameQueryOpcode(ObjectGuid guid);

        void SendTrainerList(ObjectGuid guid);
        void SendTrainerList(ObjectGuid guid, const std::string& strTitle);
        void SendListInventory(ObjectGuid guid);
        void SendShowBank(ObjectGuid guid);
        bool CanOpenMailBox(ObjectGuid guid);
#ifdef LICH_KING
        void SendShowMailBox(ObjectGuid guid); //no such opcode on BC
#endif
        void SendTabardVendorActivate(ObjectGuid guid);
        void SendSpiritResurrect();
        void SendBindPoint(Creature* npc);

        void SendMeleeAttackStop(Unit const* enemy);

        void SendBattleGroundList( ObjectGuid guid, BattlegroundTypeId bgTypeId );

        void SendTradeStatus(TradeStatusInfo const& info);
        void SendCancelTrade();
        void SendUpdateTrade(bool trader_data = true);

        void SendPetitionQueryOpcode(ObjectGuid petitionguid);

        void SendMinimapPing(ObjectGuid guid, uint32 x, uint32 y);

        //pet
        void SendPetNameQuery(ObjectGuid guid, uint32 petnumber);
        void SendStablePet(ObjectGuid guid );
        void SendStablePetCallback(ObjectGuid guid, PreparedQueryResult result);
        void SendStableResult(uint8 guid);
        bool CheckStableMaster(ObjectGuid guid);

        //mount
        void SendMountResult(MountResult res);

        // Account Data
        //void SetAccountData(AccountDataType type, time_t tm, std::string const& data); //NYI
        //void LoadGlobalAccountData(); //NYI
        void LoadAccountData(PreparedQueryResult result, uint32 mask); //NYI
        AccountData* GetAccountData(AccountDataType type) { return &m_accountData[type]; }
        void SendAccountDataTimes(uint32 mask = 0);

        //Tutorial
        void LoadTutorialsData(PreparedQueryResult result);
        void SendTutorialsData();
        void SaveTutorialsData(SQLTransaction& trans);
        uint32 GetTutorialInt(uint8 index) const { return m_Tutorials[index]; }
        void SetTutorialInt(uint8 index, uint32 value)
        {
            if (m_Tutorials[index] != value)
            {
                m_Tutorials[index] = value;
                m_TutorialsChanged = true;
            }
        }

        void SendMotd();

        //title
        void SendTitleEarned(uint32 titleIndex, bool earned);

        //return item name for player local, or default to english if not found
        std::string GetLocalizedItemName(const ItemTemplate* proto);
        std::string GetLocalizedItemName(uint32 itemId);

        //auction
        void SendAuctionHello( ObjectGuid guid, Creature * unit );
        void SendAuctionCommandResult( uint32 auctionId, uint32 Action, uint32 ErrorCode, uint32 bidError = 0);
        void SendAuctionBidderNotification( uint32 location, uint32 auctionId, uint64 bidder, uint32 bidSum, uint32 diff, uint32 item_template);
        void SendAuctionOwnerNotification( AuctionEntry * auction );

        //Item Enchantment
        void SendEnchantmentLog(ObjectGuid Target, ObjectGuid Caster, uint32 ItemID, uint32 SpellID);
        void SendItemEnchantTimeUpdate(ObjectGuid Playerguid, ObjectGuid Itemguid,uint32 slot, uint32 Duration);

        //clear client target if target has given guid
        void SendClearTarget(uint64 target);

        //Taxi
        void SendTaxiStatus( ObjectGuid guid );
        void SendTaxiMenu(Creature* unit );
        void SendDoFlight(uint32 mountDisplayId, uint32 path, uint32 pathNode = 0);
        bool SendLearnNewTaxiNode( Creature* unit );
        void SendDiscoverNewTaxiNode(uint32 nodeid);

        // Guild/Arena Team
        void SendArenaTeamCommandResult(uint32 team_action, const std::string& team, const std::string& player, uint32 error_id);
        void BuildArenaTeamEventPacket(WorldPacket *data, uint8 eventid, uint8 str_count, const std::string& str1, const std::string& str2, const std::string& str3);
        void SendNotInArenaTeamPacket(uint8 type);
        void SendPetitionShowList( ObjectGuid guid );

        // Dynamic map data
        void SendPlayMusic(uint32 musicId);
        void SendWeather(WeatherState weatherId, float weatherGrade, uint8 unk = 0);
        //defaultLightId for current zone ?
        void SendOverrideLight(uint32 defaultLightId, uint32 overrideLightId, uint32 fadeTime);

        // Looking For Group
        // TRUE values set by client sending CMSG_LFG_SET_AUTOJOIN and CMSG_LFM_CLEAR_AUTOFILL before player login
        bool LookingForGroup_auto_join;
        bool LookingForGroup_auto_add;

        void BuildPartyMemberStatsChangedPacket(Player *player, WorldPacket *data);

        void DoLootRelease(ObjectGuid lguid);
        
        // Account mute time
        time_t m_muteTime;

        // Locales
        LocaleConstant GetSessionDbcLocale() const { return m_sessionDbcLocale; }
        LocaleConstant GetSessionDbLocaleIndex() const { return m_sessionDbLocaleIndex; }
        char const* GetTrinityString(int32 entry) const;

        // Latency from ping-pong
        uint32 GetLatency() const { return m_latency; }
        void SetLatency(uint32 latency) { m_latency = latency; }

        // Estimation of latency for this client, handled with CMSG_TIME_SYNC_RESP
        void ResetTimeSync();
        void SendTimeSync();
        int64 m_timeSyncClockDelta;
        uint32 m_timeSyncCounter;
        uint32 m_timeSyncTimer;
        uint32 m_timeSyncServer;

        ClientControl& GetClientControl() { return _clientControl; }
        ClientControl const& GetClientControl() const { return _clientControl; }

        bool IsReplaying() const { return m_replayPlayer != nullptr; }
        bool IsRecording() const { return m_replayRecorder != nullptr; }

        //Replay functions
        bool StartRecording(std::string const& recordName);
        bool StopRecording();
        bool StartReplaying(std::string const& recordName);
        bool StopReplaying();
        std::shared_ptr<ReplayPlayer> GetReplayPlayer() { return m_replayPlayer; }
        std::shared_ptr<ReplayRecorder> GetReplayRecorder() { return m_replayRecorder; }

        std::atomic<time_t> m_timeOutTime;

        void ResetTimeOutTime(bool onlyActive);

        bool IsConnectionIdle() const;

        PlayerAntiCheat anticheat;

    public:                                                 // opcodes handlers

        void Handle_NULL(WorldPacket& recvPacket);          // not used
        void Handle_EarlyProccess( WorldPacket& recvPacket);// just mark packets processed in WorldSocket::OnRead
        void Handle_ServerSide(WorldPacket& recvPacket);    // sever side only, can't be accepted from client
        void Handle_Deprecated(WorldPacket& recvPacket);    // never used anymore by client

        void HandleCharEnumOpcode(WorldPacket& recvPacket);
        void HandleCharDeleteOpcode(WorldPacket& recvPacket);
        void HandleCharCreateOpcode(WorldPacket& recvPacket);
        void HandlePlayerLoginOpcode(WorldPacket& recvPacket);
        void HandleCharEnum(PreparedQueryResult result);
        void HandlePlayerLogin(LoginQueryHolder * holder);
        void _HandlePlayerLogin(Player* player, LoginQueryHolder * holder);

        void SendCharCreate(ResponseCodes result);
        void SendCharDelete(ResponseCodes result);
        void SendCharRename(ResponseCodes result, CharacterRenameInfo const* renameInfo);

        // played time
        void HandlePlayedTime(WorldPacket& recvPacket);

        // new
        void HandleMovementFlagChangeAck(WorldPacket& recvPacket);
        void HandleMovementFlagChangeToggleAck(WorldPacket& recvPacket);
        void HandleLookingForGroup(WorldPacket& recvPacket);

        // new inspect
        void HandleInspectOpcode(WorldPacket& recvPacket);

        // new party stats
        void HandleInspectHonorStatsOpcode(WorldPacket& recvPacket);

        void HandleMountSpecialAnimOpcode(WorldPacket &recvdata);

        // character view
        void HandleShowingHelmOpcode(WorldPacket& recvData);
        void HandleShowingCloakOpcode(WorldPacket& recvData);

        // repair
        void HandleRepairItemOpcode(WorldPacket& recvPacket);

        // Knockback
        void HandleMoveKnockBackAck(WorldPacket& recvPacket);

        void HandleMoveTeleportAck(WorldPacket& recvPacket);
        void HandleForceSpeedChangeAck(WorldPacket & recvData);
#ifdef LICH_KING
        void HandleCollisionHeightChangeAck(WorldPacket& recvData);
#endif

        void HandleRepopRequestOpcode(WorldPacket& recvPacket);
        void HandleAutostoreLootItemOpcode(WorldPacket& recvPacket);
        void HandleLootMoneyOpcode(WorldPacket& recvPacket);
        void HandleLootOpcode(WorldPacket& recvPacket);
        void HandleLootReleaseOpcode(WorldPacket& recvPacket);
        void HandleLootMasterGiveOpcode(WorldPacket& recvPacket);
        void HandleWhoOpcode(WorldPacket& recvPacket);
        void HandleLogoutRequestOpcode(WorldPacket& recvPacket);
        void HandlePlayerLogoutOpcode(WorldPacket& recvPacket);
        void HandleLogoutCancelOpcode(WorldPacket& recvPacket);

        // GM Ticket opcodes
        void HandleGMTicketCreateOpcode(WorldPacket& recvPacket);
        void HandleGMTicketUpdateOpcode(WorldPacket& recvPacket);
        void HandleGMTicketDeleteOpcode(WorldPacket& recvPacket);
        void HandleGMTicketGetTicketOpcode(WorldPacket& recvPacket);
        void HandleGMTicketSystemStatusOpcode(WorldPacket& recvPacket);
        void SendGMTicketGetTicket(uint32 status, char const* text);
        void HandleGMSurveySubmit(WorldPacket& recvPacket);

        void HandleTogglePvP(WorldPacket& recvPacket);

        void HandleZoneUpdateOpcode(WorldPacket& recvPacket);
        void HandleSetTargetOpcode(WorldPacket& recvPacket);
        void HandleSetSelectionOpcode(WorldPacket& recvPacket);
        void HandleStandStateChangeOpcode(WorldPacket& recvPacket);
        void HandleEmoteOpcode(WorldPacket& recvPacket);
        void HandleContactListOpcode(WorldPacket& recvPacket);
        void HandleAddFriendOpcode(WorldPacket& recvPacket);
        void HandleDelFriendOpcode(WorldPacket& recvPacket);
        void HandleAddIgnoreOpcode(WorldPacket& recvPacket);
        void HandleDelIgnoreOpcode(WorldPacket& recvPacket);
        void HandleSetContactNotesOpcode(WorldPacket& recvPacket);
        void HandleBugOpcode(WorldPacket& recvPacket);
        void HandleSetAmmoOpcode(WorldPacket& recvPacket);
        void HandleItemNameQueryOpcode(WorldPacket& recvPacket);

        void HandleAreaTriggerOpcode(WorldPacket& recvPacket);

        void HandleSetFactionAtWar( WorldPacket & recvData );
        void HandleSetFactionCheat( WorldPacket & recvData );
        void HandleSetWatchedFactionOpcode(WorldPacket & recvData);
        void HandleSetFactionInactiveOpcode(WorldPacket & recvData);

        void HandleUpdateAccountData(WorldPacket& recvPacket);
        void HandleRequestAccountData(WorldPacket& recvPacket);
        void HandleSetActionButtonOpcode(WorldPacket& recvPacket);

        void HandleGameObjectUseOpcode(WorldPacket& recPacket);
        void HandleMeetingStoneInfo(WorldPacket& recPacket);

        void HandleNameQueryOpcode(WorldPacket& recvPacket);

        void HandleQueryTimeOpcode(WorldPacket& recvPacket);

        void HandleCreatureQueryOpcode(WorldPackets::Query::QueryCreature& query);

        void HandleGameObjectQueryOpcode(WorldPackets::Query::QueryGameObject& query);

        void HandleMoveWorldportAckOpcode(WorldPacket& recvPacket);
        void HandleMoveWorldportAck();                // for server-side calls

        void HandleMovementOpcodes(WorldPacket& recvPacket);
        void HandleSetActiveMoverOpcode(WorldPacket &recvData);
        void HandleMoveNotActiveMover(WorldPacket &recvData);
        void HandleMoveTimeSkippedOpcode(WorldPacket &recvData);

        void HandleRequestRaidInfoOpcode( WorldPacket & recvData );

        void HandleBattlefieldStatusOpcode(WorldPacket &recvData);

        void HandleGroupInviteOpcode(WorldPacket& recvPacket);
        void _HandleGroupInviteOpcode(Player* player, std::string membername);
        void HandleGroupAcceptOpcode(WorldPacket& recvPacket);
        void HandleGroupDeclineOpcode(WorldPacket& recvPacket);
        void HandleGroupUninviteOpcode(WorldPacket& recvPacket);
        void HandleGroupUninviteGuidOpcode(WorldPacket& recvPacket);
        void HandleGroupSetLeaderOpcode(WorldPacket& recvPacket);
        void HandleGroupDisbandOpcode(WorldPacket& recvPacket);
        void HandleOptOutOfLootOpcode( WorldPacket &recvData );
        void HandleLootMethodOpcode(WorldPacket& recvPacket);
        void HandleLootRoll( WorldPacket &recvData );
        void HandleRequestPartyMemberStatsOpcode( WorldPacket &recvData );
        void HandleGroupSwapSubGroupOpcode(WorldPacket& recv_data);
        void HandleRaidTargetUpdateOpcode( WorldPacket & recvData );
        void HandleRaidReadyCheckOpcode( WorldPacket & recvData );
        void HandleRaidReadyCheckFinishedOpcode( WorldPacket & recvData );
        void HandleGroupRaidConvertOpcode( WorldPacket & recvData );
        void HandleGroupChangeSubGroupOpcode( WorldPacket & recvData );
        void HandleGroupAssistantLeaderOpcode( WorldPacket & recvData );
        void HandlePartyAssignmentOpcode( WorldPacket & recvData );

        void HandlePetitionBuyOpcode(WorldPacket& recvData);
        void HandlePetitionShowSignOpcode(WorldPacket& recvData);
        void SendPetitionSigns(Petition const* petition, Player* sendTo);
        void HandlePetitionQueryOpcode(WorldPacket& recvData);
        void HandlePetitionRenameOpcode(WorldPacket& recvData);
        void HandlePetitionSignOpcode(WorldPacket& recvData);
        void HandlePetitionDeclineOpcode(WorldPacket& recvData);
        void HandleOfferPetitionOpcode(WorldPacket& recvData);
        void HandleTurnInPetitionOpcode(WorldPacket& recvData);

        void HandleGuildQueryOpcode(WorldPacket& recvPacket);
        void HandleGuildCreateOpcode(WorldPacket& recvPacket);
        void HandleGuildInviteOpcode(WorldPacket& recvPacket);
        void HandleGuildRemoveOpcode(WorldPacket& recvPacket);
        void HandleGuildAcceptOpcode(WorldPacket& recvPacket);
        void HandleGuildDeclineOpcode(WorldPacket& recvPacket);
        void HandleGuildInfoOpcode(WorldPacket& recvPacket);
        void HandleGuildEventLogQueryOpcode(WorldPacket& recvPacket);
        void HandleGuildRosterOpcode(WorldPacket& recvPacket);
        void HandleGuildPromoteOpcode(WorldPacket& recvPacket);
        void HandleGuildDemoteOpcode(WorldPacket& recvPacket);
        void HandleGuildLeaveOpcode(WorldPacket& recvPacket);
        void HandleGuildDisbandOpcode(WorldPacket& recvPacket);
        void HandleGuildLeaderOpcode(WorldPacket& recvPacket);
        void HandleGuildMOTDOpcode(WorldPacket& recvPacket);
        void HandleGuildSetPublicNoteOpcode(WorldPacket& recvPacket);
        void HandleGuildSetOfficerNoteOpcode(WorldPacket& recvPacket);
        void HandleGuildRankOpcode(WorldPacket& recvPacket);
        void HandleGuildAddRankOpcode(WorldPacket& recvPacket);
        void HandleGuildDelRankOpcode(WorldPacket& recvPacket);
        void HandleGuildChangeInfoTextOpcode(WorldPacket& recvPacket);
        void HandleSaveGuildEmblemOpcode(WorldPacket& recvPacket);

        void HandleTaxiNodeStatusQueryOpcode(WorldPacket& recvPacket);
        void HandleTaxiQueryAvailableNodes(WorldPacket& recvPacket);
        void HandleActivateTaxiOpcode(WorldPacket& recvPacket);
        void HandleActivateTaxiExpressOpcode(WorldPacket& recvPacket);
        void HandleMoveSplineDoneOpcode(WorldPacket& recvPacket);
        void SendActivateTaxiReply(ActivateTaxiReply reply);

        void HandleTabardVendorActivateOpcode(WorldPacket& recvPacket);
        void HandleBankerActivateOpcode(WorldPacket& recvPacket);
        void HandleBuyBankSlotOpcode(WorldPacket& recvPacket);
        void HandleTrainerListOpcode(WorldPacket& recvPacket);
        void HandleTrainerBuySpellOpcode(WorldPacket& recvPacket);
        void HandlePetitionShowListOpcode(WorldPacket& recvPacket);
        void HandleGossipHelloOpcode(WorldPacket& recvPacket);
        void HandleGossipSelectOptionOpcode(WorldPacket& recvPacket);
        void HandleSpiritHealerActivateOpcode(WorldPacket& recvPacket);
        void HandleNpcTextQueryOpcode(WorldPacket& recvPacket);
        void HandleBinderActivateOpcode(WorldPacket& recvPacket);
        void HandleListStabledPetsOpcode(WorldPacket& recvPacket);
        void HandleStablePet(WorldPacket& recvPacket);
        void HandleStablePetCallback(PreparedQueryResult result);
        void HandleUnstablePet(WorldPacket& recvPacket);
        //check if there is any pet in current slot
        void HandleUnstablePetCallback(uint32 petId, PreparedQueryResult result);
        //actually unstable pet
        void HandleUnstablePetCallback2(uint32 petId, uint32 petEntry, PreparedQueryResult result);
        void HandleBuyStableSlot(WorldPacket& recvPacket);
        void HandleStableRevivePet(WorldPacket& recvPacket);
        void HandleStableSwapPet(WorldPacket& recvPacket);
        void HandleStableSwapPetCallback(uint32 petId, PreparedQueryResult result);

        void HandleDuelAcceptedOpcode(WorldPacket& recvPacket);
        void HandleDuelCancelledOpcode(WorldPacket& recvPacket);

        void HandleAcceptTradeOpcode(WorldPacket& recvPacket);
        void HandleBeginTradeOpcode(WorldPacket& recvPacket);
        void HandleBusyTradeOpcode(WorldPacket& recvPacket);
        void HandleCancelTradeOpcode(WorldPacket& recvPacket);
        void HandleClearTradeItemOpcode(WorldPacket& recvPacket);
        void HandleIgnoreTradeOpcode(WorldPacket& recvPacket);
        void HandleInitiateTradeOpcode(WorldPacket& recvPacket);
        void HandleSetTradeGoldOpcode(WorldPacket& recvPacket);
        void HandleSetTradeItemOpcode(WorldPacket& recvPacket);
        void HandleUnacceptTradeOpcode(WorldPacket& recvPacket);

        void HandleAuctionHelloOpcode(WorldPacket& recvPacket);
        void HandleAuctionListItems( WorldPacket & recvData );
        void HandleAuctionListBidderItems( WorldPacket & recvData );
        void HandleAuctionSellItem( WorldPacket & recvData );
        void HandleAuctionRemoveItem( WorldPacket & recvData );
        void HandleAuctionListOwnerItems( WorldPacket & recvData );
        void HandleAuctionPlaceBid( WorldPacket & recvData );

        void HandleGetMailList(WorldPacket & recvData);
        void HandleSendMail(WorldPacket & recvData);
        void HandleMailTakeMoney(WorldPacket & recvData);
        void HandleMailTakeItem(WorldPacket & recvData);
        void HandleMailMarkAsRead(WorldPacket & recvData);
        void HandleMailReturnToSender(WorldPacket & recvData);
        void HandleMailDelete(WorldPacket & recvData);
        void HandleItemTextQuery(WorldPacket & recvData);
        void HandleMailCreateTextItem(WorldPacket & recvData);
        void HandleQueryNextMailTime(WorldPacket & recvData);
        void HandleCancelChanneling(WorldPacket & recvData);

        void HandleSplitItemOpcode(WorldPacket& recvPacket);
        void HandleSwapInvItemOpcode(WorldPacket& recvPacket);
        void HandleDestroyItemOpcode(WorldPacket& recvPacket);
        void _HandleAutoEquipItemOpcode(uint8 srcBag, uint8 srcSlot);
        void HandleAutoEquipItemOpcode(WorldPacket& recvPacket);
        void HandleItemQuerySingleOpcode(WorldPackets::Query::QueryItemSingle& query);
        void HandleSellItemOpcode(WorldPacket& recvPacket);
        void HandleBuyItemInSlotOpcode(WorldPacket& recvPacket);
        void HandleBuyItemOpcode(WorldPacket& recvPacket);
        void HandleListInventoryOpcode(WorldPacket& recvPacket);
        void HandleAutoStoreBagItemOpcode(WorldPacket& recvPacket);
        void HandleReadItem(WorldPacket& recvPacket);
        void HandleAutoEquipItemSlotOpcode(WorldPacket & recvPacket);
        void HandleSwapItem( WorldPacket & recvPacket);
        void HandleBuybackItem(WorldPacket & recvPacket);
        void HandleAutoBankItemOpcode(WorldPacket& recvPacket);
        void HandleAutoStoreBankItemOpcode(WorldPacket& recvPacket);
        void HandleWrapItemOpcode(WorldPacket& recvPacket);

        void HandleAttackSwingOpcode(WorldPacket& recvPacket);
        void HandleAttackStopOpcode(WorldPacket& recvPacket);
        void HandleSetSheathedOpcode(WorldPacket& recvPacket);

        bool _HandleUseItemOpcode(uint8 bagIndex, uint8 slot, uint8 spell_count, uint8 cast_count, ObjectGuid item_guid, SpellCastTargets targets);
        void HandleUseItemOpcode(WorldPacket& recvPacket);
        void HandleOpenItemOpcode(WorldPacket& recvPacket);
        void HandleOpenWrappedItemCallback(uint16 pos, ObjectGuid itemGuid, PreparedQueryResult result);
        void HandleCastSpellOpcode(WorldPacket& recvPacket);
        void HandleCancelCastOpcode(WorldPacket& recvPacket);
        void HandleCancelAuraOpcode(WorldPacket& recvPacket);
        void HandleCancelGrowthAuraOpcode(WorldPacket& recvPacket);
        void HandleCancelAutoRepeatSpellOpcode(WorldPacket& recvPacket);

        void HandleLearnTalentOpcode(WorldPacket& recvPacket);
        void HandleTalentWipeConfirmOpcode(WorldPacket& recvPacket);
        void HandleUnlearnSkillOpcode(WorldPacket& recvPacket);

        void HandleQuestgiverStatusQueryOpcode(WorldPacket& recvPacket);
        void HandleQuestgiverStatusMultipleQuery(WorldPacket& recvPacket);
        void HandleQuestgiverHelloOpcode(WorldPacket& recvPacket);
        void HandleQuestgiverAcceptQuestOpcode(WorldPacket& recvPacket);
        void HandleQuestgiverQueryQuestOpcode(WorldPacket& recvPacket);
        void HandleQuestgiverChooseRewardOpcode(WorldPacket& recvPacket);
        void HandleQuestgiverRequestRewardOpcode(WorldPacket& recvPacket);
        void HandleQuestQueryOpcode(WorldPackets::Quest::QueryQuestInfo& query);
        void HandleQuestgiverCancel(WorldPacket& recvData );
        void HandleQuestLogSwapQuest(WorldPacket& recvData );
        void HandleQuestLogRemoveQuest(WorldPacket& recvData);
        void HandleQuestConfirmAccept(WorldPacket& recvData);
        void HandleQuestgiverCompleteQuest(WorldPacket& recvData);
        void HandleQuestgiverQuestAutoLaunch(WorldPacket& recvPacket);
        void HandlePushQuestToParty(WorldPacket& recvPacket);
        void HandleQuestPushResult(WorldPacket& recvPacket);

        void HandleMessagechatOpcode(WorldPacket& recvPacket);
        void SendPlayerNotFoundNotice(std::string const& name);
        void SendWrongFactionNotice();
        void HandleTextEmoteOpcode(WorldPacket& recvPacket);
        void HandleChatIgnoredOpcode(WorldPacket& recvPacket);

        void HandleReclaimCorpseOpcode( WorldPacket& recvPacket );
        void HandleCorpseQueryOpcode( WorldPacket& recvPacket );
        void HandleResurrectResponseOpcode(WorldPacket& recvPacket);
        void HandleSummonResponseOpcode(WorldPacket& recvData);

        void HandleJoinChannel(WorldPacket& recvPacket);
        void HandleLeaveChannel(WorldPacket& recvPacket);
        void HandleChannelList(WorldPacket& recvPacket);
        void HandleChannelPassword(WorldPacket& recvPacket);
        void HandleChannelSetOwner(WorldPacket& recvPacket);
        void HandleChannelOwner(WorldPacket& recvPacket);
        void HandleChannelModerator(WorldPacket& recvPacket);
        void HandleChannelUnmoderator(WorldPacket& recvPacket);
        void HandleChannelMute(WorldPacket& recvPacket);
        void HandleChannelUnmute(WorldPacket& recvPacket);
        void HandleChannelInvite(WorldPacket& recvPacket);
        void HandleChannelKick(WorldPacket& recvPacket);
        void HandleChannelBan(WorldPacket& recvPacket);
        void HandleChannelUnban(WorldPacket& recvPacket);
        void HandleChannelAnnouncements(WorldPacket& recvPacket);
        void HandleChannelModerate(WorldPacket& recvPacket);
        void HandleChannelDisplayListQuery(WorldPacket& recvPacket);
        void HandleGetChannelMemberCount(WorldPacket& recvPacket);
        void HandleSetChannelWatch(WorldPacket& recvPacket);
        void HandleChannelDeclineInvite(WorldPacket& recvPacket);
        void HandleSpellClick(WorldPacket& recvPacket);

        void HandleCompleteCinematic(WorldPacket& recvPacket);
        void HandleNextCinematicCamera(WorldPacket& recvPacket);

        void HandlePageQuerySkippedOpcode(WorldPacket& recvPacket);
        void HandlePageTextQueryOpcode(WorldPacket& recvPacket);

        void HandleTutorialFlag ( WorldPacket & recvData );
        void HandleTutorialClear( WorldPacket & recvData );
        void HandleTutorialReset( WorldPacket & recvData );

        //Pet
		void HandlePetActionHelper(Unit* pet, ObjectGuid guid1, uint32 spellid, uint16 flag, ObjectGuid guid2);
        void HandlePetAction( WorldPacket & recvData );
        void HandlePetStopAttack(WorldPacket& recvData);
        void HandlePetNameQuery( WorldPacket & recvData );
        void HandlePetSetAction( WorldPacket & recvData );
        void HandlePetAbandon( WorldPacket & recvData );
        void HandlePetRename( WorldPacket & recvData );
        void HandlePetCancelAuraOpcode( WorldPacket& recvPacket );
        void HandlePetUnlearnOpcode( WorldPacket& recvPacket );
        void HandlePetSpellAutocastOpcode( WorldPacket& recvPacket );
        void HandlePetCastSpellOpcode( WorldPacket& recvPacket );

        void HandleSetActionBarToggles(WorldPacket& recvData);

        void HandleCharRenameOpcode(WorldPacket& recvData);
        void HandleCharRenameCallback(std::shared_ptr<CharacterRenameInfo> renameInfo, PreparedQueryResult result);
        void HandleSetPlayerDeclinedNames(WorldPacket& recvData);

        void HandleTotemDestroyed(WorldPacket& recvData);

        //Battleground
        void HandleBattleMasterHelloOpcode(WorldPacket &recvData);
        void _HandleBattlegroundJoin(BattlegroundTypeId bgTypeId, uint32 instanceId, bool joinAsGroup);
        void HandleBattlemasterJoinOpcode(WorldPacket &recvData);
        void HandleBattlegroundPlayerPositionsOpcode(WorldPacket& recvData);
        void HandlePVPLogDataOpcode( WorldPacket &recvData );
        void HandleBattleFieldPortOpcode( WorldPacket &recvData );
        void HandleBattlefieldListOpcode( WorldPacket &recvData );
        void HandleBattlefieldLeaveOpcode( WorldPacket &recvData );
        void HandleBattlemasterJoinArena( WorldPacket &recvData );
        void HandleReportPvPAFK( WorldPacket &recvData );

        #ifdef PLAYERBOT
        void HandleBotPackets();
        #endif

        void HandleWardenDataOpcode(WorldPacket& recvData);
        void HandleWorldTeleportOpcode(WorldPacket& recvData);
        void HandleMinimapPingOpcode(WorldPacket& recvData);
        void HandleRandomRollOpcode(WorldPacket& recvData);
        void HandleFarSightOpcode(WorldPacket& recvData);
        void HandleSetLfgOpcode(WorldPacket& recvData);
        void HandleSetDungeonDifficultyOpcode(WorldPacket& recvData);
        void HandleLfgAutoJoinOpcode(WorldPacket& recvData);
        void HandleLfgCancelAutoJoinOpcode(WorldPacket& recvData);
        void HandleLfmAutoAddMembersOpcode(WorldPacket& recvData);
        void HandleLfmCancelAutoAddmembersOpcode(WorldPacket& recvData);
        void HandleLfgClearOpcode(WorldPacket& recvData);
        void HandleLfmSetNoneOpcode(WorldPacket& recvData);
        void HandleLfmSetOpcode(WorldPacket& recvData);
        void HandleLfgSetCommentOpcode(WorldPacket& recvData);
        void HandleSetTitleOpcode(WorldPacket& recvData);
        void HandleRealmSplitOpcode(WorldPacket& recvData);
        void HandleTimeSyncResp(WorldPacket& recvData);
        void HandleWhoisOpcode(WorldPacket& recvData);
        void HandleResetInstancesOpcode(WorldPacket& recvData);

        // Arena Team
        void HandleInspectArenaTeamsOpcode(WorldPacket& recvData);
        void HandleArenaTeamQueryOpcode(WorldPacket& recvData);
        void HandleArenaTeamRosterOpcode(WorldPacket& recvData);
        void HandleArenaTeamInviteOpcode(WorldPacket& recvData);
        void HandleArenaTeamAcceptOpcode(WorldPacket& recvData);
        void HandleArenaTeamDeclineOpcode(WorldPacket& recvData);
        void HandleArenaTeamLeaveOpcode(WorldPacket& recvData);
        void HandleArenaTeamRemoveOpcode(WorldPacket& recvData);
        void HandleArenaTeamDisbandOpcode(WorldPacket& recvData);
        void HandleArenaTeamLeaderOpcode(WorldPacket& recvData);

        void HandleAreaSpiritHealerQueryOpcode(WorldPacket& recvData);
        void HandleAreaSpiritHealerQueueOpcode(WorldPacket& recvData);
        void HandleCancelMountAuraOpcode(WorldPacket& recvData);
        void HandleSelfResOpcode(WorldPacket& recvData);
        void HandleComplainOpcode(WorldPacket& recvData);
        void HandleRequestPetInfoOpcode(WorldPacket& recvData);

        // Socket gem
        void HandleSocketOpcode(WorldPacket& recvData);

        void HandleCancelTempEnchantmentOpcode(WorldPacket& recvData);

        void HandleChannelVoiceOnOpcode(WorldPacket & recvData);
        void HandleVoiceSessionEnableOpcode(WorldPacket& recvData);
        void HandleSetActiveVoiceChannel(WorldPacket& recvData);
        void HandleSetTaxiBenchmarkOpcode(WorldPacket& recvData);

        // Guild Bank
        void HandleGuildPermissions(WorldPacket& recvData);
        void HandleGuildBankMoneyWithdrawn(WorldPacket& recvData);
        void HandleGuildBankerActivate(WorldPacket& recvData);
        void HandleGuildBankQueryTab(WorldPacket& recvData);
        void HandleGuildBankLogQuery(WorldPacket& recvData);
        void HandleGuildBankDepositMoney(WorldPacket& recvData);
        void HandleGuildBankWithdrawMoney(WorldPacket& recvData);
        void HandleGuildBankSwapItems(WorldPacket& recvData);
        void HandleGuildBankUpdateTab(WorldPacket& recvData);
        void HandleGuildBankBuyTab(WorldPacket& recvData);
        void HandleQueryGuildBankTabText(WorldPacket& recvData);
        void HandleGuildBankSetTabText(WorldPacket& recvData);

        void HandleMirrorImageDataRequest(WorldPacket& recvData);

#ifdef LICH_KING
        void HandleReadyForAccountDataTimes(WorldPacket& recvData);
#endif

        //for HandleDebugValuesSnapshot command
        typedef std::vector<uint32> SnapshotType;
#ifdef TRINITY_DEBUG
        SnapshotType snapshot_values;
#endif

    public:
        QueryCallbackProcessor& GetQueryProcessor() { return _queryProcessor; }

    private:
        void ProcessQueryCallbacks();

        QueryResultHolderFuture _realmAccountLoginCallback;
        QueryResultHolderFuture _charLoginCallback;

        QueryCallbackProcessor _queryProcessor;

    friend class World;
    protected:
        class DosProtection
        {
            friend class World;
            public:
                DosProtection(WorldSession* s) : Session(s), _policy((Policy)sWorld->getIntConfig(CONFIG_PACKET_SPOOF_POLICY)) { }
                //returns true if all okay, false if player was kicked
                bool EvaluateOpcode(WorldPacket& p, time_t time) const;
            protected:
                enum Policy
                {
                    POLICY_LOG,
                    POLICY_KICK,
                    POLICY_BAN,
                };

                uint32 GetMaxPacketCounterAllowed(uint16 opcode) const;

                WorldSession* Session;

            private:
                Policy _policy;
                typedef std::unordered_map<uint16, PacketCounter> PacketThrottlingMap;
                // mark this member as "mutable" so it can be modified even in const functions
                mutable PacketThrottlingMap _PacketThrottlingMap;

                DosProtection(DosProtection const& right) = delete;
                DosProtection& operator=(DosProtection const& right) = delete;
        } AntiDOS;

    private:
        // private trade methods
        void moveItems(std::vector<Item*> myItems, std::vector<Item*> hisItems);

        bool CanUseBank(ObjectGuid bankerGUID = ObjectGuid::Empty) const;

        // logging helper
        void LogUnexpectedOpcode(WorldPacket* packet, const char* status, const char *reason);
        void LogUnprocessedTail(WorldPacket* packet);

        // EnumData helpers
        bool IsLegitCharacterForAccount(ObjectGuid::LowType lowGUID)
        {
            return _legitCharacters.find(lowGUID) != _legitCharacters.end();
        }

        // this stores the GUIDs of the characters who can login
        // characters who failed on Player::BuildEnumData shouldn't login
        std::set<uint32> _legitCharacters;

        Player* _player;
        std::shared_ptr<WorldSocket> m_Socket;
        std::string m_Address;

        AccountTypes _security;
        uint32 _accountId;
        std::string _accountName;
        uint8 m_expansion;
        ClientBuild m_clientBuild;

       // uint32 _groupid;
        
        typedef std::list<AddonInfo> AddonsList;
        void ReadAddon(ByteBuffer& data);

        // Warden
        WardenBase* _warden;

        time_t _logoutTime;
        bool m_inQueue;                                     // session wait in auth.queue
        bool m_playerLoading;                               // code processed in LoginPlayer
        bool m_playerLogout;                                // code processed in LogoutPlayer
        bool m_playerRecentlyLogout;
        bool m_playerSave;
        LocaleConstant m_sessionDbcLocale;
        LocaleConstant m_sessionDbLocaleIndex;
        uint32 m_latency;
        
        AccountData m_accountData[NUM_ACCOUNT_DATA_TYPES];
        uint32 m_Tutorials[MAX_ACCOUNT_TUTORIAL_VALUES];
        bool   m_TutorialsChanged;
        AddonsList m_addonsList;
        uint32 expireTime;
        bool forceExit;
        ObjectGuid m_currentBankerGUID;

        LockedQueue<WorldPacket*> _recvQueue;

        std::shared_ptr<ReplayRecorder> m_replayRecorder;
        std::shared_ptr<ReplayPlayer> m_replayPlayer;

        ClientControl _clientControl;
};
#endif
