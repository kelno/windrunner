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
SDName: Karazhan
SD%Complete: 100
SDComment: Support for Barnes (Opera controller) and Berthold (Doorman), Support for Quest 9645.
SDCategory: Karazhan
EndScriptData */

/* ContentData
npc_barnes
npc_berthold
npc_image_of_medivh
npc_archmage_leryda
go_sealed_tome
woefulhealer
npc_arcane_anomaly
EndContentData */

#include "precompiled.h"
#include "def_karazhan.h"
#include "EscortAI.h"
#include "Chat.h"
#include "ObjectMgr.h"

/*######
# npc_barnesAI
######*/

#define GOSSIP_READY        "I'm not an actor."

#define SAY_READY           "Splendid, I'm going to get the audience ready. Break a leg!"
#define SAY_OZ_INTRO1       "Finally, everything is in place. Are you ready for your big stage debut?"
#define OZ_GOSSIP1          "I'm not an actor."
#define SAY_OZ_INTRO2       "Don't worry, you'll be fine. You look like a natural!"
#define OZ_GOSSIP2          "Ok, I'll give it a try, then."

#define SAY_RAJ_INTRO1      "The romantic plays are really tough, but you'll do better this time. You have TALENT. Ready?"
#define RAJ_GOSSIP1         "I've never been more ready."

struct Dialogue
{
    int32 textid;
    uint32 timer;
};

static Dialogue OzDialogue[]=
{
    {-1532103, 6000},
    {-1532104, 18000},
    {-1532105, 9000},
    {-1532106, 15000}
};

static Dialogue HoodDialogue[]=
{
    {-1532107, 6000},
    {-1532108, 10000},
    {-1532109, 14000},
    {-1532110, 15000}
};

static Dialogue RAJDialogue[]=
{
    {-1532111, 5000},
    {-1532112, 7000},
    {-1532113, 14000},
    {-1532114, 14000}
};

// Entries and spawn locations for creatures in Oz event
float Spawns[6][2]=
{
    {17535, -10896},                                        // Dorothee
    {17546, -10891},                                        // Roar
    {17547, -10884},                                        // Tinhead
    {17543, -10902},                                        // Strawman
    {17603, -10892},                                        // Grandmother
    {17534, -10900},                                        // Julianne
};

float StageLocations[6][2]=
{
    {-10866.711, -1779.816},                                // Open door, begin walking (0)
    {-10894.917, -1775.467},                                // (1)
    {-10896.044, -1782.619},                                // Begin Speech after this (2)
    {-10894.917, -1775.467},                                // Resume walking (back to spawn point now) after speech (3)
    {-10866.711, -1779.816},                                // (4)
    {-10866.700, -1781.030}                                 // Summon mobs, open curtains, close door (5)
};

#define CREATURE_SPOTLIGHT  19525

#define SPELL_SPOTLIGHT     25824
#define SPELL_TUXEDO        32616

#define SPAWN_Z             90.5
#define SPAWN_Y             -1758
#define SPAWN_O             4.738

struct npc_barnesAI : public npc_escortAI
{
    npc_barnesAI(Creature* c) : npc_escortAI(c)
    {
        RaidWiped = false;
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
    }

    ScriptedInstance* pInstance;

    uint64 SpotlightGUID;

    uint32 TalkCount;
    uint32 TalkTimer;
    uint32 CurtainTimer;
    uint32 WipeTimer;
    uint32 Event;

    bool PerformanceReady;
    bool RaidWiped;
    bool IsTalking;

    void Reset()
    {
        TalkCount = 0;
        TalkTimer = 2000;
        CurtainTimer = 5000;
        WipeTimer = 5000;

        PerformanceReady = false;
        IsTalking = false;

        if(pInstance)
        {
            pInstance->SetData(DATA_OPERA_EVENT, NOT_STARTED);

            Event = pInstance->GetData(DATA_OPERA_PERFORMANCE);

             if (GameObject* Door = GameObject::GetGameObject((*m_creature), pInstance->GetData64(DATA_GAMEOBJECT_STAGEDOORLEFT)))
             {
                if (RaidWiped)
                    Door->SetGoState(GO_STATE_ACTIVE);
                else
                    Door->SetGoState(GO_STATE_READY);
             }

             if (GameObject* Curtain = GameObject::GetGameObject((*m_creature), pInstance->GetData64(DATA_GAMEOBJECT_CURTAINS)))
                Curtain->SetGoState(GO_STATE_READY);
        }
    }

