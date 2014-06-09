/* Copyright (C) 2006 - 2008 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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

/* ScriptData
SDName: Zulaman
SD%Complete: 90
SDComment: Forest Frog will turn into different NPC's. Workaround to prevent new entry from running this script
SDCategory: Zul'Aman
EndScriptData */

/* ContentData
npc_forest_frog
npc_zulaman_hostage
npc_harrison_jones
go_za_gong
npc_amanishi_lookout
npc_amanishi_tempest
npc_amanishi_berserker
npc_amanishi_scout
at_quest_X_marks
mob_akilzon_gauntlet
EndContentData */

#include "precompiled.h"
#include "def_zulaman.h"

/*######
## npc_forest_frog
######*/

#define SPELL_REMOVE_AMANI_CURSE    43732
#define SPELL_PUSH_MOJO             43923
#define ENTRY_FOREST_FROG           24396

struct npc_forest_frogAI : public ScriptedAI
{
    npc_forest_frogAI(Creature* c) : ScriptedAI(c)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
    }

    ScriptedInstance *pInstance;

    void Reset() { }

    void EnterCombat(Unit *who) { }

    void DoSpawnRandom()
    {
        if( pInstance )
        {
            uint32 cEntry = 0;
            switch(rand()%10)
            {
                case 0: cEntry = 24397; break;          //Mannuth
                case 1: cEntry = 24403; break;          //Deez
                case 2: cEntry = 24404; break;          //Galathryn
                case 3: cEntry = 24405; break;          //Adarrah
                case 4: cEntry = 24406; break;          //Fudgerick
                case 5: cEntry = 24407; break;          //Darwen
                case 6: cEntry = 24445; break;          //Mitzi
                case 7: cEntry = 24448; break;          //Christian
                case 8: cEntry = 24453; break;          //Brennan
                case 9: cEntry = 24455; break;          //Hollee
            }

            if (!pInstance->GetData(TYPE_RAND_VENDOR_1))
                if(rand()%10 == 1) cEntry = 24408;      //Gunter
            if (!pInstance->GetData(TYPE_RAND_VENDOR_2))
                if(rand()%10 == 1) cEntry = 24409;      //Kyren

            if (cEntry) m_creature->UpdateEntry(cEntry);

            if (cEntry == 24408) pInstance->SetData(TYPE_RAND_VENDOR_1,DONE);
            if (cEntry == 24409) pInstance->SetData(TYPE_RAND_VENDOR_2,DONE);
        }
    }

    void SpellHit(Unit *caster, const SpellEntry *spell)
    {
        if( spell->Id == SPELL_REMOVE_AMANI_CURSE && caster->GetTypeId() == TYPEID_PLAYER && m_creature->GetEntry() == ENTRY_FOREST_FROG )
        {
            //increase or decrease chance of mojo?
            if( rand()%99 == 50 ) DoCast(caster,SPELL_PUSH_MOJO,true);
            else DoSpawnRandom();
        }
    }
};
CreatureAI* GetAI_npc_forest_frog(Creature *_Creature)
{
    return new npc_forest_frogAI (_Creature);
}

/*######
## npc_zulaman_hostage
######*/

#define GOSSIP_HOSTAGE1        "I am glad to help you."

static uint32 HostageEntry[] = {23790, 23999, 24024, 24001};
static uint32 ChestEntry[] = {186648, 187021, 186667, 186672};

struct npc_zulaman_hostageAI : public ScriptedAI
{
    npc_zulaman_hostageAI(Creature *c) : ScriptedAI(c) {IsLoot = false;}
    bool IsLoot;
    uint64 PlayerGUID;
    void Reset() {}
    void EnterCombat(Unit *who) {}
    void JustDied(Unit *)
    {
        Player* player = Unit::GetPlayer(PlayerGUID);
        if(player) player->SendLoot(m_creature->GetGUID(), LOOT_CORPSE);
    }
    void UpdateAI(const uint32 diff)
    {
        if(IsLoot) m_creature->CastSpell(m_creature, 7, false);
    }
};

