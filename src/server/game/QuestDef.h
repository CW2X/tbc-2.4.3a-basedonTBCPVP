#ifndef TRINITYCORE_QUEST_H
#define TRINITYCORE_QUEST_H

#include "Define.h"
#include "Database/DatabaseEnv.h"
#include "SharedDefines.h"
#include "WorldPacket.h"
#include "DBCEnums.h"

#include <string>
#include <vector>

class Player;

class ObjectMgr;

#define MAX_QUEST_LOG_SIZE 25

#define QUEST_OBJECTIVES_COUNT 4
#ifdef LICH_KING
#define QUEST_ITEM_OBJECTIVES_COUNT 6
#else
#define QUEST_ITEM_OBJECTIVES_COUNT QUEST_OBJECTIVES_COUNT
#endif
#define QUEST_SOURCE_ITEM_IDS_COUNT 4
#define QUEST_REWARD_CHOICES_COUNT 6
#define QUEST_REWARDS_COUNT 4
#define QUEST_DEPLINK_COUNT 10
#define QUEST_REPUTATIONS_COUNT 5
#define QUEST_EMOTE_COUNT 4
#define QUEST_PVP_KILL_SLOT 0

enum QuestFailedReasons
{
    INVALIDREASON_DONT_HAVE_REQ                 = 0,
    INVALIDREASON_QUEST_FAILED_LOW_LEVEL        = 1,        //You are not high enough level for that quest.
    INVALIDREASON_QUEST_FAILED_WRONG_RACE       = 6,        //That quest is not available to your race.
    INVALIDREASON_QUEST_ALREADY_DONE            = 7,        //You have completed that quest.
    INVALIDREASON_QUEST_ONLY_ONE_TIMED          = 12,       //You can only be on one timed quest at a time.
    INVALIDREASON_QUEST_ALREADY_ON              = 13,       //You are already on that quest
    INVALIDREASON_QUEST_FAILED_EXPANSION        = 16,       //This quest requires an expansion enabled account.
    INVALIDREASON_QUEST_ALREADY_ON2             = 18,       //You are already on that quest
    INVALIDREASON_QUEST_FAILED_MISSING_ITEMS    = 21,       //You don't have the required items with you. Check storage.
    INVALIDREASON_QUEST_FAILED_NOT_ENOUGH_MONEY = 23,       //You don't have enough money for that quest.
    INVALIDREASON_DAILY_QUESTS_REMAINING        = 26,       //You have already completed 10 daily quests today
    INVALIDREASON_QUEST_FAILED_CAIS             = 27,       //You cannot complete quests once you have reached tired time
};

enum QuestShareMessages
{
    QUEST_PARTY_MSG_SHARING_QUEST   = 0,
    QUEST_PARTY_MSG_CANT_TAKE_QUEST = 1,
    QUEST_PARTY_MSG_ACCEPT_QUEST    = 2,
    QUEST_PARTY_MSG_REFUSE_QUEST    = 3,
    QUEST_PARTY_MSG_TOO_FAR         = 4,
    QUEST_PARTY_MSG_BUSY            = 5,
    QUEST_PARTY_MSG_LOG_FULL        = 6,
    QUEST_PARTY_MSG_HAVE_QUEST      = 7,
    QUEST_PARTY_MSG_FINISH_QUEST    = 8,
#ifdef LICH_KING
    QUEST_PARTY_MSG_SHARING_TIMER_EXPIRED   = 9,
    QUEST_PARTY_MSG_NOT_IN_PARTY            = 10,
    QUEST_PARTY_MSG_NOT_ELIGIBLE_TODAY      = 11
#endif
};

enum __QuestTradeSkill
{
    QUEST_TRSKILL_NONE           = 0,
    QUEST_TRSKILL_ALCHEMY        = 1,
    QUEST_TRSKILL_BLACKSMITHING  = 2,
    QUEST_TRSKILL_COOKING        = 3,
    QUEST_TRSKILL_ENCHANTING     = 4,
    QUEST_TRSKILL_ENGINEERING    = 5,
    QUEST_TRSKILL_FIRSTAID       = 6,
    QUEST_TRSKILL_HERBALISM      = 7,
    QUEST_TRSKILL_LEATHERWORKING = 8,
    QUEST_TRSKILL_POISONS        = 9,
    QUEST_TRSKILL_TAILORING      = 10,
    QUEST_TRSKILL_MINING         = 11,
    QUEST_TRSKILL_FISHING        = 12,
    QUEST_TRSKILL_SKINNING       = 13,
    QUEST_TRSKILL_JEWELCRAFTING  = 14,
};

