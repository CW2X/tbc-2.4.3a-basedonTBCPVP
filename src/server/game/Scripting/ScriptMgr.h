/* Copyright (C) 2008 Trinity <http://www.trinitycore.org/>
 *
 * Thanks to the original authors: ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 *
 * This program is free software licensed under GPL version 2
 * Please see the included DOCS/LICENSE.TXT for more information */

#ifndef SCRIPTMGR_H
#define SCRIPTMGR_H

#include "QuestDef.h"
#include "ObjectMgr.h"
#include "DBCStores.h"

class WorldSocket;
class WorldSession;
class WorldPacket;

#ifndef _TRINITY_SCRIPT_CONFIG
# define _TRINITY_SCRIPT_CONFIG  "worldserver.conf"
#endif // _TRINITY_SCRIPT_CONFIG

class Player;
class Creature;
class CreatureAI;
class GameObject;
class GameObjectAI;
class InstanceScript;
class ChatCommand;
class Quest;
class Item;
class SpellCastTargets;
class Map;
class Unit;
class WorldObject;
class MotionTransport;
class Guild;
class Group;
class SpellInfo;
class Spell;
struct AreaTriggerEntry;
struct AreaTableEntry;
enum ShutdownExitCode : int;
enum ShutdownMask : int;
struct SpellSummary;
class ScriptMgr;
class SpellScript;
class AuraScript;
class ModuleReference;
class TestCase;
class OutdoorPvP;

class TC_GAME_API ScriptObject
{
    friend class ScriptMgr;

    public:

        const std::string& GetName() const { return _name; }

    protected:

        ScriptObject(const char* name);
        virtual ~ScriptObject();

    private:

        const std::string _name;
};

template<class TObject> class UpdatableScript
{
protected:

    UpdatableScript()
    = default;
    virtual ~UpdatableScript() { }

public:

    virtual void OnUpdate(TObject* /*obj*/, uint32 /*diff*/) { }
};

class TC_GAME_API SpellScriptLoader : public ScriptObject
{
protected:

    SpellScriptLoader(const char* name);

public:

    // Should return a fully valid SpellScript pointer.
    virtual SpellScript* GetSpellScript() const { return nullptr; }

    // Should return a fully valid AuraScript pointer.
    virtual AuraScript* GetAuraScript() const { return nullptr; }
};

template<class TMap> 
class TC_GAME_API MapScript : public UpdatableScript<TMap>
{
    MapEntry const* _mapEntry;

protected:

    MapScript(MapEntry const* mapEntry) : _mapEntry(mapEntry) { }

public:

    // Gets the MapEntry structure associated with this script. Can return NULL.
    MapEntry const* GetEntry() { return _mapEntry; }

    // Called when the map is created.
    virtual void OnCreate(TMap* /*map*/) { }

    // Called just before the map is destroyed.
    virtual void OnDestroy(TMap* /*map*/) { }

    // Called when a grid map is loaded.
    virtual void OnLoadGridMap(TMap* /*map*/, GridMap* /*gmap*/, uint32 /*gx*/, uint32 /*gy*/) { }

    // Called when a grid map is unloaded.
    virtual void OnUnloadGridMap(TMap* /*map*/, GridMap* /*gmap*/, uint32 /*gx*/, uint32 /*gy*/) { }

    // Called when a player enters the map.
    virtual void OnPlayerEnter(TMap* /*map*/, Player* /*player*/) { }

    // Called when a player leaves the map.
    virtual void OnPlayerLeave(TMap* /*map*/, Player* /*player*/) { }

    // Called on every map update tick.
    void OnUpdate(TMap* /*map*/, uint32 /*diff*/) override { }
};

class TC_GAME_API WorldMapScript : public ScriptObject, public MapScript<Map>
{
protected:

    WorldMapScript(const char* name, uint32 mapId);
};

class TC_GAME_API InstanceMapScript : public ScriptObject, public MapScript<InstanceMap>
{
protected:

    InstanceMapScript(const char* name, uint32 mapId);

public:

