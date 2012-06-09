#include "precompiled.h"
#include "def_black_temple.h"
#include "CreatureScript.h"
#include "CreatureAINew.h"

enum ReliquaryOfSoulsData {
    // Essence of Suffering
    TALK_SUFF_SAY_FREED             = 0,
    TALK_SUFF_SAY_AGGRO             = 1,
    TALK_SUFF_SAY_SLAY1             = 2,
    TALK_SUFF_SAY_SLAY2             = 3,
    TALK_SUFF_SAY_SLAY3             = 4,
    TALK_SUFF_SAY_RECAP             = 5,
    TALK_SUFF_SAY_AFTER             = 6,
    TALK_SUFF_EMOTE_ENRAGE          = 7,
    // Essence of Desire
    TALK_DESI_SAY_FREED             = 0,
    TALK_DESI_SAY_SLAY1             = 1,
    TALK_DESI_SAY_SLAY2             = 2,
    TALK_DESI_SAY_SLAY3             = 3,
    TALK_DESI_SAY_SPEC              = 4,
    TALK_DESI_SAY_RECAP             = 5,
    TALK_DESI_SAY_AFTER             = 6,
    // Essence of Anger
    TALK_ANGER_SAY_FREED            = 0,
    TALK_ANGER_SAY_FREED2           = 1,
    TALK_ANGER_SAY_SLAY1            = 2,
    TALK_ANGER_SAY_SLAY2            = 3,
    TALK_ANGER_SAY_SPEC             = 4,
    TALK_ANGER_SAY_BEFORE           = 5,
    TALK_ANGER_SAY_DEATH            = 6,
    
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
    Boss_reliquary_of_souls() : CreatureScript("boss_reliquary_of_souls_new") {}
    
    class Boss_reliquary_of_soulsAI : public CreatureAINew
    {
    public:
        Boss_reliquary_of_soulsAI(Creature* creature) : CreatureAINew(creature)
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
                
            if (Unit* target = selectUnit(TARGET_RANDOM, 0)) {
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
                            summon->getAI()->attackStart(selectUnit(TARGET_TOPAGGRO, 0));
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
                            mergeThreatList(essence);
                            essence->RemoveAllAuras();
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
                    timer = 1500;
                    if (essence->IsWithinDistInMap(me, 10.0f)) {
                        timer = 2000;
                        me->RemoveAurasDueToSpell(SPELL_SUBMERGE);
                        essence->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_SUBMERGE); // Rotate and disappear
                    }
                    else { // Really necessary?
                        mergeThreatList(essence);
                        essence->RemoveAllAuras();
                        essence->DeleteThreatList();
                        essence->SetReactState(REACT_PASSIVE);
                        essence->GetMotionMaster()->MoveFollow(me, 0, 0);
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
