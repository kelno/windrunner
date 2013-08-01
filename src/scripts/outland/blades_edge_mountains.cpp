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
SDName: Blades_Edge_Mountains
SD%Complete: 90
SDComment: Quest support: 10503, 10504, 10556, 10609, 10682, 10821, 10980, 10998, 11000 Ogri'la->Skettis Flight. (npc_daranelle needs bit more work before consider complete)
SDCategory: Blade's Edge Mountains
EndScriptData */

/* ContentData
mobs_bladespire_ogre
mobs_nether_drake
npc_daranelle
npc_overseer_nuaar
npc_saikkal_the_elder
npc_skyguard_handler_irena
go_legion_obelisk
npc_prophecy_questcredit
npc_grishna_falconwing
go_ethereum_chamber
npc_kolphis_darkscale
trigger_vimgol_circle_bunny
npc_vimgol
npc_soulgrinder
npc_skulloc
npc_sundered_ghost
go_apexis_relic
npc_simon_bunny
go_blue_cluster
go_green_cluster
go_red_cluster
go_yellow_cluster
go_fel_crystal_prism
npc_braxxus
npc_moarg_incinerator
npc_galvanoth
npc_zarcsin
npc_aether_ray
npc_wrangled_aether_ray
go_drake_egg
npc_rivendark
npc_obsidia
npc_insidion
npc_furywing
trigger_banishing_crystal_bunny01
npc_rally_zapnabber
npc_grulloc
npc_grishna
EndContentData */

#include "precompiled.h"
#include "ObjectMgr.h"
/*#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"*/

/*######
## mobs_bladespire_ogre
######*/

//TODO: add support for quest 10512 + creature abilities
struct mobs_bladespire_ogreAI : public ScriptedAI
{
    mobs_bladespire_ogreAI(Creature *c) : ScriptedAI(c) {}

    void Reset()
    {
    }

    void Aggro(Unit* pWho)
    {
    }

    void JustDied(Unit* pKiller)
    {
        if (pKiller->GetTypeId() == TYPEID_PLAYER)
            CAST_PLR(pKiller)->KilledMonster(19995, m_creature->GetGUID());
    }
};
CreatureAI* GetAI_mobs_bladespire_ogre(Creature *pCreature)
{
    return new mobs_bladespire_ogreAI (pCreature);
}

/*######
## mobs_nether_drake
######*/

enum eNetherDrake
{
SAY_NIHIL_1                 = -1000396,
SAY_NIHIL_2                 = -1000397,
SAY_NIHIL_3                 = -1000398,
SAY_NIHIL_4                 = -1000399,
SAY_NIHIL_INTERRUPT         = -1000400,

ENTRY_WHELP                 = 20021,
ENTRY_PROTO                 = 21821,
ENTRY_ADOLE                 = 21817,
ENTRY_MATUR                 = 21820,
ENTRY_NIHIL                 = 21823,

SPELL_T_PHASE_MODULATOR     = 37573,
SPELL_ARCANE_BLAST          = 38881,
SPELL_MANA_BURN             = 38884,
SPELL_INTANGIBLE_PRESENCE   = 36513
};

struct mobs_nether_drakeAI : public ScriptedAI
{
    mobs_nether_drakeAI(Creature *c) : ScriptedAI(c) {}

    bool IsNihil;
    uint32 NihilSpeech_Timer;
    uint32 NihilSpeech_Phase;

    uint32 ArcaneBlast_Timer;
    uint32 ManaBurn_Timer;
    uint32 IntangiblePresence_Timer;

    void Reset()
    {
        NihilSpeech_Timer = 2000;
        IsNihil = false;
        if( m_creature->GetEntry() == ENTRY_NIHIL )
        {
            m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            IsNihil = true;
        }
        NihilSpeech_Phase = 1;

        ArcaneBlast_Timer = 7500;
        ManaBurn_Timer = 10000;
        IntangiblePresence_Timer = 15000;
    }

    void Aggro(Unit* pWho) { }

    void SpellHit(Unit *caster, const SpellEntry *spell)
    {
        if (spell->Id == SPELL_T_PHASE_MODULATOR && caster->GetTypeId() == TYPEID_PLAYER)
        {
            uint32 cEntry = 0;

            switch (m_creature->GetEntry())
            {
                case ENTRY_WHELP:
                    cEntry = RAND(ENTRY_PROTO, ENTRY_ADOLE, ENTRY_MATUR, ENTRY_NIHIL);
                    break;
                case ENTRY_PROTO:
                case ENTRY_ADOLE:
                case ENTRY_MATUR:
                    cEntry = RAND(ENTRY_ADOLE, ENTRY_MATUR, ENTRY_NIHIL);
                    break;
                case ENTRY_NIHIL:
                    if (NihilSpeech_Phase)
                    {
                        DoScriptText(SAY_NIHIL_INTERRUPT, m_creature);
                        IsNihil = false;
                        cEntry = RAND(ENTRY_ADOLE, ENTRY_MATUR, ENTRY_NIHIL);
                    }
                    break;
            }

            if (cEntry)
            {
                m_creature->UpdateEntry(cEntry);

                if (cEntry == ENTRY_NIHIL)
                {
                    m_creature->InterruptNonMeleeSpells(true);
                    m_creature->RemoveAllAuras();
                    m_creature->DeleteThreatList();
                    m_creature->CombatStop();
                    InCombat = false;
                    Reset();
                }
            }
        }
    }

