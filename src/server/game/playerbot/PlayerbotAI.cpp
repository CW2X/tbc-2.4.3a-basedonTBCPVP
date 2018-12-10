
#include "PlayerbotMgr.h"
#include "playerbot.h"

#include "AiFactory.h"

#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "strategy/values/LastMovementValue.h"
#include "strategy/actions/LogLevelAction.h"
#include "strategy/values/LastSpellCastValue.h"
#include "LootObjectStack.h"
#include "PlayerbotAIConfig.h"
#include "PlayerbotAI.h"
#include "PlayerbotFactory.h"
#include "PlayerbotSecurity.h"
#include "Group.h"
#include "Pet.h"
#include "SpellAuras.h"
#include "CharacterCache.h"
#include "SpellAuraEffects.h"
#include "SpellHistory.h"

using namespace ai;
using namespace std;

vector<std::string>& split(const std::string &s, char delim, vector<std::string> &elems);
vector<std::string> split(const std::string &s, char delim);
std::string &trim(std::string &s);

PlayerbotChatHandler::PlayerbotChatHandler(Player* pMasterPlayer) 
    : ChatHandler(pMasterPlayer->GetSession()) 
{ }

uint32 PlayerbotChatHandler::extractQuestId(std::string str)
{
    char* source = (char*)str.c_str();
    char* cId = extractKeyFromLink(source,"Hquest");
    return cId ? atol(cId) : 0;
}

void PacketHandlingHelper::AddHandler(uint16 opcode, std::string handler)
{
    handlers[opcode] = handler;
}

void PacketHandlingHelper::Handle(ExternalEventHelper &helper)
{
    while (!queue.empty())
    {
        helper.HandlePacket(handlers, queue.top());
        queue.pop();
    }
}

void PacketHandlingHelper::AddPacket(const WorldPacket& packet)
{
    if (handlers.find(packet.GetOpcode()) != handlers.end())
        queue.push(WorldPacket(packet));
}


PlayerbotAI::PlayerbotAI() : PlayerbotAIBase(), bot(nullptr), aiObjectContext(nullptr),
    currentEngine(nullptr), chatHelper(this), chatFilter(this), accountId(0), security(nullptr), master(nullptr), currentState(BOT_STATE_NON_COMBAT)
{
    for (int i = 0 ; i < BOT_STATE_MAX; i++)
        engines[i] = nullptr;
}

PlayerbotAI::PlayerbotAI(Player* bot) :
    PlayerbotAIBase(), chatHelper(this), chatFilter(this), security(bot), master(nullptr)
{
    this->bot = bot;

    accountId = sCharacterCache->GetCharacterAccountIdByGuid(bot->GetGUID());

    aiObjectContext = AiFactory::createAiObjectContext(bot, this);

    engines[BOT_STATE_COMBAT] = AiFactory::createCombatEngine(bot, this, aiObjectContext);
    engines[BOT_STATE_NON_COMBAT] = AiFactory::createNonCombatEngine(bot, this, aiObjectContext);
    engines[BOT_STATE_DEAD] = AiFactory::createDeadEngine(bot, this, aiObjectContext);
    currentEngine = engines[BOT_STATE_NON_COMBAT];
    currentState = BOT_STATE_NON_COMBAT;

#ifdef LICH_KING
    masterIncomingPacketHandlers.AddHandler(CMSG_GAMEOBJ_REPORT_USE, "use game object");
#endif
    masterIncomingPacketHandlers.AddHandler(CMSG_AREATRIGGER, "area trigger");
    masterIncomingPacketHandlers.AddHandler(CMSG_GAMEOBJ_USE, "use game object");
    masterIncomingPacketHandlers.AddHandler(CMSG_LOOT_ROLL, "loot roll");
    masterIncomingPacketHandlers.AddHandler(CMSG_GOSSIP_HELLO, "gossip hello");
    masterIncomingPacketHandlers.AddHandler(CMSG_QUESTGIVER_HELLO, "gossip hello");
    masterIncomingPacketHandlers.AddHandler(CMSG_QUESTGIVER_COMPLETE_QUEST, "complete quest");
    masterIncomingPacketHandlers.AddHandler(CMSG_QUESTGIVER_ACCEPT_QUEST, "accept quest");
    masterIncomingPacketHandlers.AddHandler(CMSG_ACTIVATETAXI, "activate taxi");
    masterIncomingPacketHandlers.AddHandler(CMSG_ACTIVATETAXIEXPRESS, "activate taxi");
    masterIncomingPacketHandlers.AddHandler(CMSG_MOVE_SPLINE_DONE, "taxi done");
    masterIncomingPacketHandlers.AddHandler(CMSG_GROUP_UNINVITE_GUID, "uninvite");
    masterIncomingPacketHandlers.AddHandler(CMSG_PUSHQUESTTOPARTY, "quest share");
    masterIncomingPacketHandlers.AddHandler(CMSG_GUILD_INVITE, "guild invite");
#ifdef LICH_KING
    masterIncomingPacketHandlers.AddHandler(CMSG_LFG_TELEPORT, "lfg teleport");
#endif

    botOutgoingPacketHandlers.AddHandler(SMSG_GROUP_INVITE, "group invite");
    botOutgoingPacketHandlers.AddHandler(BUY_ERR_NOT_ENOUGHT_MONEY, "not enough money");
    botOutgoingPacketHandlers.AddHandler(BUY_ERR_REPUTATION_REQUIRE, "not enough reputation");
    botOutgoingPacketHandlers.AddHandler(SMSG_GROUP_SET_LEADER, "group set leader");
    botOutgoingPacketHandlers.AddHandler(SMSG_FORCE_RUN_SPEED_CHANGE, "check mount state");
    botOutgoingPacketHandlers.AddHandler(SMSG_RESURRECT_REQUEST, "resurrect request");
    botOutgoingPacketHandlers.AddHandler(SMSG_INVENTORY_CHANGE_FAILURE, "cannot equip");
    botOutgoingPacketHandlers.AddHandler(SMSG_TRADE_STATUS, "trade status");
    botOutgoingPacketHandlers.AddHandler(SMSG_LOOT_RESPONSE, "loot response");
    botOutgoingPacketHandlers.AddHandler(SMSG_QUESTUPDATE_ADD_KILL, "quest objective completed");
    botOutgoingPacketHandlers.AddHandler(SMSG_ITEM_PUSH_RESULT, "item push result");
    botOutgoingPacketHandlers.AddHandler(SMSG_PARTY_COMMAND_RESULT, "party command");
    botOutgoingPacketHandlers.AddHandler(SMSG_CAST_FAILED, "cast failed");
    botOutgoingPacketHandlers.AddHandler(SMSG_DUEL_REQUESTED, "duel requested");
#ifdef LICH_KING
    botOutgoingPacketHandlers.AddHandler(SMSG_LFG_ROLE_CHECK_UPDATE, "lfg role check");
    botOutgoingPacketHandlers.AddHandler(SMSG_LFG_PROPOSAL_UPDATE, "lfg proposal");
#endif

    masterOutgoingPacketHandlers.AddHandler(SMSG_PARTY_COMMAND_RESULT, "party command");
    masterOutgoingPacketHandlers.AddHandler(MSG_RAID_READY_CHECK, "ready check");
    masterOutgoingPacketHandlers.AddHandler(MSG_RAID_READY_CHECK_FINISHED, "ready check finished");
}

