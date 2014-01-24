/*
 * Copyright (C) 2005-2008 MaNGOS <http://www.mangosproject.org/>
 *
 * Copyright (C) 2008 Trinity <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef TRINITY_GRIDNOTIFIERS_H
#define TRINITY_GRIDNOTIFIERS_H

#include "ObjectGridLoader.h"
#include "ByteBuffer.h"
#include "UpdateData.h"
#include <iostream>

#include "Corpse.h"
#include "Object.h"
#include "DynamicObject.h"
#include "GameObject.h"
#include "Player.h"
#include "Unit.h"

class Player;
//class Map;

namespace Trinity
{
    struct PlayerVisibilityNotifier
    {
        Player &i_player;
        UpdateData i_data;
        Player::ClientGUIDs i_clientGUIDs;
        std::set<WorldObject*> i_visibleNow;

        PlayerVisibilityNotifier(Player &player) : i_player(player),i_clientGUIDs(player.m_clientGUIDs) {}

        template<class T> inline void Visit(GridRefManager<T> &);

        void Notify(void);
    };

    struct PlayerRelocationNotifier : public PlayerVisibilityNotifier
    {
        PlayerRelocationNotifier(Player &player) : PlayerVisibilityNotifier(player) {}
        template<class T> inline void Visit(GridRefManager<T> &m) { PlayerVisibilityNotifier::Visit(m); }
        #ifdef WIN32
        template<> inline void Visit(PlayerMapType &);
        template<> inline void Visit(CreatureMapType &);
        #endif
    };

    struct CreatureRelocationNotifier
    {
        Creature &i_creature;
        CreatureRelocationNotifier(Creature &c) : i_creature(c) {}
        template<class T> void Visit(GridRefManager<T> &) {}
        #ifdef WIN32
        template<> inline void Visit(PlayerMapType &);
        template<> inline void Visit(CreatureMapType &);
        #endif
    };

    struct VisibleChangesNotifier
    {
        WorldObject &i_object;

        explicit VisibleChangesNotifier(WorldObject &object) : i_object(object) {}
        template<class T> void Visit(GridRefManager<T> &) {}
        void Visit(PlayerMapType &);
    };

    struct GridUpdater
    {
        GridType &i_grid;
        uint32 i_timeDiff;
        GridUpdater(GridType &grid, uint32 diff) : i_grid(grid), i_timeDiff(diff) {}

        template<class T> void updateObjects(GridRefManager<T> &m)
        {
            for(typename GridRefManager<T>::iterator iter = m.begin(); iter != m.end(); ++iter)
                iter->getSource()->Update(i_timeDiff);
        }

        void Visit(PlayerMapType &m) { updateObjects<Player>(m); }
        void Visit(CreatureMapType &m){ updateObjects<Creature>(m); }
        void Visit(GameObjectMapType &m) { updateObjects<GameObject>(m); }
        void Visit(DynamicObjectMapType &m) { updateObjects<DynamicObject>(m); }
        void Visit(CorpseMapType &m) { updateObjects<Corpse>(m); }
    };

    struct Deliverer
    {
        WorldObject &i_source;
        WorldPacket *i_message;
        std::set<uint64> plr_list;
        bool i_toPossessor;
        bool i_toSelf;
        float i_dist;
        Deliverer(WorldObject &src, WorldPacket *msg, bool to_possessor, bool to_self, float dist = 0.0f) : i_source(src), i_message(msg), i_toPossessor(to_possessor), i_toSelf(to_self), i_dist(dist) {}
        void Visit(PlayerMapType &m);
        void Visit(CreatureMapType &m);
        void Visit(DynamicObjectMapType &m);
        virtual void VisitObject(Player* plr) = 0;
        void SendPacket(Player* plr);
        template<class SKIP> void Visit(GridRefManager<SKIP> &) {}
    };

    struct MessageDeliverer : public Deliverer
    {
        MessageDeliverer(Player &pl, WorldPacket *msg, bool to_possessor, bool to_self) : Deliverer(pl, msg, to_possessor, to_self) {}
        void VisitObject(Player* plr);
    };

    struct ObjectMessageDeliverer : public Deliverer
    {
        explicit ObjectMessageDeliverer(WorldObject &src, WorldPacket *msg, bool to_possessor) : Deliverer(src, msg, to_possessor, false) {}
        void VisitObject(Player* plr) { SendPacket(plr); }
    };

    struct MessageDistDeliverer : public Deliverer
    {
        bool i_ownTeamOnly;
        MessageDistDeliverer(Player &pl, WorldPacket *msg, bool to_possessor, float dist, bool to_self, bool ownTeamOnly) : Deliverer(pl, msg, to_possessor, to_self, dist), i_ownTeamOnly(ownTeamOnly) {}
        void VisitObject(Player* plr);
    };

    struct ObjectMessageDistDeliverer : public Deliverer
    {
        ObjectMessageDistDeliverer(WorldObject &obj, WorldPacket *msg, bool to_possessor, float dist) : Deliverer(obj, msg, to_possessor, false, dist) {}
        void VisitObject(Player* plr) { SendPacket(plr); }
    };

    struct ObjectUpdater
    {
        uint32 i_timeDiff;
        explicit ObjectUpdater(const uint32 &diff) : i_timeDiff(diff) {}
        template<class T> void Visit(GridRefManager<T> &m);
        void Visit(PlayerMapType &) {}
        void Visit(CorpseMapType &) {}
        void Visit(CreatureMapType &);
    };

    struct DynamicObjectUpdater
    {
        DynamicObject &i_dynobject;
        Unit* i_check;
        DynamicObjectUpdater(DynamicObject &dynobject, Unit* caster) : i_dynobject(dynobject)
        {
            i_check = caster;
            Unit* owner = i_check->GetOwner();
            if(owner)
                i_check = owner;
        }

        template<class T> inline void Visit(GridRefManager<T>  &) {}
        #ifdef WIN32
        template<> inline void Visit<Player>(PlayerMapType &);
        template<> inline void Visit<Creature>(CreatureMapType &);
        #endif

        void VisitHelper(Unit* target);
    };

    // SEARCHERS & LIST SEARCHERS & WORKERS

    // WorldObject searchers & workers

    template<class Check>
        struct WorldObjectSearcher
    {
        WorldObject* &i_object;
        Check &i_check;

        WorldObjectSearcher(WorldObject* & result, Check& check) : i_object(result),i_check(check) {}

        void Visit(GameObjectMapType &m);
        void Visit(PlayerMapType &m);
        void Visit(CreatureMapType &m);
        void Visit(CorpseMapType &m);
        void Visit(DynamicObjectMapType &m);

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED> &) {}
    };

    template<class Check>
        struct WorldObjectListSearcher
    {
        std::list<WorldObject*> &i_objects;
        Check& i_check;

        WorldObjectListSearcher(std::list<WorldObject*> &objects, Check & check) : i_objects(objects),i_check(check) {}

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
        Do const& i_do;

        explicit WorldObjectWorker(Do const& _do) : i_do(_do) {}

        void Visit(GameObjectMapType &m)
        {
            for(GameObjectMapType::iterator itr=m.begin(); itr != m.end(); ++itr)
                i_do(itr->getSource());
        }

        void Visit(PlayerMapType &m)
        {
            for(PlayerMapType::iterator itr=m.begin(); itr != m.end(); ++itr)
                i_do(itr->getSource());
        }
        void Visit(CreatureMapType &m)
        {
            for(CreatureMapType::iterator itr=m.begin(); itr != m.end(); ++itr)
                i_do(itr->getSource());
        }

        void Visit(CorpseMapType &m)
        {
            for(CorpseMapType::iterator itr=m.begin(); itr != m.end(); ++itr)
                i_do(itr->getSource());
        }

        void Visit(DynamicObjectMapType &m)
        {
            for(DynamicObjectMapType::iterator itr=m.begin(); itr != m.end(); ++itr)
                i_do(itr->getSource());
        }

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED> &) {}
    };

    // Gameobject searchers

    template<class Check>
        struct GameObjectSearcher
    {
        GameObject* &i_object;
        Check &i_check;

        GameObjectSearcher(GameObject* & result, Check& check) : i_object(result),i_check(check) {}

        void Visit(GameObjectMapType &m);

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED> &) {}
    };

    // Last accepted by Check GO if any (Check can change requirements at each call)
    template<class Check>
        struct GameObjectLastSearcher
    {
        GameObject* &i_object;
        Check& i_check;

        GameObjectLastSearcher(GameObject* & result, Check& check) : i_object(result),i_check(check) {}

        void Visit(GameObjectMapType &m);

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED> &) {}
    };

    template<class Check>
        struct GameObjectListSearcher
    {
        std::list<GameObject*> &i_objects;
        Check& i_check;

        GameObjectListSearcher(std::list<GameObject*> &objects, Check & check) : i_objects(objects),i_check(check) {}

        void Visit(GameObjectMapType &m);

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED> &) {}
    };

    // Unit searchers

    // First accepted by Check Unit if any
    template<class Check>
        struct UnitSearcher
    {
        Unit* &i_object;
        Check & i_check;

        UnitSearcher(Unit* & result, Check & check) : i_object(result),i_check(check) {}

        void Visit(CreatureMapType &m);
        void Visit(PlayerMapType &m);

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED> &) {}
    };

    // Last accepted by Check Unit if any (Check can change requirements at each call)
    template<class Check>
        struct UnitLastSearcher
    {
        Unit* &i_object;
        Check & i_check;

        UnitLastSearcher(Unit* & result, Check & check) : i_object(result),i_check(check) {}

        void Visit(CreatureMapType &m);
        void Visit(PlayerMapType &m);

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED> &) {}
    };

    // All accepted by Check units if any
    template<class Check>
        struct UnitListSearcher
    {
        std::list<Unit*> &i_objects;
        Check& i_check;

        UnitListSearcher(std::list<Unit*> &objects, Check & check) : i_objects(objects),i_check(check) {}

        void Visit(PlayerMapType &m);
        void Visit(CreatureMapType &m);

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED> &) {}
    };
    
    template<class Check>
        struct PlayerListSearcher
    {
        std::list<Player*> &i_objects;
        Check& i_check;
        
        PlayerListSearcher(std::list<Player*> &objects, Check& check) : i_objects(objects), i_check(check) {}
        
        void Visit(PlayerMapType &m);
        
        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED> &) {}
    };

    // Creature searchers

    template<class Check>
        struct CreatureSearcher
    {
        Creature* &i_object;
        Check & i_check;

        CreatureSearcher(Creature* & result, Check & check) : i_object(result),i_check(check) {}

        void Visit(CreatureMapType &m);

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED> &) {}
    };

    // Last accepted by Check Creature if any (Check can change requirements at each call)
    template<class Check>
        struct CreatureLastSearcher
    {
        Creature* &i_object;
        Check & i_check;

        CreatureLastSearcher(Creature* & result, Check & check) : i_object(result),i_check(check) {}

        void Visit(CreatureMapType &m);

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED> &) {}
    };

    template<class Check>
        struct CreatureListSearcher
    {
        std::list<Creature*> &i_objects;
        Check& i_check;

        CreatureListSearcher(std::list<Creature*> &objects, Check & check) : i_objects(objects),i_check(check) {}

        void Visit(CreatureMapType &m);

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED> &) {}
    };

    // Player searchers

    template<class Check>
    struct PlayerSearcher
    {
        Player* &i_object;
        Check & i_check;

        PlayerSearcher(Player* & result, Check & check) : i_object(result),i_check(check) {}

        void Visit(PlayerMapType &m);

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED> &) {}
    };

    template<class Do>
    struct PlayerWorker
    {
        Do& i_do;

        explicit PlayerWorker(Do& _do) : i_do(_do) {}

        void Visit(PlayerMapType &m)
        {
            for(PlayerMapType::iterator itr=m.begin(); itr != m.end(); ++itr)
                i_do(itr->getSource());
        }

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED> &) {}
    };

    // CHECKS && DO classes

    // WorldObject check classes
    class CannibalizeObjectCheck
    {
        public:
            CannibalizeObjectCheck(Unit* funit, float range) : i_funit(funit), i_range(range) {}
            bool operator()(Player* u)
            {
                if( i_funit->IsFriendlyTo(u) || u->IsAlive() || u->isInFlight() )
                    return false;

                if(i_funit->IsWithinDistInMap(u, i_range) )
                    return true;

                return false;
            }
            bool operator()(Corpse* u);
            bool operator()(Creature* u)
            {
                if( i_funit->IsFriendlyTo(u) || u->IsAlive() || u->isInFlight() ||
                    (u->GetCreatureTypeMask() & CREATURE_TYPEMASK_HUMANOID_OR_UNDEAD)==0)
                    return false;

                if(i_funit->IsWithinDistInMap(u, i_range) )
                    return true;

                return false;
            }
            template<class NOT_INTERESTED> bool operator()(NOT_INTERESTED*) { return false; }
        private:
            Unit* const i_funit;
            float i_range;
    };

    // WorldObject do classes

    class RespawnDo
    {
        public:
            RespawnDo() {}
            void operator()(Creature* u) const { u->Respawn(); }
            void operator()(GameObject* u) const { u->Respawn(); }
            void operator()(WorldObject*) const {}
            void operator()(Corpse*) const {}
    };

    class FactionDo
    {
        public:
            FactionDo(uint32 id, uint32 f) : i_entry(id), i_faction(f) {}
            void operator()(Creature* u) const {
                if (u->GetEntry() == i_entry)
                    u->setFaction(i_faction);
            }
            void operator()(GameObject*) const {}
            void operator()(WorldObject*) const {}
            void operator()(Corpse*) const {}
        private:
            uint32 i_entry, i_faction;
    };

    // GameObject checks

    class GameObjectFocusCheck
    {
        public:
            GameObjectFocusCheck(Unit const* unit,uint32 focusId) : i_unit(unit), i_focusId(focusId) {}
            bool operator()(GameObject* go) const
            {
                if(go->GetGOInfo()->type != GAMEOBJECT_TYPE_SPELL_FOCUS)
                    return false;

                if(go->GetGOInfo()->spellFocus.focusId != i_focusId)
                    return false;

                float dist = (float)((go->GetGOInfo()->spellFocus.dist)/2);

                return go->IsWithinDistInMap(i_unit, dist);
            }
        private:
            Unit const* i_unit;
            uint32 i_focusId;
    };

    // Find the nearest Fishing hole and return true only if source object is in range of hole
    class NearestGameObjectFishingHole
    {
        public:
            NearestGameObjectFishingHole(WorldObject const& obj, float range) : i_obj(obj), i_range(range) {}
            bool operator()(GameObject* go)
            {
                if(go->GetGOInfo()->type == GAMEOBJECT_TYPE_FISHINGHOLE && go->isSpawned() && i_obj.IsWithinDistInMap(go, i_range) && i_obj.IsWithinDistInMap(go, go->GetGOInfo()->fishinghole.radius))
                {
                    i_range = i_obj.GetDistance(go);
                    return true;
                }
                return false;
            }
            float GetLastRange() const { return i_range; }
        private:
            WorldObject const& i_obj;
            float  i_range;

            // prevent clone
            NearestGameObjectFishingHole(NearestGameObjectFishingHole const&);
    };

    // Success at unit in range, range update for next check (this can be use with GameobjectLastSearcher to find nearest GO)
    class NearestGameObjectEntryInObjectRangeCheck
    {
        public:
            NearestGameObjectEntryInObjectRangeCheck(WorldObject const& obj,uint32 entry, float range) : i_obj(obj), i_entry(entry), i_range(range) {}
            bool operator()(GameObject* go)
            {
                if(go->GetEntry() == i_entry && i_obj.IsWithinDistInMap(go, i_range))
                {
                    i_range = i_obj.GetDistance(go);        // use found GO range as new range limit for next check
                    return true;
                }
                return false;
            }
            float GetLastRange() const { return i_range; }
        private:
            WorldObject const& i_obj;
            uint32 i_entry;
            float  i_range;

            // prevent clone this object
            NearestGameObjectEntryInObjectRangeCheck(NearestGameObjectEntryInObjectRangeCheck const&);
    };

    class GameObjectWithDbGUIDCheck
    {
        public:
            GameObjectWithDbGUIDCheck(WorldObject const& obj,uint32 db_guid) : i_obj(obj), i_db_guid(db_guid) {}
            bool operator()(GameObject const* go) const
            {
                return go->GetDBTableGUIDLow() == i_db_guid;
            }
        private:
            WorldObject const& i_obj;
            uint32 i_db_guid;
    };

    // Unit checks

    class AnyUnfriendlyUnitInObjectRangeCheck
    {
        public:
            AnyUnfriendlyUnitInObjectRangeCheck(WorldObject const* obj, Unit const* funit, float range) : i_obj(obj), i_funit(funit), i_range(range) {}
            bool operator()(Unit* u)
            {
                if(u->IsAlive() && i_obj->IsWithinDistInMap(u, i_range) && !i_funit->IsFriendlyTo(u))
                    return true;
                else
                    return false;
            }
        private:
            WorldObject const* i_obj;
            Unit const* i_funit;
            float i_range;
    };

    class AnyUnfriendlyAoEAttackableUnitInObjectRangeCheck
    {
        public:
            AnyUnfriendlyAoEAttackableUnitInObjectRangeCheck(WorldObject const* obj, Unit const* funit, float range) : i_obj(obj), i_funit(funit), i_range(range) {}
            bool operator()(Unit* u)
            {
                if(!u->isAttackableByAOE())
                    return false;
                    
                // From 2.1.0 Feral Charge ignored traps, from 2.3.0 Intercept and Charge started to do so too
                if(u->HasUnitState(UNIT_STAT_CHARGING))
                    return false;

                if(!i_obj->IsWithinDistInMap(u, i_range))
                    return false;

                if(i_funit->IsFriendlyTo(u))
                    return false;

                return true;
            }
        private:
            WorldObject const* i_obj;
            Unit const* i_funit;
            float i_range;
    };

    class CreatureWithDbGUIDCheck
    {
        public:
            CreatureWithDbGUIDCheck(WorldObject const* obj, uint32 lowguid) : i_obj(obj), i_lowguid(lowguid) {}
            bool operator()(Creature* u)
            {
                return u->GetDBTableGUIDLow() == i_lowguid;
            }
        private:
            WorldObject const* i_obj;
            uint32 i_lowguid;
    };

    class AnyFriendlyUnitInObjectRangeCheck
    {
        public:
            AnyFriendlyUnitInObjectRangeCheck(WorldObject const* obj, Unit const* funit, float range, bool playerOnly = false) : i_obj(obj), i_funit(funit), i_range(range), i_playerOnly(playerOnly) {}
            bool operator()(Unit* u)
            {
                if(u->IsAlive() && i_obj->IsWithinDistInMap(u, i_range) && i_funit->IsFriendlyTo(u) && (!i_playerOnly || u->GetTypeId() == TYPEID_PLAYER))
                    return true;
                else
                    return false;
            }
        private:
            WorldObject const* i_obj;
            Unit const* i_funit;
            float i_range;
            bool i_playerOnly;
    };

    class AnyUnitInObjectRangeCheck
    {
        public:
            AnyUnitInObjectRangeCheck(WorldObject const* obj, float range) : i_obj(obj), i_range(range) {}
            bool operator()(Unit* u)
            {
                if(u->IsAlive() && i_obj->IsWithinDistInMap(u, i_range))
                    return true;

                return false;
            }
        private:
            WorldObject const* i_obj;
            float i_range;
    };

    // Success at unit in range, range update for next check (this can be use with UnitLastSearcher to find nearest unit)
    class NearestAttackableUnitInObjectRangeCheck
    {
        public:
            NearestAttackableUnitInObjectRangeCheck(WorldObject const* obj, Unit const* funit, float range) : i_obj(obj), i_funit(funit), i_range(range) {}
            bool operator()(Unit* u)
            {
                if( i_funit->canAttack(u) && i_obj->IsWithinDistInMap(u, i_range) &&
                    !i_funit->IsFriendlyTo(u) && u->isVisibleForOrDetect(i_funit,false)  )
                {
                    i_range = i_obj->GetDistance(u);        // use found unit range as new range limit for next check
                    return true;
                }

                return false;
            }
        private:
            WorldObject const* i_obj;
            Unit const* i_funit;
            float i_range;

            // prevent clone this object
            NearestAttackableUnitInObjectRangeCheck(NearestAttackableUnitInObjectRangeCheck const&);
    };

    class AnyAoETargetUnitInObjectRangeCheck
    {
        public:
            AnyAoETargetUnitInObjectRangeCheck(WorldObject const* obj, Unit const* funit, float range)
                : i_obj(obj), i_funit(funit), i_range(range)
            {
                Unit const* check = i_funit;
                Unit const* owner = i_funit->GetOwner();
                if(owner)
                    check = owner;
                i_targetForPlayer = ( check->GetTypeId()==TYPEID_PLAYER );
            }
            bool operator()(Unit* u)
            {
                // Check contains checks for: live, non-selectable, non-attackable flags, flight check and GM check, ignore totems
                if (!i_funit->canAttack(u))
                    return false;
                if(u->GetTypeId()==TYPEID_UNIT && (u->ToCreature())->isTotem())
                    return false;

                if(( i_targetForPlayer ? !i_funit->IsFriendlyTo(u) : i_funit->IsHostileTo(u) )&& i_obj->IsWithinDistInMap(u, i_range))
                    return true;

                return false;
            }
        private:
            bool i_targetForPlayer;
            WorldObject const* i_obj;
            Unit const* i_funit;
            float i_range;
    };

    struct AnyDeadUnitCheck
    {
        bool operator()(Unit* u) { return !u->IsAlive(); }
    };

    struct AnyStealthedCheck
    {
        bool operator()(Unit* u) { return u->GetVisibility()==VISIBILITY_GROUP_STEALTH; }
    };

    // Creature checks

    class NearestHostileUnitInAttackDistanceCheck
    {
        public:
            explicit NearestHostileUnitInAttackDistanceCheck(Creature const* creature, float dist = 0, bool playerOnly = false) : m_creature(creature), i_playerOnly(playerOnly)
            {
                m_range = (dist == 0 ? 9999 : dist);
                m_force = (dist == 0 ? false : true);
            }
            bool operator()(Unit* u)
            {
                // TODO: addthreat for every enemy in range?
                if(!m_creature->IsWithinDistInMap(u, m_range))
                    return false;

                if(m_force)
                {
                    if(!m_creature->canAttack(u))
                        return false;
                }
                else
                {
                    if(!m_creature->canStartAttack(u))
                        return false;
                }

                if (i_playerOnly && u->GetTypeId() != TYPEID_PLAYER)
                    return false;

                m_range = m_creature->GetDistance(u);
                return true;
            }
            float GetLastRange() const { return m_range; }
        private:
            Creature const *m_creature;
            float m_range;
            bool m_force;
            bool i_playerOnly;
            NearestHostileUnitInAttackDistanceCheck(NearestHostileUnitInAttackDistanceCheck const&);
    };

    class NearestAssistCreatureInCreatureRangeCheck
    {
        public:
            NearestAssistCreatureInCreatureRangeCheck(Creature* obj,Unit* enemy, float range)
                : i_obj(obj), i_enemy(enemy), i_range(range) {}

            bool operator()(Creature* u)
            {
                if(u->getFaction() == i_obj->getFaction() && !u->IsInCombat() && !u->GetCharmerOrOwnerGUID() && u->IsHostileTo(i_enemy) && u->IsAlive()&& i_obj->IsWithinDistInMap(u, i_range) && i_obj->IsWithinLOSInMap(u))
                {
                    i_range = i_obj->GetDistance(u);         // use found unit range as new range limit for next check
                    return true;
                }
                return false;
            }
            float GetLastRange() const { return i_range; }
        private:
            Creature* const i_obj;
            Unit* const i_enemy;
            float  i_range;

            // prevent clone this object
            NearestAssistCreatureInCreatureRangeCheck(NearestAssistCreatureInCreatureRangeCheck const&);
    };

    class NearestGeneralizedAssistCreatureInCreatureRangeCheck
    {
        public:
            NearestGeneralizedAssistCreatureInCreatureRangeCheck(Creature* obj, uint32 entry, uint32 faction, float range)
                : i_obj(obj), i_entry(entry), i_faction(faction), i_range(range) {}

            bool operator()(Creature* u)
            {
		if ( u->GetEntry() == i_entry && u->getFaction() == i_faction && !u->IsInCombat() && !u->GetCharmerOrOwnerGUID() && u->IsAlive()&& i_obj->IsWithinDistInMap(u, i_range) && i_obj->IsWithinLOSInMap(u) )
                {
                    i_range = i_obj->GetDistance(u);         // use found unit range as new range limit for next check
                    return true;
                }
                return false;
            }
            float GetLastRange() const { return i_range; }
        private:
            Creature* const i_obj;
            uint32 i_entry;
	    uint32 i_faction;
            float  i_range;

            // prevent cloning this object
            NearestGeneralizedAssistCreatureInCreatureRangeCheck(NearestGeneralizedAssistCreatureInCreatureRangeCheck const&);
    };

    class AnyAssistCreatureInRangeCheck
    {
        public:
            AnyAssistCreatureInRangeCheck(Unit* funit, Unit* enemy, float range)
                : i_funit(funit), i_enemy(enemy), i_range(range)
            {
            }
            bool operator()(Creature* u)
            {
                if(u == i_funit)
                    return false;

                if ( !u->CanAssistTo(i_funit, i_enemy) )
                    return false;

                // too far
                if( !i_funit->IsWithinDistInMap(u, i_range, true) )
                    return false;

                // only if see assisted creature
                if( !i_funit->IsWithinLOSInMap(u) )
                    return false;

                return true;
            }
        private:
            Unit* const i_funit;
            Unit* const i_enemy;
            float i_range;
    };
    
    class AllWorldObjectsInRange
    {
        public:
            AllWorldObjectsInRange(const WorldObject* pObject, float fMaxRange) : m_pObject(pObject), m_fRange(fMaxRange) {}
            bool operator() (WorldObject* pGo)
            {
                return m_pObject->IsWithinDistInMap(pGo, m_fRange, false);
            }
        private:
            const WorldObject* m_pObject;
            float m_fRange;
    };

    // Success at unit in range, range update for next check (this can be use with CreatureLastSearcher to find nearest creature)
    class NearestCreatureEntryWithLiveStateInObjectRangeCheck
    {
        public:
            NearestCreatureEntryWithLiveStateInObjectRangeCheck(WorldObject const& obj,uint32 entry, bool alive, float range)
                : i_obj(obj), i_entry(entry), i_alive(alive), i_range(range) {}

            bool operator()(Creature* u)
            {
                if(u->GetEntry() == i_entry && u->IsAlive()==i_alive && i_obj.IsWithinDistInMap(u, i_range))
                {
                    i_range = i_obj.GetDistance(u);         // use found unit range as new range limit for next check
                    return true;
                }
                return false;
            }
            float GetLastRange() const { return i_range; }
        private:
            WorldObject const& i_obj;
            uint32 i_entry;
            bool   i_alive;
            float  i_range;

            // prevent clone this object
            NearestCreatureEntryWithLiveStateInObjectRangeCheck(NearestCreatureEntryWithLiveStateInObjectRangeCheck const&);
    };

    class AnyPlayerInObjectRangeCheck
    {
    public:
        AnyPlayerInObjectRangeCheck(WorldObject const* obj, float range) : i_obj(obj), i_range(range) {}
        bool operator()(Player* u)
        {
            if(u->IsAlive() && i_obj->IsWithinDistInMap(u, i_range))
                return true;

            return false;
        }
    private:
        WorldObject const* i_obj;
        float i_range;
    };

    class NearestPlayerInObjectRangeCheck
    {
    public:
        public:
            NearestPlayerInObjectRangeCheck(WorldObject const& obj, bool alive, float range)
                : i_obj(obj), i_alive(alive), i_range(range) {}

            bool operator()(Player* u)
            {
                if(u->IsAlive()==i_alive && i_obj.IsWithinDistInMap(u, i_range))
                {
                    i_range = i_obj.GetDistance(u);         // use found unit range as new range limit for next check
                    return true;
                }
                return false;
            }
            float GetLastRange() const { return i_range; }
        private:
            WorldObject const& i_obj;
            bool   i_alive;
            float  i_range;

            // prevent clone this object
            NearestPlayerInObjectRangeCheck(NearestPlayerInObjectRangeCheck const&);
    };

    // Searchers used by ScriptedAI
    class MostHPMissingInRange
    {
    public:
        MostHPMissingInRange(Unit const* obj, float range, uint32 hp) : i_obj(obj), i_range(range), i_hp(hp) {}
        bool operator()(Unit* u)
        {
            if(u->IsAlive() && u->IsInCombat() && !i_obj->IsHostileTo(u) && i_obj->IsWithinDistInMap(u, i_range) && u->GetMaxHealth() - u->GetHealth() > i_hp)
            {
                i_hp = u->GetMaxHealth() - u->GetHealth();
                return true;
            }
            return false;
        }
    private:
        Unit const* i_obj;
        float i_range;
        uint32 i_hp;
    };

    class FriendlyCCedInRange
    {
    public:
        FriendlyCCedInRange(Unit const* obj, float range) : i_obj(obj), i_range(range) {}
        bool operator()(Unit* u)
        {
            if(u->IsAlive() && u->IsInCombat() && !i_obj->IsHostileTo(u) && i_obj->IsWithinDistInMap(u, i_range) &&
                (u->isFeared() || u->isCharmed() || u->isFrozen() || u->HasUnitState(UNIT_STAT_STUNNED) || u->HasUnitState(UNIT_STAT_CONFUSED)))
            {
                return true;
            }
            return false;
        }
    private:
        Unit const* i_obj;
        float i_range;
    };

    class FriendlyMissingBuffInRange
    {
    public:
        FriendlyMissingBuffInRange(Unit const* obj, float range, uint32 spellid) : i_obj(obj), i_range(range), i_spell(spellid) {}
        bool operator()(Unit* u)
        {
            if(u->IsAlive() && u->IsInCombat() && /*!i_obj->IsHostileTo(u)*/ i_obj->IsFriendlyTo(u) && i_obj->IsWithinDistInMap(u, i_range) &&
                !(u->HasAura(i_spell, 0) || u->HasAura(i_spell, 1) || u->HasAura(i_spell, 2)))
            {
                return true;
            }
            return false;
        }
    private:
        Unit const* i_obj;
        float i_range;
        uint32 i_spell;
    };
    
    class FriendlyMissingBuffInRangeOutOfCombat
    {
    public:
        FriendlyMissingBuffInRangeOutOfCombat(Unit const* obj, float range, uint32 spellid) : i_obj(obj), i_range(range), i_spell(spellid) {}
        bool operator()(Unit* u)
        {
            if (u->IsAlive() && !u->IsInCombat() && i_obj->IsFriendlyTo(u) && i_obj->IsWithinDistInMap(u, i_range) &&
                !(u->HasAura(i_spell)) && i_obj != u)
            {
                return true;
            }
            return false;
        }
    private:
        Unit const* i_obj;
        float i_range;
        uint32 i_spell;
    };

    class AllFriendlyCreaturesInGrid
    {
    public:
        AllFriendlyCreaturesInGrid(Unit const* obj) : pUnit(obj) {}
        bool operator() (Unit* u)
        {
            if(u->IsAlive() && u->GetVisibility() == VISIBILITY_ON && u->IsFriendlyTo(pUnit))
                return true;

            return false;
        }
    private:
        Unit const* pUnit;
    };

    class AllGameObjectsWithEntryInGrid
    {
    public:
        AllGameObjectsWithEntryInGrid(uint32 ent) : entry(ent) {}
        bool operator() (GameObject* g)
        {
            if(g->GetEntry() == entry)
                return true;

            return false;
        }
    private:
        uint32 entry;
    };
    
    class AllGameObjectsWithEntryInRange
    {
    public:
        AllGameObjectsWithEntryInRange(const WorldObject* pObject, uint32 uiEntry, float fMaxRange) : m_pObject(pObject), m_uiEntry(uiEntry), m_fRange(fMaxRange) {}
        bool operator() (GameObject* pGo)
        {
            if (pGo->GetEntry() == m_uiEntry && m_pObject->IsWithinDistInMap(pGo,m_fRange,false))
                return true;

            return false;
        }
    private:
        const WorldObject* m_pObject;
        uint32 m_uiEntry;
        float m_fRange;
    };

    class AllCreaturesOfEntryInRange
    {
    public:
        AllCreaturesOfEntryInRange(Unit const* obj, uint32 ent, float ran) : pUnit(obj), entry(ent), range(ran) {}
        bool operator() (Unit* u)
        {
            if(u->GetEntry() == entry && pUnit->IsWithinDistInMap(u, range))
                return true;

            return false;
        }
    private:
        Unit const* pUnit;
        uint32 entry;
        float range;
    };
    
    class AllPlayersInRange
    {
    public:
        AllPlayersInRange(WorldObject const* obj, float range) : i_object(obj), i_range(range) {}
        bool operator() (Player* u)
        {
            if (i_object->IsWithinDistInMap(u, i_range))
                return true;
                
            return false;
        }
    private:
        WorldObject const* i_object;
        float i_range;
    };

    #ifndef WIN32
    template<> inline void PlayerRelocationNotifier::Visit<Creature>(CreatureMapType &);
    template<> inline void PlayerRelocationNotifier::Visit<Player>(PlayerMapType &);
    template<> inline void CreatureRelocationNotifier::Visit<Player>(PlayerMapType &);
    template<> inline void CreatureRelocationNotifier::Visit<Creature>(CreatureMapType &);
    template<> inline void DynamicObjectUpdater::Visit<Creature>(CreatureMapType &);
    template<> inline void DynamicObjectUpdater::Visit<Player>(PlayerMapType &);
    #endif
}
#endif

