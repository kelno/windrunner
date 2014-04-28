#include "precompiled.h"
#include "def_sunwell_plateau.h"

enum SpellIds
{
    /* Hand of the Deceiver's spells and cosmetics */
    SPELL_SHADOW_BOLT_VOLLEY                            = 45770, // ~30 yard range Shadow Bolt Volley for ~2k(?) damage
    SPELL_SHADOW_INFUSION                               = 45772, // They gain this at 20% - Immunity to Stun/Silence and makes them look angry!
    SPELL_FELFIRE_PORTAL                                = 46875, // Creates a portal that spawns Felfire Fiends (LIVE FOR THE SWARM!1 FOR THE OVERMIND!)
    SPELL_SHADOW_CHANNELING                             = 46757, // Channeling animation out of combat
    /* Felfire Portal */
    SPELL_SUMMON_FELFIRE_FIEND                          = 46464,
    
    /* Volatile Felfire Fiend's spells */
    SPELL_FELFIRE_FISSION                               = 45779, // Felfire Fiends explode when they die or get close to target.

    /* Kil'Jaeden's spells and cosmetics */
    SPELL_TRANS                                         = 23188, // Surprisingly, this seems to be the right spell.. (Where is it used?)
    SPELL_REBIRTH                                       = 44200, // Emerge from the Sunwell
    SPELL_SOUL_FLAY                                     = 45442, // 9k Shadow damage over 3 seconds. Spammed throughout all the fight.
    SPELL_LEGION_LIGHTNING                              = 45664, // Chain Lightning, 4 targets, ~3k Shadow damage, 1.5k mana burn
    SPELL_FIRE_BLOOM                                    = 45641, // Places a debuff on 5 raid members, which causes them to deal 2k Fire damage to nearby allies and selves. MIGHT NOT WORK


    SPELL_SINISTER_REFLECTION                           = 45892,
    SPELL_SINISTER_REFLECTION_CLONE                     = 45785, // Summon shadow copies of 5 raid members that fight against KJ's enemies
    SPELL_SINISTER_REFLECTION_CLASS                     = 45893, // Increase the size of the clones

    SPELL_SHADOW_SPIKE                                  = 46680, // Bombard random raid members with Shadow Spikes (Very similar to Void Reaver orbs)
    SPELL_FLAME_DART                                    = 45737, // Bombards the raid with flames every 3(?) seconds
    SPELL_DARKNESS_OF_A_THOUSAND_SOULS                  = 46605, // Begins a 8-second channeling, after which he will deal 50'000 damage to the raid

    /* Armageddon spells wrong visual */
    SPELL_ARMAGEDDON_TRIGGER                            = 45909, // Meteor spell trigger missile should cast creature on himself
    SPELL_ARMAGEDDON_VISUAL                             = 45911, // Does the hellfire visual to indicate where the meteor missle lands
    SPELL_ARMAGEDDON_VISUAL2                            = 45914, // Does the light visual to indicate where the meteor missle lands
    SPELL_ARMAGEDDON_VISUAL3                            = 24207, // This shouldn't correct but same as seen on the movie
    SPELL_ARMAGEDDON_PERIODIC_SUMMON                    = 45921, // Summon trigger on target that cast the other spells on himself
    SPELL_ARMAGEDDON_SUMMON_TRIGGER                     = 45910, // actually summon trigger 25735
    SPELL_ARMAGEDDON_DAMAGE                             = 45915, // This does the area damage

    /* Shield Orb Spells*/
    SPELL_SHADOW_BOLT                                   = 45680, //45679 would be correct but triggers to often //TODO fix console error


    /* Anveena's spells and cosmetics (Or, generally, everything that has "Anveena" in name) */
    SPELL_ANVEENA_PRISON                                = 46367, // She hovers locked within a bubble
    SPELL_ANVEENA_ENERGY_DRAIN                          = 46410, // Sunwell energy glow animation (Control mob uses this)
    SPELL_SACRIFICE_OF_ANVEENA                          = 46474, // This is cast on Kil'Jaeden when Anveena sacrifices herself into the Sunwell

    /* Sinister Reflection Spells */
    SPELL_SR_CURSE_OF_AGONY                             = 46190,
    SPELL_SR_SHADOW_BOLT                                = 47076,

    SPELL_SR_EARTH_SHOCK                                = 47071,

    SPELL_SR_FIREBALL                                   = 47074,

    SPELL_SR_HEMORRHAGE                                 = 45897,

    SPELL_SR_HOLY_SHOCK                                 = 38921,
    SPELL_SR_HAMMER_OF_JUSTICE                          = 37369,

    SPELL_SR_HOLY_SMITE                                 = 47077,
    SPELL_SR_RENEW                                      = 47079,

    SPELL_SR_SHOOT                                      = 16496,
    SPELL_SR_MULTI_SHOT                                 = 48098,
    SPELL_SR_WING_CLIP                                  = 40652,

    SPELL_SR_WHIRLWIND                                  = 17207,

    SPELL_SR_MOONFIRE                                   = 47072,

    /*** Other Spells (used by players, etc) ***/
    SPELL_VENGEANCE_OF_THE_BLUE_FLIGHT                  = 45839, // Possess the blue dragon from the orb to help the raid.
    SPELL_POWER_OF_THE_BLUE_FLIGHT                      = 45833,
    SPELL_POSSESS_DRAKE_IMMUNE                          = 45838, // immunity while the player possesses the dragon
    SPELL_RING_OF_BLUE_FLAMES                           = 45825, // Cast this spell when the go is activated
    SPELL_DESTROY_DRAKES                                = 46707,
    SPELL_VISUAL_MOONFIRE                               = 45821, // uppon orb activation
    SPELL_KNOCK_BACK                                    = 45800, // custom spell, no damage

    // outro
    SPELL_TELEPORT_VISUAL                               = 41232,
    SPELL_KALEC_TELEPORT                                = 46473, // teleports and transforms Kalec in human form
    SPELL_CALL_ENTROPIUS                                = 46818, // pulling effect
    SPELL_ENTROPIUS_BODY                                = 46819, // Visual for Entropius at the Epilogue
    SPELL_BLAZE_TO_LIGHT                                = 46821,
    SPELL_SUNWELL_IGNITION                              = 46822, // same visual as SPELL_ANVEENA_ENERGY_DRAIN
    SPELL_OPEN_PORTAL                                   = 46801,
    SPELL_PORTAL_VISUAL                                 = 42047,

};

/*** GameObjects ***/
#define GAMEOBJECT_ORB_OF_THE_BLUE_DRAGONFLIGHT 188415

/*** Others ***/
#define FLOOR_Z 28.050388
#define SHIELD_ORB_Z 45.000
#define CREATURE_INVISIBLE_DUMMY 9
#define GOSSIP_KALEC_END 25319
#define DISPLAYID_PORTAL_OPENING 22742

enum Phase
{
    PHASE_DECEIVERS     = 1, // Fight 3 adds
    PHASE_NORMAL        = 2, // Kil'Jaeden emerges from the sunwell
    PHASE_DARKNESS      = 3, // At 85%, he gains few abilities; Kalecgos joins the fight
    PHASE_ARMAGEDDON    = 4, // At 55%, he gains even more abilities
    PHASE_SACRIFICE     = 5, // At 25%, Anveena sacrifices herself into the Sunwell; at this point he becomes enraged and has *significally* shorter cooldowns.
};

//Events
enum KilJaedenEvents
{
    EVENT_KALEC_JOIN            = 0,

    //Phase 2
    EVENT_SOUL_FLAY             = 1,
    EVENT_LEGION_LIGHTNING      = 2,
    EVENT_FIRE_BLOOM            = 3,
    EVENT_SUMMON_SHILEDORB      = 4,

    //Phase 3
    EVENT_SHADOW_SPIKE          = 5,
    EVENT_FLAME_DART            = 6,
    EVENT_DARKNESS              = 7,
    EVENT_ORBS_EMPOWER          = 8,
    EVENT_SINISTER_REFLECTION   = 9,

    //Phase 4
    EVENT_ARMAGEDDON            = 10
};

enum KilJaedenControllerEvents
{
    EVENT_SAY                   = 0
};

enum HandOfTheDeceiverEvents
{
    EVENT_SHADOWBOLT            = 0,
    EVENT_FELFIRE               = 1
};

enum FelfirePortal
{
    EVENT_SPAWNFIEND            = 0
};

enum FelfireFiend
{
    EVENT_STUN                  = 0,
    EVENT_EXPLODE               = 1,
    EVENT_DIE_F                 = 2
};

enum Armageddontarget
{
    EVENT_VISUAL1               = 0,
    EVENT_TRIGGER               = 1,
    EVENT_DIE                   = 2
};

enum ShieldOrb
{
    EVENT_SHADOWBOLT_S          = 0
};

// Locations of the Hand of Deceiver adds
float DeceiverLocations[3][3]=
{
    {1687.777f, 638.084f, 5.527f},
    {1684.099f, 618.848f, 0.589f},
    {1704.398f, 612.848f, 1.884f},
};

// Locations, where Shield Orbs will spawn
float ShieldOrbLocations[4][2]=
{
    {1698.900f, 627.870f},       //middle pont of Sunwell
    {12.0f, 3.14f},              // First one spawns northeast of KJ
    {12.0f, 3.14f/0.7f},         // Second one spawns southeast
    {12.0f, 3.14f*3.8f}          // Third one spawns (?)
};

float OrbLocations[4][5] =
{
    (1694.48f, 674.29f,  28.0502f, 4.86985f),
    (1745.68f, 621.823f, 28.0505f, 2.93777f),
    (1704.14f, 583.591f, 28.1696f, 1.59003f),
    (1653.12f, 635.41f,  28.0932f, 0.0977725f),
};

float KJLocation[4] = { 1698.930, 628.264, 28.400, 3.7 };

struct Speech
{
    int32 textid;
    uint32 creature, timer;
};

enum Controller
{
    SAY_KJ_OFFCOMBAT = 0,
};

enum KilJaeden
{
    SAY_KJ_DEATH = 0,
    SAY_KJ_SLAY = 1,
    SAY_KJ_REFLECTION = 2,
    SAY_KJ_EMERGE = 3,
    SAY_KJ_DARKNESS = 4,
    SAY_KJ_PHASE3 = 5,
    SAY_KJ_PHASE4 = 6,
    SAY_KJ_PHASE5 = 7,
    EMOTE_KJ_DARKNESS = 8,
};

enum Kalecgos
{
    SAY_KALECGOS_ENCOURAGE = 0,
    SAY_KALEC_ORB_READY1 = 1,
    SAY_KALEC_ORB_READY2 = 2,
    SAY_KALEC_ORB_READY3 = 3,
    SAY_KALEC_ORB_READY4 = 4,
    /*SAY_KALECGOS_AWAKEN = 5,
    SAY_KALECGOS_LETGO = 6,
    SAY_KALECGOS_FOCUS = 7,
    SAY_KALECGOS_FATE = 8,
    SAY_KALECGOS_GOODBYE = 9, */
    SAY_KALEC_JOIN = 10,
    SAY_KALEC_1    = -1580082,
    SAY_KALEC_2    = -1580084,
    SAY_KALEC_3    = -1580086,
    SAY_KALEC_4    = -1580088,
    SAY_KALEC_5    = -1580091,

};