PlayerbotAI::~PlayerbotAI()
{
    for (int i = 0 ; i < BOT_STATE_MAX; i++)
    {
        if (engines[i])
            delete engines[i];
    }

    if (aiObjectContext)
        delete aiObjectContext;
}

void PlayerbotAI::UpdateAI(uint32 elapsed)
{
    if (bot->IsBeingTeleported())
        return;

    if (nextAICheckDelay > sPlayerbotAIConfig.globalCoolDown &&
            bot->IsNonMeleeSpellCast(true, true, false) &&
            *GetAiObjectContext()->GetValue<bool>("invalid target", "current target"))
    {
        Spell* spell = bot->GetCurrentSpell(CURRENT_GENERIC_SPELL);
        if (spell && !spell->GetSpellInfo()->IsPositive())
        {
            InterruptSpell();
            SetNextCheckDelay(sPlayerbotAIConfig.globalCoolDown);
        }
    }

    if (nextAICheckDelay > sPlayerbotAIConfig.maxWaitForMove && bot->IsInCombat() && !bot->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
    {
        nextAICheckDelay = sPlayerbotAIConfig.maxWaitForMove;
    }

    PlayerbotAIBase::UpdateAI(elapsed);
}


void PlayerbotAI::UpdateAIInternal(uint32 elapsed)
{
    ExternalEventHelper helper(aiObjectContext);
    while (!chatCommands.empty())
    {
        ChatCommandHolder holder = chatCommands.top();
        std::string command = holder.GetCommand();
        Player* owner = holder.GetOwner();
        if (!helper.ParseChatCommand(command, owner) && holder.GetType() == CHAT_MSG_WHISPER)
        {
            std::ostringstream out; out << "Unknown command " << command;
            TellMaster(out);
            helper.ParseChatCommand("help");
        }
        chatCommands.pop();
    }

    botOutgoingPacketHandlers.Handle(helper);
    masterIncomingPacketHandlers.Handle(helper);
    masterOutgoingPacketHandlers.Handle(helper);

    DoNextAction();
}

void PlayerbotAI::HandleTeleportAck()
{
    bot->GetMotionMaster()->Clear();
    if (bot->IsBeingTeleportedNear())
    {
        WorldPacket p = WorldPacket(MSG_MOVE_TELEPORT_ACK, 8 + 4 + 4);
#ifdef LICH_KING
        p.appendPackGUID(bot->GetGUID());
#else
        p << uint64(bot->GetGUID());
#endif
        p << (uint32) 0; // flags, not used currently
        p << (uint32) time(0); // time - not currently used
        bot->GetSession()->HandleMoveTeleportAck(p);
    }
    else if (bot->IsBeingTeleportedFar())
    {
        bot->GetSession()->HandleMoveWorldportAck();
        SetNextCheckDelay(1000);
    }
}

void PlayerbotAI::Reset()
{
    if (bot->IsFlying())
        return;

    currentEngine = engines[BOT_STATE_NON_COMBAT];
    nextAICheckDelay = 0;

    aiObjectContext->GetValue<Unit*>("old target")->Set(NULL);
    aiObjectContext->GetValue<Unit*>("current target")->Set(NULL);
    aiObjectContext->GetValue<LootObject>("loot target")->Set(LootObject());
    aiObjectContext->GetValue<uint32>("lfg proposal")->Set(0);

    LastSpellCast & lastSpell = aiObjectContext->GetValue<LastSpellCast& >("last spell cast")->Get();
    lastSpell.Reset();

    LastMovement & lastMovement = aiObjectContext->GetValue<LastMovement& >("last movement")->Get();
    lastMovement.Set(NULL);

    bot->GetMotionMaster()->Clear();
    bot->m_taxi.ClearTaxiDestinations();
    InterruptSpell();

    for (int i = 0 ; i < BOT_STATE_MAX; i++)
    {
        engines[i]->Init();
    }
}

void PlayerbotAI::HandleCommand(uint32 type, const std::string& text, Player& fromPlayer)
{
    if (!GetSecurity()->CheckLevelFor(PLAYERBOT_SECURITY_INVITE, type != CHAT_MSG_WHISPER, &fromPlayer))
        return;

    if (type == CHAT_MSG_ADDON)
        return;

    std::string filtered = text;
    if (!sPlayerbotAIConfig.commandPrefix.empty())
    {
        if (filtered.find(sPlayerbotAIConfig.commandPrefix) != 0)
            return;

        filtered = filtered.substr(sPlayerbotAIConfig.commandPrefix.size());
    }

    filtered = chatFilter.Filter(trim((string&)filtered));
    if (filtered.empty())
        return;

    if (filtered.find("who") != 0 && !GetSecurity()->CheckLevelFor(PLAYERBOT_SECURITY_ALLOW_ALL, type != CHAT_MSG_WHISPER, &fromPlayer))
        return;

    if (type == CHAT_MSG_RAID_WARNING && filtered.find(bot->GetName()) != std::string::npos && filtered.find("award") == std::string::npos)
    {
        ChatCommandHolder cmd("warning", &fromPlayer, type);
        chatCommands.push(cmd);
        return;
    }

    if ((filtered.size() > 2 && filtered.substr(0, 2) == "d ") || (filtered.size() > 3 && filtered.substr(0, 3) == "do "))
    {
        std::string action = filtered.substr(filtered.find(" ") + 1);
        DoSpecificAction(action);
    }
    else if (filtered == "reset")
    {
        Reset();
    }
    else
    {
        ChatCommandHolder cmd(filtered, &fromPlayer, type);
        chatCommands.push(cmd);
    }
}

void PlayerbotAI::HandleBotOutgoingPacket(const WorldPacket& packet)
{
    switch (packet.GetOpcode())
    {
    case SMSG_MOVE_SET_CAN_FLY:
        { //BC + LK okay
            WorldPacket p(packet);
            ObjectGuid guid;
            p >> guid.ReadAsPacked();
            if (guid != bot->GetGUID())
                return;

            bot->SetUnitMovementFlags((MovementFlags)(MOVEMENTFLAG_PLAYER_FLYING | MOVEMENTFLAG_CAN_FLY));
            return;
        }
    case SMSG_MOVE_UNSET_CAN_FLY:
        { //BC + LK okay
            WorldPacket p(packet);
            ObjectGuid guid;
            p >> guid.ReadAsPacked();
            if (guid != bot->GetGUID())
                return;
            bot->GetMovementInfo().RemoveMovementFlag(MOVEMENTFLAG_PLAYER_FLYING);
            return;
        }
    case SMSG_CAST_FAILED:
        { //BC + LK okay
            WorldPacket p(packet);
            p.rpos(0);
            uint8 castCount, result;
            uint32 spellId;
#ifdef LICH_KING
            p >> castCount >> spellId >> result;
#else
            p >> spellId >> result >> castCount;
#endif
            SpellInterrupted(spellId);
            botOutgoingPacketHandlers.AddPacket(packet);
            return;
        }
    case SMSG_SPELL_FAILURE:
        { //BC + LK okay
            WorldPacket p(packet);
            p.rpos(0);
            ObjectGuid casterGuid;
            p >> casterGuid.ReadAsPacked();
            if (casterGuid != bot->GetGUID())
                return;

            uint32 spellId;
#ifdef LICH_KING
            uint8 castCount;
            p >> castCount;
#endif
            p >> spellId;
            SpellInterrupted(spellId);
            return;
        }
    case SMSG_SPELL_DELAYED:
        { //BC + LK okay
            WorldPacket p(packet);
            p.rpos(0);
            ObjectGuid casterGuid;
            p >> casterGuid.ReadAsPacked();

            if (casterGuid != bot->GetGUID())
                return;

            uint32 delaytime;
            p >> delaytime;
            if (delaytime <= 1000)
                IncreaseNextCheckDelay(delaytime);
            return;
        }
    default:
        botOutgoingPacketHandlers.AddPacket(packet);
    }
}

void PlayerbotAI::SpellInterrupted(uint32 spellid)
{
    LastSpellCast& lastSpell = aiObjectContext->GetValue<LastSpellCast&>("last spell cast")->Get();
    if (lastSpell.id != spellid)
        return;

    lastSpell.Reset();

    time_t now = time(0);
    if (now <= lastSpell.time)
        return;

    uint32 castTimeSpent = 1000 * (now - lastSpell.time);

    int32 globalCooldown = CalculateGlobalCooldown(lastSpell.id);
    if (castTimeSpent < globalCooldown)
        SetNextCheckDelay(globalCooldown - castTimeSpent);
    else
        SetNextCheckDelay(0);

    lastSpell.id = 0;
}

int32 PlayerbotAI::CalculateGlobalCooldown(uint32 spellid)
{
    if (!spellid)
        return 0;

    SpellInfo const *spellInfo = sSpellMgr->GetSpellInfo(spellid);

    if(bot->GetSpellHistory()->HasGlobalCooldown(spellInfo))
        return sPlayerbotAIConfig.globalCoolDown;

    return sPlayerbotAIConfig.reactDelay;
}

void PlayerbotAI::HandleMasterIncomingPacket(const WorldPacket& packet)
{
    masterIncomingPacketHandlers.AddPacket(packet);
}

void PlayerbotAI::HandleMasterOutgoingPacket(const WorldPacket& packet)
{
    masterOutgoingPacketHandlers.AddPacket(packet);
}

void PlayerbotAI::ChangeEngine(BotState type)
{
    Engine* engine = engines[type];

    if (currentEngine != engine)
    {
        currentEngine = engine;
        currentState = type;
        ReInitCurrentEngine();

        switch (type)
        {
        case BOT_STATE_COMBAT:
            sLog->outMessage("playerbot", LOG_LEVEL_DEBUG, "=== %s COMBAT ===", bot->GetName().c_str());
            break;
        case BOT_STATE_NON_COMBAT:
            sLog->outMessage("playerbot", LOG_LEVEL_DEBUG, "=== %s NON-COMBAT ===", bot->GetName().c_str());
            break;
        case BOT_STATE_DEAD:
            sLog->outMessage("playerbot", LOG_LEVEL_DEBUG, "=== %s DEAD ===", bot->GetName().c_str());
            break;
        }
    }
}

void PlayerbotAI::DoNextAction()
{
    if (bot->IsBeingTeleported() || (GetMaster() && GetMaster()->IsBeingTeleported()))
        return;

    currentEngine->DoNextAction(NULL);

    if (bot->HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED))
    {
        bot->SetUnitMovementFlags((MovementFlags)(MOVEMENTFLAG_PLAYER_FLYING|MOVEMENTFLAG_CAN_FLY));

        // TODO
        //WorldPacket packet(CMSG_MOVE_SET_FLY);
        //packet.appendPackGUID(bot->GetGUID());
        //packet << bot->m_movementInfo;
        //bot->SetMovedUnit(bot);
        //bot->GetSession()->HandleMovementOpcodes(packet);
    }

    Player* _master = GetMaster();
    if (bot->IsMounted() && bot->IsFlying())
    {
        bot->SetUnitMovementFlags((MovementFlags)(MOVEMENTFLAG_PLAYER_FLYING|MOVEMENTFLAG_CAN_FLY));
        bot->SetSpeedRate(MOVE_FLIGHT, 1.0f, true);
        bot->SetSpeedRate(MOVE_RUN, 1.0f, true);

        if (_master)
        {
            bot->SetSpeedRate(MOVE_FLIGHT, _master->GetSpeedRate(MOVE_FLIGHT), true);
            bot->SetSpeedRate(MOVE_RUN, _master->GetSpeedRate(MOVE_FLIGHT), true);
        }

    }

    if (currentEngine != engines[BOT_STATE_DEAD] && !bot->IsAlive())
        ChangeEngine(BOT_STATE_DEAD);

    if (currentEngine == engines[BOT_STATE_DEAD] && bot->IsAlive())
        ChangeEngine(BOT_STATE_NON_COMBAT);

    Group *group = bot->GetGroup();
    if (!_master && group)
    {
        for (GroupReference *gref = group->GetFirstMember(); gref; gref = gref->next())
        {
            Player* member = gref->GetSource();
            PlayerbotAI* ai = bot->GetPlayerbotAI();
            if (member && member->IsInWorld() && !member->GetPlayerbotAI() && (!_master || _master->GetPlayerbotAI()))
            {
                ai->SetMaster(_master);
                ai->ResetStrategies();
                ai->TellMaster("Hello");
                break;
            }
        }
    }
}

