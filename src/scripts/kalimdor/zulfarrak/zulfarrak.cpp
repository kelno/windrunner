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
SDName: Zulfarrak
SD%Complete: 50
SDComment: Consider it temporary, no instance script made for this instance yet.
SDCategory: Zul'Farrak
EndScriptData */

/* ContentData
npc_sergeant_bly
npc_weegli_blastfuse
at_zumrah
at_antusul
go_shallow_grave
go_troll_cage
EndContentData */

#include "precompiled.h"
#include "def_zulfarrak.h"

/*######
## npc_sergeant_bly
######*/

enum blyAndCrewFactions {
    FACTION_HOSTILE         = 14,
    FACTION_FRIENDLY        = 35,       // While in cages (so the trolls won't attack them while they're caged)
    FACTION_FREED           = 250       // After release (so they'll be hostile towards trolls)
};

enum weegliActionParam {
    BLY_INITIATED = 1,
    PLAYER_INITIATED = 2
};

enum blySays {
    SAY_1 = -1209002,
    SAY_2 = -1209003
};

enum blySpells {
    SPELL_SHIELD_BASH          = 11972,
    SPELL_REVENGE              = 12170
};

#define GOSSIP_BLY_RESTART_EVENT    "Repartons au combat !"
#define GOSSIP_BLY                  "C'en est assez ! Je n'en peux plus de vous aider. Il est temps de régler nos comptes par les armes !"

struct npc_sergeant_blyAI : public ScriptedAI
{
    npc_sergeant_blyAI(Creature *c) : ScriptedAI(c)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
        postGossipStep = 0;
    }

    ScriptedInstance* pInstance;
    uint32 postGossipStep;
    uint32 Text_Timer;

    uint32 ShieldBash_Timer;
    uint32 Revenge_Timer;                                   //this is wrong, spell should never be used unless m_creature->GetVictim() dodge, parry or block attack. Trinity support required.

    uint64 gossipPlayerGUID;

    void Reset()
    {
        ShieldBash_Timer = 5000;
        Revenge_Timer = 8000;

        m_creature->setFaction(FACTION_FRIENDLY);
    }

    void EnterCombat(Unit *who) {}

    void JustDied(Unit *victim) {}

    void UpdateAI(const uint32 diff)
    {
        if (postGossipStep > 0 && postGossipStep < 4) {
            if (Text_Timer <= diff) {
                switch (postGossipStep)
                {
                case 1:
                    //weegli doesn't fight - he goes & blows up the door
                    if (Creature* weegli = pInstance->instance->GetCreature(pInstance->GetData64(ENTRY_WEEGLI))) {
                        weegli->AI()->DoAction(BLY_INITIATED);
                    }
                    DoScriptText(SAY_1,m_creature);
                    Text_Timer = 5000;
                    break;
                case 2:
                    DoScriptText(SAY_2,m_creature);
                    Text_Timer = 5000;
                    break;
                case 3:
                    if (pInstance) {
                        switchFactionIfAlive(pInstance, ENTRY_BLY);
                        switchFactionIfAlive(pInstance, ENTRY_RAVEN);
                        switchFactionIfAlive(pInstance, ENTRY_ORO);
                        switchFactionIfAlive(pInstance, ENTRY_MURTA);
                    }
                }
                postGossipStep++;
            } else Text_Timer -= diff;
        }

        if( !UpdateVictim() )
            return;

        if( ShieldBash_Timer < diff )
        {
            DoCast(m_creature->GetVictim(),SPELL_SHIELD_BASH);
            ShieldBash_Timer = 15000;
        }else ShieldBash_Timer -= diff;

        if( Revenge_Timer < diff )
        {
            DoCast(m_creature->GetVictim(),SPELL_REVENGE);
            Revenge_Timer = 10000;
        }else Revenge_Timer -= diff;

        DoMeleeAttackIfReady();
    }
    
    void DoAction(const int32 param) {
        postGossipStep=1;
        Text_Timer = 0;
    }
    
    void switchFactionIfAlive(ScriptedInstance* pInstance,uint32 entry) {
       if (Creature* crew = pInstance->instance->GetCreature(pInstance->GetData64(entry))) {
           if (crew->IsAlive()) {
                crew->setFaction(FACTION_HOSTILE);
                crew->SetHealth(crew->GetMaxHealth());
                if (Player* target = Player::GetPlayer(gossipPlayerGUID)) {
                    crew->AI()->AttackStart(target);
                }
           }
       }
    }
};
CreatureAI* GetAI_npc_sergeant_bly(Creature *_Creature)
{
    return new npc_sergeant_blyAI (_Creature);
}

