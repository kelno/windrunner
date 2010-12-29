#include "ScriptedInstance.h"
#include "../../game/Player.h"
#include "../../game/Map.h"
#include "../../game/MapReference.h"

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