    // Gets an InstanceScript object for this instance.
    virtual InstanceScript* GetInstanceScript(InstanceMap* /*map*/) const { return nullptr; }
};

class TC_GAME_API BattlegroundMapScript : public ScriptObject, public MapScript<BattlegroundMap>
{
protected:

    BattlegroundMapScript(const char* name, uint32 mapId);
};

class TC_GAME_API ItemScript : public ScriptObject
{
protected:

    ItemScript(const char* name);

public:

    // Called when a player accepts a quest from the item.
    virtual bool OnQuestAccept(Player* /*player*/, Item* /*item*/, Quest const* /*quest*/) { return false; }

    // Called when a player uses the item.
    virtual bool OnUse(Player* /*player*/, Item* /*item*/, SpellCastTargets const& /*targets*/) { return false; }

    // Called when the item expires (is destroyed).
    virtual bool OnExpire(Player* /*player*/, ItemTemplate const* /*proto*/) { return false; }
};

class TC_GAME_API CreatureScript : public ScriptObject, public UpdatableScript<Creature>
{
protected:

    CreatureScript(const char* name);

public:
    ~CreatureScript() override;

    // Called when a CreatureAI object is needed for the creature.
    virtual CreatureAI* GetAI(Creature* creature) const { return NULL; }
};

class TC_GAME_API GameObjectScript : public ScriptObject, public UpdatableScript<GameObject>
{
protected:

    GameObjectScript(const char* name);

public:

    // Called when a GameObjectAI object is needed for the gameobject.
    virtual GameObjectAI* GetAI(GameObject* /*go*/) const { return nullptr; }
};

class TC_GAME_API AreaTriggerScript : public ScriptObject
{
protected:

    AreaTriggerScript(const char* name);

public:

    // Called when the area trigger is activated by a player.
    virtual bool OnTrigger(Player* /*player*/, AreaTriggerEntry const* /*trigger*/) { return false; }
};

class TC_GAME_API TestCaseScript : public ScriptObject, public UpdatableScript<TestCase>
{
protected:

    TestCaseScript(const char* name);

public:

#ifdef TESTS
    // Called when loading tests
    virtual std::unique_ptr<TestCase> GetTest() const;
#endif
};

class TC_GAME_API OutdoorPvPScript : public ScriptObject
{
protected:

    OutdoorPvPScript(char const* name);

public:

    // Should return a fully valid OutdoorPvP object for the type ID.
    virtual OutdoorPvP* GetOutdoorPvP() const = 0;
};

class TC_GAME_API CommandScript : public ScriptObject
{
protected:

    CommandScript(char const* name);

public:

    // Should return a pointer to a valid command table (ChatCommand array) to be used by ChatHandler.
    virtual std::vector<ChatCommand> GetCommands() const = 0;
};

class TC_GAME_API PlayerScript : public ScriptObject
{
protected:
    PlayerScript(const char* name);

public:

    virtual void OnPlayerRealeasedGhost(Player* /*player*/) {}

    //Called when player kills another Player
    virtual void OnPVPKill(Player* /*killer*/, Player* /*killed*/) {}

    // Called when Player kills a creature
            // Called when a player kills a creature
    virtual void OnCreatureKill(Player* /*killer*/, Creature* /*killed*/) { }

    // Called when a player's pet kills a creature
    virtual void OnCreatureKilledByPet(Player* /*PetOwner*/, Creature* /*killed*/) { }

    // Called when a player is killed by a creature
    virtual void OnPlayerKilledByCreature(Creature* /*killer*/, Player* /*killed*/) { }

    // Called when a player's level changes (right after the level is applied)
    virtual void OnLevelChanged(Player* /*player*/, uint8 /*oldlevel*/) { }

    // Called when a player's free talent points change (right before the change is applied)
    virtual void OnFreeTalentPointsChanged(Player* /*player*/, uint32 /*points*/) { }

    // Called when a player's talent points are reset (right before the reset is done)
    virtual void OnTalentsReset(Player* /*player*/, bool /*noCost*/) { }

    // Called for player::update
    virtual void OnBeforeUpdate(Player* /*player*/, uint32 /*p_time*/) { }

