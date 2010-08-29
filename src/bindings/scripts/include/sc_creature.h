/* Copyright (C) 2008 Trinity <http://www.trinitycore.org/>
 *
 * Thanks to the original authors: ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 *
 * This program is free software licensed under GPL version 2
 * Please see the included DOCS/LICENSE.TXT for more information */

#ifndef SC_CREATURE_H
#define SC_CREATURE_H

#include "CreatureAI.h"
#include "Creature.h"
#include "CreatureAIImpl.h"

#define SCRIPT_CAST_TYPE dynamic_cast

#define CAST_PLR(a)     (SCRIPT_CAST_TYPE<Player*>(a))
#define CAST_CRE(a)     (SCRIPT_CAST_TYPE<Creature*>(a))
#define CAST_SUM(a)     (SCRIPT_CAST_TYPE<TempSummon*>(a))
#define CAST_PET(a)     (SCRIPT_CAST_TYPE<Pet*>(a))
#define CAST_AI(a,b)    (SCRIPT_CAST_TYPE<a*>(b))

#define GET_SPELL(a)    (const_cast<SpellEntry*>(GetSpellStore()->LookupEntry(a)))

float GetSpellMaxRange(uint32 id);

class SummonList : std::list<uint64>
{
public:
    SummonList(Creature* creature) : m_creature(creature) {}
    void Summon(Creature *summon) {push_back(summon->GetGUID());}
    void Despawn(Creature *summon);
    void DespawnEntry(uint32 entry);
    void DespawnAll();
    bool IsEmpty();
private:
    Creature *m_creature;
};

//Get a single creature of given entry
Unit* FindCreature(uint32 entry, float range, Unit* Finder);

//Get a single gameobject of given entry
GameObject* FindGameObject(uint32 entry, float range, Unit* Finder);

struct TRINITY_DLL_DECL ScriptedAI : public CreatureAI
{
    ScriptedAI(Creature* creature) : CreatureAI(creature), m_creature(creature), InCombat(false), IsFleeing(false) {}
    ~ScriptedAI() {}

    //*************
    //CreatureAI Functions
    //*************

    //Called at each attack of m_creature by any victim
    void AttackStart(Unit *);
    void AttackStart(Unit *, bool melee);
    void AttackStartNoMove(Unit *pTarget);

    //Called at stoping attack by any attacker
    void EnterEvadeMode();

    // Called at any Damage from any attacker (before damage apply)
    void DamageTaken(Unit *done_by, uint32 &damage) {}
    
    void HealReceived(Unit* /*done_by*/, uint32& /*addhealth*/) {}

    //Called at World update tick
    void UpdateAI(const uint32);

    //Called at creature death
    void JustDied(Unit*){}

    //Called at creature killing another unit
    void KilledUnit(Unit*){}

    // Called when the creature summon successfully other creature
    void JustSummoned(Creature* ) {}

    // Called when a summoned creature is despawned
    void SummonedCreatureDespawn(Creature* /*unit*/) {}

    // Called when hit by a spell
    void SpellHit(Unit* caster, const SpellEntry*) {}

    // Called when spell hits a target
    void SpellHitTarget(Unit* target, const SpellEntry*) {}

    // Called when creature is spawned or respawned (for reseting variables)
    void JustRespawned();

    //Called at waypoint reached or PointMovement end
    void MovementInform(uint32, uint32){}

    // Called when AI is temporarily replaced or put back when possess is applied or removed
    void OnPossess(bool apply) {}

    //*************
    // Variables
    //*************

    //Pointer to creature we are manipulating
    Creature* m_creature;

    //Bool for if we are in combat or not
    bool InCombat;

    //For fleeing
    bool IsFleeing;

    //*************
    //Pure virtual functions
    //*************

    //Called at creature reset either by death or evade
    void Reset() {}

    //Called at creature aggro either by MoveInLOS or Attack Start
    virtual void Aggro(Unit*) = 0;

    //*************
    //AI Helper Functions
    //*************

    //Start movement toward victim
    void DoStartMovement(Unit* victim, float distance = 0, float angle = 0);

    //Start no movement on victim
    void DoStartNoMovement(Unit* victim);

    //Stop attack of current victim
    void DoStopAttack();

    //Cast spell by Id
    void DoCast(Unit* victim, uint32 spellId, bool triggered = false);
    void DoCastAOE(uint32 spellId, bool triggered = false);

