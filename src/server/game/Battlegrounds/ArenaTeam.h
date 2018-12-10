
#ifndef TRINITYCORE_ARENATEAM_H
#define TRINITYCORE_ARENATEAM_H

enum ArenaTeamCommandTypes
{
    ERR_ARENA_TEAM_CREATE_S                 = 0x00,
    ERR_ARENA_TEAM_INVITE_SS                = 0x01,
    //ERR_ARENA_TEAM_QUIT_S                   = 0x02,
    ERR_ARENA_TEAM_QUIT_S                   = 0x03,
    ERR_ARENA_TEAM_FOUNDER_S                = 0x0C          // need check, probably wrong...
};

enum ArenaTeamCommandErrors
{
    ERR_ARENA_TEAM_INTERNAL                 = 0x01,
    ERR_ALREADY_IN_ARENA_TEAM               = 0x02,
    ERR_ALREADY_IN_ARENA_TEAM_S             = 0x03,
    ERR_INVITED_TO_ARENA_TEAM               = 0x04,
    ERR_ALREADY_INVITED_TO_ARENA_TEAM_S     = 0x05,
    ERR_ARENA_TEAM_NAME_INVALID             = 0x06,
    ERR_ARENA_TEAM_NAME_EXISTS_S            = 0x07,
    ERR_ARENA_TEAM_LEADER_LEAVE_S           = 0x08,
    ERR_ARENA_TEAM_PERMISSIONS              = 0x08,
    ERR_ARENA_TEAM_PLAYER_NOT_IN_TEAM       = 0x09,
    ERR_ARENA_TEAM_PLAYER_NOT_IN_TEAM_SS    = 0x0A,
    ERR_ARENA_TEAM_PLAYER_NOT_FOUND_S       = 0x0B,
    ERR_ARENA_TEAM_NOT_ALLIED               = 0x0C,
    ERR_ARENA_TEAM_IGNORING_YOU_S           = 0x13,
    ERR_ARENA_TEAM_TARGET_TOO_LOW_S         = 0x15,
    ERR_ARENA_TEAM_FULL                     = 0x16
};

enum ArenaTeamEvents
{
    ERR_ARENA_TEAM_JOIN_SS                  = 3,            // player name + arena team name
    ERR_ARENA_TEAM_LEAVE_SS                 = 4,            // player name + arena team name
    ERR_ARENA_TEAM_REMOVE_SSS               = 5,            // player name + arena team name + captain name
    ERR_ARENA_TEAM_LEADER_IS_SS             = 6,            // player name + arena team name
    ERR_ARENA_TEAM_LEADER_CHANGED_SSS       = 7,            // old captain + new captain + arena team name
    ERR_ARENA_TEAM_DISBANDED_S              = 8             // captain name + arena team name
};

enum ArenaTeamEventsLog {
    AT_EV_CREATE        = 1,
    AT_EV_DISBAND       = 2,
    AT_EV_JOIN          = 3,
    AT_EV_LEAVE         = 4,
    AT_EV_PROMOTE       = 5
};

/*
need info how to send these ones:
ERR_ARENA_TEAM_YOU_JOIN_S - client show it automatically when accept invite
ERR_ARENA_TEAM_TARGET_TOO_LOW_S
ERR_ARENA_TEAM_TOO_MANY_MEMBERS_S
ERR_ARENA_TEAM_LEVEL_TOO_LOW_I
*/

enum ArenaTeamStatTypes
{
    STAT_TYPE_RATING         = 0,
    STAT_TYPE_GAMES_WEEK     = 1,
    STAT_TYPE_WINS_WEEK      = 2,
    STAT_TYPE_GAMES_SEASON   = 3,
    STAT_TYPE_WINS_SEASON    = 4,
    STAT_TYPE_RANK           = 5,
    STAT_TYPE_NONPLAYEDWEEKS = 6
};

enum ArenaTeamTypes
{
    ARENA_TEAM_2v2      = 2,
    ARENA_TEAM_3v3      = 3,
    ARENA_TEAM_5v5      = 5
};

struct TC_GAME_API ArenaTeamMember
{
    ObjectGuid Guid;
    std::string Name;
    uint8 Class;
    uint32 WeekGames;
    uint32 WeekWins;
    uint32 SeasonGames;
    uint32 SeasonWins;
    uint32 PersonalRating;

    void ModifyPersonalRating(Player* plr, int32 mod, uint32 slot);
#ifdef LICH_KING
    void ModifyMatchmakerRating(int32 mod, uint32 slot);
#endif
};

