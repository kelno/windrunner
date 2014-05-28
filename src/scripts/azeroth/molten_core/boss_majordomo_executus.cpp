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
SDName: Boss_Majordomo_Executus
SD%Complete: 30
SDComment: Correct spawning and Event NYI
SDCategory: Molten Core
EndScriptData */

#include "precompiled.h"
#include "def_molten_core.h"

#define TEXT_ID_SUMMON_1 4995
#define TEXT_ID_SUMMON_2 5011
#define TEXT_ID_SUMMON_3 5012
 
#define GOSSIP_ITEM_SUMMON_1 "Dites m'en plus."
#define GOSSIP_ITEM_SUMMON_2 "Qu'avez vous d'autre à dire?"
#define GOSSIP_ITEM_SUMMON_3 "Vous nous avez défiés et nous sommes venus. Où est donc ce maître dont vous parlez?"

enum
{
    SAY_AGGRO                  = -1409003,
    SAY_SPAWN                  = -1409004,
    SAY_SLAY                   = -1409005,
    SAY_SPECIAL                = -1409006,
    SAY_DEFEAT                 = -1409007,

    // major spells
    SPELL_MAGIC_REFLECTION     = 20619,
    SPELL_DAMAGE_REFLECTION    = 21075,
    SPELL_BLASTWAVE            = 20229,
    SPELL_AEGIS                = 20620,                  //This is self casted whenever we are below 50%
    SPELL_TELEPORT             = 20618,
    SPELL_SUMMON_RAGNAROS      = 19774,

    // Healer spells
    SPELL_SHADOWSHOCK          = 20603,
    SPELL_SHADOWBOLT           = 21077,

    // Elite spells
    SPELL_FIREBLAST            = 20623,
    SPELL_SEPARATION_ANXIETY   = 21095,

    CREATURE_FLAMEWALKER_HEALER   = 11663,
    CREATURE_FLAMEWALKER_ELITE    = 11664,

    GOBJECT_CACHEOFTHEFIRELORD = 179703,

    SPELL_HOTCOAL              = 30512
};

struct Locations
{
    float x, y, z, o;
};

static Locations RoomCenter =
{
    736.55, -1175.83, -119.08, 0
};

static Locations CacheLocation =
{
    748.365, -1195.52, -118.145, 0
};

static Locations CaolLocation =
{
    736.663025, -1176.569946, -119.797997, 0
};


static Locations GuardsLocations[] =
{
    { 757.638245, -1187.058594, -118.657173, 2.579740 },
    { 753.697266, -1192.992554, -118.387291, 2.395171 },
    { 747.905396, -1197.361450, -118.173851, 2.049596 },
    { 741.920837, -1200.637817, -118.056541, 1.892516 },
    { 759.999695, -1172.765747, -119.046082, 3.278744 },
    { 757.849915, -1167.811523, -119.006889, 3.506510 },
    { 755.279541, -1163.614624, -119.140846, 3.822240 },
    { 752.244751, -1159.199585, -119.253311, 3.841875 }
};

enum DomoPhases
{
    NOT_VISIBLE,
    VISIBLE,
    DOWN,
    RAGNAGNA,
};

class Boss_Majordomo : public CreatureScript
{
    public:
        Boss_Majordomo() : CreatureScript("Boss_Majordomo") {}

    class Boss_MajordomoAI : public CreatureAINew
    {
        public:
            enum event
            {
                EV_REFLECTION       = 0,
                EV_TELEPORT         = 1,
                EV_BLASTWAVE        = 2,
                EV_DOWN             = 3,
                EV_CHECK_PHASE      = 4
            };
            ScriptedInstance* _instance;
            uint8 guardCount;

            Boss_MajordomoAI(Creature* creature) : CreatureAINew(creature), Summons(me)
            {
                _instance = ((ScriptedInstance*)creature->GetInstanceData());
            }

