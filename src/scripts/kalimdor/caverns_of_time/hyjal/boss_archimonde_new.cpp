#include "precompiled.h"
#include "def_hyjal.h"

enum {
    YELL_AGGRO  = 0,
    YELL_DOOMFIRE,
    YELL_AIR_BURST,
    YELL_SLAY,
    YELL_ENRAGE,
    YELL_DEATH,
    YELL_SOULCHARGE,
};

enum {
    SPELL_DENOUEMENT_WISP       = 32124,
    SPELL_ANCIENT_SPARK         = 39349,
    SPELL_PROTECTION_OF_ELUNE   = 38528,

    SPELL_DRAIN_WORLD_TREE      = 39140,
    SPELL_DRAIN_WORLD_TREE_2    = 39141,

    SPELL_FINGER_OF_DEATH       = 31984,                    // 20k dmg
    SPELL_HAND_OF_DEATH         = 35354,                    // 99k dmg
    SPELL_AIR_BURST             = 32014,                    // bump
    SPELL_GRIP_OF_THE_LEGION    = 31972,                    // dot, 2500/sec, 5 min
    
    SPELL_DOOMFIRE_STRIKE       = 31903,                    // summons two creatures
    SPELL_DOOMFIRE_SPAWN        = 32074,                    // spawn visual
    SPELL_DOOMFIRE_VISUAL       = 42344,                    // fire visual
    SPELL_DOOMFIRE_DAMAGE       = 31944,
    SPELL_DOOMFIRE_TRIGGER      = 31943,                    // triggers 31944 every second
    SPELL_DOOMFIRE_TRIG_TRIG    = 31945,                    // triggers 31943 every second (seems to be the one to use on the not walking fire)
    
    SPELL_SOUL_CHARGE_YELLOW    = 32045,
    SPELL_SOUL_CHARGE_GREEN     = 32051,
    SPELL_SOUL_CHARGE_RED       = 32052,
    
    SPELL_UNLEASH_SOUL_YELLOW   = 32054,
    SPELL_UNLEASH_SOUL_GREEN    = 32057,
    SPELL_UNLEASH_SOUL_RED      = 32053,
    
    SPELL_FEAR                  = 31970
};

enum {
    CREATURE_ARCHIMONDE             = 17968,
    CREATURE_DOOMFIRE               = 18095,
    CREATURE_DOOMFIRE_TARGETING     = 18104,
    CREATURE_ANCIENT_WISP           = 17946,
    CREATURE_CHANNEL_TARGET         = 22418
};

#define NORDRASSIL_X        5503.713
#define NORDRASSIL_Y       -3523.436
#define NORDRASSIL_Z        1608.781

class Mob_Ancient_Whisp : public CreatureScript
{
public:
    Mob_Ancient_Whisp() : CreatureScript("mob_ancient_whisp") {}
    
    class Mob_Ancient_WhispAI : public CreatureAINew
    {
    public:
        enum events
        {
            EV_CHECK    = 0
        };
        
        Mob_Ancient_WhispAI(Creature* creature) : CreatureAINew(creature)
        {
            _instance = ((ScriptedInstance*)creature->GetInstanceData());
        }
        
        void onReset(bool onSpawn)
        {
            if (onSpawn)
                addEvent(EV_CHECK, 1000, 1000);
            else
                scheduleEvent(EV_CHECK, 1000);
            
            _archimondeGUID = 0;
            
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        }
        
        void onDamageTaken(Unit* /*attacker*/, uint32& damage)
        {
            damage = 0;
        }
        
        void update(uint32 const diff)
        {
            if (!_archimondeGUID) {
                if (_instance)
                    _archimondeGUID = _instance->GetData64(DATA_ARCHIMONDE);
            }
            
            updateEvents(diff);
            
            while (executeEvent(diff, m_currEvent)) {
                switch (m_currEvent) {
                case EV_CHECK:
                    if (!_archimondeGUID)
                        break;
                    
                    if (Creature* archimonde = Creature::GetCreature(*me, _archimondeGUID)) {
                        if (archimonde->IsBelowHPPercent(2.0f) || archimonde->isDead())
                            doCast(me, SPELL_DENOUEMENT_WISP);
                        else
                            doCast(archimonde, SPELL_ANCIENT_SPARK);
                    }
                    
                    scheduleEvent(EV_CHECK, 1000);
                    
                    break;
                }
            }
        }
    