enum blyGossips {
    BLY_GOSSIP_CAGED = 1515,
    BLY_GOSSIP_FIGHTING = 1516,
    BLY_GOSSIP_DONE = 1517
};

bool GossipHello_npc_sergeant_bly(Player *player, Creature* creature )
{
    if (ScriptedInstance* pInstance = ((ScriptedInstance*)creature->GetInstanceData())) {
        if (pInstance->GetData(EVENT_PYRAMID) == PYRAMID_KILLED_ALL_TROLLS) {
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_BLY, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
            player->SEND_GOSSIP_MENU(BLY_GOSSIP_DONE, creature->GetGUID());
        }
        else if (pInstance->GetData(EVENT_PYRAMID) == PYRAMID_NOT_STARTED)
            player->SEND_GOSSIP_MENU(BLY_GOSSIP_CAGED, creature->GetGUID());
        else
            player->SEND_GOSSIP_MENU(BLY_GOSSIP_FIGHTING, creature->GetGUID());
        return true;
    }
    return false;
}

bool GossipSelect_npc_sergeant_bly(Player *player, Creature *creature, uint32 sender, uint32 action )
{
    if (action == GOSSIP_ACTION_INFO_DEF+1) {
        player->CLOSE_GOSSIP_MENU();
        CAST_AI(npc_sergeant_blyAI,creature->AI())->gossipPlayerGUID = player->GetGUID();
        CAST_AI(npc_sergeant_blyAI,creature->AI())->DoAction(0);
    }
    else if (action == GOSSIP_ACTION_INFO_DEF+2) {
        player->CLOSE_GOSSIP_MENU();
        CAST_AI(npc_sergeant_blyAI,creature->AI())->gossipPlayerGUID = player->GetGUID();
        CAST_AI(npc_sergeant_blyAI,creature->AI())->DoAction(0);
    }

    return true;
}

void initBlyCrewMember(Player* pPlayer, uint32 entry,float x,float y, float z)
{
    if (Creature* crew = pPlayer->FindCreatureInGrid(entry, 10.0f, true)) {
        crew->addUnitState(UNIT_STAT_IGNORE_PATHFINDING);
        crew->SetReactState(REACT_AGGRESSIVE);
        crew->AddUnitMovementFlag(MOVEMENTFLAG_WALK_MODE);
        crew->SetHomePosition(x,y,z,0);
        crew->GetMotionMaster()->MovePoint(1,x,y,z);
        crew->setFaction(FACTION_FREED);
    }
}

/*######
## go_troll_cage
######*/

bool GOHello_go_troll_cage(Player* pPlayer, GameObject* pGo)
{
    if (ScriptedInstance* pInstance = ((ScriptedInstance*)pGo->GetInstanceData())) {
        pInstance->SetData(EVENT_PYRAMID, PYRAMID_CAGES_OPEN);
        //set bly & co to aggressive & start moving to top of stairs
        initBlyCrewMember(pPlayer,ENTRY_BLY,1884.99,1263,41.52);
        initBlyCrewMember(pPlayer,ENTRY_RAVEN,1882.5,1263,41.52);
        initBlyCrewMember(pPlayer,ENTRY_ORO,1886.47,1270.68,41.68);
        initBlyCrewMember(pPlayer,ENTRY_WEEGLI,1890,1263,41.52);
        initBlyCrewMember(pPlayer,ENTRY_MURTA,1891.19,1272.03,41.60);
    }
    return false;
}

/*######
## npc_weegli_blastfuse
######*/