    void EnterCombat(Unit* who) {}

    void WaypointReached(uint32 i)
    {
        switch(i)
        {
            case 2:
                IsBeingEscorted = false;
                TalkCount = 0;
                IsTalking = true;

                float x,y,z;
                m_creature->GetPosition(x, y, z);
                if (Creature* Spotlight = m_creature->SummonCreature(CREATURE_SPOTLIGHT, x, y, z, 0, TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 50000))
                {
                    Spotlight->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    Spotlight->CastSpell(Spotlight, SPELL_SPOTLIGHT, false);
                    SpotlightGUID = Spotlight->GetGUID();
                }
                break;

            case 5:
                if(pInstance)
                {
                    if (GameObject* Door = GameObject::GetGameObject((*m_creature), pInstance->GetData64(DATA_GAMEOBJECT_STAGEDOORLEFT)))
                        Door->SetGoState(GO_STATE_READY);
                }
                IsBeingEscorted = false;
                PerformanceReady = true;
                break;
        }
    }

    void Talk(uint32 count)
    {
        int32 text = 0;

        switch(Event)
        {
            case EVENT_OZ:
                if (OzDialogue[count].textid)
                     text = OzDialogue[count].textid;
                if(OzDialogue[count].timer)
                    TalkTimer = OzDialogue[count].timer;
                break;

            case EVENT_HOOD:
                if (HoodDialogue[count].textid)
                    text = HoodDialogue[count].textid;
                if(HoodDialogue[count].timer)
                    TalkTimer = HoodDialogue[count].timer;
                break;

            case EVENT_RAJ:
                 if (RAJDialogue[count].textid)
                     text = RAJDialogue[count].textid;
                if(RAJDialogue[count].timer)
                    TalkTimer = RAJDialogue[count].timer;
                break;
        }

        if(text)
             DoScriptText(text, m_creature);
    }

