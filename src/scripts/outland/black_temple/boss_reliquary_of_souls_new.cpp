#include "precompiled.h"
#include "def_black_temple.h"
#include "CreatureScript.h"
#include "CreatureAINew.h"
#include "Spell.h"

enum ReliquaryOfSoulsData {
    // Essence of Suffering
    TALK_SUFF_SAY_FREED             = 0,
    TALK_SUFF_SAY_AGGRO             = 1,
    TALK_SUFF_SAY_SLAY              = 2,
    TALK_SUFF_SAY_RECAP             = 3,
    TALK_SUFF_SAY_AFTER             = 4,
    TALK_SUFF_EMOTE_ENRAGE          = 5,
    // Essence of Desire
    TALK_DESI_SAY_FREED             = 0,
    TALK_DESI_SAY_SLAY              = 1,
    TALK_DESI_SAY_SPEC              = 2,
    TALK_DESI_SAY_RECAP             = 3,
    TALK_DESI_SAY_AFTER             = 4,
    // Essence of Anger
    TALK_ANGER_SAY_FREED            = 0,
    TALK_ANGER_SAY_SLAY             = 1,
    TALK_ANGER_SAY_SPEC             = 2,
    TALK_ANGER_SAY_BEFORE           = 3,
    TALK_ANGER_SAY_DEATH            = 4,
    
    // Spells
    AURA_OF_SUFFERING               = 41292,
    AURA_OF_SUFFERING_ARMOR         = 42017, // linked aura, need core support
    ESSENCE_OF_SUFFERING_PASSIVE    = 41296, // periodic trigger 41294
    ESSENCE_OF_SUFFERING_PASSIVE2   = 41623,
    SPELL_FIXATE_TARGET             = 41294, // dummy, select target
    SPELL_FIXATE_TAUNT              = 41295, // force taunt
    SPELL_ENRAGE                    = 41305,
    SPELL_SOUL_DRAIN                = 41303,

    AURA_OF_DESIRE                  = 41350,
    AURA_OF_DESIRE_DAMAGE           = 41352,
    SPELL_RUNE_SHIELD               = 41431,
    SPELL_DEADEN                    = 41410,
    SPELL_SOUL_SHOCK                = 41426,

    AURA_OF_ANGER                   = 41337,
    SPELL_SELF_SEETHE               = 41364, // force cast 41520
    SPELL_ENEMY_SEETHE              = 41520,
    SPELL_SOUL_SCREAM               = 41545,
    SPELL_SPITE_TARGET              = 41376, // cast 41377 after 6 sec
    SPELL_SPITE_DAMAGE              = 41377,

    ENSLAVED_SOUL_PASSIVE           = 41535,
    SPELL_SOUL_RELEASE              = 41542,
    SPELL_SUBMERGE                  = 37550, //dropout 'head'
    
    // Misc
    CREATURE_ENSLAVED_SOUL          = 23469,
    NUMBER_ENSLAVED_SOUL            = 8,
    DATA_SOUL_DEATH                 = 0,
    
    // Phases
    PHASE_NONE                      = 0,
    PHASE_SUFFERING                 = 1,
    PHASE_DESIRE                    = 2,
    PHASE_ANGER                     = 3
};

static float soulsPos[][2] = { {450.4, 212.3},
                               {542.1, 212.3},
                               {542.1, 168.3},
                               {542.1, 137.4},
                               {450.4, 137.4},
                               {450.4, 168.3} };

class Boss_reliquary_of_souls : public CreatureScript
{
public:
    Boss_reliquary_of_souls() : CreatureScript("boss_reliquary_of_souls") {}
    
    class Boss_reliquary_of_soulsAI : public Creature_NoMovementAINew
    {
    public:
        Boss_reliquary_of_soulsAI(Creature* creature) : Creature_NoMovementAINew(creature)
        {
            instance = ((ScriptedInstance*)creature->GetInstanceData());
            essenceGUID = 0;
            phase = PHASE_NONE;
            step = 0;
            timer = 0;
        }
        
