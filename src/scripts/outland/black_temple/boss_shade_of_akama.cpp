/* TODO : 
- Delay & Akala placement on event start
- Correc sorcerers timers
- Defender not sensible to aggro until they reach Akama
*/

#include "precompiled.h"
#include "def_black_temple.h"

enum Texts {
    SAY_LOW_HEALTH             = -1564013,
    SAY_DEATH                  = -1564014,
    
    // Ending cinematic texts
    SAY_FREE                   = -1564015, 
    SAY_BROKEN_FREE_01         = -1564016,
    SAY_BROKEN_FREE_02         = -1564017,

    GOSSIP_HELLO               = 393,
    GOSSIP_ITEM                = 394
};

struct Location
{
    float x, y, o, z;
};

static Location ChannelerLocations[]=
{
    {463.161285, 401.219757, 3.141592, 118.54},
    {457.377625, 391.227661, 2.106461, 118.54},
    {446.012421, 391.227661, 1.071904, 118.54},
    {439.533783, 401.219757, 0.000000, 118.54},
    {446.012421, 411.211853, 5.210546, 118.54},
    {457.377625, 411.211853, 4.177494, 118.54}
};

static Location SpawnLocations[]=
{
    {498.652740, 461.728119, 0, 113.537949},
    {498.505003, 339.619324, 0, 113.537949}
};

static Location AkamaWP[]=
{
    //begin event
    {515.32,   400.793, M_PI, 112.784},
    //end event
    {482.35,   401.163,    0, 112.784},
    {469.35,   400.880,    0, 118.540}
};

static Location BrokenCoords[]=
{
    {541.375916, 401.439575, M_PI,     112.783997},         // The place where Akama channels
    {534.130005, 352.394531, 2.164150, 112.783737},         // Behind a 'pillar' which is behind the east alcove
    {499.621185, 341.534729, 1.652856, 112.783730},         // East Alcove
    {499.151093, 461.036438, 4.770888, 112.783700},         // West Alcove
};

static Location BrokenWP[]=
{
    {492.491638, 400.744690, 3.122336, 112.783737},
    {494.335724, 382.221771, 2.676230, 112.783737},
    {489.555939, 373.507202, 2.416263, 112.783737},
    {491.136353, 427.868774, 3.519748, 112.783737},
};

enum AkamaSpells {
    SPELL_VERTEX_SHADE_BLACK    =    39833,
    SPELL_SHADE_SOUL_CHANNEL    =    40401, //aura dummy + trigger SPELL_SHADE_SOUL_CHANNEL_2 (see Aura::HandleAuraDummy)
    SPELL_DESTRUCTIVE_POISON    =    40874,
    SPELL_LIGHTNING_BOLT        =    42024,
    SPELL_AKAMA_SOUL_CHANNEL    =    40447, //self stun + dummy aura on target
    SPELL_AKAMA_SOUL_RETRIEVE   =    40902,
    SPELL_AKAMA_SOUL_EXPEL      =    40855,
    SPELL_SHADE_SOUL_CHANNEL_2  =    40520, //descrease speed
    SPELL_AKAMA_STEALTH         =    34189
};

#define TIMER_SPELL_DESTRUCTIVE_POISON 6000 + rand()%1000
#define TIMER_SPELL_LIGHTNING_BOLT 10000
#define TIMER_SUMMON_DEFENDER 15000
#define TIMER_SUMMON_PACK 35000
#define TIMER_SUMMON_PACK_FIRST 5000

enum AkamaCreatures {
    CREATURE_CHANNELER          =    23421,
    CREATURE_SORCERER           =    23215,
    CREATURE_DEFENDER           =    23216,
    CREATURE_BROKEN             =    23319,

    CREATURE_ELEMENTALIST       =    23523,
    CREATURE_ROGUE              =    23318,
    CREATURE_SPIRITBINDER       =    23524
};

const uint32 spawnEntries[]= { CREATURE_ELEMENTALIST, CREATURE_ROGUE, CREATURE_SPIRITBINDER };

struct mob_ashtongue_channelerAI : public ScriptedAI
{
    mob_ashtongue_channelerAI(Creature* c) : 
        ScriptedAI(c),
        ShadeGUID(0),
        checkTimer(0)
    { }

    uint64 ShadeGUID;
    uint32 checkTimer;

    enum Messages {
        SMESSAGE_SHADE_GUID
    };
   