enum QuestStatus
{
    QUEST_STATUS_NONE           = 0,
    QUEST_STATUS_COMPLETE       = 1,
 // QUEST_STATUS_UNAVAILABLE    = 2,
    QUEST_STATUS_INCOMPLETE     = 3,
 // QUEST_STATUS_AVAILABLE      = 4,
    QUEST_STATUS_FAILED         = 5,
    QUEST_STATUS_REWARDED       = 6, // Not used in DB
    MAX_QUEST_STATUS
};

enum QuestGiverStatus
{
    DIALOG_STATUS_NONE                     = 0,
    DIALOG_STATUS_UNAVAILABLE              = 1,
#ifdef LICH_KING
    DIALOG_STATUS_LOW_LEVEL_AVAILABLE      = 2,
    DIALOG_STATUS_LOW_LEVEL_REWARD_REP     = 3,
    DIALOG_STATUS_LOW_LEVEL_AVAILABLE_REP  = 4,
    DIALOG_STATUS_INCOMPLETE               = 5,
    DIALOG_STATUS_REWARD_REP               = 6,
    DIALOG_STATUS_AVAILABLE_REP            = 7,
    DIALOG_STATUS_AVAILABLE                = 8,
    DIALOG_STATUS_REWARD2                  = 9,   // no yellow dot on minimap
    DIALOG_STATUS_REWARD                   = 10,  // yellow dot on minimap
#else
    DIALOG_STATUS_CHAT                     = 2,
    DIALOG_STATUS_INCOMPLETE               = 3,
    DIALOG_STATUS_REWARD_REP               = 4,
    DIALOG_STATUS_AVAILABLE_REP            = 5,
    DIALOG_STATUS_AVAILABLE                = 6,
    DIALOG_STATUS_REWARD2                  = 7,   // not yellow dot on minimap
    DIALOG_STATUS_REWARD                   = 8,   // yellow dot on minimap
#endif
};

enum __QuestFlags
{
    // Flags used at server and sent to client
    QUEST_FLAGS_STAY_ALIVE              = 0x00000001,   // Not used currently
    QUEST_FLAGS_PARTY_ACCEPT            = 0x00000002,   // If player in party, all players that can accept this quest will receive confirmation box to accept quest CMSG_QUEST_CONFIRM_ACCEPT/SMSG_QUEST_CONFIRM_ACCEPT
    QUEST_FLAGS_EXPLORATION             = 0x00000004,   // Not used currently
    QUEST_FLAGS_SHARABLE                = 0x00000008,   // Can be shared: Player::CanShareQuest()
    //QUEST_FLAGS_NONE2                 = 0x00000010,   // Not used currently
    QUEST_FLAGS_EPIC                    = 0x00000020,   // Not used currently: Unsure of content
    QUEST_FLAGS_RAID                    = 0x00000040,   // Not used currently
    QUEST_FLAGS_TBC                     = 0x00000080,   // Not used currently: Available if TBC expension enabled only
    QUEST_FLAGS_UNK2                    = 0x00000100,   // Not used currently: _DELIVER_MORE Quest needs more than normal _q-item_ drops from mobs
    QUEST_FLAGS_HIDDEN_REWARDS          = 0x00000200,   // Items and money rewarded only sent in SMSG_QUESTGIVER_OFFER_REWARD (not in SMSG_QUESTGIVER_QUEST_DETAILS or in client quest log(SMSG_QUEST_QUERY_RESPONSE))
    QUEST_FLAGS_TRACKING                = 0x00000400,   // These quests are automatically rewarded on quest complete and they will never appear in quest log client side.
    QUEST_FLAGS_TBC_RACES               = 0x00000800,   // Not used currently: Blood elf/Draenei starting zone quests
    QUEST_FLAGS_DAILY                   = 0x00001000,   // Used to know quest is Daily one
#ifdef LICH_KING
    QUEST_FLAGS_FLAGS_PVP               = 0x00002000,   // Having this quest in log forces PvP flag
    QUEST_FLAGS_UNAVAILABLE             = 0x00004000,   // Used on quests that are not generically available
    QUEST_FLAGS_WEEKLY                  = 0x00008000,
    QUEST_FLAGS_AUTOCOMPLETE            = 0x00010000,   // auto complete
    QUEST_FLAGS_DISPLAY_ITEM_IN_TRACKER = 0x00020000,   // Displays usable item in quest tracker
    QUEST_FLAGS_OBJ_TEXT                = 0x00040000,   // use Objective text as Complete text
    QUEST_FLAGS_AUTO_ACCEPT             = 0x00080000,   // The client recognizes this flag as auto-accept. However, NONE of the current quests (3.3.5a) have this flag. Maybe blizz used to use it, or will use it in the future.
#endif
};

