/*
 * Copyright (C) 2008-2010 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#define QUEST_BALANCEOFLIGHT   7622

#define CREATURE_ENRIS         14494
#define CREATURE_ARCHER        14489
#define CREATURE_PEASANTT1     14485
#define CREATURE_PEASANTT2     14484
#define CREATURE_FOOTSOLDIER   14486

#define SPELL_ARROW            22411
#define SPELL_DEATHSDOOR       23127 //on arrows
#define SPELL_SEETHINGPLAGUE   23072 //at inc

#define WAVE_TIMER             50000
#define WAVE_COUNT             5

#define PEASANTT_ARRIVAL_SAY1  -1590000  //The Scourge are upon us ! Run ! Run for your lives !
#define PEASANTT_ARRIVAL_SAY2  -1590001  //Please help us ! The Prince has gone mad !
#define PEASANTT_ARRIVAL_SAY3  -1590002  //Seek sanctuary in Hearthglen ! It is our only hope !
#define ERIS_END_SAY1          -1590003  //We are saved ! The peasants have escaped the Scourge ! / Nous sommes sauvés ! Les paysans ont échappés au Fléau !
#define ERIS_END_SAY2          -1590004  //I have failed once more...
#define PEASANT_END_SAY1       -1590005  //Thank you, kind stranger. May your heroism never be forgotten.
#define PEASANT_END_SAY2       -1590006  //The power of the light is truly great and merciful.
#define PEASANT_END_SAY3       -1590007  //Stranger, find the fallen Prince Menethil and end his reign of terror.

enum EventsErisHavenfire
{
    EVENT_START,
    EVENT_NEWWAVE,
    EVENT_PEASANT,
    EVENT_FOOTSOLDIERS,
    EVENT_END,
    EVENT_SUCCESS,
    EVENT_FAILURE,
};

struct Locations
{
    float x, y, z, o;
};

static Locations ArchersPositions[] ={
    { 3347.6, -3071.3, 177.9, 1.6},
    { 3357, -3063.8, 172.4, 1.8},
    { 3371.9, -3069.4, 175.3, 2.1},
    { 3334.6, -3053.3, 174.05, 0.9},
    { 3369.6, -3024.6, 171.77, 3.3}
};

static Locations FootSoldiersPositions[] ={
    { 3344.922852, -3047.982178, 164.136826, 0},
    { 3349.338379, -3058.127197, 168.775848, 0},
    { 3367.953857, -3043.435547, 164.905167, 0}
};

static Locations PeasantsPositions[] ={
    { 3358.496826, -3055.713379, 165.544006, 0},
    { 3360.218018, -3055.480469, 165.379150, 0},
    { 3362.431641, -3056.194580, 165.278198, 0},
    { 3364.003662, -3055.811279, 165.295990, 0},
    { 3365.399902, -3055.755859, 165.378128, 0},
    { 3367.164063, -3055.320801, 165.716324, 0},
    { 3367.295654, -3057.399170, 165.714828, 0},
    { 3364.462402, -3058.087646, 165.585693, 0},
    { 3360.964111, -3058.046631, 165.993729, 0},
    { 3361.451416, -3053.601074, 165.298401, 0},
    { 3365.114990, -3060.439453, 166.099304, 0},
    { 3361.784668, -3060.594971, 166.723602, 0},
    { 3368.362061, -3057.696533, 166.116379, 0},
    { 3330.148193, -2986.815674, 160.813156, 4.5} //End event position
};

static Locations PeasantsArrivalPositions[] = {
    { 3329.598877, -2975.505127, 160.366943, 0},
    { 3333.926025, -2973.781982, 160.776489, 0},
    { 3324.293457, -2976.472900, 160.308640, 0},
    { 3331.634766, -2974.401367, 160.427734, 0},
    { 3326.627930, -2975.803223, 160.591400, 0}
};

struct TRINITY_DLL_DECL npc_eris_havenfireAI : public Scripted_NoMovementAI
{

    npc_eris_havenfireAI(Creature * c) : Scripted_NoMovementAI(c), Summons(me)
    {
        me->SetReactState(REACT_PASSIVE);
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
    }

    bool EventStarted;
    bool EventDone;
    uint32 Wave_Timer;
    uint16 DiedCount;
    uint16 CurrentWave;
    uint16 EndPhase;
    SummonList Summons; //for the bad guys only
    Player* MyLittlePriest;
    uint32 FootSoldiers_Timer;
    uint32 Peasants_Timer;
    uint32 End_Timer;
    bool SoldiersFirstSpawn;
    bool Peasants_Spawned;
    Creature* EndPeasant;

    void Reset()
    {
        Summons.DespawnAll();
        EventDone = false;
        EventStarted = false;
        DiedCount = 0;
        CurrentWave = 1;
        EndPhase = 1;
        Wave_Timer = WAVE_TIMER;
        MyLittlePriest = NULL;
        FootSoldiers_Timer = 12000;
        Peasants_Timer = 7000;
        End_Timer = 12000;
        SoldiersFirstSpawn = true;
        Peasants_Spawned = false;
        EndPeasant = NULL;
    }
    
    void Aggro(Unit* who) {}

    void UpdateAI(const uint32 diff)
    {
        if (EventStarted && MyLittlePriest) {
            if (!EventDone) {
                Wave_Timer -= diff;
                if (CurrentWave <= WAVE_COUNT) {
                    if (CurrentWave >= 2 && FootSoldiers_Timer < diff) {
                        PlayEvent(EVENT_FOOTSOLDIERS);
                    }
                    else FootSoldiers_Timer -= diff;

                    if (!Peasants_Spawned && Peasants_Timer < diff) {
                        PlayEvent(EVENT_PEASANT);
                    }
                    else Peasants_Timer -= diff;

                    if (Wave_Timer > -diff) //WTF.
                    {
                        PlayEvent(EVENT_NEWWAVE);
                    }
                }

                if (DiedCount >= 15) {
                    PlayEvent(EVENT_FAILURE);
                }
            }
            else {
                if (End_Timer < diff)
                    PlayEvent(EVENT_END);
                else End_Timer -= diff;
            }
        }
    }

    void PlayEvent(int event)
    {
        switch (event) {
        case EVENT_START:
            for (int i = 0; i < 5; i++) {
                Creature* Archer = me->SummonCreature(CREATURE_ARCHER, ArchersPositions[i].x, ArchersPositions[i].y, ArchersPositions[i].z, ArchersPositions[i].o, TEMPSUMMON_TIMED_DESPAWN, 300000);
                if (Archer)
                    Summons.Summon(Archer);
            }
            break;
        case EVENT_NEWWAVE:
            CurrentWave++;
            if (CurrentWave > WAVE_COUNT)
                EventDone = true;
            else {
                SoldiersFirstSpawn = true;
                FootSoldiers_Timer = 10000;
                Peasants_Timer = 5000;
                Peasants_Spawned = false;
                Wave_Timer = WAVE_TIMER;
            }
            break;
        case EVENT_PEASANT:
            Creature* Peasant;
            for (int i = 0; i < 13; i++) {
                uint32 summon = rand() % 8 ? CREATURE_PEASANTT2 : CREATURE_PEASANTT1;
                Peasant = me->SummonCreature(summon, PeasantsPositions[i].x, PeasantsPositions[i].y, PeasantsPositions[i].z, 0, TEMPSUMMON_TIMED_DESPAWN, 67000);
                if (Peasant) {
                    Peasant->SetOwnerGUID(me->GetGUID());
                    if (summon == CREATURE_PEASANTT1) Peasant->CastSpell(Peasant, SPELL_SEETHINGPLAGUE, true);
                    for (std::list<uint64>::iterator i = Summons.begin(); i != Summons.end(); ++i) {
                        Creature* BadGuy = me->GetCreature((*me), (*i));
                        if (BadGuy)
                            BadGuy->AddThreat(Peasant, 0);
                    }
                }
            }
            if (Peasant) {
                switch (CurrentWave) {
                case 1:
                    Peasant->Say(PEASANTT_ARRIVAL_SAY1, 0, NULL);
                    break;
                case 2:
                    Peasant->Say(PEASANTT_ARRIVAL_SAY2, 0, NULL);
                    break;
                case 3:
                    Peasant->Say(PEASANTT_ARRIVAL_SAY3, 0, NULL);
                    break;
                }
            }
            Peasants_Spawned = true;
            break;
        case EVENT_FOOTSOLDIERS:
            uint8 HowMuch;
            if (!SoldiersFirstSpawn) {
                HowMuch = rand() % 3 + 1;
            }
            else {
                HowMuch = 3;
                SoldiersFirstSpawn = false;
            }

            for (int i = 0; i < HowMuch; i++) {
                Creature* BadGuy = m_creature->SummonCreature(CREATURE_FOOTSOLDIER, FootSoldiersPositions[i].x, FootSoldiersPositions[i].y, FootSoldiersPositions[i].z, 0, TEMPSUMMON_TIMED_DESPAWN, 60000);
                if (BadGuy) {
                    Summons.Summon(BadGuy);
                    BadGuy->AddThreat(MyLittlePriest, 0);
                    BadGuy->AI()->AttackStart(MyLittlePriest);
                }
            }
            FootSoldiers_Timer = HowMuch * 5000;
            break;
        case EVENT_END:
            if (EndPeasant) {
                switch (EndPhase) {
                case 1:
                    PlayEvent(EVENT_SUCCESS);
                    break;
                case 2:
                    EndPeasant->SetInFront(MyLittlePriest);
                    EndPeasant->StopMoving();
                    EndPeasant->Say(PEASANT_END_SAY1, 0, NULL);
                    EndPeasant->HandleEmoteCommand(EMOTE_ONESHOT_TALK);
                    break;
                case 3:
                    EndPeasant->SetInFront(MyLittlePriest);
                    EndPeasant->StopMoving();
                    EndPeasant->Say(PEASANT_END_SAY2, 0, NULL);
                    EndPeasant->HandleEmoteCommand(EMOTE_ONESHOT_TALK);
                    break;
                case 4:
                    EndPeasant->SetInFront(MyLittlePriest);
                    EndPeasant->StopMoving();
                    EndPeasant->Say(PEASANT_END_SAY3, 0, NULL);
                    EndPeasant->HandleEmoteCommand(EMOTE_ONESHOT_TALK);
                    break;
                case 5:
                    EndPeasant->DisappearAndDie();
                    Reset();
                    break;
                }

                EndPhase++;
                End_Timer = 6000;
            }
            else {
                EndPeasant = me->SummonCreature(CREATURE_PEASANTT2, PeasantsPositions[13].x, PeasantsPositions[13].y, PeasantsPositions[13].z, PeasantsPositions[13].o, TEMPSUMMON_DEAD_DESPAWN, 0);
                if (EndPeasant)
                    EndPeasant->GetMotionMaster()->Clear();
            }
            break;
        case EVENT_SUCCESS:
            MyLittlePriest->AreaExploredOrEventHappens(QUEST_BALANCEOFLIGHT); //quest complete
            me->Say(ERIS_END_SAY1, 0, NULL);
            Summons.DespawnAll();
            EventDone = true;
            break;
        case EVENT_FAILURE:
            MyLittlePriest->FailQuest(QUEST_BALANCEOFLIGHT);
            me->Say(ERIS_END_SAY2, 0, NULL);
            Reset();
            me->DisappearAndDie();
            break;
        }
    }
};

bool QuestAccept_npc_eris_havenfire(Player* pPlayer, Creature* pCreature, Quest const* quest)
{
    if (((npc_eris_havenfireAI*) pCreature->AI())->EventStarted == false) {
        ((npc_eris_havenfireAI*) pCreature->AI())->Reset();
        ((npc_eris_havenfireAI*) pCreature->AI())->EventStarted = true;
        ((npc_eris_havenfireAI*) pCreature->AI())->MyLittlePriest = pPlayer;
        ((npc_eris_havenfireAI*) pCreature->AI())->PlayEvent(EVENT_START);
    }

    return true;
}

struct TRINITY_DLL_DECL npc_escaping_peasantAI : public Scripted_NoMovementAI
{

    npc_escaping_peasantAI(Creature * c) : Scripted_NoMovementAI(c)
    {
        me->SetReactState(REACT_PASSIVE);
        m_creature->AddUnitMovementFlag(MOVEMENTFLAG_WALK_MODE);
        Eris = NULL;
        uint8 pos = rand() % 6;
        me->GetMotionMaster()->MovePoint(0, PeasantsArrivalPositions[pos].x, PeasantsArrivalPositions[pos].y, PeasantsArrivalPositions[pos].z);
    }
    Creature* Eris;

    void JustDied(Unit* /* who */)
    {
        if (Eris)
            ((npc_eris_havenfireAI*) Eris->AI())->DiedCount++;
    }
    
    void Aggro(Unit* who) {}

    void UpdateAI(const uint32 diff)
    {
        if (Eris) {
            if (!((npc_eris_havenfireAI*) Eris->AI())->EventStarted) //disappear at failure
                me->DisappearAndDie();
        }
        else Eris = me->GetCreature(*me, me->GetOwnerGUID());
    }
};