    private:
        ScriptedInstance* _instance;
        
        uint64 _archimondeGUID;
    };
    
    CreatureAINew* getAI(Creature* creature)
    {
        return new Mob_Ancient_WhispAI(creature);
    }
};

class Mob_Walking_Doomfire : public CreatureScript
{
public:
    Mob_Walking_Doomfire() : CreatureScript("mob_walking_doomfire") {}
    
    class Mob_Walking_DoomfireAI : public CreatureAINew
    {
    public:
        enum events
        {
            EV_CHECK    = 0,
            EV_CHANGE_TARGET
        };
    
        Mob_Walking_DoomfireAI(Creature* creature) : CreatureAINew(creature)
        {
            _instance = ((ScriptedInstance*)creature->GetInstanceData());
        }
        
        void onReset(bool onSpawn)
        {
            if (onSpawn) {
                addEvent(EV_CHECK, 3000, 3000);
                addEvent(EV_CHANGE_TARGET, 500, 500);
            }
            else {
                scheduleEvent(EV_CHECK, 3000);
                scheduleEvent(EV_CHANGE_TARGET, 500);
            }
            
            _archimondeGUID = 0;
            
            doCast(me, SPELL_DOOMFIRE_SPAWN, true);
            doCast(me, SPELL_DOOMFIRE_TRIG_TRIG);
        }
        
        void onDamageTaken(Unit* /*attacker*/, uint32& damage)
        {
            damage = 0;
        }
        
        void update(uint32 const diff)
        {
            me->addUnitState(UNIT_STAT_IGNORE_PATHFINDING);

            if (!_archimondeGUID) {
                if (_instance)
                    _archimondeGUID = _instance->GetData64(DATA_ARCHIMONDE);
            }
            
            if (Unit* target = me->getVictim()) {
                if (!target->isInAccessiblePlaceFor(me) || me->IsWithinMeleeRange(target))
                    scheduleEvent(EV_CHANGE_TARGET, 1);
            }
            
            updateEvents(diff);
            
            while (executeEvent(diff, m_currEvent)) {
                switch (m_currEvent) {
                case EV_CHECK:
                {
                    Unit* archimonde = Unit::GetUnit(*me, _archimondeGUID);
                    if (!archimonde || archimonde->isDead() || !archimonde->isInCombat())
                        me->DisappearAndDie();
                        
                    scheduleEvent(EV_CHECK, 1000);
                    
                    break;
                }
                case EV_CHANGE_TARGET:
                {
                    doResetThreat();
                    Unit* target = NULL;
                    if (Creature* archimonde = Creature::GetCreature(*me, _archimondeGUID))
                        target = archimonde->getAI()->selectUnit(SELECT_TARGET_RANDOM, 0);

                    if (target) {
                        me->AddThreat(target, 1000000.0f);
                        me->GetMotionMaster()->MoveFollow(target, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
                    }
                    
                    scheduleEvent(EV_CHANGE_TARGET, 5000);

                    break;
                }
                }
            }
        }
        
    private:
        ScriptedInstance* _instance;
        
        uint64 _archimondeGUID;
    };
    
    CreatureAINew* getAI(Creature* creature)
    {
        return new Mob_Walking_DoomfireAI(creature);
    }
};

// This is used to sort by distance in order to see who is the closest target, when checking for Finger of Death
struct TargetDistanceOrder : public std::binary_function<const Unit, const Unit, bool>
{
    const Unit* MainTarget;
    TargetDistanceOrder(const Unit* Target) : MainTarget(Target) {};
    // functor for operator "<"
    bool operator()(const Unit* _Left, const Unit* _Right) const
    {
        return (MainTarget->GetDistance(_Left) < MainTarget->GetDistance(_Right));
    }
};

class Boss_Archimonde : public CreatureScript
{
public:
    Boss_Archimonde() : CreatureScript("boss_archimonde") {}
    
    class Boss_ArchimondeAI : public CreatureAINew
    {
    public:
        enum events
        {
            EV_FEAR = 0,
            EV_AIR_BURST,
            EV_GRIP_LEGION,
            EV_DOOMFIRE,
            EV_MELEE_CHECK,
            EV_NORDRASSIL_CHECK,
            EV_ENRAGE,
            EV_ENRAGE_CAST,
            EV_UNLEASH_SOULCHARGE,
            EV_UNDER_10_PERCENT
        };
    