    uint64 message(uint32 id, uint64 data) 
    {
        if(id == SMESSAGE_SHADE_GUID)
            ShadeGUID = data;        
            
        return 0;     
    }

    bool HasMyStack(Creature* c)
    {
        Aura* myStack = c->GetAuraByCasterSpell(SPELL_SHADE_SOUL_CHANNEL,me->GetGUID());
        if(myStack)
            return true;
        else
            return false;
    }

    void removeMyStackIfAny(Creature* shade)
    {
        Aura* myStack = shade->GetAuraByCasterSpell(SPELL_SHADE_SOUL_CHANNEL,me->GetGUID());
        if(myStack)
            myStack->SetRemoveMode(AURA_REMOVE_BY_DEFAULT);
    }

    void JustDied(Unit* killer)
    {
        Creature* shade = me->GetMap()->GetCreatureInMap(ShadeGUID);
        if(shade)
            removeMyStackIfAny(shade);
    }
    void AttackStart(Unit* who) {}
    void MoveInLineOfSight(Unit* who) {}
    void UpdateAI(const uint32 diff) 
    {
        if(checkTimer < diff)
        {
        if(!ShadeGUID)
            return;

        Creature* shade = me->GetMap()->GetCreatureInMap(ShadeGUID);
        if(!shade)
            return;

        if(HasMyStack(shade) && me->IsNonMeleeSpellCasted(false))
            return;

        me->CastStop();
        removeMyStackIfAny(shade);

        if(me->GetDistance2d(shade) < 20.0f && me->IsWithinLOSInMap(shade))
        {
            me->GetMotionMaster()->Clear(false);
            me->GetMotionMaster()->MoveIdle();
            me->CastSpell(shade, SPELL_SHADE_SOUL_CHANNEL, false);
        } else {
            if(!me->isMoving())
                me->GetMotionMaster()->MoveChase(shade);
        }
        checkTimer = 2000;
        } else checkTimer -= diff;
    }
};

struct mob_ashtongue_sorcererAI : public mob_ashtongue_channelerAI
{
    mob_ashtongue_sorcererAI(Creature* c) : mob_ashtongue_channelerAI(c) {}
};