void PlayerbotAI::ReInitCurrentEngine()
{
    InterruptSpell();
    currentEngine->Init();
}

void PlayerbotAI::ChangeStrategy(std::string names, BotState type)
{
    Engine* e = engines[type];
    if (!e)
        return;

    e->ChangeStrategy(names);
}

void PlayerbotAI::DoSpecificAction(std::string name)
{
    for (int i = 0 ; i < BOT_STATE_MAX; i++)
    {
        std::ostringstream out;
        ActionResult res = engines[i]->ExecuteAction(name);
        switch (res)
        {
        case ACTION_RESULT_UNKNOWN:
            continue;
        case ACTION_RESULT_OK:
            out << name << ": done";
            TellMaster(out);
            return;
        case ACTION_RESULT_IMPOSSIBLE:
            out << name << ": impossible";
            TellMaster(out);
            return;
        case ACTION_RESULT_USELESS:
            out << name << ": useless";
            TellMaster(out);
            return;
        case ACTION_RESULT_FAILED:
            out << name << ": failed";
            TellMaster(out);
            return;
        }
    }
    std::ostringstream out;
    out << name << ": unknown action";
    TellMaster(out);
}

bool PlayerbotAI::ContainsStrategy(StrategyType type)
{
    for (int i = 0 ; i < BOT_STATE_MAX; i++)
    {
        if (engines[i]->ContainsStrategy(type))
            return true;
    }
    return false;
}