        Boss_ArchimondeAI(Creature* creature) : CreatureAINew(creature)
        {
            _instance = ((ScriptedInstance*)creature->GetInstanceData());
        }
        
        void onReset(bool onSpawn)
        {
            if (onSpawn) {
                addEvent(EV_FEAR, 42000, 42000, EVENT_FLAG_DELAY_IF_CASTING);
                addEvent(EV_AIR_BURST, 30000, 30000, EVENT_FLAG_DELAY_IF_CASTING);
                addEvent(EV_GRIP_LEGION, 5000, 25000, EVENT_FLAG_DELAY_IF_CASTING);
                addEvent(EV_DOOMFIRE, 20000, 20000, EVENT_FLAG_DELAY_IF_CASTING);
                addEvent(EV_MELEE_CHECK, 2000, 2000);
                addEvent(EV_NORDRASSIL_CHECK, 3000, 3000);
                addEvent(EV_ENRAGE, 600000, 600000);
                addEvent(EV_ENRAGE_CAST, 2000, 2000, EVENT_FLAG_NONE, false);
                addEvent(EV_UNLEASH_SOULCHARGE, 2000, 30000, EVENT_FLAG_DELAY_IF_CASTING, false);
                addEvent(EV_UNDER_10_PERCENT, 1000, 1000, EVENT_FLAG_DELAY_IF_CASTING, false);
            }
            else {
                scheduleEvent(EV_FEAR, 42000);
                scheduleEvent(EV_AIR_BURST, 30000);
                scheduleEvent(EV_GRIP_LEGION, 5000, 25000);
                scheduleEvent(EV_DOOMFIRE, 20000);
                scheduleEvent(EV_MELEE_CHECK, 2000);
                scheduleEvent(EV_NORDRASSIL_CHECK, 3000);
                enableEvent(EV_NORDRASSIL_CHECK);
                scheduleEvent(EV_ENRAGE, 600000);
                enableEvent(EV_ENRAGE);
                scheduleEvent(EV_ENRAGE_CAST, 2000);
                disableEvent(EV_ENRAGE_CAST);
                scheduleEvent(EV_UNLEASH_SOULCHARGE, 2000, 30000);
                disableEvent(EV_UNLEASH_SOULCHARGE);
                scheduleEvent(EV_UNDER_10_PERCENT, 1000, 1000);
                disableEvent(EV_UNDER_10_PERCENT);
            }

            _enraged = false;
            _under10Percent = false;
            _checkTimer = 1000;

            me->RemoveAurasDueToSpell(SPELL_SOUL_CHARGE_YELLOW);
            me->RemoveAurasDueToSpell(SPELL_SOUL_CHARGE_RED);
            me->RemoveAurasDueToSpell(SPELL_SOUL_CHARGE_GREEN);

            _chargeCount = 0;

            if (_instance)
                _instance->SetData(DATA_ARCHIMONDEEVENT, NOT_STARTED);
        }
        
        void message(uint32 type, uint32 data)
        {
            if (type == 0) {
                talk(YELL_SLAY);
                switch (data) {
                case CLASS_PRIEST:
                case CLASS_PALADIN:
                case CLASS_WARLOCK:
                    doCast(me, SPELL_SOUL_CHARGE_RED, true);
                    break;
                case CLASS_MAGE:
                case CLASS_ROGUE:
                case CLASS_WARRIOR:
                    doCast(me, SPELL_SOUL_CHARGE_YELLOW, true);
                    break;
                case CLASS_DRUID:
                case CLASS_SHAMAN:
                case CLASS_HUNTER:
                    doCast(me, SPELL_SOUL_CHARGE_GREEN, true);
                    break;
                }
                
                ++_chargeCount;
                enableEvent(EV_UNLEASH_SOULCHARGE);
            }
        }
        
        void onDeath(Unit* /*killer*/)
        {
            talk(YELL_DEATH);
            if (_instance)
                _instance->SetData(DATA_ARCHIMONDEEVENT, DONE);
        }
        
        void onCombatStart(Unit* /*victim*/)
        {
            if (_instance) {
                _instance->SendScriptInTestNoLootMessageToAll();
                _instance->SetData(DATA_ARCHIMONDEEVENT, IN_PROGRESS);
            }
        }
        