    void UpdateAI(const uint32 diff)
    {
        npc_escortAI::UpdateAI(diff);

        if(IsTalking)
        {
            if(TalkTimer < diff)
            {
                if(TalkCount > 3)
                {
                    if (Unit* Spotlight = Unit::GetUnit((*m_creature), SpotlightGUID))
                    {
                        Spotlight->RemoveAllAuras();
                        Spotlight->SetVisibility(VISIBILITY_OFF);
                    }

                    m_creature->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_STAND);
                    IsTalking = false;
                    IsBeingEscorted = true;
                    return;
                }

                m_creature->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_TALK);
                Talk(TalkCount);
                ++TalkCount;
            }else TalkTimer -= diff;
        }

        if(PerformanceReady)
        {
            if(CurtainTimer)
            {
                if(CurtainTimer <= diff)
                {
                    PrepareEncounter();

                    if(!pInstance)
                        return;

                    if (GameObject* Curtain = GameObject::GetGameObject((*m_creature), pInstance->GetData64(DATA_GAMEOBJECT_CURTAINS)))
                        Curtain->SetGoState(GO_STATE_ACTIVE);

                    CurtainTimer = 0;
                }else CurtainTimer -= diff;
            }

            if(!RaidWiped)
            {
                if(WipeTimer < diff)
                {
                    Map *map = m_creature->GetMap();
                    if(!map->IsDungeon()) return;

                    Map::PlayerList const &PlayerList = map->GetPlayers();
                    if(PlayerList.isEmpty())
                        return;

                    RaidWiped = true;
                    for(Map::PlayerList::const_iterator i = PlayerList.begin();i != PlayerList.end(); ++i)
                    {
                        if (i->getSource()->IsAlive() && !i->getSource()->isGameMaster())
                        {
                            RaidWiped = false;
                            break;
                        }
                    }

                    if(RaidWiped)
                    {
                        RaidWiped = true;
                        EnterEvadeMode();
                        return;
                    }

                    WipeTimer = 15000;
                }else WipeTimer -= diff;
            }

        }

        if(!UpdateVictim())
            return;

        DoMeleeAttackIfReady();
    }

    void StartEvent()
    {
        if(!pInstance)
            return;

        pInstance->SetData(DATA_OPERA_EVENT, IN_PROGRESS);

        if (GameObject* Door = GameObject::GetGameObject((*m_creature), pInstance->GetData64(DATA_GAMEOBJECT_STAGEDOORLEFT)))
            Door->SetGoState(GO_STATE_ACTIVE);

        m_creature->CastSpell(m_creature, SPELL_TUXEDO, true);
        m_creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);

        Start(false, false, false);
    }

    void PrepareEncounter()
    {
        uint8 index = 0;
        uint8 count = 0;
        switch(Event)
        {
            case EVENT_OZ:
                index = 0;
                count = 4;
                break;

            case EVENT_HOOD:
                index = 4;
                count = index+1;
                break;

            case EVENT_RAJ:
                index = 5;
                count = index+1;
                break;
        }

        for( ; index < count; ++index)
        {
            uint32 entry = ((uint32)Spawns[index][0]);
            float PosX = Spawns[index][1];
            if (Creature* pCreature = m_creature->SummonCreature(entry, PosX, SPAWN_Y, SPAWN_Z, SPAWN_O, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 120000))
            {
                                                            // In case database has bad flags
                pCreature->SetUInt32Value(UNIT_FIELD_FLAGS, 0);
                pCreature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            }
        }

        CurtainTimer = 10000;
        PerformanceReady = true;
        RaidWiped = false;
    }
};

CreatureAI* GetAI_npc_barnesAI(Creature* _Creature)
{
    npc_barnesAI* Barnes_AI = new npc_barnesAI(_Creature);

    for(uint8 i = 0; i < 6; ++i)
        Barnes_AI->AddWaypoint(i, StageLocations[i][0], StageLocations[i][1], 90.465);

    return ((CreatureAI*)Barnes_AI);
}

bool GossipHello_npc_barnes(Player* player, Creature* _Creature)
{
    // Check for death of Moroes.
    ScriptedInstance* pInstance = ((ScriptedInstance*)_Creature->GetInstanceData());
    if (pInstance && (pInstance->GetData(DATA_OPERA_EVENT) < DONE))
    {
        if(pInstance->GetData(DATA_MOROES_EVENT) >= DONE)
        {
            player->ADD_GOSSIP_ITEM(0, OZ_GOSSIP1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);

            if(!((npc_barnesAI*)_Creature->AI())->RaidWiped)
                player->SEND_GOSSIP_MENU(8970, _Creature->GetGUID()); //try again text
            else
                player->SEND_GOSSIP_MENU(8975, _Creature->GetGUID());
        }else {
            player->SEND_GOSSIP_MENU(8978, _Creature->GetGUID()); //Someone should take care of Moroes
        }
    }

    return true;
}

bool GossipSelect_npc_barnes(Player *player, Creature *_Creature, uint32 sender, uint32 action)
{
    switch(action)
    {
        case GOSSIP_ACTION_INFO_DEF+1:
            player->ADD_GOSSIP_ITEM(0, OZ_GOSSIP2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);
            player->SEND_GOSSIP_MENU(8971, _Creature->GetGUID());
            break;

        case GOSSIP_ACTION_INFO_DEF+2:
            player->CLOSE_GOSSIP_MENU();
            ((npc_barnesAI*)_Creature->AI())->StartEvent();
            break;
    }

    return true;
}

/*###
# npc_hastings
####*/

#define TEXT_HELLO             554
#define TEXT_MENU1             555
#define TEXT_MENU2             556
#define GOSSIP_ITEM_1          557
#define GOSSIP_ITEM_2          19999

