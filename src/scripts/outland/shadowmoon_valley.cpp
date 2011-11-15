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
SDName: Shadowmoon_Valley
SD%Complete: 100
SDComment: Quest support: 10519, 10583, 10601, 10814, 10804, 10854, 10458, 10481, 10480, 11082, 10781, 10451. Vendor Drake Dealer Hurlunk.
SDCategory: Shadowmoon Valley
EndScriptData */

/* ContentData
mob_mature_netherwing_drake
mob_enslaved_netherwing_drake
npc_drake_dealer_hurlunk
npcs_flanis_swiftwing_and_kagrosh
npc_murkblood_overseer
npc_neltharaku
npc_karynaku
npc_oronok_tornheart
npc_overlord_morghor
npc_earthmender_wilda
mob_torloth_the_magnificent
mob_illidari_spawn
npc_lord_illidan_stormrage
go_crystal_prison
npc_enraged_spirit
npc_deathbringer_jovaan
npc_grand_commander_ruusk
npc_skartax
npc_invis_deathforge_caster
go_arcano_control_unit
npc_akama_prelude - quest 10944
npc_commander_arcus
npc_xiri
EndContentData */

#include "precompiled.h"
#include "EscortAI.h"

/*#####
# mob_mature_netherwing_drake
#####*/

#define SPELL_PLACE_CARCASS             38439
#define SPELL_JUST_EATEN                38502
#define SPELL_NETHER_BREATH             38467

#define SAY_JUST_EATEN                  -1000222

struct mob_mature_netherwing_drakeAI : public ScriptedAI
{
    mob_mature_netherwing_drakeAI(Creature* c) : ScriptedAI(c)
    {
        PlayerGUID = 0;
    }

    uint64 PlayerGUID;

    bool IsEating;
    bool Evade;

    uint32 ResetTimer;
    uint32 CastTimer;
    uint32 EatTimer;

    void Reset()
    {
        IsEating = false;
        Evade = false;

        ResetTimer = 120000;
        EatTimer = 5000;
        CastTimer = 5000;
    }

    void Aggro(Unit* who) { }

    void MoveInLineOfSight(Unit* who)
    {
        if(m_creature->GetMotionMaster()->GetCurrentMovementGeneratorType() == POINT_MOTION_TYPE)
            return;

        ScriptedAI::MoveInLineOfSight(who);
    }

    void SpellHit(Unit* caster, const SpellEntry* spell)
    {
        if(!caster)
            return;

        if(caster->GetTypeId() == TYPEID_PLAYER && spell->Id == SPELL_PLACE_CARCASS && !m_creature->HasAura(SPELL_JUST_EATEN, 0) && !PlayerGUID)
        {
            float PlayerX, PlayerY, PlayerZ;
            caster->GetClosePoint(PlayerX, PlayerY, PlayerZ, m_creature->GetObjectSize());
            m_creature->AddUnitMovementFlag(MOVEMENTFLAG_ONTRANSPORT + MOVEMENTFLAG_LEVITATING);
            m_creature->GetMotionMaster()->MovePoint(1, PlayerX, PlayerY, PlayerZ);
            PlayerGUID = caster->GetGUID();
        }
    }

    void MovementInform(uint32 type, uint32 id)
    {
        if(type != POINT_MOTION_TYPE)
            return;

        if(id == 1)
        {
            IsEating = true;
            EatTimer = 5000;
            m_creature->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_ATTACKUNARMED);
            m_creature->RemoveUnitMovementFlag(MOVEMENTFLAG_ONTRANSPORT + MOVEMENTFLAG_LEVITATING);
        }
    }

    void UpdateAI(const uint32 diff)
    {
        if(IsEating)
            if(EatTimer < diff)
        {
            IsEating = false;
            DoCast(m_creature, SPELL_JUST_EATEN);
            m_creature->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_NONE);
            DoScriptText(SAY_JUST_EATEN, m_creature);
            if(PlayerGUID)
            {
                Player* plr = Unit::GetPlayer(PlayerGUID);
                if(plr && plr->GetQuestStatus(10804) == QUEST_STATUS_INCOMPLETE)
                {
                    plr->KilledMonster(22131, m_creature->GetGUID());
                    Evade = true;
                    PlayerGUID = 0;
                }
            }
        }else EatTimer -= diff;

        if(Evade)
            if(ResetTimer < diff)
            {
                EnterEvadeMode();
                return;
            }else ResetTimer -= diff;

        if(!UpdateVictim())
            return;

        if(CastTimer < diff)
        {
            DoCast(m_creature->getVictim(), SPELL_NETHER_BREATH);
            CastTimer = 5000;
        }else CastTimer -= diff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_mob_mature_netherwing_drake(Creature *_creature)
{
    return new mob_mature_netherwing_drakeAI(_creature);
}

/*###
# mob_enslaved_netherwing_drake
####*/

#define FACTION_DEFAULT     62
#define FACTION_FRIENDLY    1840                            // Not sure if this is correct, it was taken off of Mordenai.

#define SPELL_HIT_FORCE_OF_NELTHARAKU   38762
#define SPELL_FORCE_OF_NELTHARAKU       38775

#define CREATURE_DRAGONMAW_SUBJUGATOR   21718
#define CREATURE_ESCAPE_DUMMY           22317

struct mob_enslaved_netherwing_drakeAI : public ScriptedAI
{
    mob_enslaved_netherwing_drakeAI(Creature* c) : ScriptedAI(c)
    {
        PlayerGUID = 0;
        Tapped = false;
    }

    uint64 PlayerGUID;
    uint32 FlyTimer;
    bool Tapped;

    void Reset()
    {
        if(!Tapped)
            m_creature->setFaction(FACTION_DEFAULT);

        FlyTimer = 10000;
        m_creature->RemoveUnitMovementFlag(MOVEMENTFLAG_ONTRANSPORT + MOVEMENTFLAG_LEVITATING);
        m_creature->SetVisibility(VISIBILITY_ON);
    }

    void Aggro(Unit* who) { }

    void SpellHit(Unit* caster, const SpellEntry* spell)
    {
        if(!caster)
            return;

        if(caster->GetTypeId() == TYPEID_PLAYER && spell->Id == SPELL_HIT_FORCE_OF_NELTHARAKU && !Tapped)
        {
            Tapped = true;
            PlayerGUID = caster->GetGUID();

            m_creature->setFaction(FACTION_FRIENDLY);
            DoCast(caster, SPELL_FORCE_OF_NELTHARAKU, true);

            Unit* Dragonmaw = FindCreature(CREATURE_DRAGONMAW_SUBJUGATOR, 50, m_creature);

            if(Dragonmaw)
            {
                m_creature->AddThreat(Dragonmaw, 100000.0f);
                AttackStart(Dragonmaw);
            }

            HostilReference* ref = m_creature->getThreatManager().getOnlineContainer().getReferenceByTarget(caster);
            if(ref)
                ref->removeReference();
        }
    }

