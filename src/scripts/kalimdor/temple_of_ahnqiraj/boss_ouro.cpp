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
SDName: Boss_Ouro
SD%Complete: 85
SDComment: No model for submerging. Currently just invisible.
SDCategory: Temple of Ahn'Qiraj
EndScriptData */

#include "precompiled.h"
#include "def_temple_of_ahnqiraj.h"
 
#define CREATURE_OURO_MOUND        15712
#define CREATURE_OURO_SCARAB    15718
 
#define DISPLAYID_OURO            15509
#define DISPLAYID_MOUND            15294
 
#define SPELL_SWEEP             26103
#define SPELL_SANDBLAST         26102 //incorrect angle (must be 180°), core fix needed
#define SPELL_GROUND_RUPTURE    26100
#define SPELL_EMERGE            26262
#define SPELL_SUBMERGE            26063
#define SPELL_SUMMON_DIRTMOUNDS 26058 //unused
#define SPELL_BERSERK            26615 //incomplete effect
#define SPELL_BOULDER            26616
#define SPELL_QUAKE                26093 //incorrect damage
//missing hole visual
 
#define SWEEP_TIMER             15000 + rand()%5000
#define SANDBLAST_TIMER         25000 + rand()%10000
#define EMERGED_TIMER           90000
#define SUBMERGED_TIMER         35000 + rand()%10000
#define SCARABS_SPAWN_TIMER     15000
#define ANIM_TIMER              2500
#define IWANTATANK_TIMER1       8000
#define IWANTATANK_TIMER2       2000
#define QUAKE_TIMER             1000
#define NEWTARGET_TIMER         5000
 
enum PhasesOuro
{
    PHASE_EMERGED           =   1,
    PHASE_SUBMERGED         =   2,
    PHASE_ANIM_EMERGING     =   3,
    PHASE_BERSERK           =   4,
};
 
struct boss_ouroAI : public Scripted_NoMovementAI
{
    SummonList Summons;
    uint16 Sweep_Timer;
    uint16 SandBlast_Timer;
    uint32 Emerged_Timer;
    uint32 Submerged_Timer;
    uint16 Scarabs_Spawn_Timer;
    uint16 Anim_Timer;
    uint32 IWantATank_Timer;
    uint16 Quake_Timer;
    uint16 NewTarget_Timer;
    uint8 Phase;
    uint16 Morphed_Timer;
    bool Morphed;
    const CreatureInfo* cinfo;
    float homeX, homeY, homeZ;
 
    boss_ouroAI(Creature *c) : 
        Scripted_NoMovementAI(c), 
        Summons(m_creature),
        cinfo(me->GetCreatureInfo())
    {
        me->GetHomePosition(homeX,homeY,homeZ,*new float());
    }
 
