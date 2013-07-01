/* Copyright (C) 2006 - 2008 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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

/* ScriptData
SDName: Boss_Brutallus
SD%Complete: 80
SDComment: Find a way to start the intro, best code for the intro
EndScriptData */

#include "precompiled.h"
#include "def_sunwell_plateau.h"

enum Quotes
{
    YELL_INTRO                 =   -1580017,
    YELL_INTRO_BREAK_ICE       =   -1580018,
    YELL_INTRO_CHARGE          =   -1580019,
    YELL_INTRO_KILL_MADRIGOSA  =   -1580020,
    YELL_INTRO_TAUNT           =   -1580021,

    YELL_MADR_ICE_BARRIER      =   -1580031,
    YELL_MADR_INTRO            =   -1580032,
    YELL_MADR_ICE_BLOCK        =   -1580033,
    YELL_MADR_TRAP             =   -1580034,
    YELL_MADR_DEATH            =   -1580035,

    YELL_AGGRO                 =   -1580022,
    YELL_KILL1                 =   -1580023,
    YELL_KILL2                 =   -1580024,
    YELL_KILL3                 =   -1580025,
    YELL_LOVE1                 =   -1580026,
    YELL_LOVE2                 =   -1580027,
    YELL_LOVE3                 =   -1580028,
    YELL_BERSERK               =   -1580029,
    YELL_DEATH                 =   -1580030
};

enum Spells
{
    SPELL_METEOR_SLASH                 =   45150,
    SPELL_BURN                         =   46394,
    SPELL_STOMP                        =   45185,
    SPELL_BERSERK                      =   26662,
    SPELL_DUAL_WIELD                   =   42459,

    SPELL_INTRO_FROST_BLAST            =   45203,
    SPELL_INTRO_FROSTBOLT              =   44843,
    SPELL_INTRO_ENCAPSULATE            =   45665,
    SPELL_INTRO_ENCAPSULATE_CHANELLING =   45661
};

#define FELMYST 25038

struct boss_brutallusAI : public ScriptedAI
{
    boss_brutallusAI(Creature *c) : ScriptedAI(c){
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
        Intro = true;
    }

    ScriptedInstance* pInstance;

    uint32 SlashTimer;
    uint32 BurnTimer;
    uint32 StompTimer;
    uint32 BerserkTimer;

    uint32 IntroPhase;
    uint32 IntroPhaseTimer;
    uint32 IntroFrostBoltTimer;

    bool Intro;
    bool IsIntro;
    bool Enraged;

    void Reset()
    {
        SlashTimer = 11000;
        StompTimer = 30000;
        BurnTimer = 20000;
        BerserkTimer = 360000;

        IntroPhase = 0;
        IntroPhaseTimer = 0;
        IntroFrostBoltTimer = 0;

        IsIntro = false;
        Enraged = false;
        //Intro = true; // TODO: RESTORE INTRO IN CONSTRUCTOR AFTER TESTS

        m_creature->CastSpell(m_creature, SPELL_DUAL_WIELD, true);

        if (pInstance && pInstance->GetData(DATA_BRUTALLUS_EVENT) != DONE)
            pInstance->SetData(DATA_BRUTALLUS_EVENT, NOT_STARTED);

        if (pInstance && pInstance->GetData(DATA_BRUTALLUS_EVENT) == DONE && pInstance->GetData(DATA_FELMYST_EVENT) != DONE) {
            if (Player *plr = pInstance->GetPlayerInMap()) {
                float x,y,z;
                m_creature->GetPosition(x,y,z);
                Creature *felmyst = plr->SummonCreature(FELMYST, x,y, z+30, m_creature->GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN, 0);
                if (Creature *Madrigosa = Unit::GetCreature(*m_creature, pInstance->GetData64(DATA_MADRIGOSA)))
                    Madrigosa->SetVisibility(VISIBILITY_OFF);
            }
        }
        
        m_creature->SetFullTauntImmunity(false);
        me->RemoveAurasDueToSpell(45769);
        me->CastSpell(me, 45769, true);
    }

    void Aggro(Unit *who)
    {
        if (Intro || IsIntro)
            return;

        DoScriptText(YELL_AGGRO, m_creature);

        if (pInstance)
            pInstance->SetData(DATA_BRUTALLUS_EVENT, IN_PROGRESS);
            
        if (who->ToPlayer() && !Intro && !IsIntro) {
            if (Creature *Madrigosa = Unit::GetCreature(*me, pInstance ? pInstance->GetData64(DATA_MADRIGOSA) : 0))
                Madrigosa->SetVisibility(VISIBILITY_OFF);
        }
    }

