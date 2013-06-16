#include "precompiled.h"
#include "def_zulaman.h"
#include "CreatureScript.h"
#include "CreatureAINew.h"

enum {
    YELL_INTRO  = 0,
    YELL_TRANSFORM_TO_LYNX,
    YELL_TRANSFORM_TO_BEAR,
    YELL_TRANSFORM_TO_DRAGONHAWK,
    YELL_TRANSFORM_TO_EAGLE,
    YELL_KILL,
    YELL_FIRE_BREATH,
    YELL_AGGRO,
    YELL_BERSERK,
    YELL_DEATH,
};

enum {
    // Troll Form
    SPELL_WHIRLWIND                 = 17207,
    SPELL_GRIEVOUS_THROW            = 43093,   // remove debuff after full healed
    
    // Bear Form
    SPELL_CREEPING_PARALYSIS        = 43095,   // should cast on the whole raid
    SPELL_OVERPOWER                 = 43456,   // use after melee attack dodged
    
    // Eagle Form
    SPELL_ENERGY_STORM              = 43983,   // enemy area aura, trigger 42577
    SPELL_ZAP_INFORM                = 42577,
    SPELL_ZAP_DAMAGE                = 43137,   // 1250 damage
    SPELL_SUMMON_CYCLONE            = 43112,   // summon four feather vortex
    CREATURE_FEATHER_VORTEX         = 24136,
    SPELL_CYCLONE_VISUAL            = 43119,   // trigger 43147 visual
    SPELL_CYCLONE_PASSIVE           = 43120,   // trigger 43121 (4y aoe) every second
    
    // Lynx Form
    SPELL_CLAW_RAGE_CHARGE          = 42583,
    SPELL_CLAW_RAGE_TRIGGER         = 43149,
    SPELL_CLAW_RAGE_DAMAGE          = 43150,
    SPELL_LYNX_RUSH_HASTE           = 43152,
    SPELL_LYNX_RUSH_DAMAGE          = 43153,
    
    // Dragonhawk Form
    SPELL_FLAME_WHIRL               = 43213,   // trigger two spells
    SPELL_FLAME_BREATH              = 43215,
    SPELL_SUMMON_PILLAR             = 43216,   // summon 24187
    CREATURE_COLUMN_OF_FIRE         = 24187,
    SPELL_PILLAR_TRIGGER            = 43218,   // trigger 43217

    // Cosmetic
    SPELL_SPIRIT_AURA               = 42466,
    SPELL_SIPHON_SOUL               = 43501,

    // Transforms
    SPELL_SHAPE_OF_THE_BEAR         = 42594,   // 15% dmg
    SPELL_SHAPE_OF_THE_EAGLE        = 42606,
    SPELL_SHAPE_OF_THE_LYNX         = 42607,   // haste melee 30%
    SPELL_SHAPE_OF_THE_DRAGONHAWK   = 42608,

    SPELL_BERSERK                   = 45078
};

enum {
    PHASE_TROLL = 1,
    PHASE_BEAR,
    PHASE_EAGLE,
    PHASE_LYNX,
    PHASE_DRAGONHAWK,
};


#define CENTER_X 120.148811
#define CENTER_Y 703.713684
#define CENTER_Z 45.111477

struct SpiritInfoStruct
{
    uint32 entry;
    float x, y, z, orient;
};

static SpiritInfoStruct SpiritInfo[] =
{
    {23878, 147.87, 706.51, 45.11, 3.04},
    {23880, 88.95, 705.49, 45.11, 6.11},
    {23877, 137.23, 725.98, 45.11, 3.71},
    {23879, 104.29, 726.43, 45.11, 5.43}
};

struct TransformStruct
{
    uint32 textid;
    uint32 spell;
    uint32 unaura;
};
    
static TransformStruct Transform[] =
{
    {YELL_TRANSFORM_TO_BEAR, SPELL_SHAPE_OF_THE_BEAR, SPELL_WHIRLWIND},
    {YELL_TRANSFORM_TO_EAGLE, SPELL_SHAPE_OF_THE_EAGLE, SPELL_SHAPE_OF_THE_BEAR},
    {YELL_TRANSFORM_TO_LYNX, SPELL_SHAPE_OF_THE_LYNX, SPELL_SHAPE_OF_THE_EAGLE},
    {YELL_TRANSFORM_TO_DRAGONHAWK, SPELL_SHAPE_OF_THE_DRAGONHAWK, SPELL_SHAPE_OF_THE_LYNX}
};

class Boss_zuljin : public CreatureScript
{
public:
    Boss_zuljin() : CreatureScript("boss_zuljin_new") {}
    
