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
SDName: Netherstorm
SD%Complete: 80
SDComment: Quest support: 10337, 10438, 10652 (special flight paths), 10299,10321,10322,10323,10329,10330,10338,10365(Shutting Down Manaforge), 10198, 10191
SDCategory: Netherstorm
EndScriptData */

/* ContentData
npc_manaforge_control_console
go_manaforge_control_console
npc_commander_dawnforge
npc_protectorate_nether_drake
npc_veronia
npc_bessy
npc_maxx_a_million
npc_dr_boom
npc_boom_bot
npc_drijya
EndContentData */

#include "precompiled.h"
#include "EscortAI.h"

/*######
## npc_manaforge_control_console
######*/

#define EMOTE_START     -1000296
#define EMOTE_60        -1000297
#define EMOTE_30        -1000298
#define EMOTE_10        -1000299
#define EMOTE_COMPLETE  -1000300
#define EMOTE_ABORT     -1000301

#define ENTRY_BNAAR_C_CONSOLE   20209
#define ENTRY_CORUU_C_CONSOLE   20417
#define ENTRY_DURO_C_CONSOLE    20418
#define ENTRY_ARA_C_CONSOLE     20440

#define ENTRY_SUNFURY_TECH      20218
#define ENTRY_SUNFURY_PROT      20436

#define ENTRY_ARA_TECH          20438
#define ENTRY_ARA_ENGI          20439
#define ENTRY_ARA_GORKLONN      20460

#define SPELL_DISABLE_VISUAL    35031
#define SPELL_INTERRUPT_1       35016                       //ACID mobs should cast this
#define SPELL_INTERRUPT_2       35176                       //ACID mobs should cast this (Manaforge Ara-version)

struct npc_manaforge_control_consoleAI : public ScriptedAI
{
    npc_manaforge_control_consoleAI(Creature *c) : ScriptedAI(c) {}

    uint32 Event_Timer;
    uint32 Wave_Timer;
    uint32 Phase;
    bool Wave;
    uint64 someplayer;
    uint64 goConsole;
    Creature* add;

    void Reset()
    {
        Event_Timer = 3000;
        Wave_Timer = 0;
        Phase = 1;
        Wave = false;
        someplayer = 0;
        goConsole = 0;
        Creature* add = NULL;
    }

    void EnterCombat(Unit *who) { return; }

    /*void SpellHit(Unit *caster, const SpellEntry *spell)
    {
        //we have no way of telling the creature was hit by spell -> got aura applied after 10-12 seconds
        //then no way for the mobs to actually stop the shutdown as intended.
        if( spell->Id == SPELL_INTERRUPT_1 )
            DoSay("Silence! I kill you!",LANG_UNIVERSAL, NULL);
    }*/