enum Anveena
{
    SAY_ANVEENA_1  = -1580083,
    SAY_ANVEENA_2  = -1580085,
    SAY_ANVEENA_3  = -1580087,
    SAY_ANVEENA_4  = -1580089,
};

// outro
enum Outro
{
    SAY_KALECGOS_GOODBYE        = -1580090,
    SAY_OUTRO_1                 = -1580099,
    SAY_OUTRO_2                 = -1580100,
    SAY_OUTRO_3                 = -1580111,
    SAY_OUTRO_4                 = -1580101,
    SAY_OUTRO_5                 = -1580107,
    SAY_OUTRO_6                 = -1580102,
    SAY_OUTRO_7                 = -1580108,
    SAY_OUTRO_8                 = -1580103,
    SAY_OUTRO_9                 = -1580104,
    SAY_OUTRO_10                = -1580109,
    SAY_OUTRO_11                = -1580105,
    SAY_OUTRO_12                = -1580106,
};

enum
{
    POINT_KILJAEDEN_DIE = 1,
    POINT_TELEPORT_KALECGOS,
    POINT_SUMMON_SHATTERED,
    POINT_SUMMON_PORTAL,
    POINT_SUMMON_PORTAL_ENDOPENANIM,
    POINT_SUMMON_SOLDIERS_RIGHT,
    POINT_SUMMON_SOLDIERS_LEFT,
    POINT_SUMMON_PROPHET,
    POINT_SUMMON_LIADRIN,
    POINT_CALL_ENTROPIUS,
    POINT_MOVE_LIADRIN,
    POINT_BLAZE,
    POINT_IGNITE,
    POINT_EVENT_SOLDIER_EXIT,
    POINT_EVENT_VELEN_EXIT,
    POINT_END_STUN,
};

static const DialogueEntry firstDialogue[] =
{
    {SAY_KALEC_1,                 CREATURE_KALECGOS,  5000},
    {SAY_ANVEENA_1,               CREATURE_ANVEENA,   3000},
    {0,                           0,                  0},
};

static const DialogueEntry secondDialogue[] =
{
    {SAY_KALEC_2,                 CREATURE_KALECGOS,  7000},
    {SAY_ANVEENA_2,               CREATURE_ANVEENA,   5000},
    {0,                           0,                  0},
};

static const DialogueEntry thirdDialogue[] =
{
    {SAY_KALEC_3,                 CREATURE_KALECGOS,  10000},
    {SAY_ANVEENA_3,               CREATURE_ANVEENA,   3000},
    {SAY_KALEC_4,                 CREATURE_KALECGOS,  5000},
    {SAY_ANVEENA_4,               CREATURE_ANVEENA,   6000},
    {SAY_KALEC_5,                 CREATURE_KALECGOS,  5000},
    {SAY_KJ_PHASE5,               CREATURE_KILJAEDEN, 5000},
    {POINT_END_STUN,              0,                  0},
    {0,                           0,                  0},
};

// Epilogue dialogue
static const DialogueEntry aOutroDialogue[] =
{
    {POINT_KILJAEDEN_DIE,         0,                  15000},
    {POINT_TELEPORT_KALECGOS,     0,                  2000},
    {SAY_KALECGOS_GOODBYE,        CREATURE_KALECGOS,  15000},
    {POINT_SUMMON_SHATTERED,      0,                  15000},
    {POINT_SUMMON_PORTAL,         0,                  500},
    {POINT_SUMMON_PORTAL_ENDOPENANIM, 0,              3500},
    {POINT_SUMMON_SOLDIERS_RIGHT, 0,                  8000},
    {POINT_SUMMON_SOLDIERS_LEFT,  0,                  10000},
    {POINT_SUMMON_PROPHET,        0,                  2000},
    {POINT_SUMMON_LIADRIN,        0,                  4000},
    {SAY_OUTRO_1,                 CREATURE_PROPHET,   25000},
    {SAY_OUTRO_2,                 CREATURE_PROPHET,   14000},
    {SAY_OUTRO_3,                 CREATURE_PROPHET,   10000},
    {POINT_CALL_ENTROPIUS,        0,                  10000},
    {SAY_OUTRO_4,                 CREATURE_PROPHET,   22000},
    {POINT_MOVE_LIADRIN,          0,                  6000},
    {SAY_OUTRO_5,                 CREATURE_LIADRIN,   10000},
    {SAY_OUTRO_6,                 CREATURE_PROPHET,   15000},
    {SAY_OUTRO_7,                 CREATURE_LIADRIN,   2500},
    {SAY_OUTRO_8,                 CREATURE_PROPHET,   4000},
    {POINT_BLAZE,                 0,                  10000},
    {POINT_IGNITE,                0,                  500},
    {SAY_OUTRO_9,                 CREATURE_PROPHET,   15000},
    {SAY_OUTRO_10,                CREATURE_LIADRIN,   20000},
    {SAY_OUTRO_11,                CREATURE_PROPHET,   6000},
    {SAY_OUTRO_12,                CREATURE_PROPHET,   2000},
    {POINT_EVENT_SOLDIER_EXIT,    0,                  8000},
    {POINT_EVENT_VELEN_EXIT,      0,                  0},
    {0,                           0,                  0},
};

struct EventLocations
{
    float m_fX, m_fY, m_fZ, m_fO;
};

static const EventLocations aOutroLocations[] =
{
    {1728.337f, 656.641f, 28.55f, 3.86f},       // portal summon loc
    {1703.159f, 654.043f, 28.05f, 1.06f},       // first shattered summon loc
    {1723.888f, 631.342f, 28.05f, 0.16f},       // second shattered summon loc
    {1716.969f, 646.407f, 28.05f, 3.91f},       // velen summon loc
    {1718.862f, 644.528f, 28.05f, 3.87f},       // liadrin summon loc
    {1709.719f, 639.359f, 27.28f},              // velen move forward
    {1711.537f, 637.600f, 27.34f},              // liadrin move forward
    {1716.962f, 661.839f, 28.05f},              // first shattered move
    {1735.141f, 643.680f, 28.23f},              // second shattered move
    {1723.220f, 663.026f, 30.00f},              // first shattered spell target
    {1734.877f, 649.574f, 30.00f},              // second shattered spell target
};

static const EventLocations SoldierLocations[] =
{
    {1722.709f, 640.308f, 28.05f, 3.774}, // summon first
    {1727.329f, 639.419f, 28.05f, 3.721}, // summon first
    {1724.606f, 645.376f, 28.05f, 3.755}, // summon first
    {1719.543f, 644.635f, 28.05f, 3.718}, // summon first
    {1720.937f, 649.376f, 28.05f, 3.859}, // summon first
    {1715.420f, 648.200f, 28.05f, 4.507}, // summon first
    {1715.948f, 653.606f, 28.05f, 3.892}, // summon first
    {1710.871f, 651.645f, 28.05f, 4.252}, // summon first
    {1709.924f, 656.442f, 28.05f, 4.035}, // summon first
    {1705.944f, 654.201f, 28.05f, 4.290}, // summon first
    {1707.783f, 653.139f, 28.05f, 4.375}, // summon second
    {1712.047f, 655.015f, 28.05f, 4.208}, // summon second
    {1712.432f, 650.853f, 28.05f, 4.166}, // summon second
    {1717.019f, 652.191f, 28.05f, 3.986}, // summon second
    {1716.611f, 647.399f, 28.05f, 4.060}, // summon second
    {1721.737f, 648.310f, 28.05f, 3.893}, // summon second
    {1720.509f, 643.163f, 28.05f, 3.851}, // summon second
    {1725.338f, 643.300f, 28.05f, 3.634}, // summon second
    {1723.557f, 638.104f, 28.05f, 3.401}, // summon second
    {1728.344f, 636.061f, 28.05f, 3.305}  // summon second
};

static const EventLocations SoldierMiddle[] =
{
    {1718.604f, 608.202f, 28.05f, 1.090}, // first
    {1679.347f, 648.365f, 28.05f, 0.368}, // second
};

class AllOrbsInGrid
{
    public:
        AllOrbsInGrid() {}
        bool operator() (GameObject* go)
        {
            if(go->GetEntry() == GAMEOBJECT_ORB_OF_THE_BLUE_DRAGONFLIGHT)
                return true;
            return false;
        }
};

bool GOHello_go_orb_of_the_blue_flight(Player *plr, GameObject* go)
{
    if (go->GetUInt32Value(GAMEOBJECT_FACTION) == 35)
    {
        ScriptedInstance* pInstance = ((ScriptedInstance*)go->GetInstanceData());

        //remove blue ring, a bit hacky but simple
        if(Creature* dummy = plr->FindCreatureInGrid(CREATURE_INVISIBLE_DUMMY, 20.0f, true))
            if (DynamicObject* Dyn = dummy->GetDynObject(SPELL_RING_OF_BLUE_FLAMES))
                Dyn->RemoveFromWorld();

        plr->CastSpell(plr, SPELL_POWER_OF_THE_BLUE_FLIGHT, true);
        go->SetUInt32Value(GAMEOBJECT_FACTION, 0); //not usable anymore
        go->Refresh();
    }

    return true;
}

//AI for Kalecgos
class boss_kalecgos_kj : public CreatureScript
{
public:
    boss_kalecgos_kj() : CreatureScript("boss_kalecgos_kj") {}
    
    class boss_kalecgos_kjAI : public CreatureAINew
    {
        public:
        boss_kalecgos_kjAI(Creature* creature) : CreatureAINew(creature)
        {
            pInstance = ((ScriptedInstance*)creature->GetInstanceData());
            if(!pInstance)
                me->ForcedDespawn();

            for (uint8 i = 0; i < 4; ++i)
                OrbDummies[i] = 0;
        }
            void onReset(bool onSpawn)
            {
                for (uint8 i = 0; i < 4; ++i)
                {
                    Orb[i] = 0;
                    for (uint8 i = 0; i < 4; ++i)
                        if(OrbDummies[i] != 0)
                        {
                            //despawn already spawned dummies
                            if(Creature* dummy = pInstance->instance->GetCreatureInMap(OrbDummies[i]))
                                dummy->DisappearAndDie();
                            OrbDummies[i] = 0;
                        }
                }

                EmpowerCount = 0;
                me->SetDisableGravity(true);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                me->setActive(true);
                Searched = false;
                me->SetVisibility(VISIBILITY_OFF);
                me->SetReactState(REACT_PASSIVE);
            }

            void FindOrbs()
            {
                CellPair pair(Trinity::ComputeCellPair(me->GetPositionX(), me->GetPositionY()));
                Cell cell(pair);
                cell.data.Part.reserved = ALL_DISTRICT;
                cell.SetNoCreate();
                std::list<GameObject*> orbList;
                AllOrbsInGrid check;
                Trinity::GameObjectListSearcher<AllOrbsInGrid> searcher(orbList, check);
                TypeContainerVisitor<Trinity::GameObjectListSearcher<AllOrbsInGrid>, GridTypeMapContainer> visitor(searcher);
                cell.Visit(pair, visitor, *(me->GetMap()));

                if (orbList.empty())
                    return;

                uint8 i = 0;
                for (std::list<GameObject*>::iterator itr = orbList.begin(); itr != orbList.end(); ++itr, ++i)
                {
                    Orb[i] = (*itr)->GetGUID();
                    if (Unit* dummy = (*itr)->SummonCreature(CREATURE_INVISIBLE_DUMMY,(*itr)->GetPositionX(),(*itr)->GetPositionY(),(*itr)->GetPositionZ(),0,TEMPSUMMON_MANUAL_DESPAWN,0))
                        OrbDummies[i] = dummy->GetGUID();
                }
            }

