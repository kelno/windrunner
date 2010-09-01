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
SDName: Stratholme
SD%Complete: 100
SDComment: Misc mobs for instance. GO-script to apply aura and start event for quest 8945
SDCategory: Stratholme
EndScriptData */

/* ContentData
go_gauntlet_gate
mob_freed_soul
mob_restless_soul
mobs_spectral_ghostly_citizen
at_timmy_the_cruel
npc_ashari_crystal
EndContentData */

#include "precompiled.h"
#include "def_stratholme.h"

/*######
## go_gauntlet_gate (this is the _first_ of the gauntlet gates, two exist)
######*/

bool GOHello_go_gauntlet_gate(Player *player, GameObject* _GO)
{
    ScriptedInstance* pInstance = (ScriptedInstance*)_GO->GetInstanceData();

    if (!pInstance)
        return false;

    if (pInstance->GetData(TYPE_BARON_RUN) != NOT_STARTED)
        return false;

    if (Group *pGroup = player->GetGroup())
    {
        for(GroupReference *itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            Player* pGroupie = itr->getSource();
            if (!pGroupie)
                continue;

            if (pGroupie->GetQuestStatus(QUEST_DEAD_MAN_PLEA) == QUEST_STATUS_INCOMPLETE &&
                !pGroupie->HasAura(SPELL_BARON_ULTIMATUM,0) &&
                pGroupie->GetMap() == _GO->GetMap())
                pGroupie->CastSpell(pGroupie,SPELL_BARON_ULTIMATUM,true);
        }
    } else if (player->GetQuestStatus(QUEST_DEAD_MAN_PLEA) == QUEST_STATUS_INCOMPLETE &&
                !player->HasAura(SPELL_BARON_ULTIMATUM,0) &&
                player->GetMap() == _GO->GetMap())
                player->CastSpell(player,SPELL_BARON_ULTIMATUM,true);

    pInstance->SetData(TYPE_BARON_RUN,IN_PROGRESS);
    return false;
}

/*######
## mob_freed_soul
######*/

//Possibly more of these quotes around.
#define SAY_ZAPPED0 "Thanks to Egan"
#define SAY_ZAPPED1 "Rivendare must die"
#define SAY_ZAPPED2 "Who you gonna call?"
#define SAY_ZAPPED3 "Don't cross those beams!"

struct mob_freed_soulAI : public ScriptedAI
{
    mob_freed_soulAI(Creature *c) : ScriptedAI(c) {}

    void Reset()
    {
        switch (rand()%4)
        {
            case 0: DoSay(SAY_ZAPPED0,LANG_UNIVERSAL,NULL); break;
            case 1: DoSay(SAY_ZAPPED1,LANG_UNIVERSAL,NULL); break;
            case 2: DoSay(SAY_ZAPPED2,LANG_UNIVERSAL,NULL); break;
            case 3: DoSay(SAY_ZAPPED3,LANG_UNIVERSAL,NULL); break;
        }
    }

    void Aggro(Unit* who) { }
};

CreatureAI* GetAI_mob_freed_soul(Creature *_Creature)
{
    return new mob_freed_soulAI (_Creature);
}

/*######
## mob_restless_soul
######*/

#define SPELL_EGAN_BLASTER  17368
#define SPELL_SOUL_FREED    17370
#define QUEST_RESTLESS_SOUL 5282
#define ENTRY_RESTLESS      11122
#define ENTRY_FREED         11136

struct mob_restless_soulAI : public ScriptedAI
{
    mob_restless_soulAI(Creature *c) : ScriptedAI(c) {}

    uint64 Tagger;
    uint32 Die_Timer;
    bool Tagged;

    void Reset()
    {
        Tagger = 0;
        Die_Timer = 5000;
        Tagged = false;
    }

    void Aggro(Unit* who) { }

    void SpellHit(Unit *caster, const SpellEntry *spell)
    {
        if (caster->GetTypeId() == TYPEID_PLAYER)
        {
            if (!Tagged && spell->Id == SPELL_EGAN_BLASTER && (caster->ToPlayer())->GetQuestStatus(QUEST_RESTLESS_SOUL) == QUEST_STATUS_INCOMPLETE)
            {
                Tagged = true;
                Tagger = caster->GetGUID();
            }
        }
    }