struct TRINITY_DLL_DECL npc_scourge_archerAI : public Scripted_NoMovementAI
{

    npc_scourge_archerAI(Creature * c) : Scripted_NoMovementAI(c)
    {
        me->SetReactState(REACT_PASSIVE);
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        me->SetByteValue(UNIT_FIELD_BYTES_2, 0, SHEATH_STATE_RANGED);
    }
    int Arrow_Timer;
    uint64 targetGUID;

    void Reset()
    {
        Arrow_Timer = 2700;
        targetGUID = NULL;
    }

    void Aggro(Unit* who) {}
    
    void UpdateAI(const uint32 diff)
    {
        if (Arrow_Timer < diff) {
            Creature* target = me->GetCreature(*me, targetGUID);
            if (target && target->isAlive() && me->GetDistance(target) < 60 && target->GetEntry() != CREATURE_ENRIS) {
                AttackStart(target);
                me->AttackStop(); //visual debug purpose
                Arrow_Timer = 2700;
                DoCast(target, SPELL_ARROW, false);
                if (rand() % 10 == 0) target->CastSpell(target, SPELL_DEATHSDOOR, true);
            }
            else {
                target = (Creature*) SelectUnit(SELECT_TARGET_RANDOM, 0, 60, false);
                if (target)
                    targetGUID = target->GetGUID();
            }
        }
        else Arrow_Timer -= diff;
    }
};

CreatureAI* GetAI_npc_eris_havenfire(Creature *_Creature)
{
    return new npc_eris_havenfireAI(_Creature);
}

CreatureAI* GetAI_npc_escaping_peasant(Creature *_Creature)
{
    return new npc_escaping_peasantAI(_Creature);
}

CreatureAI* GetAI_npc_scourge_archer(Creature *_Creature)
{
    return new npc_scourge_archerAI(_Creature);
}

void AddSC_the_balance_of_light_and_shadow()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name = "npc_eris_havenfire";
    newscript->GetAI = &GetAI_npc_eris_havenfire;
    newscript->pQuestAccept = &QuestAccept_npc_eris_havenfire;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_escaping_peasant";
    newscript->GetAI = &GetAI_npc_escaping_peasant;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_scourge_archer";
    newscript->GetAI = &GetAI_npc_scourge_archer;
    newscript->RegisterSelf();
}