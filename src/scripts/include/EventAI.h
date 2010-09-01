/* Copyright (C) 2006 - 2008 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 * This program is free software licensed under GPL version 2
 * Please see the included DOCS/LICENSE.TXT for more information */

#ifndef SC_EVENTAI_H
#define SC_EVENTAI_H

#define MAX_ACTIONS     3

enum Event_Types
{
    EVENT_T_TIMER                   = 0,    //InitialMin, InitialMax, RepeatMin, RepeatMax
    EVENT_T_TIMER_OOC               = 1,    //InitialMin, InitialMax, RepeatMin, RepeatMax
    EVENT_T_HP                      = 2,    //HPMax%, HPMin%, RepeatMin, RepeatMax
    EVENT_T_MANA                    = 3,    //ManaMax%,ManaMin% RepeatMin, RepeatMax
    EVENT_T_AGGRO                   = 4,    //NONE
    EVENT_T_KILL                    = 5,    //RepeatMin, RepeatMax
    EVENT_T_DEATH                   = 6,    //NONE
    EVENT_T_EVADE                   = 7,    //NONE
    EVENT_T_SPELLHIT                = 8,    //SpellID, School, RepeatMin, RepeatMax
    EVENT_T_RANGE                   = 9,    //MinDist, MaxDist, RepeatMin, RepeatMax
    EVENT_T_OOC_LOS                 = 10,   //NoHostile, NoFriendly, RepeatMin, RepeatMax
    EVENT_T_SPAWNED                 = 11,   //NONE
    EVENT_T_TARGET_HP               = 12,   //HPMax%, HPMin%, RepeatMin, RepeatMax
    EVENT_T_TARGET_CASTING          = 13,   //RepeatMin, RepeatMax
    EVENT_T_FRIENDLY_HP             = 14,   //HPDeficit, Radius, RepeatMin, RepeatMax
    EVENT_T_FRIENDLY_IS_CC          = 15,   //DispelType, Radius, RepeatMin, RepeatMax
    EVENT_T_FRIENDLY_MISSING_BUFF   = 16,   //SpellId, Radius, RepeatMin, RepeatMax
    EVENT_T_SUMMONED_UNIT           = 17,   //CreatureId, RepeatMin, RepeatMax
    EVENT_T_TARGET_MANA             = 18,   //ManaMax%, ManaMin%, RepeatMin, RepeatMax
    EVENT_T_QUEST_ACCEPT            = 19,   //QuestID
    EVENT_T_QUEST_COMPLETE          = 20,   //
    EVENT_T_REACHED_HOME            = 21,   //NONE

    EVENT_T_END,
};

