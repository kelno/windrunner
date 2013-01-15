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
SDName: Arathi Highlands
SD%Complete: 100
SDComment: Quest support: 665
SDCategory: Arathi Highlands
EndScriptData */

/* ContentData
npc_professor_phizzlethorpe
EndContentData */

#include "precompiled.h"
#include "EscortAI.h"

/*######
## npc_professor_phizzlethorpe
######*/

enum ePhizzleThorpe
{
SAY_PROGRESS_1          = -1000235,
SAY_PROGRESS_2          = -1000236,
SAY_PROGRESS_3          = -1000237,
EMOTE_PROGRESS_4        = -1000238,
SAY_AGGRO               = -1000239,
SAY_PROGRESS_5          = -1000240,
SAY_PROGRESS_6          = -1000241,
SAY_PROGRESS_7          = -1000242,
EMOTE_PROGRESS_8        = -1000243,
SAY_PROGRESS_9          = -1000244,

QUEST_SUNKEN_TREASURE   = 665,
MOB_VENGEFUL_SURGE      = 2776
};

struct npc_professor_phizzlethorpeAI : public npc_escortAI
{
    npc_professor_phizzlethorpeAI(Creature *c) : npc_escortAI(c) {}

    bool Completed;

    void WaypointReached(uint32 i)
    {
        Player* player = GetPlayerForEscort();

        switch(i)
        {
        case 4:DoScriptText(SAY_PROGRESS_2, m_creature, player);break;
        case 5:DoScriptText(SAY_PROGRESS_3, m_creature, player);break;
        case 8:DoScriptText(EMOTE_PROGRESS_4, m_creature);break;
        case 9:
            {
            m_creature->SummonCreature(MOB_VENGEFUL_SURGE, -2052.96, -2142.49, 20.15, 1.0f, TEMPSUMMON_CORPSE_DESPAWN, 0);
            m_creature->SummonCreature(MOB_VENGEFUL_SURGE, -2052.96, -2142.49, 20.15, 1.0f, TEMPSUMMON_CORPSE_DESPAWN, 0);
            break;
            }
        case 10:DoScriptText(SAY_PROGRESS_5, m_creature, player);break;
        case 11:DoScriptText(SAY_PROGRESS_6, m_creature, player);break;
        case 19:DoScriptText(SAY_PROGRESS_7, m_creature, player); break;
        case 20:
            DoScriptText(EMOTE_PROGRESS_8, m_creature);
            DoScriptText(SAY_PROGRESS_9, m_creature, player);
            Completed = true;
            if(player)
                player->GroupEventHappens(QUEST_SUNKEN_TREASURE, m_creature);
            break;
        }
    }

    void JustSummoned(Creature *summoned)
    {
        summoned->AI()->AttackStart(m_creature);
    }

    void Reset()
    {
        Completed = true;
        m_creature->setFaction(35);
    }

    void Aggro(Unit* who)
    {
        DoScriptText(SAY_AGGRO, m_creature, NULL);
    }

    void JustDied(Unit* killer)
    {
        if (PlayerGUID && !Completed )
        {
            Player* player = GetPlayerForEscort();
            if (player)
                player->FailQuest(QUEST_SUNKEN_TREASURE);
        }
    }

    void UpdateAI(const uint32 diff)
    {
        npc_escortAI::UpdateAI(diff);
    }
};

bool QuestAccept_npc_professor_phizzlethorpe(Player* pPlayer, Creature* pCreature, Quest const* pQuest)
{
    if (pQuest->GetQuestId() == QUEST_SUNKEN_TREASURE)
    {
        DoScriptText(SAY_PROGRESS_1, pCreature, pPlayer);
        if (npc_escortAI* pEscortAI = CAST_AI(npc_professor_phizzlethorpeAI, (pCreature->AI())))
            pEscortAI->Start(false, false, false, pPlayer->GetGUID(), pCreature->GetEntry());
            
        pCreature->setFaction(113);
    }
    return true;
}

CreatureAI* GetAI_npc_professor_phizzlethorpeAI(Creature *pCreature)
{
    return new npc_professor_phizzlethorpeAI(pCreature);
}

struct npc_myzraelAI : public ScriptedAI
{
    npc_myzraelAI(Creature* c) : ScriptedAI(c)
    {
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        me->SetReactState(REACT_PASSIVE);
        
        intro = true;
        introStep = 0;
        introTimer = 1000;
    }
    
    uint32 summon1Timer;
    uint32 summon2Timer;
    uint32 seismeTimer;
    bool intro;
    uint32 introTimer;
    uint8 introStep;
    
    void Reset()
    {
        summon1Timer = 10000;
        summon2Timer = 30000;
        seismeTimer = 20000;
    }
    
    void Aggro(Unit* who) {}
    
    void UpdateAI(uint32 const diff)
    {
        if (intro) {
            if (introTimer <= diff) {
                switch (introStep) {
                case 0:
                    me->MonsterSay("Quoi ? Hé bien, vous m'avez servi avec dévotion, mais...", LANG_UNIVERSAL, 0);
                    ++introStep;
                    introTimer = 3000;
                    break;
                case 1:
                    me->MonsterSay("Pourquoi m'avez-vous invoquée si tôt ? Je n'ai pas encore récupéré tout mon pouvoir !", LANG_UNIVERSAL, 0);
                    ++introStep;
                    introTimer = 3000;
                    break;
                case 2:
                    me->MonsterSay("Aucune importance. Vous avez été fou de m'aider, et maintenant vous allez payer !", LANG_UNIVERSAL, 0);
                    introTimer = 0;
                    intro = false;
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    me->SetReactState(REACT_AGGRESSIVE);
                    break;
                }
            }
            else
                introTimer -= diff;
            
            return;
        }
        
        if (!UpdateVictim())
            return;
        
        if (summon1Timer <= diff) {
            DoCast(me->getVictim(), 4937, true);
            summon1Timer = 20000;
        }
        else
            summon1Timer -= diff;
        
        if (summon2Timer <= diff) {
            DoCast(me->getVictim(), 10388, true);
            summon2Timer = 20000;
        }
        else
            summon2Timer -= diff;
        
        if (seismeTimer <= diff) {
            DoCast(me, 4938, false);
            seismeTimer = 20000;
        }
        else
            seismeTimer -= diff;
        
        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_myzrael(Creature* c)
{
    return new npc_myzraelAI(c);
}

void AddSC_arathi_highlands()
{
    Script * newscript;

    newscript = new Script;
    newscript->Name = "npc_professor_phizzlethorpe";
    newscript->GetAI = &GetAI_npc_professor_phizzlethorpeAI;
    newscript->pQuestAccept = &QuestAccept_npc_professor_phizzlethorpe;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_myzrael";
    newscript->GetAI = &GetAI_npc_myzrael;
    newscript->RegisterSelf();
}