    class Boss_zuljin_newAI : public CreatureAINew
    {
    public:
        enum events {
            EV_OVERPOWER_READY  = 0,    // PHASE_BEAR
            EV_WHIRLWIND        = 1,    // PHASE_TROLL
            EV_GRIEVOUS_THROW   = 2,    // PHASE TROLL
            EV_CREEPING_PARA    = 3,    // PHASE BEAR
            EV_CLAW_RAGE        = 4,    // PHASE_LYNX
            EV_CLAW_RAGE_RESET  = 5,    // PHASE_LYNX - Resets threat on target after claw rage
            EV_LYNX_RUSH        = 6,    // PHASE_LYNX
            EV_FLAME_WHIRL      = 7,    // PHASE_DRAGONHAWK
            EV_PILLAR_OF_FIRE   = 8,    // PHASE_DRAGONHAWK
            EV_FLAME_BREATH     = 9,    // PHASE_DRAGONHAWK
            EV_REINIT_SPEED     = 10    // PHASE_LYNX
        };

        Boss_zuljin_newAI(Creature* creature) : CreatureAINew(creature), summons(me)
        {
            instance = ((ScriptedInstance*)creature->GetInstanceData());
        }
        
        ScriptedInstance* instance;
        
        SummonList summons;
        
        uint32 phase;
        
        void onReset(bool onSpawn)
        {
            if (instance && instance->GetData(DATA_ZULJINEVENT) != DONE)
                instance->SetData(DATA_ZULJINEVENT, NOT_STARTED);
            
            me->SetFullTauntImmunity(true);
            me->SetByteValue(UNIT_FIELD_BYTES_2, 0, SHEATH_STATE_MELEE);
            
            setPhase(PHASE_TROLL);
            
            summons.DespawnAll();
            
            // Setup events
            if (onSpawn) {
                addEvent(EV_WHIRLWIND, 7000, 7000, EVENT_FLAG_DELAY_IF_CASTING, true, phaseMaskForPhase(PHASE_TROLL));
                addEvent(EV_GRIEVOUS_THROW, 8000, 8000, EVENT_FLAG_DELAY_IF_CASTING, true, phaseMaskForPhase(PHASE_TROLL));
                addEvent(EV_OVERPOWER_READY, 5000, 5000, 0, true, phaseMaskForPhase(PHASE_BEAR));
                addEvent(EV_CREEPING_PARA, 7000, 7000, EVENT_FLAG_DELAY_IF_CASTING, true, phaseMaskForPhase(PHASE_BEAR));
                addEvent(EV_CLAW_RAGE, 5000, 5000, EVENT_FLAG_DELAY_IF_CASTING, true, phaseMaskForPhase(PHASE_LYNX));
                addEvent(EV_CLAW_RAGE_RESET, 5500, 5500, 0, false, phaseMaskForPhase(PHASE_LYNX));
                addEvent(EV_LYNX_RUSH, 14000, 14000, 0, true, phaseMaskForPhase(PHASE_LYNX));
                addEvent(EV_FLAME_WHIRL, 5000, 5000, EVENT_FLAG_DELAY_IF_CASTING, true, phaseMaskForPhase(PHASE_DRAGONHAWK));
                addEvent(EV_PILLAR_OF_FIRE, 6000, 6000, EVENT_FLAG_DELAY_IF_CASTING, true, phaseMaskForPhase(PHASE_DRAGONHAWK));
                addEvent(EV_FLAME_BREATH, 7000, 7000, EVENT_FLAG_DELAY_IF_CASTING, true, phaseMaskForPhase(PHASE_DRAGONHAWK));
                addEvent(EV_REINIT_SPEED, 1000, 1000, 0, false, phaseMaskForPhase(PHASE_LYNX));
            }
            else {
                scheduleEvent(EV_WHIRLWIND, 7000);
                scheduleEvent(EV_GRIEVOUS_THROW, 8000);
                scheduleEvent(EV_OVERPOWER_READY, 5000);
                scheduleEvent(EV_CREEPING_PARA, 7000);
                scheduleEvent(EV_CLAW_RAGE, 5000);
                disableEvent(EV_CLAW_RAGE_RESET);
                scheduleEvent(EV_LYNX_RUSH, 14000);
                scheduleEvent(EV_FLAME_WHIRL, 5000);
                scheduleEvent(EV_PILLAR_OF_FIRE, 6000);
                scheduleEvent(EV_FLAME_BREATH, 7000);
                disableEvent(EV_REINIT_SPEED);
            }
            
            overpowerReady = false;
            clawRageTargetGUID = 0;
        }
        
        void onCombatStart(Unit* victim)
        {
            if (instance)
                instance->SetData(DATA_ZULJINEVENT, IN_PROGRESS);
                
            setZoneInCombat();
            spawnSpirits();
            
            talk(YELL_INTRO);
        }
        
        void onKill(Unit* killed)
        {
            talk(YELL_KILL);
        }
        