        void onReset(bool onSpawn)
        {
            if (instance && instance->GetData(DATA_RELIQUARYOFSOULSEVENT) != DONE)
                instance->SetData(DATA_RELIQUARYOFSOULSEVENT, NOT_STARTED);

            if (essenceGUID != 0) {
                Creature* essence = Creature::GetCreature(*me, essenceGUID);
                if (essence)
                    essence->DisappearAndDie();
                    
                essenceGUID = 0;
            }

            phase = PHASE_NONE;
            
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_NONE);
            me->RemoveAurasDueToSpell(SPELL_SUBMERGE);
            
            if (instance)
                instance->RemoveAuraOnAllPlayers(SPELL_ENEMY_SEETHE);
                
            me->SetNoCallAssistance(true);
        }
        
        void onCombatStart(Unit* victim)
        {
            me->AddThreat(victim, 10000.0f);
            setZoneInCombat();
            
            if (instance)
                instance->SetData(DATA_RELIQUARYOFSOULSEVENT, IN_PROGRESS);
                
            phase = PHASE_SUFFERING;
            step = 0;
            timer = 0;
        }
        
        bool summonSoul()
        {
            uint32 random = rand()%6;
            
            //float x = soulsPos[random][0];
            Creature* soul = me->SummonCreature(CREATURE_ENSLAVED_SOUL, soulsPos[random][0], soulsPos[random][1], me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_CORPSE_DESPAWN, 0);
            if (!soul)
                return false;
                
            if (Unit* target = selectUnit(SELECT_TARGET_RANDOM, 0)) {
                soul->SetSummoner(me);
                if (soul->getAI())
                    soul->getAI()->attackStart(target);
            }
            else
                evade();
                
            return true;
        }
        
        // Used to transfer threat between phases
        void mergeThreatList(Creature* essence)
        {
            if (!essence)
                return;
                
            std::list<HostilReference*>& threatlist = essence->getThreatManager().getThreatList();
            std::list<HostilReference*>::iterator itr = threatlist.begin();
            for (; itr != threatlist.end(); itr++) {
                Unit* unit = Unit::GetUnit(*me, (*itr)->getUnitGuid());
                if (unit) {
                    doModifyThreatPercent(unit, -100);
                    float threat = essence->getThreatManager().getThreat(unit);
                    me->AddThreat(unit, threat);
                }
            }
        }
        
        void onDeath(Unit* killer)
        {
            if (instance) {
                instance->SetData(DATA_RELIQUARYOFSOULSEVENT, DONE);
                instance->RemoveAuraOnAllPlayers(SPELL_ENEMY_SEETHE);
            }
        }
        
        void message(uint32 id, uint32 data)
        {
            if (id == DATA_SOUL_DEATH && data == 1)
                soulDeathCount++;
        }
        
        void onMoveInLoS(Unit* who)
        {
            if (!me->isInCombat() && who->ToPlayer() && who->GetDistance(me) <= 75.0f)
                attackStart(who);
        }
        