            void onReset(bool onSpawn)
            {
                Summons.DespawnAll();
                if (onSpawn)
                {
                    addEvent(EV_REFLECTION, 10000, 10000, EVENT_FLAG_NONE, true, phaseMaskForPhase(VISIBLE));
                    addEvent(EV_TELEPORT, 25000, 25000, EVENT_FLAG_NONE, true, phaseMaskForPhase(VISIBLE));
                    addEvent(EV_BLASTWAVE, 3000, 10000, EVENT_FLAG_NONE, true, phaseMaskForPhase(VISIBLE));
                    addEvent(EV_DOWN, 33000, 33000, EVENT_FLAG_NONE, true, phaseMaskForPhase(DOWN));
                    addEvent(EV_CHECK_PHASE, 10000, 10000, EVENT_FLAG_NONE, true, phaseMaskForPhase(NOT_VISIBLE));
                }
                else
                {
                    scheduleEvent(EV_REFLECTION, 10000, 10000);
                    scheduleEvent(EV_TELEPORT, 30000, 30000);
                    scheduleEvent(EV_BLASTWAVE, 3000, 10000);
                    scheduleEvent(EV_DOWN, 33000, 33000);
                    scheduleEvent(EV_CHECK_PHASE, 10000, 10000);
                }

                if (_instance)
                    _instance->SetData(DATA_MAJORDOMO, NOT_STARTED);

                setPhase(NOT_VISIBLE, true);
            }

            void onSummon(Creature* summoned)
            {
                Summons.Summon(summoned);
            }
    
            void onSummonDespawn(Creature* unit)
            {
                Summons.Despawn(unit);
            }

            void onEnterPhase(uint32 newPhase)
            {
                switch (newPhase)
                {
                    case NOT_VISIBLE:
                        me->setFaction(35);
                        me->SetVisibility(VISIBILITY_OFF);
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                        break;
                    case VISIBLE:
                        DoScriptText(SAY_SPAWN, me);
                        summonGuards();
                        me->setFaction(54);
                        me->SetVisibility(VISIBILITY_ON);
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                        break;
                    case DOWN:
                    {
                        Summons.DespawnAll();
                        me->setFaction(35);
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                        me->SetReactState(REACT_PASSIVE);
                        me->GetMotionMaster()->MoveTargetedHome();
                        me->CombatStop();
                        DoScriptText(SAY_DEFEAT, me);
                        
                        if (_instance)
                            _instance->SetData(DATA_MAJORDOMO, DONE);

                        me->SummonGameObject(GOBJECT_CACHEOFTHEFIRELORD, CacheLocation.x, CacheLocation.y, CacheLocation.z, 0, 0, 0, 0, 0, 0);

                        Map *map = me->GetMap();
                        Map::PlayerList const &PlayerList = map->GetPlayers();

                        for(Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                        if (Player* i_pl = i->getSource())
                        if (i_pl->IsAlive())
                            i_pl->CombatStop(true);

                        break;
                    }
                    case RAGNAGNA:
                        me->setFaction(35);
                        me->SetVisibility(VISIBILITY_ON);
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                        me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);  
                        me->SetSpeed(MOVE_WALK, 0.9);
                        me->AddUnitMovementFlag(MOVEMENTFLAG_WALK_MODE);
                        me->UpdateSpeed(MOVE_WALK, true);
                        break;
                }
            }

            bool isSpawnReady()
            {
                if (_instance)
                {
                    if (_instance->GetData(DATA_MAJORDOMO) == DONE)
                        return false;

                    if (_instance->GetData(DATA_MAJORDOMO) == NOT_STARTED)
                    {
                        for (int i = DATA_LUCIFRON; i <= DATA_SULFURON; i++)
                        {
                            if (_instance->GetData(i) != DONE)
                                return false;
                        }
                    }
                    else
                        return false;
                }
                return true;
            }

            void summonGuards()
            {
                guardCount = 0;
                Summons.DespawnAll();
                for (int i = 0; i < 8; i++)
                {
                    uint32 creatureId = (i%2) ? CREATURE_FLAMEWALKER_HEALER : CREATURE_FLAMEWALKER_ELITE;
                    Creature* Guard;
                    if (me->GetMapId() == 409)
                        Guard = me->SummonCreature(creatureId, GuardsLocations[i].x, GuardsLocations[i].y, GuardsLocations[i].z, GuardsLocations[i].o, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 60000); 
                    else
                    {
                        float x = ((i < 4) ? (i*10)+10 : -(i-4)*10-10);
                        Guard = me->SummonCreature(creatureId, me->GetPositionX() + x, me->GetPositionY(), me->GetPositionZ(), 0, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 60000);
                    }

                    if (Guard)
                    {
                        Guard->SetOwnerGUID(me->GetGUID());
                        guardCount++;
                    }
                }
            }