            void ResetOrbs()
            {
                for (uint8 i = 0; i < 4; ++i)
                {
                    if(Unit* dummy = pInstance->instance->GetCreatureInMap(OrbDummies[i]))
                        dummy->RemoveDynObject(SPELL_RING_OF_BLUE_FLAMES);

                    if(GameObject *orb = pInstance->instance->GetGameObjectInMap(Orb[i]))
                        orb->SetUInt32Value(GAMEOBJECT_FACTION, 0);
                }
            }

            void EmpowerOrb(uint8 i)
            {
                GameObject *orb = pInstance->instance->GetGameObjectInMap(Orb[i]);
                if (!orb)
                    return;
                if(Unit* dummy = pInstance->instance->GetCreatureInMap(OrbDummies[i]))
                {
                    dummy->CastSpell(dummy,SPELL_VISUAL_MOONFIRE,true);
                    dummy->CastSpell(dummy, SPELL_RING_OF_BLUE_FLAMES,true);
                }
                orb->SetUInt32Value(GAMEOBJECT_FACTION, 35);
                orb->setActive(true);
                orb->Refresh();
            }

            void EmpowerOrb(bool all)
            {
                if (all)
                {
                    for (uint8 i = 0; i < 4; ++i)
                        EmpowerOrb(i);
                }
                else
                {
                    uint8 random = rand()%3;
                    EmpowerOrb(random);
                }
        
                ++EmpowerCount;

                switch (EmpowerCount)
                {
                    case 1:
                        talk(SAY_KALEC_ORB_READY1);
                        break;
                    case 2:
                        talk(SAY_KALEC_ORB_READY2);
                        break;
                    case 3:
                        talk(SAY_KALEC_ORB_READY3);
                        break;
                    case 4:
                        talk(SAY_KALEC_ORB_READY4);
                        break;
                }
            }

            void update(uint32 const diff)
            {
                if (!Searched)
                {
                    FindOrbs();
                    Searched = true;
                }
            }
            
            ScriptedInstance* pInstance;
        private:
            uint64 Orb[4]; //orb gobjects
            uint64 OrbDummies[4]; //Used for some visual effects only
   
            uint8 EmpowerCount;

            bool Searched;
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new boss_kalecgos_kjAI(creature);
    }
};

//AI for Kil'Jaeden Event Controller
class mob_kiljaeden_controller : public CreatureScript
{
public:
    mob_kiljaeden_controller() : CreatureScript("mob_kiljaeden_controller") {}
    
    class mob_kiljaeden_controllerAI : public Creature_NoMovementAINew, private DialogueHelper
    {
        private:
            ScriptedInstance* pInstance;

            SummonList Summons;

            bool KiljaedenDeath;
            uint64 handDeceiver[3];
            uint64 riftGuid[2];
            uint64 riftTargets[2];
            uint64 soldiersGuid[20];
            uint64 m_EntropiusGuid;
            uint64 m_PortalGuid;
            uint32 m_currentAngleFirst;
            uint32 m_currentAngleSecond;
            
            uint32 combatCheckTimer;
        public:
            mob_kiljaeden_controllerAI(Creature* creature) : Creature_NoMovementAINew(creature), Summons(me), DialogueHelper(aOutroDialogue)
            {
                pInstance = ((ScriptedInstance*)creature->GetInstanceData());
                InitializeDialogueHelper(pInstance);
            }

            uint8 DeceiverDeathCount;

            void onReset(bool onSpawn)
            {
                if (pInstance)
                {
                    if (pInstance->GetData(DATA_KILJAEDEN_EVENT) == DONE)
                    {
                        doCast(me,SPELL_SUNWELL_IGNITION,true);
                        return;
                    } else {
                        doCast(me,SPELL_ANVEENA_ENERGY_DRAIN,true);
                        
                        pInstance->SetData(DATA_KILJAEDEN_EVENT, NOT_STARTED);
                    }
                } else {
                    me->ForcedDespawn();
                    return;
                }

                if (onSpawn)
                {
                    addEvent(EVENT_SAY, 45000, 75000);
                }
                else
                {
                    resetEvent(EVENT_SAY, 45000, 75000);
                }
                
                me->SetReactState(REACT_PASSIVE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);

                setPhase(PHASE_DECEIVERS);

                if (Creature *KalecKJ = pInstance->instance->GetCreatureInMap(pInstance->GetData64(DATA_KALECGOS_KJ)))
                    ((boss_kalecgos_kj::boss_kalecgos_kjAI*)KalecKJ->getAI())->ResetOrbs();

                DeceiverDeathCount = 0;
                KiljaedenDeath = false;

                Summons.DespawnAll();

                for(uint8 i = 0; i < 3; ++i)
                {
                    if (Creature *hand = me->SummonCreature(CREATURE_HAND_OF_THE_DECEIVER, DeceiverLocations[i][0], DeceiverLocations[i][1], FLOOR_Z, DeceiverLocations[i][2], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 30000))
                    {
                        hand->SetSummoner(me);
                        handDeceiver[i] = hand->GetGUID();
                    }
                }

                if (Creature *anveena = me->SummonCreature(CREATURE_ANVEENA,  me->GetPositionX(), me->GetPositionY(), 60, 0, TEMPSUMMON_DEAD_DESPAWN, 0))
                    anveena->SetSummoner(me);

                doCast(me,SPELL_DESTROY_DRAKES,true);

                m_currentAngleFirst = 0;
                m_currentAngleSecond = 0;

                combatCheckTimer = 10000;
            }

            void onSummon(Creature* summoned)
            {
                Summons.Summon(summoned);
                switch(summoned->GetEntry())
                {
                    case CREATURE_HAND_OF_THE_DECEIVER:
                        summoned->CastSpell(summoned, SPELL_SHADOW_CHANNELING, false);
                        break;
                    case CREATURE_ANVEENA:
                    {
                        summoned->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        summoned->SetDisableGravity(true);
                        summoned->SendMovementFlagUpdate();
                        summoned->CastSpell(summoned, SPELL_ANVEENA_PRISON, true);
                        summoned->SetReactState(REACT_PASSIVE);
                        break;
                    }
                    case CREATURE_KILJAEDEN:
                        summoned->CastSpell((Unit*)NULL, SPELL_REBIRTH, false);
                        summoned->getAI()->setPhase(PHASE_NORMAL);
                        break;
                    case NPC_RIFTWALKER:
                        summoned->CastSpell(summoned, SPELL_TELEPORT_VISUAL, true);
                        break;
                    case NPC_SOLDIER:
                        summoned->CastSpell(summoned, SPELL_TELEPORT_VISUAL, true);
                        summoned->SetWalk(false);
                        summoned->SetSpeed(MOVE_RUN, 1.0f);
                        break;
                    case CREATURE_PROPHET:
                        summoned->GetMotionMaster()->MovePoint(0, aOutroLocations[5].m_fX, aOutroLocations[5].m_fY, aOutroLocations[5].m_fZ);
                        // no break here
                    case CREATURE_LIADRIN:
                        summoned->CastSpell(summoned, SPELL_TELEPORT_VISUAL, true);
                        break;
                    case NPC_CORE_ENTROPIUS:
                        summoned->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        summoned->CastSpell(summoned, SPELL_ENTROPIUS_BODY, true);
                        summoned->SetDisableGravity(true);
                        summoned->SendMovementFlagUpdate();
                        m_EntropiusGuid = summoned->GetGUID();
                        break;
                    case NPC_BOSS_PORTAL:
                        m_PortalGuid = summoned->GetGUID();
                        break;
                }
            }

            void onSummonDespawn(Creature* unit)
            {
                Summons.Despawn(unit);
            }

            void startDialogueText()
            {
                StartNextDialogueText(POINT_KILJAEDEN_DIE);
            }