enum weegliSpells {
    SPELL_BOMB                 = 8858,
    SPELL_GOBLIN_LAND_MINE     = 21688,
    SPELL_SHOOT                = 6660,
    SPELL_WEEGLIS_BARREL       = 10772
};

enum weegliSays {
    SAY_WEEGLI_OHNO         = -1209000,
    SAY_WEEGLI_OK_I_GO      = -1209001,
    SAY_WEEGLI_OUT_OF_HERE  = -1209004
};

#define GOSSIP_WEEGLI               "[PH] Allez faire exploser la porte, s'il vous plait."

struct npc_weegli_blastfuseAI : public ScriptedAI
{
    npc_weegli_blastfuseAI(Creature *c) : ScriptedAI(c)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
        destroyingDoor=false;
        Bomb_Timer = 10000;
        LandMine_Timer = 30000;
        
    }

    uint32 Bomb_Timer;
    uint32 LandMine_Timer;
    bool destroyingDoor;
    ScriptedInstance* pInstance;

    void Reset()
    {
        /*if( pInstance )
            pInstance->SetData(0, NOT_STARTED);*/
    }

    void EnterCombat(Unit *who) {}

    void JustDied(Unit *victim) {}
    
    void MovementInform(uint32 type, uint32 id)
    {
        if (pInstance) {
            if (pInstance->GetData(EVENT_PYRAMID) == PYRAMID_CAGES_OPEN) {
                pInstance->SetData(EVENT_PYRAMID,PYRAMID_ARRIVED_AT_STAIR);
                DoScriptText(SAY_WEEGLI_OHNO,m_creature);
                m_creature->SetHomePosition(1882.69,1272.28,41.87,0);
            } else if (destroyingDoor) {
                //pInstance->HandleGameObject(pInstance->GetData64(GO_END_DOOR), true, NULL);
                //pInstance->SetData(DATA_OPEN_END_DOOR, DONE);
                //if (GameObject* door = GameObject::GetGameObject(*me, pInstance->GetData64(GO_END_DOOR)))
                    //door->UseDoorOrButton();
                if (GameObject* door = me->FindGOInGrid(GO_END_DOOR, 50.0f))
                    door->UseDoorOrButton();
                //TODO: leave the area...
                m_creature->ForcedDespawn();
            }
        }
    }
    
    void DoAction(const int32 param) {
        DestroyDoor(param);
    }
    
    void DestroyDoor(int32 param) 
    {
        if (m_creature->IsAlive()) {
            m_creature->setFaction(FACTION_FRIENDLY);
            m_creature->GetMotionMaster()->MovePoint(0, 1858.57,1146.35,14.745);
            m_creature->SetHomePosition(1858.57,1146.35,14.745,3.85); // in case he gets interrupted
            if (param == BLY_INITIATED) {
                DoScriptText(SAY_WEEGLI_OUT_OF_HERE,m_creature);
            } else {
                DoScriptText(SAY_WEEGLI_OK_I_GO,m_creature);
            }
            destroyingDoor=true;
        }
    }

    void UpdateAI(const uint32 diff)
    {
        if (!UpdateVictim())
            return;
            
        if (Bomb_Timer < diff ) {
            DoCast(m_creature->GetVictim(),SPELL_BOMB);
            Bomb_Timer = 10000;
        } else Bomb_Timer -= diff;

        if (m_creature->isAttackReady() && !m_creature->IsWithinMeleeRange(m_creature->GetVictim())) {
            DoCast(m_creature->GetVictim(),SPELL_SHOOT);
            m_creature->SetSheath(SHEATH_STATE_RANGED);
        } else {
            m_creature->SetSheath(SHEATH_STATE_MELEE);
            DoMeleeAttackIfReady();
        }
    }
};
CreatureAI* GetAI_npc_weegli_blastfuse(Creature *_Creature)
{
    return new npc_weegli_blastfuseAI (_Creature);
}

enum weegliGossips {
    WEEGLI_GOSSIP_CAGED = 1511,
    WEEGLI_GOSSIP_FIGHTING = 1513,
    WEEGLI_GOSSIP_DONE = 1514
};