    // Called when a player's money is modified (before the modification is done)
    virtual void OnMoneyChanged(Player* /*player*/, int32& /*amount*/) { }

    // Called when a player gains XP (before anything is given)
    virtual void OnGiveXP(Player* /*player*/, uint32& /*amount*/, Unit* /*victim*/) { }

    // Called when a player's reputation changes (before it is actually changed)
    virtual void OnReputationChange(Player* /*player*/, uint32 /*factionId*/, int32& /*standing*/, bool /*incremental*/) { }

    // Called when a duel is requested
    virtual void OnDuelRequest(Player* /*target*/, Player* /*challenger*/) { }

    // Called when a duel starts (after 3s countdown)
    virtual void OnDuelStart(Player* /*player1*/, Player* /*player2*/) { }

    // Called when a duel ends
    virtual void OnDuelEnd(Player* /*winner*/, Player* /*loser*/, DuelCompleteType /*type*/) { }

    // The following methods are called when a player sends a chat message.
    virtual void OnChat(Player* /*player*/, uint32 /*type*/, uint32 /*lang*/, std::string& /*msg*/) { }

    virtual void OnChat(Player* /*player*/, uint32 /*type*/, uint32 /*lang*/, std::string& /*msg*/, Player* /*receiver*/) { }

    virtual void OnChat(Player* /*player*/, uint32 /*type*/, uint32 /*lang*/, std::string& /*msg*/, Group* /*group*/) { }

    virtual void OnChat(Player* /*player*/, uint32 /*type*/, uint32 /*lang*/, std::string& /*msg*/, Guild* /*guild*/) { }

    // Both of the below are called on emote opcodes.
    virtual void OnEmote(Player* /*player*/, uint32 /*emote*/) { }

    virtual void OnTextEmote(Player* /*player*/, uint32 /*textEmote*/, uint32 /*emoteNum*/, uint64 /*guid*/) { }

    // Called in Spell::Cast.
    virtual void OnSpellCast(Player* /*player*/, Spell* /*spell*/, bool /*skipCheck*/) { }

    // Called during data loading
    virtual void OnLoadFromDB(Player* /*player*/) { }

    // Called when a player logs in.
    virtual void OnLogin(Player* /*player*/) { }

    // Called when a player logs out.
    virtual void OnLogout(Player* /*player*/) { }

    // Called when a player is created.
    virtual void OnCreate(Player* /*player*/) { }

    // Called when a player is deleted.
    virtual void OnDelete(uint64 /*guid*/) { }

    // Called when a player is about to be saved.
    virtual void OnSave(Player* /*player*/) { }

    // Called when a player is bound to an instance
    virtual void OnBindToInstance(Player* /*player*/, Difficulty /*difficulty*/, uint32 /*mapId*/, bool /*permanent*/) { }

    // Called when a player switches to a new zone
    virtual void OnUpdateZone(Player* /*player*/, uint32 /*newZone*/, uint32 /*newArea*/) { }

    // Called when a player switches to a new area (more accurate than UpdateZone)
    virtual void OnUpdateArea(Player* /*player*/, uint32 /*oldArea*/, uint32 /*newArea*/) { }

    // Called when a player changes to a new map (after moving to new map)
    virtual void OnMapChanged(Player* /*player*/) { }

    // Called before a player is being teleported to new coords
    virtual bool OnBeforeTeleport(Player* /*player*/, uint32 /*mapid*/, float /*x*/, float /*y*/, float /*z*/, float /*orientation*/, uint32 /*options*/, Unit* /*target*/) { return true; }

    // Called when team/faction is set on player
    virtual void OnUpdateFaction(Player* /*player*/) { }

    // Called when a player is added to battleground
    virtual void OnAddToBattleground(Player* /*player*/, Battleground* /*bg*/) { }

    // Called when a player is removed from battleground
    virtual void OnRemoveFromBattleground(Player* /*player*/, Battleground* /*bg*/) { }

    // Called when a player selects an option in a player gossip window
    virtual void OnGossipSelect(Player* /*player*/, uint32 /*menu_id*/, uint32 /*sender*/, uint32 /*action*/) { }