    void Reset()
    {
        me->SetDisplayId(DISPLAYID_MOUND);
        me->setFaction(14);
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        DoCast(me,SPELL_SUBMERGE,true);
        ResetAllTimers();
        Phase = PHASE_ANIM_EMERGING;
        Summons.DespawnAll();
        if (cinfo)
        {
            me->SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, cinfo->mindmg);
            me->SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, cinfo->maxdmg);
            me->UpdateDamagePhysical(BASE_ATTACK);
        }
    }
 
    void ResetAllTimers()
    {
        Sweep_Timer = rand()%5000;
        SandBlast_Timer = rand()%5000;
        Emerged_Timer = EMERGED_TIMER;
        Submerged_Timer = SUBMERGED_TIMER;
        Scarabs_Spawn_Timer = SCARABS_SPAWN_TIMER;
        Anim_Timer = ANIM_TIMER;
        IWantATank_Timer = IWANTATANK_TIMER1;
        Quake_Timer = QUAKE_TIMER;
        NewTarget_Timer = NEWTARGET_TIMER;
        Morphed_Timer = ANIM_TIMER;
    }
 
    void Aggro(Unit *who)
    {
        me->SetDisplayId(DISPLAYID_OURO);
        me->RemoveAurasDueToSpell(SPELL_SUBMERGE);
        DoCast(me->getVictim(), SPELL_EMERGE);
        DoZoneInCombat();
    }
 
    void UpdateAI(const uint32 diff)
    {
        if (me->IsNonMeleeSpellCasted(false))
            return;
 
        if (!UpdateVictim())
        {
            if (Quake_Timer < diff)
            {
                DoCast(me, SPELL_QUAKE);
                Quake_Timer = QUAKE_TIMER;
            }else Quake_Timer -= diff;
            return;
        }
 
        if ((me->GetHealth()*100 / me->GetMaxHealth()) <= 20.0 && !me->HasAura(SPELL_BERSERK, 0))
        {
            EnterPhase(PHASE_BERSERK);
        }
 
        switch (Phase)
        {
        case PHASE_EMERGED:
 
            if (GetMeleeVictim())
                IWantATank_Timer = IWANTATANK_TIMER1;
 
            if (IWantATank_Timer < diff) {
                Emerged_Timer = 0; //end phase
            } else IWantATank_Timer -= diff;
 
            if (Sweep_Timer < diff)
            {
                DoCast(me, SPELL_SWEEP);
                Sweep_Timer = SWEEP_TIMER;
            }else Sweep_Timer -= diff;
 
            if (SandBlast_Timer < diff)
            {
                Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 0, 200, true);
                if (target)
                {
                    DoCast(target, SPELL_SANDBLAST);
                    SandBlast_Timer = SANDBLAST_TIMER;                
                }
            }else SandBlast_Timer -= diff;
 
            if (Emerged_Timer < diff)
            {
                EnterPhase(PHASE_SUBMERGED);
            }else Emerged_Timer -= diff;
 
            DoMeleeAttackIfReady();
            break;
 
        case PHASE_SUBMERGED:
 
            if (!Morphed)
            {
                if (Morphed_Timer < diff)
                {
                    me->SetDisplayId(DISPLAYID_MOUND);
                    Morphed = true;
                } else Morphed_Timer -= diff;
            }
 
            if (me->GetDistance(homeX,homeY,homeZ) > 150)
            {
                //DoTeleportTo(x,y,z);  //Don't EVER use this again
                EnterEvadeMode();
            }
 
            if (Quake_Timer < diff)
            {
                DoCast(me, SPELL_QUAKE);
                Quake_Timer = QUAKE_TIMER;
            }else Quake_Timer -= diff;
 
            float x, y, z;
            if (NewTarget_Timer < diff || !me->GetMotionMaster()->GetDestination(x,y,z) ) //= dont move anymore
            {
                if (Submerged_Timer > 5000)
                {
                    Unit* target = NULL;
                    target = SelectUnit(SELECT_TARGET_RANDOM, 0, 200,true);
                    if (target)
                    {
                        NewTarget_Timer = NEWTARGET_TIMER;
                        //me->GetMotionMaster()->MoveChase(target); //buggy visual
                        target->GetPosition(x,y,z);
                        me->GetMotionMaster()->MovePoint(0,x,y,z);
                    }
                } else {
                    me->GetMotionMaster()->Clear();
                }
            } else NewTarget_Timer -= diff;
 
            if (Morphed && Submerged_Timer < 1000)
            {
                me->SetDisplayId(DISPLAYID_OURO);
                me->GetMotionMaster()->Clear(true);
                Morphed = false;
            }
 
            if (Submerged_Timer < diff)
            {
                EnterPhase(PHASE_ANIM_EMERGING);
            }else Submerged_Timer -= diff;            
            break;
 
        case PHASE_ANIM_EMERGING:
 
            if (Anim_Timer < diff)
            {
                EnterPhase(PHASE_EMERGED);
            } else Anim_Timer -= diff;
            break;
 
        case PHASE_BERSERK:
 
            if (!GetMeleeVictim())
            {
                if (IWantATank_Timer < diff)
                {
                    Unit* target = NULL;
                    target = SelectUnit(SELECT_TARGET_RANDOM, 0, 120,true);
                    if(target)
                        DoCast(target, SPELL_BOULDER, false);
                    else //im so alone
                        EnterEvadeMode();
 
                } else IWantATank_Timer -= diff;
            } else IWantATank_Timer = IWANTATANK_TIMER2;
 
            if (Scarabs_Spawn_Timer < diff)
            {
                SummonBugs(15);
                Scarabs_Spawn_Timer = SCARABS_SPAWN_TIMER;
            } else Scarabs_Spawn_Timer -= diff;
 
            if (Sweep_Timer < diff)
            {
                DoCast(me, SPELL_SWEEP);
                Sweep_Timer = SWEEP_TIMER;
            }else Sweep_Timer -= diff;
 
            DoMeleeAttackIfReady();
            break;
        }
    }
 
    void EnterPhase(int NextPhase)
    {
        switch(NextPhase)
        {
        case PHASE_EMERGED:
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->setFaction(14);
            break;
        case PHASE_SUBMERGED:
            Morphed = false;
            me->RemoveAllAuras();
            DoCast(me,SPELL_SUBMERGE,false);
            Submerged_Timer = SUBMERGED_TIMER;
            SummonMounds(4);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->setFaction(35);
            NewTarget_Timer = 0;    
            break;
        case PHASE_ANIM_EMERGING:
            if (Morphed) //just in case very high time diff, may not be done yet
            {
                me->SetDisplayId(DISPLAYID_OURO);
                Morphed = false;
            }
            me->RemoveAurasDueToSpell(SPELL_SUBMERGE);
            DoCast(me,SPELL_EMERGE,false);
            DoGroundRupture();
            SummonBugs(15);
            ResetAllTimers();
            break;
        case PHASE_BERSERK:
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->setFaction(14);
            IWantATank_Timer = IWANTATANK_TIMER2;
            DoCast(me, SPELL_BERSERK);
            //hack +100% melee damage.
            if (cinfo)
            {
                me->SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, 2 * cinfo->mindmg);
                me->SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, 2 * cinfo->maxdmg);
                me->UpdateDamagePhysical(BASE_ATTACK);
            }
            break;
        }
        Phase = NextPhase;
    }
 
    void EnterEvadeMode()
    {
        me->DisappearAndDie();
        me->Respawn();
    }
 
    void JustDied(Unit* /* who */)
    {
        Summons.DespawnAll(); //not blizz!§§ becuz else tiziz just onerous
    }
 
    void SummonBugs(int amount)
    {
        for (int i = 0; i < amount; i++)
        {
            Unit* target = NULL;
            target = SelectUnit(SELECT_TARGET_RANDOM, 0, 150,true);
            if(target)
            {
                Creature* Bug = me->SummonCreature(CREATURE_OURO_SCARAB, target->GetPositionX(), target->GetPositionY(),
                    target->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN, 45000);
                if (Bug)
                {
                    Summons.Summon(Bug);
                    Bug->AddThreat(target, 1000000.0);
                }
            }
        }
    }
   
    void SummonMounds(int amount)
    {
        for (int i = 0; i < amount; i++)
            Creature* Mound = DoSpawnCreature(CREATURE_OURO_MOUND, 0, 0, 0, 0, TEMPSUMMON_TIMED_DESPAWN, (Submerged_Timer - 5000));
    }
 
    void DoGroundRupture()
    {
        std::list<HostilReference*>& m_threatlist = me->getThreatManager().getThreatList();
        std::list<HostilReference*>::iterator i = m_threatlist.begin();
        for (i = m_threatlist.begin(); i!= m_threatlist.end();++i)
        {
            Unit* pUnit = Unit::GetUnit((*me), (*i)->getUnitGuid());
            if(pUnit && me->IsWithinMeleeRange(pUnit))
            {
               DoCast(pUnit, SPELL_GROUND_RUPTURE, true);
            }
        }
    }
 
    bool GetMeleeVictim()
    {
        if (me->IsWithinMeleeRange(me->getVictim()))
            return true;
        else
        {
            Unit* target = NULL;
            float MaxThreat = 0;
            std::list<HostilReference*>& m_threatlist = me->getThreatManager().getThreatList();
            std::list<HostilReference*>::iterator i = m_threatlist.begin();
            for (i = m_threatlist.begin(); i!= m_threatlist.end();++i)
            {
                Unit* pUnit = Unit::GetUnit((*me), (*i)->getUnitGuid());
                if(pUnit && me->IsWithinMeleeRange(pUnit))
                {
                    if ((*i)->getThreat() > MaxThreat)
                    {
                        target = pUnit;
                        MaxThreat = (*i)->getThreat();
                    }
                }
            }
            if (target)
            {
                AttackStart(target);
                return true;
            }
        }
        return false;
    }
};
 