    void UpdateAI(const uint32 diff)
    {
        if (IsNihil)
        {
            if (NihilSpeech_Phase)
            {
                if (NihilSpeech_Timer <= diff)
                {
                    switch (NihilSpeech_Phase)
                    {
                        case 1:
                            DoScriptText(SAY_NIHIL_1, m_creature);
                            ++NihilSpeech_Phase;
                            break;
                        case 2:
                            DoScriptText(SAY_NIHIL_2, m_creature);
                            ++NihilSpeech_Phase;
                            break;
                        case 3:
                            DoScriptText(SAY_NIHIL_3, m_creature);
                            ++NihilSpeech_Phase;
                            break;
                        case 4:
                            DoScriptText(SAY_NIHIL_4, m_creature);
                            ++NihilSpeech_Phase;
                            break;
                        case 5:
                            m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                                                            // + MOVEMENTFLAG_LEVITATING
                            m_creature->SetDisableGravity(true);
                            //then take off to random location. creature is initially summoned, so don't bother do anything else.
                            m_creature->GetMotionMaster()->MovePoint(0, m_creature->GetPositionX()+100, m_creature->GetPositionY(), m_creature->GetPositionZ()+100);
                            NihilSpeech_Phase = 0;
                            break;
                    }
                    NihilSpeech_Timer = 5000;
                }else NihilSpeech_Timer -=diff;
            }
            return;                                         //anything below here is not interesting for Nihil, so skip it
        }

        if (!UpdateVictim())
            return;

        if (IntangiblePresence_Timer <= diff)
        {
            DoCast(m_creature->getVictim(),SPELL_INTANGIBLE_PRESENCE);
            IntangiblePresence_Timer = 15000+rand()%15000;
        }else IntangiblePresence_Timer -= diff;

        if (ManaBurn_Timer <= diff)
        {
            Unit* target = m_creature->getVictim();
            if (target && target->getPowerType() == POWER_MANA)
                DoCast(target,SPELL_MANA_BURN);
            ManaBurn_Timer = 8000+rand()%8000;
        }else ManaBurn_Timer -= diff;

        if (ArcaneBlast_Timer <= diff)
        {
            DoCast(m_creature->getVictim(),SPELL_ARCANE_BLAST);
            ArcaneBlast_Timer = 2500+rand()%5000;
        }else ArcaneBlast_Timer -= diff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_mobs_nether_drake(Creature *pCreature)
{
    return new mobs_nether_drakeAI (pCreature);
}

/*######
## npc_daranelle
######*/

#define SAY_DARANELLE -1000401

struct npc_daranelleAI : public ScriptedAI
{
    npc_daranelleAI(Creature *c) : ScriptedAI(c) {}

    void Reset()
    {
    }

    void Aggro(Unit* pWho)
    {
    }

    void MoveInLineOfSight(Unit* pWho)
    {
        if (pWho->GetTypeId() == TYPEID_PLAYER)
        {
            if (pWho->HasAura(36904))
            {
                DoScriptText(SAY_DARANELLE, m_creature, pWho);
                //TODO: Move the below to updateAI and run if this statement == true
                CAST_PLR(pWho)->KilledMonster(21511, m_creature->GetGUID());
                CAST_PLR(pWho)->RemoveAurasDueToSpell(36904);
            }
        }

        ScriptedAI::MoveInLineOfSight(pWho);
    }
};

CreatureAI* GetAI_npc_daranelle(Creature *pCreature)
{
    return new npc_daranelleAI (pCreature);
}

/*######
## npc_overseer_nuaar
######*/

#define GOSSIP_HON "Overseer, I am here to negotiate on behalf of the Cenarion Expedition."

bool GossipHello_npc_overseer_nuaar(Player *pPlayer, Creature *pCreature)
{
    if (pPlayer->GetQuestStatus(10682) == QUEST_STATUS_INCOMPLETE)
        pPlayer->ADD_GOSSIP_ITEM(0, GOSSIP_HON, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);

    pPlayer->SEND_GOSSIP_MENU(10532, pCreature->GetGUID());

    return true;
}

bool GossipSelect_npc_overseer_nuaar(Player* pPlayer, Creature* pCreature, uint32 sender, uint32 action)
{
    if (action == GOSSIP_ACTION_INFO_DEF+1)
    {
        pPlayer->SEND_GOSSIP_MENU(10533, pCreature->GetGUID());
        pPlayer->AreaExploredOrEventHappens(10682);
    }
    return true;
}

/*######
## npc_saikkal_the_elder
######*/

#define GOSSIP_HSTE "Yes... yes, it's me."
#define GOSSIP_SSTE "Yes elder. Tell me more of the book."

bool GossipHello_npc_saikkal_the_elder(Player* pPlayer, Creature* pCreature)
{
    if (pPlayer->GetQuestStatus(10980) == QUEST_STATUS_INCOMPLETE)
        pPlayer->ADD_GOSSIP_ITEM( 0, GOSSIP_HSTE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);

    pPlayer->SEND_GOSSIP_MENU(10794, pCreature->GetGUID());

    return true;
}

bool GossipSelect_npc_saikkal_the_elder(Player* pPlayer, Creature* pCreature, uint32 sender, uint32 action)
{
    switch (action)
    {
        case GOSSIP_ACTION_INFO_DEF+1:
            pPlayer->ADD_GOSSIP_ITEM( 0, GOSSIP_SSTE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);
            pPlayer->SEND_GOSSIP_MENU(10795, pCreature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+2:
            pPlayer->TalkedToCreature(pCreature->GetEntry(), pCreature->GetGUID());
            pPlayer->SEND_GOSSIP_MENU(10796, pCreature->GetGUID());
            break;
    }
    return true;
}

/*######
## npc_skyguard_handler_irena
######*/

#define GOSSIP_SKYGUARD "Fly me to Skettis please"

bool GossipHello_npc_skyguard_handler_irena(Player* pPlayer, Creature* pCreature )
{
    if (pCreature->isQuestGiver())
        pPlayer->PrepareQuestMenu( pCreature->GetGUID() );

    if (pPlayer->GetReputationRank(1031) >= REP_HONORED)
        pPlayer->ADD_GOSSIP_ITEM( 2, GOSSIP_SKYGUARD, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);

    pPlayer->SEND_GOSSIP_MENU(pCreature->GetNpcTextId(), pCreature->GetGUID());

    return true;
}

bool GossipSelect_npc_skyguard_handler_irena(Player* pPlayer, Creature* pCreature, uint32 sender, uint32 action )
{
    if (action == GOSSIP_ACTION_INFO_DEF+1)
    {
        pPlayer->CLOSE_GOSSIP_MENU();

        std::vector<uint32> nodes;

        nodes.resize(2);
        nodes[0] = 172;                                     //from ogri'la
        nodes[1] = 171;                                     //end at skettis
        pPlayer->ActivateTaxiPathTo(nodes);                  //TaxiPath 706
    }
    return true;
}

/*######
## go_legion_obelisk
######*/

//Support for quest: You're Fired! (10821)
bool bObeliskOne, bObeliskTwo, bObeliskThree, bObeliskFour, bObeliskFive;	

enum eLegionObelisk
{
LEGION_OBELISK_ONE           = 185193,
LEGION_OBELISK_TWO           = 185195,
LEGION_OBELISK_THREE         = 185196,
LEGION_OBELISK_FOUR          = 185197,
LEGION_OBELISK_FIVE          = 185198
};

bool GOHello_go_legion_obelisk(Player *pPlayer, GameObject* pGo)
{	
	if (pPlayer->GetQuestStatus(10821) == QUEST_STATUS_INCOMPLETE)
	{
		switch(pGo->GetEntry())
		{
			case LEGION_OBELISK_ONE:
                bObeliskOne = true;
                break;
			case LEGION_OBELISK_TWO:
				bObeliskTwo = true;
                break;
			case LEGION_OBELISK_THREE:
                bObeliskThree = true;
                break;
			case LEGION_OBELISK_FOUR:
                bObeliskFour = true;
                break;
			case LEGION_OBELISK_FIVE:
                bObeliskFive = true;
                break;
		}
	
		if (bObeliskOne && bObeliskTwo && bObeliskThree && bObeliskFour && bObeliskFive)
		{
			pGo->SummonCreature(19963, 2943.40f, 4778.20f, 284.49f, 0.94f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 120000);
			//reset global var
			bObeliskOne = false;
			bObeliskTwo = false;
			bObeliskThree = false;
			bObeliskFour = false;
			bObeliskFive = false;
		}
	}
	
	return true;
}

/*######
## npc_prophecy_questcredit
######*/

#define QUEST_WHISPERS_RAVEN_GOD    10607

struct npc_prophecy_questcreditAI : public ScriptedAI
{
    npc_prophecy_questcreditAI(Creature *c) : ScriptedAI(c) {}
    
    void Aggro(Unit *pWho) {}
    
    void Reset()
    {
        m_creature->SetReactState(REACT_AGGRESSIVE);
    }
    
    void MoveInLineOfSight(Unit *pWho)
    {
        if (m_creature->GetDistance(pWho) >= 5.0f)
            return;
        if (pWho->GetTypeId() != TYPEID_PLAYER)
            return;
        
        Player *plr = reinterpret_cast<Player*>(pWho);
        if (plr->GetQuestStatus(QUEST_WHISPERS_RAVEN_GOD) == QUEST_STATUS_INCOMPLETE) {
            if (plr->HasAura(37642))
                plr->KilledMonster(m_creature->GetEntry(), m_creature->GetGUID());
        }
    }
};

CreatureAI* GetAI_npc_prophecy_questcredit(Creature *pCreature)
{
    return new npc_prophecy_questcreditAI(pCreature);
}

/*######
## npc_grishna_falconwing
######*/

struct npc_grishna_falconwingAI : public ScriptedAI
{
    npc_grishna_falconwingAI(Creature *c) : ScriptedAI(c) {}
    
    void Aggro(Unit *pWho) {}
    
    void JustDied(Unit *pKiller)
    {
        pKiller->AddAura(37642, pKiller);
    }
};

CreatureAI* GetAI_npc_grishna_falconwingAI(Creature *pCreature)
{
    return new npc_grishna_falconwingAI(pCreature);
}

/*######
## go_ethereum_chamber
######*/

uint32 ethereumPrisoners[5] = { 22828, 22826, 22827, 20888, 22825 };

bool GOHello_go_ethereum_chamber(Player *pPlayer, GameObject *pGo)
{
    if (Creature *pCreature = pPlayer->SummonCreature(ethereumPrisoners[rand()%5], pGo->GetPositionX(), pGo->GetPositionY(), pGo->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 30000))
        pCreature->AI()->AttackStart(pPlayer);
        
    return true;
}

/*######
## npc_kolphis_darkscale
######*/

bool GossipHello_npc_kolphis_darkscale(Player *pPlayer, Creature *pCreature)
{
    pPlayer->GroupEventHappens(10722, pCreature);
    
    return true;
}

/*######
## trigger_vimgol_circle_bunny
######*/

bool vimgol1, vimgol2, vimgol3, vimgol4, vimgol5;

struct trigger_vimgol_circle_bunnyAI : public Scripted_NoMovementAI
{
    trigger_vimgol_circle_bunnyAI(Creature *c) : Scripted_NoMovementAI(c) {}
    
    uint32 checkTimer;
    bool hasResetVisual;
    
    void Reset()
    {
        vimgol1 = false;
        vimgol2 = false;
        vimgol3 = false;
        vimgol4 = false;
        vimgol5 = false;
        
        checkTimer = 500;
        
        hasResetVisual = false;
    }
    
    void Aggro(Unit *pWho) {}
    
    void JustDied(Unit *pKiller)
    {
        vimgol1 = false;
        vimgol2 = false;
        vimgol3 = false;
        vimgol4 = false;
        vimgol5 = false;
    }
    
    void UpdateAI(uint32 const diff)
    {
        if (!hasResetVisual) {
            if (Creature *visualBunny = m_creature->FindCreatureInGrid(23081, 2.0f, true))
                visualBunny->GetMotionMaster()->MoveTargetedHome();
                
            hasResetVisual = true;
        }
        if (checkTimer <= diff) {
            Creature *vimgol = m_creature->FindNearestCreature(22911, 45.0f, true);
            if (vimgol) {    // No need to continue if vimgol's already there
                checkTimer = 500;
                return;
            }
                
            CellPair p(Trinity::ComputeCellPair(m_creature->GetPositionX(), m_creature->GetPositionY()));
            Cell cell(p);
            cell.data.Part.reserved = ALL_DISTRICT;
                
            Player* plr = NULL;
            Trinity::AnyPlayerInObjectRangeCheck p_check(m_creature, 0.5f);
            Trinity::PlayerSearcher<Trinity::AnyPlayerInObjectRangeCheck>  checker(plr, p_check);

            TypeContainerVisitor<Trinity::PlayerSearcher<Trinity::AnyPlayerInObjectRangeCheck>, WorldTypeMapContainer > world_object_checker(checker);
            cell.Visit(p, world_object_checker, *(m_creature->GetMap()));
            
            if (plr) {
                if (int32(m_creature->GetPositionX()) == 3304)
                    vimgol1 = true;
                else if (int32(m_creature->GetPositionX()) == 3292)
                    vimgol2 = true;
                else if (int32(m_creature->GetPositionX()) == 3261)
                    vimgol3 = true;
                else if (int32(m_creature->GetPositionX()) == 3257)
                    vimgol4 = true;
                else if (int32(m_creature->GetPositionX()) == 3279)
                    vimgol5 = true;
            }
            else {
                if (int32(m_creature->GetPositionX()) == 3304)
                    vimgol1 = false;
                else if (int32(m_creature->GetPositionX()) == 3292)
                    vimgol2 = false;
                else if (int32(m_creature->GetPositionX()) == 3261)
                    vimgol3 = false;
                else if (int32(m_creature->GetPositionX()) == 3257)
                    vimgol4 = false;
                else if (int32(m_creature->GetPositionX()) == 3279)
                    vimgol5 = false;
            }
            
            // Check if 5 players are in circles
            if (vimgol1 && vimgol2 && vimgol3 && vimgol4 && vimgol5) {
                Creature *vimgol = m_creature->FindNearestCreature(22911, 45.0f, true);
                if (!vimgol) {
                    m_creature->SummonCreature(22911, 3279.770020, 4640.019531, 216.527039, 1.5874, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 60000);

                    CellPair pair(Trinity::ComputeCellPair(m_creature->GetPositionX(), m_creature->GetPositionY()));
                    Cell cell(pair);
                    cell.data.Part.reserved = ALL_DISTRICT;
                    cell.SetNoCreate();
                    std::list<Creature*> visualBunnies;

                    Trinity::AllCreaturesOfEntryInRange check(m_creature, 23081, 50.0f);
                    Trinity::CreatureListSearcher<Trinity::AllCreaturesOfEntryInRange> searcher(visualBunnies, check);
                    TypeContainerVisitor<Trinity::CreatureListSearcher<Trinity::AllCreaturesOfEntryInRange>, GridTypeMapContainer> visitor(searcher);

                    cell.Visit(pair, visitor, *m_creature->GetMap());

                    for (std::list<Creature*>::iterator itr = visualBunnies.begin(); itr != visualBunnies.end(); itr++)
                        (*itr)->CastSpell(*itr, 39921, false);
                }
            }
            
            checkTimer = 500;
        }
        else
            checkTimer -= diff;
    }
};

CreatureAI* GetAI_trigger_vimgol_circle_bunny(Creature *pCreature)
{
    return new trigger_vimgol_circle_bunnyAI(pCreature);
}

/*######
## npc_vimgol
######*/

#define SPELL_SPAWN     7741
#define SPELL_GROWTH    40545       // FIXME: Not handled atm: it should interrupt itself if all players get back in circles during cast. Afaik, AI interface doesn't support such check for now.

struct npc_vimgolAI : public ScriptedAI
{
    npc_vimgolAI(Creature *c) : ScriptedAI(c) {}
    
    void Reset()
    {
        DoCast(m_creature, SPELL_SPAWN);
    }
    
    void Aggro(Unit *pWho) {}
    
    void JustDied(Unit *pKiller)
    {
        pKiller->SummonGameObject(185562, pKiller->GetPositionX(), pKiller->GetPositionY(), pKiller->GetPositionZ(), 0, 0, 0, 0, 0, 0);

        CellPair pair(Trinity::ComputeCellPair(m_creature->GetPositionX(), m_creature->GetPositionY()));
        Cell cell(pair);
        cell.data.Part.reserved = ALL_DISTRICT;
        cell.SetNoCreate();
        std::list<Creature*> triggers;

        Trinity::AllCreaturesOfEntryInRange check(m_creature, 23040, 50.0f);
        Trinity::CreatureListSearcher<Trinity::AllCreaturesOfEntryInRange> searcher(triggers, check);
        TypeContainerVisitor<Trinity::CreatureListSearcher<Trinity::AllCreaturesOfEntryInRange>, GridTypeMapContainer> visitor(searcher);

        cell.Visit(pair, visitor, *m_creature->GetMap());

        for (std::list<Creature*>::iterator itr = triggers.begin(); itr != triggers.end(); itr++)
            (*itr)->Kill(*itr);
    }
    
    void UpdateAI(uint32 const diff)
    {
        if (!UpdateVictim())
            return;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_vimgol(Creature *pCreature)
{
    return new npc_vimgolAI(pCreature);
}

/*######
## npc_skulloc
######*/

#define SPELL_SHADOWFORM_1      39943
#define SPELL_SHADOWFORM_2      39944
#define SPELL_SHADOWFORM_3      39946
#define SPELL_SHADOWFORM_4      39947

#define SPELL_VISUAL_INPROGRESS     39918       // At spawn
#define SPELL_VISUAL_BEAM           39920       // Blue/Black beam

#define GOB_SOUL                    185577

struct npc_skullocAI : public ScriptedAI
{
    npc_skullocAI(Creature *c) : ScriptedAI(c) {}
    
    uint8 step;
    
    uint32 step5Timer;
    
    void Reset()
    {
        step = 0;
        
        m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        m_creature->SetReactState(REACT_PASSIVE);
        m_creature->SetHasChangedReactState();
        
        step5Timer = 0;
    }
    
    void Aggro(Unit *pWho) {}
    
    void JustDied(Unit *pKiller)
    {
        pKiller->SummonGameObject(GOB_SOUL, pKiller->GetPositionX(), pKiller->GetPositionY(), pKiller->GetPositionZ()+3, 0, 0, 0, 0, 0, 0);
    }
    
    void ChangePhase(uint8 phase)
    {
        switch (phase) {
        case 0:
            DoCast(m_creature, SPELL_SHADOWFORM_1, true);
            break;
        case 1:
            m_creature->RemoveAurasDueToSpell(SPELL_SHADOWFORM_1);
            DoCast(m_creature, SPELL_SHADOWFORM_2, true);
            break;
        case 2:
            m_creature->RemoveAurasDueToSpell(SPELL_SHADOWFORM_2);
            DoCast(m_creature, SPELL_SHADOWFORM_3, true);
            break;
        case 3:
            m_creature->RemoveAurasDueToSpell(SPELL_SHADOWFORM_3);
            DoCast(m_creature, SPELL_SHADOWFORM_4, true);
            break;
        case 5:
        {
            // Remove all beams and cast one on the Soulgrinder
            CellPair pair(Trinity::ComputeCellPair(m_creature->GetPositionX(), m_creature->GetPositionY()));
            Cell cell(pair);
            cell.data.Part.reserved = ALL_DISTRICT;
            cell.SetNoCreate();
            std::list<Creature*> triggers;

            Trinity::AllCreaturesOfEntryInRange check(m_creature, 23037, 100.0f);
            Trinity::CreatureListSearcher<Trinity::AllCreaturesOfEntryInRange> searcher(triggers, check);
            TypeContainerVisitor<Trinity::CreatureListSearcher<Trinity::AllCreaturesOfEntryInRange>, GridTypeMapContainer> visitor(searcher);

            cell.Visit(pair, visitor, *m_creature->GetMap());

            for (std::list<Creature*>::iterator itr = triggers.begin(); itr != triggers.end(); itr++)
                (*itr)->Kill(*itr);
                
            if (Creature *soulgrinder = m_creature->FindNearestCreature(23019, 100.0f, true)) {
                DoCast(soulgrinder, SPELL_VISUAL_BEAM, true);
                step5Timer = 10000;
            }
            break;
        }
        case 6:
            if (Creature *soulgrinder = m_creature->FindNearestCreature(23019, 100.0f, true))
                m_creature->Kill(soulgrinder);
            m_creature->RemoveAurasDueToSpell(SPELL_SHADOWFORM_4);
            m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            m_creature->SetReactState(REACT_AGGRESSIVE);
        }
    }
    
    void UpdateAI(uint32 const diff)
    {
        if (step5Timer) {
            if (step5Timer <= diff) {
                ChangePhase(6);
                
                step5Timer = 0;
            }
            else
                step5Timer -= diff;
        }
        
        if (m_creature->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE))
            return;
            
        // Spells etc.
        
        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_skulloc(Creature *pCreature)
{
    return new npc_skullocAI(pCreature);
}

/*######
## npc_soulgrinder
######*/

float ogresSpawns[6][4] = { { 3524.897705, 5613.239258, -4.257625, 4.999918 },
                            { 3541.113037, 5606.105469, -3.300353, 4.327529 },
                            { 3543.969238, 5573.780273, -2.774169, 2.004281 },
                            { 3558.977295, 5590.019531, -4.172577, 3.183468 },
                            { 3528.284424, 5567.758789, -1.199220, 1.256619 },
                            { 3516.486328, 5582.583984, -3.198729, 0.579210 }};

#define SPELL_VISUAL_BUNNY          39936       // On bunny?

#define NPC_SKULLOC                 22910
#define NPC_SUNDERED_GHOST          24039       // Seems to have no visible displayID, probably need to cast spell 3991 on them and FIXME: Change faction!

struct npc_soulgrinderAI : public Scripted_NoMovementAI
{
    npc_soulgrinderAI(Creature *c) : Scripted_NoMovementAI(c) {}
    
    uint64 skullocGUID;
    uint32 summonTimer;
    uint32 ogresKilled;
    
    uint8 step;
    
    void Reset()
    {
        ogresKilled = 0;
        summonTimer = 8000;
        
        step = 0;
        
        DoCast(m_creature, SPELL_VISUAL_INPROGRESS, true);
        if (Creature *skulloc = m_creature->SummonCreature(NPC_SKULLOC, 3486.548340, 5554.811523, 7.519224, 3.755524, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 30000)) {
            skullocGUID = skulloc->GetGUID();
            ((npc_skullocAI*)skulloc->AI())->ChangePhase(step);
        }
    }
    
    void Aggro(Unit *pWho) {}       // FIXME: Put summoner and his group in combat and stop it when entering phase 5 -> Maybe not needed if ogres have enough hp
    
    void IncrementOgresCounter()
    {
        ogresKilled++;
        if (ogresKilled%6 == 0) {    // Increment step (and add a beam) every 6 ogres
            step++;
                
            AddBeam();
            if (Creature *skulloc = Unit::GetCreature(*m_creature, skullocGUID))
                ((npc_skullocAI*)skulloc->AI())->ChangePhase(step);
        }
    }
    
    void AddBeam()
    {
        CellPair pair(Trinity::ComputeCellPair(m_creature->GetPositionX(), m_creature->GetPositionY()));
        Cell cell(pair);
        cell.data.Part.reserved = ALL_DISTRICT;
        cell.SetNoCreate();
        std::list<Creature*> triggers;

        Trinity::AllCreaturesOfEntryInRange check(m_creature, 23037, 100.0f);
        Trinity::CreatureListSearcher<Trinity::AllCreaturesOfEntryInRange> searcher(triggers, check);
        TypeContainerVisitor<Trinity::CreatureListSearcher<Trinity::AllCreaturesOfEntryInRange>, GridTypeMapContainer> visitor(searcher);

        cell.Visit(pair, visitor, *m_creature->GetMap());

        for (std::list<Creature*>::iterator itr = triggers.begin(); itr != triggers.end(); itr++) {
            (*itr)->GetMotionMaster()->MoveTargetedHome();
            if (step == 1 && int32((*itr)->GetPositionX()) == 3493)
                (*itr)->CastSpell(m_creature, SPELL_VISUAL_BEAM, false);
            else if (step == 2 && int32((*itr)->GetPositionX()) == 3465)
                (*itr)->CastSpell(m_creature, SPELL_VISUAL_BEAM, false);
            else if (step == 3 && int32((*itr)->GetPositionX()) == 3515)
                (*itr)->CastSpell(m_creature, SPELL_VISUAL_BEAM, false);
            else if (step == 4 && int32((*itr)->GetPositionX()) == 3472)
                (*itr)->CastSpell(m_creature, SPELL_VISUAL_BEAM, false);
        }
    }
    
    void UpdateAI(uint32 const diff)
    {
        if (step >= 5)
            return;

        if (summonTimer <= diff) {
            uint8 randomPoint = rand()%6;
            m_creature->SummonCreature(NPC_SUNDERED_GHOST, ogresSpawns[randomPoint][0], ogresSpawns[randomPoint][1], ogresSpawns[randomPoint][2], ogresSpawns[randomPoint][3], TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 30000);
            
            summonTimer = 8000;
        }
        else
            summonTimer -= diff;
    }
};

CreatureAI* GetAI_npc_soulgrinder(Creature *pCreature)
{
    return new npc_soulgrinderAI(pCreature);
}

/*######
## npc_sundered_ghost
######*/

#define SPELL_TRANSFORM_GHOST       39916
#define SPELL_CRIPPLE               11443
#define SPELL_SHADOW_BOLT           20816

struct npc_sundered_ghostAI : public ScriptedAI
{
    npc_sundered_ghostAI(Creature *c) : ScriptedAI(c) {}
    
    uint32 crippleTimer;
    uint32 shadowBoltTimer;
    
    void Reset()
    {
        m_creature->setFaction(14);
        DoCast(m_creature, SPELL_TRANSFORM_GHOST);
        
        crippleTimer = 4000;
        shadowBoltTimer = 3000;
    }
    
    void Aggro(Unit *pWho) {}
    
    void JustDied(Unit *pKiller)
    {
        if (Creature *pSoulgrinder = m_creature->FindCreatureInGrid(23019, 40.0f, true))
            ((npc_soulgrinderAI*)pSoulgrinder->AI())->IncrementOgresCounter();
    }
    
    void UpdateAI(uint32 const diff)
    {
        if (!UpdateVictim())
            return;
            
        if (crippleTimer <= diff) {
            DoCast(m_creature->getVictim(), SPELL_CRIPPLE);
            
            crippleTimer = 20000+rand()%4000;
        }
        else
            crippleTimer -= diff;
            
        if (shadowBoltTimer <= diff) {
            DoCast(m_creature->getVictim(), SPELL_SHADOW_BOLT);
            
            shadowBoltTimer = 5000+rand()%3000;
        }
        else
            shadowBoltTimer -= diff;
        
        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_sundered_ghost(Creature *pCreature)
{
    return new npc_sundered_ghostAI(pCreature);
}

/*######
## go_apexis_relic
######*/

#define GOSSIP_START_GAME       "[PH] Démarrer le jeu !"
#define ITEM_APEXIS_SHARD       32569
#define GO_RELIC_DECHARGER      185894
#define NPC_SIMON_BUNNY         22923
//#define NPC_SIMON_SUMMONTARGET  23382     // Not used

bool GOHello_go_apexis_relic(Player* pPlayer, GameObject* pGo)
{
    //if (pPlayer->GetQuestStatus(11058) != QUEST_STATUS_INCOMPLETE && pPlayer->GetQuestStatus(11080) != QUEST_STATUS_INCOMPLETE)                      // Only with quest
        //return false;

    if (Creature *bunny = pGo->FindNearestCreature(NPC_SIMON_BUNNY, 5.0f, true))        // Event is running, don't launch it twice
        return false;

    if (pPlayer->HasItemCount(ITEM_APEXIS_SHARD, 1, false)) {
        pPlayer->ADD_GOSSIP_ITEM(0, GOSSIP_START_GAME, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
        pPlayer->SEND_GOSSIP_MENU(31000, pGo->GetGUID());
        
        return true;
    }
    
    return false;
}

bool GOSelect_go_apexis_relic(Player* pPlayer, GameObject* pGO, uint32 uiSender, uint32 uiAction)
{
    if (uiAction == GOSSIP_ACTION_INFO_DEF) {
        // Summon trigger that will handle the event
        if (GameObject *gDecharger = pPlayer->FindNearestGameObject(GO_RELIC_DECHARGER, 5.0f))
            pPlayer->SummonCreature(NPC_SIMON_BUNNY, gDecharger->GetPositionX(), gDecharger->GetPositionY(), gDecharger->GetPositionZ()+5, pPlayer->GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN, 0);
    }
    
    pPlayer->CLOSE_GOSSIP_MENU();
    
    return true;
}

/*######
## npc_simon_bunny
######*/

enum eSimonSpells
{
    //SPELL_START_SOLO        = 41145,  // Not used
    //SPELL_START_GROUP       = 41146,  // Not used
    SPELL_START_VISUAL      = 40387,
    //SPELL_START_VISUAL_HIGH = 39993,  // Not used
    
    SPELL_WHITEAURA_BLUE    = 40281,
    SPELL_WHITEAURA_GREEN   = 40287,
    SPELL_WHITEAURA_RED     = 40288,
    SPELL_WHITEAURA_YELLOW  = 40289,
    
    SPELL_BEAM_BLUE         = 40244,
    SPELL_BEAM_GREEN        = 40245,
    SPELL_BEAM_RED          = 40246,
    SPELL_BEAM_YELLOW       = 40247,
    
    SPELL_LEVELSTART        = 40436,
    //SPELL_SOUND             = 41398,  // Not used
    
    SPELL_BAD_PRESS         = 41241,
    SPELL_GAME_FAILED       = 40437,
    
    SPELL_INTROSPECTION     = 40165,
    
    SPELL_APEXIS_VIBRATIONS = 40310
};

enum eSimonSteps
{
    STEP_BEGIN              = 0,
    STEP_WHITEAURA          = 1,
    STEP_SHOWSEQUENCE       = 2,
    STEP_REPRODSEQUENCE     = 3
};

enum eSimonBeams
{
    BEAM_BLUE               = 0,
    BEAM_GREEN              = 1,
    BEAM_RED                = 2,
    BEAM_YELLOW             = 3
};

enum eSimonSounds
{
    SOUND_BLUE              = 11588,
    SOUND_GREEN             = 11589,
    SOUND_RED               = 11590,
    SOUND_YELLOW            = 11591
};

struct npc_simon_bunnyAI : public ScriptedAI
{
    npc_simon_bunnyAI(Creature *c) : ScriptedAI(c) {}
    
    uint64 playerGUID;
    
    uint8 step;
    uint8 level;
    uint8 levelCounter;
    
    uint32 stepTimer;
    uint32 inactiveTimer;
    
    std::list<uint8> beamList;
    
    void Reset()
    {
        m_creature->GetMotionMaster()->MoveTargetedHome();
        
        if (Unit *summoner = ((TemporarySummon*)m_creature)->GetSummoner()) {
            summoner->ToPlayer()->DestroyItemCount(ITEM_APEXIS_SHARD, 1, true);
            playerGUID = summoner->GetGUID();
        }
            
        if (!playerGUID) {
            sLog.outError("Simon Game: no player found to start event, aborting.");
            m_creature->DisappearAndDie();
        }
        
        step = STEP_BEGIN;
        stepTimer = 0;
        level = 0;
        inactiveTimer = 0;
        
        beamList.clear();
    }
    
    // This is hacky. I need to move npc to get him back in the air, or visual effect is fucked up.
    void JustReachedHome()
    {
        stepTimer = 1;
    }
    
    void Aggro(Unit *pWho) {}
    
    uint32 SelectRandomBeam()
    {
        Player *plr = objmgr.GetPlayer(playerGUID);
        if (!plr)
            return 0;

        switch (rand()%4) {
        case BEAM_BLUE:
            beamList.push_back(BEAM_BLUE);
            plr->PlaySound(GetSoundForButton(BEAM_BLUE), false);
            return SPELL_BEAM_BLUE;
        case BEAM_GREEN:
            beamList.push_back(BEAM_GREEN);
            plr->PlaySound(GetSoundForButton(BEAM_GREEN), false);
            return SPELL_BEAM_GREEN;
        case BEAM_RED:
            beamList.push_back(BEAM_RED);
            plr->PlaySound(GetSoundForButton(BEAM_RED), false);
            return SPELL_BEAM_RED;
        case BEAM_YELLOW:
            beamList.push_back(BEAM_YELLOW);
            plr->PlaySound(GetSoundForButton(BEAM_YELLOW), false);
            return SPELL_BEAM_YELLOW;
        }
    }
    
    uint32 GetSpellForBeam(uint8 beam)
    {
        switch (beam) {
        case BEAM_BLUE:
            return SPELL_BEAM_BLUE;
        case BEAM_GREEN:
            return SPELL_BEAM_GREEN;
        case BEAM_RED:
            return SPELL_BEAM_RED;
        case BEAM_YELLOW:
            return SPELL_BEAM_YELLOW;
        }
    }

    uint32 GetSoundForButton(uint8 button)
    {
        switch (button) {
        case BEAM_BLUE:
            return SOUND_BLUE;
        case BEAM_GREEN:
            return SOUND_GREEN;
        case BEAM_RED:
            return SOUND_RED;
        case BEAM_YELLOW:
            return SOUND_YELLOW;
        }
    }

    void PlayerProposal(uint8 prop)
    {
        if (step != STEP_REPRODSEQUENCE)
            return;

        uint8 next = beamList.front();
        if (next == prop) {     // Good
            DoCast(m_creature, GetSpellForBeam(next), true);
            if (Player *plr = objmgr.GetPlayer(playerGUID))
                plr->PlaySound(GetSoundForButton(next), false);
            
            beamList.pop_front();
            if (beamList.empty()) {     // All beams processed. Level up!
                step = STEP_BEGIN;
                stepTimer = 500;
            }
        }
        else {                  // Wrong, hurt player and restart level
            if (Player *plr = objmgr.GetPlayer(playerGUID)) {
                plr->CastSpell(plr, SPELL_GAME_FAILED, true);
                plr->CastSpell(plr, SPELL_BAD_PRESS, true);
            }
            
            beamList.clear();
            level--;
            
            step = STEP_BEGIN;
            stepTimer = 500;
        }
    }
    
    void UpdateAI(uint32 const diff)
    {
        Player *summoner = objmgr.GetPlayer(playerGUID);
        if (!summoner || !summoner->IsInWorld()) {
            m_creature->DisappearAndDie();
            return;
        }
        
        if (level > 8) {        // Complete quest and stop event
            //DoCast(summoner, SPELL_APEXIS_VIBRATIONS, true);
            summoner->CastSpell(summoner, SPELL_APEXIS_VIBRATIONS, false);
            if (summoner->ToPlayer()->GetQuestStatus(11058) == QUEST_STATUS_INCOMPLETE) {
                summoner->ToPlayer()->CompleteQuest(11058);
                summoner->ToPlayer()->GroupEventHappens(11058, m_creature);
            }
            if (summoner->ToPlayer()->GetQuestStatus(11080) == QUEST_STATUS_INCOMPLETE) {
                summoner->ToPlayer()->CompleteQuest(11080);
                summoner->ToPlayer()->GroupEventHappens(11080, m_creature);
            }
            m_creature->DisappearAndDie();
            return;
        }
        
        if (stepTimer) {
            if (stepTimer <= diff) {
                switch (step) {
                case STEP_BEGIN:
                    DoCast(m_creature, SPELL_START_VISUAL, true);
                    step = STEP_WHITEAURA;
                    stepTimer = 1800;
                    break;
                case STEP_WHITEAURA:
                    DoCast(m_creature, SPELL_WHITEAURA_BLUE, true);
                    DoCast(m_creature, SPELL_WHITEAURA_GREEN, true);
                    DoCast(m_creature, SPELL_WHITEAURA_RED, true);
                    DoCast(m_creature, SPELL_WHITEAURA_YELLOW, true);
                    
                    beamList.clear();
                    step = STEP_SHOWSEQUENCE;
                    stepTimer = 5000;
                    level++;
                    levelCounter = 0;
                    break;
                case STEP_SHOWSEQUENCE:
                    if (levelCounter < level) {
                        DoCast(m_creature, SelectRandomBeam(), true);
                        levelCounter++;
                        stepTimer = 800;
                        break;
                    }
                    step = STEP_REPRODSEQUENCE;
                    stepTimer = 500;
                    break;
                case STEP_REPRODSEQUENCE:
                    DoCast(m_creature, SPELL_LEVELSTART, true);
                    stepTimer = 0;      // Stop event, we are waiting for player
                    inactiveTimer = 0;
                    break;
                default:
                    sLog.outError("Phase not handled for now.");
                    break;
                }
            }
            else stepTimer -= diff;
        }
        else
            inactiveTimer += diff;
            
        if (inactiveTimer >= 300000)    // 5 min
            m_creature->DisappearAndDie();
    }
};

CreatureAI* GetAI_npc_simon_bunny(Creature *pCreature)
{
    return new npc_simon_bunnyAI(pCreature);
}

/*######
## go_blue_cluster
######*/

bool GOHello_go_blue_cluster(Player *pPlayer, GameObject *pGo)
{
    Creature *bunny = pGo->FindNearestCreature(NPC_SIMON_BUNNY, 5.0f, true);
    if (!bunny)
        return false;
        
    pPlayer->CastSpell(pPlayer, SPELL_INTROSPECTION, true);
    ((npc_simon_bunnyAI*)bunny->AI())->PlayerProposal(BEAM_BLUE);
    
    return true;
}

/*######
## go_green_cluster
######*/

bool GOHello_go_green_cluster(Player *pPlayer, GameObject *pGo)
{
    Creature *bunny = pGo->FindNearestCreature(NPC_SIMON_BUNNY, 5.0f, true);
    if (!bunny)
        return false;
        
    pPlayer->CastSpell(pPlayer, SPELL_INTROSPECTION, true);
    ((npc_simon_bunnyAI*)bunny->AI())->PlayerProposal(BEAM_GREEN);
    
    return true;
}

/*######
## go_red_cluster
######*/

bool GOHello_go_red_cluster(Player *pPlayer, GameObject *pGo)
{
    Creature *bunny = pGo->FindNearestCreature(NPC_SIMON_BUNNY, 5.0f, true);
    if (!bunny)
        return false;
        
    pPlayer->CastSpell(pPlayer, SPELL_INTROSPECTION, true);
    ((npc_simon_bunnyAI*)bunny->AI())->PlayerProposal(BEAM_RED);
    
    return true;
}

/*######
## go_yellow_cluster
######*/

bool GOHello_go_yellow_cluster(Player *pPlayer, GameObject *pGo)
{
    Creature *bunny = pGo->FindNearestCreature(NPC_SIMON_BUNNY, 5.0f, true);
    if (!bunny)
        return false;
        
    pPlayer->CastSpell(pPlayer, SPELL_INTROSPECTION, true);
    ((npc_simon_bunnyAI*)bunny->AI())->PlayerProposal(BEAM_YELLOW);
    
    return true;
}

/*######
## go_apexis_monument
######*/

//#define GOSSIP_START_GAME       "[PH] Démarrer le jeu !"
#define ITEM_APEXIS_SHARD       32569
#define GO_APEXIS_DECHARGER     185945
#define NPC_SIMON_BUNNY_LARGE   23378

bool GOHello_go_apexis_monument(Player* pPlayer, GameObject* pGo)
{
    //if (pPlayer->GetQuestStatus(11059) != QUEST_STATUS_INCOMPLETE)                      // Only with quest
        //return false;

    if (Creature *bunny = pGo->FindNearestCreature(NPC_SIMON_BUNNY_LARGE, 5.0f, true))        // Event is running, don't launch it twice
        return false;

    if (pPlayer->HasItemCount(ITEM_APEXIS_SHARD, 35, false)) {
        pPlayer->ADD_GOSSIP_ITEM(0, GOSSIP_START_GAME, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
        pPlayer->SEND_GOSSIP_MENU(31000, pGo->GetGUID());
        
        return true;
    }
    
    return false;
}

bool GOSelect_go_apexis_monument(Player* pPlayer, GameObject* pGO, uint32 uiSender, uint32 uiAction)
{
    if (uiAction == GOSSIP_ACTION_INFO_DEF) {
        // Summon trigger that will handle the event
        if (GameObject *gDecharger = pPlayer->FindNearestGameObject(GO_APEXIS_DECHARGER, 5.0f))
            pPlayer->SummonCreature(NPC_SIMON_BUNNY_LARGE, gDecharger->GetPositionX(), gDecharger->GetPositionY(), gDecharger->GetPositionZ()+8, pPlayer->GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN, 0);
    }
    
    pPlayer->CLOSE_GOSSIP_MENU();
    
    return true;
}

/*######
## npc_simon_bunny_large
######*/

/*enum eSimonSpells
{
    //SPELL_START_SOLO        = 41145,  // Not used
    //SPELL_START_GROUP       = 41146,  // Not used
    SPELL_START_VISUAL      = 40387,
    //SPELL_START_VISUAL_HIGH = 39993,  // Not used
    
    SPELL_WHITEAURA_BLUE    = 40281,
    SPELL_WHITEAURA_GREEN   = 40287,
    SPELL_WHITEAURA_RED     = 40288,
    SPELL_WHITEAURA_YELLOW  = 40289,
    
    SPELL_BEAM_BLUE         = 40244,
    SPELL_BEAM_GREEN        = 40245,
    SPELL_BEAM_RED          = 40246,
    SPELL_BEAM_YELLOW       = 40247,
    
    SPELL_LEVELSTART        = 40436,
    //SPELL_SOUND             = 41398,  // Not used
    
    SPELL_BAD_PRESS         = 41241,
    SPELL_GAME_FAILED       = 40437,
    
    SPELL_INTROSPECTION     = 40165,
    
    SPELL_APEXIS_VIBRATIONS = 40310
};*/

/*enum eSimonSteps
{
    STEP_BEGIN              = 0,
    STEP_WHITEAURA          = 1,
    STEP_SHOWSEQUENCE       = 2,
    STEP_REPRODSEQUENCE     = 3
};

enum eSimonBeams
{
    BEAM_BLUE               = 0,
    BEAM_GREEN              = 1,
    BEAM_RED                = 2,
    BEAM_YELLOW             = 3
};

enum eSimonSounds
{
    SOUND_BLUE              = 11588,
    SOUND_GREEN             = 11589,
    SOUND_RED               = 11590,
    SOUND_YELLOW            = 11591
};*/

struct npc_simon_bunny_largeAI : public ScriptedAI
{
    npc_simon_bunny_largeAI(Creature *c) : ScriptedAI(c) {}
    
    uint64 playerGUID;
    
    uint8 step;
    uint8 level;
    uint8 levelCounter;
    
    uint32 stepTimer;
    uint32 inactiveTimer;
    
    std::list<uint8> beamList;
    
    void Reset()
    {
        m_creature->GetMotionMaster()->MoveTargetedHome();
        
        if (Unit *summoner = ((TemporarySummon*)m_creature)->GetSummoner()) {
            summoner->ToPlayer()->DestroyItemCount(ITEM_APEXIS_SHARD, 35, true);
            playerGUID = summoner->GetGUID();
        }
            
        if (!playerGUID) {
            sLog.outError("Simon Game: no player found to start event, aborting.");
            m_creature->DisappearAndDie();
        }
        
        step = STEP_BEGIN;
        stepTimer = 0;
        level = 0;
        inactiveTimer = 0;
        
        beamList.clear();
    }
    
    // This is hacky. I need to move npc to get him back in the air, or visual effect is fucked up.
    void JustReachedHome()
    {
        stepTimer = 1;
    }
    
    void Aggro(Unit *pWho) {}
    
    uint32 SelectRandomBeam()
    {
        Player *plr = objmgr.GetPlayer(playerGUID);
        if (!plr)
            return 0;

        switch (rand()%4) {
        case BEAM_BLUE:
            beamList.push_back(BEAM_BLUE);
            plr->PlaySound(GetSoundForButton(BEAM_BLUE), false);
            return SPELL_BEAM_BLUE;
        case BEAM_GREEN:
            beamList.push_back(BEAM_GREEN);
            plr->PlaySound(GetSoundForButton(BEAM_GREEN), false);
            return SPELL_BEAM_GREEN;
        case BEAM_RED:
            beamList.push_back(BEAM_RED);
            plr->PlaySound(GetSoundForButton(BEAM_RED), false);
            return SPELL_BEAM_RED;
        case BEAM_YELLOW:
            beamList.push_back(BEAM_YELLOW);
            plr->PlaySound(GetSoundForButton(BEAM_YELLOW), false);
            return SPELL_BEAM_YELLOW;
        }
    }
    
    uint32 GetSpellForBeam(uint8 beam)
    {
        switch (beam) {
        case BEAM_BLUE:
            return SPELL_BEAM_BLUE;
        case BEAM_GREEN:
            return SPELL_BEAM_GREEN;
        case BEAM_RED:
            return SPELL_BEAM_RED;
        case BEAM_YELLOW:
            return SPELL_BEAM_YELLOW;
        }
    }

    uint32 GetSoundForButton(uint8 button)
    {
        switch (button) {
        case BEAM_BLUE:
            return SOUND_BLUE;
        case BEAM_GREEN:
            return SOUND_GREEN;
        case BEAM_RED:
            return SOUND_RED;
        case BEAM_YELLOW:
            return SOUND_YELLOW;
        }
    }

    void PlayerProposal(uint8 prop)
    {
        if (step != STEP_REPRODSEQUENCE)
            return;

        uint8 next = beamList.front();
        if (next == prop) {     // Good
            DoCast(m_creature, GetSpellForBeam(next), true);
            if (Player *plr = objmgr.GetPlayer(playerGUID))
                plr->PlaySound(GetSoundForButton(next), false);
            
            beamList.pop_front();
            if (beamList.empty()) {     // All beams processed. Level up!
                step = STEP_BEGIN;
                stepTimer = 500;
            }
        }
        else {                  // Wrong, hurt player and restart level
            if (Player *plr = objmgr.GetPlayer(playerGUID)) {
                plr->CastSpell(plr, SPELL_GAME_FAILED, true);
                plr->CastSpell(plr, SPELL_BAD_PRESS, true);
            }
            
            beamList.clear();
            level--;
            
            step = STEP_BEGIN;
            stepTimer = 500;
        }
    }
    
    void UpdateAI(uint32 const diff)
    {
        Player *summoner = objmgr.GetPlayer(playerGUID);
        if (!summoner || !summoner->IsInWorld()) {
            m_creature->DisappearAndDie();
            return;
        }
        
        if (level > 6) {        // Complete quest and stop event
            //DoCast(summoner, SPELL_APEXIS_VIBRATIONS, true);
            //summoner->ToPlayer()->GroupEventHappens(11059, m_creature);
            m_creature->SummonCreature(22275, m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ()-8, 0, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 30000);
            m_creature->DisappearAndDie();
            return;
        }
        
        if (stepTimer) {
            if (stepTimer <= diff) {
                switch (step) {
                case STEP_BEGIN:
                    DoCast(m_creature, SPELL_START_VISUAL, true);
                    step = STEP_WHITEAURA;
                    stepTimer = 1800;
                    break;
                case STEP_WHITEAURA:
                    DoCast(m_creature, SPELL_WHITEAURA_BLUE, true);
                    DoCast(m_creature, SPELL_WHITEAURA_GREEN, true);
                    DoCast(m_creature, SPELL_WHITEAURA_RED, true);
                    DoCast(m_creature, SPELL_WHITEAURA_YELLOW, true);
                    
                    beamList.clear();
                    step = STEP_SHOWSEQUENCE;
                    stepTimer = 5000;
                    level++;
                    levelCounter = 0;
                    break;
                case STEP_SHOWSEQUENCE:
                    if (levelCounter < level) {
                        DoCast(m_creature, SelectRandomBeam(), true);
                        levelCounter++;
                        stepTimer = 800;
                        break;
                    }
                    step = STEP_REPRODSEQUENCE;
                    stepTimer = 500;
                    break;
                case STEP_REPRODSEQUENCE:
                    DoCast(m_creature, SPELL_LEVELSTART, true);
                    stepTimer = 0;      // Stop event, we are waiting for player
                    inactiveTimer = 0;
                    break;
                default:
                    sLog.outError("Phase not handled for now.");
                    break;
                }
            }
            else stepTimer -= diff;
        }
        else
            inactiveTimer += diff;
            
        if (inactiveTimer >= 300000)        // 5 min
            m_creature->DisappearAndDie();
    }
};

CreatureAI* GetAI_npc_simon_bunny_large(Creature *pCreature)
{
    return new npc_simon_bunny_largeAI(pCreature);
}

/*######
## go_blue_cluster_large
######*/

bool GOHello_go_blue_cluster_large(Player *pPlayer, GameObject *pGo)
{
    Creature *bunny = pGo->FindNearestCreature(NPC_SIMON_BUNNY_LARGE, 10.0f, true);
    if (!bunny)
        return false;
        
    pPlayer->CastSpell(pPlayer, SPELL_INTROSPECTION, true);
    ((npc_simon_bunny_largeAI*)bunny->AI())->PlayerProposal(BEAM_BLUE);
    
    return true;
}

/*######
## go_green_cluster_large
######*/

bool GOHello_go_green_cluster_large(Player *pPlayer, GameObject *pGo)
{
    Creature *bunny = pGo->FindNearestCreature(NPC_SIMON_BUNNY_LARGE, 10.0f, true);
    if (!bunny)
        return false;
        
    pPlayer->CastSpell(pPlayer, SPELL_INTROSPECTION, true);
    ((npc_simon_bunny_largeAI*)bunny->AI())->PlayerProposal(BEAM_GREEN);
    
    return true;
}

/*######
## go_red_cluster_large
######*/

bool GOHello_go_red_cluster_large(Player *pPlayer, GameObject *pGo)
{
    Creature *bunny = pGo->FindNearestCreature(NPC_SIMON_BUNNY_LARGE, 10.0f, true);
    if (!bunny)
        return false;
        
    pPlayer->CastSpell(pPlayer, SPELL_INTROSPECTION, true);
    ((npc_simon_bunny_largeAI*)bunny->AI())->PlayerProposal(BEAM_RED);
    
    return true;
}

/*######
## go_yellow_cluster_large
######*/

bool GOHello_go_yellow_cluster_large(Player *pPlayer, GameObject *pGo)
{
    Creature *bunny = pGo->FindNearestCreature(NPC_SIMON_BUNNY_LARGE, 10.0f, true);
    if (!bunny)
        return false;
        
    pPlayer->CastSpell(pPlayer, SPELL_INTROSPECTION, true);
    ((npc_simon_bunny_largeAI*)bunny->AI())->PlayerProposal(BEAM_YELLOW);
    
    return true;
}

/*######
## go_fel_crystal_prism
######*/

bool GOHello_go_fel_crystal_prism(Player* pPlayer, GameObject *pGo)
{
    if (!pPlayer->HasItemCount(ITEM_APEXIS_SHARD, 35, false))
        return false;
        
    pPlayer->DestroyItemCount(ITEM_APEXIS_SHARD, 35, true);
        
    switch (rand()%4) {
    case 0:
        pGo->SummonCreature(23353, pPlayer->GetPositionX(), pPlayer->GetPositionY(), pPlayer->GetPositionZ(), pPlayer->GetOrientation(), TEMPSUMMON_CORPSE_TIMED_DESPAWN, 30000);
        break;
    case 1:
        pGo->SummonCreature(23354, pPlayer->GetPositionX(), pPlayer->GetPositionY(), pPlayer->GetPositionZ(), pPlayer->GetOrientation(), TEMPSUMMON_CORPSE_TIMED_DESPAWN, 30000);
        break;
    case 2:
        pGo->SummonCreature(22281, pPlayer->GetPositionX(), pPlayer->GetPositionY(), pPlayer->GetPositionZ(), pPlayer->GetOrientation(), TEMPSUMMON_CORPSE_TIMED_DESPAWN, 30000);
        break;
    case 3:
        pGo->SummonCreature(23355, pPlayer->GetPositionX(), pPlayer->GetPositionY(), pPlayer->GetPositionZ(), pPlayer->GetOrientation(), TEMPSUMMON_CORPSE_TIMED_DESPAWN, 30000);
        break;
    }
    
    return true;
}

/*######
## npc_braxxus
######*/

#define SPELL_DOUBLE_BREATH         41437
#define SPELL_MANGLE                41439
#define SPELL_PANIC                 41436

struct npc_braxxusAI : public ScriptedAI
{
    npc_braxxusAI(Creature *c) : ScriptedAI(c) {}
    
    uint32 doubleBreathTimer;
    uint32 mangleTimer;
    uint32 panicTimer;
    
    void Reset()
    {
        doubleBreathTimer = 2000;
        mangleTimer = 5000;
        panicTimer = 15000;
    }
    
    void Aggro(Unit *pWho) {}
    
    void UpdateAI(uint32 const diff)
    {
        if (!UpdateVictim())
            return;
            
        if (doubleBreathTimer <= diff) {
            DoCast(m_creature->getVictim(), SPELL_DOUBLE_BREATH);
            doubleBreathTimer = 10000+rand()%3000;
        }
        else
            doubleBreathTimer -= diff;
            
        if (mangleTimer <= diff) {
            DoCast(SelectUnit(SELECT_TARGET_RANDOM, 0), SPELL_MANGLE);
            mangleTimer = 10000+rand()%4000;
        }
        else
            mangleTimer -= diff;
            
        if (panicTimer <= diff) {
            DoCast(m_creature->getVictim(), SPELL_PANIC);
            panicTimer = 15000+rand()%3000;
        }
        else
            panicTimer -= diff;
            
        DoMeleeAttackIfReady();
    }
};

CreatureAI *GetAI_npc_braxxus(Creature *pCreature)
{
    return new npc_braxxusAI(pCreature);
}

/*######
## npc_moarg_incinerator
######*/

#define SPELL_ACID_GEYSER       44431
#define SPELL_MIGHTY_CHARGE     36606
#define SPELL_SUNDERING_CLEAVE  25174

struct npc_moarg_incineratorAI : public ScriptedAI
{
    npc_moarg_incineratorAI(Creature *c) : ScriptedAI(c) {}
    
    uint32 acidGeyserTimer;
    uint32 mightyChargeTimer;
    uint32 sunderingCleaveTimer;
    
    void Reset()
    {
        acidGeyserTimer = 2000;
        sunderingCleaveTimer = 5000;
    }
    
    void Aggro(Unit *pWho)
    {
        DoCast(pWho, SPELL_MIGHTY_CHARGE);
        mightyChargeTimer = 10000;
    }
    
    void UpdateAI(uint32 const diff)
    {
        if (!UpdateVictim())
            return;
            
        if (acidGeyserTimer <= diff) {
            DoCast(m_creature->getVictim(), SPELL_ACID_GEYSER);
            acidGeyserTimer = 10000+rand()%3000;
        }
        else
            acidGeyserTimer -= diff;
            
        if (sunderingCleaveTimer <= diff) {
            DoCast(m_creature->getVictim(), SPELL_SUNDERING_CLEAVE);
            sunderingCleaveTimer = 5000+rand()%2000;
        }
        else
            sunderingCleaveTimer -= diff;
            
        if (mightyChargeTimer <= diff) {
            DoCast(SelectUnit(SELECT_TARGET_RANDOM, 1), SPELL_MIGHTY_CHARGE);
            mightyChargeTimer = 10000+rand()%3000;
        }
        else
            mightyChargeTimer -= diff;
            
        DoMeleeAttackIfReady();
    }
};

CreatureAI *GetAI_npc_moarg_incinerator(Creature *pCreature)
{
    return new npc_moarg_incineratorAI(pCreature);
}

/*######
## npc_galvanoth
######*/

#define SPELL_FEL_FLAMESTRIKE       39139
#define SPELL_MORTAL_STRIKE         15708
#define SPELL_WAR_STOMP             38750

struct npc_galvanothAI : public ScriptedAI
{
    npc_galvanothAI(Creature *c) : ScriptedAI(c) {}
    
    uint32 felFlamestrikeTimer;
    uint32 mortalStrikeTimer;
    uint32 warStompTimer;
    
    void Reset()
    {
        felFlamestrikeTimer = 2000;
        mortalStrikeTimer = 5000;
        warStompTimer = 8000;
    }
    
    void Aggro(Unit *pWho) {}
    
    void UpdateAI(uint32 const diff)
    {
        if (!UpdateVictim())
            return;
            
        if (felFlamestrikeTimer <= diff) {
            DoCast(m_creature->getVictim(), SPELL_FEL_FLAMESTRIKE);
            felFlamestrikeTimer = 10000+rand()%3000;
        }
        else
            felFlamestrikeTimer -= diff;
            
        if (mortalStrikeTimer <= diff) {
            DoCast(m_creature->getVictim(), SPELL_MORTAL_STRIKE);
            mortalStrikeTimer = 6000+rand()%2000;
        }
        else
            mortalStrikeTimer -= diff;
            
        if (warStompTimer <= diff) {
            DoCast(m_creature->getVictim(), SPELL_WAR_STOMP);
            warStompTimer = 10000+rand()%3000;
        }
        else
            warStompTimer -= diff;
            
        DoMeleeAttackIfReady();
    }
};

CreatureAI *GetAI_npc_galvanoth(Creature *pCreature)
{
    return new npc_galvanothAI(pCreature);
}

/*######
## npc_zarcsin
######*/

#define SPELL_FEL_FLAMES        41444
#define SPELL_ENRAGE            41447

struct npc_zarcsinAI : public ScriptedAI
{
    npc_zarcsinAI(Creature *c) : ScriptedAI(c) {}
    
    uint32 felFlamesTimer;
    
    void Reset()
    {
        felFlamesTimer = 2000;
    }
    
    void Aggro(Unit *pWho) {}
    
    void UpdateAI(uint32 const diff)
    {
        if (!UpdateVictim())
            return;
            
        if (felFlamesTimer <= diff) {
            DoCast(m_creature->getVictim(), SPELL_FEL_FLAMES);
            felFlamesTimer = 10000+rand()%3000;
        }
        else
            felFlamesTimer -= diff;
            
        if (m_creature->IsBelowHPPercent(50) && !m_creature->HasAura(SPELL_ENRAGE))
            DoCast(m_creature, SPELL_ENRAGE);
            
        DoMeleeAttackIfReady();
    }
};

CreatureAI *GetAI_npc_zarcsin(Creature *pCreature)
{
    return new npc_zarcsinAI(pCreature);
}

/*######
## npc_aether_ray
######*/

#define SPELL_TAIL_SWIPE        35333

struct npc_aether_rayAI : public ScriptedAI
{
    npc_aether_rayAI(Creature *c) : ScriptedAI(c) {}
    
    uint32 tailSwipeTimer;
    
    bool hasEmoted;
    
    void Reset()
    {
        m_creature->GetMotionMaster()->MoveTargetedHome();
        
        tailSwipeTimer = 2000;
        hasEmoted = false;
    }
    
    void Aggro(Unit *pWho) {}
    
    void SpellHit(Unit *pCaster, SpellEntry const *pSpell)
    {
        if (pSpell->Id == 40856 && m_creature->IsBelowHPPercent(30)) {
            m_creature->DisappearAndDie();
            pCaster->CastSpell(pCaster, 40917, false);
        }
    }
    
    void UpdateAI(uint32 const diff)
    {
        if (!UpdateVictim())
            return;
            
        if (m_creature->IsBelowHPPercent(30) && !hasEmoted) {
            DoTextEmote("est prête à être capturée.", NULL);
            hasEmoted = true;
        }
            
        if (tailSwipeTimer <= diff) {
            DoCast(m_creature->getVictim(), SPELL_TAIL_SWIPE);
            tailSwipeTimer = 10000+rand()%5000;
        }
        else
            tailSwipeTimer -= diff;
        
        DoMeleeAttackIfReady();
    }
};

CreatureAI *GetAI_npc_aether_ray(Creature *pCreature)
{
    return new npc_aether_rayAI(pCreature);
}

/*######
## npc_wrangled_aether_ray
######*/

struct npc_wrangled_aether_rayAI : public ScriptedAI
{
    npc_wrangled_aether_rayAI(Creature *c) : ScriptedAI(c) {}
    
    void Reset()
    {
        if (Unit *summoner = ((TemporarySummon*)m_creature)->GetSummoner()) {
            m_creature->GetMotionMaster()->MoveFollow(summoner, PET_FOLLOW_DIST*2, M_PI);
            DoCast(summoner, 40926, true);
        }
    }
    
    void Aggro(Unit *pWho) {}
    
    void MoveInLineOfSight(Unit *pWho)
    {
        if (pWho->ToCreature() && pWho->ToCreature()->GetEntry() == 23335 && pWho->IsWithinDistInMap(m_creature, 10.0f)) {
            if (Unit *summoner = ((TemporarySummon*)m_creature)->GetSummoner()) {
                summoner->ToPlayer()->KilledMonster(m_creature->GetEntry(), m_creature->GetGUID());
                m_creature->DisappearAndDie();
            }
        }
    }
};

CreatureAI *GetAI_npc_wrangled_aether_ray(Creature *pCreature)
{
    return new npc_wrangled_aether_rayAI(pCreature);
}

/*######
## go_drake_egg
######*/

bool GOHello_go_drake_egg(Player *pPlayer, GameObject *pGo)
{
    //if (pPlayer->GetQuestStatus(11078) != QUEST_STATUS_INCOMPLETE)
        //return false;
        
    if (!pPlayer->HasItemCount(ITEM_APEXIS_SHARD, 35, false))
        return false;
        
    pPlayer->DestroyItemCount(ITEM_APEXIS_SHARD, 35, true);
        
    uint32 bossEntry;
    switch (pGo->GetEntry()) {
    case 185936:        // Rivendark
        bossEntry = 23061;
        break;
    case 185932:        // Obsidia
        bossEntry = 23282;
        break;
    case 185938:        // Insidion
        bossEntry = 23281;
        break;
    case 185937:        // Furywing
        bossEntry = 23261;
        break;
    default:
        bossEntry = 0;
        break;
    }
    
    if (Creature *boss = pPlayer->FindCreatureInGrid(bossEntry, 150.0f, true))
        boss->AI()->AttackStart(pPlayer);
        
    return true;
}

/*######
## npc_rivendark
######*/

#define SPELL_CORRUPTION        41988
#define SPELL_TAIL_SWEEP        15847
#define SPELL_BELLOWING_ROAR    36922
#define SPELL_CLEAVE            40505
#define SPELL_FIERY_BREATH      40032
#define SPELL_FLAME_BREATH      9573

struct npc_rivendarkAI : public ScriptedAI
{
    npc_rivendarkAI(Creature *c) : ScriptedAI(c) {}
    
    uint32 corruptionTimer;
    uint32 tailSweepTimer;
    uint32 bellowingRoarTimer;
    uint32 cleaveTimer;
    uint32 fieryBreathTimer;
    uint32 flameBreathTimer;
    
    void Reset()
    {
        corruptionTimer = 2000;
        tailSweepTimer = 10000;
        bellowingRoarTimer = 15000;
        cleaveTimer = 5000;
        fieryBreathTimer = 8000;
        flameBreathTimer = 12000;
    }
    
    void Aggro(Unit *pWho) {}
    
    void UpdateAI(uint32 const diff)
    {
        if (!m_creature->isInCombat())
            m_creature->SetDisableGravity(true);
        else if (m_creature->isInCombat())
        	m_creature->SetDisableGravity(false);
            
        if (!UpdateVictim())
            return;
            
        if (corruptionTimer <= diff) {
            DoCast(SelectUnit(SELECT_TARGET_RANDOM, 0), SPELL_CORRUPTION);
            corruptionTimer = 5000+rand()%3000;
        }
        else
            corruptionTimer -= diff;
            
        if (tailSweepTimer <= diff) {
            Unit *target = SelectUnit(SELECT_TARGET_RANDOM, 1);
            if (target && !m_creature->HasInArc(M_PI, target))
                DoCast(target, SPELL_TAIL_SWEEP);
            tailSweepTimer = 4000+rand()%3000;
        }
        else
            tailSweepTimer -= diff;
            
        if (bellowingRoarTimer <= diff) {
            DoCast(m_creature->getVictim(), SPELL_BELLOWING_ROAR);
            bellowingRoarTimer = 15000+rand()%3000;
        }
        else
            bellowingRoarTimer -= diff;
            
        if (cleaveTimer <= diff) {
            DoCast(m_creature->getVictim(), SPELL_CLEAVE);
            cleaveTimer = 5000+rand()%3000;
        }
        else
            cleaveTimer -= diff;
            
        if (fieryBreathTimer <= diff) {
            DoCast(SelectUnit(SELECT_TARGET_RANDOM, 0), SPELL_FIERY_BREATH);
            fieryBreathTimer = 5000+rand()%2000;
        }
        else
            fieryBreathTimer -= diff;
            
        if (flameBreathTimer <= diff) {
            DoCast(m_creature->getVictim(), SPELL_FLAME_BREATH);
            flameBreathTimer = 12000+rand()%3000;
        }
        else
            flameBreathTimer -= diff;
        
        DoMeleeAttackIfReady();
    }
};

CreatureAI *GetAI_npc_rivendark(Creature *pCreature)
{
    return new npc_rivendarkAI(pCreature);
}

/*######
## npc_obsidia
######*/

#define SPELL_HELLFIRE      40717

struct npc_obsidiaAI : public ScriptedAI
{
    npc_obsidiaAI(Creature *c) : ScriptedAI(c) {}
    
    uint32 bellowingRoarTimer;
    uint32 cleaveTimer;
    uint32 fieryBreathTimer;
    uint32 flameBreathTimer;
    uint32 hellfireTimer;
    
    void Reset()
    {
        bellowingRoarTimer = 15000;
        cleaveTimer = 5000;
        fieryBreathTimer = 8000;
        flameBreathTimer = 12000;
        hellfireTimer = 15000;
        m_creature->SetDisableGravity(true);
    }
    
    void Aggro(Unit *pWho)
    {
    	m_creature->SetDisableGravity(false);
    }
    
    void UpdateAI(uint32 const diff)
    {
        if (!UpdateVictim())
            return;
            
        if (bellowingRoarTimer <= diff) {
            DoCast(m_creature->getVictim(), SPELL_BELLOWING_ROAR);
            bellowingRoarTimer = 15000+rand()%3000;
        }
        else
            bellowingRoarTimer -= diff;
            
        if (cleaveTimer <= diff) {
            DoCast(m_creature->getVictim(), SPELL_CLEAVE);
            cleaveTimer = 5000+rand()%3000;
        }
        else
            cleaveTimer -= diff;
            
        if (fieryBreathTimer <= diff) {
            DoCast(SelectUnit(SELECT_TARGET_RANDOM, 0), SPELL_FIERY_BREATH);
            fieryBreathTimer = 5000+rand()%2000;
        }
        else
            fieryBreathTimer -= diff;
            
        if (flameBreathTimer <= diff) {
            DoCast(m_creature->getVictim(), SPELL_FLAME_BREATH);
            flameBreathTimer = 12000+rand()%3000;
        }
        else
            flameBreathTimer -= diff;
            
        if (hellfireTimer <= diff) {
            DoCast(m_creature->getVictim(), SPELL_HELLFIRE);
            hellfireTimer = 20000+rand()%8000;
        }
        else
            hellfireTimer -= diff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI *GetAI_npc_obsidia(Creature *pCreature)
{
    return new npc_obsidiaAI(pCreature);
}

/*######
## npc_insidion
######*/

#define SPELL_FLAME_BUFFET      40719

struct npc_insidionAI : public ScriptedAI
{
    npc_insidionAI(Creature *c) : ScriptedAI(c) {}
    
    uint32 bellowingRoarTimer;
    uint32 cleaveTimer;
    uint32 fieryBreathTimer;
    uint32 flameBreathTimer;
    uint32 flameBuffetTimer;
    
    void Reset()
    {
        bellowingRoarTimer = 15000;
        cleaveTimer = 5000;
        fieryBreathTimer = 8000;
        flameBreathTimer = 12000;
        flameBuffetTimer = 15000;

        m_creature->SetDisableGravity(true);
    }
    
    void Aggro(Unit *pWho)
    {
    	m_creature->SetDisableGravity(false);
    }
    
    void UpdateAI(uint32 const diff)
    {
        if (!UpdateVictim())
            return;
            
        if (bellowingRoarTimer <= diff) {
            DoCast(m_creature->getVictim(), SPELL_BELLOWING_ROAR);
            bellowingRoarTimer = 15000+rand()%3000;
        }
        else
            bellowingRoarTimer -= diff;
            
        if (cleaveTimer <= diff) {
            DoCast(m_creature->getVictim(), SPELL_CLEAVE);
            cleaveTimer = 5000+rand()%3000;
        }
        else
            cleaveTimer -= diff;
            
        if (fieryBreathTimer <= diff) {
            DoCast(SelectUnit(SELECT_TARGET_RANDOM, 0), SPELL_FIERY_BREATH);
            fieryBreathTimer = 5000+rand()%2000;
        }
        else
            fieryBreathTimer -= diff;
            
        if (flameBreathTimer <= diff) {
            DoCast(m_creature->getVictim(), SPELL_FLAME_BREATH);
            flameBreathTimer = 12000+rand()%3000;
        }
        else
            flameBreathTimer -= diff;
            
        if (flameBuffetTimer <= diff) {
            DoCast(SelectUnit(SELECT_TARGET_RANDOM, 0), SPELL_FLAME_BUFFET);
            flameBuffetTimer = 15000+rand()%8000;
        }
        else
            flameBuffetTimer -= diff;
            
        DoMeleeAttackIfReady();
    }
};

CreatureAI *GetAI_npc_insidion(Creature *pCreature)
{
    return new npc_insidionAI(pCreature);
}

/*######
## npc_furywing
######*/

#define SPELL_WING_BUFFET       41572

struct npc_furywingAI : public ScriptedAI
{
    npc_furywingAI(Creature *c) : ScriptedAI(c) {}
    
    uint32 tailSweepTimer;
    uint32 bellowingRoarTimer;
    uint32 cleaveTimer;
    uint32 fieryBreathTimer;
    uint32 flameBreathTimer;
    uint32 wingBuffetTimer;
    
    void Reset()
    {
        tailSweepTimer = 10000;
        bellowingRoarTimer = 15000;
        cleaveTimer = 5000;
        fieryBreathTimer = 8000;
        flameBreathTimer = 12000;
        wingBuffetTimer = 2000;

        m_creature->SetDisableGravity(true);
    }
    
    void Aggro(Unit *pWho)
    {
    	m_creature->SetDisableGravity(false);
    }
    
    void UpdateAI(uint32 const diff)
    {
        if (!UpdateVictim())
            return;
            
        if (tailSweepTimer <= diff) {
            Unit *target = SelectUnit(SELECT_TARGET_RANDOM, 1);
            if (target && !m_creature->HasInArc(M_PI, target))
                DoCast(target, SPELL_TAIL_SWEEP);
            tailSweepTimer = 4000+rand()%3000;
        }
        else
            tailSweepTimer -= diff;
            
        if (bellowingRoarTimer <= diff) {
            DoCast(m_creature->getVictim(), SPELL_BELLOWING_ROAR);
            bellowingRoarTimer = 15000+rand()%3000;
        }
        else
            bellowingRoarTimer -= diff;
            
        if (cleaveTimer <= diff) {
            DoCast(m_creature->getVictim(), SPELL_CLEAVE);
            cleaveTimer = 5000+rand()%3000;
        }
        else
            cleaveTimer -= diff;
            
        if (fieryBreathTimer <= diff) {
            DoCast(SelectUnit(SELECT_TARGET_RANDOM, 0), SPELL_FIERY_BREATH);
            fieryBreathTimer = 5000+rand()%2000;
        }
        else
            fieryBreathTimer -= diff;
            
        if (flameBreathTimer <= diff) {
            DoCast(m_creature->getVictim(), SPELL_FLAME_BREATH);
            flameBreathTimer = 12000+rand()%3000;
        }
        else
            flameBreathTimer -= diff;
            
        if (wingBuffetTimer <= diff) {
            DoCast(m_creature->getVictim(), SPELL_WING_BUFFET);
            wingBuffetTimer = 15000+rand()%3000;
        }
        else
            wingBuffetTimer -= diff;
            
        DoMeleeAttackIfReady();
    }
};

CreatureAI *GetAI_npc_furywing(Creature *pCreature)
{
    return new npc_furywingAI(pCreature);
}

/*######
## trigger_banishing_crystal_bunny01
######*/

struct trigger_banishing_crystal_bunny01AI : public Scripted_NoMovementAI
{
    trigger_banishing_crystal_bunny01AI(Creature *c) : Scripted_NoMovementAI(c) {}
    
    void Reset()
    {
        m_creature->AddAura(40849, m_creature);
        m_creature->AddAura(40857, m_creature);
    }
    
    void Aggro(Unit *pWho) {}
};

CreatureAI *GetAI_trigger_banishing_crystal_bunny01(Creature *pCreature)
{
    return new trigger_banishing_crystal_bunny01AI(pCreature);
}

/*######
## npc_rally_zapnabber
######*/

#define QUEST_ZEPHYRIUM_CAPACITORIUM    10557
#define QUEST_SINGING_RIDGE             10710
#define GO_ZEPHYRIUM                    184628
#define SPELL_COSMETIC_LIGHTNING        37071
#define SPELL_SOARING_ZEPHYR            37910
#define SPELL_SOARING_SINGING           37962

struct npc_rally_zapnabberAI : public ScriptedAI
{
    npc_rally_zapnabberAI(Creature* c) : ScriptedAI(c)
    {
        isEvent = false;
        playerGUID = 0;
        lightningCount = 0;
        triggerGUID = 0;
        quest= 0;
    }
    
    bool isEvent;
    
    uint8 lightningCount;
    
    uint32 blueRayTimer;
    uint32 quest;
    
    uint64 playerGUID;
    uint64 triggerGUID;
    
    void Reset()
    {
        blueRayTimer = 0;
    }
    
    void Aggro(Unit* who) {}
    
    void StartEvent()
    {
        isEvent = true;
        if (Player* player = Unit::GetPlayer(playerGUID))
            DoTeleportPlayer(player, 1920.138916, 5581.740723, 269.222229, 5.243360);
            
        if (Creature* trigger = me->SummonCreature(WORLD_TRIGGER, 1925.056152, 5574.165527, 269.162231, 0, TEMPSUMMON_MANUAL_DESPAWN, 0)) {
            triggerGUID = trigger->GetGUID();
            trigger->setFaction(14);
        }
            
        blueRayTimer = 2000;
    }
    
    void EndEvent()
    {
        isEvent = false;
        playerGUID = 0;
        blueRayTimer = 0;
        lightningCount = 0;
        if (Creature* trigger = Creature::GetCreature(*me, triggerGUID))
            trigger->ForcedDespawn();
        triggerGUID = 0;
        quest = 0;
    }
    
    void UpdateAI(uint32 const diff)
    {
        if (!isEvent)
            return;
            
        Player* player = Unit::GetPlayer(playerGUID);
        if (!player || player->GetDistance(me) > 50.0f) {
            EndEvent();
            return;
        }
        
        if (lightningCount >= 5) {
            if (Creature* trigger = Creature::GetCreature(*me, triggerGUID))
                player->CastSpell(player, (quest == QUEST_ZEPHYRIUM_CAPACITORIUM) ? SPELL_SOARING_ZEPHYR : SPELL_SOARING_SINGING, true);
                
            EndEvent();
        }
            
        if (blueRayTimer <= diff) {
            if (Creature* trigger = Creature::GetCreature(*me, triggerGUID)) {
                trigger->CastSpell(player, SPELL_COSMETIC_LIGHTNING, true);
                lightningCount++;
            }
            
            blueRayTimer = 500;
        }
        else
            blueRayTimer -= diff;
    }
};

CreatureAI* GetAI_npc_rally_zapnabber(Creature* creature)
{
    return new npc_rally_zapnabberAI(creature);
}

bool GossipHello_npc_rally_zapnabber(Player* player, Creature* creature)
{
    if (creature->isQuestGiver())
        player->PrepareQuestMenu(creature->GetGUID());

    if (player->GetQuestStatus(QUEST_SINGING_RIDGE) == QUEST_STATUS_INCOMPLETE)
        player->ADD_GOSSIP_ITEM(0, "Emmenez-moi à la Crête chantante.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
    else if (player->GetQuestStatus(QUEST_SINGING_RIDGE) == QUEST_STATUS_INCOMPLETE)
        player->ADD_GOSSIP_ITEM(0, "Je viens tester le Zephyrium Capacitorium", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
        
    player->SEND_GOSSIP_MENU(creature->GetNpcTextId(), creature->GetGUID());

    return true;
}

bool GossipSelect_npc_rally_zapnabber(Player* player, Creature* creature, uint32 sender, uint32 action)
{
    player->CLOSE_GOSSIP_MENU();
    if (action == GOSSIP_ACTION_INFO_DEF) {
        player->CastSpell(player, 37958, true);
        ((npc_rally_zapnabberAI*)creature->AI())->quest = QUEST_SINGING_RIDGE;
        ((npc_rally_zapnabberAI*)creature->AI())->playerGUID = player->GetGUID();
        ((npc_rally_zapnabberAI*)creature->AI())->StartEvent();
    }
    else if (action == GOSSIP_ACTION_INFO_DEF+1) {
        player->CastSpell(player, 37958, true);
        ((npc_rally_zapnabberAI*)creature->AI())->quest = QUEST_ZEPHYRIUM_CAPACITORIUM;
        ((npc_rally_zapnabberAI*)creature->AI())->playerGUID = player->GetGUID();
        ((npc_rally_zapnabberAI*)creature->AI())->StartEvent();
    }

    return true;
}

/*######
## npc_grulloc
######*/

enum grullocSpells {
    SPELL_BURNING_RAGE      = 38771,
    SPELL_CRUSH_ARMOR       = 21055,
    SPELL_GRIEVOUS_WOUND    = 38772
};

struct npc_grullocAI : public ScriptedAI
{
    npc_grullocAI(Creature* c) : ScriptedAI(c)
    {
        isEvent = false;
    }
    
    bool isEvent;
    
    uint32 burningRageTimer;
    uint32 crushArmorTimer;
    uint32 grievousWoundTimer;
    uint32 killHufferTimer;
    
    uint64 hufferGUID;
    
    void Reset()
    {
        burningRageTimer = 20000;
        crushArmorTimer = 7000;
        grievousWoundTimer = 10000;
        killHufferTimer = 0;
        hufferGUID = 0;
    }
    
    void MoveInLineOfSight(Unit* who)
    {
        if (isEvent)
            return;
            
        if (who->ToCreature() && who->GetEntry() == 22114) {
            isEvent = true;
            me->GetMotionMaster()->MoveFollow(who, 1.0f, M_PI);
            hufferGUID = who->GetGUID();
            killHufferTimer = 18000;
        }
    }
    
    void Aggro(Unit* who) {}
    
    void UpdateAI(uint32 const diff)
    {
            
        if (killHufferTimer) {
            if (killHufferTimer <= diff) {
                if (Creature* huffer = Creature::GetCreature(*me, hufferGUID)) {
                    me->Kill(huffer);
                    EnterEvadeMode();
                }
                killHufferTimer = 0;
            }
            else
                killHufferTimer -= diff;
        }

        if (!UpdateVictim())
            return;
        
        if (me->getVictim()->GetEntry() == 22114)
            return;
            
        if (burningRageTimer <= diff) {
            DoCast(me, SPELL_BURNING_RAGE, true);
            burningRageTimer = 20000 + rand() % 5000;
        }
        else
            burningRageTimer -= diff;
            
        if (crushArmorTimer <= diff) {
            DoCast(me->getVictim(),SPELL_CRUSH_ARMOR);
            crushArmorTimer = 12000 + rand() % 4000;
        }
        else
            crushArmorTimer -= diff;
            
        if (grievousWoundTimer <= diff) {
            DoCast(me->getVictim(),SPELL_GRIEVOUS_WOUND);
            grievousWoundTimer = 45000;
        }
        else
            grievousWoundTimer -= diff;
            
        DoMeleeAttackIfReady();
    }

    void JustDied(Unit* killer)
    {
        killer->SummonGameObject(185567, 2694.32, 5525.05, 1.18, 0, 0, 0, 0, 0, 60000);
    }
};

CreatureAI* GetAI_npc_grulloc(Creature* creature)
{
    return new npc_grullocAI(creature);
}

/*######
## npc_huffler
######*/

struct npc_hufferAI : public ScriptedAI
{
    npc_hufferAI(Creature* c) : ScriptedAI(c) {}
    
    void Reset()
    {
        if (Creature* grulloc = me->FindCreatureInGrid(20216, 30.0f, true))
            me->GetMotionMaster()->MovePath(22114, true);
    }
    
    void Aggro(Unit* who) {}
    
    void UpdateAI(uint32 const diff) {}
};

CreatureAI* GetAI_npc_huffer(Creature* creature)
{
    return new npc_hufferAI(creature);
}

/*######
## npc_grishna
######*/

struct npc_grishnaAI : public ScriptedAI
{
    npc_grishnaAI(Creature* c) : ScriptedAI(c) {}
    
    void Aggro(Unit* who) {}
    
    void JustDied(Unit* killer)
    {
        killer->CastSpell(killer, 37466, true);
    }
    
    void UpdateAI(uint32 const diff)
    {
        if (!UpdateVictim())
            return;
            
        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_grishna(Creature* creature)
{
    return new npc_grishnaAI(creature);
}

/*######
## AddSC
######*/

void AddSC_blades_edge_mountains()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name="mobs_bladespire_ogre";
    newscript->GetAI = &GetAI_mobs_bladespire_ogre;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="mobs_nether_drake";
    newscript->GetAI = &GetAI_mobs_nether_drake;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="npc_daranelle";
    newscript->GetAI = &GetAI_npc_daranelle;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="npc_overseer_nuaar";
    newscript->pGossipHello = &GossipHello_npc_overseer_nuaar;
    newscript->pGossipSelect = &GossipSelect_npc_overseer_nuaar;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="npc_saikkal_the_elder";
    newscript->pGossipHello = &GossipHello_npc_saikkal_the_elder;
    newscript->pGossipSelect = &GossipSelect_npc_saikkal_the_elder;
    newscript->RegisterSelf();
	
    newscript = new Script;
    newscript->Name="go_legion_obelisk";
    newscript->pGOHello =           &GOHello_go_legion_obelisk;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="npc_skyguard_handler_irena";
    newscript->pGossipHello =  &GossipHello_npc_skyguard_handler_irena;
    newscript->pGossipSelect = &GossipSelect_npc_skyguard_handler_irena;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_prophecy_questcredit";
    newscript->GetAI = &GetAI_npc_prophecy_questcredit;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_grishna_falconwing";
    newscript->GetAI = &GetAI_npc_grishna_falconwingAI;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "go_ethereum_chamber";
    newscript->pGOHello = &GOHello_go_ethereum_chamber;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_kolphis_darkscale";
    newscript->pGossipHello = &GossipHello_npc_kolphis_darkscale;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "trigger_vimgol_circle_bunny";
    newscript->GetAI = &GetAI_trigger_vimgol_circle_bunny;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_vimgol";
    newscript->GetAI = &GetAI_npc_vimgol;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_soulgrinder";
    newscript->GetAI = &GetAI_npc_soulgrinder;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_skulloc";
    newscript->GetAI = &GetAI_npc_skulloc;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_sundered_ghost";
    newscript->GetAI = &GetAI_npc_sundered_ghost;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "go_apexis_relic";
    newscript->pGOHello = &GOHello_go_apexis_relic;
    newscript->pGOSelect = &GOSelect_go_apexis_relic;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_simon_bunny";
    newscript->GetAI = &GetAI_npc_simon_bunny;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "go_blue_cluster";
    newscript->pGOHello = &GOHello_go_blue_cluster;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "go_green_cluster";
    newscript->pGOHello = &GOHello_go_green_cluster;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "go_red_cluster";
    newscript->pGOHello = &GOHello_go_red_cluster;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "go_yellow_cluster";
    newscript->pGOHello = &GOHello_go_yellow_cluster;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "go_apexis_monument";
    newscript->pGOHello = &GOHello_go_apexis_monument;
    newscript->pGOSelect = &GOSelect_go_apexis_monument;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_simon_bunny_large";
    newscript->GetAI = &GetAI_npc_simon_bunny_large;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "go_blue_cluster_large";
    newscript->pGOHello = &GOHello_go_blue_cluster_large;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "go_green_cluster_large";
    newscript->pGOHello = &GOHello_go_green_cluster_large;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "go_red_cluster_large";
    newscript->pGOHello = &GOHello_go_red_cluster_large;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "go_yellow_cluster_large";
    newscript->pGOHello = &GOHello_go_yellow_cluster_large;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "go_fel_crystal_prism";
    newscript->pGOHello = &GOHello_go_fel_crystal_prism;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_braxxus";
    newscript->GetAI = &GetAI_npc_braxxus;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_moarg_incinerator";
    newscript->GetAI = &GetAI_npc_moarg_incinerator;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_galvanoth";
    newscript->GetAI = &GetAI_npc_galvanoth;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_zarcsin";
    newscript->GetAI = &GetAI_npc_zarcsin;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_aether_ray";
    newscript->GetAI = &GetAI_npc_aether_ray;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_wrangled_aether_ray";
    newscript->GetAI = &GetAI_npc_wrangled_aether_ray;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "go_drake_egg";
    newscript->pGOHello = &GOHello_go_drake_egg;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_rivendark";
    newscript->GetAI = &GetAI_npc_rivendark;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_obsidia";
    newscript->GetAI = &GetAI_npc_obsidia;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_insidion";
    newscript->GetAI = &GetAI_npc_insidion;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_furywing";
    newscript->GetAI = &GetAI_npc_furywing;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "trigger_banishing_crystal_bunny01";
    newscript->GetAI = &GetAI_trigger_banishing_crystal_bunny01;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_rally_zapnabber";
    newscript->GetAI = &GetAI_npc_rally_zapnabber;
    newscript->pGossipHello = &GossipHello_npc_rally_zapnabber;
    newscript->pGossipSelect = &GossipSelect_npc_rally_zapnabber;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_grulloc";
    newscript->GetAI = &GetAI_npc_grulloc;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_huffer";
    newscript->GetAI = &GetAI_npc_huffer;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_grishna";
    newscript->GetAI = &GetAI_npc_grishna;
    newscript->RegisterSelf();
}