    // Called when a player selects an option in a player gossip window
    virtual void OnGossipSelectCode(Player* /*player*/, uint32 /*menu_id*/, uint32 /*sender*/, uint32 /*action*/, const char* /*code*/) { }

    // On player getting charmed
    virtual void OnBeingCharmed(Player* /*player*/, Unit* /*charmer*/, uint32 /*oldFactionId*/, uint32 /*newFactionId*/) { }

    // To change behaviour of set visible item slot
    virtual void OnAfterSetVisibleItemSlot(Player* /*player*/, uint8 /*slot*/, Item* /*item*/) { }

    // After an item has been moved from inventory
    virtual void OnAfterMoveItemFromInventory(Player* /*player*/, Item* /*it*/, uint8 /*bag*/, uint8 /*slot*/, bool /*update*/) { }

    // After an item has been equipped
    virtual void OnEquip(Player* /*player*/, Item* /*it*/, uint8 /*bag*/, uint8 /*slot*/, bool /*update*/) { }

    // After player enters queue for BG
    virtual void OnPlayerJoinBG(Player* /*player*/) { }

    // After player enters queue for Arena
    virtual void OnPlayerJoinArena(Player* /*player*/) { }

    //After looting item
    virtual void OnLootItem(Player* /*player*/, Item* /*item*/, uint32 /*count*/, uint64 /*lootguid*/) { }

    //After creating item (eg profession item creation)
    virtual void OnCreateItem(Player* /*player*/, Item* /*item*/, uint32 /*count*/) { }

    //After receiving item as a quest reward
    virtual void OnQuestRewardItem(Player* /*player*/, Item* /*item*/, uint32 /*count*/) { }

    //Before buying something from any vendor
    virtual void OnBeforeBuyItemFromVendor(Player* /*player*/, uint64 /*vendorguid*/, uint32 /*vendorslot*/, uint32 &/*item*/, uint8 /*count*/, uint8 /*bag*/, uint8 /*slot*/) { };

    //Before buying something from any vendor
    virtual void OnAfterStoreOrEquipNewItem(Player* /*player*/, uint32 /*vendorslot*/, uint32& /*item*/, uint8 /*count*/, uint8 /*bag*/, uint8 /*slot*/, ItemTemplate const* /*pProto*/, Creature* /*pVendor*/, VendorItem const* /*crItem*/, bool /*bStore*/) { };

    virtual void OnAfterUpdateMaxPower(Player* /*player*/, Powers& /*power*/, float& /*value*/) { }

    virtual void OnAfterUpdateMaxHealth(Player* /*player*/, float& /*value*/) { }

    virtual void OnBeforeUpdateAttackPowerAndDamage(Player* /*player*/, float& /*level*/, float& /*val2*/, bool /*ranged*/) { }
    virtual void OnAfterUpdateAttackPowerAndDamage(Player* /*player*/, float& /*level*/, float& /*base_attPower*/, float& /*attPowerMod*/, float& /*attPowerMultiplier*/, bool /*ranged*/) { }

    virtual void OnBeforeInitTalentForLevel(Player* /*player*/, uint8& /*level*/, uint32& /*talentPointsForLevel*/) { }

    virtual void OnFirstLogin(Player* /*player*/) { }
};

// Manages registration, loading, and execution of scripts.
class TC_GAME_API ScriptMgr
{
    private:
        ScriptMgr();
        ~ScriptMgr();

    public:
        static ScriptMgr* instance()
        {
            static ScriptMgr instance;
            return &instance;
        }

        void Initialize(char const* cfg_file);
        void LoadDatabase();

        /* Add given script to m_script list if the script is used in database. DO NOT use script afterwards as this function may delete it.
          Pointer script* will be freed upon ScriptMgr deletion, or immediately if registering failed
        **/
        void FillSpellSummary();

        void IncreaseScriptCount() { ++_scriptCount; }
        void DecreaseScriptCount() { --_scriptCount; }

        uint32 GetScriptCount() const { return _scriptCount; }

        typedef void(*ScriptLoaderCallbackType)();