enum QuestSpecialFlags
{
    QUEST_SPECIAL_FLAGS_NONE                 = 0x000,
    // Trinity flags for set SpecialFlags in DB if required but used only at server
    QUEST_SPECIAL_FLAGS_REPEATABLE           = 0x001,   // Set by 1 in SpecialFlags from DB
    QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT = 0x002,   // Set by 2 in SpecialFlags from DB (if required area explore, spell SPELL_EFFECT_QUEST_COMPLETE casting, table `FECT_QUEST_COMPLETE casting, table `*_script` command SCRIPT_COMMAND_QUEST_EXPLORED use, set from script)
#ifdef LICH_KING
    QUEST_SPECIAL_FLAGS_AUTO_ACCEPT          = 0x004,   // Set by 4 in SpecialFlags in DB if the quest is to be auto-accepted.
    QUEST_SPECIAL_FLAGS_DF_QUEST             = 0x008,   // Set by 8 in SpecialFlags in DB if the quest is used by Dungeon Finder. (LK only)
    QUEST_SPECIAL_FLAGS_MONTHLY              = 0x010,   // Set by 16 in SpecialFlags in DB if the quest is reset at the begining of the month
#endif
    QUEST_SPECIAL_FLAGS_CAST                 = 0x020,   // Set by 32 in SpecialFlags in DB if the quest requires RequiredOrNpcGo killcredit but NOT kill (a spell cast)
    // room for more custom flags

    QUEST_SPECIAL_FLAGS_DB_ALLOWED = QUEST_SPECIAL_FLAGS_REPEATABLE | QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT 
#ifdef LICH_KING
    | QUEST_SPECIAL_FLAGS_AUTO_ACCEPT | QUEST_SPECIAL_FLAGS_DF_QUEST | QUEST_SPECIAL_FLAGS_MONTHLY
#endif
    | QUEST_SPECIAL_FLAGS_CAST,

    QUEST_SPECIAL_FLAGS_DELIVER              = 0x080,   // Internal flag computed only
    QUEST_SPECIAL_FLAGS_SPEAKTO              = 0x100,   // Internal flag computed only
    QUEST_SPECIAL_FLAGS_KILL                 = 0x200,   // Internal flag computed only
    QUEST_SPECIAL_FLAGS_TIMED                = 0x400,   // Internal flag computed only
#ifdef LICH_KING
    QUEST_SPECIAL_FLAGS_PLAYER_KILL          = 0x800,   // Internal flag computed only
#endif
    QUEST_SPECIAL_FLAGS_COMPLETED_AT_START   = 0x1000   // Internal flag computed only
};

struct QuestLocale
{
    QuestLocale() { ObjectiveText.resize(QUEST_OBJECTIVES_COUNT); }

    std::vector<std::string> Title;
    std::vector<std::string> Details;
    std::vector<std::string> Objectives;
    std::vector<std::string> _offerRewardText;
    std::vector<std::string> _requestItemsText;
    std::vector<std::string> AreaDescription;
    std::vector< std::vector<std::string> > ObjectiveText;
};