bool GossipHello_npc_zulaman_hostage(Player* player, Creature* _Creature)
{
    player->ADD_GOSSIP_ITEM(0, GOSSIP_HOSTAGE1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
    player->SEND_GOSSIP_MENU(_Creature->GetNpcTextId(), _Creature->GetGUID());
    return true;
}

bool GossipSelect_npc_zulaman_hostage(Player* player, Creature* _Creature, uint32 sender, uint32 action)
{
    if(action == GOSSIP_ACTION_INFO_DEF + 1)
        player->CLOSE_GOSSIP_MENU();

    if(!_Creature->HasFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP))
        return true;
    _Creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);

    ScriptedInstance* pInstance = ((ScriptedInstance*)_Creature->GetInstanceData());
    if(pInstance)
    {
        //uint8 progress = pInstance->GetData(DATA_CHESTLOOTED);
        pInstance->SetData(DATA_CHESTLOOTED, 0);
        float x, y, z;
        _Creature->GetPosition(x, y, z);
        uint32 entry = _Creature->GetEntry();
        for(uint8 i = 0; i < 4; ++i)
        {
            if(HostageEntry[i] == entry)
            {
                sLog.outError("ZUL'AMAN CHEST: Data 7 was %u when spawning gameobject.", pInstance->GetData(DATA_CHESTLOOTED));
                _Creature->SummonGameObject(ChestEntry[i], x-2, y, z, 0, 0, 0, 0, 0, 0);
                break;
            }
        }
        /*Creature* summon = _Creature->SummonCreature(HostageInfo[progress], x-2, y, z, 0, TEMPSUMMON_DEAD_DESPAWN, 0);
        if(summon)
        {
            ((npc_zulaman_hostageAI*)summon->AI())->PlayerGUID = player->GetGUID();
            ((npc_zulaman_hostageAI*)summon->AI())->IsLoot = true;
            summon->SetDisplayId(10056);
            summon->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            summon->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
        }*/
    }
    return true;
}

CreatureAI* GetAI_npc_zulaman_hostage(Creature *_Creature)
{
    return new npc_zulaman_hostageAI(_Creature);
}

/*######
## npc_harrison_jones
######*/

#define GONG_ENTRY          187359
#define MASSIVEGATE_ENTRY   186728

struct TRINITY_DLL_DECL npc_harrison_jonesAI : public ScriptedAI
{
    npc_harrison_jonesAI(Creature *c) : ScriptedAI(c) {}
    
    uint8 gongClicked;
    uint32 suicideTimer;
    uint32 deathTimer;
    
    std::vector<uint64> clickGUIDs;
    
    void Reset()
    {
        gongClicked = 0;
        suicideTimer = 0;
        deathTimer = 0;
    }
    
    void EnterCombat(Unit* pWho) {}
    
    void IncreaseClick(uint64 pGUID)
    {
        // Don't allow the same player to click several times
        for (std::vector<uint64>::const_iterator itr = clickGUIDs.begin(); itr != clickGUIDs.end(); itr++) {
            if ((*itr) == pGUID)
                return;
        }
        
        gongClicked++;
        std::string s = "";
        if (gongClicked < 4)
            s = "s";
        std::stringstream sst;
        sst << "Encore " << 5 - gongClicked << " personne" << s << " !";
        if (gongClicked < 5)
            m_creature->Say(sst.str().c_str(), LANG_UNIVERSAL, 0);
        
        clickGUIDs.push_back(pGUID);
        
        if (gongClicked >= 5)
            OpenDoorAndStartTimer();
    }
    
    void OpenDoorAndStartTimer()
    {
        if (GameObject* gong = m_creature->FindGOInGrid(GONG_ENTRY, 15.0f))   // Lock the gong again
            gong->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_LOCKED);
        
        if (ScriptedInstance* pInstance = ((ScriptedInstance*)m_creature->GetInstanceData())) {
            pInstance->SetData(DATA_QUESTMINUTE, IN_PROGRESS);
            pInstance->SetData(DATA_GONG_EVENT, DONE);
            // Also remove aura on all players in map
            Map::PlayerList const& players = pInstance->instance->GetPlayers();
            if (!players.isEmpty()) {
                for(Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr) {
                    if (Player* plr = itr->getSource())
                        plr->RemoveAurasDueToSpell(45225);
                }
            }
        }
        