        void update(uint32 const diff)
        {
            if (phase == PHASE_NONE)
                return;
                
            if (me->getThreatManager().getThreatList().empty()) {
                evade();
                return;
            }
            
            Creature* essence;
            if (essenceGUID != 0) {
                essence = Creature::GetCreature(*me, essenceGUID);
                if (!essence) {
                    evade();
                    return;
                }
            }
            
            if (timer <= diff) {
                switch (step) {
                case 0:
                    me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_READY2H);
                    timer = 3000;
                    break;
                case 1:
                    me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_SUBMERGE);
                    doCast(me, SPELL_SUBMERGE);
                    timer = 2800;
                    break;
                case 2:
                    if (Creature* summon = me->SummonCreature(23417 + phase, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_DEAD_DESPAWN, 0)) {
                        me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_SUBMERGED);
                        if (summon->getAI()) {
                            summon->getAI()->attackStart(selectUnit(SELECT_TARGET_TOPAGGRO, 0));
                            essenceGUID = summon->GetGUID();
                            summon->SetSummoner(me);
                            me->GetMotionMaster()->MoveIdle();
                        }
                    }
                    else
                        evade();
                    timer = 5000;
                    break;
                case 3: // FIXME: How dumb is this..
                    if (phase == PHASE_ANGER) {
                        if (!essence->isAlive())
                            doCast(me, 7, true);
                        else
                            return;
                    }
                    else {
                        if (essence->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE)) {
                            //mergeThreatList(essence);
                            essence->DeleteThreatList();
                            essence->SetReactState(REACT_PASSIVE);
                            essence->GetMotionMaster()->MoveFollow(me, 0, 0);
                        }
                        else
                            return;
                    }
                    timer = 1000;
                    break;
                case 4:
                    timer = 500;
                    if (essence->IsWithinDistInMap(me, 10.0f)) {
                        timer = 2000;
                        me->RemoveAurasDueToSpell(SPELL_SUBMERGE);
                        essence->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_SUBMERGE); // Rotate and disappear
                    }
                    else { // Really necessary?
                        //mergeThreatList(essence);
                        essence->DeleteThreatList();
                        essence->SetReactState(REACT_PASSIVE);
                        essence->GetMotionMaster()->MoveFollow(me, 0, 0);
                        return;
                    }
                    break;
                case 5:
                    if (phase == PHASE_SUFFERING) {
                        if (essence->getAI())
                            essence->getAI()->talk(TALK_SUFF_SAY_AFTER);
                    }
                    else {
                        if (essence->getAI())
                            essence->getAI()->talk(TALK_DESI_SAY_AFTER);
                    }
                    
                    essence->SetVisibility(VISIBILITY_OFF);
                    essence->setDeathState(DEAD);
                    me->SetUInt32Value(UNIT_NPC_EMOTESTATE, 0);
                    essenceGUID = 0;
                    soulCount = 0;
                    soulDeathCount = 0;
                    timer = 3000;
                    break;
                case 6:
                    if (soulCount < NUMBER_ENSLAVED_SOUL) {
                        if (summonSoul())
                            soulCount++;
                            
                        timer = 500;
                        return;
                    }
                    break;
                case 7:
                    if (soulDeathCount >= soulCount) {
                        step = 1;
                        phase++;
                        timer = 5000;
                    }
                    return;
                default:
                    break;
                }
                
                step++;
            }
            else
                timer -= diff;
        }
        
        
    private:
        ScriptedInstance* instance;
    
        uint64 essenceGUID;
        
        uint32 timer;
        uint32 soulCount;
        uint32 soulDeathCount;
        
        uint8 phase;
        uint8 step;
    };
    
    CreatureAINew* getAI(Creature* creature)
    {
        return new Boss_reliquary_of_soulsAI(creature);
    }
};

class Boss_essence_of_suffering : public CreatureScript
{
public:
    Boss_essence_of_suffering() : CreatureScript("boss_essence_of_suffering") {}
    
    class Boss_essence_of_sufferingAI : public CreatureAINew
    {
    public:
        Boss_essence_of_sufferingAI(Creature* creature) : CreatureAINew(creature) {}
        
        enum events {
            EV_FIXATE       = 0,
            EV_ENRAGE       = 1,
            EV_SOUL_DRAIN   = 2
        };
        
        void onReset(bool onSpawn)
        {
            if (onSpawn) {
                addEvent(EV_FIXATE, 8000, 8000, EVENT_FLAG_DELAY_IF_CASTING);
                addEvent(EV_ENRAGE, 30000, 30000, EVENT_FLAG_DELAY_IF_CASTING);
                addEvent(EV_SOUL_DRAIN, 45000, 45000, EVENT_FLAG_DELAY_IF_CASTING);
            }
            else {
                scheduleEvent(EV_FIXATE, 8000);
                scheduleEvent(EV_ENRAGE, 30000);
                scheduleEvent(EV_SOUL_DRAIN, 45000);
            }
            
            me->SetFullTauntImmunity(true);
            me->SetNoCallAssistance(true);
        }
        
