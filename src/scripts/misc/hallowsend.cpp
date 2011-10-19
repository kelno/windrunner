/*
 * Copyright (C) 2005-2008 MaNGOS <http://www.mangosproject.org/>
 *
 * Copyright (C) 2008 Trinity <http://www.trinitycore.org/>
 *
 * Copyright (C) 2009 - 2011 Windrunner
 *
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

#include "precompiled.h"

struct npc_fire_effigy_fire : public Scripted_NoMovementAI
{
    npc_fire_effigy_fire(Creature* c) : Scripted_NoMovementAI(c)
    {
        me->SetReactState(REACT_PASSIVE);
    }
    
    uint32 addAuraTimer;
    
    void Reset()
    {
        addAuraTimer = 0;
    }
    
    void Aggro(Unit* who) {}
    
    void DamageTaken(Unit* doneBy, uint32& damage)
    {
        damage = 0;
    }
    
    bool sOnDummyEffect(Unit* caster, uint32 spellId, uint32 effIndex)
    {
        if (!caster->ToPlayer())
            return false;
            
        if (!me->HasAura(42074))
            return false;

        if (spellId == 42339) {
            uint32 questId = 0;

            switch (me->GetZoneId()) {
            case 85: // Tirisfal
                questId = 11449;
                break;
            case 14: // Durotar
                questId = 11361;
                break;
            case 3430: // Eversong Woods
                questId = 11450;
                break;
            case 12: // Elwynn
                questId = 11360;
                break;
            case 3524: // Azure Watch
                questId = 11440;
                break;
            case 1: // Dun Morogh
                questId = 11439;
                break;
            default:
                break;
            }
            
            if (questId)
                caster->ToPlayer()->KilledMonster(me->GetEntry(), me->GetGUID(), questId);
            
            me->RemoveAurasDueToSpell(42074);
            addAuraTimer = 10000;
        }
        
        return true;
    }
    
    void UpdateAI(uint32 const diff)
    {
        if (addAuraTimer) {
            if (addAuraTimer <= diff) {
                DoCast(me, 42074, true);
                addAuraTimer = 0;
            }
            else
                addAuraTimer -= diff;
        }
    }
};

CreatureAI* GetAI_fire_effigy_fire(Creature* creature)
{
    return new npc_fire_effigy_fire(creature);
}

bool GOHello_go_wickerman_ember(Player* player, GameObject* go)
{
    player->CastSpell(player, 24705, true);
    
    return true;
}

void AddSC_hallows_end()
{
    Script* newscript;
    
    newscript = new Script;
    newscript->Name = "npc_fire_effigy_fire";
    newscript->GetAI = &GetAI_fire_effigy_fire;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "go_wickerman_ember";
    newscript->pGOHello = &GOHello_go_wickerman_ember;
    newscript->RegisterSelf();
}    