            void JustDidDialogueStep(int32 iEntry)
            {
                if (!pInstance)
                    return;

                switch (iEntry)
                {
                    case POINT_KILJAEDEN_DIE:
                        // While Kil'Jaeden die
                        if (Creature* Anveena = pInstance->instance->GetCreatureInMap(pInstance->GetData64(DATA_ANVEENA)))
                            Anveena->ForcedDespawn(); //this should already be done but let's do it again in case phase was gm rushed

                        if (Creature* pKalec = pInstance->instance->GetCreatureInMap(pInstance->GetData64(DATA_KALECGOS_KJ)))
                            ((boss_kalecgos_kj::boss_kalecgos_kjAI*)pKalec->getAI())->ResetOrbs();
                        break;
                    case POINT_TELEPORT_KALECGOS:
                        if (Creature* pKalec = pInstance->instance->GetCreatureInMap(pInstance->GetData64(DATA_KALECGOS_KJ)))
                        {
                            pKalec->SetVisibility(VISIBILITY_ON);
                            pKalec->CastSpell(pKalec, SPELL_KALEC_TELEPORT, true);
                            pKalec->SetDisableGravity(false);
                            pKalec->SendMovementFlagUpdate();
                        }

                        if (Creature* pKJ = pInstance->instance->GetCreatureInMap(pInstance->GetData64(DATA_KILJAEDEN)))
                            pKJ->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE); //allow loot
                        break;
                    case POINT_SUMMON_SHATTERED:
                        if (Creature *portal = me->SummonCreature(NPC_BOSS_PORTAL, aOutroLocations[0].m_fX, aOutroLocations[0].m_fY, aOutroLocations[0].m_fZ, aOutroLocations[0].m_fO, TEMPSUMMON_CORPSE_DESPAWN, 0))
                            portal->SetSummoner(me);

                        for (uint8 i = 1; i < 3; i++)
                        {
                            if (Creature * riftWalker = me->SummonCreature(NPC_RIFTWALKER, aOutroLocations[i].m_fX, aOutroLocations[i].m_fY, aOutroLocations[i].m_fZ, aOutroLocations[i].m_fO, TEMPSUMMON_CORPSE_DESPAWN, 0))
                            {
                                riftGuid[i - 1] = riftWalker->GetGUID();
                                riftWalker->SetSummoner(me);
                                if (i == 1)
                                    riftWalker->GetMotionMaster()->MovePoint(0, aOutroLocations[7].m_fX, aOutroLocations[7].m_fY, aOutroLocations[7].m_fZ);
                                else
                                    riftWalker->GetMotionMaster()->MovePoint(1, aOutroLocations[8].m_fX, aOutroLocations[8].m_fY, aOutroLocations[8].m_fZ);

                                if (Creature * riftTarget = me->SummonCreature(CREATURE_INVISIBLE_DUMMY, aOutroLocations[8+i].m_fX, aOutroLocations[8+i].m_fY, aOutroLocations[8+i].m_fZ, 0.0f, TEMPSUMMON_CORPSE_DESPAWN, 0))
                                {
                                    riftTargets[i - 1] = riftTarget->GetGUID();
                                    riftTarget->SetSummoner(me);
                                    riftTarget->SetSpeed(MOVE_RUN, 0.5f);
                                    riftTarget->SetDisableGravity(true);
                                    riftTarget->ToCreature();
                                }
                            }
                        }
                        break;
                    case POINT_SUMMON_PORTAL:
                        if (Creature* portal = pInstance->GetSingleCreatureFromStorage(NPC_BOSS_PORTAL))
                        {                     
                            portal->SetDisplayId(DISPLAYID_PORTAL_OPENING);
                            for (uint8 i = 0; i < 2; i++)
                            {
                                if (Creature* riftTarget = pInstance->instance->GetCreatureInMap(riftTargets[i]))
                                    riftTarget->ForcedDespawn(1000);
                            }
                        }
                        break;
                    case POINT_SUMMON_PORTAL_ENDOPENANIM:
                        if (Creature* portal = pInstance->GetSingleCreatureFromStorage(NPC_BOSS_PORTAL))
                            portal->SetStandState(UNIT_STAND_STATE_SIT); //this smoothly stop the explosion effect and just let the smokes continues
                        break;
                    case POINT_SUMMON_SOLDIERS_RIGHT:
                        for (uint8 i = 0; i < 2; i++)
                        {
                            if (Creature* rift = pInstance->instance->GetCreatureInMap(riftGuid[i]))
                            {
                                rift->RemoveAurasDueToSpell(SPELL_OPEN_PORTAL);
                                rift->InterruptNonMeleeSpells(false);
                            }
                        }

                        for (uint8 i = 0; i < 10; i++)
                        {
                            if (Creature *soldier = me->SummonCreature(NPC_SOLDIER, SoldierLocations[i].m_fX, SoldierLocations[i].m_fY, SoldierLocations[i].m_fZ, SoldierLocations[i].m_fO, TEMPSUMMON_CORPSE_DESPAWN, 0))
                            {
                                soldier->SetSummoner(me);
                                soldiersGuid[i] = soldier->GetGUID();
                                soldier->GetMotionMaster()->MovePoint(0, SoldierMiddle[0].m_fX, SoldierMiddle[0].m_fY, SoldierMiddle[0].m_fZ, false);
                                
                                if(i==0)
                                {
                                    soldier->GetMotionMaster()->MovePoint(0, SoldierMiddle[0].m_fX, SoldierMiddle[0].m_fY, SoldierMiddle[0].m_fZ, false);
                                } else {
                                    float sx, sy;
                                    float angle = m_currentAngleFirst * (2*M_PI) / 360;
                                    float rayon = 5.0f;
                                    sx = SoldierMiddle[0].m_fX + cos(angle) * rayon;
                                    sy = SoldierMiddle[0].m_fY + sin(angle) * rayon;
                                    soldier->GetMotionMaster()->MovePoint(0, sx, sy, SoldierMiddle[0].m_fZ, false);
                                    m_currentAngleFirst = m_currentAngleFirst + 36;
                                }
                            }
                        }
                        break;
                    case POINT_SUMMON_SOLDIERS_LEFT:
                        for (uint8 i = 10; i < 20; i++)
                        {
                            if (Creature *soldier = me->SummonCreature(NPC_SOLDIER, SoldierLocations[i].m_fX, SoldierLocations[i].m_fY, SoldierLocations[i].m_fZ, SoldierLocations[i].m_fO, TEMPSUMMON_CORPSE_DESPAWN, 0))
                            {
                                soldier->SetSummoner(me);
                                soldiersGuid[i] = soldier->GetGUID();

                                if(i == 10)
                                {
                                    soldier->GetMotionMaster()->MovePoint(0, SoldierMiddle[1].m_fX, SoldierMiddle[1].m_fY, SoldierMiddle[1].m_fZ, false);
                                } else {
                                    float sx, sy;
                                    float angle = m_currentAngleFirst * (2*M_PI) / 360;
                                    float rayon = 5.0f;
                                    sx = SoldierMiddle[1].m_fX + cos(angle) * rayon;
                                    sy = SoldierMiddle[1].m_fY + sin(angle) * rayon;
                                    soldier->GetMotionMaster()->MovePoint(0, sx, sy, SoldierMiddle[1].m_fZ, false);
                                    m_currentAngleFirst = m_currentAngleFirst + 36;
                                }
                            }
                        }
                        break;
                    case POINT_SUMMON_PROPHET:
                        if (Creature *prophet = me->SummonCreature(CREATURE_PROPHET, aOutroLocations[3].m_fX, aOutroLocations[3].m_fY, aOutroLocations[3].m_fZ, aOutroLocations[3].m_fO, TEMPSUMMON_CORPSE_DESPAWN, 0))
                            prophet->SetSummoner(me);

                        if (Creature * core = me->SummonCreature(NPC_CORE_ENTROPIUS, me->GetPositionX(), me->GetPositionY(), 85.0f, 0, TEMPSUMMON_CORPSE_DESPAWN, 0))
                            core->SetSummoner(me);
                        break;
                    case POINT_SUMMON_LIADRIN:
                        if (Creature *liadrin = me->SummonCreature(CREATURE_LIADRIN, aOutroLocations[4].m_fX, aOutroLocations[4].m_fY, aOutroLocations[4].m_fZ, aOutroLocations[4].m_fO, TEMPSUMMON_TIMED_DESPAWN, 4 * MINUTE * IN_MILLISECONDS))
                            liadrin->SetSummoner(me);
                        break;
                    case POINT_CALL_ENTROPIUS:
                        // Set point id = 1 for movement event
                        if (Creature* pEntropius = me->GetMap()->GetCreature(m_EntropiusGuid))
                        {
                            if (Creature* pVelen = pInstance->GetSingleCreatureFromStorage(CREATURE_PROPHET))
                                pVelen->CastSpell(pEntropius, SPELL_CALL_ENTROPIUS, false);

                            pEntropius->SetWalk(false);
                            pEntropius->GetMotionMaster()->MovePoint(1, me->GetPositionX(), me->GetPositionY(), 40.0f);
                        }
                        break;
                    case POINT_MOVE_LIADRIN:
                        if (Creature* pLiadrin = pInstance->GetSingleCreatureFromStorage(CREATURE_LIADRIN))
                            pLiadrin->GetMotionMaster()->MovePoint(0, aOutroLocations[6].m_fX, aOutroLocations[6].m_fY, aOutroLocations[6].m_fZ);
                        break;
                    case POINT_BLAZE:
                        if (Creature* pEntropius = me->GetMap()->GetCreature(m_EntropiusGuid))
                        {
                            pEntropius->CastSpell(pEntropius, SPELL_BLAZE_TO_LIGHT, true);
                            pEntropius->RemoveAurasDueToSpell(SPELL_ENTROPIUS_BODY);
                        }
                        break;
                    case POINT_IGNITE:
                        // When the purified Muru reaches the ground the sunwell ignites and Muru despawns
                        doCast(me, SPELL_SUNWELL_IGNITION);

                        if (Creature* pLiadrin = pInstance->GetSingleCreatureFromStorage(CREATURE_LIADRIN))
                            pLiadrin->SetStandState(UNIT_STAND_STATE_KNEEL);

                        if (Creature* pEntropius = me->GetMap()->GetCreature(m_EntropiusGuid))
                            pEntropius->ForcedDespawn();
                        break;
                    case POINT_EVENT_SOLDIER_EXIT:
                        for (uint8 i = 0; i < 20; i++)
                        {
                            if (Creature* soldier = pInstance->instance->GetCreatureInMap(soldiersGuid[i]))
                            {
                                soldier->SetWalk(false);
                                soldier->SetSpeed(MOVE_RUN, 1.0f);
                                soldier->GetMotionMaster()->MovePoint(2, SoldierLocations[i].m_fX, SoldierLocations[i].m_fY, SoldierLocations[i].m_fZ, false);
                            }
                        }
                        break;
                    case POINT_EVENT_VELEN_EXIT:
                        // Set point id = 1 for the despawn event
                        if (Creature* pVelen = pInstance->GetSingleCreatureFromStorage(CREATURE_PROPHET))
                            pVelen->GetMotionMaster()->MovePoint(1, aOutroLocations[3].m_fX, aOutroLocations[3].m_fY, aOutroLocations[3].m_fZ);

                        if (Creature* pKalec = pInstance->GetSingleCreatureFromStorage(CREATURE_KALECGOS))
                        {
                            pKalec->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                            pKalec->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        }
                        break;
                }
            }

            void summonedMovementInform(Creature* pSummoned, uint32 uiType, uint32 uiPointId)
            {
                if (uiType != POINT_MOTION_TYPE)
                    return;

                switch(uiPointId)
                {
                case 0:
                    switch(pSummoned->GetEntry())
                    {
                    case NPC_RIFTWALKER:
                        if(Creature* riftTarget = pSummoned->GetMap()->GetCreature(riftTargets[0]))
                        {
                            pSummoned->CastSpell(riftTarget, SPELL_OPEN_PORTAL, false);
                            riftTarget->GetMotionMaster()->MovePoint(0, aOutroLocations[9].m_fX, aOutroLocations[9].m_fY, aOutroLocations[9].m_fZ+13.0f);
                        }
                        break;
                    case NPC_SOLDIER:
                        if (pSummoned->GetGUID() == soldiersGuid[0] || pSummoned->GetGUID() == soldiersGuid[10])
                            pSummoned->SetStandState(UNIT_STAND_STATE_KNEEL);

                        if(Creature* portal = pInstance->GetSingleCreatureFromStorage(NPC_BOSS_PORTAL))
                        {
                            pSummoned->SetOrientation(pSummoned->GetAngle(aOutroLocations[5].m_fX, aOutroLocations[5].m_fY)); //velen talk position
                            pSummoned->SendMovementFlagUpdate();
                        }
                        break;
                    }
                    break;
                case 1:
                    switch(pSummoned->GetEntry())
                    {
                    case NPC_RIFTWALKER:
                        if(Creature* riftTarget = pSummoned->GetMap()->GetCreature(riftTargets[1]))
                        {
                            pSummoned->CastSpell(riftTarget, SPELL_OPEN_PORTAL, false);
                            riftTarget->GetMotionMaster()->MovePoint(0, aOutroLocations[10].m_fX, aOutroLocations[10].m_fY, aOutroLocations[10].m_fZ+13.0f);
                        }
                        break;
                    case NPC_CORE_ENTROPIUS:
                        if (Creature* pVelen = pInstance->GetSingleCreatureFromStorage(CREATURE_PROPHET))
                            pVelen->InterruptNonMeleeSpells(false);
                        break;
                    case CREATURE_PROPHET:
                        pSummoned->ForcedDespawn(1000);

                        // Note: portal should despawn only after all the soldiers have reached this point and "teleported" outside
                        if (Creature* pPortal = me->GetMap()->GetCreature(m_PortalGuid))
                           pPortal->ForcedDespawn(30000);

                        for (uint8 i = 0; i < 2; i++)
                        {
                            if (Creature* rift = pInstance->instance->GetCreatureInMap(riftGuid[i]))
                                rift->ForcedDespawn(1000);
                        }

                        me->ForcedDespawn(300000);
                        break;
                    }
                    break;
                case 2:
                    if (pSummoned->GetEntry() == NPC_SOLDIER)
                        pSummoned->ForcedDespawn(1000);
                    break;
                case 10:
                    if (pSummoned->GetEntry() == NPC_SOLDIER)
                    {
                        pSummoned->SetOrientation(SoldierMiddle[0].m_fO);
                        pSummoned->SendMovementFlagUpdate();
                    }
                    break;
                case 11:
                    if (pSummoned->GetEntry() == NPC_SOLDIER)
                    {
                        pSummoned->SetOrientation(SoldierMiddle[1].m_fO);
                        pSummoned->SendMovementFlagUpdate();
                    }
                    break;
                }
            }