    void JustSummoned(Creature *summoned)
    {
        summoned->CastSpell(summoned,SPELL_SOUL_FREED,false);
    }

    void JustDied(Unit* Killer)
    {
        if (Tagged)
            m_creature->SummonCreature(ENTRY_FREED, m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ(), m_creature->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN, 300000);
    }

    void UpdateAI(const uint32 diff)
    {
        if (Tagged)
        {
            if (Die_Timer < diff)
            {
                if (Unit* pTemp = Unit::GetUnit(*m_creature,Tagger))
                {
                    CAST_PLR(pTemp)->KilledMonster(ENTRY_RESTLESS, m_creature->GetGUID());
                    m_creature->Kill(m_creature);
                }
            }else Die_Timer -= diff;
        }
    }
};

CreatureAI* GetAI_mob_restless_soul(Creature *_Creature)
{
    return new mob_restless_soulAI (_Creature);
}

/*######
## mobs_spectral_ghostly_citizen
######*/

#define SPELL_HAUNTING_PHANTOM  16336

struct mobs_spectral_ghostly_citizenAI : public ScriptedAI
{
    mobs_spectral_ghostly_citizenAI(Creature *c) : ScriptedAI(c) {}

    uint32 Die_Timer;
    bool Tagged;

    void Reset()
    {
        Die_Timer = 5000;
        Tagged = false;
    }

    void Aggro(Unit* who) { }

    void SpellHit(Unit *caster, const SpellEntry *spell)
    {
        if (!Tagged && spell->Id == SPELL_EGAN_BLASTER)
            Tagged = true;
    }

    void JustDied(Unit* Killer)
    {
        if (Tagged)
        {
            for(uint32 i = 1; i <= 4; i++)
            {
                float x,y,z;
                 m_creature->GetRandomPoint(m_creature->GetPositionX(),m_creature->GetPositionY(),m_creature->GetPositionZ(),20.0f,x,y,z);

                 //100%, 50%, 33%, 25% chance to spawn
                 uint32 j = urand(1,i);
                 if (j==1)
                     m_creature->SummonCreature(ENTRY_RESTLESS,x,y,z,0,TEMPSUMMON_CORPSE_DESPAWN,600000);
            }
        }
    }

