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
#include "../../npc/npc_escortAI.h"

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

void AddSC_arathi_highlands()
{
    Script * newscript;

    newscript = new Script;
    newscript->Name = "npc_professor_phizzlethorpe";
    newscript->GetAI = &GetAI_npc_professor_phizzlethorpeAI;
    newscript->pQuestAccept = &QuestAccept_npc_professor_phizzlethorpe;
    newscript->RegisterSelf();
}

