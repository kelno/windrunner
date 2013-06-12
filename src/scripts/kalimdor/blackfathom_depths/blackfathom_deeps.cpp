/*
* Copyright (C) 2008-2009 Trinity <http://www.trinitycore.org/>
*
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

#include "precompiled.h"
#include "def_blackfathom_deeps.h"

enum Spells
{
    SPELL_BLESSING_OF_BLACKFATHOM                          = 8733
};

bool GoHello_blackfathom_altar(Player *pPlayer, GameObject* pGo)
{
    if (!pPlayer->HasAura(SPELL_BLESSING_OF_BLACKFATHOM))
        pPlayer->AddAura(SPELL_BLESSING_OF_BLACKFATHOM,pPlayer);
    return true;
}

bool GoHello_blackfathom_fire(Player *pPlayer, GameObject* pGo)
{
    ScriptedInstance* pInstance = ((ScriptedInstance*)pGo->GetInstanceData());
    
    if (pInstance)
    {
        pGo->SetGoState(GO_STATE_ACTIVE);
        pGo->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_UNK1);
        switch (pGo->GetEntry()) {
        case 21118: // North West
            for (uint8 i = 0; i < 4; i++) {
                if (Creature* summon = pPlayer->SummonCreature(4825, -769.013123, -153.537262, -25.879938, 3.121223, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 60000))
                    summon->AI()->AttackStart(pPlayer);
            }
            break;
        case 21119: // North East
            for (uint8 i = 0; i < 4; i++) {
                if (Creature* summon = pPlayer->SummonCreature(4823, -768.617126, -174.640518, -25.870447, 2.980356, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 60000))
                    summon->AI()->AttackStart(pPlayer);
            }
            break;
        case 21120: // South East
            for (uint8 i = 0; i < 2; i++) {
                if (Creature* summon = pPlayer->SummonCreature(4978, -868.376831, -174.436081, -25.870043, 0.109292, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 60000))
                    summon->AI()->AttackStart(pPlayer);
            }
            break;
        case 21121: // South West
            for (uint8 i = 0; i < 8; i++) {
                if (Creature* summon = pPlayer->SummonCreature(4977, -868.235535, -153.906906, -25.881172, 6.167040, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 60000))
                    summon->AI()->AttackStart(pPlayer);
            }
            break;
        }
        pInstance->SetData(DATA_FIRE, pInstance->GetData(DATA_FIRE) + 1);
        return true;
    }
    return false;
}


void AddSC_blackfathom_deeps()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name = "go_blackfathom_altar";
    newscript->pGOHello = &GoHello_blackfathom_altar;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "go_blackfathom_fire";
    newscript->pGOHello = &GoHello_blackfathom_fire;
    newscript->RegisterSelf();
}
