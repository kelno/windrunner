/* Copyright (C) 2006 - 2008 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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

/* ScriptData
SDName: boss_Halazzi
SD%Complete: 80
SDComment:
SDCategory: Zul'Aman
EndScriptData */

#include "precompiled.h"
#include "def_zulaman.h"
//#include "spell.h"

#define YELL_AGGRO "À genoux, les idiots… devant la griffe et le croc !"
#define SOUND_AGGRO                     12020
#define YELL_SABER_ONE "Vous partirez pas en un seul morceau !"
#define SOUND_SABER_ONE                 12024
#define YELL_SABER_TWO "J'vais te couper en deux !"
#define SOUND_SABER_TWO                 12023
#define YELL_SPLIT "L'esprit en moi, il est indompté !"
#define SOUND_SPLIT                    12021
#define YELL_MERGE "Esprit, reviens à moi !"
#define SOUND_MERGE                    12022
#define YELL_KILL_ONE "J'ai trop la puissance pour vous !"
#define SOUND_KILL_ONE                12026
#define YELL_KILL_TWO "Vous allez tous échouer !"
#define SOUND_KILL_TWO                12027
#define YELL_DEATH "Chaga... choka'jinn."
#define SOUND_DEATH                    12028
#define YELL_BERSERK "Alors les gars, on retient ses coups ?"
#define SOUND_BERSERK                12025

#define SPELL_DUAL_WIELD                29651
#define SPELL_SABER_LASH                43267
#define SPELL_FRENZY                    43139
#define SPELL_FLAMESHOCK                43303
#define SPELL_EARTHSHOCK                43305
#define SPELL_TRANSFORM_SPLIT           43142
#define SPELL_TRANSFORM_SPLIT2          43573
#define SPELL_TRANSFORM_MERGE           43271
#define SPELL_SUMMON_LYNX               43143
#define SPELL_SUMMON_TOTEM              43302
#define SPELL_BERSERK                   45078

#define MOB_SPIRIT_LYNX                 24143
#define SPELL_LYNX_FRENZY               43290
#define SPELL_SHRED_ARMOR               43243

#define MOB_TOTEM                       24224
#define SPELL_LIGHTNING                 43301

enum PhaseHalazzi
{
    PHASE_NONE = 0,
    PHASE_LYNX = 1,
    PHASE_SPLIT = 2,
    PHASE_HUMAN = 3,
    PHASE_MERGE = 4,
    PHASE_ENRAGE = 5
};