        void evade()
        {
            if (Creature* reliquary = me->GetSummoner()->ToCreature())
                reliquary->getAI()->evade();
                
            CreatureAINew::evade();
        }
        
        void onDamageTaken(Unit* /*attacker*/, uint32& damage)
        {
            if (damage >= me->GetHealth()) {
                damage = 0;
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->RemoveAllAuras();
                talk(TALK_SUFF_SAY_RECAP);
            }
        }
        
        void onCombatStart(Unit* victim)
        {
            talk(TALK_SUFF_SAY_FREED);
            setZoneInCombat();
            doCast(me, AURA_OF_SUFFERING, true);
            doCast(me, ESSENCE_OF_SUFFERING_PASSIVE, true);
            doCast(me, ESSENCE_OF_SUFFERING_PASSIVE2, true);
        }
        
        void onKill(Unit* victim)
        {
            talk(TALK_SUFF_SAY_SLAY);
        }
        
        void update(uint32 const diff)
        {
            if (me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE))
                return;

            if (!updateVictim())
                return;
                
            updateEvents(diff);
            
            while (executeEvent(diff, m_currEvent)) {
                switch (m_currEvent) {
                case EV_FIXATE:
                {
                    Unit* target = selectUnit(SELECT_TARGET_NEAREST, 0, 30.0f, true);

                    if (target) {
                        target->CastSpell(me, SPELL_FIXATE_TAUNT, true);
                        doResetThreat();
                        me->AddThreat(target, 1000000.0f);
                        
                        if ((rand() % 16) == 0)
                            talk(TALK_SUFF_SAY_AGGRO);
                    }
                    
                    scheduleEvent(EV_FIXATE, 5000);
                    break;
                }
                case EV_ENRAGE:
                    doCast(me, SPELL_ENRAGE);
                    talk(TALK_SUFF_EMOTE_ENRAGE);
                    scheduleEvent(EV_ENRAGE, 60000);
                    break;
                case EV_SOUL_DRAIN:
                    doCast(selectUnit(SELECT_TARGET_RANDOM, 0), SPELL_SOUL_DRAIN);
                    scheduleEvent(EV_SOUL_DRAIN, 60000);
                    break;
                }
            }
            
            doMeleeAttackIfReady();
        }
    };
    
    CreatureAINew* getAI(Creature* creature)
    {
        return new Boss_essence_of_sufferingAI(creature);
    }
};

class Boss_essence_of_desire : public CreatureScript
{
public:
    Boss_essence_of_desire() : CreatureScript("boss_essence_of_desire") {}
    
    class Boss_essence_of_desireAI : public CreatureAINew
    {
    public:
        Boss_essence_of_desireAI(Creature* creature) : CreatureAINew(creature) {}
        
        enum events {
            EV_RUNE_SHIELD  = 0,
            EV_DEADEN       = 1,
            EV_SOUL_SHOCK   = 2
        };
        
        void onReset(bool onSpawn)
        {
            if (onSpawn) {
                addEvent(EV_RUNE_SHIELD, 60000, 60000, EVENT_FLAG_DELAY_IF_CASTING);
                addEvent(EV_DEADEN, 30000, 30000, EVENT_FLAG_DELAY_IF_CASTING);
                addEvent(EV_SOUL_SHOCK, 5000, 5000, EVENT_FLAG_DELAY_IF_CASTING);
            }
            else {
                scheduleEvent(EV_RUNE_SHIELD, 60000);
                scheduleEvent(EV_DEADEN, 30000);
                scheduleEvent(EV_SOUL_SHOCK, 5000);
            }
            
            me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_CONFUSE, true);
            me->SetNoCallAssistance(true);
        }
        