        /// Sets the script loader callback which is invoked to load scripts
        /// (Workaround for circular dependency game <-> scripts)
        void SetScriptLoader(ScriptLoaderCallbackType script_loader_callback)
        {
            _script_loader_callback = script_loader_callback;
        }

    public:

public: /* Script contexts */
        /// Set the current script context, which allows the ScriptMgr
        /// to accept new scripts in this context.
        /// Requires a SwapScriptContext() call afterwards to load the new scripts.
        void SetScriptContext(std::string const& context);
        /// Returns the current script context.
        std::string const& GetCurrentScriptContext() const { return _currentContext; }
        /// Releases all scripts associated with the given script context immediately.
        /// Requires a SwapScriptContext() call afterwards to finish the unloading.
        void ReleaseScriptContext(std::string const& context);
        /// Executes all changed introduced by SetScriptContext and ReleaseScriptContext.
        /// It is possible to combine multiple SetScriptContext and ReleaseScriptContext
        /// calls for better performance (bulk changes).
        void SwapScriptContext(bool initialize = false);

        /// Returns the context name of the static context provided by the worldserver
        static std::string const& GetNameOfStaticContext();

        /// Acquires a strong module reference to the module containing the given script name,
        /// which prevents the shared library which contains the script from unloading.
        /// The shared library is lazy unloaded as soon as all references to it are released.
        std::shared_ptr<ModuleReference> AcquireModuleReferenceOfScriptName(
            std::string const& scriptname) const;

        std::unordered_multimap<std::string /*context*/, std::unique_ptr<TestCaseScript>> const& GetAllTests() const;

    public: /* Unloading */

        void Unload();

    public: /* SpellScriptLoader */

		void CreateSpellScripts(uint32 spellId, std::vector<SpellScript*>& scriptVector, Spell* invoker) const;
        void CreateAuraScripts(uint32 spellId, std::vector<AuraScript*>& scriptVector, Aura* invoker) const;
        SpellScriptLoader* GetSpellScriptLoader(uint32 scriptId);

    public: /* AccountScript */

        /*
        void OnAccountLogin(uint32 accountId);
        void OnFailedAccountLogin(uint32 accountId);
        void OnEmailChange(uint32 accountId);
        void OnFailedEmailChange(uint32 accountId);
        void OnPasswordChange(uint32 accountId);
        void OnFailedPasswordChange(uint32 accountId);
       */

    public: /* GuildScript */

        /*
        void OnGuildAddMember(Guild* guild, Player* player, uint8& plRank);
        void OnGuildRemoveMember(Guild* guild, Player* player, bool isDisbanding, bool isKicked);
        void OnGuildMOTDChanged(Guild* guild, const std::string& newMotd);
        void OnGuildInfoChanged(Guild* guild, const std::string& newInfo);
        void OnGuildCreate(Guild* guild, Player* leader, const std::string& name);
        void OnGuildDisband(Guild* guild);
        void OnGuildMemberWitdrawMoney(Guild* guild, Player* player, uint32 &amount, bool isRepair);
        void OnGuildMemberDepositMoney(Guild* guild, Player* player, uint32 &amount);
        void OnGuildItemMove(Guild* guild, Player* player, Item* pItem, bool isSrcBank, uint8 srcContainer, uint8 srcSlotId,
            bool isDestBank, uint8 destContainer, uint8 destSlotId);
        void OnGuildEvent(Guild* guild, uint8 eventType, ObjectGuid::LowType playerGuid1, ObjectGuid::LowType playerGuid2, uint8 newRank);
        void OnGuildBankEvent(Guild* guild, uint8 eventType, uint8 tabId, ObjectGuid::LowType playerGuid, uint32 itemOrMoney, uint16 itemStackCount, uint8 destTabId);
        */

    public: /* GroupScript */

        /*
        void OnGroupAddMember(Group* group, ObjectGuid guid);
        void OnGroupInviteMember(Group* group, ObjectGuid guid);
//        void OnGroupRemoveMember(Group* group, ObjectGuid guid, RemoveMethod method, uint64 kicker, const char* reason);
        void OnGroupChangeLeader(Group* group, ObjectGuid newLeaderGuid, ObjectGuid oldLeaderGuid);
        void OnGroupDisband(Group* group);
        */