struct boss_shade_of_akamaAI : public ScriptedAI
{
    boss_shade_of_akamaAI(Creature* c) : ScriptedAI(c), summons(me)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
        AkamaGUID = pInstance ? pInstance->GetData64(DATA_AKAMA_SHADE) : NULL;
        me->setActive(true);//if view distance is too low
    }
    
    enum Messages {
        SMESSAGE_AKAMA_GUID,
        SMESSAGE_SELECTABLE_CHANNELERS,
        SMESSAGE_IN_COMBAT,
        SMESSAGE_HAS_KILLED_AKAMA,
        SMESSAGE_AKAMA_RESET,
        QMESSAGE_IS_BANISHED
    };

    ScriptedInstance* pInstance;

    std::list<uint64> Channelers;
    uint64 AkamaGUID;

    uint32 SummonTimer;
    uint32 ResetTimer; //Reset encounter timer on Akama's death
    uint32 DefenderTimer;                                   

    bool IsBanished;
    bool HasKilledAkama;
    bool reseting;
    bool HasKilledAkamaAndReseting;
    SummonList summons;

    uint64 message(uint32 id, uint64 data)
    {
        switch(id)
        {
        case SMESSAGE_AKAMA_GUID:
            AkamaGUID = data;
            break;
        case SMESSAGE_SELECTABLE_CHANNELERS:
            SetSelectableChannelers();
            break;
        case SMESSAGE_IN_COMBAT: 
            InCombat = true;
            break;
        case SMESSAGE_HAS_KILLED_AKAMA:
            HasKilledAkama = (bool)data;
            break;
        case SMESSAGE_AKAMA_RESET:
            EnterEvadeMode();
        case QMESSAGE_IS_BANISHED:
            return (uint64)IsBanished;
        }
        return 0;
    }

    void MoveInLineOfSight(Unit* who) {}
    void Reset()
    {
        InCombat = false;
        reseting = true;
        HasKilledAkamaAndReseting = false;

        summons.DespawnAll();

        if(Creature* Akama = me->GetMap()->GetCreatureInMap(AkamaGUID))
        {
            if(Akama->isDead())
            {
                Akama->Respawn();//respawn akama if dead
                Akama->AI()->EnterEvadeMode();
            }
        }

        SummonTimer = TIMER_SUMMON_PACK_FIRST;
        ResetTimer = 30000;
        DefenderTimer = TIMER_SUMMON_DEFENDER;

        IsBanished = true;
        HasKilledAkama = false;

        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_STUN);
        
        reseting = false;

        if(pInstance && me->isAlive())
            pInstance->SetData(DATA_SHADEOFAKAMAEVENT, NOT_STARTED);
        else
            return;

        SpawnChannelers();
    }

    void DamageTaken(Unit *done_by, uint32 &damage) 
    {
        if(HasKilledAkamaAndReseting || reseting)
        {
            damage = 0;
            EnterEvadeMode();
        }
    }

    void JustDied(Unit* killer)
    {
        summons.DespawnAll();
        
        WorldPacket data;
        me->BuildHeartBeatMsg(&data);
        me->SendMessageToSet(&data,true);
    }

    void JustSummoned(Creature *summon) 
    {
        summons.Summon(summon);
    }

    void SummonedCreatureDespawn(Creature *summon) 
    {
        summons.Despawn(summon);
    }
    
    void KillRemainingChannelers()
    {
        for(auto itr : Channelers)
            if(Creature* Channeler = me->GetMap()->GetCreatureInMap(itr))
                Channeler->DisappearAndDie();
    }

    void SpawnChannelers()
    {
        me->RemoveAurasDueToSpell(SPELL_SHADE_SOUL_CHANNEL);
        KillRemainingChannelers();

        for(uint8 i = 0; i < 6; i++)
        {
            Creature* Channeler = me->SummonCreature(CREATURE_CHANNELER, ChannelerLocations[i].x, ChannelerLocations[i].y, ChannelerLocations[i].z, ChannelerLocations[i].o, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 15000);
            if(Channeler)
            {
                Channeler->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                Channeler->AI()->message(mob_ashtongue_channelerAI::SMESSAGE_SHADE_GUID,me->GetGUID());
                Channelers.push_back(Channeler->GetGUID());
            }
        }
    }

    void SummonCreaturesPack()
    {
        for(uint8 i = 0; i < 2; i++)
        {
            for(uint8 j = 0; j < 3; ++j)
            {
                Creature* Spawn = me->SummonCreature(spawnEntries[j], SpawnLocations[i].x, SpawnLocations[i].y, SpawnLocations[i].z, 0, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 15000);
                if(Spawn)
                {
                    Spawn->RemoveUnitMovementFlag(MOVEMENTFLAG_WALK_MODE);
                    Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 0,100.0f,true);
                    if(target) {
                        Spawn->AI()->AttackStart(target);
                    } else {
                        if(Creature* Akama = me->GetMap()->GetCreatureInMap(AkamaGUID))
                            Spawn->AI()->AttackStart(Akama);
                    }
                }
            }
        }
    }

    void SummonSorcerer()
    {
        uint32 random = rand()%2;
        Creature* Sorcerer = me->SummonCreature(CREATURE_SORCERER, SpawnLocations[random].x, SpawnLocations[random].y, SpawnLocations[random].z, 0, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 15000);
        if(Sorcerer)
        {
            Sorcerer->AI()->message(mob_ashtongue_sorcererAI::SMESSAGE_SHADE_GUID,me->GetGUID());
            Sorcerer->RemoveUnitMovementFlag(MOVEMENTFLAG_WALK_MODE);
            Sorcerer->GetMotionMaster()->MovePoint(0, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ());
            Sorcerer->SetUInt64Value(UNIT_FIELD_TARGET, me->GetGUID());
            Channelers.push_back(Sorcerer->GetGUID());
        }
    }

    void SummonDefender()
    {
        uint32 ran = rand()%2;
        Creature* Defender = me->SummonCreature(CREATURE_DEFENDER, SpawnLocations[ran].x, SpawnLocations[ran].y, SpawnLocations[ran].z, 0, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 15000);
        if(Defender)
        {
            Defender->RemoveUnitMovementFlag(MOVEMENTFLAG_WALK_MODE);
            if(Creature* Akama = me->GetMap()->GetCreatureInMap(AkamaGUID))
            {
                float x, y, z;
                Akama->GetPosition(x,y,z);
                Defender->GetMotionMaster()->MovePoint(0, x, y, z);
                Defender->AI()->AttackStart(Akama);
                Defender->AddThreat(Akama, 5000.0f);
            }
        }
    }

    void SetSelectableChannelers()
    {
        for(auto itr : Channelers)
            if(Creature* Channeler = me->GetMap()->GetCreatureInMap(itr))
                Channeler->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
    }

    void RewardRepFromAkamaDeath()
    {
        Creature* Akama = Unit::GetCreature((*me), AkamaGUID);
        if(!Akama)
            return;

        for(auto itr : me->getThreatManager().getThreatList())
        {
            Player* p = me->GetPlayer(itr->getUnitGuid());
            if(p)
                p->RewardReputation(Akama,1);
        }
    }

    bool HasReachedAkama()
    {
        if(Creature* Akama = Unit::GetCreature(*me, AkamaGUID))
        {
            if(me->GetDistance2d(Akama) < 5.0f)
                return true;
        }
        return false;
    }

    void UpdateAI(const uint32 diff)
    {
        if(!InCombat)
            return;

        if(IsBanished)
        {
            if(DefenderTimer < diff)
            {
                SummonDefender();
                if(me->GetAuraCount(SPELL_SHADE_SOUL_CHANNEL) < 6)
                    SummonSorcerer();
                DefenderTimer = TIMER_SUMMON_DEFENDER;
            }else DefenderTimer -= diff;

            if(SummonTimer < diff)
            {
                SummonCreaturesPack();
                SummonTimer = TIMER_SUMMON_PACK;
            }else SummonTimer -= diff;

            if(AkamaGUID)
            {
                Creature* Akama = me->GetMap()->GetCreatureInMap(AkamaGUID);
                if(Akama && Akama->isAlive())
                {
                    if(HasReachedAkama())
                    {
                        KillRemainingChannelers();
                        IsBanished = false;
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        Akama->Attack(me,true);
                    }
                }
            }
        }

        if(HasKilledAkama)
        {
            if(!HasKilledAkamaAndReseting)//do not let players kill Shade if Akama is dead and Shade is waiting for ResetTimer!! event would bug
            {
                HasKilledAkamaAndReseting = true;
                me->RemoveAllAuras();
                RewardRepFromAkamaDeath();
                me->DeleteThreatList();
                me->CombatStop();
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                me->GetMotionMaster()->MoveTargetedHome();
                summons.DespawnAll();
                me->SetReactState(REACT_PASSIVE);
                me->RemoveAurasDueToSpell(SPELL_SHADE_SOUL_CHANNEL_2);//just to hide it from players
                if(pInstance && me->isAlive())
                    pInstance->SetData(DATA_SHADEOFAKAMAEVENT, NOT_STARTED);
                //reward players with reputation
            }
            if(ResetTimer < diff)
            {
                EnterEvadeMode();// Reset a little while after killing Akama, evade and respawn Akama
                return;
            }else ResetTimer -= diff;
        }

        if(!IsBanished)
            DoMeleeAttackIfReady();
    }
};

