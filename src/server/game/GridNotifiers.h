
#ifndef TRINITY_GRIDNOTIFIERS_H
#define TRINITY_GRIDNOTIFIERS_H

#include "UpdateData.h"
#include <iostream>

#include "Corpse.h"
#include "Object.h"
#include "DynamicObject.h"
#include "GameObject.h"
#include "Player.h"
#include "Unit.h"

//class Map;

namespace Trinity
{
	struct TC_GAME_API VisibleNotifier
	{
		Player &i_player;
		UpdateData i_data;
		std::set<Unit*> i_visibleNow;
		GuidUnorderedSet vis_guids;

		VisibleNotifier(Player &player) : i_player(player), vis_guids(player.m_clientGUIDs) { }
		template<class T> void Visit(GridRefManager<T> &m);
		void SendToSelf(void);
	};

	struct VisibleChangesNotifier
	{
		WorldObject &i_object;

		explicit VisibleChangesNotifier(WorldObject &object) : i_object(object) { }
		template<class T> void Visit(GridRefManager<T> &) { }
		void Visit(PlayerMapType &);
		void Visit(CreatureMapType &);
		void Visit(DynamicObjectMapType &);
	};

    struct TC_GAME_API PlayerRelocationNotifier : public VisibleNotifier
    {
        PlayerRelocationNotifier(Player &player) : VisibleNotifier(player) {}

        template<class T> inline void Visit(GridRefManager<T> &m) { VisibleNotifier::Visit(m); }
		void Visit(CreatureMapType &);
		void Visit(PlayerMapType &);
    };

	struct TC_GAME_API CreatureRelocationNotifier
	{
		Creature &i_creature;
		CreatureRelocationNotifier(Creature &c) : i_creature(c) { }
		template<class T> void Visit(GridRefManager<T> &) { }
		void Visit(CreatureMapType &);
		void Visit(PlayerMapType &);
	};

	struct TC_GAME_API DelayedUnitRelocation
	{
		Map &i_map;
		Cell &cell;
		CellCoord &p;
		const float i_radius;
		DelayedUnitRelocation(Cell &c, CellCoord &pair, Map &map, float radius) :
			i_map(map), cell(c), p(pair), i_radius(radius) { }
		template<class T> void Visit(GridRefManager<T> &) { }
		void Visit(CreatureMapType &);
		void Visit(PlayerMapType   &);
	};

	struct TC_GAME_API AIRelocationNotifier
	{ 
		Unit &i_unit;
		bool isCreature;
		explicit AIRelocationNotifier(Unit &unit) : i_unit(unit), isCreature(unit.GetTypeId() == TYPEID_UNIT) { }
		template<class T> void Visit(GridRefManager<T> &) { }
		void Visit(CreatureMapType &);
	};

    struct GridUpdater
    {
        GridType &i_grid;
        uint32 i_timeDiff;
        GridUpdater(GridType &grid, uint32 diff) : i_grid(grid), i_timeDiff(diff) {}

        template<class T> void updateObjects(GridRefManager<T> &m)
        {
            for(typename GridRefManager<T>::iterator iter = m.begin(); iter != m.end(); ++iter)
                iter->GetSource()->Update(i_timeDiff);
        }

        void Visit(PlayerMapType &m) { updateObjects<Player>(m); }
        void Visit(CreatureMapType &m){ updateObjects<Creature>(m); }
        void Visit(GameObjectMapType &m) { updateObjects<GameObject>(m); }
        void Visit(DynamicObjectMapType &m) { updateObjects<DynamicObject>(m); }
        void Visit(CorpseMapType &m) { updateObjects<Corpse>(m); }
    };

	struct TC_GAME_API MessageDistDeliverer
	{
		WorldObject* i_source;
		WorldPacket const* i_message;
		uint32 i_phaseMask;
		float i_distSq;
		Team team;
		Player const* skipped_receiver;
		MessageDistDeliverer(WorldObject* src, WorldPacket const* msg, float dist, bool own_team_only = false, Player const* skipped = NULL)
			: i_source(src), i_message(msg), i_phaseMask(src->GetPhaseMask()), i_distSq(dist * dist)
			, team(Team(0))
			, skipped_receiver(skipped)
		{
			if (own_team_only)
				if (Player* player = src->ToPlayer())
					team = Team(player->GetTeam());
		}
		void Visit(PlayerMapType &m);
		void Visit(CreatureMapType &m);
		void Visit(DynamicObjectMapType &m);
		template<class SKIP> void Visit(GridRefManager<SKIP> &) {}

		void SendPacket(Player* player)
		{
			// never send packet to self
			if (player == i_source || (team != Team(0) && Team(player->GetTeam()) != team) || skipped_receiver == player)
				return;

			if (!player->HaveAtClient(i_source))
				return;

			player->GetSession()->SendPacket(i_message);
		}
	};

    struct TC_GAME_API ObjectUpdater
    {
        uint32 i_timeDiff;
        explicit ObjectUpdater(const uint32 &diff) : i_timeDiff(diff) {}
        template<class T> void Visit(GridRefManager<T> &m);
        void Visit(PlayerMapType &) {}
        void Visit(CorpseMapType &) {}
        void Visit(CreatureMapType &);
    };