// This Quest class provides a convenient way to access a few pretotaled (cached) quest details,
// all base quest information, and any utility functions such as generating the amount of
// xp to give
class TC_GAME_API Quest
{
    friend class ObjectMgr;
    public:
        Quest(Field * questRecord);
        void LoadQuestDetails(Field* fields);
        void LoadQuestRequestItems(Field* fields);
        void LoadQuestOfferReward(Field* fields);
        void LoadQuestTemplateAddon(Field* fields);
        void LoadQuestMailSender(Field* fields);

        uint32 XPValue( Player *pPlayer ) const;

        bool HasFlag( uint32 flag ) const { return ( _flags & flag ) != 0; }
        void SetFlag( uint32 flag ) { _flags |= flag; }

        bool HasSpecialFlag(uint32 flag) const { return (_specialFlags & flag) != 0; }
        void SetSpecialFlag(uint32 flag) { _specialFlags |= flag; }

        // table data accessors:
        uint32 GetQuestId() const { return _id; }
        uint32 GetQuestMethod() const { return _method; }
        int32  GetZoneOrSort() const { return _zoneOrSort; }
        uint32 GetMinLevel() const { return MinLevel; }
        uint32 GetMaxLevel() const { return _maxLevel; }
        uint32 GetQuestLevel() const { return _level; }
        QuestTypes GetType() const { return Type; }
        uint32 GetRequiredClasses() const { return _requiredClasses; }
        uint32 GetAllowableRaces() const { return _allowableRaces; }
        uint32 GetRequiredSkill() const { return _requiredSkillId; }
        uint32 GetRequiredSkillValue() const { return _requiredSkillPoints; }
        uint32 GetRepObjectiveFaction() const { return _requiredFactionId1; }
        int32  GetRepObjectiveValue() const { return _requiredFactionValue1; }
        uint32 GetRequiredMinRepFaction() const { return _requiredMinRepFaction; }
        int32  GetRequiredMinRepValue() const { return _requiredMinRepValue; }
        uint32 GetRequiredMaxRepFaction() const { return _requiredMaxRepFaction; }
        int32  GetRequiredMaxRepValue() const { return _requiredMaxRepValue; }
        uint32 GetSuggestedPlayers() const { return _suggestedPlayers; }
        uint32 GetTimeAllowed() const { return _timeAllowed; }
        int32  GetPrevQuestId() const { return _prevQuestId; }
        int32  GetNextQuestId() const { return _nextQuestId; }
        int32  GetExclusiveGroup() const { return _exclusiveGroup; }
        uint32 GetNextQuestInChain() const { return _rewardNextQuest; }
        uint32 GetCharTitleId() const { return _rewardTitleId; }
        uint32 GetSrcItemId() const { return _startItem; }
        uint32 GetSrcItemCount() const { return _startItemCount; }
        uint32 GetSrcSpell() const { return _sourceSpellid; }
        std::string GetTitle() const { return Title; }
        std::string GetDetails() const { return Details; }
        std::string GetObjectives() const { return Objectives; }
        std::string GetOfferRewardText() const { return _offerRewardText; }
        std::string GetRequestItemsText() const { return _requestItemsText; }
        inline std::string GetAreaDescription() const { return AreaDescription; } //compat
        int32  GetRewOrReqMoney() const;
        uint32 GetRewHonorableKills() const { return RewardHonorableKills; }
        uint32 GetRewMoneyMaxLevel() const { return RewardMoneyMaxLevel; }
                                                            // use in XP calculation at client
        uint32 GetRewSpell() const { return _rewardDisplaySpell; }
        uint32 GetRewSpellCast() const { return _rewardSpell; }
        uint32 GetRewMailTemplateId() const { return _rewardMailTemplateId; }
        uint32 GetRewMailDelaySecs() const { return _rewardMailDelay; }
        uint32 GetRewMailSenderEntry() const { return _rewardMailSenderEntry; }
        uint32 GetPOIContinent() const { return PointMapId; }
        float  GetPOIx() const { return PointX; }
        float  GetPOIy() const { return PointY; }
        uint32 GetPointOpt() const { return PointOpt; }
        uint32 GetIncompleteEmote() const { return _emoteOnIncomplete; }
        uint32 GetCompleteEmote() const { return _emoteOnComplete; }
        uint32 GetQuestStartScript() const { return QuestStartScript; }
        uint32 GetQuestCompleteScript() const { return QuestCompleteScript; }
        bool   IsRepeatable() const { return _specialFlags & QUEST_SPECIAL_FLAGS_REPEATABLE; }
        bool   IsAutoComplete() const;
        bool   IsAutoAccept() const { return false; } //tc compat
        uint32 GetFlags() const { return _flags; }
        bool   IsDaily() const { return _flags & QUEST_FLAGS_DAILY; }
#ifdef LICH_KING
        bool   IsWeekly() const { return (_flags & QUEST_FLAGS_WEEKLY) != 0; }
        bool   IsMonthly() const { return (_specialFlags & QUEST_SPECIAL_FLAGS_MONTHLY) != 0; }
        bool   IsDailyOrWeekly() const { return (_flags & (QUEST_FLAGS_DAILY | QUEST_FLAGS_WEEKLY)) != 0; }
        bool   IsSeasonal() const { return (_zoneOrSort == -QUEST_SORT_SEASONAL || _zoneOrSort == -QUEST_SORT_SPECIAL || _zoneOrSort == -QUEST_SORT_LUNAR_FESTIVAL || _zoneOrSort == -QUEST_SORT_MIDSUMMER || _zoneOrSort == -QUEST_SORT_BREWFEST || _zoneOrSort == -QUEST_SORT_LOVE_IS_IN_THE_AIR || _zoneOrSort == -QUEST_SORT_NOBLEGARDEN) && !IsRepeatable(); }
#else
        bool   IsWeekly() const { return false; }
        bool   IsMonthly() const { return false; }
        bool   IsDailyOrWeekly() const { return IsDaily(); }
#endif
        bool   IsSeasonal() const 
        { 
            return (_zoneOrSort == -QUEST_SORT_SEASONAL || _zoneOrSort == -QUEST_SORT_SPECIAL || _zoneOrSort == -QUEST_SORT_LUNAR_FESTIVAL 
                || _zoneOrSort == -QUEST_SORT_MIDSUMMER || _zoneOrSort == -QUEST_SORT_BREWFEST 
#ifdef LICH_KING
            || _zoneOrSort == -QUEST_SORT_LOVE_IS_IN_THE_AIR || _zoneOrSort == -QUEST_SORT_NOBLEGARDEN
#endif
                   ) && !IsRepeatable(); 
        }
        bool   IsRaidQuest(Difficulty difficulty) const;
        bool   IsAllowedInRaid(Difficulty difficulty) const;
#ifdef LICH_KING
        bool   IsDFQuest() const { return (_specialFlags & QUEST_SPECIAL_FLAGS_DF_QUEST) != 0; }
#else
        bool   IsDFQuest() const { return false; }
#endif
        bool   CanIncreaseRewardedQuestCounters() const;
        uint32 CalculateHonorGain(uint8 level) const;