        void update(uint32 const diff)
        {
            if (!me->isInCombat()) {
                if (_checkTimer <= diff) {
                    // Visibility check
                    if ((_instance->GetData(DATA_AZGALOREVENT) < DONE) && ((me->GetVisibility() != VISIBILITY_OFF) || (me->getFaction() != 35))) {
                        me->SetVisibility(VISIBILITY_OFF);
                        me->setFaction(35);
                    }
                    else if ((_instance->GetData(DATA_AZGALOREVENT) >= DONE) && ((me->GetVisibility() != VISIBILITY_ON) || (me->getFaction() == 35))) {
                        me->setFaction(1720);
                        me->SetVisibility(VISIBILITY_ON);
                    }
                    
                    // Refresh channeling visual
                    if (!me->m_currentSpells[CURRENT_CHANNELED_SPELL]) {
                        if (Creature* trigger = me->SummonCreature(CREATURE_CHANNEL_TARGET, NORDRASSIL_X, NORDRASSIL_Y, NORDRASSIL_Z, 0, TEMPSUMMON_MANUAL_DESPAWN, 0))
                            doCast(trigger, SPELL_DRAIN_WORLD_TREE);
                    }
                    
                    if (Creature* trigger = me->SummonCreature(CREATURE_CHANNEL_TARGET, NORDRASSIL_X, NORDRASSIL_Y, NORDRASSIL_Z, 0, TEMPSUMMON_TIMED_DESPAWN, 2000)) {
                        trigger->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        trigger->SetUInt32Value(UNIT_FIELD_DISPLAYID, 11686);
                        trigger->CastSpell(me, SPELL_DRAIN_WORLD_TREE_2, true);
                    }
                    
                    _checkTimer = 1000;
                }
                else
                    _checkTimer -= diff;
                
                return;
            }
            
            if (!updateVictim())
                return;
            
            if (me->IsBelowHPPercent(10.0f)) {
                _under10Percent = true;
                enableEvent(EV_UNDER_10_PERCENT);
                doCast(me->getVictim(), SPELL_PROTECTION_OF_ELUNE);
            }
                
            updateEvents(diff);
            
            while (executeEvent(diff, m_currEvent)) {
                switch (m_currEvent) {
                case EV_FEAR:
                    doCast(me->getVictim(), SPELL_FEAR);
                    scheduleEvent(EV_FEAR, 42000);
                    break;
                case EV_AIR_BURST:
                    talk(YELL_AIR_BURST);
                    doCast(selectUnit(SELECT_TARGET_RANDOM, 1), SPELL_AIR_BURST);
                    scheduleEvent(EV_AIR_BURST, 25000, 40000);
                    break;
                case EV_GRIP_LEGION:
                    doCast(selectUnit(SELECT_TARGET_RANDOM, 0, 100.0f, true), SPELL_GRIP_OF_THE_LEGION);
                    scheduleEvent(EV_GRIP_LEGION, 5000, 25000);
                    break;
                case EV_DOOMFIRE:
                    /*float x, y, z;
                    me->GetRandomPoint(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 40.0f, x, y, z);
                    if (Creature* doomfire = me->SummonCreature(CREATURE_DOOMFIRE_TARGETING, x, y, z, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 30000)) {
                        doomfire->setFaction(me->getFaction());
                        me->CastSpell(x, y, z, SPELL_DOOMFIRE_SPAWN, true);
                        talk(YELL_DOOMFIRE);
                    }*/
                    doCast(me, SPELL_DOOMFIRE_STRIKE);
                    scheduleEvent(EV_DOOMFIRE, 40000);
                    break;
                case EV_MELEE_CHECK:
                    if (_canUseFingerOfDeath())
                        doCast(selectUnit(SELECT_TARGET_RANDOM, 0), SPELL_FINGER_OF_DEATH);
                        
                    scheduleEvent(EV_MELEE_CHECK, 2000);
                    break;
                case EV_NORDRASSIL_CHECK:
                    if (me->GetDistance(NORDRASSIL_X, NORDRASSIL_Y, NORDRASSIL_Z) < 75.0f)
                        scheduleEvent(EV_ENRAGE, 1);
                    
                    scheduleEvent(EV_NORDRASSIL_CHECK, 2000);
                        
                    break;
                case EV_ENRAGE:
                    disableEvent(EV_NORDRASSIL_CHECK);
                    disableEvent(EV_ENRAGE);
                    enableEvent(EV_ENRAGE_CAST);
                    talk(YELL_ENRAGE);
                    me->GetMotionMaster()->Clear(false);
                    me->GetMotionMaster()->MoveIdle();
                    _enraged = true;
                    break;
                case EV_ENRAGE_CAST:
                    doCast(me->getVictim(), SPELL_HAND_OF_DEATH);
                    scheduleEvent(EV_ENRAGE_CAST, 2000);
                    break;
                case EV_UNLEASH_SOULCHARGE:
                {
                    std::list<uint32> unleashSpells;
                    if (me->HasAura(SPELL_SOUL_CHARGE_GREEN))
                        unleashSpells.push_back(SPELL_UNLEASH_SOUL_GREEN);
                    if (me->HasAura(SPELL_SOUL_CHARGE_RED))
                        unleashSpells.push_back(SPELL_UNLEASH_SOUL_RED);
                    if (me->HasAura(SPELL_SOUL_CHARGE_YELLOW))
                        unleashSpells.push_back(SPELL_UNLEASH_SOUL_YELLOW);
                    
                    if (unleashSpells.empty()) { // Something went wrong
                        _chargeCount = 0;
                        disableEvent(EV_UNLEASH_SOULCHARGE);
                    }
                        
                    Trinity::RandomResizeList(unleashSpells, 1);
                    doCast(me, unleashSpells.front());
                    switch (unleashSpells.front()) {
                    case SPELL_UNLEASH_SOUL_GREEN:
                        me->RemoveSingleAuraFromStack(SPELL_SOUL_CHARGE_GREEN, 0);
                        break;
                    case SPELL_UNLEASH_SOUL_RED:
                        me->RemoveSingleAuraFromStack(SPELL_SOUL_CHARGE_RED, 0);
                        break;
                    case SPELL_UNLEASH_SOUL_YELLOW:
                        me->RemoveSingleAuraFromStack(SPELL_SOUL_CHARGE_YELLOW, 0);
                        break;
                    }
                    
                    --_chargeCount;
                    if (_chargeCount)
                        scheduleEvent(EV_UNLEASH_SOULCHARGE, 2000, 30000);
                    else
                        disableEvent(EV_UNLEASH_SOULCHARGE);
                    break;
                }
                case EV_UNDER_10_PERCENT:
                    doCast(me, SPELL_HAND_OF_DEATH);
                    scheduleEvent(EV_UNDER_10_PERCENT, 1000);
                    break;
                }
            }
            
            doMeleeAttackIfReady();
        }
        
