#include "ScriptedInstance.h"
#include "../../game/Player.h"
#include "../../game/Map.h"
#include "../../game/MapReference.h"
#include "../../game/Chat.h"
#include "../../game/Language.h"

void ScriptedInstance::CastOnAllPlayers(uint32 spellId)
{
    Map::PlayerList const& players = instance->GetPlayers();

    if (!players.isEmpty())
    {
        for(Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
        {
            if (Player* plr = itr->getSource())
                plr->CastSpell(plr, spellId, true);                
        }
    }
}

void ScriptedInstance::RemoveAuraOnAllPlayers(uint32 spellId)
{
    Map::PlayerList const& players = instance->GetPlayers();

    if (!players.isEmpty())
    {
        for(Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
        {
            if (Player* plr = itr->getSource())
                plr->RemoveAurasDueToSpell(spellId);
        }
    }
}

void ScriptedInstance::MonsterPulled(Creature* creature, Unit* puller)
{
    
}

void ScriptedInstance::PlayerDied(Player* player)
{
    
}

void ScriptedInstance::SendScriptInTestNoLootMessageToAll()
{
    Map::PlayerList const& players = instance->GetPlayers();

    if (!players.isEmpty()) {
        for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr) {
            if (Player* plr = itr->getSource())
                ChatHandler(plr).SendSysMessage(LANG_SCRIPT_IN_TEST_NO_LOOT);
        }
    }
}