    void JustDied(Unit* killer)
    {
        DoScriptText(EMOTE_ABORT, m_creature);

        if( someplayer )
        {
            Player* p = Unit::GetPlayer(someplayer);
            if(p)
            {
                switch( m_creature->GetEntry() )
                {
                    case ENTRY_BNAAR_C_CONSOLE:
                        p->FailQuest(10299);
                        p->FailQuest(10329);
                        break;
                    case ENTRY_CORUU_C_CONSOLE:
                        p->FailQuest(10321);
                        p->FailQuest(10330);
                        break;
                    case ENTRY_DURO_C_CONSOLE:
                        p->FailQuest(10322);
                        p->FailQuest(10338);
                        break;
                    case ENTRY_ARA_C_CONSOLE:
                        p->FailQuest(10323);
                        p->FailQuest(10365);
                        break;
                }
            }
        }

        if( goConsole )
        {
            if( GameObject* go = GameObject::GetGameObject((*m_creature),goConsole) )
                go->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_IN_USE);
        }
    }

    void DoWaveSpawnForCreature(Creature *creature)
    {
        switch( creature->GetEntry() )
        {
            case ENTRY_BNAAR_C_CONSOLE:
                if( rand()%2 )
                {
                    add = m_creature->SummonCreature(ENTRY_SUNFURY_TECH,2933.68,4162.55,164.00,1.60,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 120000);
                    if( add ) add->GetMotionMaster()->MovePoint(0,2927.36,4212.97,164.00);
                }
                else
                {
                    add = m_creature->SummonCreature(ENTRY_SUNFURY_TECH,2927.36,4212.97,164.00,4.94,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 120000);
                    if( add ) add->GetMotionMaster()->MovePoint(0,2933.68,4162.55,164.00);
                }
                Wave_Timer = 30000;
                break;
            case ENTRY_CORUU_C_CONSOLE:
                add = m_creature->SummonCreature(ENTRY_SUNFURY_TECH,2445.21,2765.26,134.49,3.93,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 120000);
                if( add ) add->GetMotionMaster()->MovePoint(0,2424.21,2740.15,133.81);
                add = m_creature->SummonCreature(ENTRY_SUNFURY_TECH,2429.86,2731.85,134.53,1.31,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 120000);
                if( add ) add->GetMotionMaster()->MovePoint(0,2435.37,2766.04,133.81);
                Wave_Timer = 20000;
                break;
            case ENTRY_DURO_C_CONSOLE:
                add = m_creature->SummonCreature(ENTRY_SUNFURY_TECH,2986.80,2205.36,165.37,3.74,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 120000);
                if( add ) add->GetMotionMaster()->MovePoint(0,2985.15,2197.32,164.79);
                add = m_creature->SummonCreature(ENTRY_SUNFURY_TECH,2952.91,2191.20,165.32,0.22,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 120000);
                if( add ) add->GetMotionMaster()->MovePoint(0,2060.01,2185.27,164.67);
                Wave_Timer = 15000;
                break;
            case ENTRY_ARA_C_CONSOLE:
                if( rand()%2 )
                {
                    add = m_creature->SummonCreature(ENTRY_ARA_TECH,4035.11,4038.97,194.27,2.57,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 120000);
                    if( add ) add->GetMotionMaster()->MovePoint(0,4003.42,4040.19,193.49);
                    add = m_creature->SummonCreature(ENTRY_ARA_TECH,4033.66,4036.79,194.28,2.57,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 120000);
                    if( add ) add->GetMotionMaster()->MovePoint(0,4003.42,4040.19,193.49);
                    add = m_creature->SummonCreature(ENTRY_ARA_TECH,4037.13,4037.30,194.23,2.57,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 120000);
                    if( add ) add->GetMotionMaster()->MovePoint(0,4003.42,4040.19,193.49);
                }
                else
                {
                    add = m_creature->SummonCreature(ENTRY_ARA_TECH,3099.59,4049.30,194.22,0.05,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 120000);
                    if( add ) add->GetMotionMaster()->MovePoint(0,4028.01,4035.17,193.59);
                    add = m_creature->SummonCreature(ENTRY_ARA_TECH,3999.72,4046.75,194.22,0.05,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 120000);
                    if( add ) add->GetMotionMaster()->MovePoint(0,4028.01,4035.17,193.59);
                    add = m_creature->SummonCreature(ENTRY_ARA_TECH,3996.81,4048.26,194.22,0.05,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 120000);
                    if( add ) add->GetMotionMaster()->MovePoint(0,4028.01,4035.17,193.59);
                }
                Wave_Timer = 15000;
                break;
        }
    }
    void DoFinalSpawnForCreature(Creature *creature)
    {
        switch( creature->GetEntry() )
        {
            case ENTRY_BNAAR_C_CONSOLE:
                add = m_creature->SummonCreature(ENTRY_SUNFURY_TECH,2946.52,4201.42,163.47,3.54,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 120000);
                if( add ) add->GetMotionMaster()->MovePoint(0,2927.49,4192.81,163.00);
                break;
            case ENTRY_CORUU_C_CONSOLE:
                add = m_creature->SummonCreature(ENTRY_SUNFURY_TECH,2453.88,2737.85,133.27,2.59,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 120000);
                if( add ) add->GetMotionMaster()->MovePoint(0,2433.96,2751.53,133.85);
                add = m_creature->SummonCreature(ENTRY_SUNFURY_TECH,2441.62,2735.32,134.49,1.97,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 120000);
                if( add ) add->GetMotionMaster()->MovePoint(0,2433.96,2751.53,133.85);
                add = m_creature->SummonCreature(ENTRY_SUNFURY_TECH,2450.73,2754.50,134.49,3.29,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 120000);
                if( add ) add->GetMotionMaster()->MovePoint(0,2433.96,2751.53,133.85);
                break;
            case ENTRY_DURO_C_CONSOLE:
                add = m_creature->SummonCreature(ENTRY_SUNFURY_TECH,2956.18,2202.85,165.32,5.45,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 120000);
                if( add ) add->GetMotionMaster()->MovePoint(0,2972.27,2193.22,164.48);
                add = m_creature->SummonCreature(ENTRY_SUNFURY_TECH,2975.30,2211.50,165.32,4.55,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 120000);
                if( add ) add->GetMotionMaster()->MovePoint(0,2972.27,2193.22,164.48);
                add = m_creature->SummonCreature(ENTRY_SUNFURY_PROT,2965.02,2217.45,164.16,4.96,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 120000);
                if( add ) add->GetMotionMaster()->MovePoint(0,2972.27,2193.22,164.48);
                break;
            case ENTRY_ARA_C_CONSOLE:
                add = m_creature->SummonCreature(ENTRY_ARA_ENGI,3994.51,4020.46,192.18,0.91,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 120000);
                if( add ) add->GetMotionMaster()->MovePoint(0,4008.35,4035.04,192.70);
                add = m_creature->SummonCreature(ENTRY_ARA_GORKLONN,4021.56,4059.35,193.59,4.44,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 120000);
                if( add ) add->GetMotionMaster()->MovePoint(0,4016.62,4039.89,193.46);
                break;
        }
    }

    void UpdateAI(const uint32 diff)
    {
        if( Event_Timer < diff )
        {
            switch(Phase)
            {
                case 1:
                    if( someplayer )
                    {
                        Unit* u = Unit::GetUnit((*m_creature), someplayer);
                        if( u && u->GetTypeId() == TYPEID_PLAYER ) DoScriptText(EMOTE_START, m_creature, u);
                    }
                    Event_Timer = 60000;
                    Wave = true;
                    ++Phase;
                    break;
                case 2:
                    DoScriptText(EMOTE_60, m_creature);
                    Event_Timer = 30000;
                    ++Phase;
                    break;
                case 3:
                    DoScriptText(EMOTE_30, m_creature);
                    Event_Timer = 20000;
                    DoFinalSpawnForCreature(m_creature);
                    ++Phase;
                    break;
                case 4:
                    DoScriptText(EMOTE_10, m_creature);
                    Event_Timer = 10000;
                    Wave = false;
                    ++Phase;
                    break;
                case 5:
                    DoScriptText(EMOTE_COMPLETE, m_creature);
                    if( someplayer )
                    {
                        Player* player = Unit::GetPlayer(someplayer);
                        if(player)
                           player->KilledMonster(m_creature->GetEntry(),m_creature->GetGUID());
                        DoCast(m_creature,SPELL_DISABLE_VISUAL);
                    }
                    if( goConsole )
                    {
                        if( GameObject* go = GameObject::GetGameObject((*m_creature),goConsole) )
                            go->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_IN_USE);
                    }
                    ++Phase;
                    break;
            }
        } else Event_Timer -= diff;

        if( Wave )
        {
            if( Wave_Timer < diff )
            {
                DoWaveSpawnForCreature(m_creature);
            } else Wave_Timer -= diff;
        }
    }
};
CreatureAI* GetAI_npc_manaforge_control_console(Creature *_Creature)
{
    return new npc_manaforge_control_consoleAI (_Creature);
}

/*######
## go_manaforge_control_console
######*/

//TODO: clean up this workaround when Trinity adds support to do it properly (with gossip selections instead of instant summon)
bool GOHello_go_manaforge_control_console(Player *player, GameObject* _GO)
{
    if (_GO->GetGoType() == GAMEOBJECT_TYPE_QUESTGIVER)
    {
        player->PrepareQuestMenu(_GO->GetGUID());
        player->SendPreparedQuest(_GO->GetGUID());
    }

    Creature* manaforge;
    manaforge = NULL;

    switch( _GO->GetAreaId() )
    {
        case 3726:                                          //b'naar
            if( (player->GetQuestStatus(10299) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(10329) == QUEST_STATUS_INCOMPLETE) &&
                player->HasItemCount(29366,1))
                manaforge = player->SummonCreature(ENTRY_BNAAR_C_CONSOLE,2918.95,4189.98,161.88,0.34,TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN,125000);
            break;
        case 3730:                                          //coruu
            if( (player->GetQuestStatus(10321) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(10330) == QUEST_STATUS_INCOMPLETE) &&
                player->HasItemCount(29396,1))
                manaforge = player->SummonCreature(ENTRY_CORUU_C_CONSOLE,2426.77,2750.38,133.24,2.14,TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN,125000);
            break;
        case 3734:                                          //duro
            if( (player->GetQuestStatus(10322) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(10338) == QUEST_STATUS_INCOMPLETE) &&
                player->HasItemCount(29397,1))
                manaforge = player->SummonCreature(ENTRY_DURO_C_CONSOLE,2976.48,2183.29,163.20,1.85,TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN,125000);
            break;
        case 3722:                                          //ara
            if( (player->GetQuestStatus(10323) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(10365) == QUEST_STATUS_INCOMPLETE) &&
                player->HasItemCount(29411,1))
                manaforge = player->SummonCreature(ENTRY_ARA_C_CONSOLE,4013.71,4028.76,192.10,1.25,TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN,125000);
            break;
    }

    if( manaforge )
    {
        ((npc_manaforge_control_consoleAI*)manaforge->AI())->someplayer = player->GetGUID();
        ((npc_manaforge_control_consoleAI*)manaforge->AI())->goConsole = _GO->GetGUID();
        _GO->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_IN_USE);
    }
    return true;
}