    void KilledUnit(Unit* victim)
    {
        DoScriptText(RAND(YELL_KILL1,YELL_KILL2,YELL_KILL3), me);
    }
    
    void EnterEvadeMode()
    {
        if (!Intro)
            ScriptedAI::EnterEvadeMode();
    }

    void JustDied(Unit* Killer)
    {
        if (Creature *Madrigosa = Unit::GetCreature(*me, pInstance ? pInstance->GetData64(DATA_MADRIGOSA) : 0)) {
            Madrigosa->SetVisibility(VISIBILITY_OFF);
            Madrigosa->Respawn();
            Madrigosa->Kill(Madrigosa);
            Madrigosa->RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
            Madrigosa->SetVisibility(VISIBILITY_ON);
            Madrigosa->SummonCreature(25703, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN, 0);
        }

        DoScriptText(YELL_DEATH, m_creature);

        if (pInstance){
            pInstance->SetData(DATA_BRUTALLUS_EVENT, DONE);
            
            // Force removal of Burn aura on all players in map
            Map::PlayerList const& players = pInstance->instance->GetPlayers();
            if (!players.isEmpty()) {
                for(Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr) {
                    if (Player* plr = itr->getSource())
                        plr->RemoveAurasDueToSpell(SPELL_BURN);
                }
            }
        }
    }

    void StartIntro()
    {
        if (!Intro || IsIntro)
            return;
            
        if (me->isDead())
            return;
            
        m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_OOC_NOT_ATTACKABLE);