            void onCombatStart(Unit* victim)
            {
                // DEBUG
                char w[50];
                snprintf(w, 50, "pull : %s",victim->GetName());
                me->Say(w,LANG_UNIVERSAL,0);
                //
                setZoneInCombat(true);
                for (uint8 i = 0; i < 3; i++)
                {
                    if (Creature *hand = pInstance->instance->GetCreatureInMap(handDeceiver[i]))
                    {
                        hand->getAI()->setZoneInCombat(true);
                        if (!hand->IsInCombat())
                            hand->getAI()->attackStart(victim);

                        hand->SetReactState(REACT_AGGRESSIVE);
                        hand->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        hand->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    }
                }
            }

            void update(uint32 const diff)
            {
                DialogueUpdate(diff);

                updateEvents(diff);
            
                while (executeEvent(diff, m_currEvent))
                {
                    switch (m_currEvent)
                    {
                        case EVENT_SAY:
                            if (pInstance->GetData(DATA_MURU_EVENT) != DONE && pInstance->GetData(DATA_KILJAEDEN_EVENT) == NOT_STARTED)
                                talk(SAY_KJ_OFFCOMBAT); //TODO : MISSING TEXT

                            scheduleEvent(EVENT_SAY, 45000, 75000);
                            break;
                    }
                }

                if(pInstance && pInstance->GetData(DATA_MURU_EVENT) != DONE || pInstance->GetData(DATA_KILJAEDEN_EVENT) == DONE)
                    return;

                if (pInstance->GetData(DATA_KILJAEDEN_EVENT) == NOT_STARTED)
                {
                    if(combatCheckTimer < diff)
                    {
                        Map::PlayerList const& players = pInstance->instance->GetPlayers();
                        for(Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                        //for(auto itr : players) //why does using auto call ~GroupReference() for every ref ?
                            if (Player* plr = itr->getSource())
                                if (me->GetDistance(plr) <= 50.0f && me->IsHostileTo(plr))
                                {
                                    pInstance->SetData(DATA_KILJAEDEN_EVENT,IN_PROGRESS);
                                    onCombatStart(plr);
                                    break;
                                }
                        combatCheckTimer = 2500;
                    } else combatCheckTimer -= diff;
                }

                if (pInstance->GetData(DATA_MURU_EVENT) != DONE)
                {
                    evade();
                    return;
                }

                if (DeceiverDeathCount > 2 && getPhase() == PHASE_DECEIVERS)
                {
                    me->RemoveAurasDueToSpell(SPELL_ANVEENA_ENERGY_DRAIN);
                    setPhase(PHASE_NORMAL);
                    if (Creature *kiljaeden = me->SummonCreature(CREATURE_KILJAEDEN, KJLocation[0], KJLocation[1], KJLocation[2], KJLocation[3], TEMPSUMMON_MANUAL_DESPAWN, 0))
                        kiljaeden->SetSummoner(me);
                }
            }
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new mob_kiljaeden_controllerAI(creature);
    }
};

//AI for Kil'jaeden
class boss_kiljaeden : public CreatureScript
{
public:
    boss_kiljaeden() : CreatureScript("boss_kiljaeden") {}

    class boss_kiljaedenAI : public Creature_NoMovementAINew, private DialogueHelper
    {
        private:
            ScriptedInstance* pInstance;

            SummonList Summons;
            BumpHelper bumpHelper;

            uint32 animSpawnTimer;
            bool firstDialogueStep;
            bool secondDialogueStep;
            bool thirdDialogueStep;
            
            uint8 phaseDarknessCount;
        public:
        boss_kiljaedenAI(Creature* creature) : Creature_NoMovementAINew(creature), Summons(me), DialogueHelper(firstDialogue), bumpHelper(2000)
        {
            pInstance = ((ScriptedInstance*)creature->GetInstanceData());
            InitializeDialogueHelper(pInstance);
            me->SetDisableGravity(true);
            me->Relocate(KJLocation[0], KJLocation[1], KJLocation[2]);
            me->SendMovementFlagUpdate();
            phaseDarknessCount = 0;
        }

            void onReset(bool onSpawn)
            {
                Summons.DespawnAll();
                if(!onSpawn)
                {   
                    if(me->GetSummoner() && me->GetSummoner()->ToCreature() && me->GetSummoner()->ToCreature()->getAI())
                        me->GetSummoner()->ToCreature()->getAI()->evade();

                    if(pInstance)
                        pInstance->SetData(DATA_KILJAEDEN_EVENT,NOT_STARTED);
                    me->ForcedDespawn();
                    return;
                }
                me->SetDisableGravity(true);
                setPhase(PHASE_DECEIVERS);

                addEvent(EVENT_KALEC_JOIN, 30000, 30000, EVENT_FLAG_DELAY_IF_CASTING, true, phaseMaskForPhase(2) | phaseMaskForPhase(3) | phaseMaskForPhase(4) | phaseMaskForPhase(5));
                // Phase 2
                addEvent(EVENT_SOUL_FLAY, 1000, 1000, EVENT_FLAG_DELAY_IF_CASTING, true, phaseMaskForPhase(2) | phaseMaskForPhase(3) | phaseMaskForPhase(4) | phaseMaskForPhase(5));
                addEvent(EVENT_LEGION_LIGHTNING, 10000, 20000, EVENT_FLAG_DELAY_IF_CASTING, true, phaseMaskForPhase(2) | phaseMaskForPhase(3) | phaseMaskForPhase(4) | phaseMaskForPhase(5));
                addEvent(EVENT_FIRE_BLOOM, 10000, 15000, EVENT_FLAG_DELAY_IF_CASTING, true, phaseMaskForPhase(2) | phaseMaskForPhase(3) | phaseMaskForPhase(4) | phaseMaskForPhase(5));
                addEvent(EVENT_SUMMON_SHILEDORB, 10000, 15000, EVENT_FLAG_DELAY_IF_CASTING, true, phaseMaskForPhase(2) | phaseMaskForPhase(3) | phaseMaskForPhase(4) | phaseMaskForPhase(5));
                // Phase 3
                addEvent(EVENT_SHADOW_SPIKE, 4000, 4000, EVENT_FLAG_DELAY_IF_CASTING, true, phaseMaskForPhase(3) | phaseMaskForPhase(4) | phaseMaskForPhase(5));
                addEvent(EVENT_FLAME_DART, 4500, 4500, EVENT_FLAG_DELAY_IF_CASTING, true, phaseMaskForPhase(3) | phaseMaskForPhase(4) | phaseMaskForPhase(5));
                addEvent(EVENT_DARKNESS, 75000, 75000, EVENT_FLAG_DELAY_IF_CASTING, true, phaseMaskForPhase(3) | phaseMaskForPhase(4) | phaseMaskForPhase(5));
                addEvent(EVENT_ORBS_EMPOWER, 35000, 35000, EVENT_FLAG_DELAY_IF_CASTING, true, phaseMaskForPhase(3) | phaseMaskForPhase(4) | phaseMaskForPhase(5));
                addEvent(EVENT_SINISTER_REFLECTION, 500, 500, EVENT_FLAG_DELAY_IF_CASTING, true, phaseMaskForPhase(3) | phaseMaskForPhase(4) | phaseMaskForPhase(5));
                // Phase 4
                addEvent(EVENT_ARMAGEDDON, 21000, 21000, EVENT_FLAG_DELAY_IF_CASTING, true, phaseMaskForPhase(4) | phaseMaskForPhase(5));

                firstDialogueStep = false;
                secondDialogueStep = false;
                thirdDialogueStep = false;

                me->SetFullTauntImmunity(true);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->addUnitState(UNIT_STAT_STUNNED);
                animSpawnTimer = 11000;
            }

            void onRemove()
            {
                if(pInstance) pInstance->SetData(DATA_KILJAEDEN_EVENT,NOT_STARTED);
            }

            void onEnterPhase(uint32 newPhase)
            {
                switch (newPhase)
                {
                    case PHASE_DARKNESS:
                        // Phase 2
                        scheduleEvent(EVENT_SOUL_FLAY, 1000);
                        scheduleEvent(EVENT_LEGION_LIGHTNING, 10000, 20000);
                        scheduleEvent(EVENT_FIRE_BLOOM, 10000, 15000);
                        scheduleEvent(EVENT_SUMMON_SHILEDORB, 10000, 15000);

                        talk(SAY_KJ_PHASE3);
                        break;
                    case PHASE_ARMAGEDDON:
                        // Phase 2
                        scheduleEvent(EVENT_SOUL_FLAY, 1000);
                        scheduleEvent(EVENT_LEGION_LIGHTNING, 10000, 20000);
                        scheduleEvent(EVENT_FIRE_BLOOM, 10000, 15000);
                        scheduleEvent(EVENT_SUMMON_SHILEDORB, 10000, 15000);
                        // Phase 3
                        scheduleEvent(EVENT_SHADOW_SPIKE, 4000);
                        scheduleEvent(EVENT_FLAME_DART, 50000,65000);
                        scheduleEvent(EVENT_DARKNESS, 75000);
                        scheduleEvent(EVENT_ORBS_EMPOWER, 35000);
                        scheduleEvent(EVENT_SINISTER_REFLECTION, 500);

                        talk(SAY_KJ_PHASE4);
                        doCast(NULL, SPELL_DESTROY_DRAKES, true);
                        enableEvent(EVENT_SINISTER_REFLECTION);
                        enableEvent(EVENT_SHADOW_SPIKE);
                        enableEvent(EVENT_ORBS_EMPOWER);
                        break;
                    case PHASE_SACRIFICE:
                        // Phase 2
                        scheduleEvent(EVENT_SOUL_FLAY, 1000);
                        scheduleEvent(EVENT_LEGION_LIGHTNING, 10000, 20000);
                        disableEvent(EVENT_FIRE_BLOOM);
                        scheduleEvent(EVENT_SUMMON_SHILEDORB, 10000, 15000);
                        // Phase 3
                        scheduleEvent(EVENT_SHADOW_SPIKE, 4000);
                        scheduleEvent(EVENT_FLAME_DART, 50000,65000);
                        scheduleEvent(EVENT_DARKNESS, 25000);
                        scheduleEvent(EVENT_ORBS_EMPOWER, 500);
                        scheduleEvent(EVENT_SINISTER_REFLECTION, 500);
                        // Phase 4
                        scheduleEvent(EVENT_ARMAGEDDON, 21000);

                        talk(SAY_KJ_PHASE5);
                        doCast(NULL, SPELL_DESTROY_DRAKES, true);
                        enableEvent(EVENT_SINISTER_REFLECTION);
                        enableEvent(EVENT_SHADOW_SPIKE);
                        enableEvent(EVENT_ORBS_EMPOWER);
                        break;
                }
                phaseDarknessCount = 0;
            }