/*######
## npc_commander_dawnforge
######*/

// The Speech of Dawnforge, Ardonis & Pathaleon
#define SAY_COMMANDER_DAWNFORGE_1           -1000128
#define SAY_ARCANIST_ARDONIS_1              -1000129
#define SAY_COMMANDER_DAWNFORGE_2           -1000130
#define SAY_PATHALEON_CULATOR_IMAGE_1       -1000131
#define SAY_COMMANDER_DAWNFORGE_3           -1000132
#define SAY_PATHALEON_CULATOR_IMAGE_2       -1000133
#define SAY_PATHALEON_CULATOR_IMAGE_2_1     -1000134
#define SAY_PATHALEON_CULATOR_IMAGE_2_2     -1000135
#define SAY_COMMANDER_DAWNFORGE_4           -1000136
#define SAY_ARCANIST_ARDONIS_2              -1000136
#define SAY_COMMANDER_DAWNFORGE_5           -1000137

#define QUEST_INFO_GATHERING                10198
#define SPELL_SUNFURY_DISGUISE              34603

// Entries of Arcanist Ardonis, Commander Dawnforge, Pathaleon the Curators Image
int CreatureEntry[3][1] =
{
    {19830},                                                // Ardonis
    {19831},                                                // Dawnforge
    {21504}                                                 // Pathaleon
};

struct npc_commander_dawnforgeAI : public ScriptedAI
{
    npc_commander_dawnforgeAI(Creature *c) : ScriptedAI(c) { Reset (); }


    uint64 playerGUID;
    uint64 ardonisGUID;
    uint64 pathaleonGUID;


    uint32 Phase;
    uint32 PhaseSubphase;
    uint32 Phase_Timer;
    bool isEvent;

    float angle_dawnforge;
    float angle_ardonis;

    void Reset()
    {
        playerGUID = 0;
        ardonisGUID = 0;
        pathaleonGUID = 0;

        Phase = 1;
        PhaseSubphase = 0;
        Phase_Timer = 4000;
        isEvent = false;
    }

    void EnterCombat(Unit *who) { }

    //Select any creature in a grid
    Creature* SelectCreatureInGrid(uint32 entry, float range)
    {
        Creature* pCreature = NULL;

        CellPair pair(Trinity::ComputeCellPair(m_creature->GetPositionX(), m_creature->GetPositionY()));
        Cell cell(pair);
        cell.data.Part.reserved = ALL_DISTRICT;
        cell.SetNoCreate();

        Trinity::NearestCreatureEntryWithLiveStateInObjectRangeCheck creature_check(*m_creature, entry, true, range);
        Trinity::CreatureLastSearcher<Trinity::NearestCreatureEntryWithLiveStateInObjectRangeCheck> searcher(pCreature, creature_check);

        TypeContainerVisitor<Trinity::CreatureLastSearcher<Trinity::NearestCreatureEntryWithLiveStateInObjectRangeCheck>, GridTypeMapContainer> creature_searcher(searcher);

        cell.Visit(pair, creature_searcher, *(m_creature->GetMap()));

        return pCreature;
    }

    void JustSummoned(Creature *summoned)
    {
        pathaleonGUID = summoned->GetGUID();
    }

    // Emote Ardonis and Pathaleon
    void Turn_to_Pathaleons_Image()
    {
        Creature *ardonis = Unit::GetCreature(*m_creature,ardonisGUID);
        Creature *pathaleon = Unit::GetCreature(*m_creature,pathaleonGUID);
        Player *player = Unit::GetPlayer(playerGUID);

        if (!ardonis || !pathaleon || !player)
            return;

        //Calculate the angle to Pathaleon
        angle_dawnforge = m_creature->GetAngle(pathaleon->GetPositionX(), pathaleon->GetPositionY());
        angle_ardonis = ardonis->GetAngle(pathaleon->GetPositionX(), pathaleon->GetPositionY());

        //Turn Dawnforge and update
        m_creature->SetOrientation(angle_dawnforge);
        m_creature->SendUpdateToPlayer(player);
        //Turn Ardonis and update
        ardonis->SetOrientation(angle_ardonis);
        ardonis->SendUpdateToPlayer(player);

        //Set them to kneel
        m_creature->SetStandState(PLAYER_STATE_KNEEL);
        ardonis->SetStandState(PLAYER_STATE_KNEEL);
    }

    //Set them back to each other
    void Turn_to_eachother()
    {
        if (Unit *ardonis = Unit::GetUnit(*m_creature,ardonisGUID))
        {
            Player *player = Unit::GetPlayer(playerGUID);

            if (!player)
                return;

            angle_dawnforge = m_creature->GetAngle(ardonis->GetPositionX(), ardonis->GetPositionY());
            angle_ardonis = ardonis->GetAngle(m_creature->GetPositionX(), m_creature->GetPositionY());

            //Turn Dawnforge and update
            m_creature->SetOrientation(angle_dawnforge);
            m_creature->SendUpdateToPlayer(player);
            //Turn Ardonis and update
            ardonis->SetOrientation(angle_ardonis);
            ardonis->SendUpdateToPlayer(player);

            //Set state
            m_creature->SetStandState(PLAYER_STATE_NONE);
            ardonis->SetStandState(PLAYER_STATE_NONE);
        }
    }

    bool CanStartEvent(Player *player)
    {
        if (!isEvent)
        {
            Creature *ardonis = SelectCreatureInGrid(CreatureEntry[0][0], 10.0f);
            if (!ardonis)
                return false;

            ardonisGUID = ardonis->GetGUID();
            playerGUID = player->GetGUID();

            isEvent = true;

            Turn_to_eachother();
            return true;
        }

        return false;
    }