bool PlayerbotAI::HasStrategy(std::string name, BotState type)
{
    return engines[type]->HasStrategy(name);
}

void PlayerbotAI::ResetStrategies()
{
    for (int i = 0 ; i < BOT_STATE_MAX; i++)
        engines[i]->removeAllStrategies();

    AiFactory::AddDefaultCombatStrategies(bot, this, engines[BOT_STATE_COMBAT]);
    AiFactory::AddDefaultNonCombatStrategies(bot, this, engines[BOT_STATE_NON_COMBAT]);
    AiFactory::AddDefaultDeadStrategies(bot, this, engines[BOT_STATE_DEAD]);
}

bool PlayerbotAI::IsRanged(Player* player)
{
    PlayerbotAI* botAi = player->GetPlayerbotAI();
    if (botAi)
        return botAi->ContainsStrategy(STRATEGY_TYPE_RANGED);

    switch (player->GetClass())
    {
    case CLASS_DEATH_KNIGHT:
    case CLASS_PALADIN:
    case CLASS_WARRIOR:
    case CLASS_ROGUE:
        return false;
    case CLASS_DRUID:
        return !HasAnyAuraOf(player, { "cat form", "bear form", "dire bear form" });
    }
    return true;
}

bool PlayerbotAI::IsTank(Player* player)
{
    PlayerbotAI* botAi = player->GetPlayerbotAI();
    if (botAi)
        return botAi->ContainsStrategy(STRATEGY_TYPE_TANK);

    switch (player->GetClass())
    {
    case CLASS_DEATH_KNIGHT:
    case CLASS_PALADIN:
    case CLASS_WARRIOR:
        return true;
    case CLASS_DRUID:
        return HasAnyAuraOf(player, { "bear form", "dire bear form" });
    }
    return false;
}

bool PlayerbotAI::IsHeal(Player* player)
{
    PlayerbotAI* botAi = player->GetPlayerbotAI();
    if (botAi)
        return botAi->ContainsStrategy(STRATEGY_TYPE_HEAL);

    switch (player->GetClass())
    {
    case CLASS_PRIEST:
        return true;
    case CLASS_DRUID:
        return HasAnyAuraOf(player, { "tree of life form" });
    }
    return false;
}



namespace MaNGOS
{

    class UnitByGuidInRangeCheck
    {
    public:
        UnitByGuidInRangeCheck(WorldObject const* obj, ObjectGuid guid, float range) : i_obj(obj), i_range(range), i_guid(guid) {}
        WorldObject const& GetFocusObject() const { return *i_obj; }
        bool operator()(Unit* u)
        {
            return u->GetGUID() == i_guid && i_obj->IsWithinDistInMap(u, i_range);
        }
    private:
        WorldObject const* i_obj;
        float i_range;
        ObjectGuid i_guid;
    };

    class GameObjectByGuidInRangeCheck
    {
    public:
        GameObjectByGuidInRangeCheck(WorldObject const* obj, ObjectGuid guid, float range) : i_obj(obj), i_range(range), i_guid(guid) {}
        WorldObject const& GetFocusObject() const { return *i_obj; }
        bool operator()(GameObject* u)
        {
            if (u && i_obj->IsWithinDistInMap(u, i_range) && u->isSpawned() && u->GetGOInfo() && u->GetGUID() == i_guid)
                return true;

            return false;
        }
    private:
        WorldObject const* i_obj;
        float i_range;
        ObjectGuid i_guid;
    };

};


Unit* PlayerbotAI::GetUnit(ObjectGuid guid)
{
    if (!guid)
        return nullptr;

    Map* map = bot->FindMap();
    if (!map)
        return nullptr;

    if (!guid.IsUnit())
        return nullptr;

    return (Unit*)ObjectAccessor::GetWorldObject(*bot, guid);
}


Creature* PlayerbotAI::GetCreature(ObjectGuid guid)
{
    if (!guid)
        return NULL;

    Map* map = bot->FindMap();
    if (!map)
        return NULL;

    return map->GetCreature(guid);
}

GameObject* PlayerbotAI::GetGameObject(ObjectGuid guid)
{
    if (!guid)
        return NULL;

    Map* map = bot->FindMap();
    if (!map)
        return NULL;

    return map->GetGameObject(guid);
}

bool PlayerbotAI::TellMasterNoFacing(std::string text, PlayerbotSecurityLevel securityLevel)
{
    Player* _master = GetMaster();
    if (!_master)
        return false;

    if (!GetSecurity()->CheckLevelFor(securityLevel, true, _master))
        return false;

    if (sPlayerbotAIConfig.whisperDistance && !bot->GetGroup() && sRandomPlayerbotMgr.IsRandomBot(bot) &&
            _master->GetSession()->GetSecurity() < SEC_GAMEMASTER1 &&
            (bot->GetMapId() != _master->GetMapId() || bot->GetDistance(_master) > sPlayerbotAIConfig.whisperDistance))
        return false;

    bot->Whisper(text, LANG_UNIVERSAL, _master);
    return true;
}

bool PlayerbotAI::TellMaster(std::string text, PlayerbotSecurityLevel securityLevel)
{
    if (!TellMasterNoFacing(text, securityLevel))
        return false;

    if (!bot->isMoving() && !bot->IsInCombat() && bot->GetMapId() == master->GetMapId())
    {
        if (!bot->isInFront(master, M_PI / 2))
            bot->SetFacingTo(bot->GetAbsoluteAngle(master));

        bot->HandleEmoteCommand(EMOTE_ONESHOT_TALK);
    }

    return true;
}