    void MovementInform(uint32 type, uint32 id)
    {
        if(type != POINT_MOTION_TYPE)
            return;

        if(id == 1)
        {
            if(PlayerGUID)
            {
                Unit* plr = Unit::GetUnit((*m_creature), PlayerGUID);
                if(plr)
                    DoCast(plr, SPELL_FORCE_OF_NELTHARAKU, true);

                PlayerGUID = 0;
            }
            m_creature->SetVisibility(VISIBILITY_OFF);
            m_creature->RemoveUnitMovementFlag(MOVEMENTFLAG_ONTRANSPORT + MOVEMENTFLAG_LEVITATING);
            m_creature->DealDamage(m_creature, m_creature->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
            m_creature->RemoveCorpse();
        }
    }

    void UpdateAI(const uint32 diff)
    {
        if(!UpdateVictim())
        {
            if(Tapped)
                if(FlyTimer < diff)
            {
                Tapped = false;
                if(PlayerGUID)
                {
                    Player* plr = Unit::GetPlayer(PlayerGUID);
                    if(plr && plr->GetQuestStatus(10854) == QUEST_STATUS_INCOMPLETE)
                    {
                        plr->KilledMonster(22316, m_creature->GetGUID());
                        /*
                        float x,y,z;
                        m_creature->GetPosition(x,y,z);

                        float dx,dy,dz;
                        m_creature->GetRandomPoint(x, y, z, 20, dx, dy, dz);
                        dz += 20; // so it's in the air, not ground*/

                        float dx, dy, dz;

                        Unit* EscapeDummy = FindCreature(CREATURE_ESCAPE_DUMMY, 30, m_creature);
                        if(EscapeDummy)
                            EscapeDummy->GetPosition(dx, dy, dz);
                        else
                        {
                            m_creature->GetRandomPoint(m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ(), 20, dx, dy, dz);
                            dz += 25;
                        }

                        m_creature->AddUnitMovementFlag(MOVEMENTFLAG_ONTRANSPORT + MOVEMENTFLAG_LEVITATING);
                        m_creature->GetMotionMaster()->MovePoint(1, dx, dy, dz);
                    }
                }
            }else FlyTimer -= diff;
            return;
        }

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_mob_enslaved_netherwing_drake(Creature* _Creature)
{
    return new mob_enslaved_netherwing_drakeAI(_Creature);
}

/*#####
# mob_dragonmaw_peon
#####*/

enum eDragonmawPeon {
    SPELL_KICK          = 34802,
    SPELL_SUNDER_ARMOR  = 15572
};

struct mob_dragonmaw_peonAI : public ScriptedAI
{
    mob_dragonmaw_peonAI(Creature* c) : ScriptedAI(c) {}

    uint64 PlayerGUID;
    bool Tapped;
    uint32 PoisonTimer;
    uint32 KickTimer, SunderArmorTimer;

    void Reset()
    {
        PlayerGUID = 0;
        Tapped = false;
        PoisonTimer = 0;
        KickTimer = 15000;
        SunderArmorTimer = 500;
    }

    void Aggro(Unit* who) { }

    void SpellHit(Unit* caster, const SpellEntry* spell)
    {
        if(!caster)
            return;

        if(caster->GetTypeId() == TYPEID_PLAYER && spell->Id == 40468 && !Tapped)
        {
            PlayerGUID = caster->GetGUID();

            Tapped = true;
            float x, y, z;
            caster->GetClosePoint(x, y, z, m_creature->GetObjectSize());

            m_creature->RemoveUnitMovementFlag(MOVEMENTFLAG_WALK_MODE);
            m_creature->GetMotionMaster()->MovePoint(1, x, y, z);
        }
    }

    void MovementInform(uint32 type, uint32 id)
    {
        if(type != POINT_MOTION_TYPE)
            return;

        if(id)
        {
            m_creature->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_EAT);
            PoisonTimer = 5000;
        }
    }

    void UpdateAI(const uint32 diff)
    {
        if(PoisonTimer && PoisonTimer <= diff)
        {
            if(PlayerGUID)
            {
                Player* plr = Unit::GetPlayer(PlayerGUID);
                if(plr && plr->GetQuestStatus(11020) == QUEST_STATUS_INCOMPLETE)
                    plr->KilledMonster(23209, m_creature->GetGUID());
            }
            PoisonTimer = 0;
            m_creature->DealDamage(m_creature, m_creature->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
        }else PoisonTimer -= diff;
        
        if (KickTimer <= diff) {
            if (m_creature->getVictim()) {
                DoCast(m_creature->getVictim(), SPELL_KICK, true);
                KickTimer = 15000;
            }
        }else KickTimer -= diff;
        
        if (SunderArmorTimer <= diff) {
            if (m_creature->getVictim()) {
                DoCast(m_creature->getVictim(), SPELL_SUNDER_ARMOR, true);
                SunderArmorTimer = 8000;
            }
        }else SunderArmorTimer -= diff;
        
        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_mob_dragonmaw_peon(Creature* _Creature)
{
    return new mob_dragonmaw_peonAI(_Creature);
}

/*######
## npc_drake_dealer_hurlunk
######*/

bool GossipHello_npc_drake_dealer_hurlunk(Player *player, Creature *_Creature)
{
    if (_Creature->isVendor() && player->GetReputationRank(1015) == REP_EXALTED)
        player->ADD_GOSSIP_ITEM(1, GOSSIP_TEXT_BROWSE_GOODS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_TRADE);

    player->SEND_GOSSIP_MENU(_Creature->GetNpcTextId(), _Creature->GetGUID());

    return true;
}

bool GossipSelect_npc_drake_dealer_hurlunk(Player *player, Creature *_Creature, uint32 sender, uint32 action)
{
    if (action == GOSSIP_ACTION_TRADE)
        player->SEND_VENDORLIST( _Creature->GetGUID() );

    return true;
}

/*######
## npc_flanis_swiftwing_and_kagrosh
######*/

#define GOSSIP_HSK1 "Take Flanis's Pack"
#define GOSSIP_HSK2 "Take Kagrosh's Pack"

bool GossipHello_npcs_flanis_swiftwing_and_kagrosh(Player *player, Creature *_Creature)
{
    if (player->GetQuestStatus(10583) == QUEST_STATUS_INCOMPLETE && !player->HasItemCount(30658,1,true))
        player->ADD_GOSSIP_ITEM( 0, GOSSIP_HSK1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
    if (player->GetQuestStatus(10601) == QUEST_STATUS_INCOMPLETE && !player->HasItemCount(30659,1,true))
        player->ADD_GOSSIP_ITEM( 0, GOSSIP_HSK2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);

    player->SEND_GOSSIP_MENU(_Creature->GetNpcTextId(), _Creature->GetGUID());

    return true;
}

bool GossipSelect_npcs_flanis_swiftwing_and_kagrosh(Player *player, Creature *_Creature, uint32 sender, uint32 action)
{
    if (action == GOSSIP_ACTION_INFO_DEF+1)
    {
        ItemPosCountVec dest;
        uint8 msg = player->CanStoreNewItem( NULL_BAG, NULL_SLOT, dest, 30658, 1, false);
        if( msg == EQUIP_ERR_OK )
        {
            player->StoreNewItem( dest, 30658, 1, true);
            player->PlayerTalkClass->ClearMenus();
        }
    }
    if (action == GOSSIP_ACTION_INFO_DEF+2)
    {
        ItemPosCountVec dest;
        uint8 msg = player->CanStoreNewItem( NULL_BAG, NULL_SLOT, dest, 30659, 1, false);
        if( msg == EQUIP_ERR_OK )
        {
            player->StoreNewItem( dest, 30659, 1, true);
            player->PlayerTalkClass->ClearMenus();
        }
    }
    return true;
}

/*######
## npc_murkblood_overseer
######*/

#define QUEST_11082     11082

#define GOSSIP_HMO "I am here for you, overseer."
#define GOSSIP_SMO1 "How dare you question an overseer of the Dragonmaw!"
#define GOSSIP_SMO2 "Who speaks of me? What are you talking about, broken?"
#define GOSSIP_SMO3 "Continue please."
#define GOSSIP_SMO4 "Who are these bidders?"
#define GOSSIP_SMO5 "Well... yes."

bool GossipHello_npc_murkblood_overseer(Player *player, Creature *_Creature)
{
    if (player->GetQuestStatus(QUEST_11082) == QUEST_STATUS_INCOMPLETE)
        player->ADD_GOSSIP_ITEM( 0, GOSSIP_HMO, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);

    player->SEND_GOSSIP_MENU(10940, _Creature->GetGUID());
    return true;
}

bool GossipSelect_npc_murkblood_overseer(Player *player, Creature *_Creature, uint32 sender, uint32 action)
{
    switch (action)
    {
        case GOSSIP_ACTION_INFO_DEF+1:
            player->ADD_GOSSIP_ITEM(0, GOSSIP_SMO1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);
                                                            //correct id not known
            player->SEND_GOSSIP_MENU(10940, _Creature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+2:
            player->ADD_GOSSIP_ITEM(0, GOSSIP_SMO2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+3);
                                                            //correct id not known
            player->SEND_GOSSIP_MENU(10940, _Creature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+3:
            player->ADD_GOSSIP_ITEM(0, GOSSIP_SMO3, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+4);
                                                            //correct id not known
            player->SEND_GOSSIP_MENU(10940, _Creature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+4:
            player->ADD_GOSSIP_ITEM(0, GOSSIP_SMO4, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+5);
                                                            //correct id not known
            player->SEND_GOSSIP_MENU(10940, _Creature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+5:
            player->ADD_GOSSIP_ITEM(0, GOSSIP_SMO5, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+6);
                                                            //correct id not known
            player->SEND_GOSSIP_MENU(10940, _Creature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+6:
                                                            //correct id not known
            player->SEND_GOSSIP_MENU(10940, _Creature->GetGUID());
            _Creature->CastSpell(player,41121,false);
            player->AreaExploredOrEventHappens(QUEST_11082);
            break;
    }
    return true;
}

/*######
## npc_neltharaku
######*/

#define GOSSIP_HN "I am listening, dragon"
#define GOSSIP_SN1 "But you are dragons! How could orcs do this to you?"
#define GOSSIP_SN2 "Your mate?"
#define GOSSIP_SN3 "I have battled many beasts, dragon. I will help you."

bool GossipHello_npc_neltharaku(Player *player, Creature *_Creature)
{
    if (_Creature->isQuestGiver())
        player->PrepareQuestMenu( _Creature->GetGUID() );

    if (player->GetQuestStatus(10814) == QUEST_STATUS_INCOMPLETE)
        player->ADD_GOSSIP_ITEM( 0, GOSSIP_HN, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);

    player->SEND_GOSSIP_MENU(10613, _Creature->GetGUID());

    return true;
}

bool GossipSelect_npc_neltharaku(Player *player, Creature *_Creature, uint32 sender, uint32 action)
{
    switch (action)
    {
        case GOSSIP_ACTION_INFO_DEF+1:
            player->ADD_GOSSIP_ITEM( 0, GOSSIP_SN1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);
            player->SEND_GOSSIP_MENU(10614, _Creature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+2:
            player->ADD_GOSSIP_ITEM( 0, GOSSIP_SN2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+3);
            player->SEND_GOSSIP_MENU(10615, _Creature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+3:
            player->ADD_GOSSIP_ITEM( 0, GOSSIP_SN3, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+4);
            player->SEND_GOSSIP_MENU(10616, _Creature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+4:
            player->CLOSE_GOSSIP_MENU();
            player->AreaExploredOrEventHappens(10814);
            break;
    }
    return true;
}

/*######
## npc_oronok
######*/

#define GOSSIP_ORONOK1 "I am ready to hear your story, Oronok."
#define GOSSIP_ORONOK2 "How do I find the cipher?"
#define GOSSIP_ORONOK3 "How do you know all of this?"
#define GOSSIP_ORONOK4 "Yet what? What is it, Oronok?"
#define GOSSIP_ORONOK5 "Continue, please."
#define GOSSIP_ORONOK6 "So what of the cipher now? And your boys?"
#define GOSSIP_ORONOK7 "I will find your boys and the cipher, Oronok."

bool GossipHello_npc_oronok_tornheart(Player *player, Creature *_Creature)
{
    if (_Creature->isQuestGiver())
        player->PrepareQuestMenu( _Creature->GetGUID() );
    if (_Creature->isVendor())
        player->ADD_GOSSIP_ITEM(1, GOSSIP_TEXT_BROWSE_GOODS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_TRADE);

    if (player->GetQuestStatus(10519) == QUEST_STATUS_INCOMPLETE)
    {
        player->ADD_GOSSIP_ITEM( 0, GOSSIP_ORONOK1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
        player->SEND_GOSSIP_MENU(10312, _Creature->GetGUID());
    }else
    player->SEND_GOSSIP_MENU(_Creature->GetNpcTextId(), _Creature->GetGUID());

    return true;
}

bool GossipSelect_npc_oronok_tornheart(Player *player, Creature *_Creature, uint32 sender, uint32 action)
{
    switch (action)
    {
        case GOSSIP_ACTION_TRADE:
            player->SEND_VENDORLIST( _Creature->GetGUID() );
            break;
        case GOSSIP_ACTION_INFO_DEF:
            player->ADD_GOSSIP_ITEM( 0, GOSSIP_ORONOK2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
            player->SEND_GOSSIP_MENU(10313, _Creature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+1:
            player->ADD_GOSSIP_ITEM( 0, GOSSIP_ORONOK3, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);
            player->SEND_GOSSIP_MENU(10314, _Creature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+2:
            player->ADD_GOSSIP_ITEM( 0, GOSSIP_ORONOK4, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+3);
            player->SEND_GOSSIP_MENU(10315, _Creature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+3:
            player->ADD_GOSSIP_ITEM( 0, GOSSIP_ORONOK5, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+4);
            player->SEND_GOSSIP_MENU(10316, _Creature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+4:
            player->ADD_GOSSIP_ITEM( 0, GOSSIP_ORONOK6, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+5);
            player->SEND_GOSSIP_MENU(10317, _Creature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+5:
            player->ADD_GOSSIP_ITEM( 0, GOSSIP_ORONOK7, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+6);
            player->SEND_GOSSIP_MENU(10318, _Creature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+6:
            player->CLOSE_GOSSIP_MENU();
            player->AreaExploredOrEventHappens(10519);
            break;
    }
    return true;
}

/*####
# npc_karynaku
####*/

bool QuestAccept_npc_karynaku(Player* player, Creature* creature, Quest const* quest)
{
    if(quest->GetQuestId() == 10870)                        // Ally of the Netherwing
    {
        std::vector<uint32> nodes;

        nodes.resize(2);
        nodes[0] = 161;                                     // From Karynaku
        nodes[1] = 162;                                     // To Mordenai
        error_log("TSCR: Player %s started quest 10870 which has disabled taxi node, need to be fixed in core", player->GetName());
        //player->ActivateTaxiPathTo(nodes, 20811);
    }

    return true;
}

/*####
# npc_overlord_morghor
####*/

#define QUEST_LORD_ILLIDAN_STORMRAGE 11108

#define C_ILLIDAN 22083
#define C_YARZILL 23141

#define SPELL_RED_BOLT 39990 // Red Lightning Bolt
#define SPELL_MARK_OF_STORMRAGE 41528 // Mark of Stormrage

#define OVERLORD_SAY_1 -1000206
#define OVERLORD_SAY_2 -1000207
#define OVERLORD_SAY_3 -1000208
#define OVERLORD_SAY_4 -1000209
#define OVERLORD_SAY_5 -1000210
#define OVERLORD_SAY_6 -1000211

#define OVERLORD_YELL_1 -1000212
#define OVERLORD_YELL_2 -1000213

#define LORD_ILLIDAN_SAY_1 -1000214
#define LORD_ILLIDAN_SAY_2 -1000215
#define LORD_ILLIDAN_SAY_3 -1000216
#define LORD_ILLIDAN_SAY_4 -1000217
#define LORD_ILLIDAN_SAY_5 -1000218
#define LORD_ILLIDAN_SAY_6 -1000219
#define LORD_ILLIDAN_SAY_7 -1000220

#define YARZILL_THE_MERC_SAY -1000221

struct npc_overlord_morghorAI : public ScriptedAI
{
    npc_overlord_morghorAI(Creature *c) : ScriptedAI(c) {}

    uint64 PlayerGUID;
    uint64 IllidanGUID;

    uint32 ConversationTimer;
    uint32 Step;

    bool Event;

    void Reset()
    {
        PlayerGUID = 0;
        IllidanGUID = 0;

        ConversationTimer = 0;
        Step = 0;

        Event = false;
        m_creature->SetUInt32Value(UNIT_NPC_FLAGS, 2);
    }

    void Aggro(Unit* who){}

    void StartEvent()
    {
        m_creature->SetUInt32Value(UNIT_NPC_FLAGS, 0);
        m_creature->SetUInt32Value(UNIT_FIELD_BYTES_1,0);
        
        Unit* Illidan = m_creature->SummonCreature(C_ILLIDAN, -5107.83, 602.584, 85.2393, 4.92598, TEMPSUMMON_CORPSE_DESPAWN, 0);
        if (Illidan) {
            IllidanGUID = Illidan->GetGUID();
            Illidan->SetVisibility(VISIBILITY_OFF);
            Illidan->ToCreature()->SetReactState(REACT_PASSIVE);
            Illidan->setFaction(14);
            Illidan->SetMaxHealth(4200000);
            Illidan->SetHealth(Illidan->GetMaxHealth());
            Illidan->SetMaxPower(POWER_MANA, 1400000);
            Illidan->SetPower(POWER_MANA, Illidan->GetMaxPower(POWER_MANA));
        }
        
        if(PlayerGUID) {
            Player* player = Unit::GetPlayer(PlayerGUID);
            if(player)
                DoScriptText(OVERLORD_SAY_1, m_creature, player);
        }

        ConversationTimer = 4200;
        Step = 0;
        Event = true;
    }

    uint32 NextStep(uint32 Step)
    {
        Player* plr = Unit::GetPlayer(PlayerGUID);

        Unit* Illi = Unit::GetUnit((*me), IllidanGUID);

        if (!plr || (!Illi && Step < 23)) {
            EnterEvadeMode();
            return 0;
        }

        switch(Step)
        {
        case 0:
            return 0;
        case 1:
            m_creature->GetMotionMaster()->MovePoint(0, -5104.41, 595.297, 85.6838);
            return 9000;
        case 2:
            DoScriptText(OVERLORD_YELL_1, m_creature, plr);
            return 4500;
        case 3:
            m_creature->SetInFront(plr);
            return 3200;
        case 4:
            DoScriptText(OVERLORD_SAY_2, m_creature, plr);
            return 2000;
        case 5:
            Illi->SetVisibility(VISIBILITY_ON);
            Illi->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            return 350;
        case 6:
            Illi->CastSpell(Illi, SPELL_RED_BOLT, true);
            Illi->SetUInt64Value(UNIT_FIELD_TARGET, m_creature->GetGUID());
            m_creature->SetUInt64Value(UNIT_FIELD_TARGET, IllidanGUID);
            return 2000;
        case 7:
            DoScriptText(OVERLORD_YELL_2, m_creature);
            return 4500;
        case 8:
            m_creature->SetUInt32Value(UNIT_FIELD_BYTES_1, 8);
            return 2500;
        case 9:
            DoScriptText(OVERLORD_SAY_3, m_creature);
            return 6500;
        case 10:
            DoScriptText(LORD_ILLIDAN_SAY_1, Illi);
            return 5000;
        case 11:
            DoScriptText(OVERLORD_SAY_4, m_creature, plr);
            return 6000;
        case 12:
            DoScriptText(LORD_ILLIDAN_SAY_2, Illi);
            return 5500;
        case 13:
            DoScriptText(LORD_ILLIDAN_SAY_3, Illi);
            return 4000;
        case 14:
            Illi->SetUInt64Value(UNIT_FIELD_TARGET, PlayerGUID);
            return 1500;
        case 15:
            DoScriptText(LORD_ILLIDAN_SAY_4, Illi);
            return 1500;
        case 16:
            if (plr) {
                plr->CastSpell(plr, SPELL_MARK_OF_STORMRAGE, true);
                return 5000;
            }
            else {
                Step = 30;
                return 100;
            }

            break;
        case 17:
            DoScriptText(LORD_ILLIDAN_SAY_5, Illi);
            return 5000;
        case 18: 
            DoScriptText(LORD_ILLIDAN_SAY_6, Illi);
            return 5000;
        case 19: 
            DoScriptText(LORD_ILLIDAN_SAY_7, Illi);
            return 5000;
        case 20:
            Illi->HandleEmoteCommand(EMOTE_ONESHOT_LIFTOFF);
            Illi->AddUnitMovementFlag(MOVEMENTFLAG_ONTRANSPORT + MOVEMENTFLAG_LEVITATING);
            return 500;
        case 21:
            DoScriptText(OVERLORD_SAY_5, m_creature);
            return 500;
        case 22:
            Illi->SetVisibility(VISIBILITY_OFF);
            Illi->setDeathState(JUST_DIED);
            return 1000;
        case 23: 
            m_creature->SetUInt32Value(UNIT_FIELD_BYTES_1, 0);
            return 2000;
        case 24: 
            m_creature->SetUInt64Value(UNIT_FIELD_TARGET, PlayerGUID);
            return 5000;
        case 25: 
            DoScriptText(OVERLORD_SAY_6, m_creature);
            return 2000;
        case 26:
            if (plr)
                plr->GroupEventHappens(QUEST_LORD_ILLIDAN_STORMRAGE, m_creature);

            return 6000;
        case 27:
        {
            Unit* Yarzill = FindCreature(C_YARZILL, 50, m_creature);
            if (Yarzill)
                Yarzill->SetUInt64Value(UNIT_FIELD_TARGET, PlayerGUID);

            return 500;
        }
        case 28:
            plr->RemoveAurasDueToSpell(SPELL_MARK_OF_STORMRAGE);
            plr->RemoveAurasDueToSpell(41519);

            return 1000;
        case 29:
        {
            Unit* Yarzill = FindCreature(C_YARZILL, 50, m_creature);
            if(Yarzill)
                DoScriptText(YARZILL_THE_MERC_SAY, Yarzill, plr);
    
            return 5000;
        }
        case 30:
        {
            Unit* Yarzill = FindCreature(C_YARZILL, 50, m_creature);
            if (Yarzill)
                Yarzill->SetUInt64Value(UNIT_FIELD_TARGET, 0);

            return 5000;
        }
        case 31:
        {
            std::vector<uint32> nodes;

            nodes.resize(2);
            nodes[0] = 173;                                     //from
            nodes[1] = 174;                                     //end at
            plr->ActivateTaxiPathTo(nodes);

            return 1000;
        }

        case 32:
            m_creature->GetMotionMaster()->MovePoint(0, -5085.77, 577.231, 86.6719);
            return 5000;
        case 33:
            Reset();
            return 100;
        default:
            return 0;
        }
    }

    void UpdateAI(const uint32 diff)
    {
        if (!ConversationTimer)
            return;

        if(ConversationTimer <= diff) {
            if(Event && IllidanGUID && PlayerGUID)
                ConversationTimer = NextStep(++Step);
        }
        else
            ConversationTimer -= diff;
    }
};

CreatureAI* GetAI_npc_overlord_morghorAI(Creature *_Creature)
{
return new npc_overlord_morghorAI(_Creature);
}

bool QuestAccept_npc_overlord_morghor(Player *player, Creature *_Creature, const Quest *_Quest )
{
    if(_Quest->GetQuestId() == QUEST_LORD_ILLIDAN_STORMRAGE)
    {
        ((npc_overlord_morghorAI*)_Creature->AI())->PlayerGUID = player->GetGUID();
        ((npc_overlord_morghorAI*)_Creature->AI())->StartEvent();
        return true;
    }
    return false;
}

/*####
# npc_earthmender_wilda
####*/

#define SAY_START -1000223
#define SAY_AGGRO1 -1000224
#define SAY_AGGRO2 -1000225
#define ASSASSIN_SAY_AGGRO1 -1000226
#define ASSASSIN_SAY_AGGRO2 -1000227
#define SAY_PROGRESS1 -1000228
#define SAY_PROGRESS2 -1000229
#define SAY_PROGRESS3 -1000230
#define SAY_PROGRESS4 -1000231
#define SAY_PROGRESS5 -1000232
#define SAY_PROGRESS6 -1000233
#define SAY_END -1000234

#define QUEST_ESCAPE_FROM_COILSKAR_CISTERN 10451
#define NPC_COILSKAR_ASSASSIN 21044

struct npc_earthmender_wildaAI : public npc_escortAI
{
    npc_earthmender_wildaAI(Creature *c) : npc_escortAI(c) {}

    bool Completed;

    void Aggro(Unit *who)
    {
        Player* player = GetPlayerForEscort();

        if(who->GetTypeId() == TYPEID_UNIT && who->GetEntry() == NPC_COILSKAR_ASSASSIN)
            DoScriptText(SAY_AGGRO2, m_creature, player);
        else DoScriptText(SAY_AGGRO1, m_creature, player);
    }

    void Reset()
    {
        m_creature->setFaction(1726);
        Completed = false;
    }

    void WaypointReached(uint32 i)
    {
        Player* player = GetPlayerForEscort();

        if (!player)
            return;

        switch(i)
        {
               case 0: DoScriptText(SAY_START, m_creature, player); break;
               case 13: DoScriptText(SAY_PROGRESS1, m_creature, player);
                   SummonAssassin();
                   break;
               case 14: SummonAssassin(); break;
               case 15: DoScriptText(SAY_PROGRESS3, m_creature, player); break;
               case 19:
                   switch(rand()%3)
                   {
                   case 0: DoScriptText(SAY_PROGRESS2, m_creature, player); break;
                   case 1: DoScriptText(SAY_PROGRESS4, m_creature, player); break;
                   case 2: DoScriptText(SAY_PROGRESS5, m_creature, player); break;
                   }
                   break;
               case 20: SummonAssassin(); break;
               case 26:
                   switch(rand()%3)
                   {
                   case 0: DoScriptText(SAY_PROGRESS2, m_creature, player); break;
                   case 1: DoScriptText(SAY_PROGRESS4, m_creature, player); break;
                   case 2: DoScriptText(SAY_PROGRESS5, m_creature, player); break;
                   }
                   break;
               case 27: SummonAssassin(); break;
               case 33:
                   switch(rand()%3)
                   {
                   case 0: DoScriptText(SAY_PROGRESS2, m_creature, player); break;
                   case 1: DoScriptText(SAY_PROGRESS4, m_creature, player); break;
                   case 2: DoScriptText(SAY_PROGRESS5, m_creature, player); break;
                   }
                   break;
               case 34: SummonAssassin(); break;
               case 37:
                   switch(rand()%3)
                   {
                   case 0: DoScriptText(SAY_PROGRESS2, m_creature, player); break;
                   case 1: DoScriptText(SAY_PROGRESS4, m_creature, player); break;
                   case 2: DoScriptText(SAY_PROGRESS5, m_creature, player); break;
                   }
                   break;
               case 38: SummonAssassin(); break;
               case 39: DoScriptText(SAY_PROGRESS6, m_creature, player); break;
               case 43:
                   switch(rand()%3)
                   {
                   case 0: DoScriptText(SAY_PROGRESS2, m_creature, player); break;
                   case 1: DoScriptText(SAY_PROGRESS4, m_creature, player); break;
                   case 2: DoScriptText(SAY_PROGRESS5, m_creature, player); break;
                   }
                   break;
               case 44: SummonAssassin(); break;
               case 50:
                   DoScriptText(SAY_END, m_creature, player);
                   player->GroupEventHappens(QUEST_ESCAPE_FROM_COILSKAR_CISTERN, m_creature);
                   Completed = true;
                   break;
               }
       }

       void SummonAssassin()
       {
           Player* player = GetPlayerForEscort();

           Unit* CoilskarAssassin = m_creature->SummonCreature(NPC_COILSKAR_ASSASSIN, m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ(), m_creature->GetOrientation(), TEMPSUMMON_DEAD_DESPAWN, 0);
           if( CoilskarAssassin )
           {
               switch(rand()%2)
               {
               case 0: DoScriptText(ASSASSIN_SAY_AGGRO1, CoilskarAssassin, player); break;
               case 1: DoScriptText(ASSASSIN_SAY_AGGRO2, CoilskarAssassin, player); break;
               }
               (CoilskarAssassin->ToCreature())->AI()->AttackStart(m_creature);
           }
           else error_log("TSCR ERROR: Coilskar Assassin couldn't be summmoned");
       }

       void JustDied(Unit* killer)
       {
           if (PlayerGUID && !Completed)
           {
               Player* player = GetPlayerForEscort();
               if (player)
                   player->FailQuest(QUEST_ESCAPE_FROM_COILSKAR_CISTERN);
           }
       }

       void UpdateAI(const uint32 diff)
       {
               npc_escortAI::UpdateAI(diff);
       }
};

CreatureAI* GetAI_npc_earthmender_wildaAI(Creature *pCreature)
{
    return new npc_earthmender_wildaAI(pCreature);
}

bool QuestAccept_npc_earthmender_wilda(Player* player, Creature* creature, Quest const* quest)
{
    if (quest->GetQuestId() == QUEST_ESCAPE_FROM_COILSKAR_CISTERN)
    {
        creature->setFaction(113);
        ((npc_escortAI*)(creature->AI()))->Start(true, true, false, player->GetGUID(), creature->GetEntry());
    }
    return true;
}

/*#####
# Quest: Battle of the crimson watch
#####*/

/* ContentData
Battle of the crimson watch - creatures, gameobjects and defines
mob_illidari_spawn : Adds that are summoned in the Crimson Watch battle.
mob_torloth_the_magnificent : Final creature that players have to face before quest is completed
npc_lord_illidan_stormrage : Creature that controls the event.
go_crystal_prison : GameObject that begins the event and hands out quest
EndContentData */

#define END_TEXT -1000366

#define QUEST_BATTLE_OF_THE_CRIMSON_WATCH 10781
#define EVENT_AREA_RADIUS 65 //65yds
#define EVENT_COOLDOWN 30000 //in ms. appear after event completed or failed (should be = Adds despawn time)

struct TorlothCinematic
{
    int32 TextId;
    uint32 Creature, Timer;
};

// Creature 0 - Torloth, 1 - Illidan
static TorlothCinematic TorlothAnim[]=
{
    {-1000367, 0, 2000},
    {-1000368, 1, 7000},
    {-1000369, 0, 3000},
    {NULL, 0, 2000}, // Torloth stand
    {-1000370, 0, 1000},
    {NULL, 0, 3000},
    {NULL, 0, NULL}
};

struct Location
{
    float x, y, z, o;
};

//Cordinates for Spawns
static Location SpawnLocation[]=
{
    //Cords used for:
    {-4615.8556, 1342.2532, 139.9, 1.612},//Illidari Soldier
    {-4598.9365, 1377.3182, 139.9, 3.917},//Illidari Soldier
    {-4598.4697, 1360.8999, 139.9, 2.427},//Illidari Soldier
    {-4589.3599, 1369.1061, 139.9, 3.165},//Illidari Soldier
    {-4608.3477, 1386.0076, 139.9, 4.108},//Illidari Soldier
    {-4633.1889, 1359.8033, 139.9, 0.949},//Illidari Soldier
    {-4623.5791, 1351.4574, 139.9, 0.971},//Illidari Soldier
    {-4607.2988, 1351.6099, 139.9, 2.416},//Illidari Soldier
    {-4633.7764, 1376.0417, 139.9, 5.608},//Illidari Soldier
    {-4600.2461, 1369.1240, 139.9, 3.056},//Illidari Mind Breaker
    {-4631.7808, 1367.9459, 139.9, 0.020},//Illidari Mind Breaker
    {-4600.2461, 1369.1240, 139.9, 3.056},//Illidari Highlord
    {-4631.7808, 1367.9459, 139.9, 0.020},//Illidari Highlord
    {-4615.5586, 1353.0031, 139.9, 1.540},//Illidari Highlord
    {-4616.4736, 1384.2170, 139.9, 4.971},//Illidari Highlord
    {-4627.1240, 1378.8752, 139.9, 2.544} //Torloth The Magnificent
};

struct WaveData
{
    uint8 SpawnCount, UsedSpawnPoint;
    uint32 CreatureId, SpawnTimer,YellTimer;
    int32 WaveTextId;
};

static WaveData WavesInfo[]=
{
    {9, 0, 22075, 10000, 7000, -1000371},   //Illidari Soldier
    {2, 9, 22074, 10000, 7000, -1000372},   //Illidari Mind Breaker
    {4, 11, 19797, 10000, 7000, -1000373},  //Illidari Highlord
    {1, 15, 22076, 10000, 7000, -1000374}   //Torloth The Magnificent
};

struct SpawnSpells
{
 uint32 Timer1, Timer2, SpellId;
};

static SpawnSpells SpawnCast[]=
{
    {10000, 15000, 35871},  // Illidari Soldier Cast - Spellbreaker
    {10000, 10000, 38985},  // Illidari Mind Breake Cast - Focused Bursts
    {35000, 35000, 22884},  // Illidari Mind Breake Cast - Psychic Scream
    {20000, 20000, 17194},  // Illidari Mind Breake Cast - Mind Blast
    {8000, 15000, 38010},   // Illidari Highlord Cast - Curse of Flames
    {12000, 20000, 16102},  // Illidari Highlord Cast - Flamestrike
    {10000, 15000, 15284},  // Torloth the Magnificent Cast - Cleave
    {18000, 20000, 39082},  // Torloth the Magnificent Cast - Shadowfury
    {25000, 28000, 33961}   // Torloth the Magnificent Cast - Spell Reflection
};

/*######
# mob_illidari_spawn
######*/

struct mob_illidari_spawnAI : public ScriptedAI
{
    mob_illidari_spawnAI(Creature* c) : ScriptedAI(c)
    {
        LordIllidanGUID = 0;
    }

    uint64 LordIllidanGUID;
    uint32 SpellTimer1, SpellTimer2, SpellTimer3;
    bool Timers;

    void Reset()
    {
        Timers = false;
    }

    void Aggro(Unit* who) {}
    void JustDied(Unit* slayer);

    void UpdateAI(const uint32 diff)
    {
        if(!UpdateVictim())
            return;

        if(!Timers)
        {
            if(m_creature->GetEntry() == 22075)//Illidari Soldier
            {
                SpellTimer1 = SpawnCast[0].Timer1 + (rand()%4 * 1000);
            }
            if(m_creature->GetEntry() == 22074)//Illidari Mind Breaker
            {
                SpellTimer1 = SpawnCast[1].Timer1 + (rand()%10 * 1000);
                SpellTimer2 = SpawnCast[2].Timer1 + (rand()%4 * 1000);
                SpellTimer3 = SpawnCast[3].Timer1 + (rand()%4 * 1000);
            }
            if(m_creature->GetEntry() == 19797)// Illidari Highlord
            {
                SpellTimer1 = SpawnCast[4].Timer1 + (rand()%4 * 1000);
                SpellTimer2 = SpawnCast[5].Timer1 + (rand()%4 * 1000);
            }
            Timers = true;
        }
        //Illidari Soldier
        if(m_creature->GetEntry() == 22075)
        {
            if(SpellTimer1 < diff)
            {
                DoCast(m_creature->getVictim(), SpawnCast[0].SpellId);//Spellbreaker
                SpellTimer1 = SpawnCast[0].Timer2 + (rand()%5 * 1000);
            }else SpellTimer1 -= diff;
        }
        //Illidari Mind Breaker
        if(m_creature->GetEntry() == 22074)
        {
            if(SpellTimer1 < diff)
            {
                if(Unit *target = SelectUnit(SELECT_TARGET_RANDOM,0))
                {
                    if(target->GetTypeId() == TYPEID_PLAYER)
                    {
                        DoCast(target, SpawnCast[1].SpellId); //Focused Bursts
                        SpellTimer1 = SpawnCast[1].Timer2 + (rand()%5 * 1000);
                    }else SpellTimer1 = 2000;
                }
            }else SpellTimer1 -= diff;

            if(SpellTimer2 < diff)
            {
                DoCast(m_creature->getVictim(), SpawnCast[2].SpellId);//Psychic Scream
                SpellTimer2 = SpawnCast[2].Timer2 + (rand()%13 * 1000);
            }else SpellTimer2 -= diff;

            if(SpellTimer3 < diff)
            {
                DoCast(m_creature->getVictim(), SpawnCast[3].SpellId);//Mind Blast
                SpellTimer3 = SpawnCast[3].Timer2 + (rand()%8 * 1000);
            }else SpellTimer3 -= diff;
        }
        //Illidari Highlord
        if(m_creature->GetEntry() == 19797)
        {
            if(SpellTimer1 < diff)
            {
                DoCast(m_creature->getVictim(), SpawnCast[4].SpellId);//Curse Of Flames
                SpellTimer1 = SpawnCast[4].Timer2 + (rand()%10 * 1000);
            }else SpellTimer1 -= diff;

            if(SpellTimer2 < diff)
            {
                DoCast(m_creature->getVictim(), SpawnCast[5].SpellId);//Flamestrike
                SpellTimer2 = SpawnCast[5].Timer2 + (rand()%7 * 13000);
            }else SpellTimer2 -= diff;
        }

        DoMeleeAttackIfReady();
    }
};

/*######
# mob_torloth_the_magnificent
#####*/

struct mob_torloth_the_magnificentAI : public ScriptedAI
{
    mob_torloth_the_magnificentAI(Creature* c) : ScriptedAI(c) {}

    uint32 AnimationTimer, SpellTimer1, SpellTimer2, SpellTimer3;

    uint8 AnimationCount;

    uint64 LordIllidanGUID;
    uint64 AggroTargetGUID;

    bool Timers;

    void Reset()
    {
        AnimationTimer = 4000;
        AnimationCount = 0;
        LordIllidanGUID = 0;
        AggroTargetGUID = 0;
        Timers = false;

        m_creature->addUnitState(UNIT_STAT_ROOT);
        m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        m_creature->SetUInt64Value(UNIT_FIELD_TARGET, 0);
    }

    void Aggro(Unit* who){}

    void HandleAnimation()
    {
        Creature* pCreature = m_creature;

        if(TorlothAnim[AnimationCount].Creature == 1)
        {
            pCreature = (Unit::GetCreature(*m_creature, LordIllidanGUID));

            if(!pCreature)
                return;
        }

        if(TorlothAnim[AnimationCount].TextId)
            DoScriptText(TorlothAnim[AnimationCount].TextId, pCreature);

        AnimationTimer = TorlothAnim[AnimationCount].Timer;

        switch(AnimationCount)
        {
        case 0:
            m_creature->SetUInt32Value(UNIT_FIELD_BYTES_1,8);
            break;
        case 3:
            m_creature->RemoveFlag(UNIT_FIELD_BYTES_1,8);
            break;
        case 5:
            if(Player* AggroTarget = (Unit::GetPlayer(AggroTargetGUID)))
            {
                m_creature->SetUInt64Value(UNIT_FIELD_TARGET, AggroTarget->GetGUID());
                m_creature->AddThreat(AggroTarget, 1);
                m_creature->HandleEmoteCommand(EMOTE_ONESHOT_POINT);
            }
            break;
        case 6:
            if(Player* AggroTarget = (Unit::GetPlayer(AggroTargetGUID)))
            {
                m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                m_creature->clearUnitState(UNIT_STAT_ROOT);

                float x, y, z;
                AggroTarget->GetPosition(x,y,z);
                m_creature->GetMotionMaster()->MovePoint(0,x,y,z);
            }
            break;
        }
        ++AnimationCount;
    }

    void UpdateAI(const uint32 diff)
    {
        if(AnimationTimer)
        {
            if(AnimationTimer <= diff)
            {
                HandleAnimation();
            }else AnimationTimer -= diff;
        }

        if(AnimationCount < 6)
        {
            m_creature->CombatStop();
        }else if(!Timers)
        {

            SpellTimer1 = SpawnCast[6].Timer1;
            SpellTimer2 = SpawnCast[7].Timer1;
            SpellTimer3 = SpawnCast[8].Timer1;
            Timers = true;
        }

        if(Timers)
        {
            if(SpellTimer1 < diff)
            {
                DoCast(m_creature->getVictim(), SpawnCast[6].SpellId);//Cleave
                SpellTimer1 = SpawnCast[6].Timer2 + (rand()%10 * 1000);
            }else SpellTimer1 -= diff;

            if(SpellTimer2 < diff)
            {
                DoCast(m_creature->getVictim(), SpawnCast[7].SpellId);//Shadowfury
                SpellTimer2 = SpawnCast[7].Timer2 + (rand()%5 * 1000);
            }else SpellTimer2 -= diff;

            if(SpellTimer3 < diff)
            {
                DoCast(m_creature, SpawnCast[8].SpellId);
                SpellTimer3 = SpawnCast[8].Timer2 + (rand()%7 * 1000);//Spell Reflection
            }else SpellTimer3 -= diff;
        }

        DoMeleeAttackIfReady();
    }

    void JustDied(Unit* slayer)
    {
        if(slayer)
            switch(slayer->GetTypeId())
        {
            case TYPEID_UNIT:
                if((slayer->ToCreature())->isPet() && ((Pet*)slayer)->GetOwner()->GetTypeId() == TYPEID_PLAYER)
                    ((Pet*)slayer)->GetOwner()->ToPlayer()->GroupEventHappens(QUEST_BATTLE_OF_THE_CRIMSON_WATCH, m_creature);
                break;

            case TYPEID_PLAYER:
                (slayer->ToPlayer())->GroupEventHappens(QUEST_BATTLE_OF_THE_CRIMSON_WATCH, m_creature);
                break;
        }

        if(Creature* LordIllidan = (Unit::GetCreature(*m_creature, LordIllidanGUID)))
        {
            DoScriptText(END_TEXT, LordIllidan, slayer);
            LordIllidan->AI()->EnterEvadeMode();
        }
    }
};

/*#####
# npc_lord_illidan_stormrage
#####*/

struct npc_lord_illidan_stormrageAI : public ScriptedAI
{
    npc_lord_illidan_stormrageAI(Creature* c) : ScriptedAI(c) {}

    uint64 PlayerGUID;

    uint32 WaveTimer;
    uint32 AnnounceTimer;

    int8 LiveCount;
    uint8 WaveCount;

    bool EventStarted;
    bool Announced;
    bool Failed;

    void Reset()
    {
        PlayerGUID = 0;

        WaveTimer = 10000;
        AnnounceTimer = 7000;
        LiveCount = 0;
        WaveCount = 0;

        EventStarted = false;
        Announced = false;
        Failed = false;

        m_creature->SetVisibility(VISIBILITY_OFF);
    }

    void Aggro(Unit* who) {}
    void MoveInLineOfSight(Unit* who) {}
    void AttackStart(Unit* who) {}

    void SummonNextWave()
    {
        uint8 count = WavesInfo[WaveCount].SpawnCount;
        uint8 locIndex = WavesInfo[WaveCount].UsedSpawnPoint;
        srand(time(NULL));//initializing random seed
        uint8 FelguardCount = 0;
        uint8 DreadlordCount = 0;

        for(uint8 i = 0; i < count; ++i)
        {
            Creature* Spawn = NULL;
            float X = SpawnLocation[locIndex + i].x;
            float Y = SpawnLocation[locIndex + i].y;
            float Z = SpawnLocation[locIndex + i].z;
            float O = SpawnLocation[locIndex + i].o;
            Spawn = m_creature->SummonCreature(WavesInfo[WaveCount].CreatureId, X, Y, Z, O, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 60000);
            ++LiveCount;

            if(Spawn)
            {
                Spawn->LoadCreaturesAddon();

                if(WaveCount == 0)//1 Wave
                {
                    if(rand()%3 == 1 && FelguardCount<2)
                    {
                        Spawn->SetUInt32Value(UNIT_FIELD_DISPLAYID,18654);
                        ++FelguardCount;
                    }
                    else if(DreadlordCount < 3)
                    {
                        Spawn->SetUInt32Value(UNIT_FIELD_DISPLAYID,19991);
                        ++DreadlordCount;
                    }
                    else if(FelguardCount<2)
                    {
                        Spawn->SetUInt32Value(UNIT_FIELD_DISPLAYID,18654);
                        ++FelguardCount;
                    }
                }

                if(WaveCount < 3)//1-3 Wave
                {
                    if(PlayerGUID)
                    {
                        if(Player* pTarget = Unit::GetPlayer(PlayerGUID))
                        {
                            float x, y, z;
                            pTarget->GetPosition(x,y,z);
                            Spawn->GetMotionMaster()->MovePoint(0,x, y, z);
                        }
                    }
                    ((mob_illidari_spawnAI*)Spawn->AI())->LordIllidanGUID = m_creature->GetGUID();
                }

                if(WavesInfo[WaveCount].CreatureId == 22076) // Torloth
                {
                    ((mob_torloth_the_magnificentAI*)Spawn->AI())->LordIllidanGUID = m_creature->GetGUID();
                    if(PlayerGUID)
                        ((mob_torloth_the_magnificentAI*)Spawn->AI())->AggroTargetGUID = PlayerGUID;
                }
            }
        }
        ++WaveCount;
        WaveTimer = WavesInfo[WaveCount].SpawnTimer;
        AnnounceTimer = WavesInfo[WaveCount].YellTimer;
    }
    
    void SummonedCreatureDespawn(Creature* creature)
    {
        LiveCounter();
    }

    void CheckEventFail()
    {
        Player* pPlayer = Unit::GetPlayer(PlayerGUID);

        if (!pPlayer) {
            Failed = true;
            EventStarted = false;
            EnterEvadeMode();
            
            return;
        }

        if(Group *EventGroup = pPlayer->GetGroup())
        {
            Player* GroupMember;

            uint8 GroupMemberCount = 0;
            uint8 DeadMemberCount = 0;
            uint8 FailedMemberCount = 0;

            const Group::MemberSlotList members = EventGroup->GetMemberSlots();

            for(Group::member_citerator itr = members.begin(); itr!= members.end(); itr++)
            {
                GroupMember = (Unit::GetPlayer(itr->guid));
                if(!GroupMember)
                    continue;
                if(!GroupMember->IsWithinDistInMap(m_creature, EVENT_AREA_RADIUS) && GroupMember->GetQuestStatus(QUEST_BATTLE_OF_THE_CRIMSON_WATCH) == QUEST_STATUS_INCOMPLETE)
                {
                    GroupMember->FailQuest(QUEST_BATTLE_OF_THE_CRIMSON_WATCH);
                    GroupMember->SetQuestStatus(QUEST_BATTLE_OF_THE_CRIMSON_WATCH, QUEST_STATUS_NONE);
                    ++FailedMemberCount;
                }
                ++GroupMemberCount;

                if(GroupMember->isDead())
                {
                    ++DeadMemberCount;
                }
            }

            if(GroupMemberCount == FailedMemberCount)
            {
                Failed = true;
            }

            if(GroupMemberCount == DeadMemberCount)
            {
                for(Group::member_citerator itr = members.begin(); itr!= members.end(); itr++)
                {
                    GroupMember = Unit::GetPlayer(itr->guid);

                    if(GroupMember && GroupMember->GetQuestStatus(QUEST_BATTLE_OF_THE_CRIMSON_WATCH) == QUEST_STATUS_INCOMPLETE)
                    {
                        GroupMember->FailQuest(QUEST_BATTLE_OF_THE_CRIMSON_WATCH);
                        GroupMember->SetQuestStatus(QUEST_BATTLE_OF_THE_CRIMSON_WATCH, QUEST_STATUS_NONE);
                    }
                }
                Failed = true;
            }
        }else if (pPlayer->isDead() || !pPlayer->IsWithinDistInMap(m_creature, EVENT_AREA_RADIUS))
        {
            pPlayer->FailQuest(QUEST_BATTLE_OF_THE_CRIMSON_WATCH);
            Failed = true;
            EventStarted = false;
        }
    }

    void LiveCounter()
    {
        --LiveCount;
        if(!LiveCount)
            Announced = false;
    }

    void UpdateAI(const uint32 diff)
    {
        if(!PlayerGUID || !EventStarted)
            return;

        if(!LiveCount && WaveCount < 4)
        {
            if(!Announced && AnnounceTimer < diff)
            {
                DoScriptText(WavesInfo[WaveCount].WaveTextId, m_creature);
                Announced = true;
            }else AnnounceTimer -= diff;

            if(WaveTimer < diff)
            {
                SummonNextWave();
            }else WaveTimer -= diff;
        }
        CheckEventFail();

        if (Failed)
            EnterEvadeMode();
    }
};

void mob_illidari_spawnAI::JustDied(Unit *slayer)
{
    m_creature->RemoveCorpse();
}

/*#####
# go_crystal_prison
######*/

bool GOQuestAccept_GO_crystal_prison(Player* plr, GameObject* go, Quest const* quest)
{
    if(quest->GetQuestId() == QUEST_BATTLE_OF_THE_CRIMSON_WATCH )
    {
        Unit* Illidan = FindCreature(22083, 50, plr);

        if(Illidan && !(((npc_lord_illidan_stormrageAI*)(Illidan->ToCreature())->AI())->EventStarted))
        {
            ((npc_lord_illidan_stormrageAI*)(Illidan->ToCreature())->AI())->PlayerGUID = plr->GetGUID();
            ((npc_lord_illidan_stormrageAI*)(Illidan->ToCreature())->AI())->LiveCount = 0;
            ((npc_lord_illidan_stormrageAI*)(Illidan->ToCreature())->AI())->EventStarted=true;
        }
    }
 return true;
}

CreatureAI* GetAI_npc_lord_illidan_stormrage(Creature* c)
{
    return new npc_lord_illidan_stormrageAI(c);
}

CreatureAI* GetAI_mob_illidari_spawn(Creature* c)
{
    return new mob_illidari_spawnAI(c);
}

CreatureAI* GetAI_mob_torloth_the_magnificent(Creature* c)
{
    return new mob_torloth_the_magnificentAI(c);
}

/*####
# npc_enraged_spirits
####*/

/* QUESTS */
#define QUEST_ENRAGED_SPIRITS_FIRE_EARTH 10458
#define QUEST_ENRAGED_SPIRITS_AIR 10481
#define QUEST_ENRAGED_SPIRITS_WATER 10480

/* Totem */
#define ENTRY_TOTEM_OF_SPIRITS 21071
#define RADIUS_TOTEM_OF_SPIRITS 15

/* SPIRITS */
#define ENTRY_ENRAGED_EARTH_SPIRIT 21050
#define ENTRY_ENRAGED_FIRE_SPIRIT 21061
#define ENTRY_ENRAGED_AIR_SPIRIT 21060
#define ENTRY_ENRAGED_WATER_SPIRIT 21059

/* SOULS */
#define ENTRY_EARTHEN_SOUL 21073
#define ENTRY_FIERY_SOUL 21097
#define ENTRY_ENRAGED_AIRY_SOUL 21116
#define ENTRY_ENRAGED_WATERY_SOUL 21109  // wrong model

/* SPELL KILLCREDIT - not working!?! - using KilledMonster */
#define SPELL_EARTHEN_SOUL_CAPTURED_CREDIT 36108
#define SPELL_FIERY_SOUL_CAPTURED_CREDIT 36117
#define SPELL_AIRY_SOUL_CAPTURED_CREDIT 36182
#define SPELL_WATERY_SOUL_CAPTURED_CREDIT 36171

/* KilledMonster Workaround */
#define CREDIT_FIRE 21094
#define CREDIT_WATER 21095
#define CREDIT_AIR 21096
#define CREDIT_EARTH 21092

/* Captured Spell/Buff */
#define SPELL_SOUL_CAPTURED 36115

/* Factions */
#define ENRAGED_SOUL_FRIENDLY 35
#define ENRAGED_SOUL_HOSTILE 14

struct npc_enraged_spiritAI : public ScriptedAI
{
    npc_enraged_spiritAI(Creature *c) : ScriptedAI(c) {}

    void Reset()   { }

    void Aggro(Unit *who){}

    void JustDied(Unit* killer)
    {
        // always spawn spirit on death
        // if totem around
        // move spirit to totem and cast kill count
        uint32 entry = 0;
        uint32 credit = 0;

        switch(m_creature->GetEntry()) {
          case ENTRY_ENRAGED_FIRE_SPIRIT:
            entry  = ENTRY_FIERY_SOUL;
            //credit = SPELL_FIERY_SOUL_CAPTURED_CREDIT;
            credit = CREDIT_FIRE;
          break;
          case ENTRY_ENRAGED_EARTH_SPIRIT:
            entry  = ENTRY_EARTHEN_SOUL;
            //credit = SPELL_EARTHEN_SOUL_CAPTURED_CREDIT;
            credit = CREDIT_EARTH;
          break;
          case ENTRY_ENRAGED_AIR_SPIRIT:
            entry  = ENTRY_ENRAGED_AIRY_SOUL;
            //credit = SPELL_AIRY_SOUL_CAPTURED_CREDIT;
            credit = CREDIT_AIR;
          break;
          case ENTRY_ENRAGED_WATER_SPIRIT:
            entry  = ENTRY_ENRAGED_WATERY_SOUL;
            //credit = SPELL_WATERY_SOUL_CAPTURED_CREDIT;
            credit = CREDIT_WATER;
          break;
        }

        // Spawn Soul on Kill ALWAYS!
        Creature* Summoned = NULL;
        Unit* totemOspirits = NULL;

        if ( entry != 0 )
            Summoned = DoSpawnCreature(entry, 0, 0, 1, 0, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 5000);

        // FIND TOTEM, PROCESS QUEST
        if (Summoned)
        {
             totemOspirits = FindCreature(ENTRY_TOTEM_OF_SPIRITS, RADIUS_TOTEM_OF_SPIRITS, m_creature);
             if (totemOspirits)
             {
                 Summoned->setFaction(ENRAGED_SOUL_FRIENDLY);
                 Summoned->GetMotionMaster()->MovePoint(0,totemOspirits->GetPositionX(), totemOspirits->GetPositionY(), Summoned->GetPositionZ());

                 Player* Owner = totemOspirits->GetOwner()->ToPlayer();
                 if (Owner)
                     // DoCast(Owner, credit); -- not working!
                     Owner->KilledMonster(credit, Summoned->GetGUID());
                 DoCast(totemOspirits,SPELL_SOUL_CAPTURED);
             }
        }
    }
};

CreatureAI* GetAI_npc_enraged_spirit(Creature *_Creature)
{
return new npc_enraged_spiritAI(_Creature);
}

/*######
## npc_spirits_totem
######*/

struct npc_spirits_totemAI : public ScriptedAI
{
    npc_spirits_totemAI(Creature* c) : ScriptedAI(c) {}
    
    uint32 checkTimer;
    
    void Reset()
    {
        checkTimer = 1000;
    }
    
    void Aggro(Unit* who) {}
    
    void UpdateAI(uint32 const diff)
    {
        uint32 souls[] = {21073, 21097, 21116, 21109}, credits[] = {21092, 21094, 21096, 21095};
    
        if (!me->GetOwner())
            return;
            
        if (!me->GetOwner()->ToPlayer())
            return;

        if (checkTimer <= diff) {
            for (uint8 i = 0; i < 4; i++) {
                if (Creature* soul = me->FindNearestCreature(souls[i], 30.0f, true)) {
                    me->GetOwner()->ToPlayer()->KilledMonster(credits[i], soul->GetGUID());
                    DoCast(me, SPELL_SOUL_CAPTURED);
                    soul->DisappearAndDie();
                }
            }
            
            checkTimer = 1000;
        }
        else
            checkTimer -= diff;
    }
};

CreatureAI* GetAI_npc_spirits_totem(Creature* creature)
{
    return new npc_spirits_totemAI(creature);
}

/*######
## npc_deathbringer_jovaan
######*/

const char* Conv[] = {"Everything is in readiness, warbringer.",
                        "Doom Lord Kazzak will be pleased. You are to increase the pace of your attacks. Destroy the orcish and dwarven strongholds with all haste.",
                        "Warbringer, that will require the use of all the hold's infernals. It may leave us vulnerable to a counterattack.",
                        "Don't worry about that. I've increased production at the Deathforge. You'll have all the infernals you need to carry out your orders. Don't fail, Jovaan.",
                        "It shall be as you say, warbringer. One last question, if I may...",
                        "Yes?",
                        "What's in the crate?",
                        "Crate? I didn't send you a crate, Jovaan. Don't you have more important things to worry about? Go see to them!"};

struct npc_deathbringer_jovaanAI : public ScriptedAI
{
    npc_deathbringer_jovaanAI(Creature *c) : ScriptedAI(c) {}
 
    uint32 convTimer;
    uint8 convIndex;
    bool jovaanTurn; //determine which NPC should talk at each timer
    
    Creature* razuun;
    
    void Reset()
    {
        convTimer = 5000;
        convIndex = 0;
        jovaanTurn = true;
        
        razuun = m_creature->SummonCreature(21502, -3300.373291, 2927.192139, 173.890091, 2.625873, TEMPSUMMON_CORPSE_DESPAWN, 80000);
    }
    
    void Aggro(Unit* who) {}
    
    void UpdateAI(const uint32 diff)
    {
        if (convIndex == 8) //completion of quest is done in SpellEffects.cpp, just despawn the two creatures
        {
            if (razuun)
            {
                m_creature->DealDamage(razuun, razuun->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
                razuun->RemoveCorpse();
            }
            
            if (m_creature)
            {
                m_creature->DealDamage(m_creature, m_creature->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
                m_creature->RemoveCorpse();
            }
            
            return;
        }
        
        if (convTimer <= diff)
        {
            if (!razuun)
                return; //prevent crash
                
            if (jovaanTurn)
                m_creature->MonsterSay(Conv[convIndex], LANG_UNIVERSAL, 0);
            else
                razuun->MonsterSay(Conv[convIndex], LANG_UNIVERSAL, 0);

            jovaanTurn = !jovaanTurn;            
            convTimer = 5000;
            convIndex++;
        }else convTimer -= diff;
    }
};

CreatureAI* GetAI_npc_deathbringer_jovaan(Creature *pCreature)
{
return new npc_deathbringer_jovaanAI(pCreature);
}

/*######
## npc_grand_commander_ruusk
######*/

#define GOSSIP_1 "[PH] Remettre le message a Ruusk"

#define QUEST_WHAT_ILLIDAN_WANTS 10577

bool GossipHello_npc_grand_commander_ruusk(Player* pPlayer, Creature* pCreature)
{
    if (pPlayer->GetQuestStatus(QUEST_WHAT_ILLIDAN_WANTS) == QUEST_STATUS_INCOMPLETE) {
        pPlayer->ADD_GOSSIP_ITEM(0, GOSSIP_1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
        pPlayer->SEND_GOSSIP_MENU(9420, pCreature->GetGUID());
        
        return true;
    }
    
    return false;
}

bool GossipSelect_npc_grand_commander_ruusk(Player* pPlayer, Creature* pCreature, uint32 sender, uint32 action)
{
    if (action == GOSSIP_ACTION_INFO_DEF+1) {
        pPlayer->CompleteQuest(QUEST_WHAT_ILLIDAN_WANTS);
        pPlayer->CLOSE_GOSSIP_MENU();
        return true;
    }
    
    return false;
}

/*######
## npc_skartax
######*/

#define SPELL_PURPLE_BEAM		36384
#define SPELL_AUTO_AURA			36382
#define SPELL_SHADOWBOLT		12471
#define SPELL_INCINERATE		38401

struct npc_skartaxAI : public ScriptedAI
{
	npc_skartaxAI(Creature *c) : ScriptedAI(c) {}
	
	uint32 SummonTimer;
	uint32 ShadowBoltTimer;
	uint32 IncinerateTimer;
	
	void Reset()
	{
		m_creature->AddAura(SPELL_AUTO_AURA, m_creature);
		DoCast(m_creature, SPELL_PURPLE_BEAM);
		
		SummonTimer = 2000;
		ShadowBoltTimer = 2000;
		IncinerateTimer = 500;
	}
	void Aggro(Unit *pWho)
	{
		m_creature->InterruptNonMeleeSpells(true);
		m_creature->RemoveAurasDueToSpell(SPELL_AUTO_AURA);
	}
	
	void UpdateAI(uint32 const diff)
	{
		if (SummonTimer <= diff) {
			m_creature->SummonCreature(19757, -3368.94, 2145.35, -8.28, 0.382, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 180000);
			SummonTimer = 180000;
		}
		else
			SummonTimer -= diff;
			
		if (ShadowBoltTimer <= diff) {
			DoCast(m_creature->getVictim(), SPELL_SHADOWBOLT);
			ShadowBoltTimer = 5000;
		}
		else
			ShadowBoltTimer -= diff;
			
		if (IncinerateTimer <= diff) {
			DoCast(m_creature->getVictim(), SPELL_INCINERATE);
			IncinerateTimer = 10000;
		}
		else
			IncinerateTimer -= diff;
			
		DoMeleeAttackIfReady();
	}
};

CreatureAI* GetAI_npc_skartax(Creature *pCreature)
{
	return new npc_skartaxAI(pCreature);
}

/*######
## npc_invis_deathforge_caster
######*/

struct npc_invis_deathforge_casterAI : public ScriptedAI
{
	npc_invis_deathforge_casterAI(Creature *c) : ScriptedAI(c) {}
	
	void Aggro(Unit *pWho) {}
	
	void UpdateAI(uint32 const diff)
	{
		if (Creature *pTarget = m_creature->FindNearestCreature(21207, 30.0f)) {
			if (!pTarget->HasAura(36384))
				DoCast(pTarget, 36384);
		}
	}
};

CreatureAI* GetAI_npc_invis_deathforge_caster(Creature *pCreature)
{
	return new npc_invis_deathforge_casterAI(pCreature);
}

/*######
## go_arcano_control_unit
######*/

bool GOHello_go_arcano_control_unit(Player *pPlayer, GameObject* pGo)
{
	if (Creature *pCreature = pPlayer->FindNearestCreature(21909, 25.0f)) {
		pPlayer->CastSpell(pCreature, 37868, true);
		pCreature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
		
		return true;
	}
	
	return false;
}

/*######
## npc_quest_spectrecles
######*/

bool GossipHello_npc_quest_spectrecles(Player* player, Creature* creature)
{
    if (creature->isQuestGiver())
        player->PrepareQuestMenu(creature->GetGUID());

    if (player->GetQuestStatus(10625) == QUEST_STATUS_INCOMPLETE && !player->HasItemCount(30719, 1, true))
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "J'ai perdu mes Lunectoplasmes", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
    if (player->GetQuestStatus(10643) == QUEST_STATUS_INCOMPLETE && !player->HasItemCount(30719, 1, true))
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "J'ai perdu mes Lunectoplasmes", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);

    player->SEND_GOSSIP_MENU(creature->GetNpcTextId(), creature->GetGUID());

    return true;
}

bool GossipSelect_npc_quest_spectrecles(Player* player, Creature* creature, uint32 sender, uint32 action)
{
    if (action == GOSSIP_ACTION_INFO_DEF) {
        ItemPosCountVec dest;
        uint8 msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, 30719, 1, false);
        if (msg == EQUIP_ERR_OK)
            player->StoreNewItem(dest, 30719, 1, true);
    }
    
    player->CLOSE_GOSSIP_MENU();
    
    return true;
}

/*######
## npc_akama_prelude
######*/

/*
TODO
* change olum modelid
*/

enum akamaPreludeData {
    QUEST_SECRET_COMPROMISED    = 10944,
    
    NPC_SEER_OLUM               = 22820,
    NPC_OLUM_SPIRIT             = 22870,
    NPC_ILLIDAN_PRESENCE        = 22865,
    
    SPELL_OLUM_SACRIFICE        = 39552,
    SPELL_SHADOWFORM            = 40973,
    SPELL_KNEEL                 = 39656,
   
    SAY_OLUM_1          = -1000729,
    SAY_AKAMA_1         = -1000730,
    SAY_OLUM_2          = -1000731,
    SAY_AKAMA_2         = -1000732,
    SAY_OLUM_3          = -1000733,
    SAY_AKAMA_3         = -1000734,
    SAY_OLUM_4          = -1000735,
    SAY_AKAMA_4         = -1000736,
    SAY_OLUM_5          = -1000737,
    SAY_AKAMA_5         = -1000738,
    SAY_AKAMA_6         = -1000739,
    SAY_ILLIDAN_1       = -1000740,
    SAY_AKAMA_7         = -1000741,
    SAY_ILLIDAN_2       = -1000742,
    SAY_ILLIDAN_3       = -1000743,
    SAY_AKAMA_8         = -1000744
};

struct npc_akama_preludeAI : public ScriptedAI
{
    npc_akama_preludeAI(Creature* c) : ScriptedAI(c)
    {
        eventTimer = 0;
        step = 0;
        talkId = 0;
        olumGUID = 0;
        presenceGUID = 0;
    }
    
    uint64 olumGUID;
    uint64 presenceGUID;
    
    uint32 eventTimer;
    int32 talkId;

    uint8 step;
    
    void Reset() {}
    
    void Aggro(Unit* who) {}
    
    void StartEvent()
    {
        me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER | UNIT_NPC_FLAG_GOSSIP);
        eventTimer = 500;
        step = 0;
        talkId = 0;
        olumGUID = 0;
        presenceGUID = 0;
    }
    
    void EndEvent()
    {
        if (Creature* presence = Creature::GetCreature(*me, presenceGUID))
            presence->DisappearAndDie();

        me->RemoveAurasDueToSpell(SPELL_KNEEL);
        me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER | UNIT_NPC_FLAG_GOSSIP);
        eventTimer = 0;
        step = 0;
        talkId = 0;
        olumGUID = 0;
        presenceGUID = 0;
    }
    
    void OnSpellFinish(Unit* caster, uint32 spellId, Unit* target, bool ok)
    {
        if (spellId == SPELL_OLUM_SACRIFICE) {
            me->Kill(target);
            if (Creature* spirit = me->SummonCreature(NPC_OLUM_SPIRIT, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), target->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN, 16000)) {
                spirit->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                spirit->SetUnitMovementFlags(MOVEMENTFLAG_LEVITATING);
                spirit->GetMotionMaster()->MovePoint(0, spirit->GetPositionX(), spirit->GetPositionY(), spirit->GetPositionZ() + 4.0f, false);
            }
        }
    }
    
    void MovementInform(uint32 type, uint32 id)
    {
        if (id == 0) {
            if (Creature* olum = Creature::GetCreature(*me, olumGUID))
                DoCast(olum, SPELL_OLUM_SACRIFICE);
        }
    }
    
    uint32 NextStep()
    {
        uint32 timer = 0;

        switch (step) {
        case 0:
            if (olumGUID)
                break;

            if (Creature* olum = me->SummonCreature(NPC_SEER_OLUM, -3729.779541, 1037.054443, 55.956562, 5.667850, TEMPSUMMON_MANUAL_DESPAWN, 0)) {
                olumGUID = olum->GetGUID();
                DoScriptText(SAY_OLUM_1, olum, NULL);
                olum->GetMotionMaster()->MovePoint(0, -3722.463867, 1032.153564, 55.956562);
                olum->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            }

            timer = 11000;
            break;
        case 1:
            DoScriptText(SAY_AKAMA_1, me, NULL);
            timer = 4500;
            break;
        case 2:
            if (Creature* olum = Creature::GetCreature(*me, olumGUID))
                DoScriptText(SAY_OLUM_2, olum, NULL);
                
            timer = 7000;
            break;
        case 3:
            DoScriptText(SAY_AKAMA_2, me, NULL);
            timer = 6500;
            break;
        case 4:
            if (Creature* olum = Creature::GetCreature(*me, olumGUID))
                DoScriptText(SAY_OLUM_3, olum, NULL);
                
            timer = 26000;
            break;
        case 5:
            DoScriptText(SAY_AKAMA_3, me, NULL);
            timer = 6500;
            break;
        case 6:
            if (Creature* olum = Creature::GetCreature(*me, olumGUID))
                DoScriptText(SAY_OLUM_4, olum, NULL);
                
            timer = 14000;
            break;
        case 7:
            DoScriptText(SAY_AKAMA_4, me, NULL);
            timer = 9500;
            break;
        case 8:
            if (Creature* olum = Creature::GetCreature(*me, olumGUID))
                DoScriptText(SAY_OLUM_5, olum, NULL);
                
            timer = 15000;
            break;
        case 9: // Akama moves to kill Olum
            me->GetMotionMaster()->MovePoint(0, -3718.926514, 1030.366211, 55.956562);
            timer = 14000;
            break;
        case 10:
            DoScriptText(SAY_AKAMA_5, me, NULL);
            timer = 13000;
            break;
        case 11:
            me->GetMotionMaster()->MoveTargetedHome();
            timer = 5000;
            break;
        case 12:
            DoScriptText(SAY_AKAMA_6, me, NULL);
            DoCast(me, SPELL_KNEEL, true);
            if (Creature* presence = me->SummonCreature(NPC_ILLIDAN_PRESENCE, -3724.522705, 1029.824463, 55.955868, 6.259977, TEMPSUMMON_MANUAL_DESPAWN, 0)) {
                presenceGUID = presence->GetGUID();
                presence->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                presence->CastSpell(presence, SPELL_SHADOWFORM, true);
                presence->SetReactState(REACT_PASSIVE);
            }
                
            timer = 9000;
            break;
        case 13:
            if (Creature* presence = Creature::GetCreature(*me, presenceGUID))
                DoScriptText(SAY_ILLIDAN_1, presence, NULL);
            
            timer = 13000;
            break;
        case 14:
            if (Creature* olum = Creature::GetCreature(*me, olumGUID))
                olum->DisappearAndDie();

            DoScriptText(SAY_AKAMA_7, me, NULL);
            timer = 18000;
            break;
        case 15:
            if (Creature* presence = Creature::GetCreature(*me, presenceGUID))
                DoScriptText(SAY_ILLIDAN_2, presence, NULL);
            
            timer = 20000;
            break;
        case 16:
            if (Creature* presence = Creature::GetCreature(*me, presenceGUID))
                DoScriptText(SAY_ILLIDAN_3, presence, NULL);
            
            timer = 21500;
            break;
        case 17:
            DoScriptText(SAY_AKAMA_8, me, NULL);
            timer = 2500;
            break;
        case 18:
            EndEvent();
            return 0;
        }
        
        step++;
        return timer;
    }
    
    void UpdateAI(uint32 const diff)
    {
        if (!eventTimer)
            return;
            
        if (eventTimer <= diff)
            eventTimer = NextStep();
        else
            eventTimer -= diff;
    }
};

CreatureAI* GetAI_npc_akama_prelude(Creature* creature)
{
    return new npc_akama_preludeAI(creature);
}

bool ChooseReward_npc_akama_prelude(Player* player, Creature* creature, Quest const* quest, uint32 opt)
{
    if (quest->GetQuestId() == QUEST_SECRET_COMPROMISED)
        ((npc_akama_preludeAI*)creature->AI())->StartEvent();
    
    return true;
}

/*######
## Quests 11101 && 11097
######*/

#define QUEST_DEADLIEST_TRAP_ALDOR  11101
#define QUEST_DEADLIEST_TRAP_SCYER  11097

#define COMMANDER_HOBB_SAY1         -1000745
#define COMMANDER_HOBB_SAY2         -1000746
#define COMMANDER_HOBB_SAY3         -1000756
#define SKYBREAKER_SAY1             -1000747
#define SKYBREAKER_SAY2             -1000748
#define SKYBREAKER_SAY3             -1000749
#define SKYBREAKER_SAYALDOR1        -1000750
#define SKYBREAKER_SAYALDOR2        -1000751
#define SKYBREAKER_SAYSCYER1        -1000752
#define SKYBREAKER_SAYSCYER2        -1000753
#define COMMANDER_ARCUS_SAY1        -1000754
#define COMMANDER_ARCUS_SAY2        -1000755
#define COMMANDER_ARCUS_SAY3         -1000757

#define SPELL_AIMED_SHOT            38370
#define SPELL_MULTI_SHOT            41448
#define SPELL_SHOOT                 41440

#define NPC_DRAGONMAW_SKYBREAKER_SCYER  23440
#define NPC_DRAGONMAW_SKYBREAKER_ALDOR  23441

#define NPC_DEFENDER_SCYER              23435
#define NPC_DEFENDER_ALDOR              23453

float skybreakerPosAldor[10][3] = {
    { -3150.351807, 756.830444, 37.261200 },
    { -3158.962646, 745.880676, 37.261200 },
    { -3170.083252, 731.739441, 37.261200 },
    { -3176.036865, 716.404663, 37.261200 },
    { -3179.811768, 706.681763, 37.261200 },
    { -3183.206543, 697.937683, 37.261200 },
    { -3185.165039, 684.075378, 37.261200 },
    { -3186.469482, 674.786499, 37.261200 },
    { -3187.608398, 666.676086, 37.261200 },
    { -3128.346436, 763.912537, 37.261200 }};

float skybreakerPosScyer[10][3] = {
    { -4080.790527, 970.862976, 79.887848 },
    { -4067.590820, 974.339661, 79.887848 },
    { -4053.443359, 978.065979, 79.887848 },
    { -4041.665039, 981.168274, 79.887848 },
    { -4027.382080, 984.930298, 79.887848 },
    { -4016.010010, 987.925659, 79.887848 },
    { -4007.221436, 1001.788818, 77.243546 },
    { -3996.254883, 1010.603210, 77.243546 },
    { -3985.991211, 1031.524292, 74.557518 },
    { -4114.185059, 975.281982, 74.557518 }};
    
// Orientation 2.337
float defendersPosAldor[10][3] = {
    { -3081.971924, 700.679260, -16.377661 },
    { -3083.283691, 697.540344, -16.844158 },
    { -3084.743896, 694.046204, -17.461650 },
    { -3085.826416, 691.456299, -17.954538 },
    { -3086.954834, 688.756592, -18.279066 },
    { -3088.004883, 686.244202, -18.166872 },
    { -3089.394043, 682.920410, -17.503175 },
    { -3093.030029, 677.387085, -16.527822 },
    { -3095.304199, 675.197266, -16.083666 },
    { -3097.482422, 673.099731, -15.411659 }};
    
// Orientation 5.109
float defendersPosScyer[10][3] = {
    { -4059.349121, 1075.253418, 31.234081 },
    { -4062.739014, 1074.006592, 30.966137 },
    { -4064.821533, 1073.240601, 30.761585 },
    { -4064.855469, 1068.144043, 30.224163 },
    { -4067.918457, 1069.042358, 30.359905 },
    { -4069.360596, 1063.678589, 30.409506 },
    { -4074.669922, 1065.100098, 30.804943 },
    { -4079.707275, 1065.286499, 31.153090 },
    { -4082.981689, 1062.328369, 31.237562 },
    { -4085.491699, 1058.670410, 30.850157 }};
    // -4072.696777, 1071.352295, 30.769413

struct npc_commander_arcusAI : public ScriptedAI
{
    npc_commander_arcusAI(Creature* c) : ScriptedAI(c), summons(me)
    {
        playerGUID = 0;
        isEvent = false;
    }
    
    float homeX, homeY, homeZ, homeOri;
    
    bool isEvent;
    
    uint64 playerGUID;
    
    uint32 aimedShotTimer;
    uint32 multiShotTimer;
    uint32 shootTimer;
    uint32 killCounter;
    uint32 summonTimer;
    
    SummonList summons;
    
    void Reset()
    {
        aimedShotTimer = 5000;
        multiShotTimer = 10000;
        shootTimer = 1000;
        killCounter = 0;
        summonTimer = 0;
    }

    void Aggro(Unit* who) {}
    
    void JustSummoned(Creature* summon)
    {
        summons.Summon(summon);
        
        if (Unit *target = me->FindCreatureInGrid(0, 150.0f, true))
            summon->AI()->AttackStart(target);
    }
    
    void SummonedCreatureDespawn(Creature* summon)
    {
        summons.Despawn(summon);
        if (summon->GetEntry() == NPC_DRAGONMAW_SKYBREAKER_ALDOR)
            killCounter++;
    }
    
    void MovementInform(uint32 type, uint32 id)
    {
        if (id == 0) {
            DoScriptText(COMMANDER_ARCUS_SAY2, me);
            Player* player = Unit::GetPlayer(playerGUID);
            summonTimer = 15000 + rand() % 10000;
            for (uint8 i = 0; i < 10; i++) {
                if (Creature* skybreaker = me->SummonCreature(NPC_DRAGONMAW_SKYBREAKER_ALDOR, skybreakerPosAldor[i][0], skybreakerPosAldor[i][1], skybreakerPosAldor[i][2], 0, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 1000)) {
                    skybreaker->AddUnitMovementFlag(MOVEMENTFLAG_LEVITATING + MOVEMENTFLAG_ONTRANSPORT);
                    if (player)
                        skybreaker->AI()->AttackStart(player);
                }
            }
        }
    }
    
    void StartEvent()
    {
        isEvent = true;
        DoScriptText(COMMANDER_ARCUS_SAY1, me, NULL);
        me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
        me->RemoveUnitMovementFlag(MOVEFLAG_WALK);
        me->GetMotionMaster()->MovePoint(0, -3098.666748, 682.178040, -18.633110, true);
        
        for (uint8 i = 0; i < 10; i++)
            me->SummonCreature(NPC_DEFENDER_ALDOR, defendersPosAldor[i][0], defendersPosAldor[i][1], defendersPosAldor[i][2], 0, TEMPSUMMON_MANUAL_DESPAWN, 0);
        
        me->GetHomePosition(homeX, homeY, homeZ, homeOri);
        me->SetHomePosition(-3098.666748, 682.178040, -18.633110, me->GetOrientation());
    }
    
    void EndEvent(bool success)
    {
        Player* player = Unit::GetPlayer(playerGUID);
        if (success) {
            DoScriptText(COMMANDER_ARCUS_SAY3, me);
            if (player)
                player->GroupEventHappens(QUEST_DEADLIEST_TRAP_ALDOR, me);
        }
        else {
            if (player)
                player->FailQuest(QUEST_DEADLIEST_TRAP_ALDOR);
        }

        playerGUID = 0;
        me->SetHomePosition(homeX, homeY, homeZ, homeOri);
        EnterEvadeMode();
        me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
        summons.DespawnAll();
        isEvent = false;
    }
    
    void UpdateAI(uint32 const diff)
    {
        if (!isEvent)
            return;

        Player* player = Unit::GetPlayer(playerGUID);
        if (!player || player->GetDistance(me) > 110.0f) {
            EndEvent(false);
            return;
        }
            
        if (killCounter >= 15) {
            EndEvent(true);
            return;
        }

        if (summonTimer) {
            if (summonTimer <= diff) {
                if (Creature* skybreaker = me->SummonCreature(NPC_DRAGONMAW_SKYBREAKER_ALDOR, skybreakerPosAldor[rand()%10][0], skybreakerPosAldor[rand()%10][1], skybreakerPosAldor[rand()%10][2], 0, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 1000)) {
                    skybreaker->AddUnitMovementFlag(MOVEMENTFLAG_LEVITATING + MOVEMENTFLAG_ONTRANSPORT);
                    skybreaker->AI()->AttackStart(player);
                }
                
                summonTimer = 15000 + rand() % 10000;
            }
            else
                summonTimer -= diff;
        }

        if (!UpdateVictim(false))
            return;

        if (aimedShotTimer <= diff) {
            DoCast(SelectUnit(SELECT_TARGET_RANDOM, 0), SPELL_AIMED_SHOT);
            aimedShotTimer = 5000 + rand() % 3000;
        }
        else
            aimedShotTimer -= diff;
            
        if (multiShotTimer <= diff) {
            DoCast(SelectUnit(SELECT_TARGET_RANDOM, 0), SPELL_MULTI_SHOT);
            multiShotTimer = 5000 + rand() % 3000;
        }
        else
            multiShotTimer -= diff;
            
        if (shootTimer <= diff) {
            DoCast(me->getVictim(), SPELL_SHOOT);
            shootTimer = 5000 + rand() % 3000;
        }
        else
            shootTimer -= diff;
        
        if (me->getVictim()->IsWithinMeleeRange(me))
            DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_commander_arcus(Creature* creature)
{
    return new npc_commander_arcusAI(creature);
}

bool QuestAccept_npc_commander_arcus(Player* player, Creature* creature, Quest const* quest)
{
    if (creature->isQuestGiver())
        player->PrepareQuestMenu(creature->GetGUID());

    if (quest->GetQuestId() == QUEST_DEADLIEST_TRAP_ALDOR) {
        ((npc_commander_arcusAI*)creature->AI())->playerGUID = player->GetGUID();
        ((npc_commander_arcusAI*)creature->AI())->StartEvent();
    }
    
    return true;
}

struct npc_commander_hobbAI : public ScriptedAI
{
    npc_commander_hobbAI(Creature* c) : ScriptedAI(c), summons(me)
    {
        playerGUID = 0;
        isEvent = false;
    }
    
    float homeX, homeY, homeZ, homeOri;
    
    bool isEvent;
    
    uint64 playerGUID;
    
    uint32 aimedShotTimer;
    uint32 multiShotTimer;
    uint32 shootTimer;
    uint32 killCounter;
    uint32 summonTimer;
    
    SummonList summons;
    
    void Reset()
    {
        aimedShotTimer = 5000;
        multiShotTimer = 10000;
        shootTimer = 1000;
        killCounter = 0;
        summonTimer = 0;
    }

    void Aggro(Unit* who) {}
    
    void JustSummoned(Creature* summon)
    {
        summons.Summon(summon);
        
        if (Unit *target = me->FindCreatureInGrid(0, 150.0f, true))
            summon->AI()->AttackStart(target);
    }
    
    void SummonedCreatureDespawn(Creature* summon)
    {
        summons.Despawn(summon);
        if (summon->GetEntry() == NPC_DRAGONMAW_SKYBREAKER_SCYER)
            killCounter++;
    }
    
    void MovementInform(uint32 type, uint32 id)
    {
        if (id == 0) {
            DoScriptText(COMMANDER_HOBB_SAY2, me);
            Player* player = Unit::GetPlayer(playerGUID);
            summonTimer = 15000 + rand() % 10000;
            for (uint8 i = 0; i < 10; i++) {
                if (Creature* skybreaker = me->SummonCreature(NPC_DRAGONMAW_SKYBREAKER_SCYER, skybreakerPosScyer[i][0], skybreakerPosScyer[i][1], skybreakerPosScyer[i][2], 0, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 1000)) {
                    skybreaker->AddUnitMovementFlag(MOVEMENTFLAG_LEVITATING + MOVEMENTFLAG_ONTRANSPORT);
                    if (player)
                        skybreaker->AI()->AttackStart(player);
                }
            }
        }
    }
    
    void StartEvent()
    {
        isEvent = true;
        DoScriptText(COMMANDER_HOBB_SAY1, me, NULL);
        me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
        me->RemoveUnitMovementFlag(MOVEFLAG_WALK);
        me->GetMotionMaster()->MovePoint(0, -4072.696777, 1071.352295, 30.769413, true);
        
        for (uint8 i = 0; i < 10; i++)
            me->SummonCreature(NPC_DEFENDER_SCYER, defendersPosScyer[i][0], defendersPosScyer[i][1], defendersPosScyer[i][2], 0, TEMPSUMMON_MANUAL_DESPAWN, 0);
        
        me->GetHomePosition(homeX, homeY, homeZ, homeOri);
        me->SetHomePosition(-4072.696777, 1071.352295, 30.769413, me->GetOrientation());
    }
    
    void EndEvent(bool success)
    {
        Player* player = Unit::GetPlayer(playerGUID);
        if (success) {
            DoScriptText(COMMANDER_HOBB_SAY3, me);
            if (player)
                player->GroupEventHappens(QUEST_DEADLIEST_TRAP_SCYER, me);
        }
        else {
            if (player)
                player->FailQuest(QUEST_DEADLIEST_TRAP_SCYER);
        }

        playerGUID = 0;
        me->SetHomePosition(homeX, homeY, homeZ, homeOri);
        EnterEvadeMode();
        me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
        summons.DespawnAll();
        isEvent = false;
    }
    
    void UpdateAI(uint32 const diff)
    {
        if (!isEvent)
            return;

        Player* player = Unit::GetPlayer(playerGUID);
        if (!player || player->GetDistance(me) > 110.0f) {
            EndEvent(false);
            return;
        }
            
        if (killCounter >= 15) {
            EndEvent(true);
            return;
        }

        if (summonTimer) {
            if (summonTimer <= diff) {
                if (Creature* skybreaker = me->SummonCreature(NPC_DRAGONMAW_SKYBREAKER_SCYER, skybreakerPosScyer[rand()%10][0], skybreakerPosScyer[rand()%10][1], skybreakerPosScyer[rand()%10][2], 0, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 1000)) {
                    skybreaker->AddUnitMovementFlag(MOVEMENTFLAG_LEVITATING + MOVEMENTFLAG_ONTRANSPORT);
                    skybreaker->AI()->AttackStart(player);
                }
                
                summonTimer = 15000 + rand() % 10000;
            }
            else
                summonTimer -= diff;
        }

        if (!UpdateVictim(false))
            return;

        if (aimedShotTimer <= diff) {
            DoCast(SelectUnit(SELECT_TARGET_RANDOM, 0), SPELL_AIMED_SHOT);
            aimedShotTimer = 5000 + rand() % 3000;
        }
        else
            aimedShotTimer -= diff;
            
        if (multiShotTimer <= diff) {
            DoCast(SelectUnit(SELECT_TARGET_RANDOM, 0), SPELL_MULTI_SHOT);
            multiShotTimer = 5000 + rand() % 3000;
        }
        else
            multiShotTimer -= diff;
            
        if (shootTimer <= diff) {
            DoCast(me->getVictim(), SPELL_SHOOT);
            shootTimer = 5000 + rand() % 3000;
        }
        else
            shootTimer -= diff;
        
        if (me->getVictim()->IsWithinMeleeRange(me))
            DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_commander_hobb(Creature* creature)
{
    return new npc_commander_hobbAI(creature);
}

bool QuestAccept_npc_commander_hobb(Player* player, Creature* creature, Quest const* quest)
{
    if (creature->isQuestGiver())
        player->PrepareQuestMenu(creature->GetGUID());

    if (quest->GetQuestId() == QUEST_DEADLIEST_TRAP_SCYER) {
        ((npc_commander_hobbAI*)creature->AI())->playerGUID = player->GetGUID();
        ((npc_commander_hobbAI*)creature->AI())->StartEvent();
    }
    
    return true;
}

struct npc_dragonmaw_skybreakerAI : public ScriptedAI
{
    npc_dragonmaw_skybreakerAI(Creature* c) : ScriptedAI(c)
    {
        aldor = (me->GetEntry() == NPC_DRAGONMAW_SKYBREAKER_ALDOR) ? true : false;
    }
    
    bool aldor;
    
    uint32 aimedShotTimer;
    uint32 multiShotTimer;
    uint32 scatterShotTimer;
    uint32 shootTimer;
    
    uint32 randomTauntTimer;
    
    void Reset()
    {
        randomTauntTimer = 1000 + rand() % 5000;
    }
    
    void Aggro(Unit* who)
    {
        if (rand()%10 < 6)
            return;

        DoScriptText(RAND(SKYBREAKER_SAY1, SKYBREAKER_SAY2), me, NULL);
    }
    
    void UpdateAI(uint32 const diff)
    {
        me->addUnitState(UNIT_STAT_IGNORE_PATHFINDING);

        if (!UpdateVictim(false))
            return;
            
        if (randomTauntTimer <= diff) {
            if (aldor)
                DoScriptText(RAND(SKYBREAKER_SAYALDOR1, SKYBREAKER_SAYALDOR2, SKYBREAKER_SAY3), me);
            else
                DoScriptText(RAND(SKYBREAKER_SAYSCYER1, SKYBREAKER_SAYSCYER2, SKYBREAKER_SAY3), me);
                
            randomTauntTimer = 8000 + rand() % 4000;
        }
        else
            randomTauntTimer -= diff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_dragonmaw_skybreaker(Creature* creature)
{
    return new npc_dragonmaw_skybreakerAI(creature);
}

/*######
## npc_xiri
######*/

/*

SPELLS

39828 - self cast - effet de traits blancs qui montent
39829 - idem mais une seule fois
39831 - grosse explosion blanche

NPCS

22863 - défenseur clairvoyant
22864 - chef clairvoyant
22861 - défenseur aldor
22862 - chef aldor
18528 - xi'ri
21166 - seigneur de l'effroi illidari

VIDEO

http://www.youtube.com/watch?v=zET3VQusn9o (lien privé)

*/

struct npc_xiriAI : public Scripted_NoMovementAI
{
    npc_xiriAI(Creature* c) : Scripted_NoMovementAI(c)
    {
        isEvent = false;
    }
    
    bool isEvent;
    
    uint32 summonTimer;
    
    void Reset()
    {
        isEvent = false;
        summonTimer = 15000;
    }
    
    void Aggro(Unit* who) {}
    
    void StartEvent()
    {
        isEvent = true;
    }
    
    void StopEvent()
    {
        isEvent = false;
    }
    
    void UpdateAI(uint32 const diff)
    {
        if (!isEvent)
            return;

        
    }
};

CreatureAI* GetAI_npc_xiri(Creature* creature)
{
    return new npc_xiriAI(creature);
}

bool GossipHello_npc_xiri(Player* player, Creature* creature)
{
    if (creature->isQuestGiver())
        player->PrepareQuestMenu(creature->GetGUID());
        
    if (player->GetQuestStatus(10985) == QUEST_STATUS_INCOMPLETE)
        player->ADD_GOSSIP_ITEM(0, "Je suis prêt à rejoindre vos forces dans la bataille, Xi'ri.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
        
    player->SEND_GOSSIP_MENU(creature->GetNpcTextId(), creature->GetGUID());
    
    return true;
}

bool GossipSelect_npc_xiri(Player* player, Creature* creature, uint32 sender, uint32 action)
{
    if (action == GOSSIP_ACTION_INFO_DEF) {
        player->CLOSE_GOSSIP_MENU();
        ((npc_xiriAI*)creature->AI())->StartEvent();
    }
    
    return true;
}

/*######
## AddSC
#######*/

void AddSC_shadowmoon_valley()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name = "mob_mature_netherwing_drake";
    newscript->GetAI = &GetAI_mob_mature_netherwing_drake;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "mob_enslaved_netherwing_drake";
    newscript->GetAI = &GetAI_mob_enslaved_netherwing_drake;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "mob_dragonmaw_peon";
    newscript->GetAI = &GetAI_mob_dragonmaw_peon;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="npc_drake_dealer_hurlunk";
    newscript->pGossipHello =  &GossipHello_npc_drake_dealer_hurlunk;
    newscript->pGossipSelect = &GossipSelect_npc_drake_dealer_hurlunk;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="npcs_flanis_swiftwing_and_kagrosh";
    newscript->pGossipHello =  &GossipHello_npcs_flanis_swiftwing_and_kagrosh;
    newscript->pGossipSelect = &GossipSelect_npcs_flanis_swiftwing_and_kagrosh;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="npc_murkblood_overseer";
    newscript->pGossipHello =  &GossipHello_npc_murkblood_overseer;
    newscript->pGossipSelect = &GossipSelect_npc_murkblood_overseer;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="npc_neltharaku";
    newscript->pGossipHello =  &GossipHello_npc_neltharaku;
    newscript->pGossipSelect = &GossipSelect_npc_neltharaku;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_karynaku";
    newscript->pQuestAccept = &QuestAccept_npc_karynaku;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="npc_oronok_tornheart";
    newscript->pGossipHello =  &GossipHello_npc_oronok_tornheart;
    newscript->pGossipSelect = &GossipSelect_npc_oronok_tornheart;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_overlord_morghor";
    newscript->GetAI = &GetAI_npc_overlord_morghorAI;
    newscript->pQuestAccept = &QuestAccept_npc_overlord_morghor;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_earthmender_wilda";
    newscript->GetAI = &GetAI_npc_earthmender_wildaAI;
    newscript->pQuestAccept = &QuestAccept_npc_earthmender_wilda;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_lord_illidan_stormrage";
    newscript->GetAI = &GetAI_npc_lord_illidan_stormrage;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "go_crystal_prison";
    newscript->pGOQuestAccept = &GOQuestAccept_GO_crystal_prison;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "mob_illidari_spawn";
    newscript->GetAI = &GetAI_mob_illidari_spawn;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "mob_torloth_the_magnificent";
    newscript->GetAI = &GetAI_mob_torloth_the_magnificent;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_enraged_spirit";
    newscript->GetAI = &GetAI_npc_enraged_spirit;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_spirits_totem";
    newscript->GetAI = &GetAI_npc_spirits_totem;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_deathbringer_jovaan";
    newscript->GetAI = &GetAI_npc_deathbringer_jovaan;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_grand_commander_ruusk";
    newscript->pGossipHello = &GossipHello_npc_grand_commander_ruusk;
    newscript->pGossipSelect = &GossipSelect_npc_grand_commander_ruusk;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_skartax";
    newscript->GetAI = &GetAI_npc_skartax;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_invis_deathforge_caster";
    newscript->GetAI = &GetAI_npc_invis_deathforge_caster;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "go_arcano_control_unit";
    newscript->pGOHello = &GOHello_go_arcano_control_unit;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_quest_spectrecles";
    newscript->pGossipHello = &GossipHello_npc_quest_spectrecles;
    newscript->pGossipSelect = &GossipSelect_npc_quest_spectrecles;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_akama_prelude";
    newscript->GetAI = &GetAI_npc_akama_prelude;
    newscript->pChooseReward = &ChooseReward_npc_akama_prelude;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_commander_arcus";
    newscript->GetAI = &GetAI_npc_commander_arcus;
    newscript->pQuestAccept = &QuestAccept_npc_commander_arcus;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_commander_hobb";
    newscript->GetAI = &GetAI_npc_commander_hobb;
    newscript->pQuestAccept = &QuestAccept_npc_commander_hobb;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_dragonmaw_skybreaker";
    newscript->GetAI = &GetAI_npc_dragonmaw_skybreaker;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_xiri";
    newscript->GetAI = &GetAI_npc_xiri;
    newscript->pGossipHello = &GossipHello_npc_xiri;
    newscript->pGossipSelect = &GossipSelect_npc_xiri;
    newscript->RegisterSelf();
}