            void onCombatStart(Unit* /*victim*/)
            {
                if (_instance)
                    _instance->SetData(DATA_MAJORDOMO, IN_PROGRESS);

                DoScriptText(SAY_AGGRO, me);
            }

            void onKill(Unit* /*victim*/)
            {
                if (!(rand()%5))
                    DoScriptText(SAY_SLAY, me);
            }

            void onDamageTaken(Unit* /*attacker*/, uint32& damage)
            {
                if (damage > me->GetHealth())
                {
                    damage = 0;
                    me->SetHealth(me->GetMaxHealth());
                }
            }
        
            void update(uint32 const diff)
            {
                updateEvents(diff, 24);
            
                while (executeEvent(diff, m_currEvent))
                {
                    switch (m_currEvent)
                    {
                        case EV_CHECK_PHASE:
                            if (me->GetOwnerGUID() != 0)
                                setPhase(RAGNAGNA);
                            else if (isSpawnReady())
                                setPhase(VISIBLE);

                            scheduleEvent(EV_CHECK_PHASE, 10000);
                            break;
                        case EV_REFLECTION:
                        {
                            uint32 spellID = rand()%2 ? SPELL_MAGIC_REFLECTION : SPELL_DAMAGE_REFLECTION;
                            Creature* NagaFriend;
                            for (std::list<uint64>::iterator i = Summons.begin(); i!= Summons.end();++i)
                            {
                                NagaFriend = me->GetCreature((*me), (*i));
                                if (NagaFriend && NagaFriend->IsAlive())       
                                    me->AddAura(spellID, NagaFriend); 
                            }
                            me->AddAura(spellID, me);
                            scheduleEvent(EV_REFLECTION, 30000);
                            break;
                        }
                        case EV_TELEPORT:
                            if (Unit* target = selectUnit(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                doCast(target, SPELL_TELEPORT, true);
                            scheduleEvent(EV_TELEPORT, 30000);
                            break;
                        case EV_BLASTWAVE:
                            doCast(me->GetVictim(), SPELL_BLASTWAVE);
                            scheduleEvent(EV_BLASTWAVE, 3000, 10000);
                            break;
                        case EV_DOWN:
                            doCast(me, SPELL_TELEPORT, true);
                            setPhase(NOT_VISIBLE);
                            me->CombatStop();
                            break;
                    }
                }

                if (!updateVictim())
                    return;

                updateEvents(diff, 7);

                if (me->GetDistance(RoomCenter.x, RoomCenter.y , RoomCenter.z) > 100)
                    evade();

                if (guardCount < 1 && getPhase() != DOWN)
                    setPhase(DOWN);

                if ((me->GetHealth() / me->GetMaxHealth()) < 0.5)
                    doCast(me, SPELL_AEGIS);

                doMeleeAttackIfReady();
            }

        private:
            SummonList Summons;
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new Boss_MajordomoAI(creature);
    }
};

class Mob_FlameWalker_Healer : public CreatureScript
{
    public:
        Mob_FlameWalker_Healer() : CreatureScript("Mob_FlameWalker_Healer") {}
    
    class Mob_FlameWalker_HealerAI : public CreatureAINew
    {
        public:
            enum event
            {
                EV_SHADOWBOLT         = 0,
                EV_SHADOWSHOCK        = 1,
                EV_SEPARATION_ANX     = 2
            };
            Boss_Majordomo::Boss_MajordomoAI* domoAI;

            Mob_FlameWalker_HealerAI(Creature* creature) : CreatureAINew(creature)
            {
                _instance = ((ScriptedInstance*)creature->GetInstanceData());
            }

            void onReset(bool onSpawn)
            {
                domoAI = NULL;

                if (onSpawn)
                {
                    addEvent(EV_SHADOWBOLT, 8000, 12000, EVENT_FLAG_DELAY_IF_CASTING);
                    addEvent(EV_SHADOWSHOCK, 7000, 12000, EVENT_FLAG_DELAY_IF_CASTING);
                    addEvent(EV_SEPARATION_ANX, 100, 100, EVENT_FLAG_NONE, false);
                }
                else
                {
                    scheduleEvent(EV_SHADOWBOLT, 8000, 12000);
                    scheduleEvent(EV_SHADOWSHOCK, 7000, 12000);
                    scheduleEvent(EV_SEPARATION_ANX, 100, 100);
                }
            }