struct boss_ouro_moundAI : public Scripted_NoMovementAI
{
    uint16 Quake_Timer;
    uint16 NewTarget_Timer;
    Creature* Master;
 
    boss_ouro_moundAI(Creature *c) : Scripted_NoMovementAI(c)
    {
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        Quake_Timer = 0;
    }
    
    void Aggro(Unit* who) {}
 
    void UpdateAI(const uint32 diff)    
    {
        Unit* target = NULL;
        if (NewTarget_Timer < diff || !me->isMoving())
        {
            target = SelectUnit(SELECT_TARGET_RANDOM, 0, 200,true);
            NewTarget_Timer = NEWTARGET_TIMER;
        } else NewTarget_Timer -= diff;
 
        if (Quake_Timer < diff)
        {
            if (target)
            {
                float x, y ,z;
                //me->GetMotionMaster()->MoveChase(target); //buggy
                target->GetPosition(x,y,z);
                me->GetMotionMaster()->MovePoint(0,x,y,z);
            }
            DoCast(me, SPELL_QUAKE);
            Quake_Timer = QUAKE_TIMER;
        }else Quake_Timer -= diff;
    }
    void MoveInLineOfSight(Unit *who)
    {
        m_creature->AddThreat(who,0.0);
    }
};
 
CreatureAI* GetAI_boss_ouro(Creature *_Creature)
{
    return new boss_ouroAI (_Creature);
}
 
CreatureAI* GetAI_boss_ouro_mound(Creature *_Creature)
{
    return new boss_ouro_moundAI (_Creature);
}
 
void AddSC_boss_ouro()
{
    Script *newscript;
 
    newscript = new Script;
    newscript->Name="boss_ouro";
    newscript->GetAI = &GetAI_boss_ouro;
    newscript->RegisterSelf();
 
    newscript = new Script;
    newscript->Name="boss_ouro_mound";
    newscript->GetAI = &GetAI_boss_ouro_mound;
    newscript->RegisterSelf();
}