    private:
        bool _canUseFingerOfDeath()
        {
            Unit* victim = me->getVictim();
            if (victim && me->IsWithinDistInMap(victim, me->GetAttackDistance(victim)))
                return false;
                
            if (victim && victim->isAlive()) {
                float x, y, z, zHeightMap;
                me->GetPosition(x, y, z);
                zHeightMap = me->GetMap()->GetHeight(x, y, z);

                // Victim is flying
                if ((z - zHeightMap) > 5.0f)
                    return false;
            }
            
            std::list<HostilReference*>& threatList = me->getThreatManager().getThreatList();
            if (threatList.empty())
                return false;
                
            std::list<Unit*> targets;
            std::list<HostilReference*>::iterator itr = threatList.begin();
            for (; itr != threatList.end(); ++itr) {
                Unit* unit = Unit::GetUnit((*me), (*itr)->getUnitGuid());
                if (unit && unit->isAlive())
                    targets.push_back(unit);
            }

            if (targets.empty())
                return false;

            targets.sort(TargetDistanceOrder(me));
            Unit* target = targets.front();
            if (target) {
                if (!me->IsWithinDistInMap(target, me->GetAttackDistance(target)) && abs(me->GetPositionZ() - target->GetPositionZ()) < 5.0f)
                    return true; // Cast Finger of Death
                else // This target is closest, he is our new tank
                    me->AddThreat(target, doGetThreat(me->getVictim()));
            }

            return false;
        }
    
        ScriptedInstance* _instance;
        
        bool _enraged;
        bool _under10Percent;
        
        uint32 _checkTimer;
        uint32 _chargeCount;
    };
    
    CreatureAINew* getAI(Creature* creature)
    {
        return new Boss_ArchimondeAI(creature);
    }
};

void AddSC_boss_archimonde_new()
{
    sScriptMgr.addScript(new Mob_Ancient_Whisp());
    sScriptMgr.addScript(new Mob_Walking_Doomfire());
    sScriptMgr.addScript(new Boss_Archimonde());
}
