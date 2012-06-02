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
SDName: boss_rage_winterchill
SD%Complete: 100
SDCategory: Hyjal
EndScriptData */

#include "precompiled.h"
#include "def_hyjal.h"
#include "hyjal_trash.h"

#define SPELL_FROST_ARMOR 31256
#define SPELL_DEATH_AND_DECAY 31258

#define SPELL_FROST_NOVA 31250
#define SPELL_ICEBOLT 31249

#define SPELL_BERSERK 26662

#define SAY_ONDEATH "Vous avez gagné cette bataille, mais... pas... la guerre !"
#define SOUND_ONDEATH 11026

#define SAY_ONSLAY1 "Toute vie doit périr !"
#define SAY_ONSLAY2 "Victoire à la Légion !"
#define SOUND_ONSLAY1 11025
#define SOUND_ONSLAY2 11057

#define SAY_DECAY1 "Tombez et pourrissez !"
#define SAY_DECAY2 "Des cendres aux cendres, de poussière à poussière !"
#define SOUND_DECAY1 11023
#define SOUND_DECAY2 11055

#define SAY_NOVA1 "Succombez au frisson glacé... de la mort !"
#define SAY_NOVA2 "Il fera bien plus froid dans vos tombes !"
#define SOUND_NOVA1 11024
#define SOUND_NOVA2 11058

#define SAY_ONAGGRO "L'ultime invasion de la Légion a commencé ! Une fois de plus, l'asservissement de ce monde est à notre portée. Ne faites pas de quartier !"
#define SOUND_ONAGGRO 11022