        void evade()
        {
            if (Creature* reliquary = me->GetSummoner()->ToCreature())
                reliquary->getAI()->evade();
                
            CreatureAINew::evade();
        }
        
        void onDamageTaken(Unit* attacker, uint32& damage)
        {
            if (attacker == me)
                return;
                
            if (damage >= me->GetHealth()) {
                damage = 0;
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->RemoveAllAuras();
                talk(TALK_DESI_SAY_RECAP);
            }
            else {
                int32 bp0 = damage / 2;
                me->CastCustomSpell(attacker, AURA_OF_DESIRE_DAMAGE, &bp0, NULL, NULL, true);
            }
        }
        
        void onHitBySpell(Unit* caster, SpellEntry const* spell)
        {
            if (me->m_currentSpells[CURRENT_GENERIC_SPELL]) {
                for (uint8 i = 0; i < 3; ++i) {
                    if (spell->Effect[i] == SPELL_EFFECT_INTERRUPT_CAST) {
                        if (me->m_currentSpells[CURRENT_GENERIC_SPELL]->m_spellInfo->Id == SPELL_SOUL_SHOCK
                                || me->m_currentSpells[CURRENT_GENERIC_SPELL]->m_spellInfo->Id == SPELL_DEADEN)
                            me->InterruptSpell(CURRENT_GENERIC_SPELL, false);
                    }
                }
            }
        }
        
        void onCombatStart(Unit* victim)
        {
            talk(TALK_DESI_SAY_FREED);
            setZoneInCombat();
            doCast(me, AURA_OF_DESIRE, true);
        }
        
        void onKill(Unit* victim)
        {
            talk(TALK_DESI_SAY_SLAY);
        }
        
        void update(uint32 const diff)
        {
            if (me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE))
                return;

            if (!updateVictim())
                return;
                
            updateEvents(diff);
            
            while (executeEvent(diff, m_currEvent)) {
                switch (m_currEvent) {
                case EV_RUNE_SHIELD:
                    doCast(me, SPELL_RUNE_SHIELD, true);
                    delayEvent(EV_SOUL_SHOCK, 2000);
                    delayEvent(EV_DEADEN, 2000);
                    scheduleEvent(EV_RUNE_SHIELD, 60000);
                    break;
                case EV_SOUL_SHOCK:
                    doCast(me->getVictim(), SPELL_SOUL_SHOCK);
                    scheduleEvent(EV_SOUL_SHOCK, 5000);
                    break;
                case EV_DEADEN:
                    doCast(me->getVictim(), SPELL_DEADEN);
                    scheduleEvent(EV_DEADEN, 25000, 35000);
                    if ((rand() % 2) == 0)
                        talk(TALK_DESI_SAY_SPEC);
                    break;
                }
            }
            
            doMeleeAttackIfReady();
        }
    };
    
    CreatureAINew* getAI(Creature* creature)
    {
        return new Boss_essence_of_desireAI(creature);
    }
};

class Boss_essence_of_anger : public CreatureScript
{
public:
    Boss_essence_of_anger() : CreatureScript("boss_essence_of_anger") {}
    
    class Boss_essence_of_angerAI : public CreatureAINew
    {
    public:
        Boss_essence_of_angerAI(Creature* creature) : CreatureAINew(creature) {}
        
        enum events {
            EV_CHECK_TANK   = 0,
            EV_SOUL_SCREAM  = 1,
            EV_SPITE        = 2
        };
        
