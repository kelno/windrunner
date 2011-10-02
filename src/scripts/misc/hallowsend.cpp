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
    
    void Reset() {}
    
    void Aggro(Unit* who) {}
    
    bool sOnDummyEffect(Unit* caster, uint32 spellId, uint32 effIndex)
    {
        sLog.outString("Pom1");
        if (!caster->ToPlayer())
            return false;
        sLog.outString("Pom2");
        if (spellId == 42339) {
            sLog.outString("Pom3");
            caster->ToPlayer()->KilledMonster(me->GetEntry(), me->GetGUID());
            me->DisappearAndDie();
        }
        
        return true;
    }
};

CreatureAI* GetAI_fire_effigy_fire(Creature* creature)
{
    return new npc_fire_effigy_fire(creature);
}

void AddSC_hallows_end()
{
    Script* newscript;
    
    newscript = new Script;
    newscript->Name = "npc_fire_effigy_fire";
    newscript->GetAI = &GetAI_fire_effigy_fire;
    newscript->RegisterSelf();
}    