struct npc_akamaAI : public ScriptedAI
{
    npc_akamaAI(Creature* c) : ScriptedAI(c), summons(me)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
        ShadeGUID = pInstance ? pInstance->GetData64(DATA_SHADEOFAKAMA) : NULL;
        me->setActive(true);        
        Creature* Shade = me->GetMap()->GetCreatureInMap(ShadeGUID);
        if(Shade)
            Shade->AI()->message(boss_shade_of_akamaAI::SMESSAGE_AKAMA_GUID,me->GetGUID());
    }

    ScriptedInstance* pInstance;

    uint64 ShadeGUID;

    uint32 DestructivePoisonTimer;
    uint32 LightningBoltTimer;
    uint32 CheckTimer; // ShadeHasDied check
    uint32 CastSoulRetrieveTimer;
    uint32 SoulRetrieveTimer;
    uint32 SummonBrokenTimer;
    uint32 EndingTalkCount;
    uint32 BrokenSummonIndex;
    uint8 outroProgress;
    uint8 introProgress;
    uint32 introDelayTimer;
    bool SummoningBrokenEvent;

    bool EventBegun; //players started event
    bool ShadeHasDied;
    bool StartCombat; //shade reached akama
    bool HasYelledOnce; // low health yell
    SummonList summons;

    void Reset()
    {
        EventBegun = false;
        ShadeHasDied = false;
        StartCombat = false;
        HasYelledOnce = false;
        DestructivePoisonTimer = TIMER_SPELL_DESTRUCTIVE_POISON;
        LightningBoltTimer = TIMER_SPELL_LIGHTNING_BOLT;
        CheckTimer = 2000;
        m_creature->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_HEAL, true);
        summons.DespawnAll();
        me->SetReactState(REACT_PASSIVE);
        me->AI()->SetCombatMovementAllowed(false);
        if (pInstance && pInstance->GetData(DATA_SHADEOFAKAMAEVENT) != DONE)
        {
            DoCast(me, SPELL_AKAMA_STEALTH, true);
            me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            Creature* Shade = me->GetMap()->GetCreatureInMap(ShadeGUID);
            if(Shade)
                Shade->AI()->message(boss_shade_of_akamaAI::SMESSAGE_AKAMA_RESET,0); // also reset shadow
        } else {
            me->Relocate(AkamaWP[2].x, AkamaWP[2].y, AkamaWP[2].z, AkamaWP[2].o);
        }
        
        outroProgress = 0;
        SoulRetrieveTimer = 16000;
        SummonBrokenTimer = 0;
        CastSoulRetrieveTimer = 0;
        EndingTalkCount = 0;
        BrokenSummonIndex = 0;
        SummoningBrokenEvent = false;

        introProgress = 0;
        introDelayTimer = 5000;
    }

    void JustSummoned(Creature *summon) 
    {
        summons.Summon(summon);
    }
    void SummonedCreatureDespawn(Creature *summon) 
    {
        summons.Despawn(summon);
    }

    void HealReceived(Unit* done_by, uint32& addhealth)
    {
        addhealth = 0;
    }

    void BeginEvent(Player* pl)
    {
        if(!pInstance)
            return;

        ShadeGUID = pInstance->GetData64(DATA_SHADEOFAKAMA);
        if(!ShadeGUID)
            return;

        Creature* Shade = me->GetMap()->GetCreatureInMap(ShadeGUID);
        if(Shade && Shade->isAlive())
        {
            pInstance->SetData(DATA_SHADEOFAKAMAEVENT, IN_PROGRESS);
            me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            introProgress = 1;
        }
    }

    void JustDied(Unit* killer)
    {
        DoScriptText(SAY_DEATH, me);
        Creature* Shade = me->GetMap()->GetCreatureInMap(ShadeGUID);
        if(Shade && Shade->isAlive())
            Shade->AI()->message(boss_shade_of_akamaAI::SMESSAGE_HAS_KILLED_AKAMA,1);
        summons.DespawnAll();
    }

    void UpdateAI(const uint32 diff)
    {
        if(!EventBegun)
        {
            HandleIntro(diff);
            return;
        }

        if ((me->GetHealth()*100 / me->GetMaxHealth()) < 15 && !HasYelledOnce)
        {
            DoScriptText(SAY_LOW_HEALTH, me);
            HasYelledOnce = true;
        }

        if(ShadeGUID && !StartCombat)
        {
            Creature* Shade = (Unit::GetCreature((*me), ShadeGUID));
            if(Shade && Shade->isAlive())
            {
                bool IsBanished = Shade->AI()->message(boss_shade_of_akamaAI::QMESSAGE_IS_BANISHED,0);
                if(!IsBanished)
                {
                    me->InterruptNonMeleeSpells(false);
                    StartCombat = true;
                }
            }
        }

        if(!ShadeHasDied && StartCombat)
        {
            if(CheckTimer < diff)
            {
                if(ShadeGUID)
                {
                    Creature* Shade = me->GetMap()->GetCreatureInMap(ShadeGUID);
                    if(Shade && !Shade->isAlive())
                    {
                        ShadeHasDied = true;
                        if(pInstance) pInstance->SetData(DATA_SHADEOFAKAMAEVENT, DONE);
                        outroProgress = 1;
                        me->SetReactState(REACT_PASSIVE);
                    }
                }
                CheckTimer = 5000;
            }else CheckTimer -= diff;
        }

        HandleOutro(diff);

        if(!StartCombat)
            return;

        if(DestructivePoisonTimer <= diff)
        {
            Creature* Shade = me->GetMap()->GetCreatureInMap(ShadeGUID);
            if (Shade && Shade->isAlive())
                DoCast(Shade, SPELL_DESTRUCTIVE_POISON);
            DestructivePoisonTimer = TIMER_SPELL_DESTRUCTIVE_POISON;
        }else DestructivePoisonTimer -= diff;

        if(LightningBoltTimer < diff)
        {
            DoCast(me->getVictim(), SPELL_LIGHTNING_BOLT);
            LightningBoltTimer = TIMER_SPELL_LIGHTNING_BOLT;
        }else LightningBoltTimer -= diff;

        DoMeleeAttackIfReady();
    }

    void HandleIntro(const uint32 diff)
    {
        switch(introProgress)
        {
        case 0: break;
        case 1: 
            me->GetMotionMaster()->MovePoint(0, AkamaWP[0].x, AkamaWP[0].y, AkamaWP[0].z);
            introProgress = 0; // re set in MovementInform when point reached
            break;
        case 2:
            {
            Creature* Shade = me->GetMap()->GetCreatureInMap(ShadeGUID);
            if (Shade && Shade->isAlive())
            {
                me->SetReactState(REACT_AGGRESSIVE);
                Shade->SetReactState(REACT_AGGRESSIVE);
                Shade->AI()->message(boss_shade_of_akamaAI::SMESSAGE_AKAMA_GUID,me->GetGUID());
                Shade->AI()->message(boss_shade_of_akamaAI::SMESSAGE_IN_COMBAT,1);
                DoZoneInCombat(Shade);
                AttackStart(Shade,false);
                Shade->AI()->AttackStart(me);
                me->AddThreat(Shade, 10000000.0f);
                Shade->AddThreat(me, 10000000.0f);
                DoCast(Shade, SPELL_AKAMA_SOUL_CHANNEL);
            } else { 
                Reset();
                return; 
            }
            }
            introProgress = 3;
            break;
        case 3:
            if(introDelayTimer < diff)
            {
                EventBegun = true;
                Creature* Shade = me->GetMap()->GetCreatureInMap(ShadeGUID);
                if (Shade) Shade->AI()->message(boss_shade_of_akamaAI::SMESSAGE_SELECTABLE_CHANNELERS,0);
                introProgress = 0; //intro end
            } else introDelayTimer -= diff;
            break;
        }
    }

    void HandleOutro(const uint32 diff)
    {
        switch(outroProgress)
        {
        case 0: break;
        case 1:
            me->SetUnitMovementFlags(MOVEMENTFLAG_WALK_MODE);
            me->GetMotionMaster()->MovePoint(1, AkamaWP[1].x, AkamaWP[1].y, AkamaWP[1].z);
            outroProgress = 0; // re set in MovementInform when point reached
            break;
        case 2:
            me->GetMotionMaster()->MovePoint(2, AkamaWP[2].x, AkamaWP[2].y, AkamaWP[2].z);
            outroProgress = 0; // re set in MovementInform when point reached
            break;
        case 3:
            if(Creature* Shade = me->GetMap()->GetCreatureInMap(ShadeGUID))
            {
                me->SetUInt64Value(UNIT_FIELD_TARGET, ShadeGUID);
                DoCast(Shade, SPELL_AKAMA_SOUL_RETRIEVE,true);
            }
            outroProgress = 4;
            break;
        case 4:
            if(SoulRetrieveTimer <= diff)
            {
                switch(EndingTalkCount)
                {
                case 0:
                    me->HandleEmoteCommand(EMOTE_ONESHOT_ROAR);
                    ++EndingTalkCount;
                    SoulRetrieveTimer = 2000;
                    SummoningBrokenEvent = true;
                    break;
                case 1:
                    DoScriptText(SAY_FREE, me);
                    ++EndingTalkCount;
                    SoulRetrieveTimer = 25000;
                    break;
                case 2:
                    {
                    bool Yelled = false;
                    for(auto itr : summons)
                        if(Creature* pUnit = me->GetMap()->GetCreatureInMap(itr))
                        {
                            if(!Yelled)
                            {
                                DoScriptText(SAY_BROKEN_FREE_01, pUnit);
                                Yelled = true;
                            }
                            pUnit->HandleEmoteCommand(EMOTE_ONESHOT_KNEEL);
                        }
                    }
                    ++EndingTalkCount;
                    SoulRetrieveTimer = 1500;
                    break;
                case 3:
                    for(auto itr : summons)
                        if(Creature* pUnit = me->GetMap()->GetCreatureInMap(itr))
                            // This is the incorrect spell, but can't seem to find the right one.
                            pUnit->CastSpell(pUnit, 39656, true);
                    ++EndingTalkCount;
                    SoulRetrieveTimer = 5000;
                    break;
                case 4:
                    bool firstSkipped = false; //the first one made the call !
                    for(auto itr : summons)
                        if(Creature* pUnit = me->GetMap()->GetCreatureInMap(itr))
                        {
                            if(firstSkipped)
                                pUnit->MonsterYell(SAY_BROKEN_FREE_02, LANG_UNIVERSAL, 0);
                            else
                                firstSkipped = true;
                        }
                    outroProgress = 0; //outro end
                    break;
                }
            }else SoulRetrieveTimer -= diff;
            break;
        }

        if(SummoningBrokenEvent)
        {
            if(SummonBrokenTimer <= diff)
            {
                for(uint8 i = 0; i < 4; ++i)
                {
                    float x = BrokenCoords[BrokenSummonIndex].x + (i*5);
                    float y = BrokenCoords[BrokenSummonIndex].y + (1*5);
                    float z = BrokenCoords[BrokenSummonIndex].z;
                    float o = BrokenCoords[BrokenSummonIndex].o;
                    Creature* Broken = me->SummonCreature(CREATURE_BROKEN, x, y, z, o, TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 360000);
                    if(Broken)
                    {
                        float wx = BrokenWP[BrokenSummonIndex].x + (i*5);
                        float wy = BrokenWP[BrokenSummonIndex].y + (i*5);
                        float wz = BrokenWP[BrokenSummonIndex].z;
                        Broken->GetMotionMaster()->MovePoint(0, wx, wy, wz);
                        Broken->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    }
                }
                ++BrokenSummonIndex;
                SummonBrokenTimer = 1000;
            }else SummonBrokenTimer -= diff;

            if(BrokenSummonIndex == 4)
                SummoningBrokenEvent = false;
        }
    }

    void MovementInform(uint32 type, uint32 id)
    {
        if(type != POINT_MOTION_TYPE)
            return;

        switch(id)
        {
        case 0: introProgress = 2; break;
        case 1: outroProgress = 2; break;
        case 2: outroProgress = 3; break;
        }
    }
};