    public: /* MapScript */
        void OnCreateMap(Map* map);
        void OnDestroyMap(Map* map);
        void OnLoadGridMap(Map* map, GridMap* gmap, uint32 gx, uint32 gy);
        void OnUnloadGridMap(Map* map, GridMap* gmap, uint32 gx, uint32 gy);
        void OnPlayerEnterMap(Map* map, Player* player);
        void OnPlayerLeaveMap(Map* map, Player* player);
        void OnMapUpdate(Map* map, uint32 diff);

    public: /* InstanceMapScript */

        InstanceScript* CreateInstanceData(InstanceMap* map);

    public: /* TransportScript */

        void OnAddPassenger(Transport* transport, Player* player);
        void OnAddCreaturePassenger(Transport* transport, Creature* creature);
        void OnRemovePassenger(Transport* transport, Player* player);
        void OnTransportUpdate(Transport* transport, uint32 diff);
        void OnRelocate(Transport* transport, uint32 waypointId, uint32 mapId, float x, float y, float z);

    public: /* PlayerScript */

        void OnPVPKill(Player* killer, Player* killed);
        void OnCreatureKill(Player* killer, Creature* killed);
        void OnPlayerKilledByCreature(Creature* killer, Player* killed);
        void OnPlayerLevelChanged(Player* player, uint8 oldLevel);
        void OnPlayerFreeTalentPointsChanged(Player* player, uint32 newPoints);
        void OnPlayerTalentsReset(Player* player, bool noCost);
        void OnPlayerMoneyChanged(Player* player, int32& amount);
        void OnGivePlayerXP(Player* player, uint32& amount, Unit* victim);
        void OnPlayerReputationChange(Player* player, uint32 factionID, int32& standing, bool incremental);
        void OnPlayerDuelRequest(Player* target, Player* challenger);
        void OnPlayerDuelStart(Player* player1, Player* player2);
        void OnPlayerDuelEnd(Player* winner, Player* loser, DuelCompleteType type);
        void OnPlayerChat(Player* player, uint32 type, uint32 lang, std::string& msg);
        void OnPlayerChat(Player* player, uint32 type, uint32 lang, std::string& msg, Player* receiver);
        void OnPlayerChat(Player* player, uint32 type, uint32 lang, std::string& msg, Group* group);
        void OnPlayerChat(Player* player, uint32 type, uint32 lang, std::string& msg, Guild* guild);        
        void OnPlayerEmote(Player* player, uint32 emote);
        void OnPlayerTextEmote(Player* player, uint32 textEmote, uint32 emoteNum, ObjectGuid guid);
        void OnPlayerSpellCast(Player* player, Spell* spell, bool skipCheck);
        void OnPlayerLogin(Player* player, bool firstLogin);
        void OnPlayerLogout(Player* player);
        void OnPlayerCreate(Player* player);
        void OnPlayerDelete(ObjectGuid guid, uint32 accountId);
        void OnPlayerSave(Player* player);
        void OnPlayerBindToInstance(Player* player, Difficulty difficulty, uint32 mapid, bool permanent);
        void OnPlayerUpdateZone(Player* player, uint32 newZone, uint32 newArea);
        
    public: /* ServerScript */

        /*
        void OnNetworkStart();
        void OnNetworkStop();
        void OnSocketOpen(std::shared_ptr<WorldSocket> socket);
        void OnSocketClose(std::shared_ptr<WorldSocket> socket);
        void OnPacketReceive(WorldSession* session, WorldPacket const& packet);
        void OnPacketSend(WorldSession* session, WorldPacket const& packet);
        void OnUnknownPacketReceive(WorldSession* session, WorldPacket const& packet);
        */
    public: /* WorldScript */
        /*
        void OnOpenStateChange(bool open) {}
        void OnConfigLoad(bool reload) {}
        void OnMotdChange(std::string& newMotd) {}
        void OnShutdownInitiate(ShutdownExitCode code, ShutdownMask mask) {}
        void OnShutdownCancel() {}
        void OnWorldUpdate(uint32 diff) {}
        void OnStartup() {}
        void OnShutdown() {}
        */
    public: /* ItemScript */