            void onDeath(Unit* /*killer*/)
            {
                if (domoAI)
                    domoAI->guardCount--;
            }
        
            void update(uint32 const diff)
            {
                if (!updateVictim())
                    return;

                if (!domoAI)
                {
                    if (Creature* domo = _instance->instance->GetCreature(_instance->GetData64(DATA_MAJORDOMO)))
                        domoAI = (Boss_Majordomo::Boss_MajordomoAI*)domo->getAI();
                }
                else
                {
                    if (domoAI->guardCount <= 4 && !me->HasAura(SPELL_SEPARATION_ANXIETY, 0))
                        enableEvent(EV_SEPARATION_ANX);
                }
            
                updateEvents(diff);
            
                while (executeEvent(diff, m_currEvent))
                {
                    switch (m_currEvent)
                    {
                        case EV_SHADOWBOLT:
                            if (Unit* target = selectUnit(SELECT_TARGET_TOPAGGRO, 0, 100.0f, true))
                                doCast(target, SPELL_SHADOWBOLT);
                            scheduleEvent(EV_SHADOWBOLT, urand(8000, 12000));
                            break;
                        case EV_SHADOWSHOCK:
                            if (Unit* target = selectUnit(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                doCast(target, SPELL_SHADOWSHOCK);
                            scheduleEvent(EV_SHADOWSHOCK, urand(15000, 25000));
                            break;
                        case EV_SEPARATION_ANX:
                            doCast(me, SPELL_SEPARATION_ANXIETY, true);
                            disableEvent(EV_SEPARATION_ANX);
                            break;
                    }
                }

                doMeleeAttackIfReady();
            }

        private:
            ScriptedInstance* _instance;
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new Mob_FlameWalker_HealerAI(creature);
    }
};

class Mob_FlameWalker_Elite : public CreatureScript
{
    public:
        Mob_FlameWalker_Elite() : CreatureScript("Mob_FlameWalker_Elite") {}
    
    class Mob_FlameWalker_EliteAI : public CreatureAINew
    {
        public:
            enum event
            {
                EV_BLASTWAVE          = 0,
                EV_FIREBLAST          = 1,
                EV_SEPARATION_ANX     = 2
            };
            Boss_Majordomo::Boss_MajordomoAI* domoAI;

            Mob_FlameWalker_EliteAI(Creature* creature) : CreatureAINew(creature)
            {
                _instance = ((ScriptedInstance*)creature->GetInstanceData());
            }

            void onReset(bool onSpawn)
            {
                domoAI = NULL;

                if (onSpawn)
                {
                    addEvent(EV_BLASTWAVE, 3000, 10000);
                    addEvent(EV_FIREBLAST, 10000, 10000);
                    addEvent(EV_SEPARATION_ANX, 100, 100, EVENT_FLAG_NONE, false);
                }
                else
                {
                    scheduleEvent(EV_BLASTWAVE, 3000, 10000);
                    scheduleEvent(EV_FIREBLAST, 10000, 10000);
                    scheduleEvent(EV_SEPARATION_ANX, 100, 100);
                }
            }

            void onDeath(Unit* /*killer*/)
            {
                if (domoAI)
                    domoAI->guardCount--;
            }
        