bool IsRealAura(Player* bot, Aura const* aura, Unit* unit)
{
    if (!aura)
        return false;

    if (!unit->IsHostileTo(bot))
        return true;

    uint32 stacks = aura->GetStackAmount();
    if (stacks >= aura->GetSpellInfo()->StackAmount)
        return true;

    if (aura->GetCaster() == bot || aura->GetSpellInfo()->IsPositive() || aura->IsArea())
        return true;

    return false;
}

bool PlayerbotAI::HasAura(std::string name, Unit* unit)
{
    if (!unit)
        return false;

    uint32 spellId = aiObjectContext->GetValue<uint32>("spell id", name)->Get();
    if (spellId)
        return HasAura(spellId, unit);

    wstring wnamepart;
    if (!Utf8toWStr(name, wnamepart))
        return 0;

    wstrToLower(wnamepart);

    auto map = unit->GetAppliedAuras();
    for (auto i = map.begin(); i != map.end(); ++i)
    {
        Aura const* aura  = i->second->GetBase();
        if (!aura)
            continue;

        const std::string auraName = aura->GetSpellInfo()->SpellName[0];
        if (auraName.empty() || auraName.length() != wnamepart.length() || !Utf8FitTo(auraName, wnamepart))
            continue;

        if (IsRealAura(bot, aura, unit))
            return true;
    }

    return false;
}

bool PlayerbotAI::HasAura(uint32 spellId, const Unit* unit)
{
    if (!spellId || !unit)
        return false;

    Aura* aura = ((Unit*)unit)->GetAura(spellId);

    if (IsRealAura(bot, aura, (Unit*)unit))
        return true;

    return false;
}

bool PlayerbotAI::HasAnyAuraOf(Unit* player, std::initializer_list<string> auras)
{
    if (!player)
        return false;

    for (auto aura : auras)
        if (!aura.empty() && HasAura(aura, player))
            return true;

    return false;
}

bool PlayerbotAI::CanCastSpell(std::string name, Unit* target)
{
    return CanCastSpell(aiObjectContext->GetValue<uint32>("spell id", name)->Get(), target);
}

bool PlayerbotAI::CanCastSpell(uint32 spellid, Unit* target, bool checkHasSpell)
{
    if (!spellid)
        return false;

    if (!target)
        target = bot;

    if (checkHasSpell && !bot->HasSpell(spellid))
        return false;

    if (bot->GetSpellHistory()->HasCooldown(spellid))
        return false;

    SpellInfo const *spellInfo = sSpellMgr->GetSpellInfo(spellid );
    if (!spellInfo)
        return false;

    bool positiveSpell = spellInfo->IsPositive();
    if (positiveSpell && bot->IsHostileTo(target))
        return false;

    if (!positiveSpell && bot->IsFriendlyTo(target))
        return false;

    if (target->IsImmunedToSpell(spellInfo, bot))
        return false;

    if (bot != target && bot->GetDistance(target) > sPlayerbotAIConfig.sightDistance)
        return false;

    Unit* oldSel = bot->GetSelectedUnit();
    bot->SetSelection(target->GetGUID());
    Spell *spell = new Spell(bot, spellInfo, TRIGGERED_NONE);

    spell->m_targets.SetUnitTarget(target);
    spell->m_CastItem = aiObjectContext->GetValue<Item*>("item for spell", spellid)->Get();
    spell->m_targets.SetItemTarget(spell->m_CastItem);
    SpellCastResult result = spell->CheckCast(false);
    delete spell;
    if (oldSel)
        bot->SetSelection(oldSel->GetGUID());

    switch (result)
    {
    case SPELL_FAILED_NOT_INFRONT:
    case SPELL_FAILED_NOT_STANDING:
    case SPELL_FAILED_UNIT_NOT_INFRONT:
    //case SPELL_FAILED_SUCCESS:
    case SPELL_FAILED_MOVING:
    case SPELL_FAILED_TRY_AGAIN:
    case SPELL_FAILED_NOT_IDLE:
    /*case SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW:
    case SPELL_FAILED_SUMMON_PENDING:*/
    case SPELL_FAILED_BAD_IMPLICIT_TARGETS:
    case SPELL_FAILED_BAD_TARGETS:
    case SPELL_CAST_OK:
    case SPELL_FAILED_ITEM_NOT_FOUND:
        return true;
    default:
        return false;
    }
}


bool PlayerbotAI::CastSpell(std::string name, Unit* target)
{
    bool result = CastSpell(aiObjectContext->GetValue<uint32>("spell id", name)->Get(), target);
    if (result)
    {
        aiObjectContext->GetValue<time_t>("last spell cast time", name)->Set(time(0));
    }

    return result;
}