struct boss_halazziAI : public ScriptedAI
{
    boss_halazziAI(Creature *c) : ScriptedAI(c)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
        // need to find out what controls totem's spell cooldown
        /*SpellEntry *TempSpell = (SpellEntry*)spellmgr.LookupSpell(SPELL_LIGHTNING);
        if(TempSpell && TempSpell->CastingTimeIndex != 5)
            TempSpell->CastingTimeIndex = 5; // 2000 ms casting time*/
    }

    ScriptedInstance *pInstance;

    uint32 FrenzyTimer;
    uint32 SaberlashTimer;
    uint32 ShockTimer;
    uint32 TotemTimer;
    uint32 CheckTimer;
    uint32 BerserkTimer;

    uint32 TransformCount;

    PhaseHalazzi Phase;

    uint64 LynxGUID;

    void Reset()
    {
        if(pInstance && pInstance->GetData(DATA_HALAZZIEVENT) != DONE)
            pInstance->SetData(DATA_HALAZZIEVENT, NOT_STARTED);

        TransformCount = 0;
        BerserkTimer = 600000;
        CheckTimer = 1000;

        DoCast(m_creature, SPELL_DUAL_WIELD, true);

        Phase = PHASE_NONE;
        EnterPhase(PHASE_LYNX);
    }

    void EnterCombat(Unit *who)
    {
        if(pInstance)
            pInstance->SetData(DATA_HALAZZIEVENT, IN_PROGRESS);

        DoYell(YELL_AGGRO, LANG_UNIVERSAL, NULL);
        DoPlaySoundToSet(m_creature, SOUND_AGGRO);

        EnterPhase(PHASE_LYNX);
    }

    void JustSummoned(Creature* summon)
    {
        summon->AI()->AttackStart(m_creature->GetVictim());
        if(summon->GetEntry() == MOB_SPIRIT_LYNX)
            LynxGUID = summon->GetGUID();
    }

    void DamageTaken(Unit *done_by, uint32 &damage)
    {
        if(damage >= m_creature->GetHealth() && Phase != PHASE_ENRAGE)
            damage = 0;
    }

    void SpellHit(Unit*, const SpellEntry *spell)
    {
        if(spell->Id == SPELL_TRANSFORM_SPLIT2)
            EnterPhase(PHASE_HUMAN);
    }

    void AttackStart(Unit *who)
    {
        if(Phase != PHASE_MERGE) ScriptedAI::AttackStart(who);
    }

    void EnterPhase(PhaseHalazzi NextPhase)
    {
        switch(NextPhase)
        {
        case PHASE_LYNX:
        case PHASE_ENRAGE:
            if(Phase == PHASE_MERGE)
            {
                m_creature->CastSpell(m_creature, SPELL_TRANSFORM_MERGE, true);
                m_creature->Attack(m_creature->GetVictim(), true);
                m_creature->GetMotionMaster()->MoveChase(m_creature->GetVictim());
            }
            if(Unit *Lynx = Unit::GetUnit(*m_creature, LynxGUID))
            {
                Lynx->SetVisibility(VISIBILITY_OFF);
                Lynx->setDeathState(JUST_DIED);
            }
            m_creature->SetMaxHealth(600000);
            m_creature->SetHealth(600000 - 150000 * TransformCount);
            FrenzyTimer = 16000;
            SaberlashTimer = 20000;
            ShockTimer = 10000;
            TotemTimer = 12000;
            break;
        case PHASE_SPLIT:
            DoYell(YELL_SPLIT, LANG_UNIVERSAL, NULL);
            DoPlaySoundToSet(m_creature, SOUND_SPLIT);
            m_creature->CastSpell(m_creature, SPELL_TRANSFORM_SPLIT, true);
            break;
        case PHASE_HUMAN:
            //DoCast(m_creature, SPELL_SUMMON_LYNX, true);
            DoSpawnCreature(MOB_SPIRIT_LYNX, 5,5,0,0, TEMPSUMMON_CORPSE_DESPAWN, 0);
            m_creature->SetMaxHealth(400000);
            m_creature->SetHealth(400000);
            ShockTimer = 10000;
            TotemTimer = 12000;
            break;
        case PHASE_MERGE:
            if(Unit *Lynx = Unit::GetUnit(*m_creature, LynxGUID))
            {
                DoYell(YELL_MERGE, LANG_UNIVERSAL, NULL);
                DoPlaySoundToSet(m_creature, SOUND_MERGE);
                Lynx->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                Lynx->GetMotionMaster()->Clear();
                Lynx->GetMotionMaster()->MoveFollow(m_creature, 0, 0);
                m_creature->GetMotionMaster()->Clear();
                m_creature->GetMotionMaster()->MoveFollow(Lynx, 0, 0);
                TransformCount++;
            }break;
        default:
            break;
        }
        Phase = NextPhase;
    }

     void UpdateAI(const uint32 diff)
    {
        if(!UpdateVictim())
            return;

        if(BerserkTimer < diff)
        {
            DoYell(YELL_BERSERK, LANG_UNIVERSAL, NULL);
            DoPlaySoundToSet(m_creature, SOUND_BERSERK);
            DoCast(m_creature, SPELL_BERSERK, true);
            BerserkTimer = 60000;
        }else BerserkTimer -= diff;

        if(Phase == PHASE_LYNX || Phase == PHASE_ENRAGE)
        {
            if(SaberlashTimer < diff)
            {
                switch(rand()%2) {
                case 0:
                    DoYell(YELL_SABER_ONE, LANG_UNIVERSAL, NULL);
                    DoPlaySoundToSet(me, SOUND_SABER_ONE);
                    break;

                case 1:
                    DoYell(YELL_SABER_TWO, LANG_UNIVERSAL, NULL);
                    DoPlaySoundToSet(me, SOUND_SABER_TWO);
                    break;
                }
                // A tank with more than 490 defense skills should receive no critical hit
                //m_creature->CastSpell(m_creature, 41296, true);
                m_creature->CastSpell(m_creature->GetVictim(), SPELL_SABER_LASH, true);
                //m_creature->RemoveAurasDueToSpell(41296);
                SaberlashTimer = 30000;
            }else SaberlashTimer -= diff;

            if(FrenzyTimer < diff)
            {
                DoCast(m_creature, SPELL_FRENZY);
                FrenzyTimer = (10+rand()%5)*1000;
            }else FrenzyTimer -= diff;

            if(Phase == PHASE_LYNX)
                if(CheckTimer < diff)
                {
                    if(m_creature->GetHealth() * 4 < m_creature->GetMaxHealth() * (3 - TransformCount))
                        EnterPhase(PHASE_SPLIT);
                    CheckTimer = 1000;
                }else CheckTimer -= diff;
        }

        if(Phase == PHASE_HUMAN || Phase == PHASE_ENRAGE)
        {
            if(TotemTimer < diff)
            {
                //DoCast(m_creature, SPELL_SUMMON_TOTEM);
                float totemX, totemY, totemZ;
                m_creature->GetRandomPoint(m_creature->GetPositionX(),m_creature->GetPositionY(),m_creature->GetPositionZ(), 1.0f, totemX, totemY, totemZ);
                m_creature->SummonCreature(MOB_TOTEM, totemX, totemY, totemZ, 0, TEMPSUMMON_DEAD_DESPAWN, 0);
                TotemTimer = 20000;
            }else TotemTimer -= diff;

            if(ShockTimer < diff)
            {
                if(Unit* target = SelectUnit(SELECT_TARGET_RANDOM,0,80.0f,true))
                {
                    if(target->IsNonMeleeSpellCasted(false))
                        DoCast(target,SPELL_EARTHSHOCK);
                    else
                        DoCast(target,SPELL_FLAMESHOCK);
                    ShockTimer = 10000 + rand()%5000;
                }
            }else ShockTimer -= diff;

            if(Phase == PHASE_HUMAN)
                if(CheckTimer < diff)
                {
                    if( ((m_creature->GetHealth()*100) / m_creature->GetMaxHealth() <= 20)/*m_creature->GetHealth() * 10 < m_creature->GetMaxHealth()*/)
                        EnterPhase(PHASE_MERGE);
                    else /* WTF? */
                    {
                        Unit *Lynx = Unit::GetUnit(*m_creature, LynxGUID);
                        if(Lynx && ((Lynx->GetHealth()*100) / Lynx->GetMaxHealth() <= 20)/*Lynx->GetHealth() * 10 < Lynx->GetMaxHealth()*/)
                            EnterPhase(PHASE_MERGE);
                    }
                    CheckTimer = 1000;
                }else CheckTimer -= diff;
        }

        if(Phase == PHASE_MERGE)
        {
            if(CheckTimer < diff)
            {
                Unit *Lynx = Unit::GetUnit(*m_creature, LynxGUID);
                if(Lynx)
                {
                    Lynx->GetMotionMaster()->MoveFollow(m_creature, 0, 0);
                    m_creature->GetMotionMaster()->MoveFollow(Lynx, 0, 0);
                    if(m_creature->IsWithinDistInMap(Lynx, 6.0f))
                    {
                        if(TransformCount < 3)
                            EnterPhase(PHASE_LYNX);
                        else
                            EnterPhase(PHASE_ENRAGE);
                    }
                }
                CheckTimer = 1000;
            }else CheckTimer -= diff;
        }

        DoMeleeAttackIfReady();
    }

    void KilledUnit(Unit* victim)
    {
        switch(rand()%2)
        {
        case 0:
            DoYell(YELL_KILL_ONE, LANG_UNIVERSAL, NULL);
            DoPlaySoundToSet(m_creature, SOUND_KILL_ONE);
            break;

        case 1:
            DoYell(YELL_KILL_TWO, LANG_UNIVERSAL, NULL);
            DoPlaySoundToSet(m_creature, SOUND_KILL_TWO);
            break;
        }
    }

    void JustDied(Unit* Killer)
    {
        if(pInstance)
            pInstance->SetData(DATA_HALAZZIEVENT, DONE);

        DoYell(YELL_DEATH, LANG_UNIVERSAL, NULL);
        DoPlaySoundToSet(m_creature, SOUND_DEATH);
    }
};