        Creature *Madrigosa = Unit::GetCreature(*m_creature, pInstance->GetData64(DATA_MADRIGOSA));
        if (Madrigosa) {
            (Madrigosa->ToCreature())->Respawn();
            Madrigosa->setActive(true);
            IsIntro = true;
            Madrigosa->SetMaxHealth(10000000);
            Madrigosa->SetHealth(m_creature->GetMaxHealth());
            IntroPhaseTimer = 1000;
        }
        else {
            //Madrigosa not found, end intro
            error_log("Madrigosa was not found");
            EndIntro();
        }
    }

    void EndIntro()
    {
        //error_log("Brutallus: Ending intro");
        m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_OOC_NOT_ATTACKABLE);
        me->SetReactState(REACT_AGGRESSIVE);
        Intro = false;
        IsIntro = false;
    }

    void DoIntro()
    {
        Creature *Madrigosa = Unit::GetCreature(*me, pInstance ? pInstance->GetData64(DATA_MADRIGOSA) : 0);
        if (!Madrigosa)
            return;
            
        //sLog.outString("IntroPhase %u", IntroPhase);

        switch (IntroPhase) {
        case 0:
            DoScriptText(YELL_MADR_ICE_BARRIER, Madrigosa);
            if (GameObject *IceBarrier = GameObject::GetGameObject(*me, pInstance ? pInstance->GetData64(DATA_GO_ICE_BARRIER) : 0)) {
                IceBarrier->SetLootState(GO_READY);
                IceBarrier->UseDoorOrButton();
            }
            else
                sLog.outError("Sunwell/Brutallus Intro: Ice Barrier not found!");
            me->AttackStop();
            me->SetReactState(REACT_DEFENSIVE);
            Madrigosa->SetReactState(REACT_DEFENSIVE);
            IntroPhaseTimer = 7000;
            ++IntroPhase;
            break;
        case 1:
            me->SetInFront(Madrigosa);
            Madrigosa->SetInFront(me);
            DoScriptText(YELL_MADR_INTRO, Madrigosa, me);
            IntroPhaseTimer = 9000;
            ++IntroPhase;
            break;
        case 2:
            DoScriptText(YELL_INTRO, me, Madrigosa);
            AttackStart(Madrigosa);
            Madrigosa->AI()->AttackStart(me);
            IntroPhaseTimer = 13000;
            ++IntroPhase;
            break;
        case 3:
        {
            Madrigosa->GetMotionMaster()->Clear(false);
            Madrigosa->HandleEmoteCommand(EMOTE_ONESHOT_LIFTOFF);
            m_creature->SetCanFly(true);
            IntroPhaseTimer = 500;
            ++IntroPhase;
            break;
        }
        case 4:
        {
            Madrigosa->GetMotionMaster()->MovePoint(0, Madrigosa->GetPositionX(), Madrigosa->GetPositionY()+2, Madrigosa->GetPositionZ()+8);
            me->AttackStop();
            me->SetReactState(REACT_PASSIVE);
            Madrigosa->AttackStop();
            Madrigosa->SetReactState(REACT_PASSIVE);
            IntroPhaseTimer = 3000;
            ++IntroPhase;
            break;
        }
        case 5:
        {
            //((ScriptedAI*)Madrigosa->AI())->AttackStart(me, false);
            Madrigosa->CastSpell(me, 45203, true);
            IntroPhaseTimer = 800;
            ++IntroPhase;
            break;
        }
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        {
            Madrigosa->CastSpell(me, 44843, true);
            IntroPhaseTimer = 1100;
            ++IntroPhase;
            break;
        }
        case 12:
        {
            float x, y, z, ori;
            Madrigosa->GetHomePosition(x, y, z, ori);
            Madrigosa->GetMotionMaster()->MovePoint(1, x, y, z);
            IntroPhaseTimer = 1000;
            ++IntroPhase;
            break;
        }
        case 13:
        {
        	m_creature->SetCanFly(false);
            Madrigosa->StopMoving();
            Madrigosa->HandleEmoteCommand(EMOTE_ONESHOT_LAND);
            IntroPhaseTimer = 1000;
            ++IntroPhase;
            break;
        }
        case 14:
        {
            //Madrigosa->AI()->AttackStart(me);
            AttackStart(Madrigosa);
            IntroPhaseTimer = 6000;
            ++IntroPhase;
            break;
        }
        case 15:
            Madrigosa->CastSpell(me, SPELL_INTRO_ENCAPSULATE_CHANELLING, true);
            DoScriptText(YELL_MADR_TRAP, Madrigosa);
            DoCast(me, SPELL_INTRO_ENCAPSULATE);
            IntroPhaseTimer = 11000;
            ++IntroPhase;
            break;
        case 16:
            DoScriptText(YELL_INTRO_CHARGE, me);
            IntroPhaseTimer = 5000;
            ++IntroPhase;
            break;
        case 17:
            DoCast(Madrigosa, SPELL_STOMP);
            me->Kill(Madrigosa);
            DoScriptText(YELL_MADR_DEATH, Madrigosa);
            me->SetHealth(me->GetMaxHealth());
            me->AttackStop();
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_OOC_NOT_ATTACKABLE);
            IntroPhaseTimer = 4000;
            ++IntroPhase;
            break;
        case 18:
            DoScriptText(YELL_INTRO_KILL_MADRIGOSA, me);
            me->SetOrientation(0.14f);
            me->StopMoving();
            Madrigosa->setDeathState(CORPSE);
            Madrigosa->RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
            IntroPhaseTimer = 8000;
            ++IntroPhase;
            break;
        case 19:
            DoScriptText(YELL_INTRO_TAUNT, me);
            IntroPhaseTimer = 5000;
            ++IntroPhase;
            break;
        case 20:
            DoCast(me, 46637);
            IntroPhaseTimer = 2000;
            ++IntroPhase;
            break;
        case 21:
        {
            if (GameObject *IceBarrier = GameObject::GetGameObject(*me, pInstance ? pInstance->GetData64(DATA_GO_ICE_BARRIER) : 0)) {
                IceBarrier->SetLootState(GO_READY);
                IceBarrier->UseDoorOrButton();
            }
            else
                sLog.outError("Sunwell/Brutallus Intro: Ice Barrier not found!");
            
            IntroPhaseTimer = 200;
            ++IntroPhase;
            break;
        }
        case 22:
        {
            if (pInstance)
                pInstance->SetData(DATA_ICEBARRIER_EVENT, DONE);
            
            IntroPhaseTimer = 500;
            ++IntroPhase;
            break;
        }
        case 23:
            EndIntro();
            break;
        }
    }
    
    void DamageTaken(Unit* doneBy, uint32& damage)
    {
        if ((Intro || IsIntro) && doneBy->ToPlayer())
            damage = 0;
            
        if ((Intro || IsIntro) && damage >= me->GetHealth())
            damage = 0;
    }

    void MoveInLineOfSight(Unit *who) {
        if (!who->isTargetableForAttack() || !m_creature->IsHostileTo(who))
            return;

        if (pInstance && Intro)
            pInstance->SetData(DATA_BRUTALLUS_EVENT, SPECIAL);
        
        if (!Intro && !IsIntro)
            ScriptedAI::MoveInLineOfSight(who);
    }

    void UpdateAI(const uint32 diff)
    {
        if (IsIntro)
        {
            if (IntroPhaseTimer <= diff)
                DoIntro();
            else IntroPhaseTimer -= diff;

            if (!UpdateVictim())
                return;
            DoMeleeAttackIfReady();
        }

        if (!UpdateVictim() || IsIntro)
            return;

        me->addUnitState(UNIT_STAT_IGNORE_PATHFINDING);

        if (SlashTimer <= diff)
        {
            DoCast(me->getVictim(), SPELL_METEOR_SLASH);
            SlashTimer = 11000;
        } else SlashTimer -= diff;

        if (StompTimer <= diff)
        {
            DoScriptText(RAND(YELL_LOVE1,YELL_LOVE2,YELL_LOVE3), me);
            DoCast(me->getVictim(), SPELL_STOMP);
            StompTimer = 30000;
        } else StompTimer -= diff;

        if (BurnTimer <= diff)
        {
            if (Unit *target = SelectUnit(SELECT_TARGET_RANDOM, 0, 100.0f, true)) {
                if(!target->HasAura(SPELL_BURN, 0)) {
                    target->CastSpell(target, SPELL_BURN, true);
                    //BurnTimer = urand(60000,180000);
                    BurnTimer = 20000;
                }
                else
                    BurnTimer = 1000 + diff; // if target has SPELL_BURN, wait a bit.
            }
            else
                BurnTimer = urand(60000,180000); // no targets!?
        } else BurnTimer -= diff;

        if (BerserkTimer < diff && !Enraged)
        {
            DoScriptText(YELL_BERSERK, me);
            DoCast(me, SPELL_BERSERK, true);
            Enraged = true;
            m_creature->SetFullTauntImmunity(true);
        } else BerserkTimer -= diff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_brutallus(Creature *_Creature)
{
    return new boss_brutallusAI (_Creature);
}

bool AreaTrigger_at_brutallus_intro(Player* pPlayer, AreaTriggerEntry const *pAt)
{
    if (pPlayer->isGameMaster())
        return false;

    if (ScriptedInstance* pInstance = ((ScriptedInstance*)pPlayer->GetInstanceData())) {
        if (pInstance->GetData(DATA_BRUTALLUS_EVENT) == DONE)
            return true;
        if (Creature *Brutallus = Unit::GetCreature(*pPlayer, pInstance ? pInstance->GetData64(DATA_BRUTALLUS) : 0))
            ((boss_brutallusAI*)Brutallus->AI())->StartIntro();
    }

    return true;
}

struct trigger_death_cloudAI : public ScriptedAI
{
    trigger_death_cloudAI(Creature* c) : ScriptedAI(c)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
    }
    
    ScriptedInstance* pInstance;
    
    uint32 bornTimer;
    
    void Reset()
    {
        DoCast(me, 45212, true);
        
        me->SetSpeed(MOVE_WALK, 2.0f);
        me->SetSpeed(MOVE_RUN, 2.0f);

        Creature *Madrigosa = Unit::GetCreature(*me, pInstance ? pInstance->GetData64(DATA_MADRIGOSA) : 0);
        if (!Madrigosa)
            return;

        float x, y, z;
        Madrigosa->GetPosition(x, y, z);
        me->GetMotionMaster()->MovePoint(0, x, y, z);
        
        bornTimer = 0;
    }
    
    void Aggro(Unit* who) {}
    
    void MovementInform(uint32 type, uint32 id)
    {
        if (Creature *Madrigosa = Unit::GetCreature(*me, pInstance ? pInstance->GetData64(DATA_MADRIGOSA) : 0)) {
            me->AddAura(44885, Madrigosa);
            bornTimer = 10000;
        }
    }
    
    void UpdateAI(uint32 const diff)
    {
        if (!bornTimer)
            return;
            
        if (bornTimer <= diff) {
            if (Creature *Madrigosa = Unit::GetCreature(*me, pInstance ? pInstance->GetData64(DATA_MADRIGOSA) : 0)) {
                Madrigosa->RemoveAurasDueToSpell(44885);
                Madrigosa->SetVisibility(VISIBILITY_OFF);
                float x, y, z;
                Madrigosa->GetPosition(x, y, z);
                me->SummonCreature(FELMYST, x, y, z, m_creature->GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN, 0);
                bornTimer = 0;
                me->DisappearAndDie();
            }
        }
        else
            bornTimer -= diff;
    }
};

CreatureAI* GetAI_trigger_death_cloud(Creature* creature)
{
    return new trigger_death_cloudAI(creature);
}

void AddSC_boss_brutallus()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name="boss_brutallus";
    newscript->GetAI = &GetAI_boss_brutallus;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "at_brutallus_intro";
    newscript->pAreaTrigger = &AreaTrigger_at_brutallus_intro;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "trigger_death_cloud";
    newscript->GetAI = &GetAI_trigger_death_cloud;
    newscript->RegisterSelf();
}