        // multiple values
        std::string ObjectiveText[QUEST_OBJECTIVES_COUNT];
        uint32 RequiredItemId[QUEST_ITEM_OBJECTIVES_COUNT];
        uint32 RequiredItemCount[QUEST_ITEM_OBJECTIVES_COUNT]; 
        uint32 RequiredSourceItemId[QUEST_SOURCE_ITEM_IDS_COUNT];
        uint32 RequiredSourceItemCount[QUEST_SOURCE_ITEM_IDS_COUNT];
        uint32 ReqSourceRef[QUEST_SOURCE_ITEM_IDS_COUNT];
        int32  RequiredNpcOrGo[QUEST_OBJECTIVES_COUNT];   // >0 Creature <0 Gameobject
        uint32 RequiredNpcOrGoCount[QUEST_OBJECTIVES_COUNT];
        uint32 ReqSpell[QUEST_OBJECTIVES_COUNT];
        uint32 RewardChoiceItemId[QUEST_REWARD_CHOICES_COUNT];
        uint32 RewardChoiceItemCount[QUEST_REWARD_CHOICES_COUNT];
        uint32 RewardItemId[QUEST_REWARDS_COUNT];
        uint32 RewardItemIdCount[QUEST_REWARDS_COUNT];
        uint32 RewardFactionId[QUEST_REPUTATIONS_COUNT];
        int32  RewardRepValue[QUEST_REPUTATIONS_COUNT];
        uint32 DetailsEmote[QUEST_EMOTE_COUNT];
        uint32 DetailsEmoteDelay[QUEST_EMOTE_COUNT];
        uint32 OfferRewardEmote[QUEST_EMOTE_COUNT];
        uint32 OfferRewardEmoteDelay[QUEST_EMOTE_COUNT];