bool PlayerbotAI::CastSpell(uint32 spellId, Unit* target)
{
    if (!spellId)
        return false;

    if (!target)
        target = bot;

    Pet* pet = bot->GetPet();
    const SpellInfo* const pSpellInfo = sSpellMgr->GetSpellInfo(spellId);
    if (pet && pet->HasSpell(spellId))
    {
        pet->GetCharmInfo()->SetSpellAutocast(pSpellInfo, true);
        TellMaster("My pet will auto-cast this spell");
        return true;
    }

    aiObjectContext->GetValue<LastSpellCast&>("last spell cast")->Get().Set(spellId, ObjectGuid(target->GetGUID()), time(0));
    aiObjectContext->GetValue<LastMovement&>("last movement")->Get().Set(NULL);

//    MotionMaster &mm = *bot->GetMotionMaster();

    if (bot->IsFlying())
        return false;

    bot->ClearUnitState( UNIT_STATE_ALL_STATE_SUPPORTED );

    Unit* oldSel = bot->GetSelectedUnit();
    bot->SetSelection(target->GetGUID());

    Spell *spell = new Spell(bot, pSpellInfo, TRIGGERED_NONE);
    if (bot->isMoving() && spell->GetCastTime())
    {
        delete spell;
        return false;
    }

    SpellCastTargets targets;
    WorldObject* faceTo = target;

    if (pSpellInfo->Targets & TARGET_FLAG_SOURCE_LOCATION ||
            pSpellInfo->Targets & TARGET_FLAG_DEST_LOCATION)
    {
        targets.SetDst(target->GetPosition());
    }
    else
    {
        targets.SetUnitTarget(target);
    }

    if (pSpellInfo->Targets & TARGET_FLAG_ITEM)
    {
        spell->m_CastItem = aiObjectContext->GetValue<Item*>("item for spell", spellId)->Get();
        targets.SetItemTarget(spell->m_CastItem);
    }

    if (pSpellInfo->Effects[0].Effect == SPELL_EFFECT_OPEN_LOCK ||
        pSpellInfo->Effects[0].Effect == SPELL_EFFECT_SKINNING)
    {
        LootObject loot = *aiObjectContext->GetValue<LootObject>("loot target");
        if (!loot.IsLootPossible(bot))
        {
            delete spell;
            return false;
        }

        GameObject* go = GetGameObject(loot.guid);
        if (go && go->isSpawned())
        {
#ifdef LICH_KING
            WorldPacket* const packetgouse = new WorldPacket(CMSG_GAMEOBJ_REPORT_USE, 8);
#else
            WorldPacket* const packetgouse = new WorldPacket(CMSG_GAMEOBJ_USE, 8);
#endif
            *packetgouse << loot.guid;
            bot->GetSession()->QueuePacket(packetgouse);
            targets.SetGOTarget(go);
            faceTo = go;
        }
        else
        {
            Unit* creature = GetUnit(loot.guid);
            if (creature)
            {
                targets.SetUnitTarget(creature);
                faceTo = creature;
            }
        }
    }


    if (!bot->isInFront(faceTo, M_PI / 2))
    {
        bot->SetFacingTo(bot->GetAbsoluteAngle(faceTo));
        delete spell;
        SetNextCheckDelay(sPlayerbotAIConfig.globalCoolDown);
        return false;
    }

    spell->prepare(targets);
    WaitForSpellCast(spell);

    if (oldSel)
        bot->SetSelection(oldSel->GetGUID());

    LastSpellCast& lastSpell = aiObjectContext->GetValue<LastSpellCast&>("last spell cast")->Get();
    return lastSpell.id == spellId;
}

void PlayerbotAI::WaitForSpellCast(Spell *spell)
{
    const SpellInfo* const pSpellInfo = spell->GetSpellInfo();

    float castTime = spell->GetCastTime();
    if (pSpellInfo->IsChanneled())
    {
        int32 duration = pSpellInfo->GetDuration();
        if (duration > 0)
            castTime += duration;
    }

    castTime = ceil(castTime);

    uint32 globalCooldown = CalculateGlobalCooldown(pSpellInfo->Id);
    if (castTime < globalCooldown)
        castTime = globalCooldown;

    SetNextCheckDelay(castTime);
}