            void update(uint32 const diff)
            {
                if (!updateVictim())
                    return;

                if (!domoAI)
                {
                    if (Creature* domo = _instance->instance->GetCreature(_instance->GetData64(DATA_MAJORDOMO)))
                        domoAI = (Boss_Majordomo::Boss_MajordomoAI*)domo->getAI();
                }
                else
                {
                    if (domoAI->guardCount <= 4 && !me->HasAura(SPELL_SEPARATION_ANXIETY, 0))
                        enableEvent(EV_SEPARATION_ANX);

                    if (Creature* domo = _instance->instance->GetCreature(_instance->GetData64(DATA_MAJORDOMO)))
                        domo->AddThreat(me->GetVictim(), 0);
                }
            
                updateEvents(diff);
            
                while (executeEvent(diff, m_currEvent))
                {
                    switch (m_currEvent)
                    {
                        case EV_BLASTWAVE:
                            doCast(me->GetVictim(), SPELL_BLASTWAVE);
                            scheduleEvent(EV_BLASTWAVE, urand(3000, 10000));
                            break;
                        case EV_FIREBLAST:
                            doCast(me->GetVictim(), SPELL_FIREBLAST);
                            scheduleEvent(EV_FIREBLAST, 10000);
                            break;
                        case EV_SEPARATION_ANX:
                            doCast(me, SPELL_SEPARATION_ANXIETY, true);
                            disableEvent(EV_SEPARATION_ANX);
                            break;
                    }
                }

                doMeleeAttackIfReady();
            }

        private:
            ScriptedInstance* _instance;
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new Mob_FlameWalker_EliteAI(creature);
    }
};

class Mob_Hot_Coal : public CreatureScript
{
    public:
        Mob_Hot_Coal() : CreatureScript("Mob_Hot_Coal") {}
    
    class Mob_Hot_CoalAI : public CreatureAINew
    {
        public:
            enum event
            {
                EV_COAL          = 0
            };

            Mob_Hot_CoalAI(Creature* creature) : CreatureAINew(creature)
            {
                _instance = ((ScriptedInstance*)creature->GetInstanceData());
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            }

            void onReset(bool onSpawn)
            {
                if (onSpawn)
                {
                    addEvent(EV_COAL, 1000, 1000);
                }
                else
                {
                    scheduleEvent(EV_COAL, 1000, 1000);
                }
                me->SetVisibility(VISIBILITY_OFF);
            }

            void update(uint32 const diff)
            {
                updateEvents(diff);
            
                while (executeEvent(diff, m_currEvent))
                {
                    switch (m_currEvent)
                    {
                        case EV_COAL:
                            Map *map = me->GetMap();
                            Map::PlayerList const &PlayerList = map->GetPlayers();

                    for(Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                        if (Player* i_pl = i->getSource())
                            if (i_pl->IsAlive() && i_pl->isAttackableByAOE() && i_pl->GetDistance(CaolLocation.x, CaolLocation.y, CaolLocation.z) <= 8)
                                {
                                    doCast(i_pl, SPELL_HOTCOAL, true);
                                    i_pl->CombatStop(true);
                                }

                            me->CombatStop(true);
                            scheduleEvent(EV_COAL, 1000);
                            break;
                    }
                }
            }

        private:
            ScriptedInstance* _instance;
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new Mob_Hot_CoalAI(creature);
    }
};

bool GossipHello_boss_majordomo(Player *player, Creature *_Creature)
{
    if (((Boss_Majordomo::Boss_MajordomoAI*)_Creature->getAI())->_instance)
    {
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_SUMMON_1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);

        player->PlayerTalkClass->SendGossipMenu(TEXT_ID_SUMMON_1,_Creature->GetGUID());
    }

    return true;
}

bool GossipSelect_boss_majordomo(Player *player, Creature *_Creature, uint32 sender, uint32 action )
{
    switch (action)
    {
        case GOSSIP_ACTION_INFO_DEF + 1:
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_SUMMON_2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
            player->SEND_GOSSIP_MENU(TEXT_ID_SUMMON_2, _Creature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF + 2:
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_SUMMON_3, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
            player->SEND_GOSSIP_MENU(TEXT_ID_SUMMON_3, _Creature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF + 3:
            player->CLOSE_GOSSIP_MENU();
            _Creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            break;
    }

    return true;
}

void AddSC_boss_majordomo()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name="Boss_Majordomo";
    newscript->pGossipHello = &GossipHello_boss_majordomo;
    newscript->pGossipSelect = &GossipSelect_boss_majordomo;
    newscript->RegisterSelf();

    sScriptMgr.addScript(new Boss_Majordomo());
    sScriptMgr.addScript(new Mob_FlameWalker_Healer());
    sScriptMgr.addScript(new Mob_FlameWalker_Elite());
    sScriptMgr.addScript(new Mob_Hot_Coal);
}