    //Cast spell by spell info
    void DoCastSpell(Unit* who,SpellEntry const *spellInfo, bool triggered = false);

    //Creature say
    void DoSay(const char* text, uint32 language, Unit* target, bool SayEmote = false);

    //Creature Yell
    void DoYell(const char* text, uint32 language, Unit* target);

    //Creature Text emote, optional bool for boss emote text
    void DoTextEmote(const char* text, Unit* target, bool IsBossEmote = false);

    //Creature whisper, optional bool for boss whisper
    void DoWhisper(const char* text, Unit* reciever, bool IsBossWhisper = false);

    //Plays a sound to all nearby players
    void DoPlaySoundToSet(Unit* unit, uint32 sound);

    //Places the entire map into combat with creature
    void DoZoneInCombat(Unit* pUnit = 0);

    //Drops all threat to 0%. Does not remove players from the threat list
    void DoResetThreat();

    float DoGetThreat(Unit *u);
    void DoModifyThreatPercent(Unit *pUnit, int32 pct);

    void DoTeleportTo(float x, float y, float z, uint32 time = 0);

    void DoAction(const int32 param) {}

    //Teleports a player without dropping threat (only teleports to same map)
    void DoTeleportPlayer(Unit* pUnit, float x, float y, float z, float o);
    void DoTeleportAll(float x, float y, float z, float o);

    //Returns friendly unit with the most amount of hp missing from max hp
    Unit* DoSelectLowestHpFriendly(float range, uint32 MinHPDiff = 1);

    //Returns a list of friendly CC'd units within range
    std::list<Creature*> DoFindFriendlyCC(float range);

    //Returns a list of all friendly units missing a specific buff within range
    std::list<Creature*> DoFindFriendlyMissingBuff(float range, uint32 spellid);

    //Spawns a creature relative to m_creature
    Creature* DoSpawnCreature(uint32 id, float x, float y, float z, float angle, uint32 type, uint32 despawntime);

    //Selects a unit from the creature's current aggro list
    Unit* SelectUnit(SelectAggroTarget target, uint32 position);
    Unit* SelectUnit(SelectAggroTarget target, uint32 position, float dist, bool playerOnly);
    Unit* SelectUnit(SelectAggroTarget target, uint32 position, float distNear, float distFar, bool playerOnly);
    Unit* SelectUnit(uint32 position, float dist, bool playerOnly, bool auraCheck, bool exceptPossesed, uint32 spellId, uint32 effIndex);
    void SelectUnitList(std::list<Unit*> &targetList, uint32 num, SelectAggroTarget target, float dist, bool playerOnly);

    //Returns spells that meet the specified criteria from the creatures spell list
    SpellEntry const* SelectSpell(Unit* Target, int32 School, int32 Mechanic, SelectTarget Targets,  uint32 PowerCostMin, uint32 PowerCostMax, float RangeMin, float RangeMax, SelectEffect Effect);

    //Checks if you can cast the specified spell
    bool CanCast(Unit* Target, SpellEntry const *Spell, bool Triggered = false);
    
    void SetEquipmentSlots(bool bLoadDefault, int32 uiMainHand = EQUIP_NO_CHANGE, int32 uiOffHand = EQUIP_NO_CHANGE, int32 uiRanged = EQUIP_NO_CHANGE);
    
    //Generally used to control if MoveChase() is to be used or not in AttackStart(). Some creatures does not chase victims
    void SetCombatMovement(bool CombatMove);
    bool IsCombatMovement() { return m_bCombatMovement; }
    
    private:
        bool m_bCombatMovement;
};

struct TRINITY_DLL_DECL Scripted_NoMovementAI : public ScriptedAI
{
    Scripted_NoMovementAI(Creature* creature) : ScriptedAI(creature) {}

    //Called if IsVisible(Unit *who) is true at each *who move
    //void MoveInLineOfSight(Unit *);

    //Called at each attack of m_creature by any victim
    void AttackStart(Unit *);
};

struct TRINITY_DLL_DECL NullCreatureAI : public ScriptedAI
{
    NullCreatureAI(Creature* c) : ScriptedAI(c) {}
    ~NullCreatureAI() {}

    void Reset() {}
    void Aggro(Unit*) {}
    void MoveInLineOfSight(Unit *) {}
    void AttackStart(Unit *) {}
    void EnterEvadeMode() {}
    bool IsVisible(Unit *) const { return false; }

    void UpdateAI(const uint32) {}
};

#endif