        m_creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);   // Prevent restarting the timer I don't know how...
        m_creature->RemoveAurasDueToSpell(45225);
        
        m_creature->GetMotionMaster()->MovePath(24358, false);
        suicideTimer = 14000;
    }
    
    void UpdateAI(uint32 const diff)
    {
        if (suicideTimer) {
            if (suicideTimer <= diff) {
                if (GameObject* massiveDoor = m_creature->FindGOInGrid(MASSIVEGATE_ENTRY, 50.0f))
                    massiveDoor->SetUInt32Value(GAMEOBJECT_STATE, 0);
                suicideTimer = 0;
                deathTimer = 5000;
            }
            else
                suicideTimer -= diff;
        }
        
        if (deathTimer) {
            if (deathTimer <= diff) {
                m_creature->DisappearAndDie();
            }
            else
                deathTimer -= diff;
        }
    }
};

CreatureAI* GetAI_npc_harrison_jones(Creature* pCreature)
{
    return new npc_harrison_jonesAI(pCreature);
}

#define GOSSIP_GONG         "[PH] Nous sommes prêts."
#define GOSSIP_GONG_DEBUG   "[DEBUG] Open Door."

bool GossipHello_npc_harrison_jones(Player* pPlayer, Creature* pCreature)
{
    pPlayer->ADD_GOSSIP_ITEM(0, GOSSIP_GONG, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
    if (pPlayer->IsGameMaster())
        pPlayer->ADD_GOSSIP_ITEM(0, GOSSIP_GONG_DEBUG, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
    pPlayer->SEND_GOSSIP_MENU(pCreature->GetNpcTextId(), pCreature->GetGUID());
    
    return true;
}

bool GossipSelect_npc_harrison_jones(Player* pPlayer, Creature* pCreature, uint32 sender, uint32 action)
{
    if (action == GOSSIP_ACTION_INFO_DEF+1) {
        pPlayer->PlayerTalkClass->CloseGossip();
        if (GameObject* pGo = pCreature->FindGOInGrid(GONG_ENTRY, 15.0f)) {
            pGo->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_LOCKED);
            pCreature->SetOrientation(5.7499);
            pCreature->Say("Aidez-moi à frapper sur ce gong !", LANG_UNIVERSAL, 0);
            pCreature->AddAura(45225, pCreature);
        }
        else
            sLog.outError("ERROR: Zul'aman: Harrison Jones couldn't find the gong.");
            
        return true;
    }
    else if (action == GOSSIP_ACTION_INFO_DEF+2 && pPlayer->IsGameMaster()) {
        pPlayer->PlayerTalkClass->CloseGossip();
        CAST_AI(npc_harrison_jonesAI, (pCreature->AI()))->OpenDoorAndStartTimer();
    }
    
    return false;
}

#define HARRISON_ENTRY  24358

/*######
## go_za_gong
######*/

bool GOHello_go_za_gong(Player* pPlayer, GameObject* pGo)
{
    if (pGo->HasFlag(GAMEOBJECT_FLAGS, GO_FLAG_LOCKED))
        return false;
    else {
        if (Creature* harrisonJones = pGo->FindCreatureInGrid(HARRISON_ENTRY, 15.0f, true)) {
            CAST_AI(npc_harrison_jonesAI, (harrisonJones->AI()))->IncreaseClick(pPlayer->GetGUID());
            pPlayer->InterruptNonMeleeSpells(true);
            pPlayer->CastSpell(pPlayer, 45226, true);
            //pPlayer->CastSpell(pPlayer, 44762, true);
            if (!pPlayer->HasAura(45225))
                pPlayer->AddAura(45225, pPlayer);
        }
            
        return true;
    }
    
    return false;
}

/*######
## npc_amanishi_lookout
######*/

struct TRINITY_DLL_DECL npc_amanishi_lookoutAI : public ScriptedAI
{
    npc_amanishi_lookoutAI(Creature *c) : ScriptedAI(c)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
    }
    
    ScriptedInstance *pInstance;
    
    void Reset()
    {
        m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        if (pInstance && pInstance->GetData(DATA_GAUNTLET) != DONE)
            pInstance->SetData(DATA_GAUNTLET, NOT_STARTED);
    }
    
    void EnterCombat(Unit *pWho) {}
    
    void MoveInLineOfSight(Unit *pWho)
    {
        if (!pInstance)
            return;
            
        if (m_creature->GetDistance(pWho) <= 23.0f) {
            if (pInstance->GetData(DATA_GAUNTLET) == NOT_STARTED) {
                pInstance->SetData(DATA_GAUNTLET, IN_PROGRESS);
                m_creature->SetSpeed(MOVE_WALK, 5);
                m_creature->GetMotionMaster()->MovePoint(1, 227.822556, 1402.593384, 37.555401);
                m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                //m_creature->GetMotionMaster()->MovePoint(0, 228.332535, 1435.903687, 26.650408);
            }
        }
    }
    
    void MovementInform(uint32 type, uint32 id)
    {
        if (id == 0) {
            m_creature->GetMotionMaster()->MovementExpired();
            m_creature->GetMotionMaster()->MovePoint(1, 227.822556, 1402.593384, 37.555401);
        }
        else if (id == 1) {
            m_creature->GetMotionMaster()->MovementExpired();
            m_creature->DisappearAndDie();
        }
    }
};