// Spirits Lynx AI

struct boss_spiritlynxAI : public ScriptedAI
{
    boss_spiritlynxAI(Creature *c) : ScriptedAI(c) {}

    uint32 FrenzyTimer;
    uint32 shredder_timer;

    void Reset()
    {
        FrenzyTimer = (30+rand()%20)*1000;  //frenzy every 30-50 seconds
        shredder_timer = 4000;
    }

    void DamageTaken(Unit *done_by, uint32 &damage)
    {
        if(damage >= m_creature->GetHealth())
            damage = 0;
    }

    void AttackStart(Unit *who)
    {
        if(!m_creature->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE))
            ScriptedAI::AttackStart(who);
    }

    void EnterCombat(Unit *who) {/*DoZoneInCombat();*/}

    void UpdateAI(const uint32 diff)
    {
        if (!UpdateVictim())
            return;

        if(FrenzyTimer < diff)
        {
            DoCast(m_creature, SPELL_LYNX_FRENZY);
            FrenzyTimer = (30+rand()%20)*1000;
        }else FrenzyTimer -= diff;

        if(shredder_timer < diff)
        {
            DoCast(m_creature->GetVictim(), SPELL_SHRED_ARMOR);
            shredder_timer = 4000;
        }else shredder_timer -= diff;

        DoMeleeAttackIfReady();
    }

};

