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
SDName: Boss_Kelidan_The_Breaker
SD%Complete: 100
SDComment:
SDCategory: Hellfire Citadel, Blood Furnace
EndScriptData */

/* ContentData
boss_kelidan_the_breaker
mob_shadowmoon_channeler
EndContentData */

#include "precompiled.h"
#include "def_blood_furnace.h"

#define SAY_WAKE                    -1542000

#define SAY_ADD_AGGRO_1             -1542001
#define SAY_ADD_AGGRO_2             -1542002
#define SAY_ADD_AGGRO_3             -1542003

#define SAY_KILL_1                  -1542004
#define SAY_KILL_2                  -1542005
#define SAY_NOVA                    -1542006
#define SAY_DIE                     -1542007

#define SPELL_CORRUPTION            30938
#define SPELL_EVOCATION             30935
#define SPELL_BURNING_NOVA          30940

#define SPELL_FIRE_NOVA             33132
#define H_SPELL_FIRE_NOVA           37371

#define SPELL_SHADOW_BOLT_VOLLEY    28599
#define H_SPELL_SHADOW_BOLT_VOLLEY  40070

#define ENTRY_KELIDAN               17377
#define ENTRY_CHANNELER             17653

const float ShadowmoonChannelers[5][4]=
{
    {302,-87,-24.4,0.157},
    {321,-63.5,-24.6,4.887},
    {346,-74.5,-24.6,3.595},
    {344,-103.5,-24.5,2.356},
    {316,-109,-24.6,1.257}
};

class BurningNovaAura : public Aura
{
    public:
        BurningNovaAura(SpellEntry *spell, uint32 eff, Unit *target, Unit *caster) : Aura(spell, eff, NULL, target, caster, NULL){}
};