CreatureAI* GetAI_npc_amanishi_lookout(Creature *pCreature)
{
    return new npc_amanishi_lookoutAI(pCreature);
}

/*######
## npc_amanishi_tempest
######*/

#define SPELL_THUNDERCLAP   44033

struct TRINITY_DLL_DECL npc_amanishi_tempestAI : public ScriptedAI
{
    npc_amanishi_tempestAI(Creature *c) : ScriptedAI(c)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
    }
    
    ScriptedInstance *pInstance;
    uint32 thunderclapTimer;
    
    void Reset()
    {
        thunderclapTimer = 3000;
    }
    
    void EnterCombat(Unit *pWho)
    {
        if (pInstance && pInstance->GetData(DATA_GAUNTLET) == IN_PROGRESS)
            pInstance->SetData(DATA_GAUNTLET, DONE);
    }
    
    void UpdateAI(uint32 const diff)
    {
        if (!UpdateVictim())
            return;
            
        if (thunderclapTimer <= diff) {
            DoCast(m_creature->GetVictim(), SPELL_THUNDERCLAP);
            thunderclapTimer = 8000;
        }
        else
            thunderclapTimer -= diff;
            
        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_amanishi_tempest(Creature *pCreature)
{
    return new npc_amanishi_tempestAI(pCreature);
}

/*######
## npc_amanishi_berserker
######*/

#define SPELL_MIGHTY_BLOW       43673
#define SPELL_FRENZY            28747

struct TRINITY_DLL_DECL npc_amanishi_berserkerAI : public ScriptedAI
{
    npc_amanishi_berserkerAI(Creature *c) : ScriptedAI(c) {}
    
    uint32 MightyBlowTimer;
    
    void Reset() {
        m_creature->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true);
        MightyBlowTimer = 5000;
    }
    
    void EnterCombat(Unit *pWho) {}
    
    void UpdateAI(uint32 const diff) {
        if (!UpdateVictim())
            return;
            
        if (MightyBlowTimer <= diff) {
            DoCast(m_creature->GetVictim(), SPELL_MIGHTY_BLOW, true);
            MightyBlowTimer = 12000;
        } else MightyBlowTimer -= diff;
        
        if (m_creature->GetHealth() <= m_creature->GetMaxHealth()/5.0f) {   // Cast Frenzy at 20% health
            if (!m_creature->HasAura(SPELL_FRENZY))
                DoCast(m_creature, SPELL_FRENZY, true);
        }
        
        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_amanishi_berserker(Creature *pCreature)
{
    return new npc_amanishi_berserkerAI(pCreature);
}

/*######
## npc_amanishi_scout
######*/

enum eAmanishiScout
{
    SPELL_ALERT_DRUMS       = 42177,
    SPELL_MULTI_SHOOT       = 43205,
    SPELL_SHOOT             = 16496,
    GO_AMANI_DRUM           = 186865
};

struct npc_amanishi_scoutAI : public ScriptedAI
{
    npc_amanishi_scoutAI(Creature *c) : ScriptedAI(c) {}
    
    uint32 shootTimer;
    uint32 multiShootTimer;
    
    bool hasRunToDrum;
    
    void Reset()
    {
        shootTimer = 2000;
        multiShootTimer = 6000;
        
        hasRunToDrum = false;
    }
    
    void EnterCombat(Unit *pWho)
    {
        if (GameObject *amaniDrum = m_creature->FindGOInGrid(GO_AMANI_DRUM, 50.0f))
            m_creature->GetMotionMaster()->MovePoint(0, amaniDrum->GetPositionX(), amaniDrum->GetPositionY(), amaniDrum->GetPositionZ());
    }
    
    void UpdateAI(uint32 const diff)
    {
        if (!UpdateVictim())
            return;
            
        // Check here if we're close from the drum
        
        if (shootTimer <= diff) {
            DoCast(SelectUnit(SELECT_TARGET_RANDOM, 0), SPELL_SHOOT);
            shootTimer = 4000+rand()%1000;
        }
        else
            shootTimer -= diff;
            
        if (multiShootTimer <= diff) {
            DoCast(SelectUnit(SELECT_TARGET_RANDOM, 0), SPELL_MULTI_SHOOT);
            multiShootTimer = 20000+rand()%4000;
        }
        else
            multiShootTimer -= diff;
            
        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_amanishi_scout(Creature *pCreature)
{
    return new npc_amanishi_scoutAI(pCreature);
}

/*######
## at_quest_X_marks
######*/

bool AreaTrigger_at_quest_X_marks(Player *pPlayer, AreaTriggerEntry const *pAt) {
    if (pPlayer->GetQuestStatus(11166) != QUEST_STATUS_INCOMPLETE)
        return false;
        
    uint32 cEntry = 0;
    switch (pAt->id) {
    case 4726:
        cEntry = 23815;
        break;
    case 4724:
        cEntry = 23813;
        break;
    case 4725:
        cEntry = 23814;
        break;
    default:
        sLog.outError("Scriptname at_quest_X_marks assigned to wrong AreaTrigger!");
        break;
    }

    pPlayer->KilledMonster(cEntry, 0);
    
    return true;
}

/*######
## mob_akilzon_gauntlet
######*/

struct mob_akilzon_gauntletAI : public ScriptedAI
{
    mob_akilzon_gauntletAI(Creature* c) : ScriptedAI(c)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
    }
    
    ScriptedInstance* pInstance;
    
    void EnterCombat(Unit* pWho) {}
    
    void MovementInform(uint32 type, uint32 id)
    {
        if (me->GetEntry() == 24225 && id == 6)
            me->DisappearAndDie();
        else if (me->GetEntry() == 24159 && id == 7)
            me->DisappearAndDie();
    }
    
    void UpdateAI(uint32 const diff)
    {
        if (!UpdateVictim())
            return;
            
        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_mob_akilzon_gauntlet(Creature* pCreature)
{
    return new mob_akilzon_gauntletAI(pCreature);
}

void AddSC_zulaman()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name="npc_forest_frog";
    newscript->GetAI = &GetAI_npc_forest_frog;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_zulaman_hostage";
    newscript->GetAI = &GetAI_npc_zulaman_hostage;
    newscript->pGossipHello = &GossipHello_npc_zulaman_hostage;
    newscript->pGossipSelect = &GossipSelect_npc_zulaman_hostage;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_harrison_jones";
    newscript->GetAI = &GetAI_npc_harrison_jones;
    newscript->pGossipHello = &GossipHello_npc_harrison_jones;
    newscript->pGossipSelect = &GossipSelect_npc_harrison_jones;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "go_za_gong";
    newscript->pGOHello = &GOHello_go_za_gong;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_amanishi_lookout";
    newscript->GetAI = &GetAI_npc_amanishi_lookout;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_amanishi_tempest";
    newscript->GetAI = &GetAI_npc_amanishi_tempest;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_amanishi_berserker";
    newscript->GetAI = &GetAI_npc_amanishi_berserker;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_amanishi_scout";
    newscript->GetAI = &GetAI_npc_amanishi_scout;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "at_quest_X_marks";
    newscript->pAreaTrigger = &AreaTrigger_at_quest_X_marks;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "mob_akilzon_gauntlet";
    newscript->GetAI = &GetAI_mob_akilzon_gauntlet;
    newscript->RegisterSelf();
}