    // SEARCHERS & LIST SEARCHERS & WORKERS

    // WorldObject searchers & workers

    // Generic base class to insert elements into arbitrary containers using push_back
    template<typename Type>
    class ContainerInserter {
        using InserterType = void(*)(void*, Type&&);

        void* ref;
        InserterType inserter;

        // MSVC workaround
        template<typename T>
        static void InserterOf(void* ref, Type&& type)
        {
            static_cast<T*>(ref)->push_back(std::move(type));
        }

    protected:
        template<typename T>
        ContainerInserter(T& ref_) : ref(&ref_), inserter(&InserterOf<T>) { }

        void Insert(Type type)
        {
            inserter(ref, std::move(type));
        }
    };

    template<class Check>
    struct WorldObjectSearcher
    {
        uint32 i_mapTypeMask;
        uint32 i_phaseMask; //Not yet used
        WorldObject* &i_object;
        Check &i_check;

        WorldObjectSearcher(WorldObject const* searcher, WorldObject* & result, Check& check, uint32 mapTypeMask = GRID_MAP_TYPE_MASK_ALL) 
            : i_mapTypeMask(mapTypeMask), i_phaseMask(searcher->GetPhaseMask()), i_object(result),i_check(check) {}

        void Visit(GameObjectMapType &m);
        void Visit(PlayerMapType &m);
        void Visit(CreatureMapType &m);
        void Visit(CorpseMapType &m);
        void Visit(DynamicObjectMapType &m);

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED> &) {}
    };

    template<class Check>
    struct WorldObjectLastSearcher
    {
        uint32 i_mapTypeMask;
        uint32 i_phaseMask;
        WorldObject* &i_object;
        Check &i_check;

        WorldObjectLastSearcher(WorldObject const* searcher, WorldObject* & result, Check& check, uint32 mapTypeMask = GRID_MAP_TYPE_MASK_ALL)
            : i_mapTypeMask(mapTypeMask), i_phaseMask(searcher->GetPhaseMask()), i_object(result), i_check(check) {}

        void Visit(GameObjectMapType &m);
        void Visit(PlayerMapType &m);
        void Visit(CreatureMapType &m);
        void Visit(CorpseMapType &m);
        void Visit(DynamicObjectMapType &m);

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED> &) {}
    };

    template<class Check>
    struct WorldObjectListSearcher : ContainerInserter<WorldObject*>
    {
        uint32 i_mapTypeMask;
        uint32 i_phaseMask; //not used yet
        Check& i_check;

        template<typename Container>
        WorldObjectListSearcher(WorldObject const* searcher, Container& container, Check & check, uint32 mapTypeMask = GRID_MAP_TYPE_MASK_ALL) :
            ContainerInserter<WorldObject*>(container),
            i_phaseMask(searcher->GetPhaseMask()), i_check(check), i_mapTypeMask(mapTypeMask) {}

        void Visit(PlayerMapType &m);
        void Visit(CreatureMapType &m);
        void Visit(CorpseMapType &m);
        void Visit(GameObjectMapType &m);
        void Visit(DynamicObjectMapType &m);

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED> &) {}
    };

    template<class Do>
    struct WorldObjectWorker
    {
        uint32 i_mapTypeMask;
        Do const& i_do;

        explicit WorldObjectWorker(WorldObject const* searcher, Do const& _do, uint32 mapTypeMask = GRID_MAP_TYPE_MASK_ALL) : 
            i_do(_do), i_mapTypeMask(mapTypeMask) {}

        void Visit(GameObjectMapType &m)
        {
            if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_GAMEOBJECT))
                return;
            for(auto & itr : m)
                i_do(itr.GetSource());
        }

        void Visit(PlayerMapType &m)
        {
            if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_PLAYER))
                return;
            for(auto & itr : m)
                i_do(itr.GetSource());
        }
        void Visit(CreatureMapType &m)
        {
            if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_CREATURE))
                return;
            for(auto & itr : m)
                i_do(itr.GetSource());
        }

        void Visit(CorpseMapType &m)
        {
            if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_CORPSE))
                return;
            for(auto & itr : m)
                i_do(itr.GetSource());
        }

        void Visit(DynamicObjectMapType &m)
        {
            if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_DYNAMICOBJECT))
                return;
            for(auto & itr : m)
                i_do(itr.GetSource());
        }

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED> &) {}
    };

    // Gameobject searchers

    template<class Check>
    struct GameObjectSearcher
    {
        uint32 i_phaseMask;
        GameObject* &i_object;
        Check &i_check;

        GameObjectSearcher(WorldObject const* searcher, GameObject* & result, Check& check)
            : i_phaseMask(searcher->GetPhaseMask()), i_object(result), i_check(check) { }

        void Visit(GameObjectMapType &m);

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED> &) { }
    };

    // Last accepted by Check GO if any (Check can change requirements at each call)
    template<class Check>
    struct GameObjectLastSearcher
    {
        uint32 i_phaseMask;
        GameObject* &i_object;
        Check& i_check;

        GameObjectLastSearcher(WorldObject const* searcher, GameObject* & result, Check& check)
            : i_phaseMask(searcher->GetPhaseMask()), i_object(result), i_check(check) { }

        void Visit(GameObjectMapType &m);

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED> &) { }
    };

    template<class Check>
    struct GameObjectListSearcher : ContainerInserter<GameObject*>
    {
        uint32 i_phaseMask;
        Check& i_check;

        template<typename Container>
        GameObjectListSearcher(WorldObject const* searcher, Container& container, Check & check)
            : ContainerInserter<GameObject*>(container),
            i_phaseMask(searcher->GetPhaseMask()), i_check(check) { }

        void Visit(GameObjectMapType &m);

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED> &) { }
    };

    // Unit searchers

    // First accepted by Check Unit if any
    template<class Check>
    struct UnitSearcher
    {
		uint32 i_phaseMask;
        Unit* &i_object;
        Check & i_check;

        UnitSearcher(WorldObject const* searcher, Unit* & result, Check & check) : i_phaseMask(searcher->GetPhaseMask()), i_object(result),i_check(check) {}

        void Visit(CreatureMapType &m);
        void Visit(PlayerMapType &m);

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED> &) {}
    };

    // Last accepted by Check Unit if any (Check can change requirements at each call)
    template<class Check>
    struct UnitLastSearcher
    {
        uint32 i_phaseMask;
        Unit* &i_object;
        Check & i_check;

        UnitLastSearcher(WorldObject const* searcher, Unit* & result, Check & check)
            : i_phaseMask(searcher->GetPhaseMask()), i_object(result), i_check(check) { }

        void Visit(CreatureMapType &m);
        void Visit(PlayerMapType &m);

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED> &) { }
    };

    // All accepted by Check units if any
    template<class Check>
    struct UnitListSearcher : ContainerInserter<Unit*>
    {
        uint32 i_phaseMask;
        Check& i_check;

        template<typename Container>
        UnitListSearcher(WorldObject const* searcher, Container& container, Check& check)
            : ContainerInserter<Unit*>(container),
              i_phaseMask(searcher->GetPhaseMask()), i_check(check) { }

        void Visit(PlayerMapType &m);
        void Visit(CreatureMapType &m);

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED> &) { }
    };
    
    template<class Check>
    struct PlayerListSearcher : ContainerInserter<Player*>
    {
        uint32 i_phaseMask;
        Check& i_check;

        template<typename Container>
        PlayerListSearcher(WorldObject const* searcher, Container& container, Check & check)
            : ContainerInserter<Player*>(container),
            i_phaseMask(searcher->GetPhaseMask()), i_check(check) { }

        template<typename Container>
        PlayerListSearcher(uint32 phaseMask, Container& container, Check & check)
            : ContainerInserter<Player*>(container),
            i_phaseMask(phaseMask), i_check(check) { }

        void Visit(PlayerMapType &m);

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED> &) { }
    };

    // Creature searchers

    template<class Check>
    struct CreatureSearcher
    {
        uint32 i_phaseMask;
        Creature* &i_object;
        Check & i_check;

        CreatureSearcher(WorldObject const* searcher, Creature* & result, Check & check)
            : i_phaseMask(searcher->GetPhaseMask()), i_object(result), i_check(check) { }

        void Visit(CreatureMapType &m);

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED> &) { }
    };

    // Last accepted by Check Creature if any (Check can change requirements at each call)
    template<class Check>
    struct CreatureLastSearcher
    {
        uint32 i_phaseMask;
        Creature* &i_object;
        Check & i_check;

        CreatureLastSearcher(WorldObject const* searcher, Creature* & result, Check & check)
            : i_phaseMask(searcher->GetPhaseMask()), i_object(result), i_check(check) { }

        void Visit(CreatureMapType &m);

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED> &) { }
    };

    template<class Check>
    struct CreatureListSearcher : ContainerInserter<Creature*>
    {
        uint32 i_phaseMask;
        Check& i_check;

        template<typename Container>
        CreatureListSearcher(WorldObject const* searcher, Container& container, Check & check)
            : ContainerInserter<Creature*>(container),
              i_phaseMask(searcher->GetPhaseMask()), i_check(check) { }

        void Visit(CreatureMapType &m);

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED> &) { }
    };

    //checker must have argument : Creature*, float& range
    template<class Check>
    struct CreatureListSearcherWithRange
    {
        std::list< std::pair<Creature*,float> > &i_objects;
        Check& i_check;

        CreatureListSearcherWithRange(WorldObject const* /* searcher */, std::list< std::pair<Creature*,float> > &objects, Check & check) : i_objects(objects), i_check(check) {}

        void Visit(CreatureMapType &m);

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED> &) {}
    };

    // Player searchers

    template<class Check>
    struct PlayerSearcher
    {
        uint32 i_phaseMask;
        Player* &i_object;
        Check & i_check;

        PlayerSearcher(WorldObject const* searcher, Player* & result, Check & check)
            : i_phaseMask(searcher->GetPhaseMask()), i_object(result), i_check(check) { }

        void Visit(PlayerMapType &m);

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED> &) { }
    };

    template<class Check>
    struct PlayerLastSearcher
    {
        uint32 i_phaseMask;
        Player* &i_object;
        Check& i_check;

        PlayerLastSearcher(WorldObject const* searcher, Player*& result, Check& check) : i_phaseMask(searcher->GetPhaseMask()), i_object(result), i_check(check)
        {
        }

        void Visit(PlayerMapType& m);

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED> &) { }
    };

    template<class Do>
    struct PlayerWorker
    {
        Do& i_do;

        explicit PlayerWorker(Do& _do) : i_do(_do) {}

        void Visit(PlayerMapType &m)
        {
            for(auto & itr : m)
                i_do(itr.GetSource());
        }

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED> &) {}
    };

    template<class Do>
    struct PlayerDistWorker
    {
        WorldObject const* i_searcher;
        float i_dist;
        Do& i_do;

        PlayerDistWorker(WorldObject const* searcher, float _dist, Do& _do)
            : i_searcher(searcher), i_dist(_dist), i_do(_do) { }

        void Visit(PlayerMapType &m)
        {
            for (auto & itr : m)
                if (itr.GetSource()->InSamePhase(i_searcher) && itr.GetSource()->IsWithinDistInMap(i_searcher, i_dist))
                    i_do(itr.GetSource());
        }

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED> &) { }
    };
    
    template<class Do>
    struct CreatureWorker
    {
        uint32 i_phaseMask;
        Do& i_do;

        CreatureWorker(WorldObject const* searcher, Do& _do)
            : i_phaseMask(searcher->GetPhaseMask()), i_do(_do) { }

        void Visit(CreatureMapType &m)
        {
            for (CreatureMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
                if (itr->GetSource()->InSamePhase(i_phaseMask))
                    i_do(itr->GetSource());
        }

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED> &) { }
    };

    // CHECKS && DO classes

    // WorldObject do classes

    class TC_GAME_API RespawnDo
    {
        public:
            RespawnDo() = default;
            void operator()(Creature* u) const;
            void operator()(GameObject* u) const;
            void operator()(WorldObject*) const {}
            void operator()(Corpse*) const {}
    };

    class TC_GAME_API FactionDo
    {
        public:
            FactionDo(uint32 id, uint32 f) : i_entry(id), i_faction(f) {}
            void operator()(Creature* u) const;
            void operator()(GameObject*) const {}
            void operator()(WorldObject*) const {}
            void operator()(Corpse*) const {}
        private:
            uint32 i_entry, i_faction;
    };
    
    // WorldObject check classes

    //UNIT SEARCHERS
    
    class TC_GAME_API CannibalizeObjectCheck
    {
        public:
            CannibalizeObjectCheck(Unit* funit, float range) : i_funit(funit), i_range(range) {}
            bool operator()(Player* u);
            bool operator()(Corpse* u);
            bool operator()(Creature* u);
            template<class NOT_INTERESTED> bool operator()(NOT_INTERESTED*) { return false; }
        private:
            Unit* const i_funit;
            float i_range;
    };

    class TC_GAME_API AnyUnfriendlyUnitInObjectRangeCheck
    {
        public:
            AnyUnfriendlyUnitInObjectRangeCheck(WorldObject const* obj, Unit const* funit, float range) : i_obj(obj), i_funit(funit), i_range(range) {}
            bool operator()(Unit* u);
        private:
            WorldObject const* i_obj;
            Unit const* i_funit;
            float i_range;
    };

    class TC_GAME_API AnyUnfriendlyAoEAttackableUnitInObjectRangeCheck
    {
        public:
            AnyUnfriendlyAoEAttackableUnitInObjectRangeCheck(WorldObject const* obj, Unit const* funit, float range) : i_obj(obj), i_funit(funit), i_range(range) {}
            bool operator()(Unit* u);
        private:
            WorldObject const* i_obj;
            Unit const* i_funit;
            float i_range;
    };


    class TC_GAME_API AnyFriendlyUnitInObjectRangeCheck
    {
        public:
            AnyFriendlyUnitInObjectRangeCheck(WorldObject const* obj, Unit const* funit, float range, bool playerOnly = false, bool incOwnRadius = true, bool incTargetRadius = true)
                : i_obj(obj), i_funit(funit), i_range(range), i_playerOnly(playerOnly), i_incOwnRadius(incOwnRadius), i_incTargetRadius(incTargetRadius) { }
            
            bool operator()(Unit* u);
        private:
            WorldObject const* i_obj;
            Unit const* i_funit;
            float i_range;
            bool i_playerOnly;
            bool i_incOwnRadius;
            bool i_incTargetRadius;
    };

    class TC_GAME_API AnyFriendlyUnitInObjectRangeCheckWithRangeReturned
    {
        public:
            AnyFriendlyUnitInObjectRangeCheckWithRangeReturned(WorldObject const* obj, Unit const* funit, float range, bool playerOnly = false) : i_obj(obj), i_funit(funit), i_range(range), i_playerOnly(playerOnly) {}
            bool operator()(Unit* u, float& range);
        private:
            WorldObject const* i_obj;
            Unit const* i_funit;
            float i_range;
            bool i_playerOnly;
    };

    class TC_GAME_API NearestFriendlyUnitInObjectRangeCheck
    {
        public:
            NearestFriendlyUnitInObjectRangeCheck(WorldObject const* obj, Unit const* funit, float range, bool playerOnly = false, bool furthest = false) : i_obj(obj), i_funit(funit), i_range(range), i_minRange(0), i_playerOnly(playerOnly), i_furthest(furthest) {}
            bool operator()(Unit* u);
        private:
            WorldObject const* i_obj;
            Unit const* i_funit;
            float i_range, i_minRange;
            bool i_playerOnly, i_furthest;
    };
    
    class TC_GAME_API AnyGroupedUnitInObjectRangeCheck
    {
    public:
        AnyGroupedUnitInObjectRangeCheck(WorldObject const* obj, Unit const* funit, float range, bool raid, bool playerOnly = false, bool incOwnRadius = true, bool incTargetRadius = true)
            : _source(obj), _refUnit(funit), _range(range), _raid(raid), _playerOnly(playerOnly), i_incOwnRadius(incOwnRadius), i_incTargetRadius(incTargetRadius) { }

        bool operator()(Unit* u) const
        {
            if (_playerOnly && u->GetTypeId() != TYPEID_PLAYER)
                return false;

            if (_raid)
            {
                if (!_refUnit->IsInRaidWith(u))
                    return false;
            }
            else if (!_refUnit->IsInPartyWith(u))
                return false;

            if (_refUnit->IsHostileTo(u))
                return false;

            if (!u->IsAlive() || u->IsTotem()) //exclude totems
                return false;

            float searchRadius = _range;
            if (i_incOwnRadius)
                searchRadius += _source->GetCombatReach();
            if (i_incTargetRadius)
                searchRadius += u->GetCombatReach();

            return u->IsInMap(_source) && u->InSamePhase(_source) && u->IsWithinDoubleVerticalCylinder(_source, searchRadius, searchRadius);
        }

    private:
        WorldObject const* _source;
        Unit const* _refUnit;
        float _range;
        bool _raid;
        bool _playerOnly;
        bool i_incOwnRadius;
        bool i_incTargetRadius;
    };

    class TC_GAME_API AnyUnitInObjectRangeCheck
    {
        public:
            AnyUnitInObjectRangeCheck(WorldObject const* obj, float range) : i_obj(obj), i_range(range) {}
            bool operator()(Unit* u);
        private:
            WorldObject const* i_obj;
            float i_range;
    };

    // Success at unit in range, range update for next check (this can be use with UnitLastSearcher to find nearest unit)
    class TC_GAME_API NearestAttackableUnitInObjectRangeCheck
    {
        public:
            NearestAttackableUnitInObjectRangeCheck(WorldObject const* obj, Unit const* funit, float range) : i_obj(obj), i_funit(funit), i_range(range) {}
            bool operator()(Unit* u);
        private:
            WorldObject const* i_obj;
            Unit const* i_funit;
            float i_range;

            // prevent clone this object
            NearestAttackableUnitInObjectRangeCheck(NearestAttackableUnitInObjectRangeCheck const&) = delete;
            NearestAttackableUnitInObjectRangeCheck& operator=(NearestAttackableUnitInObjectRangeCheck const &_Right) = delete;
    };

    class TC_GAME_API AnyAoETargetUnitInObjectRangeCheck
    {
        public:
            AnyAoETargetUnitInObjectRangeCheck(WorldObject const* obj, Unit const* funit, float range, SpellInfo const* spellInfo = nullptr, bool incOwnRadius = true, bool incTargetRadius = true);
            bool operator()(Unit* u);
        private:
            WorldObject const* i_obj;
            Unit const* i_funit;
            SpellInfo const* _spellInfo;
            float i_range;
            bool i_incOwnRadius;
            bool i_incTargetRadius;
    };

    // do attack at call of help to friendly creature
    class TC_GAME_API CallOfHelpCreatureInRangeDo
    {
    public:
        CallOfHelpCreatureInRangeDo(Unit* funit, Unit* enemy, float range);
        void operator()(Creature* u);
    private:
        Unit* const i_funit;
        Unit* const i_enemy;
        float i_range;
    };

    struct TC_GAME_API AnyDeadUnitCheck
    {
        bool operator()(Unit* u);
    };

    class UnitAuraCheck
    {
    public:
        UnitAuraCheck(bool present, uint32 spellId, ObjectGuid casterGUID = ObjectGuid::Empty) : _present(present), _spellId(spellId), _casterGUID(casterGUID) { }

        bool operator()(Unit* unit) const
        {
            return unit->HasAura(_spellId, _casterGUID) == _present;
        }

        bool operator()(WorldObject* object) const
        {
            return object->ToUnit() && object->ToUnit()->HasAura(_spellId, _casterGUID) == _present;
        }

    private:
        bool _present;
        uint32 _spellId;
        ObjectGuid _casterGUID;
    };

    // Creature checks

    class NearestHostileUnitCheck
    {
    public:
        explicit NearestHostileUnitCheck(Creature const* creature, float dist = 0.f, bool playerOnly = false) : me(creature), i_playerOnly(playerOnly)
        {
            m_range = (dist == 0.f ? 9999.f : dist);
        }

        bool operator()(Unit* u);

    private:
        Creature const* me;
        float m_range;
        bool i_playerOnly;
        NearestHostileUnitCheck(NearestHostileUnitCheck const&) = delete;
    };

    class TC_GAME_API NearestHostileUnitInAttackDistanceCheck
    {
        public:
            explicit NearestHostileUnitInAttackDistanceCheck(Creature const* creature, float dist = 0, bool playerOnly = false, bool furthest = false) : me(creature), i_playerOnly(playerOnly), m_minRange(0), i_furthest(furthest)
            {
                m_range = (dist == 0 ? 9999 : dist);
                m_force = (dist == 0 ? false : true);
            }
            bool operator()(Unit* u);
            float GetLastRange() const { return m_range; }
        private:
            Creature const* me;
            float m_range, m_minRange;
            bool m_force;
            bool i_playerOnly;
            bool i_furthest;
            NearestHostileUnitInAttackDistanceCheck(NearestHostileUnitInAttackDistanceCheck const&) = delete;
    };

    class TC_GAME_API NearestHostileUnitInAggroRangeCheck
    {
    public:
        explicit NearestHostileUnitInAggroRangeCheck(Creature const* creature, bool useLOS = false) : _me(creature), _useLOS(useLOS) { }

        bool operator()(Unit* u) const
        {
            if (!u->IsHostileTo(_me))
                return false;

            if (!u->IsWithinDistInMap(_me, _me->GetAggroRange(u)))
                return false;

            if (!_me->IsValidAttackTarget(u))
                return false;

            if (_useLOS && !u->IsWithinLOSInMap(_me))
                return false;

            return true;
        }

    private:
        Creature const* _me;
        bool _useLOS;
        NearestHostileUnitInAggroRangeCheck(NearestHostileUnitInAggroRangeCheck const&) = delete;
    };

    class TC_GAME_API AllWorldObjectsInRange
    {
        public:
            AllWorldObjectsInRange(const WorldObject* pObject, float fMaxRange) : m_pObject(pObject), m_fRange(fMaxRange) {}
            bool operator() (WorldObject* pGo);
        private:
            const WorldObject* m_pObject;
            float m_fRange;
    };
    
    class TC_GAME_API ObjectGUIDCheck
    {
    public:
        ObjectGUIDCheck(ObjectGuid GUID, bool equals) : _GUID(GUID), _equals(equals) {}
        bool operator()(WorldObject const* object)
        {
            return (object->GetGUID() == _GUID) == _equals;
        }

    private:
        ObjectGuid _GUID;
        bool _equals;
    };

    class TC_GAME_API MostHPMissingInRange
    {
    public:
        MostHPMissingInRange(Unit const* obj, float range, uint32 hp) : i_obj(obj), i_range(range), i_hp(hp) {}
        bool operator()(Unit* u);
    private:
        Unit const* i_obj;
        float i_range;
        uint32 i_hp;
    };

    class TC_GAME_API FriendlyCCedInRange
    {
    public:
        FriendlyCCedInRange(Unit const* obj, float range) : i_obj(obj), i_range(range) {}
        bool operator()(Unit* u);
    private:
        Unit const* i_obj;
        float i_range;
    };

    class TC_GAME_API FriendlyMissingBuffInRange
    {
    public:
        FriendlyMissingBuffInRange(Unit const* obj, float range, uint32 spellid) : i_obj(obj), i_range(range), i_spell(spellid) {}
        bool operator()(Unit* u);
    private:
        Unit const* i_obj;
        float i_range;
        uint32 i_spell;
    };
    
    class TC_GAME_API FriendlyMissingBuffInRangeOutOfCombat
    {
    public:
        FriendlyMissingBuffInRangeOutOfCombat(Unit const* obj, float range, uint32 spellid) : i_obj(obj), i_range(range), i_spell(spellid) {}
        bool operator()(Unit* u);
    private:
        Unit const* i_obj;
        float i_range;
        uint32 i_spell;
    };

    //PLAYERS SEARCHERS
    
    class TC_GAME_API AllPlayersInRange
    {
    public:
        AllPlayersInRange(WorldObject const* obj, float range) : i_object(obj), i_range(range) {}
        bool operator() (Player* u);
    private:
        WorldObject const* i_object;
        float i_range;
    };

    class TC_GAME_API AnyPlayerInObjectRangeCheck
    {
    public:
        AnyPlayerInObjectRangeCheck(WorldObject const* obj, float range) : i_obj(obj), i_range(range) {}
        bool operator()(Player* u);
    private:
        WorldObject const* i_obj;
        float i_range;
    };

    class TC_GAME_API NearestPlayerInObjectRangeCheck
    {
    public:
        public:
            NearestPlayerInObjectRangeCheck(WorldObject const& obj, bool alive, float range)
                : i_obj(obj), i_alive(alive), i_range(range) {}

            bool operator()(Player* u);
            float GetLastRange() const { return i_range; }
        private:
            WorldObject const& i_obj;
            bool   i_alive;
            float  i_range;

            // prevent clone this object
            NearestPlayerInObjectRangeCheck(NearestPlayerInObjectRangeCheck const&);
    };

    // Prepare using Builder localized packets with caching and send to player
    template<class Builder>
    class LocalizedPacketDo
    {
        public:
            explicit LocalizedPacketDo(Builder& builder) : i_builder(builder) { }

            ~LocalizedPacketDo()
            {
                for (size_t i = 0; i < i_data_cache.size(); ++i)
                    delete i_data_cache[i];
            }
            void operator()(Player* p);

        private:
            Builder& i_builder;
            std::vector<WorldPacket*> i_data_cache;         // 0 = default, i => i-1 locale index
    };

    // Prepare using Builder localized packets with caching and send to player
    template<class Builder>
    class LocalizedPacketListDo
    {
        public:
            typedef std::vector<WorldPacket*> WorldPacketList;
            explicit LocalizedPacketListDo(Builder& builder) : i_builder(builder) { }

            ~LocalizedPacketListDo()
            {
                for (size_t i = 0; i < i_data_cache.size(); ++i)
                    for (size_t j = 0; j < i_data_cache[i].size(); ++j)
                        delete i_data_cache[i][j];
            }
            void operator()(Player* p);

        private:
            Builder& i_builder;
            std::vector<WorldPacketList> i_data_cache;
                                                            // 0 = default, i => i-1 locale index
    };

    //CREATURES SEARCHERS

    class TC_GAME_API AllFriendlyCreaturesInGrid
    {
    public:
        AllFriendlyCreaturesInGrid(Unit const* obj) : pUnit(obj) {}
        bool operator() (Creature* u);
    private:
        Unit const* pUnit;
    };

    // Success at unit in range, range update for next check (this can be use with CreatureLastSearcher to find nearest creature)
    class TC_GAME_API NearestCreatureEntryWithLiveStateInObjectRangeCheck
    {
        public:
            NearestCreatureEntryWithLiveStateInObjectRangeCheck(WorldObject const& obj,uint32 entry, bool alive, float range)
                : i_obj(obj), i_entry(entry), i_alive(alive), i_range(range) {}

            bool operator()(Creature* u);
            float GetLastRange() const { return i_range; }
        private:
            WorldObject const& i_obj;
            uint32 i_entry;
            bool   i_alive;
            float  i_range;

            // prevent clone this object
            NearestCreatureEntryWithLiveStateInObjectRangeCheck(NearestCreatureEntryWithLiveStateInObjectRangeCheck const&);
    };

    class TC_GAME_API AllCreaturesOfEntryInRange
    {
    public:
        AllCreaturesOfEntryInRange(WorldObject const* object, uint32 entry, float maxRange) : m_pObject(object), m_uiEntry(entry), m_fRange(maxRange) { }

        bool operator()(Unit* unit) const;

    private:
        WorldObject const* m_pObject;
        uint32 m_uiEntry;
        float m_fRange;
    };

    class TC_GAME_API AllCreaturesInRange
    {
    public:
        AllCreaturesInRange(Unit const* obj, float ran) : pUnit(obj), range(ran) {}
        bool operator() (Creature* u);
    private:
        Unit const* pUnit;
        float range;
    };

    class TC_GAME_API AllCreatures
    {
    public:
        AllCreatures(Unit const* obj) : pUnit(obj) {}
        bool operator() (Creature* u);
    private:
        Unit const* pUnit;
    };

    class TC_GAME_API AnyAssistCreatureInRangeCheck
    {
        public:
            AnyAssistCreatureInRangeCheck(Unit* funit, Unit* enemy, float range)
                : i_funit(funit), i_enemy(enemy), i_range(range)
            {
            }

            bool operator()(Creature* u);
        private:
            Unit* const i_funit;
            Unit* const i_enemy;
            float i_range;
    };

    class TC_GAME_API NearestAssistCreatureInCreatureRangeCheck
    {
        public:
            NearestAssistCreatureInCreatureRangeCheck(Creature* obj,Unit* enemy, float range)
                : i_obj(obj), i_enemy(enemy), i_range(range) {}

            bool operator()(Creature* u);
            float GetLastRange() const { return i_range; }
        private:
            Creature* const i_obj;
            Unit* const i_enemy;
            float  i_range;

            // prevent clone this object
            NearestAssistCreatureInCreatureRangeCheck(NearestAssistCreatureInCreatureRangeCheck const&) = delete;
            NearestAssistCreatureInCreatureRangeCheck& operator=(NearestAssistCreatureInCreatureRangeCheck const &_Right) = delete;
    };

    class TC_GAME_API NearestGeneralizedAssistCreatureInCreatureRangeCheck
    {
        public:
            NearestGeneralizedAssistCreatureInCreatureRangeCheck(Creature* obj, uint32 entry, uint32 faction, float range)
                : i_obj(obj), i_entry(entry), i_faction(faction), i_range(range) {}

            bool operator()(Creature* u);
            float GetLastRange() const { return i_range; }
        private:
            Creature* const i_obj;
            uint32 i_entry;
        uint32 i_faction;
            float  i_range;

            // prevent cloning this object
            NearestGeneralizedAssistCreatureInCreatureRangeCheck(NearestGeneralizedAssistCreatureInCreatureRangeCheck const&) = delete;
            NearestGeneralizedAssistCreatureInCreatureRangeCheck& operator=(NearestGeneralizedAssistCreatureInCreatureRangeCheck const &_Right) = delete;
    };



    //GOBJECT SEARCHERS

    class TC_GAME_API GameObjectFocusCheck
    {
        public:
            GameObjectFocusCheck(WorldObject const* caster, uint32 focusId) : _caster(caster), _focusId(focusId) {}
            bool operator()(GameObject* go) const;
        private:
            WorldObject const* _caster;
            uint32 _focusId;
    };

    // Find the nearest Fishing hole and return true only if source object is in range of hole
    class TC_GAME_API NearestGameObjectFishingHole
    {
        public:
            NearestGameObjectFishingHole(WorldObject const& obj, float range) : i_obj(obj), i_range(range) {}
            bool operator()(GameObject* go);
            float GetLastRange() const { return i_range; }
        private:
            WorldObject const& i_obj;
            float  i_range;

            // prevent clone
            NearestGameObjectFishingHole(NearestGameObjectFishingHole const&);
    };

    class NearestGameObjectCheck
    {
    public:
        NearestGameObjectCheck(WorldObject const& obj) : i_obj(obj), i_range(999.f) { }

        bool operator()(GameObject* go)
        {
            if (i_obj.IsWithinDistInMap(go, i_range))
            {
                i_range = i_obj.GetDistance(go);        // use found GO range as new range limit for next check
                return true;
            }
            return false;
        }

    private:
        WorldObject const& i_obj;
        float i_range;

        // prevent clone this object
        NearestGameObjectCheck(NearestGameObjectCheck const&) = delete;
    };

    class TC_GAME_API AllGameObjectsWithEntryInGrid
    {
    public:
        AllGameObjectsWithEntryInGrid(uint32 ent) : entry(ent) {}
        bool operator() (GameObject* g);
    private:
        uint32 entry;
    };
    
    class TC_GAME_API AllGameObjectsWithEntryInRange
    {
    public:
        AllGameObjectsWithEntryInRange(const WorldObject* pObject, uint32 uiEntry, float fMaxRange) : m_pObject(pObject), m_uiEntry(uiEntry), m_fRange(fMaxRange) {}
        bool operator() (GameObject* pGo);
    private:
        const WorldObject* m_pObject;
        uint32 m_uiEntry;
        float m_fRange;
    };

    //2D check
    class TC_GAME_API AllGameObjectsInRange
    {
    public:
        AllGameObjectsInRange(float x, float y, float z, float fMaxRange) : m_X(x), m_Y(y), m_Z(z), m_fRange(fMaxRange) {}
        bool operator() (GameObject* pGo);
    private:
        float m_X;
        float m_Y;
        float m_Z;
        float m_fRange;
    };

    // Success at unit in range, range update for next check (this can be use with GameobjectLastSearcher to find nearest GO)
    class TC_GAME_API NearestGameObjectEntryInObjectRangeCheck
    {
        public:
            NearestGameObjectEntryInObjectRangeCheck(WorldObject const& obj,uint32 entry, float range) : i_obj(obj), i_entry(entry), i_range(range) {}
            bool operator()(GameObject* go);
            float GetLastRange() const { return i_range; }
        private:
            WorldObject const& i_obj;
            uint32 i_entry;
            float  i_range;

            // prevent clone this object
            NearestGameObjectEntryInObjectRangeCheck(NearestGameObjectEntryInObjectRangeCheck const&);
    };

    // Success at unit in range, range update for next check (this can be use with GameobjectLastSearcher to find nearest GO with a certain type)
    class TC_GAME_API NearestGameObjectTypeInObjectRangeCheck
    {
    public:
        NearestGameObjectTypeInObjectRangeCheck(WorldObject const& obj, GameobjectTypes type, float range) : i_obj(obj), i_type(type), i_range(range) {}
        bool operator()(GameObject* go)
        {
            if (go->GetGoType() == i_type && i_obj.IsWithinDistInMap(go, i_range))
            {
                i_range = i_obj.GetDistance(go);        // use found GO range as new range limit for next check
                return true;
            }
            return false;
        }
    private:
        WorldObject const& i_obj;
        GameobjectTypes i_type;
        float  i_range;

        // prevent clone this object
        NearestGameObjectTypeInObjectRangeCheck(NearestGameObjectTypeInObjectRangeCheck const&);
    };

    class TC_GAME_API GameObjectWithSpawnIdCheck
    {
        public:
            GameObjectWithSpawnIdCheck(WorldObject const& obj, ObjectGuid::LowType db_guid) : i_obj(obj), i_db_guid(db_guid) {}
            bool operator()(GameObject const* go) const;
        private:
            WorldObject const& i_obj;
            ObjectGuid::LowType i_db_guid;
    };
}
#endif