    void UpdateAI(const uint32 diff)
    {
        //Is event even running?
        if (!isEvent)
            return;

        //Phase timing
        if (Phase_Timer >= diff)
        {
            Phase_Timer -= diff;
            return;
        }

        Unit *ardonis = Unit::GetUnit(*m_creature,ardonisGUID);
        Unit *pathaleon = Unit::GetUnit(*m_creature,pathaleonGUID);
        Player *player = Unit::GetPlayer(playerGUID);

        if (!ardonis || !player)
        {
            Reset();
            return;
        }

        if (Phase > 4 && !pathaleon)
        {
            Reset();
            return;
        }

        //Phase 1 Dawnforge say
        switch (Phase)
        {
        case 1:
            DoScriptText(SAY_COMMANDER_DAWNFORGE_1, m_creature);
            ++Phase;
            Phase_Timer = 16000;
            break;
            //Phase 2 Ardonis say
        case 2:
            DoScriptText(SAY_ARCANIST_ARDONIS_1, ardonis);
            ++Phase;
            Phase_Timer = 16000;
            break;
            //Phase 3 Dawnforge say
        case 3:
            DoScriptText(SAY_COMMANDER_DAWNFORGE_2, m_creature);
            ++Phase;
            Phase_Timer = 16000;
            break;
            //Phase 4 Pathaleon spawns up to phase 9
        case 4:
            //spawn pathaleon's image
            m_creature->SummonCreature(CreatureEntry[2][0], 2325.851563, 2799.534668, 133.084229, 6.038996, TEMPSUMMON_TIMED_DESPAWN, 90000);
            ++Phase;
            Phase_Timer = 500;
            break;
            //Phase 5 Pathaleon say
        case 5:
            DoScriptText(SAY_PATHALEON_CULATOR_IMAGE_1, pathaleon);
            ++Phase;
            Phase_Timer = 6000;
            break;
            //Phase 6
        case 6:
            switch(PhaseSubphase)
            {
                //Subphase 1: Turn Dawnforge and Ardonis
            case 0:
                Turn_to_Pathaleons_Image();
                ++PhaseSubphase;
                Phase_Timer = 8000;
                break;
                //Subphase 2 Dawnforge say
            case 1:
                DoScriptText(SAY_COMMANDER_DAWNFORGE_3, m_creature);
                PhaseSubphase = 0;
                ++Phase;
                Phase_Timer = 8000;
                break;
            }
            break;
            //Phase 7 Pathaleons say 3 Sentence, every sentence need a subphase
        case 7:
            switch(PhaseSubphase)
            {
                //Subphase 1
            case 0:
                DoScriptText(SAY_PATHALEON_CULATOR_IMAGE_2, pathaleon);
                ++PhaseSubphase;
                Phase_Timer = 12000;
                break;
                //Subphase 2
            case 1:
                DoScriptText(SAY_PATHALEON_CULATOR_IMAGE_2_1, pathaleon);
                ++PhaseSubphase;
                Phase_Timer = 16000;
                break;
                //Subphase 3
            case 2:
                DoScriptText(SAY_PATHALEON_CULATOR_IMAGE_2_2, pathaleon);
                PhaseSubphase = 0;
                ++Phase;
                Phase_Timer = 10000;
                break;
            }
            break;
            //Phase 8 Dawnforge & Ardonis say
        case 8:
            DoScriptText(SAY_COMMANDER_DAWNFORGE_4, m_creature);
            DoScriptText(SAY_ARCANIST_ARDONIS_2, ardonis);
            ++Phase;
            Phase_Timer = 4000;
            break;
            //Phase 9 Pathaleons Despawn, Reset Dawnforge & Ardonis angle
        case 9:
            Turn_to_eachother();
            //hide pathaleon, unit will despawn shortly
            pathaleon->SetVisibility(VISIBILITY_OFF);
            PhaseSubphase = 0;
            ++Phase;
            Phase_Timer = 3000;
            break;
            //Phase 10 Dawnforge say
        case 10:
            DoScriptText(SAY_COMMANDER_DAWNFORGE_5, m_creature);
            player->AreaExploredOrEventHappens(QUEST_INFO_GATHERING);
            Reset();
            break;
        }
     }
};

CreatureAI* GetAI_npc_commander_dawnforge(Creature* _Creature)
{
    return new npc_commander_dawnforgeAI(_Creature);
}

Creature* SearchDawnforge(Player *source, uint32 entry, float range)
{
    Creature* pCreature = NULL;

    CellPair pair(Trinity::ComputeCellPair(source->GetPositionX(), source->GetPositionY()));
    Cell cell(pair);
    cell.data.Part.reserved = ALL_DISTRICT;
    cell.SetNoCreate();

    Trinity::NearestCreatureEntryWithLiveStateInObjectRangeCheck creature_check(*source, entry, true, range);
    Trinity::CreatureLastSearcher<Trinity::NearestCreatureEntryWithLiveStateInObjectRangeCheck> searcher(pCreature, creature_check);

    TypeContainerVisitor<Trinity::CreatureLastSearcher<Trinity::NearestCreatureEntryWithLiveStateInObjectRangeCheck>, GridTypeMapContainer> creature_searcher(searcher);

    cell.Visit(pair, creature_searcher, *(source->GetMap()));

    return pCreature;
}

bool AreaTrigger_at_commander_dawnforge(Player *player, AreaTriggerEntry const *at)
{
    //if player lost aura or not have at all, we should not try start event.
    if (!player->HasAura(SPELL_SUNFURY_DISGUISE,0))
        return false;

    if (player->IsAlive() && player->GetQuestStatus(QUEST_INFO_GATHERING) == QUEST_STATUS_INCOMPLETE)
    {
        Creature* Dawnforge = SearchDawnforge(player, CreatureEntry[1][0], 30.0f);

        if (!Dawnforge)
            return false;

        if (((npc_commander_dawnforgeAI*)Dawnforge->AI())->CanStartEvent(player))
            return true;
    }
    return false;
}

/*######
## npc_protectorate_nether_drake
######*/

#define GOSSIP_ITEM "I'm ready to fly! Take me up, dragon!"

bool GossipHello_npc_protectorate_nether_drake(Player *player, Creature *_Creature)
{
    //On Nethery Wings
    if (player->GetQuestStatus(10438) == QUEST_STATUS_INCOMPLETE && player->HasItemCount(29778,1) )
        player->ADD_GOSSIP_ITEM(0, GOSSIP_ITEM, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);

    player->SEND_GOSSIP_MENU(_Creature->GetNpcTextId(), _Creature->GetGUID());

    return true;
}