            void onSummon(Creature* summoned)
            {
                Summons.Summon(summoned);
                switch (summoned->GetEntry())
                {
                    case CREATURE_ARMAGEDDON_TARGET:
                        summoned->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        summoned->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                        summoned->getAI()->setZoneInCombat(true);
                        break;
                }
            }

            void onSummonDespawn(Creature* unit)
            {
                Summons.Despawn(unit);
            }

            void onDeath(Unit* /*killer*/)
            {
                talk(SAY_KJ_DEATH);
                Summons.DespawnAll();
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);

                if (pInstance)
                    pInstance->SetData(DATA_KILJAEDEN_EVENT, DONE);

                if (Creature *controller = pInstance->instance->GetCreatureInMap(pInstance->GetData64(DATA_KILJAEDEN_CONTROLLER)))
                {
                    controller->setFaction(35);
                    controller->RemoveAllAuras();
                    controller->DeleteThreatList();
                    controller->CombatStop();
                    ((mob_kiljaeden_controller::mob_kiljaeden_controllerAI*)controller->getAI())->startDialogueText();
                }
            }

            void onKill(Unit* /*victim*/)
            {
                talk(SAY_KJ_SLAY);
            }

            void onCombatStart(Unit* /*who*/)
            {
                setZoneInCombat();
                talk(SAY_KJ_EMERGE);
            }

            void onSpellPrepare(SpellEntry const* spell, Unit* /*target*/)
            {
                if (spell->Id == 45657)
                    talk(SAY_KJ_DARKNESS);
            }

            void JustDidDialogueStep(int32 iEntry)
            {
                if (!pInstance)
                    return;

                switch (iEntry)
                {
                    case SAY_ANVEENA_4:
                        if (Creature* Anveena = pInstance->instance->GetCreatureInMap(pInstance->GetData64(DATA_ANVEENA)))
                        {
                            Anveena->RemoveAurasDueToSpell(SPELL_ANVEENA_PRISON);
                            Anveena->CastSpell((Unit*)NULL, SPELL_SACRIFICE_OF_ANVEENA, true);
                            Anveena->ForcedDespawn(3000);
                        }
                        break;
                    case SAY_KJ_PHASE5:
                        me->SetControlled(true, UNIT_STAT_STUNNED);
                        break;
                    case POINT_END_STUN:
                        me->SetControlled(false, UNIT_STAT_STUNNED);
                        setPhase(PHASE_SACRIFICE);
                        break;
                }
            }