struct boss_rage_winterchillAI : public hyjal_trashAI
{
    boss_rage_winterchillAI(Creature *c) : hyjal_trashAI(c)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
        go = false;
        pos = 0;
    }

    uint32 FrostArmorTimer;
    uint32 DecayTimer;
    uint32 NovaTimer;
    uint32 IceboltTimer;
    uint32 BerserkTimer;
    bool Enraged;
    bool go;
    uint32 pos;

    void Reset()
    {
        damageTaken = 0;
        FrostArmorTimer = 37000;
        DecayTimer = 45000;
        NovaTimer = 15000;
        IceboltTimer = 10000;
        BerserkTimer = 600000; //10 minutes
        
        Enraged = false;

        if(pInstance && IsEvent)
            pInstance->SetData(DATA_RAGEWINTERCHILLEVENT, NOT_STARTED);
    }

    void Aggro(Unit *who)
    {
        if(pInstance && IsEvent)
            pInstance->SetData(DATA_RAGEWINTERCHILLEVENT, IN_PROGRESS);
        DoPlaySoundToSet(m_creature, SOUND_ONAGGRO);
        DoYell(SAY_ONAGGRO, LANG_UNIVERSAL, NULL);
    }

    void KilledUnit(Unit *victim)
    {
        switch(rand()%2)
        {
            case 0:
                DoPlaySoundToSet(m_creature, SOUND_ONSLAY1);
                DoYell(SAY_ONSLAY1, LANG_UNIVERSAL, NULL);
                break;
            case 1:
                DoPlaySoundToSet(m_creature, SOUND_ONSLAY2);
                DoYell(SAY_ONSLAY2, LANG_UNIVERSAL, NULL);
                break;
        }
    }

    void WaypointReached(uint32 i)
    {
        pos = i;
        if (i == 7 && pInstance)
        {
            Unit* target = Unit::GetUnit((*m_creature), pInstance->GetData64(DATA_JAINAPROUDMOORE));
            if (target && target->isAlive())
                m_creature->AddThreat(target,0.0);
        }
    }

    void JustDied(Unit *victim)
    {
        hyjal_trashAI::JustDied(victim);
        if(pInstance && IsEvent)
            pInstance->SetData(DATA_RAGEWINTERCHILLEVENT, DONE);
        DoPlaySoundToSet(m_creature, SOUND_ONDEATH);
        DoYell(SAY_ONDEATH, LANG_UNIVERSAL, NULL);
    }

    void UpdateAI(const uint32 diff)
    {
        if (IsEvent)
        {
            //Must update npc_escortAI
            npc_escortAI::UpdateAI(diff);
            if(!go)
            {
                go = true;
                if(pInstance)
                {
                    ((npc_escortAI*)(m_creature->AI()))->AddWaypoint(0, 4896.08,    -1576.35,    1333.65);
                    ((npc_escortAI*)(m_creature->AI()))->AddWaypoint(1, 4898.68,    -1615.02,    1329.48);
                    ((npc_escortAI*)(m_creature->AI()))->AddWaypoint(2, 4907.12,    -1667.08,    1321.00);
                    ((npc_escortAI*)(m_creature->AI()))->AddWaypoint(3, 4963.18,    -1699.35,    1340.51);
                    ((npc_escortAI*)(m_creature->AI()))->AddWaypoint(4, 4989.16,    -1716.67,    1335.74);
                    ((npc_escortAI*)(m_creature->AI()))->AddWaypoint(5, 5026.27,    -1736.89,    1323.02);
                    ((npc_escortAI*)(m_creature->AI()))->AddWaypoint(6, 5037.77,    -1770.56,    1324.36);
                    ((npc_escortAI*)(m_creature->AI()))->AddWaypoint(7, 5067.23,    -1789.95,    1321.17);
                    ((npc_escortAI*)(m_creature->AI()))->Start(false, true, true);
                    ((npc_escortAI*)(m_creature->AI()))->SetDespawnAtEnd(false);
                }
            }
        }

        //Return since we have no target
        if (!UpdateVictim() )
            return;

        if(FrostArmorTimer < diff)
        {
            DoCast(m_creature, SPELL_FROST_ARMOR);
            FrostArmorTimer = 40000+rand()%20000;
        }else FrostArmorTimer -= diff;
        if(DecayTimer < diff)
        {
            DoCast(SelectUnit(SELECT_TARGET_RANDOM, 0, 30.0f, true), SPELL_DEATH_AND_DECAY);
            DecayTimer = 60000+rand()%20000;
            switch(rand()%2)
            {
                case 0:
                    DoPlaySoundToSet(m_creature, SOUND_DECAY1);
                    DoYell(SAY_DECAY1, LANG_UNIVERSAL, NULL);
                    break;
                case 1:
                    DoPlaySoundToSet(m_creature, SOUND_DECAY2);
                    DoYell(SAY_DECAY2, LANG_UNIVERSAL, NULL);
                    break;
            }
        }else DecayTimer -= diff;
        if(NovaTimer < diff)
        {
            DoCast(m_creature->getVictim(), SPELL_FROST_NOVA);
            NovaTimer = 30000+rand()%15000;
            switch(rand()%2)
            {
                case 0:
                    DoPlaySoundToSet(m_creature, SOUND_NOVA1);
                    DoYell(SAY_NOVA1, LANG_UNIVERSAL, NULL);
                    break;
                case 1:
                    DoPlaySoundToSet(m_creature, SOUND_NOVA2);
                    DoYell(SAY_NOVA2, LANG_UNIVERSAL, NULL);
                    break;
            }
        }else NovaTimer -= diff;
        if(IceboltTimer < diff)
        {
            DoCast(SelectUnit(SELECT_TARGET_RANDOM,0,40.0f,true), SPELL_ICEBOLT);
            IceboltTimer = 11000+rand()%20000;
        }else IceboltTimer -= diff;
        if(BerserkTimer < diff && !Enraged)
        {
            DoCast(m_creature,SPELL_BERSERK);
            Enraged = true;
        }

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_rage_winterchill(Creature *_Creature)
{
    return new boss_rage_winterchillAI (_Creature);
}

void AddSC_boss_rage_winterchill()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name="boss_rage_winterchill";
    newscript->GetAI = &GetAI_boss_rage_winterchill;
    newscript->RegisterSelf();
}