enum Action_Types
{
    ACTION_T_NONE                       = 0,    //No action
    ACTION_T_TEXT                       = 1,    //-TextId1, optionally -TextId2, optionally -TextId3(if -TextId2 exist). If more than just -TextId1 is defined, randomize. Negative values.
    ACTION_T_SET_FACTION                = 2,    //FactionId (or 0 for default)
    ACTION_T_MORPH_TO_ENTRY_OR_MODEL    = 3,    //Creature_template entry(param1) OR ModelId (param2) (or 0 for both to demorph)
    ACTION_T_SOUND                      = 4,    //SoundId
    ACTION_T_EMOTE                      = 5,    //EmoteId
    ACTION_T_RANDOM_SAY                 = 6,    //UNUSED
    ACTION_T_RANDOM_YELL                = 7,    //UNUSED
    ACTION_T_RANDOM_TEXTEMOTE           = 8,    //UNUSED
    ACTION_T_RANDOM_SOUND               = 9,    //SoundId1, SoundId2, SoundId3 (-1 in any field means no output if randomed that field)
    ACTION_T_RANDOM_EMOTE               = 10,   //EmoteId1, EmoteId2, EmoteId3 (-1 in any field means no output if randomed that field)
    ACTION_T_CAST                       = 11,   //SpellId, Target, CastFlags
    ACTION_T_SUMMON                     = 12,   //CreatureID, Target, Duration in ms
    ACTION_T_THREAT_SINGLE_PCT          = 13,   //Threat%, Target
    ACTION_T_THREAT_ALL_PCT             = 14,   //Threat%
    ACTION_T_QUEST_EVENT                = 15,   //QuestID, Target
    ACTION_T_CASTCREATUREGO             = 16,   //QuestID, SpellId, Target
    ACTION_T_SET_UNIT_FIELD             = 17,   //Field_Number, Value, Target
    ACTION_T_SET_UNIT_FLAG              = 18,   //Flags (may be more than one field OR'd together), Target
    ACTION_T_REMOVE_UNIT_FLAG           = 19,   //Flags (may be more than one field OR'd together), Target
    ACTION_T_AUTO_ATTACK                = 20,   //AllowAttackState (0 = stop attack, anything else means continue attacking)
    ACTION_T_COMBAT_MOVEMENT            = 21,   //AllowCombatMovement (0 = stop combat based movement, anything else continue attacking)
    ACTION_T_SET_PHASE                  = 22,   //Phase
    ACTION_T_INC_PHASE                  = 23,   //Value (may be negative to decrement phase, should not be 0)
    ACTION_T_EVADE                      = 24,   //No Params
    ACTION_T_FLEE                       = 25,   //No Params
    ACTION_T_QUEST_EVENT_ALL            = 26,   //QuestID
    ACTION_T_CASTCREATUREGO_ALL         = 27,   //QuestId, SpellId
    ACTION_T_REMOVEAURASFROMSPELL       = 28,   //Target, Spellid
    ACTION_T_RANGED_MOVEMENT            = 29,   //Distance, Angle
    ACTION_T_RANDOM_PHASE               = 30,   //PhaseId1, PhaseId2, PhaseId3
    ACTION_T_RANDOM_PHASE_RANGE         = 31,   //PhaseMin, PhaseMax
    ACTION_T_SUMMON_ID                  = 32,   //CreatureId, Target, SpawnId
    ACTION_T_KILLED_MONSTER             = 33,   //CreatureId, Target
    ACTION_T_SET_INST_DATA              = 34,   //Field, Data
    ACTION_T_SET_INST_DATA64            = 35,   //Field, Target
    ACTION_T_UPDATE_TEMPLATE            = 36,   //Entry, Team
    ACTION_T_DIE                        = 37,   //No Params
    ACTION_T_ZONE_COMBAT_PULSE          = 38,   //No Params

    ACTION_T_SET_ACTIVE                 = 101,  //Apply
    ACTION_T_SET_AGGRESSIVE             = 102,  //Apply
    ACTION_T_ATTACK_START_PULSE         = 103,  //Distance
    ACTION_T_SUMMON_GO                  = 104,  //GameObjectID, DespawnTime in ms

    ACTION_T_END,
};

enum Target
{
    //Self (m_creature)
    TARGET_T_SELF = 0,                      //Self cast

    //Hostile targets (if pet then returns pet owner)
    TARGET_T_HOSTILE,                       //Our current target (ie: highest aggro)
    TARGET_T_HOSTILE_SECOND_AGGRO,          //Second highest aggro (generaly used for cleaves and some special attacks)
    TARGET_T_HOSTILE_LAST_AGGRO,            //Dead last on aggro (no idea what this could be used for)
    TARGET_T_HOSTILE_RANDOM,                //Just any random target on our threat list
    TARGET_T_HOSTILE_RANDOM_NOT_TOP,        //Any random target except top threat

    //Invoker targets (if pet then returns pet owner)
    TARGET_T_ACTION_INVOKER,                //Unit who caused this Event to occur (only works for EVENT_T_AGGRO, EVENT_T_KILL, EVENT_T_DEATH, EVENT_T_SPELLHIT, EVENT_T_OOC_LOS, EVENT_T_FRIENDLY_HP, EVENT_T_FRIENDLY_IS_CC, EVENT_T_FRIENDLY_MISSING_BUFF)

