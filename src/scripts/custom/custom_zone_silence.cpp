/* Copyright (C) 2006 - 2008 WoWMania Core <https://scriptdev2.svn.sourceforge.net/>
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
SDName: npc_zone_silence
SD%Complete: 100
SDComment: Custom NPC acting like a totem silencing nearby players
SDCategory: Custom
EndScriptData */

#include "precompiled.h"

struct npc_zone_silenceAI : public Scripted_NoMovementAI
{
    npc_zone_silenceAI(Creature* c) : Scripted_NoMovementAI(c) {}
    
    void Aggro(Unit* who) {}
    
    void MoveInLineOfSight(Unit* who)
    {
        if (who->ToPlayer() || (who->ToCreature() && who->ToCreature()->isPet())) {
            if (who->IsWithinDistInMap(me, 30.0f))
                DoCast(who, 42201, true);
            else if (who->GetDistance(me) >= 30.0f && who->GetDistance(me) <= 40.0f)
                who->RemoveAurasDueToSpell(42201);
        }
    }
};

CreatureAI* GetAI_npc_zone_silence(Creature* creature)
{
    return new npc_zone_silenceAI(creature);
}

void AddSC_zone_silence()
{
    Script* newscript;
    
    newscript = new Script;
    newscript->Name = "npc_zone_silence";
    newscript->GetAI = &GetAI_npc_zone_silence;
    newscript->RegisterSelf();
}