struct ArenaTeamStats
{
    uint16 Rating;
    uint16 WeekGames;
    uint16 WeekWins;
    uint16 SeasonGames;
    uint16 SeasonWins;
    uint32 Rank;
    uint32 NonPlayedWeeks;
};

#define MAX_ARENA_SLOT 3                                    // 0..2 slots

class TC_GAME_API ArenaTeam
{
    public:
        ArenaTeam();
        ~ArenaTeam();

        bool Create(ObjectGuid captainGuid, uint8 type, std::string const teamName, uint32 backgroundColor, uint8 emblemStyle, uint32 emblemColor, uint8 borderStyle, uint32 borderColor);
        void Disband(WorldSession *session);

        typedef std::list<ArenaTeamMember> MemberList;

        uint32 GetId() const              { return TeamId; }
        uint32 GetType() const            { return Type; }
        uint8  GetSlot() const            { return GetSlotByType(GetType()); }
        static uint8 GetSlotByType(uint32 type);
        const ObjectGuid& GetCaptain() const  { return CaptainGuid; }
        std::string const& GetName() const       { return TeamName; }
        bool SetName(std::string const& name);
        const ArenaTeamStats& GetStats() const { return Stats; }
        uint32 GetRating() const          { return Stats.Rating; }
        //uint32 GetAverageMMR(Group* group) const;
        uint32 GetRank() const            { return Stats.Rank;   }

        uint32 GetEmblemStyle() const     { return EmblemStyle; }
        uint32 GetEmblemColor() const     { return EmblemColor; }
        uint32 GetBorderStyle() const     { return BorderStyle; }
        uint32 GetBorderColor() const     { return BorderColor; }
        uint32 GetBackgroundColor() const { return BackgroundColor; }

        void SetCaptain(ObjectGuid guid);
        bool AddMember(ObjectGuid PlayerGuid, SQLTransaction trans);

        // Shouldn't be const uint64& ed, because than can reference guid from members on Disband
        // and this method removes given record from list. So invalid reference can happen.
        void DeleteMember(ObjectGuid guid, bool cleanDb = true);

        void HandleDecay();

        size_t GetMembersSize() const       { return Members.size(); }
        bool   Empty() const                { return Members.empty(); }
        MemberList::iterator membersBegin() { return Members.begin(); }
        MemberList::iterator membersEnd()   { return Members.end(); }
        bool HaveMember(ObjectGuid guid) const;

        ArenaTeamMember* GetMember(ObjectGuid guid);
        ArenaTeamMember* GetMember(const std::string& name);

        void GetMembers(std::list<ArenaTeamMember*>& memberList);

        bool IsFighting() const;

        bool LoadArenaTeamFromDB(QueryResult const arenaTeamDataResult);
        bool LoadMembersFromDB(QueryResult const arenaTeamMembersResult);
        /*bool LoadArenaTeamFromDB(uint32 ArenaTeamId);
        bool LoadArenaTeamFromDB(const std::string teamname);
        bool LoadMembersFromDB(uint32 ArenaTeamId);*/

        void SaveToDB();

        void BroadcastPacket(WorldPacket *packet);

        void Roster(WorldSession *session);
        void Query(WorldSession *session);
        void SendStats(WorldSession *session);
        void Inspect(WorldSession *session, ObjectGuid guid);

        uint32 GetPoints(uint32 MemberRating);
        int32 GetRatingMod(uint32 ownRating, uint32 opponentRating, bool won);
        float GetChanceAgainst(uint32 own_rating, uint32 enemy_rating);
        int32 WonAgainst(uint32 againstRating);
        void MemberWon(Player * plr, uint32 againstRating);
        int32 LostAgainst(uint32 againstRating);
        void MemberLost(Player * plr, uint32 againstRating);
        void OfflineMemberLost(ObjectGuid guid, uint32 againstMatchmakerRating, int32 MatchmakerRatingChange = -12);

        void UpdateArenaPointsHelper(std::map<ObjectGuid::LowType, uint32> & PlayerPoints);
        void UpdateRank();

        void NotifyStatsChanged();

        //return true if any match played this week
        bool FinishWeek();

    protected:

        uint32      TeamId;
        uint8       Type;
        std::string TeamName;
        ObjectGuid  CaptainGuid;

        uint32 BackgroundColor; // ARGB format
        uint32 EmblemStyle;     // icon id
        uint32 EmblemColor;     // ARGB format
        uint32 BorderStyle;     // border image id
        uint32 BorderColor;     // ARGB format

        MemberList Members;
        ArenaTeamStats Stats;
};
#endif

