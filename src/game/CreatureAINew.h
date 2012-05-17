/*
 * Copyright (C) 2009 - 2011 Windrunner
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef WR_CREATUREAI_H
#define WR_CREATUREAI_H

enum SelectedTarget
{
    TARGET_RANDOM = 0,                               //Just selects a random target
    TARGET_TOPAGGRO,                                 //Selects targes from top aggro to bottom
    TARGET_BOTTOMAGGRO,                              //Selects targets from bottom aggro to top
    TARGET_NEAREST,
    TARGET_FARTHEST,
};

#define EVENT_MAX_ID    255

typedef struct aiEvent
{
    uint8  id;
    uint32 timer;
    uint32 flags;
    bool   active;
    bool   activeByDefault;
    uint32 phaseMask;
    
    aiEvent(uint32 _id, uint32 _minTimer, uint32 _maxTimer, uint32 _flags, bool _activeByDefault, uint32 _phaseMask) :
        id(_id), flags(_flags), activeByDefault(_activeByDefault), active(activeByDefault), phaseMask(_phaseMask)
    {
        if (_minTimer > _maxTimer) {
            sLog.outError("AIEvent::AIEVent: event %u has minTimer > maxTimer, swapping timers.", id);
            std::swap(_minTimer, _maxTimer);
        }
        
        if (_minTimer != _maxTimer)
            timer = _minTimer + rand()%(_maxTimer - _minTimer);
        else
            timer = _minTimer;
    }
    
    void calcTimer(uint32 minTimer)
    {
        calcTimer(minTimer, minTimer);
    }
    
    void calcTimer(uint32 minTimer, uint32 maxTimer)
    {
        if (minTimer > maxTimer)
            std::swap(minTimer, maxTimer);
            
        if (minTimer != maxTimer)
            timer = minTimer + rand()%(maxTimer - minTimer);
        else
            timer = minTimer;
    }
    
    bool isActiveInPhase(uint32 phase)
    {
        if (phaseMask == 0)
            return true;

        return ((phaseMask & (1 << phase)) != 0);
    }
} AIEvent;

typedef enum eventFlag
{
    EVENT_FLAG_DELAY_IF_CASTING  = 0x1
} EventFlag;

class CreatureAINew
{
    public:
        CreatureAINew(Creature* creature) : me(creature), inCombat(false), m_currEvent(EVENT_MAX_ID), m_phase(0) {}

        ~CreatureAINew();
        
        /* Events handling */
        void addEvent(uint8 id, uint32 minTimer, uint32 maxTimer, uint32 flags = 0, bool activeByDefault = true, uint32 phaseMask = 0);
        void scheduleEvent(uint8 id, uint32 minTimer, uint32 maxTimer);
        void scheduleEvent(uint8 id, uint32 timer) { scheduleEvent(id, timer, timer); }
        void disableEvent(uint8 id);
        void enableEvent(uint8 id);
        void delayEvent(uint8 id, uint32 delay);
        void delayAllEvents(uint32 delay);
        bool executeEvent(uint32 const /*diff*/, uint8& /*id*/);
        void updateEvents(uint32 const /*diff*/);
        // + ensureTimerOnEvents(uint32 minTimer); -> delay events which have timer < minTimer to minTimer
        
        /* Phases handling */
        void setPhase(uint8 phase) { m_phase = phase; onEnterPhase(m_phase); }
        void incrPhase() { m_phase++; onEnterPhase(m_phase); }
        void decrPhase() { m_phase--; onEnterPhase(m_phase); }
        uint8 getPhase() { return m_phase; }
        uint32 phaseMaskForPhase(uint8 phase) { if (phase > 0) return (1 << phase); }

        bool aiInCombat() { return inCombat; }
        void setAICombat(bool on) { inCombat = on; }
        
        /* Target selection */
        Unit* selectUnit(SelectedTarget /*target*/, uint32 /*position*/);
        Unit* selectUnit(SelectedTarget /*target*/, uint32 /*position*/, float /*radius*/, bool /*playersOnly*/);
        void getAllPlayersInRange(std::list<Player*>& /*players*/, float /*range*/);
        
        void doCast(Unit* /*victim*/, uint32 /*spellId*/, bool triggered = false, bool interrupt = false);
        void doTeleportTo(float x, float y, float z, uint32 time = 0);
        void setZoneInCombat();
        uint32 talk(uint8 /*groupid*/, uint64 targetGUID = 0);
        void deleteFromThreatList(uint64 /*guid*/);
        void deleteFromThreatList(Unit* /*target*/);

        /* At every creature update */
        virtual void update(uint32 const /*diff*/);
        bool updateVictim(bool evade = true);
        void doMeleeAttackIfReady();
        /* In Creature::AIM_Initialize() */
        virtual void initialize() { onReset(true); }
        /* When reset (spawn & evade) */
        virtual void onReset(bool /*onSpawn*/) {}
        /* When creature respawn */
        virtual void onRespawn() { onReset(true); }
        /* When entering evade mode */
        virtual void evade();
        /* When reaching home position */
        virtual void onReachedHome() {}
        /* When attacking a new target */
        virtual void attackStart(Unit* /*victim*/);
        /* When entering combat */
        virtual void onCombatStart(Unit* /*victim*/) {}
        /* On death */
        virtual void onDeath(Unit* /*killer*/) {}
        /* When killed a unit */
        virtual void onKill(Unit* /*victim*/) {}
        /* When another unit is moving in line of sight */
        virtual void onMoveInLoS(Unit* /*who*/);
        /* When adding some threat on another unit */
        virtual void onThreatAdd(Unit* /*who*/, float& /*threat*/) {}
        /* When removing some threat from another unit */
        virtual void onThreatRemove(Unit* /*who*/, float& /*threat*/) {}
        /* When changing phase */
        virtual void onEnterPhase(uint32 /*newPhase*/) {}
        /* When taking damage */
        virtual void onDamageTaken(Unit* /*attacker*/, uint32& /*damage*/) {}
        /* When taking heal */
        virtual void onHealingTaken(Unit* /*healer*/, uint32& /*heal*/) {}
        /* When summoning an add */
        virtual void onSummon(Creature* /*summoned*/) {}
        /* When summoned add despawns */
        virtual void onSummonDespawn(Creature* /*summoned*/) {}
        /* When hit by a spell */
        virtual void onHitBySpell(Unit* /*caster*/, SpellEntry const* /*spell*/) {}

    protected:
        Creature* me;
        
        bool inCombat;
        
        typedef std::map<uint8, AIEvent*> EventMap;
        EventMap m_events;
        
        uint8 m_currEvent;
        uint8 m_phase;
};

class Creature_NoMovementAINew : public CreatureAINew
{
    public:
        Creature_NoMovementAINew(Creature* creature) : CreatureAINew(creature) {}
    
        virtual void attackStart(Unit* /*victim*/);
};
 
#endif