bool GossipSelect_npc_protectorate_nether_drake(Player *player, Creature *_Creature, uint32 sender, uint32 action )
{
    if (action == GOSSIP_ACTION_INFO_DEF+1)
    {
        player->CLOSE_GOSSIP_MENU();

        std::vector<uint32> nodes;

        nodes.resize(2);
        nodes[0] = 152;                                     //from drake
        nodes[1] = 153;                                     //end at drake
        player->ActivateTaxiPathTo(nodes);                  //TaxiPath 627 (possibly 627+628(152->153->154->155) )
    }
    return true;
}

/*######
## npc_professor_dabiri
######*/

#define SPELL_PHASE_DISTRUPTOR  35780
#define GOSSIP_ITEM "I need a new phase distruptor, Professor"
#define WHISPER_DABIRI -1000302

#define QUEST_DIMENSIUS 10439
#define QUEST_ON_NETHERY_WINGS 10438

bool GossipHello_npc_professor_dabiri(Player *player, Creature *_Creature)
{
    if (_Creature->isQuestGiver())
        player->PrepareQuestMenu( _Creature->GetGUID() );

    if(player->GetQuestStatus(QUEST_ON_NETHERY_WINGS) == QUEST_STATUS_INCOMPLETE && !player->HasItemCount(29778, 1))
        player->ADD_GOSSIP_ITEM(0, GOSSIP_ITEM, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);

    player->SEND_GOSSIP_MENU(_Creature->GetNpcTextId(), _Creature->GetGUID());

    return true;
}

bool GossipSelect_npc_professor_dabiri(Player *player, Creature *_Creature, uint32 sender, uint32 action )
{
    if (action == GOSSIP_ACTION_INFO_DEF+1)
    {
        _Creature->CastSpell(player, SPELL_PHASE_DISTRUPTOR, false);
        player->CLOSE_GOSSIP_MENU();
    }

    return true;
}

bool QuestAccept_npc_professor_dabiri(Player *player, Creature *creature, Quest const *quest )
{
    if(quest->GetQuestId() == QUEST_DIMENSIUS)
        DoScriptText(WHISPER_DABIRI, creature, player);

    return true;
}

/*######
## npc_veronia
######*/

#define GOSSIP_HV "Fly me to Manaforge Coruu please"

bool GossipHello_npc_veronia(Player *player, Creature *_Creature)
{
    if (_Creature->isQuestGiver())
        player->PrepareQuestMenu( _Creature->GetGUID() );

    //Behind Enemy Lines
    if (player->GetQuestStatus(10652) && !player->GetQuestRewardStatus(10652))
        player->ADD_GOSSIP_ITEM(0, GOSSIP_HV, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);

    player->SEND_GOSSIP_MENU(_Creature->GetNpcTextId(), _Creature->GetGUID());

    return true;
}

bool GossipSelect_npc_veronia(Player *player, Creature *_Creature, uint32 sender, uint32 action )
{
    if (action == GOSSIP_ACTION_INFO_DEF)
    {
        player->CLOSE_GOSSIP_MENU();
        player->CastSpell(player,34905,true);               //TaxiPath 606
    }
    return true;
}

/*######
## mob_phase_hunter
######*/

#define SUMMONED_MOB            19595
#define EMOTE_WEAK              -1000303

// Spells
#define SPELL_PHASE_SLIP        36574
#define SPELL_MANA_BURN         13321
#define SPELL_MATERIALIZE       34804
#define SPELL_DE_MATERIALIZE    34804

struct mob_phase_hunterAI : public ScriptedAI
{

    mob_phase_hunterAI(Creature *c) : ScriptedAI(c) {}

    bool Weak;
    bool Materialize;
    bool Drained;

    int WeakPercent;
    uint64 PlayerGUID;
    uint32 Health;
    uint32 Level;
    uint32 PhaseSlipVulnerabilityTimer;
    uint32 ManaBurnTimer;

    void Reset()
    {
        Weak = false;
        Materialize = false;
        Drained = false;

        WeakPercent = 20;
        PlayerGUID = 0;
        ManaBurnTimer = 5000 + (rand()%3 * 1000); // 5-8 sec cd
    }

    void EnterCombat(Unit *who)
    {
        if(Player *player = who->GetCharmerOrOwnerPlayerOrPlayerItself())
            PlayerGUID = player->GetGUID();
    }

    void SpellHit(Unit *caster, const SpellEntry *spell)
    {
        DoCast(m_creature, SPELL_DE_MATERIALIZE);
    }