        void onReset(bool onSpawn)
        {
            if (onSpawn) {
                addEvent(EV_CHECK_TANK, 3000, 3000);
                addEvent(EV_SOUL_SCREAM, 10000, 10000, EVENT_FLAG_DELAY_IF_CASTING);
                addEvent(EV_SPITE, 30000, 30000, EVENT_FLAG_DELAY_IF_CASTING);
            }
            else {
                scheduleEvent(EV_CHECK_TANK, 3000);
                scheduleEvent(EV_SOUL_SCREAM, 10000);
                scheduleEvent(EV_SPITE, 30000);
            }
            
            tankGUID = 0;
            spiteGUIDs.clear();
            me->SetNoCallAssistance(true);
        }
        
        void evade()
        {
            if (Creature* reliquary = me->GetSummoner()->ToCreature())
                reliquary->getAI()->evade();
                
            CreatureAINew::evade();
        }
        
        void onCombatStart(Unit* victim)
        {
            tankGUID = victim->GetGUID();
            
            talk(TALK_ANGER_SAY_FREED);
            setZoneInCombat();
            doCast(me, AURA_OF_ANGER, true);
        }
        
        void onDeath(Unit* killer)
        {
            talk(TALK_ANGER_SAY_DEATH);
        }
        
        void onKill(Unit* victim)
        {
            talk(TALK_ANGER_SAY_SLAY);
        }
        
        void update(uint32 const diff)
        {
            if (me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE))
                return;

            if (!updateVictim())
                return;
                
            updateEvents(diff);
            
            while (executeEvent(diff, m_currEvent)) {
                switch (m_currEvent) {
                case EV_CHECK_TANK:
                    if (me->getVictim() && me->getVictim()->GetGUID() != tankGUID) {
                        talk(TALK_ANGER_SAY_BEFORE);
                        doCast(me, SPELL_SELF_SEETHE, true);
                        tankGUID = me->getVictim()->GetGUID();
                    }
                    scheduleEvent(EV_CHECK_TANK, 2000);
                    break;
                case EV_SOUL_SCREAM:
                    doCast(me->getVictim(), SPELL_SOUL_SCREAM);
                    if ((rand() % 3) == 0)
                        talk(TALK_ANGER_SAY_SPEC);
                    scheduleEvent(EV_SOUL_SCREAM, 9000, 11000);
                    break;
                case EV_SPITE:
                    doCast(me, SPELL_SPITE_TARGET);
                    talk(TALK_ANGER_SAY_SPEC);
                    scheduleEvent(EV_SPITE, 30000);
                    break;
                }
            }
            
            doMeleeAttackIfReady();
        }
        
    private:
        uint64 tankGUID;
        
        std::list<uint64> spiteGUIDs;
    };
    
    CreatureAINew* getAI(Creature* creature)
    {
        return new Boss_essence_of_angerAI(creature);
    }
};

class Npc_enslaved_soul : public CreatureScript
{
public:
    Npc_enslaved_soul() : CreatureScript("npc_enslaved_soul") {}
    
    class Npc_enslaved_soulAI : public CreatureAINew
    {
    public:
        Npc_enslaved_soulAI(Creature* creature) : CreatureAINew(creature) {}
        
        void onCombatStart(Unit* victim)
        {
            doCast(me, ENSLAVED_SOUL_PASSIVE, true);
            setZoneInCombat();
        }
        
        void onDeath(Unit* killer)
        {
            if (Creature* reliquary = me->GetSummoner()->ToCreature())
                reliquary->getAI()->message(DATA_SOUL_DEATH, 1);
                
            doCast(me, SPELL_SOUL_RELEASE, true);
        }
    };
    
    CreatureAINew* getAI(Creature* creature)
    {
        return new Npc_enslaved_soulAI(creature);
    }
};

void addSC_boss_reliquary_of_souls()
{
    sScriptMgr.addScript(new Boss_reliquary_of_souls());
    sScriptMgr.addScript(new Boss_essence_of_suffering());
    sScriptMgr.addScript(new Boss_essence_of_desire());
    sScriptMgr.addScript(new Boss_essence_of_anger());
    sScriptMgr.addScript(new Npc_enslaved_soul());
}