CreatureAI* GetAI_boss_shade_of_akama(Creature *_Creature)
{
    return new boss_shade_of_akamaAI (_Creature);
}

CreatureAI* GetAI_mob_ashtongue_channeler(Creature *_Creature)
{
    return new mob_ashtongue_channelerAI (_Creature);
}

CreatureAI* GetAI_mob_ashtongue_sorcerer(Creature *_Creature)
{
    return new mob_ashtongue_sorcererAI (_Creature);
}

CreatureAI* GetAI_npc_akama_shade(Creature *_Creature)
{
    return new npc_akamaAI (_Creature);
}

bool GossipSelect_npc_akama(Player *player, Creature *_Creature, uint32 sender, uint32 action )
{
    if (action == GOSSIP_ACTION_INFO_DEF + 1)               //Fight time
    {
        player->CLOSE_GOSSIP_MENU();
        ((npc_akamaAI*)_Creature->AI())->BeginEvent(player);
        _Creature->RemoveAurasDueToSpell(SPELL_AKAMA_STEALTH);
    }

    return true;
}

bool GossipHello_npc_akama(Player *player, Creature *_Creature)
{
    ScriptedInstance* pInstance = ((ScriptedInstance*)_Creature->GetInstanceData());
    if(pInstance && pInstance->GetData(DATA_SHADEOFAKAMAEVENT) == DONE)
    {
        _Creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
        return true;
    }
        
    if(player->isAlive())
    {
       // player->PlayerTalkClass->GetGossipMenu().AddMenuItem(0, GOSSIP_ITEM, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        player->ADD_GOSSIP_ITEM( 0, (int)GOSSIP_ITEM, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        player->SEND_GOSSIP_MENU(GOSSIP_HELLO, _Creature->GetGUID());
    }

    return true;
}

void AddSC_boss_shade_of_akama()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name="boss_shade_of_akama";
    newscript->GetAI = &GetAI_boss_shade_of_akama;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="mob_ashtongue_channeler";
    newscript->GetAI = &GetAI_mob_ashtongue_channeler;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="mob_ashtongue_sorcerer";
    newscript->GetAI = &GetAI_mob_ashtongue_sorcerer;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="npc_akama_shade";
    newscript->GetAI = &GetAI_npc_akama_shade;
    newscript->pGossipHello = &GossipHello_npc_akama;
    newscript->pGossipSelect = &GossipSelect_npc_akama;
    newscript->RegisterSelf();
}