struct TRINITY_DLL_DECL npc_corruptedlightningtotemAI : public Scripted_NoMovementAI
{
    npc_corruptedlightningtotemAI(Creature *c) : Scripted_NoMovementAI(c) {}
    
    uint32 globalCD;
    
    void Reset()
    {
        globalCD = 100;     // Begin after 100 ms and then every 1000 ms
    }
    
    void EnterCombat(Unit* pWho) {}
    
    void UpdateAI(const uint32 diff)
    {
        if (!UpdateVictim())
            return;
            
        Creature *halazzi = m_creature->FindCreatureInGrid(23577, 150.0f, true);
        if (!halazzi)
            m_creature->DisappearAndDie();
            
        if (globalCD < diff) {
            Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 0, 30.0f, true);
            if (target) {
                DoCast(target, SPELL_LIGHTNING);
                globalCD = 1000;
            }
        }else globalCD -= diff;
    }
};

CreatureAI* GetAI_boss_halazziAI(Creature *_Creature)
{
    return new boss_halazziAI (_Creature);
}

CreatureAI* GetAI_boss_spiritlynxAI(Creature *_Creature)
{
    return new boss_spiritlynxAI (_Creature);
}

CreatureAI* GetAI_npc_corruptedlightningtotemAI(Creature* pCreature)
{
    return new npc_corruptedlightningtotemAI(pCreature);
}

void AddSC_boss_halazzi()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name="boss_halazzi";
    newscript->GetAI = &GetAI_boss_halazziAI;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="mob_halazzi_lynx";
    newscript->GetAI = &GetAI_boss_spiritlynxAI;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "totem_halazzi";
    newscript->GetAI = &GetAI_npc_corruptedlightningtotemAI;
    newscript->RegisterSelf();
}