    //Hostile targets (including pets)
    TARGET_T_HOSTILE_WPET,                  //Current target (can be a pet)
    TARGET_T_HOSTILE_WPET_SECOND_AGGRO,     //Second highest aggro (generaly used for cleaves and some special attacks)
    TARGET_T_HOSTILE_WPET_LAST_AGGRO,       //Dead last on aggro (no idea what this could be used for)
    TARGET_T_HOSTILE_WPET_RANDOM,           //Just any random target on our threat list
    TARGET_T_HOSTILE_WPET_RANDOM_NOT_TOP,   //Any random target except top threat

    TARGET_T_ACTION_INVOKER_WPET,

    TARGET_T_END
};

enum CastFlags
{
    CAST_INTURRUPT_PREVIOUS     = 0x01,     //Interrupt any spell casting
    CAST_TRIGGERED              = 0x02,     //Triggered (this makes spell cost zero mana and have no cast time)
    CAST_FORCE_CAST             = 0x04,     //Forces cast even if creature is out of mana or out of range
    CAST_NO_MELEE_IF_OOM        = 0x08,     //Prevents creature from entering melee if out of mana or out of range
    CAST_FORCE_TARGET_SELF      = 0x10,     //Forces the target to cast this spell on itself
    CAST_AURA_NOT_PRESENT       = 0x20,     //Only casts the spell if the target does not have an aura from the spell
};

enum EventFlags
{
    EFLAG_REPEATABLE            = 0x01,     //Event repeats
    EFLAG_NORMAL                = 0x02,     //Event only occurs in Normal instance difficulty
    EFLAG_HEROIC                = 0x04,     //Event only occurs in Heroic instance difficulty
    EFLAG_RESERVED_3            = 0x08,
    EFLAG_RESERVED_4            = 0x10,
    EFLAG_RESERVED_5            = 0x20,
    EFLAG_RESERVED_6            = 0x40,
    EFLAG_DEBUG_ONLY            = 0x80,     //Event only occurs in debug build of SD2 only
};

struct EventAI_Event
{
    uint32 event_id;

    uint32 creature_id;

    uint16 event_type;
    uint32 event_inverse_phase_mask;
    uint8 event_chance;
    uint8 event_flags;
    union
    {
        uint32 event_param1;
        int32 event_param1_s;
    };
    union
    {
        uint32 event_param2;
        int32 event_param2_s;
    };
    union
    {
        uint32 event_param3;
        int32 event_param3_s;
    };
    union
    {
        uint32 event_param4;
        int32 event_param4_s;
    };

    struct _action
    {
        uint16 type;
        union
        {
            uint32 param1;
            int32 param1_s;
        };
        union
        {
            uint32 param2;
            int32 param2_s;
        };
        union
        {
            uint32 param3;
            int32 param3_s;
        };
    }action[MAX_ACTIONS];
};

//Event_Map
extern UNORDERED_MAP<uint32, std::vector<EventAI_Event> > EventAI_Event_Map;

struct EventAI_Summon
{
    uint32 id;

    float position_x;
    float position_y;
    float position_z;
    float orientation;
    uint32 SpawnTimeSecs;
};

//EventSummon_Map
extern UNORDERED_MAP<uint32, EventAI_Summon> EventAI_Summon_Map;

//EventAI Error handling
extern uint32 EAI_ErrorLevel;
/*

struct EventAI_CreatureError
{
    bool ListEmpty;
    bool NoInstance;
};

//Error prevention list
extern UNORDERED_MAP<uint32, EventAI_CreatureError> EventAI_CreatureErrorPreventionList;

//Defines
#define EVENTAI_EMPTY_EVENTLIST         "TSCR: Eventlist for Creature %i is empty but creature is using Mob_EventAI. Preventing EventAI on this creature."
*/
#endif