    void UpdateAI(const uint32 diff)
    {
        if (Tagged)
        {
            if (Die_Timer < diff)
            {
                m_creature->DealDamage(m_creature, m_creature->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
            }else Die_Timer -= diff;
        }

        if (!UpdateVictim())
            return;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_mobs_spectral_ghostly_citizen(Creature *_Creature)
{
    return new mobs_spectral_ghostly_citizenAI (_Creature);
}

bool ReciveEmote_mobs_spectral_ghostly_citizen(Player *player, Creature *_Creature, uint32 emote)
{
    switch(emote)
    {
        case TEXTEMOTE_DANCE:
            ((mobs_spectral_ghostly_citizenAI*)_Creature->AI())->EnterEvadeMode();
            break;
        case TEXTEMOTE_RUDE:
            //Should instead cast spell, kicking player back. Spell not found.
            if (_Creature->IsWithinDistInMap(player, 5))
                _Creature->HandleEmoteCommand(EMOTE_ONESHOT_RUDE);
            else
                _Creature->HandleEmoteCommand(EMOTE_ONESHOT_RUDE);
            break;
        case TEXTEMOTE_WAVE:
            _Creature->HandleEmoteCommand(EMOTE_ONESHOT_WAVE);
            break;
        case TEXTEMOTE_BOW:
            _Creature->HandleEmoteCommand(EMOTE_ONESHOT_BOW);
            break;
        case TEXTEMOTE_KISS:
            _Creature->HandleEmoteCommand(EMOTE_ONESHOT_FLEX);
            break;
    }

    return true;
}

/*######
## at_timmy_the_cruel
######*/

bool AreaTrigger_at_timmy_the_cruel(Player *pPlayer, AreaTriggerEntry *at)
{
    ScriptedInstance *pInstance = ((ScriptedInstance*)pPlayer->GetInstanceData());
    if (!pInstance)
        return false;
    if (pInstance->GetData(TYPE_TIMMY_SPAWN))
        return false;
    if (Creature* cr = pPlayer->FindCreatureInGrid(10148, 50.0f, true))
        return false;
    if (Creature* cr = pPlayer->FindCreatureInGrid(10391, 50.0f, true))
        return false;
    if (Creature* cr = pPlayer->FindCreatureInGrid(10390, 50.0f, true))
        return false;
    if (Creature* cr = pPlayer->FindCreatureInGrid(10420, 50.0f, true))
        return false;
    if (Creature* cr = pPlayer->FindCreatureInGrid(10419, 50.0f, true))
        return false;
        
    // Else, the square is clean, wait 3 sec and spawn Timmy
    pInstance->SetData(TYPE_TIMMY_SPAWN, DONE);
    return true;
}

/*######
## go_cannonball_stack
######*/

bool GOHello_go_cannonball_stack(Player *pPlayer, GameObject* pGo)
{
    //pPlayer->SendLoot(pGo->GetGUID(), LOOT_CORPSE);
    ItemPosCountVec dest;
    uint8 msg = pPlayer->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, 12973, 1);
    if (msg == EQUIP_ERR_OK)
    {
        Item* item = pPlayer->StoreNewItem( dest, 12973, true);
        pPlayer->SendNewItem(item, 1, true, false);
        pGo->SetLootState(GO_READY); // Should despawn GO, can be respawned if boss resets
        
        return false;
    }
    pGo->SetLootState(GO_ACTIVATED);
    
    return true;
}

/*######
## npc_ashari_crystal
######*/

#define NPC_THUZADIN_ACOLYTE    10399

struct npc_ashari_crystalAI : public ScriptedAI
{
    npc_ashari_crystalAI(Creature *c) : ScriptedAI(c) {}
    
    void Reset()
    {
        m_creature->SetUnitMovementFlags(MOVEMENTFLAG_ONTRANSPORT + MOVEMENTFLAG_LEVITATING);
        m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
    }
    
    void Aggro (Unit *pWho) {}
    
    void MoveInLineOfSight(Unit *pWho)
    {
        if (pWho->GetTypeId() != TYPEID_PLAYER)
            return;
            
        if (m_creature->GetDistance2d(pWho) >= 10.0f)
            return;
            
        if (Creature *acolyte = pWho->FindCreatureInGrid(NPC_THUZADIN_ACOLYTE, 15.0f, true))
            return;
            
        m_creature->Kill(m_creature);
    }
    
    void UpdateAI (uint32 const diff) 
    {
        m_creature->SetUnitMovementFlags(MOVEMENTFLAG_ONTRANSPORT + MOVEMENTFLAG_LEVITATING);
        m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
    }
};

CreatureAI* GetAI_npc_ashari_crystal(Creature *pCreature)
{
    return new npc_ashari_crystalAI(pCreature);
}

/*######
## AddSC
######*/

void AddSC_stratholme()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name = "go_gauntlet_gate";
    newscript->pGOHello = &GOHello_go_gauntlet_gate;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "mob_freed_soul";
    newscript->GetAI = &GetAI_mob_freed_soul;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "mob_restless_soul";
    newscript->GetAI = &GetAI_mob_restless_soul;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "mobs_spectral_ghostly_citizen";
    newscript->GetAI = &GetAI_mobs_spectral_ghostly_citizen;
    newscript->pReceiveEmote = &ReciveEmote_mobs_spectral_ghostly_citizen;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "at_timmy_the_cruel";
    newscript->pAreaTrigger = &AreaTrigger_at_timmy_the_cruel;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "go_cannonball_stack";
    newscript->pGOHello = &GOHello_go_cannonball_stack;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_ashari_crystal";
    newscript->GetAI = &GetAI_npc_ashari_crystal;
    newscript->RegisterSelf();
}