void PlayerbotAI::InterruptSpell()
{
    if (bot->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
        return;

    LastSpellCast& lastSpell = aiObjectContext->GetValue<LastSpellCast&>("last spell cast")->Get();

    for (int type = CURRENT_MELEE_SPELL; type < CURRENT_CHANNELED_SPELL; type++)
    {
        Spell* spell = bot->GetCurrentSpell((CurrentSpellTypes)type);
        if (!spell)
            continue;

        if (spell->m_spellInfo->IsPositive())
            continue;

        bot->InterruptSpell((CurrentSpellTypes)type);

        spell->SendInterrupted(0);

        SpellInterrupted(spell->m_spellInfo->Id);
    }

    SpellInterrupted(lastSpell.id);
}


void PlayerbotAI::RemoveAura(std::string name)
{
    uint32 spellid = aiObjectContext->GetValue<uint32>("spell id", name)->Get();
    if (spellid && HasAura(spellid, bot))
        bot->RemoveAurasDueToSpell(spellid);
}

bool PlayerbotAI::IsInterruptableSpellCasting(Unit* target, std::string spell)
{
    uint32 spellid = aiObjectContext->GetValue<uint32>("spell id", spell)->Get();
    if (!spellid || !target->IsNonMeleeSpellCast(true))
        return false;

    SpellInfo const *spellInfo = sSpellMgr->GetSpellInfo(spellid );
    if (!spellInfo)
        return false;

    if (target->IsImmunedToSpell(spellInfo, bot))
        return false;

    for (uint32 i = EFFECT_0; i <= EFFECT_2; i++)
    {
        if ((spellInfo->InterruptFlags & SPELL_INTERRUPT_FLAG_INTERRUPT) && spellInfo->PreventionType == SPELL_PREVENTION_TYPE_SILENCE)
            return true;

        if ((/*spellInfo->Effects[i].Effect == SPELL_EFFECT_REMOVE_AURA || */ spellInfo->Effects[i].Effect == SPELL_EFFECT_INTERRUPT_CAST) &&
                !target->IsImmunedToSpellEffect(spellInfo, i, bot))
            return true;
    }

    return false;
}

bool PlayerbotAI::HasAuraToDispel(Unit* target, uint32 dispelType)
{
    for (uint32 type = SPELL_AURA_NONE; type < TOTAL_AURAS; ++type)
    {
        auto const& auras = target->GetAuraEffectsByType((AuraType)type);
        for (std::list<AuraEffect*>::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
        {
            const Aura* const aura = (*itr)->GetBase();
            const SpellInfo* entry = aura->GetSpellInfo();
//            uint32 spellId = entry->Id;

            bool isPositiveSpell = entry->IsPositive();
            if (isPositiveSpell && bot->IsFriendlyTo(target))
                continue;

            if (!isPositiveSpell && bot->IsHostileTo(target))
                continue;

            if (canDispel(entry, dispelType))
                return true;
        }
    }
    return false;
}


#ifndef WIN32
inline int strcmpi(const char* s1, const char* s2)
{
    for (; *s1 && *s2 && (toupper(*s1) == toupper(*s2)); ++s1, ++s2);
    return *s1 - *s2;
}
#endif

bool PlayerbotAI::canDispel(const SpellInfo* entry, uint32 dispelType)
{
    if (entry->Dispel != dispelType)
        return false;

    return !entry->SpellName[0] ||
        (strcmpi((const char*)entry->SpellName[0], "demon skin") &&
        strcmpi((const char*)entry->SpellName[0], "mage armor") &&
        strcmpi((const char*)entry->SpellName[0], "frost armor") &&
        strcmpi((const char*)entry->SpellName[0], "wavering will") &&
        strcmpi((const char*)entry->SpellName[0], "chilled") &&
        strcmpi((const char*)entry->SpellName[0], "ice armor"));
}

bool IsAlliance(uint8 race)
{
    return race == RACE_HUMAN || race == RACE_DWARF || race == RACE_NIGHTELF ||
            race == RACE_GNOME || race == RACE_DRAENEI;
}

bool PlayerbotAI::IsOpposing(Player* player)
{
    return IsOpposing(player->GetRace(), bot->GetRace());
}

bool PlayerbotAI::IsOpposing(uint8 race1, uint8 race2)
{
    return (IsAlliance(race1) && !IsAlliance(race2)) || (!IsAlliance(race1) && IsAlliance(race2));
}

void PlayerbotAI::RemoveShapeshift()
{
    RemoveAura("bear form");
    RemoveAura("dire bear form");
    RemoveAura("moonkin form");
    RemoveAura("travel form");
    RemoveAura("cat form");
    RemoveAura("flight form");
    RemoveAura("swift flight form");
    RemoveAura("aquatic form");
    RemoveAura("ghost wolf");
    RemoveAura("tree of life");
}

uint32 PlayerbotAI::GetEquipGearScore(Player* player, bool withBags, bool withBank)
{
    std::vector<uint32> gearScore(EQUIPMENT_SLOT_END);
    uint32 twoHandScore = 0;

    for (uint8 i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
    {
        if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            _fillGearScoreData(player, item, &gearScore, twoHandScore);
    }

    if (withBags)
    {
        // check inventory
        for (int i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
        {
            if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                _fillGearScoreData(player, item, &gearScore, twoHandScore);
        }

        // check bags
        for (int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
        {
            if (Bag* pBag = (Bag*)player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            {
                for (uint32 j = 0; j < pBag->GetBagSize(); ++j)
                {
                    if (Item* item2 = pBag->GetItemByPos(j))
                        _fillGearScoreData(player, item2, &gearScore, twoHandScore);
                }
            }
        }
    }

    if (withBank)
    {
        for (uint8 i = BANK_SLOT_ITEM_START; i < BANK_SLOT_ITEM_END; ++i)
        {
            if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                _fillGearScoreData(player, item, &gearScore, twoHandScore);
        }

        for (uint8 i = BANK_SLOT_BAG_START; i < BANK_SLOT_BAG_END; ++i)
        {
            if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            {
                if (item->IsBag())
                {
                    Bag* bag = (Bag*)item;
                    for (uint8 j = 0; j < bag->GetBagSize(); ++j)
                    {
                        if (Item* item2 = bag->GetItemByPos(j))
                            _fillGearScoreData(player, item2, &gearScore, twoHandScore);
                    }
                }
            }
        }
    }

    uint8 count = EQUIPMENT_SLOT_END - 2;   // ignore body and tabard slots
    uint32 sum = 0;

    // check if 2h hand is higher level than main hand + off hand
    if (gearScore[EQUIPMENT_SLOT_MAINHAND] + gearScore[EQUIPMENT_SLOT_OFFHAND] < twoHandScore * 2)
    {
        gearScore[EQUIPMENT_SLOT_OFFHAND] = 0;  // off hand is ignored in calculations if 2h weapon has higher score
        --count;
        gearScore[EQUIPMENT_SLOT_MAINHAND] = twoHandScore;
    }

    for (uint8 i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
    {
       sum += gearScore[i];
    }

    if (count)
    {
        uint32 res = uint32(sum / count);
        return res;
    }
    else
        return 0;
}

void PlayerbotAI::_fillGearScoreData(Player *player, Item* item, std::vector<uint32>* gearScore, uint32& twoHandScore)
{
    if (!item)
        return;

    if (!player->CanUseItem(item->GetTemplate()))
        return;

    uint8 type   = item->GetTemplate()->InventoryType;
    uint32 level = item->GetTemplate()->ItemLevel;

    switch (type)
    {
        case INVTYPE_2HWEAPON:
            twoHandScore = std::max(twoHandScore, level);
            break;
        case INVTYPE_WEAPON:
        case INVTYPE_WEAPONMAINHAND:
            (*gearScore)[SLOT_MAIN_HAND] = std::max((*gearScore)[SLOT_MAIN_HAND], level);
            break;
        case INVTYPE_SHIELD:
        case INVTYPE_WEAPONOFFHAND:
            (*gearScore)[EQUIPMENT_SLOT_OFFHAND] = std::max((*gearScore)[EQUIPMENT_SLOT_OFFHAND], level);
            break;
        case INVTYPE_THROWN:
        case INVTYPE_RANGEDRIGHT:
        case INVTYPE_RANGED:
        case INVTYPE_QUIVER:
        case INVTYPE_RELIC:
            (*gearScore)[EQUIPMENT_SLOT_RANGED] = std::max((*gearScore)[EQUIPMENT_SLOT_RANGED], level);
            break;
        case INVTYPE_HEAD:
            (*gearScore)[EQUIPMENT_SLOT_HEAD] = std::max((*gearScore)[EQUIPMENT_SLOT_HEAD], level);
            break;
        case INVTYPE_NECK:
            (*gearScore)[EQUIPMENT_SLOT_NECK] = std::max((*gearScore)[EQUIPMENT_SLOT_NECK], level);
            break;
        case INVTYPE_SHOULDERS:
            (*gearScore)[EQUIPMENT_SLOT_SHOULDERS] = std::max((*gearScore)[EQUIPMENT_SLOT_SHOULDERS], level);
            break;
        case INVTYPE_BODY:
            (*gearScore)[EQUIPMENT_SLOT_BODY] = std::max((*gearScore)[EQUIPMENT_SLOT_BODY], level);
            break;
        case INVTYPE_CHEST:
            (*gearScore)[EQUIPMENT_SLOT_CHEST] = std::max((*gearScore)[EQUIPMENT_SLOT_CHEST], level);
            break;
        case INVTYPE_WAIST:
            (*gearScore)[EQUIPMENT_SLOT_WAIST] = std::max((*gearScore)[EQUIPMENT_SLOT_WAIST], level);
            break;
        case INVTYPE_LEGS:
            (*gearScore)[EQUIPMENT_SLOT_LEGS] = std::max((*gearScore)[EQUIPMENT_SLOT_LEGS], level);
            break;
        case INVTYPE_FEET:
            (*gearScore)[EQUIPMENT_SLOT_FEET] = std::max((*gearScore)[EQUIPMENT_SLOT_FEET], level);
            break;
        case INVTYPE_WRISTS:
            (*gearScore)[EQUIPMENT_SLOT_WRISTS] = std::max((*gearScore)[EQUIPMENT_SLOT_WRISTS], level);
            break;
        case INVTYPE_HANDS:
            (*gearScore)[EQUIPMENT_SLOT_HEAD] = std::max((*gearScore)[EQUIPMENT_SLOT_HEAD], level);
            break;
        // equipped gear score check uses both rings and trinkets for calculation, assume that for bags/banks it is the same
        // with keeping second highest score at second slot
        case INVTYPE_FINGER:
        {
            if ((*gearScore)[EQUIPMENT_SLOT_FINGER1] < level)
            {
                (*gearScore)[EQUIPMENT_SLOT_FINGER2] = (*gearScore)[EQUIPMENT_SLOT_FINGER1];
                (*gearScore)[EQUIPMENT_SLOT_FINGER1] = level;
            }
            else if ((*gearScore)[EQUIPMENT_SLOT_FINGER2] < level)
                (*gearScore)[EQUIPMENT_SLOT_FINGER2] = level;
            break;
        }
        case INVTYPE_TRINKET:
        {
            if ((*gearScore)[EQUIPMENT_SLOT_TRINKET1] < level)
            {
                (*gearScore)[EQUIPMENT_SLOT_TRINKET2] = (*gearScore)[EQUIPMENT_SLOT_TRINKET1];
                (*gearScore)[EQUIPMENT_SLOT_TRINKET1] = level;
            }
            else if ((*gearScore)[EQUIPMENT_SLOT_TRINKET2] < level)
                (*gearScore)[EQUIPMENT_SLOT_TRINKET2] = level;
            break;
        }
        case INVTYPE_CLOAK:
            (*gearScore)[EQUIPMENT_SLOT_BACK] = std::max((*gearScore)[EQUIPMENT_SLOT_BACK], level);
            break;
        default:
            break;
    }
}

string PlayerbotAI::HandleRemoteCommand(std::string command)
{
    if (command == "state")
    {
        switch (currentState)
        {
        case BOT_STATE_COMBAT:
            return "combat";
        case BOT_STATE_DEAD:
            return "dead";
        case BOT_STATE_NON_COMBAT:
            return "non-combat";
        default:
            return "unknown";
        }
    }
    else if (command == "position")
    {
        std::ostringstream out; out << bot->GetMapId() << "," << bot->GetPositionX() << "," << bot->GetPositionY() << "," << bot->GetPositionZ() << "," << bot->GetOrientation();
        return out.str();
    }
    else if (command == "target")
    {
        Unit* target = *GetAiObjectContext()->GetValue<Unit*>("current target");
        if (!target) {
            return "";
        }

        return target->GetName();
    }
    else if (command == "hp")
    {
        int pct = (int)((static_cast<float> (bot->GetHealth()) / bot->GetMaxHealth()) * 100);
        std::ostringstream out; out << pct << "%";

        Unit* target = *GetAiObjectContext()->GetValue<Unit*>("current target");
        if (!target) {
            return out.str();
        }

        pct = (int)((static_cast<float> (target->GetHealth()) / target->GetMaxHealth()) * 100);
        out << " / " << pct << "%";
        return out.str();
    }
    else if (command == "strategy")
    {
        return currentEngine->ListStrategies();
    }
    else if (command == "action")
    {
        return currentEngine->GetLastAction();
    }
    std::ostringstream out; out << "invalid command: " << command;
    return out.str();
}


PlayerbotTestingAI::PlayerbotTestingAI(Player* bot)
    : PlayerbotAI(bot)
{
}

void PlayerbotTestingAI::UpdateAIInternal(uint32 elapsed)
{
    /*
    ExternalEventHelper helper(aiObjectContext);
    botOutgoingPacketHandlers.Handle(helper);
    masterIncomingPacketHandlers.Handle(helper);
    masterOutgoingPacketHandlers.Handle(helper);
    */
}

void PlayerbotTestingAI::CastedDamageSpell(Unit const* target, SpellNonMeleeDamage damageInfo, SpellMissInfo missInfo, bool crit) const
{
    SpellDamageDoneInfo info(damageInfo.SpellID, damageInfo, missInfo, crit);
    spellDamageDone[target->GetGUID()].push_back(std::move(info));
}

void PlayerbotTestingAI::DoneWhiteDamage(Unit const* target, CalcDamageInfo const* damageInfo) const
{
    MeleeDamageDoneInfo info(damageInfo);
    meleeDamageDone[target->GetGUID()].push_back(std::move(info));
}

void PlayerbotTestingAI::CastedHealingSpell(Unit const* target, uint32 healing, uint32 realGain, uint32 spellID, SpellMissInfo missInfo, bool crit) const
{
    HealingDoneInfo info;
    info.spellID = spellID;
    info.healing = healing;
    info.realGain = realGain;
    info.missInfo = missInfo;
    info.crit = crit;

    healingDone[target->GetGUID()].push_back(std::move(info));
}

void PlayerbotTestingAI::SpellDamageDealt(Unit const* tartget, uint32 damage, uint32 spellID) const
{
    //useful?
}

void PlayerbotTestingAI::PeriodicTick(Unit const* target, int32 amount, uint32 spellID) const
{
    TickInfo info;
    info.spellID = spellID;
    info.amount = amount;

    ticksDone[target->GetGUID()].push_back(std::move(info));
}

std::pair<int32 /*totalDmg*/, uint32 /*tickCount*/> PlayerbotTestingAI::GetDotDamage(Unit const* victim, uint32 spellID) const
{
    auto ticksToVictim = ticksDone.find(victim->GetGUID());
    if (ticksToVictim == ticksDone.end())
        return std::make_pair(0, 0);

    uint32 ticksCount = 0;
    int32 totalAmount = 0;
    for (auto itr : ticksToVictim->second)
    {
        if (itr.spellID != spellID)
            continue;

        totalAmount += itr.amount;
        ticksCount++;
    }

    return std::make_pair(totalAmount, ticksCount);
}

std::vector<PlayerbotTestingAI::SpellDamageDoneInfo> const* PlayerbotTestingAI::GetSpellDamageDoneInfo(Unit const* target) const
{
    auto infoForVictimItr = spellDamageDone.find(target->GetGUID());
    if (infoForVictimItr == spellDamageDone.end())
        return nullptr;

    return &(*infoForVictimItr).second;
}

std::vector<PlayerbotTestingAI::MeleeDamageDoneInfo> const* PlayerbotTestingAI::GetMeleeDamageDoneInfo(Unit const* target) const
{
    auto infoForVictimItr = meleeDamageDone.find(target->GetGUID());
    if (infoForVictimItr == meleeDamageDone.end())
        return nullptr;

    return &(*infoForVictimItr).second;
}

std::vector<PlayerbotTestingAI::HealingDoneInfo> const* PlayerbotTestingAI::GetHealingDoneInfo(Unit const* target) const
{
    auto infoForVictimItr = healingDone.find(target->GetGUID());
    if (infoForVictimItr == healingDone.end())
        return nullptr;

    return &(*infoForVictimItr).second;
}

void PlayerbotTestingAI::ResetSpellCounters()
{
    TC_LOG_TRACE("test.unit_test", "PlayerbotTestingAI: Counters were reset");
    spellDamageDone.clear();
    meleeDamageDone.clear();
    healingDone.clear();
    ticksDone.clear();
}