    void UpdateAI(const uint32 diff)
    {
        if(!Materialize)
        {
            DoCast(m_creature, SPELL_MATERIALIZE);
            Materialize = true;
        }

        if(m_creature->HasAuraType(SPELL_AURA_MOD_DECREASE_SPEED) || m_creature->HasUnitState(UNIT_STAT_ROOT)) // if the mob is rooted/slowed by spells eg.: Entangling Roots, Frost Nova, Hamstring, Crippling Poison, etc. => remove it
            DoCast(m_creature, SPELL_PHASE_SLIP);

        if (!UpdateVictim())
            return;

        if(ManaBurnTimer < diff) // cast Mana Burn
        {
            if(m_creature->GetVictim()->GetCreateMana() > 0)
            {
                DoCast(m_creature->GetVictim(), SPELL_MANA_BURN);
                ManaBurnTimer = 8000 + (rand()%10 * 1000); // 8-18 sec cd
            }
        }else ManaBurnTimer -= diff;

        if(PlayerGUID) // start: support for quest 10190
        {
            Player* target = Unit::GetPlayer(PlayerGUID);

            if(target && !Weak && m_creature->GetHealth() < (m_creature->GetMaxHealth() / 100 * WeakPercent)
                && target->GetQuestStatus(10190) == QUEST_STATUS_INCOMPLETE)
            {
                DoScriptText(EMOTE_WEAK, m_creature);
                Weak = true;
            }
            if(Weak && !Drained && m_creature->HasAura(34219, 0))
            {
                Drained = true;

                Health = m_creature->GetHealth(); // get the normal mob's data
                Level = m_creature->getLevel();

                m_creature->AttackStop(); // delete the normal mob
                m_creature->DealDamage(m_creature, m_creature->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
                m_creature->RemoveCorpse();

                Creature* DrainedPhaseHunter = NULL;

                if(!DrainedPhaseHunter)
                    DrainedPhaseHunter = m_creature->SummonCreature(SUMMONED_MOB, m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ(), m_creature->GetOrientation(), TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 60000); // summon the mob

                if(DrainedPhaseHunter)
                {
                    DrainedPhaseHunter->SetLevel(Level); // set the summoned mob's data
                    DrainedPhaseHunter->SetHealth(Health);
                    DrainedPhaseHunter->AddThreat(target, 10000.0f);
                    DrainedPhaseHunter->AI()->AttackStart(target);
                }
            }
        }// end: support for quest 10190

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_mob_phase_hunter(Creature *_Creature)
{
    return new mob_phase_hunterAI (_Creature);
}

/*######
## npc_bessy
######*/

#define Q_ALMABTRIEB    10337
#define N_THADELL       20464
#define SPAWN_FIRST     20512
#define SPAWN_SECOND    19881
#define SAY_THADELL_1   -1000304
#define SAY_THADELL_2   -1000305

struct npc_bessyAI : public npc_escortAI
{

    npc_bessyAI(Creature *c) : npc_escortAI(c) {}

    bool Completed;

    void JustDied(Unit* killer)
    {
        if (PlayerGUID)
        {
            if (Player* player = GetPlayerForEscort())
                player->FailQuest(Q_ALMABTRIEB);
        }
    }

    void WaypointReached(uint32 i)
    {
        Player* player = GetPlayerForEscort();

        if (!player)
            return;

        switch(i)
        {
            case 3: //first spawn
                m_creature->SummonCreature(SPAWN_FIRST, 2449.67, 2183.11, 96.85, 6.20, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 25000);
                m_creature->SummonCreature(SPAWN_FIRST, 2449.53, 2184.43, 96.36, 6.27, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 25000);
                m_creature->SummonCreature(SPAWN_FIRST, 2449.85, 2186.34, 97.57, 6.08, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 25000);
                break;

            case 7:
                m_creature->SummonCreature(SPAWN_SECOND, 2309.64, 2186.24, 92.25, 6.06, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 25000);
                m_creature->SummonCreature(SPAWN_SECOND, 2309.25, 2183.46, 91.75, 6.22, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 25000);
                break;

            case 12:
                if (player)
                {
                    player->GroupEventHappens(Q_ALMABTRIEB, m_creature);
                    Completed = true;
                }
                {Unit* Thadell = FindCreature(N_THADELL, 30, m_creature);
                if(Thadell)
                    DoScriptText(SAY_THADELL_1, m_creature);}break;
            case 13:
                {Unit* Thadell = FindCreature(N_THADELL, 30, m_creature);
                if(Thadell)
                    DoScriptText(SAY_THADELL_2, m_creature, player);}break;
        }
    }

    void JustSummoned(Creature* summoned)
    {
        summoned->AI()->AttackStart(m_creature);
        AttackStart(summoned);
    }

    void EnterCombat(Unit* who){}

    void Reset()
    {
        Completed = false;
        m_creature->setFaction(35);
    }

    void UpdateAI(const uint32 diff)
    {
        npc_escortAI::UpdateAI(diff);
    }

};

bool QuestAccept_npc_bessy(Player* player, Creature* creature, Quest const* quest)
{
    if (quest->GetQuestId() == Q_ALMABTRIEB)
    {
        creature->setFaction(113);
        creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        ((npc_escortAI*)(creature->AI()))->Start(true, true, false, player->GetGUID(), creature->GetEntry());
    }
    return true;
}

CreatureAI* GetAI_npc_bessy(Creature *pCreature)
{
    return new npc_bessyAI(pCreature);
}

/*######
## npc_maxx_a_million
######*/

enum
{
    QUEST_MARK_V_IS_ALIVE = 10191,
    GO_DRAENEI_MACHINE = 183771
};

struct npc_maxx_a_million_escortAI : public npc_escortAI
{
    npc_maxx_a_million_escortAI(Creature* pCreature) : npc_escortAI(pCreature) {}

    bool bTake;
    uint32 uiTakeTimer;

    void Reset()
    {
        bTake=false;
        uiTakeTimer=3000;
    }
    
    void EnterCombat(Unit *pWho) {}

    void WaypointReached(uint32 i)
    {
        Player* pPlayer = GetPlayerForEscort();
        if (!pPlayer)
            return;

        switch (i) {
        case 7:
        case 17:
        case 29:
            //Find Object and "work"
            if (GameObject* pGO = m_creature->FindGOInGrid(GO_DRAENEI_MACHINE,INTERACTION_DISTANCE)) {
                // take the GO -> animation
                me->HandleEmoteCommand(428);
                SetEscortPaused(true);
                bTake=true;
            }
            break;
        case 36: //return and quest_complete
            if (pPlayer)
                pPlayer->CompleteQuest(QUEST_MARK_V_IS_ALIVE);
            break;
        }
    }

    void JustDied(Unit* pKiller)
    {
        Player* pPlayer = GetPlayerForEscort();
        if (pPlayer && pPlayer->GetQuestStatus(QUEST_MARK_V_IS_ALIVE) == QUEST_STATUS_INCOMPLETE)
            pPlayer->FailQuest(QUEST_MARK_V_IS_ALIVE);
    }

    void UpdateAI(const uint32 uiDiff)
    {
        npc_escortAI::UpdateAI(uiDiff);

        if (bTake) {
            if (uiTakeTimer < uiDiff) {
                me->HandleEmoteCommand(EMOTE_STATE_NONE);
                if (GameObject* pGO = m_creature->FindGOInGrid(GO_DRAENEI_MACHINE,INTERACTION_DISTANCE))
                {
                    SetEscortPaused(false);
                    bTake=false;
                    uiTakeTimer = 3000;
                    pGO->Delete();
                }
            }
            else
                uiTakeTimer -= uiDiff;
        }
        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_max_a_million_escort(Creature* pCreature)
{
    return new npc_maxx_a_million_escortAI(pCreature);
}

bool QuestAccept_npc_max_a_million_escort(Player* pPlayer, Creature* pCreature, const Quest* pQuest)
{
    if (pQuest->GetQuestId() == QUEST_MARK_V_IS_ALIVE) {
        pCreature->setFaction(113);
        ((npc_escortAI*)(pCreature->AI()))->Start(true, true, false, pPlayer->GetGUID(), pCreature->GetEntry());
    }
    return true;
}

enum
{
    QUEST_A_NOT_SO_MODEST_PROPOSAL = 10270,
	NPC_IMAGE_OF_WIND_TRADER_MARID = 20518
};

bool GOHello_go_ethereal_teleport_pad(Player *player, GameObject* go)
{
	if (player->GetQuestStatus(QUEST_A_NOT_SO_MODEST_PROPOSAL) == QUEST_STATUS_COMPLETE)
	{
		Creature *npc = go->FindNearestCreature(NPC_IMAGE_OF_WIND_TRADER_MARID, 1.0f); // prevents double spawn
		if (npc)
			npc->DisappearAndDie();
		go->SummonCreature(NPC_IMAGE_OF_WIND_TRADER_MARID, 4007.11f, 1517.15f, -115.535f, 4.97726f, TEMPSUMMON_TIMED_DESPAWN, 60000);

	}
    return true;
}

/*######
## npc_dr_boom
######*/

enum
{
    THROW_DYNAMITE    = 35276,
    BOOM_BOT          = 19692,
    BOOM_BOT_TARGET   = 20392
};

struct npc_dr_boomAI : public Scripted_NoMovementAI
{
    npc_dr_boomAI(Creature *pCreature) : Scripted_NoMovementAI(pCreature) {}

    std::vector<uint64> targetGUID;
    uint32 SummonTimer;
    uint32 InitTimer;

    void Reset()
    {
        SummonTimer = 1500;
        InitTimer = 1000;
    }
    
    void EnterCombat(Unit* who) {}

    void UpdateAI(const uint32 uiDiff)
    {
        if (InitTimer) {
            if (InitTimer <= uiDiff) {
                std::list<Creature*> targets;
                me->GetCreatureListWithEntryInGrid(targets, BOOM_BOT_TARGET, 30.0f);
                targetGUID.clear();
                for (std::list<Creature*>::iterator it = targets.begin(); it != targets.end(); it++)
                    targetGUID.push_back((*it)->GetGUID());
                    
                InitTimer = 0;
            }
            else
                InitTimer -= uiDiff;
        }

        if (SummonTimer <= uiDiff)
        {
            if (targetGUID.size())
            {
                if (Unit* target = Unit::GetUnit(*me, targetGUID[rand()%targetGUID.size()]))
                {
                    if (Unit* bot = DoSpawnCreature(BOOM_BOT, 0, 0, 0, 0, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 20000))
                        bot->GetMotionMaster()->MovePoint(0, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ());
                }
            }

            SummonTimer = 1500;
        }
        else SummonTimer -= uiDiff;

        if (!UpdateVictim())
            return;

        if (!me->IsWithinDistInMap(me->GetVictim(), 30.0f))
        {
            EnterEvadeMode();
            return;
        }

        if (me->isAttackReady() && me->IsWithinDistInMap(me->GetVictim(), 8.0f))
        {
            DoCast(me->GetVictim(), THROW_DYNAMITE, true);
            me->resetAttackTimer();
        }
    }
};

CreatureAI* GetAI_npc_dr_boom(Creature* pCreature)
{
    return new npc_dr_boomAI (pCreature);
}

/*######
## npc_boom_bot
######*/

#define    SPELL_BOOM    35132                         

struct npc_boom_botAI : public ScriptedAI
{
    npc_boom_botAI(Creature *pCreature) : ScriptedAI(pCreature) {}

    bool Boom;
    uint32 BoomTimer;

    void Reset()
    {
        Boom = false;
        BoomTimer = 800;
        me->SetUnitMovementFlags(MOVEMENTFLAG_WALK_MODE);
    }
    
    void EnterCombat(Unit* who) {}

    void AttackedBy(Unit* pWho) {}

    void AttackStart(Unit* pWho) {}

    void MovementInform(uint32 type, uint32 id)
    {
        if (type != POINT_MOTION_TYPE)
            return;

        DoCast(me, SPELL_BOOM);
        Boom = true;
    }

    void MoveInLineOfSight(Unit *pWho)
    {
        if (!pWho->isCharmedOwnedByPlayerOrPlayer())
            return;

        if (me->IsWithinDistInMap(pWho, 4.0f, false))
        {
            DoCast(me, SPELL_BOOM);
            Boom = true;
        }
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if (Boom)
        {
            if (BoomTimer <= uiDiff)
            {
                me->setDeathState(CORPSE);
            }
            else BoomTimer -= uiDiff;
        }
    }
};

CreatureAI* GetAI_npc_boom_bot(Creature* pCreature)
{
    return new npc_boom_botAI (pCreature);
}

/*######
## npc_drijya
######*/

enum
{
    SAY_DR_START           = -1900156,
    SAY_DR_1               = -1900157,
    SAY_DR_2               = -1900158,
    SAY_DR_3               = -1900159,
    SAY_DR_4               = -1900160,
    SAY_DR_5               = -1900161,
    SAY_DR_6               = -1900162,
    SAY_DR_7               = -1900163,
    SAY_DR_COMPLETE        = -1900164,

    QUEST_WARP_GATE        = 10310,

    MAX_TROOPER            = 9,
    MAX_IMP                = 6,

    NPC_EXPLODE_TRIGGER    = 20296,
    NPC_IMP                = 20399,
    NPC_TROOPER            = 20402,
    NPC_DESTROYER          = 20403,

    GO_SMOKE               = 185318,
    GO_FIRE                = 185317,
    GO_BIG_FIRE            = 185319
};

struct npc_drijyaAI : public npc_escortAI
{
    npc_drijyaAI(Creature* pCreature) : npc_escortAI(pCreature) {}

    bool Destroy;
    bool SummonImp;
    bool SummonTrooper;
    bool SummonDestroyer;

    uint32 Count;
    uint32 SpawnTimer;
    uint32 StartSpawnTimer;
    uint32 DestroyingTimer;

    void Reset()
    {
        Destroy = false;
        SummonImp = false;
        SummonTrooper = false;
        SummonDestroyer = false;
        Count = 0;
        SpawnTimer = 3500;
        StartSpawnTimer = 15000;
        DestroyingTimer = 60000;
    }

    void AttackedBy(Unit* pWho) {}

    void AttackStart(Unit* pWho) {}
    
    void EnterCombat(Unit* who) {}

    void SpawnImp()
    {
        ++Count;
        me->SummonCreature(NPC_IMP, 3025.752f, 2715.122, 113.758, 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10000);
    }

    void SpawnTrooper()
    {
        ++Count;
        me->SummonCreature(NPC_TROOPER, 3025.752f, 2715.122, 113.758, 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10000);
    }

    void SpawnDestroyer()
    {
        me->SummonCreature(NPC_DESTROYER, 3019.741f, 2720.757, 115.189, 2.5f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10000);
    }

    void JustSummoned(Creature* pSummoned)
    {
        if (pSummoned->GetEntry() == NPC_IMP)
        {
            if (Player* pPlayer = GetPlayerForEscort())
                pSummoned->AI()->AttackStart(pPlayer); 
        }

        if (pSummoned->GetEntry() == NPC_TROOPER)
        {
            if(Player* pPlayer = GetPlayerForEscort())
                pSummoned->AI()->AttackStart(pPlayer);
        }
        else
        {
            if (pSummoned->GetEntry() == NPC_DESTROYER)
            {
                if (Player* pPlayer = GetPlayerForEscort())
                    pSummoned->AI()->AttackStart(pPlayer);
            }
        }
    }

    void WaypointReached(uint32 uiPointId)
    {
        switch(uiPointId)
        {
            case 0:
                DoScriptText(SAY_DR_START, me);
                SetRun();
                break;
            case 1:
                DoScriptText(SAY_DR_1, me);
                break;
            case 5:
                DoScriptText(SAY_DR_2, me);
                break;
            case 7:
                SetEscortPaused(true);
                Destroy = true;
                SummonImp = true;
                break;
            case 8:
                me->SummonGameObject(GO_SMOKE, 3049.354f, 2726.431f, 113.922f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 215);
                DoScriptText(SAY_DR_4, me);
                break;
            case 12:
                SetEscortPaused(true);
                Destroy = true;
                SummonTrooper = true;
                break;
            case 13:
                me->SummonGameObject(GO_SMOKE, 3020.842f, 2697.501f, 113.368f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 140);
                DoScriptText(SAY_DR_5, me);
                break;
            case 17:
                SetEscortPaused(true);
                Destroy = true;
                SummonDestroyer = true;
                break;
            case 18:
                DoScriptText(SAY_DR_6, me);
                if (Creature* pTrigger = me->FindNearestCreature(NPC_EXPLODE_TRIGGER, 20.0f))
                {
                    pTrigger->CastSpell(pTrigger, SPELL_BOOM , false);
                }
                me->SummonGameObject(GO_SMOKE, 3008.503f, 2729.432f, 114.350f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 60);
                me->SummonGameObject(GO_FIRE, 3026.163f, 2723.538f, 113.681f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 60);
                me->SummonGameObject(GO_BIG_FIRE, 3021.556f, 2718.887f, 115.055f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 60);
                break;
            case 19:
                DoScriptText(SAY_DR_7, me);
                break;
            case 22:
                SetRun(false);
                break;
            case 26:
                if (Player* pPlayer = GetPlayerForEscort())
                {
                    DoScriptText(SAY_DR_COMPLETE, me, pPlayer);
                    pPlayer->GroupEventHappens(QUEST_WARP_GATE, me);
                }
                break;
        }
    }

    void UpdateAI(const uint32 uiDiff)
    {
        npc_escortAI::UpdateAI(uiDiff);

        if (SummonImp)
        {
            if (StartSpawnTimer <= uiDiff) 
            {
                if (SpawnTimer <= uiDiff)
                {
                    if (Count >= MAX_IMP)
                    {
                        DoScriptText(SAY_DR_3, me);
                        SummonImp = false;
                        StartSpawnTimer = 15000;
                    }
                    SpawnTimer = 3500;
                    SpawnImp();
                }
                else SpawnTimer -= uiDiff;
            }
            else StartSpawnTimer -= uiDiff;
        }

        if (SummonTrooper)
        {
            if (StartSpawnTimer <= uiDiff)
            {
                if (SpawnTimer <= uiDiff)
                {
                    if (Count >= MAX_TROOPER)
                    {
                        SummonTrooper = false;
                        StartSpawnTimer = 15000;
                    }

                    SpawnTimer = 3500;
                    SpawnTrooper();
                }
                else SpawnTimer -= uiDiff;
            }
            else StartSpawnTimer -= uiDiff;
        }

        if (SummonDestroyer)
        {
            if (StartSpawnTimer <= uiDiff)
            {
                SpawnDestroyer();
                SummonDestroyer = false;
                StartSpawnTimer = 15000;
            }
            else StartSpawnTimer -= uiDiff;
        }

        if (Destroy)
        {
            if (DestroyingTimer <= uiDiff)
            {
                SetEscortPaused(false);
                Destroy = false;
                DestroyingTimer = 60000;
            }
            else DestroyingTimer -= uiDiff;

            me->HandleEmoteCommand(173);

        }
        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_drijya(Creature* pCreature)
{
    return new npc_drijyaAI(pCreature);
}

bool QuestAccept_npc_drijya(Player* pPlayer, Creature* pCreature, const Quest* pQuest)
{
    if (pQuest->GetQuestId() == QUEST_WARP_GATE)
    {
        if (npc_drijyaAI* pEscortAI = dynamic_cast<npc_drijyaAI*>(pCreature->AI()))
        {
            pCreature->setFaction(113);
            pEscortAI->Start(true, true, false, pPlayer->GetGUID(), pCreature->GetEntry());
        }
    }

    return true;
}

/*######
## AddSC
######*/

void AddSC_netherstorm()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name="go_manaforge_control_console";
    newscript->pGOHello = &GOHello_go_manaforge_control_console;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="npc_manaforge_control_console";
    newscript->GetAI = &GetAI_npc_manaforge_control_console;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_commander_dawnforge";
    newscript->GetAI = &GetAI_npc_commander_dawnforge;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "at_commander_dawnforge";
    newscript->pAreaTrigger = &AreaTrigger_at_commander_dawnforge;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="npc_protectorate_nether_drake";
    newscript->pGossipHello =   &GossipHello_npc_protectorate_nether_drake;
    newscript->pGossipSelect =  &GossipSelect_npc_protectorate_nether_drake;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_professor_dabiri";
    newscript->pGossipHello =   &GossipHello_npc_professor_dabiri;
    newscript->pGossipSelect =  &GossipSelect_npc_professor_dabiri;
    newscript->pQuestAccept = &QuestAccept_npc_professor_dabiri;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="npc_veronia";
    newscript->pGossipHello =   &GossipHello_npc_veronia;
    newscript->pGossipSelect =  &GossipSelect_npc_veronia;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "mob_phase_hunter";
    newscript->GetAI = &GetAI_mob_phase_hunter;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_bessy";
    newscript->GetAI = &GetAI_npc_bessy;
    newscript->pQuestAccept = &QuestAccept_npc_bessy;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_maxx_a_million_escort";
    newscript->GetAI = &GetAI_npc_max_a_million_escort;
    newscript->pQuestAccept = &QuestAccept_npc_max_a_million_escort;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "go_ethereal_teleport_pad";
    newscript->pGOHello = &GOHello_go_ethereal_teleport_pad;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_dr_boom";
    newscript->GetAI = &GetAI_npc_dr_boom;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_boom_bot";
    newscript->GetAI = &GetAI_npc_boom_bot;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_drijya";
    newscript->GetAI = &GetAI_npc_drijya;
    newscript->pQuestAccept = &QuestAccept_npc_drijya;
    newscript->RegisterSelf();
}
