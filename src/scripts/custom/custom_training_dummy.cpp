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
SDName: npc_training_dummy
SD%Complete: 100
SDComment: Custom NPC Training Dummy, like in Wotlk
SDCategory: Custom
EndScriptData */

#include "precompiled.h"

typedef UNORDERED_MAP<uint64, uint32> AttackerMap;

struct npc_training_dummy : Scripted_NoMovementAI
{
    npc_training_dummy(Creature *c) : Scripted_NoMovementAI(c)
    {
        m_entry = c->GetEntry();
    }

    uint64 m_entry;
    
    AttackerMap attackers;
    
    void Reset()
    {
        m_creature->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
        m_creature->SetUInt64Value(UNIT_FIELD_TARGET, m_creature->GetGUID()); // prevent from rotating
        m_creature->SetHealth(m_creature->GetMaxHealth()/5);
    }

    void DamageTaken(Unit* done_by, uint32& damage)
    {
        if (done_by->GetTypeId() == TYPEID_PLAYER)
            attackers[done_by->GetGUID()] = 8000;
        else if (done_by->ToCreature() && done_by->ToCreature()->isPet()) {
            if (Unit* owner = done_by->ToCreature()->GetOwner())
                attackers[owner->GetGUID()] = 8000;
        }

        if (m_creature->GetHealth() < (m_creature->GetMaxHealth()/10.0f) || m_creature->GetHealth() > (m_creature->GetMaxHealth()/5.0f)) // allow players using finishers
            m_creature->SetHealth(m_creature->GetMaxHealth()/5);

        m_creature->SetUInt64Value(UNIT_FIELD_TARGET, m_creature->GetGUID()); // prevent from rotating
    }

    void Aggro(Unit *who)
    {
        m_creature->SetUInt64Value(UNIT_FIELD_TARGET, m_creature->GetGUID()); // prevent from rotating
    }

    void UpdateAI(const uint32 diff)
    {
        for (AttackerMap::iterator itr = attackers.begin(); itr != attackers.end();)
        {
            if (itr->second <= diff)
            {
                if (Player* attacker = Unit::GetPlayer(itr->first))
                {
                    attacker->CombatStop(true);
                    attacker->AttackStop();
                    attacker->CombatStopWithPets(true);
                    attacker->ClearInCombat();
                }

                itr = attackers.erase(itr);

                if (attackers.empty())
                {
                    EnterEvadeMode();
                    m_creature->SetHealth(m_creature->GetMaxHealth()/5);
                }
            }
            else
            {
                itr->second -= diff;
                ++itr;
            }
        }
    }
};

CreatureAI* GetAI_npc_training_dummy(Creature* pCreature)
{
    return new npc_training_dummy(pCreature);
}

void AddSC_training_dummy()
{
    Script* newscript;
    
    newscript = new Script;
    newscript->Name = "npc_training_dummy";
    newscript->GetAI = &GetAI_npc_training_dummy;
    newscript->RegisterSelf();
}