        void onDeath(Unit* killer)
        {
            summons.DespawnEntry(CREATURE_COLUMN_OF_FIRE);
            
            talk(YELL_DEATH);
            
            if (instance)
                instance->SetData(DATA_ZULJINEVENT, DONE);
                
            if (Unit* spirit = Unit::GetUnit(*me, spiritGUIDs[4]))
                spirit->SetUInt32Value(UNIT_FIELD_BYTES_1, PLAYER_STATE_DEAD);
        }
        
        void attackStart(Unit* victim)
        {
            CreatureAINew::attackStart(victim);
            if (getPhase() == PHASE_EAGLE)
                me->GetMotionMaster()->MoveIdle();
        }
        
        void onSummon(Creature* summoned)
        {
            summons.Summon(summoned);
        }
        
        void onSummonDespawn(Creature* summoned)
        {
            summons.Despawn(summoned);
        }
        
        void onEnterPhase(uint32 newPhase)
        {
            switch (newPhase) {
            case PHASE_TROLL:
                break;
            case PHASE_BEAR:
            case PHASE_EAGLE:
            case PHASE_LYNX:
            case PHASE_DRAGONHAWK:
                doTeleportTo(CENTER_X, CENTER_Y, CENTER_Z, 100);
                doResetThreat();

                me->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_DISPLAY, 0); // TODO: Implement a wrapper for this
                me->RemoveAurasDueToSpell(Transform[newPhase - 2].unaura);
                doCast(me, Transform[newPhase - 2].spell);

                talk(Transform[newPhase - 2].textid);

                if (Unit* spirit = Unit::GetUnit(*me, spiritGUIDs[newPhase - 2]))
                    spirit->SetUInt32Value(UNIT_FIELD_BYTES_1, PLAYER_STATE_DEAD);

                if (Unit* spirit = Unit::GetUnit(*me, spiritGUIDs[newPhase - 1]))
                    spirit->CastSpell(me, SPELL_SIPHON_SOUL, false);
                    
                if (newPhase == PHASE_EAGLE) {
                    me->GetMotionMaster()->Clear();
                    me->CastSpell(me, SPELL_ENERGY_STORM, true);
                    for (uint8 i = 0; i < 4; i++) {
                        Creature* vortex = me->SummonCreature(CREATURE_FEATHER_VORTEX, 0, 0, 0, 0, TEMPSUMMON_CORPSE_DESPAWN, 0);
                        if (vortex) {
                            vortex->CastSpell(vortex, SPELL_CYCLONE_PASSIVE, true);
                            vortex->CastSpell(vortex, SPELL_CYCLONE_VISUAL, true);
                            vortex->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                            vortex->SetSpeed(MOVE_RUN, 1.0f);
                            vortex->AI()->AttackStart(selectUnit(SELECT_TARGET_RANDOM, 0)); // FIXME: when converting vortex AI, change this
                            setZoneInCombat();
                        }
                    }
                }
                else
                    me->AI()->AttackStart(me->getVictim());

                if (newPhase == PHASE_LYNX) {
                    me->RemoveAurasDueToSpell(SPELL_ENERGY_STORM);
                    summons.DespawnEntry(CREATURE_FEATHER_VORTEX);
                    me->GetMotionMaster()->MoveChase(me->getVictim());
                }
                break;
            default:
                break;
            }
        }
        
        void spawnSpirits()
        {
            Creature* spirit = NULL;
            for (uint8 i = 1; i < 5; i++) {
                if (spirit = me->SummonCreature(SpiritInfo[i - 1].entry, SpiritInfo[i - 1].x, SpiritInfo[i - 1].y, SpiritInfo[i - 1].z, SpiritInfo[i - 1].orient, TEMPSUMMON_DEAD_DESPAWN, 0)) {
                    spirit->CastSpell(spirit, SPELL_SPIRIT_AURA, true);
                    spirit->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    spirit->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    spiritGUIDs[i] = spirit->GetGUID();
                }
            }
            spiritGUIDs[0] = 0;
        }
        
        void doMeleeAttackIfReady()
        {
            if (!me->IsNonMeleeSpellCasted(false)) {
                if (me->isAttackReady() && me->IsWithinMeleeRange(me->getVictim())) { // It seems that there is no triggering aura we could use with the proper procflags :(
                    if (getPhase() == PHASE_BEAR && overpowerReady) {
                        uint32 health = me->getVictim()->GetHealth();
                        me->AttackerStateUpdate(me->getVictim());
                        if (me->getVictim() && health == me->getVictim()->GetHealth()) { // Dodged
                            me->CastSpell(me->getVictim(), SPELL_OVERPOWER, false);
                            overpowerReady = false;
                            scheduleEvent(EV_OVERPOWER_READY, 5000);
                            enableEvent(EV_OVERPOWER_READY);
                        }
                    }
                    else
                        me->AttackerStateUpdate(me->getVictim());

                    me->resetAttackTimer();
                }
            }
        }
        