bool GossipHello_npc_weegli_blastfuse(Player *player, Creature *creature )
{
    if (ScriptedInstance* pInstance = ((ScriptedInstance*)creature->GetInstanceData())) {
        switch (pInstance->GetData(EVENT_PYRAMID))
        {
        case PYRAMID_KILLED_ALL_TROLLS:
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_WEEGLI, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
            player->SEND_GOSSIP_MENU(WEEGLI_GOSSIP_DONE, creature->GetGUID());  //if event can proceed to end
            break;
        case PYRAMID_NOT_STARTED:
            player->SEND_GOSSIP_MENU(WEEGLI_GOSSIP_CAGED, creature->GetGUID());  //if event not started        
            break;
        default:
            player->SEND_GOSSIP_MENU(WEEGLI_GOSSIP_FIGHTING, creature->GetGUID());  //if event are in progress
        }
        return true;
    }
    
    return false;
}

bool GossipSelect_npc_weegli_blastfuse(Player *player, Creature *creature, uint32 sender, uint32 action )
{
    if( action == GOSSIP_ACTION_INFO_DEF+1 )
    {
        player->CLOSE_GOSSIP_MENU();
        //here we make him run to door, set the charge and run away off to nowhere
        creature->AI()->DoAction(PLAYER_INITIATED);
    }

    return true;
}

/*######
## at_zumrah
######*/

bool AreaTrigger_at_zumrah(Player *pPlayer, AreaTriggerEntry const *pAt)
{
    if (Creature *pZumrah = pPlayer->FindNearestCreature(7271, 15.0f, true)) {
        pZumrah->setFaction(14);
        pZumrah->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_OOC_NOT_ATTACKABLE);
        pZumrah->AI()->AttackStart(pPlayer);
    }
    
    return true;
}

/*######
## at_antusul
######*/

bool AreaTrigger_at_antusul(Player* player, const AreaTriggerEntry* at)
{
    if (Creature* antusul = player->FindCreatureInGrid(8127, 70.0f, true))
        antusul->AI()->AttackStart(player);
        
    return true;
}

/*######
## go_shallow_grave
######*/

enum {
    ZOMBIE = 7286,
    DEAD_HERO = 7276,
    ZOMBIE_CHANCE = 65,
    DEAD_HERO_CHANCE = 10
};

bool GOHello_go_shallow_grave(Player* pPlayer, GameObject* pGo)
{
    if (pGo->GetUseCount() == 0) {
        uint32 randomchance = urand(0,100);
        if (randomchance < ZOMBIE_CHANCE)
            pGo->SummonCreature(ZOMBIE, pGo->GetPositionX(), pGo->GetPositionY(), pGo->GetPositionZ(), 0, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 30000);
        else if ((randomchance-ZOMBIE_CHANCE) < DEAD_HERO_CHANCE)
            pGo->SummonCreature(DEAD_HERO, pGo->GetPositionX(), pGo->GetPositionY(), pGo->GetPositionZ(), 0, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 30000);
    }
    pGo->AddUse();
    return false;
}

void AddSC_zulfarrak()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name="npc_sergeant_bly";
    newscript->GetAI = &GetAI_npc_sergeant_bly;
    newscript->pGossipHello =  &GossipHello_npc_sergeant_bly;
    newscript->pGossipSelect = &GossipSelect_npc_sergeant_bly;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="npc_weegli_blastfuse";
    newscript->GetAI = &GetAI_npc_weegli_blastfuse;
    newscript->pGossipHello =  &GossipHello_npc_weegli_blastfuse;
    newscript->pGossipSelect = &GossipSelect_npc_weegli_blastfuse;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "at_zumrah";
    newscript->pAreaTrigger = &AreaTrigger_at_zumrah;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "at_antusul";
    newscript->pAreaTrigger = &AreaTrigger_at_antusul;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "go_shallow_grave";
    newscript->pGOHello = &GOHello_go_shallow_grave;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "go_troll_cage";
    newscript->pGOHello = &GOHello_go_troll_cage;
    newscript->RegisterSelf();
}