        uint32 GetReqItemsCount() const { return _reqItemsCount; }
        uint32 GetReqCreatureOrGOcount() const { return _reqCreatureOrGOcount; }
        uint32 GetRewardChoiceItemsCount() const { return _rewChoiceItemsCount; }
        uint32 GetRewardItemsCount() const { return _rewItemsCount; }

        void SetEventIdForQuest(uint16 eventId) { _eventIdForQuest = eventId; }
        uint16 GetEventIdForQuest() const { return _eventIdForQuest; }

        std::vector<uint32> DependentPreviousQuests;

        void InitializeQueryData();
        WorldPacket BuildQueryData(LocaleConstant loc) const;

        WorldPacket QueryData[TOTAL_LOCALES];
        // cached data
    private:
        uint32 _reqItemsCount;
        uint32 _reqCreatureOrGOcount;
        uint32 _rewChoiceItemsCount;
        uint32 _rewItemsCount;
        uint16 _eventIdForQuest = 0;

        // table data
    protected:
        uint32 _id;
        uint32 _method;
        int32  _zoneOrSort;
        uint32 MinLevel;
        uint32 _level;
        QuestTypes Type; //enum QuestTypes in SharedDefines
        uint32 _allowableRaces;
        uint32 _requiredFactionId1;
        int32  _requiredFactionValue1;
        uint32 _suggestedPlayers;
        uint32 _timeAllowed;
        uint32 _flags; //enum __QuestFlags in QuestDef.h
        uint32 _rewardTitleId;
        uint32 _rewardNextQuest;
        uint32 _startItem;
        std::string Title;
        std::string Details;
        std::string Objectives;
        std::string _offerRewardText;
        std::string _requestItemsText;
        std::string AreaDescription;
        uint32 RewardHonorableKills;
        int32  RewardOrReqMoney;
        uint32 RewardMoneyMaxLevel;
        uint32 _rewardDisplaySpell;
        uint32 _rewardSpell;
        uint32 PointMapId;
        float  PointX;
        float  PointY;
        uint32 PointOpt;
        uint32 _emoteOnIncomplete;
        uint32 _emoteOnComplete;
        uint32 QuestStartScript;
        uint32 QuestCompleteScript;

        // quest_template_addon table (custom data)
        uint32 _maxLevel = 0;
        uint32 _requiredClasses = 0;
        uint32 _sourceSpellid = 0;
        int32  _prevQuestId = 0;
        uint32 _nextQuestId = 0;
        int32  _exclusiveGroup = 0;
        uint32 _rewardMailTemplateId = 0;
        uint32 _rewardMailDelay = 0;
        uint32 _requiredSkillId = 0;
        uint32 _requiredSkillPoints = 0;
        uint32 _requiredMinRepFaction = 0;
        int32  _requiredMinRepValue = 0;
        uint32 _requiredMaxRepFaction = 0;
        int32  _requiredMaxRepValue = 0;
        uint32 _startItemCount = 0;
        uint32 _rewardMailSenderEntry = 0;
        uint32 _specialFlags = 0; // custom flags, not sniffed/WDB
};

struct QuestStatusData
{
    QuestStatus Status = QUEST_STATUS_NONE;
    uint32 Timer = 0;
    uint32 ItemCount[QUEST_ITEM_OBJECTIVES_COUNT] = {};
    uint32 CreatureOrGOCount[QUEST_OBJECTIVES_COUNT] = {};
    bool Explored = false;
    uint16 PlayerCount = 0; //TLK only
};

struct AccessRequirement
{
    uint8  levelMin;
    uint8  levelMax;
    uint32 item;
    uint32 item2;
    uint32 heroicKey;
    uint32 heroicKey2;
    uint32 quest;
    uint32 questFailedText; //entry in trinity_string
    uint32 heroicQuest;
    uint32 heroicQuestFailedText; //entry in trinity_string
};

#endif