        void update(uint32 const diff)
        {
            if (!updateVictim())
                return;

            if (me->GetHealth() < ((me->GetMaxHealth()/5.0f) * (5 - getPhase())))
                incrPhase();
            
            updateEvents(diff);
            
            while (executeEvent(diff, m_currEvent))
            {
                switch (m_currEvent) {
                case EV_OVERPOWER_READY:
                    overpowerReady = true;
                    disableEvent(EV_OVERPOWER_READY);
                    break;
                case EV_WHIRLWIND:
                    doCast(me, SPELL_WHIRLWIND);
                    scheduleEvent(EV_WHIRLWIND, 15000, 20000);
                    break;
                case EV_GRIEVOUS_THROW:
                    doCast(selectUnit(SELECT_TARGET_RANDOM, 0, 80.0f, true), SPELL_GRIEVOUS_THROW);
                    scheduleEvent(EV_GRIEVOUS_THROW, 10000);
                    break;
                case EV_CREEPING_PARA:
                    doCast(me, SPELL_CREEPING_PARALYSIS);
                    scheduleEvent(EV_CREEPING_PARA, 20000);
                    break;
                case EV_CLAW_RAGE:
                {
                    Unit* clawRageTarget = selectUnit(SELECT_TARGET_RANDOM, 1, 80.0f, true);
                    if (clawRageTarget) {
                        clawRageTargetGUID = clawRageTarget->GetGUID();
                        doModifyThreat(clawRageTarget, 1000000); // 1.000.000 threat should be enough
                        me->SetSpeed(MOVE_RUN, 1.2f);
                        scheduleEvent(EV_REINIT_SPEED, 2000);
                        enableEvent(EV_REINIT_SPEED);
                        doCast(clawRageTarget, SPELL_CLAW_RAGE_CHARGE);
                        doCast(me, SPELL_CLAW_RAGE_TRIGGER, true); // Triggers SPELL_CLAW_RAGE_DAMAGE every 500 ms
                    }

                    scheduleEvent(EV_CLAW_RAGE_RESET, 5500);
                    scheduleEvent(EV_CLAW_RAGE, 15000, 20000);
                    enableEvent(EV_CLAW_RAGE_RESET);
                    delayEvent(EV_LYNX_RUSH, 5500);
                    break;
                }
                case EV_CLAW_RAGE_RESET: // reset clawRageTargetGUID, remove threat, disable EV_CLAW_RAGE_RESET event
                    if (Unit* clawRageTarget = Unit::GetUnit(*me, clawRageTargetGUID))
                        doModifyThreat(clawRageTarget, -1000000);

                    clawRageTargetGUID = 0;
                    disableEvent(EV_CLAW_RAGE_RESET);
                    break;
                case EV_LYNX_RUSH:
                    me->SetSpeed(MOVE_RUN, 1.2f);
                    doCast(selectUnit(SELECT_TARGET_RANDOM, 1, 80.0f, true), SPELL_LYNX_RUSH_DAMAGE);
                    delayEvent(EV_CLAW_RAGE, 2000);
                    scheduleEvent(EV_REINIT_SPEED, 2000);
                    enableEvent(EV_REINIT_SPEED);
                    scheduleEvent(EV_LYNX_RUSH, 25000, 30000);
                    break;
                case EV_FLAME_WHIRL:
                    doCast(me, SPELL_FLAME_WHIRL);
                    scheduleEvent(EV_FLAME_WHIRL, 12000);
                    break;
                case EV_PILLAR_OF_FIRE:
                    doCast(selectUnit(SELECT_TARGET_RANDOM, 0, 80.0f, true), SPELL_SUMMON_PILLAR);
                    scheduleEvent(EV_PILLAR_OF_FIRE, 10000);
                    break;
                case EV_FLAME_BREATH:
                    doCast(selectUnit(SELECT_TARGET_RANDOM, 0, 80.0f, true), SPELL_FLAME_BREATH);
                    scheduleEvent(EV_FLAME_BREATH, 10000);
                    break;
                case EV_REINIT_SPEED:
                    me->SetSpeed(MOVE_RUN, 1.0f);
                    disableEvent(EV_REINIT_SPEED);
                    break;
                }
            }
            
            doMeleeAttackIfReady();
        }
        
    private:
        uint64 spiritGUIDs[5];
        uint64 clawRageTargetGUID;
        
        bool overpowerReady;
    };
    
    CreatureAINew* getAI(Creature* creature)
    {
        return new Boss_zuljin_newAI(creature);
    }
};

void AddSC_boss_zuljin()
{
    sScriptMgr.addScript(new Boss_zuljin());
}