struct boss_kelidan_the_breakerAI : public ScriptedAI
{
    boss_kelidan_the_breakerAI(Creature *c) : ScriptedAI(c)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
        HeroicMode = m_creature->GetMap()->IsHeroic();
        for(int i=0; i<5; ++i) Channelers[i] = 0;
    }

    ScriptedInstance* pInstance;
    bool HeroicMode;

    uint32 ShadowVolley_Timer;
    uint32 BurningNova_Timer;
    uint32 Firenova_Timer;
    uint32 Corruption_Timer;
    uint32 check_Timer;
    bool Firenova;
    bool addYell;
    uint64 Channelers[5];

    void Reset()
    {
        ShadowVolley_Timer = 1000;
        BurningNova_Timer = 15000;
        Corruption_Timer = 5000;
        check_Timer = 0;
        Firenova = false;
        addYell = false;
        SummonChannelers();
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        me->SetReactState(REACT_PASSIVE);
        SetCombatMovementAllowed(true);
        if (pInstance && me->IsAlive())
            pInstance->SetData(DATA_KELIDANEVENT, NOT_STARTED);
    }

    void EnterCombat(Unit *who)
    {
        DoScriptText(SAY_WAKE, m_creature);
        if (m_creature->IsNonMeleeSpellCasted(false))
            m_creature->InterruptNonMeleeSpells(true);
        DoStartMovement(who);
        if (pInstance)
            pInstance->SetData(DATA_KELIDANEVENT, IN_PROGRESS);
    }
    
    void MoveInLineOfSight(Unit* who)
    {
        if (me->HasAura(SPELL_EVOCATION))
            return;
    }

    void KilledUnit(Unit* victim)
    {
        if (rand()%2)
            return;

        DoScriptText(RAND(SAY_KILL_1, SAY_KILL_2), me);
    }

    void ChannelerEngaged(Unit* who)
    {
        if (who && !addYell)
        {
            addYell = true;
            /*switch(rand()%3)
            {
                case 0: DoScriptText(SAY_ADD_AGGRO_1, m_creature); break; // This was wrong anyway, should be who instead of m_creature to make add yell, not self
                case 1: DoScriptText(SAY_ADD_AGGRO_2, m_creature); break;
                default: DoScriptText(SAY_ADD_AGGRO_3, m_creature); break;
            }*/
        }
        for(int i=0; i<5; ++i)
        {
            Creature *channeler = Unit::GetCreature(*m_creature, Channelers[i]);
            if(who && channeler && !channeler->IsInCombat())
                channeler->AI()->AttackStart(who);
        }
    }

    void ChannelerDied(Unit* killer)
    {
        for(int i=0; i<5; ++i)
        {
            Creature *channeler = Unit::GetCreature(*m_creature, Channelers[i]);
            if(channeler && channeler->IsAlive())
                return;
        }

        //release me
        if (killer)
        {
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_AGGRESSIVE);
            me->AI()->AttackStart(killer);
        }
    }

    uint64 GetChanneled(Creature *channeler1)
    {
        SummonChannelers();
        if(!channeler1) return NULL;
        int i;
        for(i=0; i<5; ++i)
        {
            Creature *channeler = Unit::GetCreature(*m_creature, Channelers[i]);
            if(channeler && channeler->GetGUID()==channeler1->GetGUID())
                break;
        }
        return Channelers[(i+2)%5];
    }

    void SummonChannelers()
    {
        if (me->isDead())
            return;

        for(int i=0; i<5; ++i)
        {
            Creature *channeler = Unit::GetCreature(*m_creature, Channelers[i]);
            if(!channeler || channeler->isDead())
                channeler = m_creature->SummonCreature(ENTRY_CHANNELER,ShadowmoonChannelers[i][0],ShadowmoonChannelers[i][1],ShadowmoonChannelers[i][2],ShadowmoonChannelers[i][3],TEMPSUMMON_CORPSE_TIMED_DESPAWN,300000);
            if(channeler)
                Channelers[i] = channeler->GetGUID();
            else
                Channelers[i] = 0;
        }
    }

    void JustDied(Unit* Killer)
    {
        DoScriptText(SAY_DIE, m_creature);
        if(pInstance)
            pInstance->SetData(DATA_KELIDANEVENT, DONE);
    }

    void UpdateAI(const uint32 diff)
    {
        if (!UpdateVictim())
        {
            if(check_Timer < diff)
            {
                if (!m_creature->IsNonMeleeSpellCasted(false))
                    DoCast(m_creature,SPELL_EVOCATION);
                check_Timer = 5000;
            }else check_Timer -= diff;
            return;
        }
        
        float x, y, z, o;
        me->GetHomePosition(x, y, z, o);
        if (me->GetDistance(x, y, z) > 80.0f) {
            EnterEvadeMode();
            return;
        }

        if (Firenova)
        {
            if (Firenova_Timer < diff)
            {
                DoCast(m_creature,HeroicMode ? H_SPELL_FIRE_NOVA : SPELL_FIRE_NOVA,true);
                Firenova = false;
                SetCombatMovementAllowed(true);
                ShadowVolley_Timer = 2000;
            }else Firenova_Timer -=diff;

            return;
        }

        if (ShadowVolley_Timer < diff)
        {
            DoCast(m_creature,HeroicMode ? H_SPELL_SHADOW_BOLT_VOLLEY : SPELL_SHADOW_BOLT_VOLLEY);
            ShadowVolley_Timer = 5000+rand()%8000;
        }else ShadowVolley_Timer -=diff;

        if (Corruption_Timer < diff)
        {
            DoCast(m_creature,SPELL_CORRUPTION);
            Corruption_Timer = 30000+rand()%20000;
        }else Corruption_Timer -=diff;

        if (BurningNova_Timer < diff)
        {
            if (m_creature->IsNonMeleeSpellCasted(false))
                m_creature->InterruptNonMeleeSpells(true);

            DoScriptText(SAY_NOVA, m_creature);

            if(SpellEntry *nova = (SpellEntry*)spellmgr.LookupSpell(SPELL_BURNING_NOVA))
            {
                for(uint32 i = 0; i < 3; ++i)
                    if(nova->Effect[i] == SPELL_EFFECT_APPLY_AURA)
                    {
                        Aura *Aur = new BurningNovaAura(nova, i, m_creature, m_creature);
                        m_creature->AddAura(Aur);
                    }
            }

            if (HeroicMode)
                DoTeleportAll(m_creature->GetPositionX(),m_creature->GetPositionY(),m_creature->GetPositionZ(),m_creature->GetOrientation());

            BurningNova_Timer = 20000+rand()%8000;
            Firenova_Timer= 5000;
            Firenova = true;
            SetCombatMovementAllowed(false);
        }else BurningNova_Timer -=diff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_kelidan_the_breaker(Creature *_Creature)
{
    return new boss_kelidan_the_breakerAI (_Creature);
}

/*######
## mob_shadowmoon_channeler
######*/

#define SPELL_SHADOW_BOLT       12739
#define H_SPELL_SHADOW_BOLT     15472

#define SPELL_MARK_OF_SHADOW    30937
#define SPELL_CHANNELING        39123

#define CHANNELER_SAY_AGGRO_1   -1765300
#define CHANNELER_SAY_AGGRO_2   -1765301
#define CHANNELER_SAY_AGGRO_3   -1765302

struct mob_shadowmoon_channelerAI : public ScriptedAI
{
    mob_shadowmoon_channelerAI(Creature *c) : ScriptedAI(c)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
        HeroicMode = m_creature->GetMap()->IsHeroic();
    }

    ScriptedInstance* pInstance;
    bool HeroicMode;

    uint32 ShadowBolt_Timer;
    uint32 MarkOfShadow_Timer;
    uint32 check_Timer;

    void Reset()
    {
        ShadowBolt_Timer = 1000+rand()%1000;
        MarkOfShadow_Timer = 5000+rand()%2000;
        check_Timer = 0;
        if (m_creature->IsNonMeleeSpellCasted(false))
            m_creature->InterruptNonMeleeSpells(true);
    }
    
    void JustRespawned()
    {
        Creature* kelidan = me->FindNearestCreature(ENTRY_KELIDAN, 50.0f, true);
        if (!kelidan)
            me->Kill(me);
    }

    void EnterCombat(Unit* who)
    {
        if(Creature *Kelidan = FindCreature(ENTRY_KELIDAN, 100, m_creature)->ToCreature())
            ((boss_kelidan_the_breakerAI*)Kelidan->AI())->ChannelerEngaged(who);

        if (m_creature->IsNonMeleeSpellCasted(false))
            m_creature->InterruptNonMeleeSpells(true);

        DoScriptText(RAND(CHANNELER_SAY_AGGRO_1, CHANNELER_SAY_AGGRO_2, CHANNELER_SAY_AGGRO_3), me, NULL);

        DoStartMovement(who);
    }

    void JustDied(Unit* Killer)
    {
       if(Creature *Kelidan = FindCreature(ENTRY_KELIDAN, 100, m_creature)->ToCreature())
           ((boss_kelidan_the_breakerAI*)Kelidan->AI())->ChannelerDied(Killer);
    }

    void UpdateAI(const uint32 diff)
    {
        if (!UpdateVictim())
        {
            if(check_Timer < diff)
            {
                if (!m_creature->IsNonMeleeSpellCasted(false))
                    if(Creature *Kelidan = FindCreature(ENTRY_KELIDAN, 100, m_creature)->ToCreature())
                    {
                        uint64 channeler = ((boss_kelidan_the_breakerAI*)Kelidan->AI())->GetChanneled(m_creature);
                        if(Unit *channeled = Unit::GetUnit(*m_creature, channeler))
                            DoCast(channeled,SPELL_CHANNELING);
                    }
                check_Timer = 5000;
            }else check_Timer -= diff;
            return;
        }

        if (MarkOfShadow_Timer < diff)
        {
            if (Unit *target = SelectUnit(SELECT_TARGET_RANDOM, 0))
                DoCast(target,SPELL_MARK_OF_SHADOW);
            MarkOfShadow_Timer = 15000+rand()%5000;
        }else MarkOfShadow_Timer -=diff;

        if (ShadowBolt_Timer < diff)
        {
            DoCast(m_creature->GetVictim(),HeroicMode ? H_SPELL_SHADOW_BOLT : SPELL_SHADOW_BOLT);
            ShadowBolt_Timer = 5000+rand()%1000;
        }else ShadowBolt_Timer -=diff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_mob_shadowmoon_channeler(Creature *_Creature)
{
    return new mob_shadowmoon_channelerAI (_Creature);
}

void AddSC_boss_kelidan_the_breaker()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name="boss_kelidan_the_breaker";
    newscript->GetAI = &GetAI_boss_kelidan_the_breaker;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="mob_shadowmoon_channeler";
    newscript->GetAI = &GetAI_mob_shadowmoon_channeler;
    newscript->RegisterSelf();
}