bool GossipHello_npc_hastings(Player* player, Creature* _Creature)
{
    player->ADD_GOSSIP_ITEM(0, GOSSIP_ITEM_1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);

    player->SEND_GOSSIP_MENU(TEXT_HELLO,_Creature->GetGUID());
    return true;
}

bool GossipSelect_npc_hastings(Player* player, Creature* _Creature, uint32 sender, uint32 action)
{
    switch (action)
    {
        case GOSSIP_ACTION_INFO_DEF+1:     
            player->ADD_GOSSIP_ITEM(0, GOSSIP_ITEM_2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);
            player->SEND_GOSSIP_MENU(TEXT_MENU1,_Creature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+2:     
            player->SEND_GOSSIP_MENU(TEXT_MENU2,_Creature->GetGUID()); 
            break;
    }

    return true;
}

/*###
# npc_berthold
####*/

#define SPELL_TELEPORT           39567

#define TEXT_HELLO                       30010
#define TEXT_MENU1                       30011
#define TEXT_MENU2                       30012
#define TEXT_MENU3                       30013

#define GOSSIP_ITEM_TELEPORT     "Teleportez moi a la Bibliotheque du Gardien"

bool GossipHello_npc_berthold(Player* player, Creature* _Creature)
{
    ScriptedInstance* pInstance = ((ScriptedInstance*)_Creature->GetInstanceData());
     
    player->ADD_GOSSIP_ITEM(0, "Quel est cet endroit ?", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
    player->ADD_GOSSIP_ITEM(0, "Ou est Medivh ?", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);
    player->ADD_GOSSIP_ITEM(0, "Comment fait-on pour se reperer dans la tour ?", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+3);
    if(pInstance && (pInstance->GetData(DATA_SHADEOFARAN_EVENT) == DONE)) // Check if Shade of Aran is dead or not
        player->ADD_GOSSIP_ITEM(0, GOSSIP_ITEM_TELEPORT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);
    
    player->SEND_GOSSIP_MENU(TEXT_HELLO,_Creature->GetGUID());
    return true;
}

bool GossipSelect_npc_berthold(Player* player, Creature* _Creature, uint32 sender, uint32 action)
{
        switch (action)
    {
        case GOSSIP_ACTION_INFO_DEF + 1:     player->SEND_GOSSIP_MENU(TEXT_MENU1,_Creature->GetGUID()); break;
        case GOSSIP_ACTION_INFO_DEF + 2:     player->SEND_GOSSIP_MENU(TEXT_MENU2,_Creature->GetGUID()); break;
        case GOSSIP_ACTION_INFO_DEF + 3:     player->SEND_GOSSIP_MENU(TEXT_MENU3,_Creature->GetGUID()); break;
        case GOSSIP_ACTION_INFO_DEF + 4:     player->CastSpell(player, SPELL_TELEPORT, true);player->CLOSE_GOSSIP_MENU(); break;
    }

    return true;
}

/*###
# npc_image_of_medivh
####*/

#define SAY_DIALOG_MEDIVH_1         "Vous avez attiré mon attention, dragon. Mais on ne m'effraie pas aussi facilement que les villageois d'en-dessous."
#define SAY_DIALOG_ARCANAGOS_2      "Votre jeu avec la force des arcanes est allé trop loin, Medivh. Vous avez attiré l'attention de puissances au-delà de votre compréhension. Vous devez quitter Karazhan sur le champ !!"
#define SAY_DIALOG_MEDIVH_3         "Vous osez me défier dans ma propre demeure ? Votre arrogance est stupéfiante, même pour un dragon !"
#define SAY_DIALOG_ARCANAGOS_4      "Une puissance obscure cherche à vous utiliser, Medivh ! Si vous restez, de sombres jours vont suivre. Il faut vous dépêcher, nous n'avons pas beaucoup de temps !"
#define SAY_DIALOG_MEDIVH_5         "Je ne vois pas de quoi vous voulez parler, dragon... mais je ne vais pas me laisser intimider par tant d'insolence. Je quitterai Karazhan quand je l'aurai décidé !"
#define SAY_DIALOG_ARCANAGOS_6      "Vous ne me laissez pas le choix. Je vous arrêterai par la force si vous ne voulez pas revenir à la raison."
#define EMOTE_DIALOG_MEDIVH_7       "commence à invoquer un sort d'une grande puissance, tissant sa propre essence avec la magie."
#define SAY_DIALOG_ARCANAGOS_8      "Qu'avez-vous fait, magicien ? Cela ne peut être... ! Je brûle de... à l'intérieur !"
#define SAY_DIALOG_MEDIVH_9         "Il n'aurait pas dû me mettre en colère. Je dois désormais aller... récupérer ma puissance..."

#define MOB_ARCANAGOS               17652
#define SPELL_FIRE_BALL             30967
#define SPELL_UBER_FIREBALL         30971
#define SPELL_CONFLAGRATION_BLAST   30977
#define SPELL_MANA_SHIELD           31635

static float MedivPos[4] = {-11161.49,-1902.24,91.48,1.94};
static float ArcanagosPos[4] = {-11169.75,-1881.48,95.39,4.83};

struct npc_image_of_medivhAI : public ScriptedAI
{
    npc_image_of_medivhAI(Creature* c) : ScriptedAI(c)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
    }

    ScriptedInstance *pInstance;

    uint64 ArcanagosGUID;

    uint32 YellTimer;
    uint32 Step;
    uint32 FireMedivhTimer;
    uint32 FireArcanagosTimer;

    bool EventStarted;

    void Reset()
    {
        ArcanagosGUID = 0;

        if(pInstance && pInstance->GetData64(DATA_IMAGE_OF_MEDIVH_NIGHTBANE) == 0)
        {
            pInstance->SetData64(DATA_IMAGE_OF_MEDIVH_NIGHTBANE, m_creature->GetGUID());
            (*m_creature).GetMotionMaster()->MovePoint(1,MedivPos[0],MedivPos[1],MedivPos[2]);
            Step = 0;
        }else
        {
            m_creature->DealDamage(m_creature,m_creature->GetHealth(),NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
            m_creature->RemoveCorpse();
        }
    }
    void EnterCombat(Unit* who){}

    void MovementInform(uint32 type, uint32 id)
    {
        if(type != POINT_MOTION_TYPE)
            return;
        if(id == 1)
        {
            StartEvent();
            m_creature->SetOrientation(MedivPos[3]);
            m_creature->SetOrientation(MedivPos[3]);
        }
    }

    void StartEvent()
    {
        Step = 1;
        EventStarted = true;
        Creature* Arcanagos = m_creature->SummonCreature(MOB_ARCANAGOS,ArcanagosPos[0],ArcanagosPos[1],ArcanagosPos[2],0,TEMPSUMMON_CORPSE_TIMED_DESPAWN,20000);
        if(!Arcanagos)
            return;
        ArcanagosGUID = Arcanagos->GetGUID();
        Arcanagos->SetDisableGravity(true);
        (*Arcanagos).GetMotionMaster()->MovePoint(0,ArcanagosPos[0],ArcanagosPos[1],ArcanagosPos[2]);
        Arcanagos->SetOrientation(ArcanagosPos[3]);
        m_creature->SetOrientation(MedivPos[3]);
        YellTimer = 10000;
    }


    uint32 NextStep(uint32 Step)
    {
        Unit* arca = Unit::GetUnit((*m_creature),ArcanagosGUID);
        Map *map = m_creature->GetMap();
        switch(Step)
        {
        case 0: return 9999999;
        case 1:
            m_creature->Yell(SAY_DIALOG_MEDIVH_1,LANG_UNIVERSAL,NULL);
            return 10000;
        case 2:
            if(arca)
                (arca->ToCreature())->Yell(SAY_DIALOG_ARCANAGOS_2,LANG_UNIVERSAL,NULL);
            return 20000;
        case 3:
            m_creature->Yell(SAY_DIALOG_MEDIVH_3,LANG_UNIVERSAL,NULL);
            return 10000;
        case 4:
            if(arca)
                (arca->ToCreature())->Yell(SAY_DIALOG_ARCANAGOS_4, LANG_UNIVERSAL, NULL);
            return 20000;
        case 5:
            m_creature->Yell(SAY_DIALOG_MEDIVH_5, LANG_UNIVERSAL, NULL);
            return 20000;
        case 6:
            if(arca)
                (arca->ToCreature())->Yell(SAY_DIALOG_ARCANAGOS_6, LANG_UNIVERSAL, NULL);
            return 10000;
        case 7:
            FireArcanagosTimer = 500;
            return 5000;
        case 8:
            FireMedivhTimer = 500;
            DoCast(m_creature, SPELL_MANA_SHIELD);
            return 10000;
        case 9:
            m_creature->TextEmote(EMOTE_DIALOG_MEDIVH_7, 0, false);
            return 10000;
        case 10:
            if(arca)
                m_creature->CastSpell(arca, SPELL_CONFLAGRATION_BLAST, false);
            return 1000;
        case 11:
            if(arca)
                (arca->ToCreature())->Yell(SAY_DIALOG_ARCANAGOS_8, LANG_UNIVERSAL, NULL);
            return 5000;
        case 12:
            arca->GetMotionMaster()->MovePoint(0, -11010.82,-1761.18, 156.47);
            arca->setActive(true);
            arca->InterruptNonMeleeSpells(true);
            arca->SetSpeed(MOVE_FLIGHT, 2.0f);
            return 10000;
        case 13:
            m_creature->Yell(SAY_DIALOG_MEDIVH_9, LANG_UNIVERSAL, NULL);
            return 10000;
        case 14:
            m_creature->SetVisibility(VISIBILITY_OFF);
            m_creature->ClearInCombat();

            if(map->IsDungeon())
            {
                InstanceMap::PlayerList const &PlayerList = ((InstanceMap*)map)->GetPlayers();
                for (InstanceMap::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                {
                    if(i->getSource()->IsAlive())
                    {
                        if(i->getSource()->GetQuestStatus(9645) == QUEST_STATUS_INCOMPLETE)
                            i->getSource()->CompleteQuest(9645);
                    }
                }
            }
            return 50000;
        case 15:
            arca->DealDamage(arca,arca->GetHealth(),NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
            return 5000;
        default : return 9999999;
        }

    }

    void UpdateAI(const uint32 diff)
    {

        if(YellTimer < diff)
        {
            if(EventStarted)
            {
                YellTimer = NextStep(Step++);
            }
        }else YellTimer -= diff;

        if(Step >= 7 && Step <= 12 )
        {
            Unit* arca = Unit::GetUnit((*m_creature),ArcanagosGUID);

            if(FireArcanagosTimer < diff)
            {
                if(arca)
                    arca->CastSpell(m_creature, SPELL_FIRE_BALL, false);
                FireArcanagosTimer = 6000;
            }else FireArcanagosTimer -= diff;

            if(FireMedivhTimer < diff)
            {
                if(arca)
                    DoCast(arca, SPELL_FIRE_BALL);
                FireMedivhTimer = 5000;
            }else FireMedivhTimer -= diff;

        }
    }
};

CreatureAI* GetAI_npc_image_of_medivh(Creature *_Creature)
{
    return new npc_image_of_medivhAI(_Creature);
}

/*######
## npc_archmage_leryda
######*/

bool GossipHello_npc_archmage_leryda(Player* pPlayer, Creature* pCreature)
{
    if (pCreature->isQuestGiver())
        pPlayer->PrepareQuestMenu(pCreature->GetGUID());
        
    if (!pPlayer->HasItemCount(29290, 1, true) && !pPlayer->HasItemCount(29279, 1, true) && !pPlayer->HasItemCount(29283, 1, true) &&
         !pPlayer->HasItemCount(29287, 1, true) && pPlayer->GetReputationRank(967) == REP_EXALTED) {
        if (pPlayer->GetQuestStatus(11031) == QUEST_STATUS_COMPLETE || pPlayer->GetQuestStatus(11032) == QUEST_STATUS_COMPLETE
                || pPlayer->GetQuestStatus(11033) == QUEST_STATUS_COMPLETE || pPlayer->GetQuestStatus(11034) == QUEST_STATUS_COMPLETE) {
            pPlayer->ADD_GOSSIP_ITEM(0, "Je voudrais une chevalière de grand guérisseur.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
            pPlayer->ADD_GOSSIP_ITEM(0, "Je voudrais une chevalière de maître assassin.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);
            pPlayer->ADD_GOSSIP_ITEM(0, "Je voudrais une chevalière de l'archimage.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+3);
            pPlayer->ADD_GOSSIP_ITEM(0, "Je voudrais une chevalière du grand protecteur.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+4);
        }
    }
    
    pPlayer->SEND_GOSSIP_MENU(pCreature->GetNpcTextId(), pCreature->GetGUID());
    
    return true;
}

bool GossipSelect_npc_archmage_leryda(Player* pPlayer, Creature* pCreature, uint32 sender, uint32 action)
{
    uint32 itemId = 0;
    switch (action) {
    case GOSSIP_ACTION_INFO_DEF+1:
        itemId = 29290;
        break;
    case GOSSIP_ACTION_INFO_DEF+2:
        itemId = 29283;
        break;
    case GOSSIP_ACTION_INFO_DEF+3:
        itemId = 29287;
        break;
    case GOSSIP_ACTION_INFO_DEF+4:
        itemId = 29279;
        break;
    }
    
    ItemPosCountVec dest;
    uint8 msg = pPlayer->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemId, 1);
    if (msg == EQUIP_ERR_OK)
    {
        Item* item = pPlayer->StoreNewItem( dest, itemId, true);
        pPlayer->SendNewItem(item, 1, true, false);
    }
    
    pPlayer->CLOSE_GOSSIP_MENU();
    
    return true;
}

/*######
## go_sealed_tome
######*/

bool GOHello_go_sealed_tome(Player* player, GameObject* go)
{
    if (rand()%3 == 1)
        player->CastSpell(player, RAND(30762, 30763, 30764, 30765, 30766), true);
    
    return false;
}

/*######
## woefulhealer
######*/

#define TIMER_GLOBALWAIT             2000 + rand() % 3000
#define MAXMANA                      10000
#define SPELL_FLASHHEAL              25235
#define VALUE_FLASHHEAL              1200   //estimation du heal
#define SPELL_PRAYEROFHEALING        25308
#define VALUE_PRAYEROFHEALING        1300

 
struct woefulhealerAI : public ScriptedAI
{
    woefulhealerAI(Creature *c) : ScriptedAI(c) {}
    
    bool flagsset;
    Unit* owner;
    Unit* healtarget;
    uint16 waittimer;
    uint8 tohealingroup;
    uint32 mostlowhp;
    
    void Reset()
    {
        owner = m_creature->GetCharmerOrOwner();
        m_creature->GetMotionMaster()->MoveFollow(owner, 0.8,210);
        m_creature->SetReactState(REACT_PASSIVE);
        waittimer = TIMER_GLOBALWAIT;
        flagsset = false;
    }
    
    void EnterCombat(Unit* who) {}
  
    void UpdateAI(const uint32 diff)
    {                
        if (!flagsset) 
        {        
            m_creature->SetMaxPower(POWER_MANA, MAXMANA);
            m_creature->SetPower(POWER_MANA, MAXMANA);
            m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            flagsset = true;
        } //Fonctionnent pas dans le reset, une autre maniere de faire plus propre?
            
        if ( !m_creature->IsAlive() || m_creature->IsNonMeleeSpellCasted(false))
            return;
    
        if (waittimer > diff)
        {
            waittimer -= diff;
            return;    
        }

        healtarget = NULL;
        tohealingroup = 0;
        mostlowhp = -1;
        
        //Selection de la/les cibles du heal
        Map *map = m_creature->GetMap();
        Map::PlayerList const& PlayerList = map->GetPlayers();
        
        for(Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
        {
            if (Player* i_pl = i->getSource())
            {
                if (i_pl->IsAlive() && i_pl->GetDistance(m_creature) < 40 
                && (i_pl->GetMaxHealth() - i_pl->GetHealth() > VALUE_PRAYEROFHEALING))
                {
                    if (mostlowhp > i_pl->GetHealth())
                    {
                        healtarget = i_pl;
                        mostlowhp = i_pl->GetHealth();
                    }
                    if (((Player*) i_pl)->IsInSameGroupWith((Player*) owner))
                        tohealingroup++;
                }
            }
        }
            
        if (!healtarget)
            return;
            
        // Cast
        if (tohealingroup >= 3)
            DoCast(me, SPELL_PRAYEROFHEALING);
        else
            DoCast(healtarget, SPELL_FLASHHEAL);        
            
        waittimer = TIMER_GLOBALWAIT;
    }
};
 
CreatureAI* GetAI_woefulhealer(Creature *_Creature)
{
    return new woefulhealerAI (_Creature);
}

#define SPELL_MANA_SHIELD 29880
#define SPELL_BLINK 29883
#define SPELL_ARCANE_VOLLEY 29885
#define SPELL_LOOSE_MANA 29882

struct npc_arcane_anomalyAI : public ScriptedAI
{
    npc_arcane_anomalyAI(Creature *c) : ScriptedAI(c) {}
    
    bool castedShield;
    uint32 blinkTimer;
    uint32 volleyTimer;

    void Reset()
    {
        blinkTimer = 5000 + rand()%10000;
        volleyTimer = 10000 + rand()%5000;
        castedShield = false;
    }

    void EnterCombat(Unit* who) 
    {
        DoCast(me,SPELL_MANA_SHIELD,true);
        castedShield = true;
    }

    //cannot die if we havent casted our mana shield
    void DamageTaken(Unit* pKiller, uint32 &damage)
    {
        if(!castedShield && damage >= me->GetHealth())
            damage = me->GetHealth()-1; //down to 1 hp
    }
  
    void UpdateAI(const uint32 diff)
    {                
        if(!UpdateVictim())
            return;

        if(blinkTimer < diff)
        {
            if(DoCast(me,SPELL_BLINK) == SPELL_CAST_OK)
                blinkTimer = 10000 + rand()%5000;
        } else blinkTimer -= diff;

        if(volleyTimer < diff)
        {
            if(DoCast(me,SPELL_ARCANE_VOLLEY) == SPELL_CAST_OK)
                volleyTimer = 20000 + rand()%5000;
        } else volleyTimer -= diff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_arcane_anomaly(Creature *_Creature)
{
    return new npc_arcane_anomalyAI (_Creature);
}

void AddSC_karazhan()
{
    Script* newscript;

    newscript = new Script;
    newscript->GetAI = &GetAI_npc_barnesAI;
    newscript->Name = "npc_barnes";
    newscript->pGossipHello = &GossipHello_npc_barnes;
    newscript->pGossipSelect = &GossipSelect_npc_barnes;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_berthold";
    newscript->pGossipHello = &GossipHello_npc_berthold;
    newscript->pGossipSelect = &GossipSelect_npc_berthold;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_image_of_medivh";
    newscript->GetAI = &GetAI_npc_image_of_medivh;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_archmage_leryda";
    newscript->pGossipHello = &GossipHello_npc_archmage_leryda;
    newscript->pGossipSelect = &GossipSelect_npc_archmage_leryda;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "go_sealed_tome";
    newscript->pGOHello = &GOHello_go_sealed_tome;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name="woefulhealer";
    newscript->GetAI = &GetAI_woefulhealer;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name="npc_arcane_anomaly";
    newscript->GetAI = &GetAI_npc_arcane_anomaly;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_hastings";
    newscript->pGossipHello = &GossipHello_npc_hastings;
    newscript->pGossipSelect = &GossipSelect_npc_hastings;
    newscript->RegisterSelf();
}