            void bumpClosePlayers(const uint32 diff)
            {
                bumpHelper.Update(diff);
                Map::PlayerList const& players = pInstance->instance->GetPlayers();
                for(Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                {
                    Player* pl = itr->getSource();
                    if (pl->isAttackableByAOE() && pl->GetExactDistance2d(me->GetPositionX(),me->GetPositionY()) <= 13.0f) //~when a player set foot in the well
                        if (bumpHelper.AddCooldown(pl)) //return true if player wasn't knocked back < 2s ago
                            me->CastSpell(pl, SPELL_KNOCK_BACK,true);
                }
            }

            void update(const uint32 diff)
            {
                if (animSpawnTimer)
                {
                    me->SetTarget(0);
                    if (animSpawnTimer <= diff)
                    {
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                        me->clearUnitState(UNIT_STAT_STUNNED);
                        setZoneInCombat(true);
                        if (Unit *unit = selectUnit(SELECT_TARGET_NEAREST, 0, 100.0f, true))
                        {
                            attackStart(unit);
                            animSpawnTimer = 0;
                        }
                        else
                            evade();
                    }
                    else
                        animSpawnTimer -= diff;
                    return;
                }

                DialogueUpdate(diff);

                if (!updateVictim())
                    return;

                bumpClosePlayers(diff);
                updateEvents(diff);

                if (!firstDialogueStep)
                {
                    if (me->IsBelowHPPercent(80))
                    {
                        firstDialogueStep = true;
                        StartNextDialogueText(SAY_KALEC_1);
                    }
                }

                if (!secondDialogueStep)
                {
                    if (me->IsBelowHPPercent(50))
                    {
                        SetNewArray(secondDialogue);
                        secondDialogueStep = true;
                        StartNextDialogueText(SAY_KALEC_2);
                    }
                }

                if (!thirdDialogueStep)
                {
                    if (me->IsBelowHPPercent(25))
                    {
                        SetNewArray(thirdDialogue);
                        thirdDialogueStep = true;
                        StartNextDialogueText(SAY_KALEC_3);
                    }
                }

                if (me->HasUnitState(UNIT_STAT_CASTING) || me->HasUnitState(UNIT_STAT_STUNNED))
                    return;

                //Phase 3
                if (getPhase() <= PHASE_NORMAL)
                {
                    if (getPhase() == PHASE_NORMAL && me->IsBelowHPPercent(85))
                        setPhase(PHASE_DARKNESS);
                }

                //Phase 4
                if (getPhase() <= PHASE_DARKNESS)
                {
                    if (getPhase() == PHASE_DARKNESS && me->IsBelowHPPercent(55))
                        setPhase(PHASE_ARMAGEDDON);
                }

                while (executeEvent(diff, m_currEvent))
                {
                    switch (m_currEvent)
                    {
                        case EVENT_KALEC_JOIN:
                            disableEvent(EVENT_KALEC_JOIN);
                            if (Creature* kalec = pInstance->instance->GetCreatureInMap(pInstance->GetData64(DATA_KALECGOS_KJ)))
                            {
                                kalec->getAI()->talk(SAY_KALEC_JOIN);
                                kalec->SetVisibility(VISIBILITY_ON);
                            }
                            break;
                        case EVENT_SOUL_FLAY:
                            doCast(me->GetVictim(), SPELL_SOUL_FLAY);
                            scheduleEvent(EVENT_SOUL_FLAY, 4000, 5000);
                            break;
                        case EVENT_LEGION_LIGHTNING:
                            if (Unit *target = selectUnit(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                doCast(target, SPELL_LEGION_LIGHTNING);

                            scheduleEvent(EVENT_LEGION_LIGHTNING, 10000, 20000);
                            break;
                        case EVENT_FIRE_BLOOM:
                            doCast(NULL, SPELL_FIRE_BLOOM);
                            switch(getPhase())
                            {
                            case PHASE_NORMAL:
                                scheduleEvent(EVENT_FIRE_BLOOM, 22000);
                                break;
                            case PHASE_DARKNESS:
                            case PHASE_ARMAGEDDON:
                                scheduleEvent(EVENT_FIRE_BLOOM, 40000, 65000);
                                break;
                            case PHASE_SACRIFICE:
                                scheduleEvent(EVENT_FIRE_BLOOM, 90000, 105000);
                                break;
                            }
                            break;
                        case EVENT_SUMMON_SHILEDORB:
                            for (uint8 i = 1; i < getPhase(); ++i)
                            {
                                float sx, sy;
                                sx = ShieldOrbLocations[0][0] + sin(ShieldOrbLocations[i][0]);
                                sy = ShieldOrbLocations[0][1] + sin(ShieldOrbLocations[i][1]);
                                if (Creature* orb = me->SummonCreature(CREATURE_SHIELD_ORB, sx, sy, SHIELD_ORB_Z, 0, TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 45000))
                                {
                                    orb->SetSummoner(me);
                                    orb->getAI()->setZoneInCombat(true);
                                    orb->getAI()->setPhase(getPhase());
                                }
                            }

                            scheduleEvent(EVENT_SUMMON_SHILEDORB, 25000, 30000);
                            if (getPhase() == PHASE_SACRIFICE)
                                disableEvent(EVENT_SUMMON_SHILEDORB);

                            break;
                        case EVENT_SHADOW_SPIKE:
                            doCast((Unit*)NULL, SPELL_SHADOW_SPIKE);
                            disableEvent(EVENT_SHADOW_SPIKE);
                            break;
                        case EVENT_FLAME_DART:
                            doCast((Unit*)NULL, SPELL_FLAME_DART);
                            scheduleEvent(EVENT_FLAME_DART, 50000, 65000);
                            break;
                        case EVENT_DARKNESS:
                            phaseDarknessCount++;
                            talk(EMOTE_KJ_DARKNESS);
                            doCast((Unit*)NULL, SPELL_DARKNESS_OF_A_THOUSAND_SOULS);
                            scheduleEvent(EVENT_DARKNESS, (getPhase() == PHASE_SACRIFICE) ? 25000 : 45000);
                            scheduleEvent(EVENT_SUMMON_SHILEDORB, 9000, 10000);
                            scheduleEvent(EVENT_FLAME_DART, 5000, 20000);
                            if(getPhase() == PHASE_NORMAL)
                                scheduleEvent(EVENT_FIRE_BLOOM, 25000);
                            else if(phaseDarknessCount == 1)
                                scheduleEvent(EVENT_FIRE_BLOOM, 10000, 40000);
                            else
                                scheduleEvent(EVENT_FIRE_BLOOM, 15000, 20000);
                            break;
                        case EVENT_ORBS_EMPOWER:
                            if (getPhase() == PHASE_SACRIFICE)
                            {
                                if (Creature* kalec = pInstance->instance->GetCreatureInMap(pInstance->GetData64(DATA_KALECGOS_KJ)))
                                    ((boss_kalecgos_kj::boss_kalecgos_kjAI*)kalec->getAI())->EmpowerOrb(true);
                            }
                            else
                            {
                                if (Creature* kalec = pInstance->instance->GetCreatureInMap(pInstance->GetData64(DATA_KALECGOS_KJ)))
                                    ((boss_kalecgos_kj::boss_kalecgos_kjAI*)kalec->getAI())->EmpowerOrb(false);

                            }

                            disableEvent(EVENT_ORBS_EMPOWER);
                            break;
                        case EVENT_SINISTER_REFLECTION:
                            talk(SAY_KJ_REFLECTION);
                            doCast((Unit*)NULL, SPELL_SINISTER_REFLECTION, true);

                            disableEvent(EVENT_SINISTER_REFLECTION);
                            break;
                        case EVENT_ARMAGEDDON:
                        { 
                            //KJ doesn't cast it when channeling
                            if(me->IsNonMeleeSpellCasted(false)) //Already casting something, recheck 1 sec later
                            {
                                scheduleEvent(EVENT_ARMAGEDDON, 1000);
                                break;
                            }

                            doCast(me, SPELL_ARMAGEDDON_PERIODIC_SUMMON, true);

                            disableEvent(EVENT_ARMAGEDDON);
                            break;
                        }
                    }
                }
            }
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new boss_kiljaedenAI(creature);
    }
};

//AI for Hand of the Deceiver
class mob_hand_of_the_deceiver : public CreatureScript
{
public:
    mob_hand_of_the_deceiver() : CreatureScript("mob_hand_of_the_deceiver") {}

    class mob_hand_of_the_deceiverAI : public CreatureAINew
    {
        private:
            ScriptedInstance* pInstance;

            SummonList Summons;
        public:
            mob_hand_of_the_deceiverAI(Creature* creature) : CreatureAINew(creature), Summons(me)
            {
                pInstance = ((ScriptedInstance*)creature->GetInstanceData());
            }

            void onSummon(Creature* summoned)
            {
                Summons.Summon(summoned);
            }

            void onSummonDespawn(Creature* unit)
            {
                Summons.Despawn(unit);
            }

            void onReset(bool onSpawn)
            {
                if (onSpawn)
                {
                    addEvent(EVENT_SHADOWBOLT, 2000, 3000, EVENT_FLAG_DELAY_IF_CASTING);
                    addEvent(EVENT_FELFIRE, 5000, 10000, EVENT_FLAG_DELAY_IF_CASTING);
                }
                else
                {
                    resetEvent(EVENT_SHADOWBOLT, 2000, 3000);
                    resetEvent(EVENT_FELFIRE, 5000, 10000);

                    pInstance->SetData(DATA_KILJAEDEN_EVENT,NOT_STARTED);
                }

                me->SetReactState(REACT_PASSIVE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);

                Summons.DespawnAll();
            }

            void onCombatStart(Unit* victim)
            {
                if(pInstance)
                {
                    pInstance->SetData(DATA_KILJAEDEN_EVENT, IN_PROGRESS);
                    if (Creature* Control = pInstance->instance->GetCreatureInMap(pInstance->GetData64(DATA_KILJAEDEN_CONTROLLER)))
                        if (!Control->getAI()->aiInCombat())
                            Control->getAI()->attackStart(victim);
                }

                me->InterruptNonMeleeSpells(true);
            }

            void onDeath(Unit* /*killer*/)
            {
                me->RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
                Summons.DespawnAll();

                if(pInstance)
                    if (Creature* Control = pInstance->instance->GetCreatureInMap(pInstance->GetData64(DATA_KILJAEDEN_CONTROLLER)))
                        ((mob_kiljaeden_controller::mob_kiljaeden_controllerAI*)Control->getAI())->DeceiverDeathCount++;
            }

            void update(uint32 const diff)
            {
                if (!me->IsInCombat())
                    doCast(me, SPELL_SHADOW_CHANNELING);

                if (!updateVictim())
                    return;

                if (pInstance->GetData(DATA_MURU_EVENT) != DONE)
                {
                    evade();
                    return;
                }

                // Gain Shadow Infusion
                if (me->IsBetweenHPPercent(20, 25) && !me->HasAura(SPELL_SHADOW_INFUSION))
                    if (rand()%2)
                        doCast(me, SPELL_SHADOW_INFUSION, true);

                updateEvents(diff);
            
                while (executeEvent(diff, m_currEvent))
                {
                    switch (m_currEvent)
                    {
                        case EVENT_SHADOWBOLT:
                            doCast(me, SPELL_SHADOW_BOLT_VOLLEY);
                            scheduleEvent(EVENT_SHADOWBOLT, 300);
                            break;
                        case EVENT_FELFIRE:
                            doCast(me, SPELL_FELFIRE_PORTAL);
                            scheduleEvent(EVENT_FELFIRE, 20000);
                            break;
                    }
                }

                doMeleeAttackIfReady();
            }
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new mob_hand_of_the_deceiverAI(creature);
    }
};

//AI for Felfire Portal
class mob_felfire_portal : public CreatureScript
{
public:
    mob_felfire_portal() : CreatureScript("mob_felfire_portal") {}
    
    class mob_felfire_portalAI : public Creature_NoMovementAINew
    {
        private:
            ScriptedInstance* pInstance;

            SummonList Summons;

        public:
        mob_felfire_portalAI(Creature* creature) : Creature_NoMovementAINew(creature), Summons(me)
        {
            pInstance = ((ScriptedInstance*)creature->GetInstanceData());
        }

            void onReset(bool onSpawn)
            {
                if (onSpawn)
                    addEvent(EVENT_SPAWNFIEND, 1000, 1000);
                else
                    resetEvent(EVENT_SPAWNFIEND, 1000);

                Summons.DespawnAll();
            }

            void onSummon(Creature* summoned)
            {
                Summons.Summon(summoned);
            }

            void onSummonDespawn(Creature* unit)
            {
                Summons.Despawn(unit);
            }

            void updateEM(uint32 const diff)
            {
                if (pInstance && pInstance->GetData(DATA_KILJAEDEN_EVENT) == NOT_STARTED)
                    me->DisappearAndDie();
            }

            void update(uint32 const diff)
            {
                if (pInstance && pInstance->GetData(DATA_KILJAEDEN_EVENT) == NOT_STARTED)
                {
                    me->DisappearAndDie();
                    return;
                }

                updateEvents(diff);

                while (executeEvent(diff, m_currEvent))
                {
                    switch (m_currEvent)
                    {
                        case EVENT_SPAWNFIEND:
                            doCast(me, SPELL_SUMMON_FELFIRE_FIEND, false);
                            disableEvent(EVENT_SPAWNFIEND);
                            break;
                    }
                }
            }
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new mob_felfire_portalAI(creature);
    }
};

//AI for Felfire Fiend
class mob_volatile_felfire_fiend : public CreatureScript
{
public:
    mob_volatile_felfire_fiend() : CreatureScript("mob_volatile_felfire_fiend") {}

    class mob_volatile_felfire_fiendAI : public CreatureAINew
    {
        private:
            ScriptedInstance* pInstance;
        public:
            mob_volatile_felfire_fiendAI(Creature* creature) : CreatureAINew(creature)
            {
                pInstance = ((ScriptedInstance*)creature->GetInstanceData());
            }

            void onReset(bool onSpawn)
            {
                if (onSpawn)
                {
                    addEvent(EVENT_STUN, 2000, 2000);
                    addEvent(EVENT_EXPLODE, 2000, 2000);
                    addEvent(EVENT_DIE_F, 500, 500, EVENT_FLAG_NONE, false);
                }
                else
                {
                    resetEvent(EVENT_STUN, 2000);
                    resetEvent(EVENT_EXPLODE, 2000);
                    resetEvent(EVENT_DIE_F, 500);
                }

                me->addUnitState(UNIT_STAT_STUNNED);
            }

            void onDamageTaken(Unit* /*attacker*/, uint32& damage)
            {
                doCast(me, SPELL_FELFIRE_FISSION);
                enableEvent(EVENT_DIE_F);
            }

            void updateEM(uint32 const diff)
            {
                if (pInstance->GetData(DATA_KILJAEDEN_EVENT) == NOT_STARTED)
                    me->DisappearAndDie();
            }

            void update(uint32 const diff)
            {
                if (pInstance->GetData(DATA_KILJAEDEN_EVENT) == NOT_STARTED)
                {
                    me->DisappearAndDie();
                    return;
                }

                updateEvents(diff, 5);

                while (executeEvent(diff, m_currEvent))
                {
                    switch (m_currEvent)
                    {
                        case EVENT_STUN:
                            me->clearUnitState(UNIT_STAT_STUNNED);
                            if (Unit* summoner = me->GetSummoner())
                            {
                                if (summoner->GetEntry() == 25837)
                                {
                                    me->GetMotionMaster()->MovePath(25851, false);
                                    disableEvent(EVENT_STUN);
                                }
                                else
                                {
                                    setZoneInCombat(true);
                                    if (Unit *unit = selectUnit(SELECT_TARGET_NEAREST, 0, 100.0f, false))
                                    {
                                        attackStart(unit);
                                        doModifyThreat(unit, 10000000.0f);
                                        disableEvent(EVENT_STUN);
                                    }
                                }
                            }
                            scheduleEvent(EVENT_STUN, 500);
                            break;
                        case EVENT_EXPLODE:
                            if (me->IsWithinMeleeRange(me->GetVictim()))
                            {
                                // Explode if it's close enough to it's target
                                doCast(me, SPELL_FELFIRE_FISSION);
                                disableEvent(EVENT_EXPLODE);
                                enableEvent(EVENT_DIE_F);
                            }
                            scheduleEvent(EVENT_EXPLODE, 2000);
                            break;
                        case EVENT_DIE_F:
                            disableEvent(EVENT_DIE_F);
                            me->DisappearAndDie();
                            break;
                    }
                }

                if (!updateVictim())
                    return;

                updateEvents(diff, 2);

                doMeleeAttackIfReady();
            }
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new mob_volatile_felfire_fiendAI(creature);
    }
};

//AI for Armageddon target
class mob_armageddon : public CreatureScript
{
public:
    mob_armageddon() : CreatureScript("mob_armageddon") {}

    class mob_armageddonAI : public Creature_NoMovementAINew
    {
        private:
            ScriptedInstance* pInstance;
        public:
            mob_armageddonAI(Creature* creature) : Creature_NoMovementAINew(creature)
            {
                pInstance = ((ScriptedInstance*)creature->GetInstanceData());
            }

            void onReset(bool onSpawn)
            {
                if (onSpawn)
                {
                    addEvent(EVENT_VISUAL1, 0, 0, EVENT_FLAG_DELAY_IF_CASTING, true);
                    addEvent(EVENT_TRIGGER, 9000, 9000, EVENT_FLAG_DELAY_IF_CASTING, false);
                    addEvent(EVENT_DIE, 5000, 5000, EVENT_FLAG_DELAY_IF_CASTING, false);
                }
                else
                {
                    resetEvent(EVENT_VISUAL1, 0);
                    resetEvent(EVENT_TRIGGER, 9000);
                    resetEvent(EVENT_DIE, 5000);
                }
            }

            void updateEM(uint32 const diff)
            {
                if (pInstance && pInstance->GetData(DATA_KILJAEDEN_EVENT) == NOT_STARTED)
                    me->DisappearAndDie();
            }

            void update(uint32 const diff)
            {
                if (pInstance && pInstance->GetData(DATA_KILJAEDEN_EVENT) == NOT_STARTED)
                {
                    me->DisappearAndDie();
                    return;
                }

                updateEvents(diff);

                while (executeEvent(diff, m_currEvent))
                {
                    switch (m_currEvent)
                    {
                        case EVENT_VISUAL1:
                            doCast(me, SPELL_ARMAGEDDON_VISUAL, true);
                            disableEvent(EVENT_VISUAL1);
                            enableEvent(EVENT_TRIGGER);
                            break;
                        case EVENT_TRIGGER:
                            doCast(me, SPELL_ARMAGEDDON_TRIGGER, true);
                            disableEvent(EVENT_TRIGGER);
                            enableEvent(EVENT_DIE);
                            break;
                        case EVENT_DIE:
                            disableEvent(EVENT_DIE);
                            me->DisappearAndDie();
                            break;
                    }
                }
            }
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new mob_armageddonAI(creature);
    }
};

//AI for Shield Orbs
class mob_shield_orb : public CreatureScript
{
public:
    mob_shield_orb() : CreatureScript("mob_shield_orb") {}

    class mob_shield_orbAI : public CreatureAINew
    {
        private:
            bool PointReached;
            bool Clockwise;
            uint32 checkTimer;

            ScriptedInstance *pInstance;

            float x, y, r, c, mx, my;
        public:
            mob_shield_orbAI(Creature* creature) : CreatureAINew(creature)
            {
                pInstance = ((ScriptedInstance*)creature->GetInstanceData());
            }

            void onDeath(Unit* /*killer*/)
            {
                PointReached = true;
            }
            void onReset(bool onSpawn)
            {
                PointReached = true;

                r = 17;
                c = 0;
                checkTimer = 2000;
        
                mx = ShieldOrbLocations[0][0];
                my = ShieldOrbLocations[0][1];

                if (rand()%2)
                    Clockwise = true;
                else
                    Clockwise = false;

                if (onSpawn)
                {
                    addEvent(EVENT_SHADOWBOLT_S, 500, 1000, EVENT_FLAG_DELAY_IF_CASTING, true, phaseMaskForPhase(2) | phaseMaskForPhase(3) | phaseMaskForPhase(4));
                }
                else
                {
                    resetEvent(EVENT_SHADOWBOLT_S, 500, 1000);
                }

                me->SetFullTauntImmunity(true);
            }

            void attackStart(Unit* victim)
            {
                if (me->Attack(victim, false))
                {
                    if (!aiInCombat())
                    {
                        setAICombat(true);
                        onCombatStart(victim);
                    }
                }
            }

            void onMovementInform(uint32 type, uint32 /*id*/)
            {
                if(type != POINT_MOTION_TYPE)
                    return;

                PointReached = true;
            }

            void updateEM(uint32 const diff)
            {
                if (pInstance->GetData(DATA_KILJAEDEN_EVENT) == NOT_STARTED)
                    me->DisappearAndDie();
            }

            void update(uint32 const diff)
            {
                if (pInstance->GetData(DATA_KILJAEDEN_EVENT) == NOT_STARTED)
                {
                    me->DisappearAndDie();
                    return;
                }

                if (PointReached)
                {
                    if (Clockwise)
                    {
                        y = my - r * sin(c);
                        x = mx - r * cos(c);
                    }
                    else
                    {
                        y = my + r * sin(c);
                        x = mx + r * cos(c);
                    }

                    PointReached = false;
                    checkTimer = 1000;

                    me->GetMotionMaster()->MovePoint(1, x, y, SHIELD_ORB_Z);
            
                    c += M_PI/32;
                }
                else
                {
                    if (checkTimer <= diff)
                        PointReached = true;
                    else
                        checkTimer -= diff;
                }

                if (!updateVictim())
                    return;

                updateEvents(diff);

                while (executeEvent(diff, m_currEvent))
                {
                    switch (m_currEvent)
                    {
                        case EVENT_SHADOWBOLT_S:
                            if (Unit* random = selectUnit(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                doCast(random, SPELL_SHADOW_BOLT, false);
                            scheduleEvent(EVENT_SHADOWBOLT_S, 500, 1000);
                            break;
                    }
                }
            }
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new mob_shield_orbAI(creature);
    }
};

//AI for Sinister Reflection
class mob_sinster_reflection : public CreatureScript
{
public:
    mob_sinster_reflection() : CreatureScript("mob_sinster_reflection") {}

    class mob_sinster_reflectionAI : public CreatureAINew
    {
        private:
            ScriptedInstance *pInstance;

            uint8 Class;
            uint32 Timer[3];
            bool canAttack;
            Creature *m_kj;
        public:
            mob_sinster_reflectionAI(Creature* creature) : CreatureAINew(creature)
            {
                pInstance = ((ScriptedInstance*)creature->GetInstanceData());
                m_kj = NULL;
            }

            void onReset(bool onSpawn)
            {
                if(pInstance)
                {
                    if (Creature* kj = pInstance->instance->GetCreatureInMap(pInstance->GetData64(DATA_KILJAEDEN)))
                        m_kj = kj;
                }

                Timer[0] = urand(1000, 4000);
                Timer[1] = urand(1000, 4000);
                Timer[2] = urand(1000, 4000);
                Class = 0;
                canAttack = false;
                if (onSpawn)
                    addEvent(EVENT_STUN, 4000, 4000);
                else
                    resetEvent(EVENT_STUN, 4000);

                me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_HASTE_SPELLS, true);
                me->addUnitState(UNIT_STAT_STUNNED);

                me->CastSpell(me, 45893, true);
            }

            void updateEM(uint32 const diff)
            {
                if (pInstance && pInstance->GetData(DATA_KILJAEDEN_EVENT) == NOT_STARTED)
                    me->DisappearAndDie();
            }

            void update(uint32 const diff)
            {
                if (pInstance && pInstance->GetData(DATA_KILJAEDEN_EVENT) == NOT_STARTED)
                {
                    me->DisappearAndDie();
                    return;
                }

                if (!updateVictim())
                    return;

                updateEvents(diff);

                while (executeEvent(diff, m_currEvent))
                {
                    switch (m_currEvent)
                    {
                        case EVENT_STUN:
                            canAttack = true;
                            me->clearUnitState(UNIT_STAT_STUNNED);
                            if (Unit* summoner = me->GetSummoner())
                                attackStart(summoner);
                            disableEvent(EVENT_STUN);
                            break;
                    }
                }

                if (!canAttack)
                    return;

                if (Class == 0)
                {
                    Class = me->GetVictim()->getClass();
                    switch(Class)
                    {
                        case CLASS_DRUID:
                        case CLASS_HUNTER:
                        case CLASS_MAGE:
                        case CLASS_WARLOCK:
                        case CLASS_PALADIN:
                        case CLASS_PRIEST:
                            break;
                        case CLASS_WARRIOR:
                        case CLASS_SHAMAN:
                        case CLASS_ROGUE:
                            me->SetCanDualWield(true);
                            break;
                    }
                }

                switch (Class)
                {
                    case CLASS_DRUID:
                        if (Timer[0] <= diff)
                        {
                            if (Unit* random = selectUnit(SELECT_TARGET_RANDOM, 0, 30.0f, true))
                                doCast(random, SPELL_SR_MOONFIRE, false);
                            Timer[0] = urand(5000, 7000);
                        }

                        doMeleeAttackIfReady();
                        break;
                    case CLASS_HUNTER:
                        if (Timer[0] <= diff)
                        {
                            if (Unit *random = selectUnit(SELECT_TARGET_RANDOM, 0, 30.0f, true))
                                doCast(random, SPELL_SR_MULTI_SHOT, false);
                            Timer[0] = urand(8000, 10000);
                        }

                        if (Timer[1] <= diff)
                        {
                            if (Unit *random = selectUnit(SELECT_TARGET_RANDOM, 0, 30.0f, true))
                                doCast(random, SPELL_SR_SHOOT, false);
                            Timer[1] = urand(4000, 6000);
                        }

                        if (me->IsWithinMeleeRange(me->GetVictim(), 6))
                        {
                            if (Timer[2] <= diff)
                            {
                                doCast(me->GetVictim(), SPELL_SR_WING_CLIP, false);
                                Timer[2] = urand(6000, 8000);
                            }
                
                            doMeleeAttackIfReady();
                        }
                        break;
                    case CLASS_MAGE:
                        if (Timer[0] <= diff)
                        {
                            doCast(me->GetVictim(), SPELL_SR_FIREBALL, false);
                            Timer[0] = (2000, 4000);
                        }

                        doMeleeAttackIfReady();
                        break;
                    case CLASS_WARLOCK:
                        if (Timer[0] <= diff)
                        {
                            doCast(me->GetVictim(), SPELL_SR_SHADOW_BOLT, false);
                            Timer[0] = urand(3000, 5000);
                        }

                        if (Timer[1] <= diff)
                        {
                            if (Unit *random = selectUnit(SELECT_TARGET_RANDOM, 0, 30.0f, true))
                                doCast(random, SPELL_SR_CURSE_OF_AGONY, false);
                            Timer[1] = urand(15000, 17000);
                        }

                        doMeleeAttackIfReady();
                        break;
                    case CLASS_WARRIOR:
                        if (Timer[0] <= diff)
                        {
                            doCast(me->GetVictim(), SPELL_SR_WHIRLWIND, false);
                            Timer[0] = urand(9000, 11000);
                        }

                        doMeleeAttackIfReady();
                        break;
                    case CLASS_PALADIN:
                        if (Timer[0] <= diff)
                        {
                            doCast(me->GetVictim(), SPELL_SR_HAMMER_OF_JUSTICE, false);
                            Timer[0] = urand(9000, 12000);
                        }

                        if (Timer[1] <= diff)
                        {
                            doCast(me->GetVictim(), SPELL_SR_HOLY_SHOCK, false);
                            Timer[1] = urand(2000, 4000);
                        }

                        doMeleeAttackIfReady();
                        break;
                    case CLASS_PRIEST:
                        if (Timer[0] <= diff)
                        {
                            doCast(me->GetVictim(), SPELL_SR_HOLY_SMITE, false);
                            Timer[0] = urand(4000, 6000);
                        }

                        if (Timer[1] <= diff)
                        {
                            doCast(urand(0, 1) ? me : m_kj,  SPELL_SR_RENEW, false);
                            Timer[1] = urand(6000, 8000);
                        }

                        doMeleeAttackIfReady();
                        break;
                    case CLASS_SHAMAN:
                        if (Timer[0] <= diff)
                        {
                            doCast(me->GetVictim(), SPELL_SR_EARTH_SHOCK, false);
                            Timer[0] = urand(4000, 6000);
                        }

                        doMeleeAttackIfReady();
                        break;
                    case CLASS_ROGUE:
                        if (Timer[0] <= diff)
                        {
                            doCast(me->GetVictim(), SPELL_SR_HEMORRHAGE, false);
                            Timer[0] = urand(4000, 6000);
                        }

                        doMeleeAttackIfReady();
                        break;
                }
                for (uint8 i = 0; i < 3; ++i)
                    Timer[i] -= diff;
            }
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new mob_sinster_reflectionAI(creature);
    }
};

bool GossipHello_kalecgos_kj(Player *player, Creature *_Creature)
{
    player->SEND_GOSSIP_MENU(GOSSIP_KALEC_END, _Creature->GetGUID());

    return true;
}

void AddSC_boss_kiljaeden_new()
{
    Script* newscript;

    newscript = new Script;
    newscript->Name = "go_orb_of_the_blue_flight";
    newscript->pGOHello = &GOHello_go_orb_of_the_blue_flight;
    newscript->RegisterSelf();

    CreatureScript* kalec_kj_script;
    kalec_kj_script = new boss_kalecgos_kj();
    kalec_kj_script->pGossipHello = &GossipHello_kalecgos_kj;
    sScriptMgr.addScript(kalec_kj_script);

    sScriptMgr.addScript(new boss_kiljaeden());
    sScriptMgr.addScript(new mob_kiljaeden_controller());
    sScriptMgr.addScript(new mob_hand_of_the_deceiver());
    sScriptMgr.addScript(new mob_felfire_portal());
    sScriptMgr.addScript(new mob_volatile_felfire_fiend());
    sScriptMgr.addScript(new mob_armageddon());
    sScriptMgr.addScript(new mob_shield_orb());
    sScriptMgr.addScript(new mob_sinster_reflection());
}