        bool OnQuestAccept(Player* player, Item* item, Quest const* quest);
        bool OnItemUse(Player* player, Item* item, SpellCastTargets const& targets);
        bool OnItemExpire(Player* player, ItemTemplate const* proto);

    public: /* CreatureScript */

        CreatureAI* GetCreatureAI(Creature* creature);

    public: /* GameObjectScript */

        GameObjectAI* GetGameObjectAI(GameObject* go);

    public: /* AreaTriggerScript */

        bool OnAreaTrigger(Player* player, AreaTriggerEntry const* trigger);

    public: /* OutdoorPvPScript */

        OutdoorPvP * CreateOutdoorPvP(uint32 scriptId);

    public: /* CommandScript */

        std::vector<ChatCommand> GetChatCommands();

    private:

        uint32 _scriptCount;

        ScriptLoaderCallbackType _script_loader_callback;

        std::string _currentContext;
};

template <class S>
class GenericSpellScriptLoader : public SpellScriptLoader
{
public:
    GenericSpellScriptLoader(char const* name) : SpellScriptLoader(name) { }
    SpellScript* GetSpellScript() const override { return new S(); }
};
#define RegisterSpellScript(spell_script) new GenericSpellScriptLoader<spell_script>(#spell_script)

template <class A>
class GenericAuraScriptLoader : public SpellScriptLoader
{
public:
    GenericAuraScriptLoader(char const* name) : SpellScriptLoader(name) { }
    AuraScript* GetAuraScript() const override { return new A(); }
};
#define RegisterAuraScript(aura_script) new GenericAuraScriptLoader<aura_script>(#aura_script)

template <class S, class A>
class GenericSpellAndAuraScriptLoader : public SpellScriptLoader
{
public:
    GenericSpellAndAuraScriptLoader(char const* name) : SpellScriptLoader(name) { }
    SpellScript* GetSpellScript() const override { return new S(); }
    AuraScript* GetAuraScript() const override { return new A(); }
};
#define RegisterSpellAndAuraScriptPair(spell_script, aura_script) new GenericSpellAndAuraScriptLoader<spell_script, aura_script>(#spell_script)

template <class AI>
class GenericCreatureScript : public CreatureScript
{
public:
    GenericCreatureScript(char const* name) : CreatureScript(name) { }
    CreatureAI* GetAI(Creature* me) const override { return new AI(me); }
};
#define RegisterCreatureAI(ai_name) new GenericCreatureScript<ai_name>(#ai_name)

template <class AI, AI*(*AIFactory)(Creature*)>
class FactoryCreatureScript : public CreatureScript
{
public:
    FactoryCreatureScript(char const* name) : CreatureScript(name) { }
    CreatureAI* GetAI(Creature* me) const override { return AIFactory(me); }
};
#define RegisterCreatureAIWithFactory(ai_name, factory_fn) new FactoryCreatureScript<ai_name, &factory_fn>(#ai_name)

template <class AI>
class GenericGameObjectScript : public GameObjectScript
{
public:
    GenericGameObjectScript(char const* name) : GameObjectScript(name) { }
    GameObjectAI* GetAI(GameObject* go) const override { return new AI(go); }
};
#define RegisterGameObjectAI(ai_name) new GenericGameObjectScript<ai_name>(#ai_name)

#ifdef TESTS
template <class TestClass>
class GenericTestScript : public TestCaseScript
{
public:
    GenericTestScript(char const* name) : TestCaseScript(name) { }
    std::unique_ptr<TestCase> GetTest() const override { return std::make_unique<TestClass>(); }
};
#define RegisterTestCase(test_name, class_name) new GenericTestScript<class_name>(test_name)
#endif

//Generic scripting text function
void TC_GAME_API DoScriptText(int32 textEntry, Unit* pSource, Unit* target = nullptr);

#define sScriptMgr ScriptMgr::instance()

#endif

