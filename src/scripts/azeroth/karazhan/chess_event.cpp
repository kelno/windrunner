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
SDName: Chess_Event
SD%Complete: xx
SDComment: Based on Hectolight script.
SDCategory: Karazhan
  
TODO:
 - Implement in instance_karazhan dead chess_piece count per side to allow Medivh to cheat when is loosing.(Or different terms was used as a treshold for cheat ?)
 - Set proper position on left or right site from chess board for killed units.
 - Half turn if no trigger in arc (facing external side of the board)
 - Figure out why after a reset, some move triggers aren't reset
 - Sounds
EndScriptData */

#include "precompiled.h"
#include "def_karazhan.h"

#define A_FACTION               1690
#define H_FACTION               1689

#define GOSSIP_POSSES           "Contrôler la pièce." // Actually each piece has "Control <<piece name>>" as gossip option

#define NPC_ATTACK_RADIUS       6
#define AGGRO_RANGE             6
#define TRIGGER_ID              22519
#define SEARCH_RANGE            5
#define DUST_COVERED_CHEST      185119

float playerTeleportPosition[4] = { -11107.241211, -1842.897461, 229.625198, 5.385472 };

enum eScriptTexts {
    SCRIPTTEXT_AT_EVENT_START   =  -1650000,
    SCRIPTTEXT_LOSE_KNIGHT_P    =  -1650001,
    SCRIPTTEXT_LOSE_KNIGHT_M    =  -1650002,
    SCRIPTTEXT_LOSE_PAWN_P_1    =  -1650003,
    SCRIPTTEXT_LOSE_PAWN_P_2    =  -1650004,
    SCRIPTTEXT_LOSE_PAWN_P_3    =  -1650005,
    SCRIPTTEXT_LOSE_PAWN_M_1    =  -1650006,
    SCRIPTTEXT_LOSE_PAWN_M_2    =  -1650007,
    SCRIPTTEXT_LOSE_PAWN_M_3    =  -1650008,
    SCRIPTTEXT_LOSE_QUEEN_P     =  -1650009,
    SCRIPTTEXT_LOSE_QUEEN_M     =  -1650010,
    SCRIPTTEXT_LOSE_BISHOP_P    =  -1650011,
    SCRIPTTEXT_LOSE_BISHOP_M    =  -1650012,
    SCRIPTTEXT_LOSE_ROOK_P      =  -1650013,
    SCRIPTTEXT_LOSE_ROOK_M      =  -1650014,
    SCRIPTTEXT_PLAYER_CHECK     =  -1650015,
    SCRIPTTEXT_MEDIVH_CHECK     =  -1650016,
    SCRIPTTEXT_PLAYER_WIN       =  -1650017,
    SCRIPTTEXT_MEDIVH_WIN       =  -1650018,
    SCRIPTTEXT_MEDIVH_CHEAT_1   =  -1650019,
    SCRIPTTEXT_MEDIVH_CHEAT_2   =  -1650020,
    SCRIPTTEXT_MEDIVH_CHEAT_3   =  -1650021
};

enum eNPCs { 
    NPC_MEDIVH   = 16816,
    NPC_PAWN_H   = 17469,
    NPC_PAWN_A   = 17211,
    NPC_KNIGHT_H = 21748,
    NPC_KNIGHT_A = 21664,
    NPC_QUEEN_H  = 21750,
    NPC_QUEEN_A  = 21683,
    NPC_BISHOP_H = 21747,
    NPC_BISHOP_A = 21682,
    NPC_ROOK_H   = 21726,
    NPC_ROOK_A   = 21160,
    NPC_KING_H   = 21752,
    NPC_KING_A   = 21684
};

enum eSpells {
    BISHOP_HEAL_H  = 37456,
    BISHOP_HEAL_A  = 37455,
    SPELL_MOVE_1   = 37146, // 8y
    SPELL_MOVE_2   = 30012, // Unlimited
    SPELL_MOVE_3   = 37144, // 15y
    SPELL_MOVE_4   = 37148, // 20y
    SPELL_MOVE_5   = 37151, // 8y
    SPELL_MOVE_6   = 37152, // 8y
    SPELL_MOVE_7   = 37153, // 8y

    SPELL_CHANGE_FACING         = 30284,
    SPELL_MOVE_MARKER           = 32261,
    SPELL_POSSESS_CHESSPIECE    = 30019,
    SPELL_RECENTLY_INGAME       = 30529,
    SPELL_HAND_OF_MEDIVH        = 39339, // 1st cheat: AOE spell burn cell under enemy chesspieces.
    SPELL_FURY_OF_MEDIVH        = 39383  // 2nd cheat: Berserk own chesspieces.
    // 3rd cheat: set own creatures to max health
};

struct move_triggerAI : public Scripted_NoMovementAI
{
    move_triggerAI(Creature *c) : Scripted_NoMovementAI(c)
    {
        pInstance = ((ScriptedInstance*)me->GetInstanceData());
        needReset = false;
    }

    ScriptedInstance* pInstance;
    
    Unit* onMarker;
    bool EndMarker;
    bool needReset;

    void Reset()
    {
        onMarker    = NULL;
        EndMarker   = false;
        needReset   = false;
    }
    
    void Aggro(Unit* pWho) {}
    
    bool IsMovementAllowed(Unit* piece)
    {
        if (!piece)
            return false;
            
        if (pInstance && pInstance->GetData(DATA_CHESS_EVENT) != IN_PROGRESS)
            return true;

        switch (piece->GetEntry()) {
        case NPC_PAWN_H:
        case NPC_PAWN_A:
        case NPC_BISHOP_H:
        case NPC_BISHOP_A:
        case NPC_KING_H:
        case NPC_KING_A:
        case NPC_ROOK_H:
        case NPC_ROOK_A:
        {
            float exactDist = piece->GetExactDistance2d(me->GetPositionX(), me->GetPositionY());
            if (!piece->HasInArc(M_PI, me) || exactDist > 8.2f)
                return false;
            break;
        }
        case NPC_KNIGHT_H:
        case NPC_KNIGHT_A:
        {
            float exactDist = piece->GetExactDistance2d(me->GetPositionX(), me->GetPositionY());
            if (exactDist > 16.0f || !piece->HasInArc(M_PI/2 - 0.2f, me))
                return false;
            break;
        }
        case NPC_QUEEN_H:
        case NPC_QUEEN_A:
            if ((piece->HasInArc(M_PI/6, me) && piece->GetExactDistance2d(me->GetPositionX(), me->GetPositionY()) > 19.0f) || !piece->HasInArc(M_PI/2 - 0.2f, me))
                return false;
            break;
        }
        
        return true;
    }
    
    void SpellHit(Unit* pCaster, const SpellEntry* pSpell)
    {
        if (pSpell->Id == SPELL_CHANGE_FACING) {
            if (pCaster->GetExactDistance2d(me->GetPositionX(), me->GetPositionY()) > 6.0f)
                return;

            pCaster->CombatStop();
            pCaster->SetInFront(me);
            pCaster->SendMovementFlagUpdate();
        }

        if (pSpell->Id == SPELL_MOVE_1 || pSpell->Id == SPELL_MOVE_2 || pSpell->Id == SPELL_MOVE_3 || pSpell->Id == SPELL_MOVE_4 ||
           pSpell->Id == SPELL_MOVE_5 || pSpell->Id == SPELL_MOVE_6 || pSpell->Id == SPELL_MOVE_7)
        {
            if (onMarker != NULL || EndMarker) {
                /*if (onMarker) {
                    char msg[100];
                    sprintf(msg, "onMarker: %s (%u)%s", onMarker->GetName(), onMarker->ToCreature()->GetDBTableGUIDLow(), EndMarker ? " (EndMarker)" : "");
                    DoSay(msg, LANG_UNIVERSAL, false);
                }*/
                if (onMarker->GetExactDistance2d(me->GetPositionX(), me->GetPositionY()) >= 4.0f && !EndMarker)
                    Reset();
                else
                    return;
            }
                
            if (!IsMovementAllowed(pCaster))
                return;

            EndMarker = true;
            onMarker = pCaster;
            
            pCaster->CombatStop();
            
            DoCast(me, SPELL_MOVE_MARKER);
            
            if (pInstance && pInstance->GetData(DATA_CHESS_EVENT) != IN_PROGRESS) // Future status when done!
                return;

            pCaster->GetMotionMaster()->Clear();
            pCaster->GetMotionMaster()->MovePoint(0, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ());
            //pCaster->ToCreature()->SetHomePosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), pCaster->GetAngle(me));
        }
    }
    
    void UpdateAI(const uint32 diff)
    {
        me->SetReactState(REACT_PASSIVE);

        if(pInstance->GetData(DATA_CHESS_EVENT) != IN_PROGRESS) {
            if (needReset)
                Reset();
            return;
        }
            
        if(onMarker) {
            /*if (onMarker->isAlive() && onMarker->GetExactDistance2d(me->GetPositionX(), me->GetPositionY()) < 8.0f && onMarker->ToCreature()->AI()->GetData(0) == 1)
                return;
            else
                Reset();*/
            needReset = true;
            if (onMarker->isDead())
                Reset();
            /*if (onMarker->ToCreature()->AI()->GetData(1) != me->GetDBTableGUIDLow() && onMarker->GetExactDistance2d(me->GetPositionX(), me->GetPositionY()) >= 7.0f)
                Reset();*/
        }
    }
};

struct npc_chesspieceAI : public Scripted_NoMovementAI
{
    npc_chesspieceAI(Creature *c) : Scripted_NoMovementAI(c)
    {
        pInstance = ((ScriptedInstance*)me->GetInstanceData());
        startingOrientation = c->GetOrientation();
        potentialTarget = NULL;
        LinkCellTimer = 2000; // To ensure move triggers are in place
    }
    
    ScriptedInstance* pInstance;
    
    Unit* npc_medivh;
    
    bool ReturnToHome;
    bool InGame;
    bool CanMove;
    bool LockInMovement;
    
    uint32 Heal_Timer;
    uint32 NextMove_Timer;
    uint32 LinkCellTimer;
    uint32 CheckForceMoveTimer;
    uint32 CheckNearEnemiesTimer;
    
    uint64 MedivhGUID;
    
    float startingOrientation;

    Creature *start_marker, *end_marker, *potentialTarget;

    std::list<Unit *> PossibleMoveUnits;
    
    Creature* SelectTarget()
    {
        Creature* target = NULL;
        
        std::list<WorldObject*> targets;
        float dist = 10.0f;
        Trinity::AllWorldObjectsInRange u_check(me, dist);
        Trinity::WorldObjectListSearcher<Trinity::AllWorldObjectsInRange> searcher(targets, u_check);
        me->VisitNearbyObject(dist, searcher);

        for (std::list<WorldObject*>::const_iterator itr = targets.begin(); itr != targets.end() && !target; itr++) {
            // Exclude not unit objects
            if ((*itr)->GetTypeId() != TYPEID_UNIT)
                continue;

            // Exclude movement triggers
            if ((*itr)->ToCreature()->GetEntry() == 22519)
                continue;

            if (!me->IsHostileTo((*itr)->ToCreature()))
                continue;

            if (me->HasInArc(M_PI/8, (*itr)) && (*itr)->GetExactDistance2d(me->GetPositionX(), me->GetPositionY()) < 6.0f)
                target = (*itr)->ToCreature();
        }
        
        return target;
    }
    
    void EnterEvadeMode()
    {
        // Just stay in place
        me->RemoveAllAuras();
        Reset();
    }
    
    void Aggro(Unit* pWho)
    {
        MedivhGUID = pInstance->GetData64(DATA_IMAGE_OF_MEDIVH);
        npc_medivh = Unit::GetUnit(*me, MedivhGUID);
        
        if (npc_medivh) {
            switch (pInstance->GetData(CHESS_EVENT_TEAM)) {
            case ALLIANCE:
                if (me->GetEntry() == NPC_KING_H)
                    DoScriptText(SCRIPTTEXT_MEDIVH_CHECK, npc_medivh);
                else if (me->GetEntry() == NPC_KING_A)
                    DoScriptText(SCRIPTTEXT_PLAYER_CHECK, npc_medivh);
            break;
            case HORDE:
                if (me->GetEntry() == NPC_KING_A)
                    DoScriptText(SCRIPTTEXT_MEDIVH_CHECK, npc_medivh);
                else if (me->GetEntry() == NPC_KING_H)
                    DoScriptText(SCRIPTTEXT_PLAYER_CHECK, npc_medivh);
            break;
            }
        }
    }
    
    void Reset()
    {
        LockInMovement = false;
        //ReturnToHome = true;
        Heal_Timer = 7000;
        InGame = true;
        CanMove = false;
        NextMove_Timer = 4000 + rand()%1000; // wait 4.5s for first moves
        CheckForceMoveTimer = 1000;
        CheckNearEnemiesTimer = 2000 + rand()%1000;
        me->setActive(true);
        
        start_marker = NULL;
        end_marker = NULL;
        
        me->ApplySpellImmune(0, IMMUNITY_ID, 39331, true);
    }
    
    uint32 GetData(uint32 id)
    {
        if (id == 0)
            return LockInMovement ? 1 : 0;
        if (id == 1 && start_marker)
            return start_marker->GetDBTableGUIDLow();
            
        return 0;
    }
    
    void MovementInform(uint32 MovementType, uint32 Data)
    {
        if (MovementType != POINT_MOTION_TYPE)
            return;

        if (start_marker)
            ((move_triggerAI*)start_marker->AI())->Reset();

        /*char msg[100];
        sprintf(msg, "Starting orientation: %f", startingOrientation);
        DoSay(msg, LANG_UNIVERSAL, false);*/
        
        me->SetOrientation(startingOrientation);
        
        if (end_marker)
            me->Relocate(end_marker->GetPositionX(), end_marker->GetPositionY(), end_marker->GetPositionZ(), startingOrientation);
            
        start_marker = end_marker;
        end_marker = NULL;

        me->SendMovementFlagUpdate();
            
        LockInMovement = false;
    }
    
    void ResetTriggers()
    {
        CellPair p(Trinity::ComputeCellPair(me->GetPositionX(), me->GetPositionY()));
        Cell cell(p);
        cell.data.Part.reserved = ALL_DISTRICT;
        cell.SetNoCreate();

        std::list<Unit*> pList;
        
        float range = 120.0f;

        Trinity::AllCreaturesOfEntryInRange u_check(me, TRIGGER_ID, range);
        Trinity::UnitListSearcher<Trinity::AllCreaturesOfEntryInRange> searcher(pList, u_check);
        TypeContainerVisitor<Trinity::UnitListSearcher<Trinity::AllCreaturesOfEntryInRange>, GridTypeMapContainer >  grid_unit_searcher(searcher);

        cell.Visit(p, grid_unit_searcher, *(me->GetMap()));
    
        for(std::list<Unit *>::iterator itr = pList.begin(); itr != pList.end(); itr++)
            (*itr)->ToCreature()->AI()->Reset();
    }
    
    void JustRespawned()
    {
        //not finally - just a presentation - need 32place two side of chesstable
        if (pInstance && pInstance->GetData(DATA_CHESS_EVENT) == IN_PROGRESS) {
            float angle = me->GetOrientation();
            float pos_x = -11066;
            float pos_y = -1898;
            int move_lenght = 2*rand()%10;
            float new_x = pos_x + move_lenght * cos(angle);
            float new_y = pos_y + move_lenght * sin(angle);
            me->Relocate(new_x, new_y, 221, 2.24);
            me->CombatStop();
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SendMovementFlagUpdate();
        }
    }
    
    void OnCharmed(Unit* charmer, bool apply)
    {
        if (!pInstance)
            return;

        if (!charmer || charmer->GetTypeId() != TYPEID_PLAYER)
            return;
            
        if (apply)
            me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
        else
            me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            
        switch (me->GetEntry()) {
        case NPC_KING_A:
        {
            if (pInstance->GetData(DATA_CHESS_EVENT) != IN_PROGRESS) {
                pInstance->SetData(DATA_CHESS_EVENT, IN_PROGRESS);
                pInstance->SetData(CHESS_EVENT_TEAM, charmer->ToPlayer()->GetTeam());
                if (Creature* medivh = Creature::GetCreature(*me, pInstance->GetData64(DATA_IMAGE_OF_MEDIVH))) {
                    medivh->GetMotionMaster()->MoveConfused();
                    medivh->AI()->Reset();
                }
            }
            break;
        }
        case NPC_KING_H:
        {
            if (pInstance->GetData(DATA_CHESS_EVENT) != IN_PROGRESS) {
                pInstance->SetData(DATA_CHESS_EVENT, IN_PROGRESS);
                pInstance->SetData(CHESS_EVENT_TEAM, charmer->ToPlayer()->GetTeam());
                if (Creature* medivh = Creature::GetCreature(*me, pInstance->GetData64(DATA_IMAGE_OF_MEDIVH))) {
                    medivh->GetMotionMaster()->MoveConfused();
                    medivh->AI()->Reset();
                }
            }
            break;
        }
        }

        if (pInstance->GetData(CHESS_EVENT_TEAM) == ALLIANCE)
            me->setFaction(A_FACTION);
        else
            me->setFaction(H_FACTION);
    }
    
    void JustDied(Unit* pKiller)
    { 
        MedivhGUID = pInstance->GetData64(DATA_IMAGE_OF_MEDIVH);
        npc_medivh = Unit::GetUnit(*me, MedivhGUID);

        if (npc_medivh && pInstance->GetData(CHESS_EVENT_TEAM) == HORDE) {
            switch (me->GetEntry()) {
            case NPC_ROOK_H:   DoScriptText(SCRIPTTEXT_LOSE_ROOK_P, npc_medivh);     break;
            case NPC_ROOK_A:   DoScriptText(SCRIPTTEXT_LOSE_ROOK_M, npc_medivh);     break;
            case NPC_QUEEN_H:  DoScriptText(SCRIPTTEXT_LOSE_QUEEN_P, npc_medivh);    break;
            case NPC_QUEEN_A:  DoScriptText(SCRIPTTEXT_LOSE_QUEEN_M, npc_medivh);    break;
            case NPC_BISHOP_H: DoScriptText(SCRIPTTEXT_LOSE_BISHOP_P, npc_medivh);   break;
            case NPC_BISHOP_A: DoScriptText(SCRIPTTEXT_LOSE_BISHOP_M, npc_medivh);   break;
            case NPC_KNIGHT_H: DoScriptText(SCRIPTTEXT_LOSE_KNIGHT_P, npc_medivh);   break;
            case NPC_KNIGHT_A: DoScriptText(SCRIPTTEXT_LOSE_KNIGHT_M, npc_medivh);   break;
            case NPC_PAWN_H:
                DoScriptText(RAND(SCRIPTTEXT_LOSE_PAWN_P_1, SCRIPTTEXT_LOSE_PAWN_P_2, SCRIPTTEXT_LOSE_PAWN_P_3), npc_medivh); break;
            case NPC_PAWN_A:
                DoScriptText(RAND(SCRIPTTEXT_LOSE_PAWN_M_1, SCRIPTTEXT_LOSE_PAWN_M_2), npc_medivh); break;
            case NPC_KING_H:
                DoScriptText(SCRIPTTEXT_MEDIVH_WIN, npc_medivh);
                pInstance->SetData(DATA_CHESS_EVENT, FAIL);
                ResetTriggers();
                if (Creature* medivh = Creature::GetCreature(*me, pInstance->GetData64(DATA_IMAGE_OF_MEDIVH))) {
                    medivh->GetMotionMaster()->MoveIdle();
                    medivh->AI()->Reset();
                }
                break;
            case NPC_KING_A:
                DoScriptText(SCRIPTTEXT_PLAYER_WIN, npc_medivh);
                pInstance->SetData(DATA_CHESS_EVENT, DONE);
                ResetTriggers();
                if (Creature* medivh = Creature::GetCreature(*me, pInstance->GetData64(DATA_IMAGE_OF_MEDIVH))) {
                    medivh->GetMotionMaster()->MoveIdle();
                    medivh->AI()->Reset();
                }
                me->SummonGameObject(DUST_COVERED_CHEST, -11058, -1903, 221, 2.24, 0, 0, 0, 0, 7200000);
                break;
            default: break;
            }
        }
        else if (npc_medivh) {
            switch(me->GetEntry()) {
            case NPC_ROOK_A:   DoScriptText(SCRIPTTEXT_LOSE_ROOK_P, npc_medivh);     break;
            case NPC_ROOK_H:   DoScriptText(SCRIPTTEXT_LOSE_ROOK_M, npc_medivh);     break;
            case NPC_QUEEN_A:  DoScriptText(SCRIPTTEXT_LOSE_QUEEN_P, npc_medivh);    break;
            case NPC_QUEEN_H:  DoScriptText(SCRIPTTEXT_LOSE_QUEEN_M, npc_medivh);    break;
            case NPC_BISHOP_A: DoScriptText(SCRIPTTEXT_LOSE_BISHOP_P, npc_medivh);   break;
            case NPC_BISHOP_H: DoScriptText(SCRIPTTEXT_LOSE_BISHOP_M, npc_medivh);   break;
            case NPC_KNIGHT_A: DoScriptText(SCRIPTTEXT_LOSE_KNIGHT_P, npc_medivh);   break;
            case NPC_KNIGHT_H: DoScriptText(SCRIPTTEXT_LOSE_KNIGHT_M, npc_medivh);   break;
            case NPC_PAWN_A:
                DoScriptText(RAND(SCRIPTTEXT_LOSE_PAWN_P_1, SCRIPTTEXT_LOSE_PAWN_P_2, SCRIPTTEXT_LOSE_PAWN_P_3), npc_medivh); break;
            case NPC_PAWN_H:
                DoScriptText(RAND(SCRIPTTEXT_LOSE_PAWN_M_1, SCRIPTTEXT_LOSE_PAWN_M_2), npc_medivh); break;
            case NPC_KING_A:
                DoScriptText(SCRIPTTEXT_MEDIVH_WIN, npc_medivh);
                pInstance->SetData(DATA_CHESS_EVENT, FAIL);
                ResetTriggers();
                if (Creature* medivh = Creature::GetCreature(*me, pInstance->GetData64(DATA_IMAGE_OF_MEDIVH))) {
                    medivh->GetMotionMaster()->MoveIdle();
                    medivh->AI()->Reset();
                }
                break;
            case NPC_KING_H:
                DoScriptText(SCRIPTTEXT_PLAYER_WIN, npc_medivh);
                pInstance->SetData(DATA_CHESS_EVENT, DONE);
                ResetTriggers();
                if (Creature* medivh = Creature::GetCreature(*me, pInstance->GetData64(DATA_IMAGE_OF_MEDIVH))) {
                    medivh->GetMotionMaster()->MoveIdle();
                    medivh->AI()->Reset();
                }
                me->SummonGameObject(DUST_COVERED_CHEST, -11058, -1903, 221, 2.24, 0, 0, 0, 0, 7200000);
                break;
            default: break;
            }
        }
        
        if (me->isCharmed()) {
            if (Unit* charmer = me->GetCharmer())
                charmer->RemoveAurasDueToSpell(30019);
            me->RemoveCharmedOrPossessedBy(me->GetCharmer());
        }
            
        InGame = false;
        me->setActive(false);
        
        if (pInstance && pInstance->GetData(DATA_CHESS_EVENT) == IN_PROGRESS) {
            float angle = me->GetOrientation();
            float pos_x = -11066;
            float pos_y = -1898;
            int move_lenght = 2*rand()%10;
            float new_x = pos_x + move_lenght * cos(angle);
            float new_y = pos_y + move_lenght * sin(angle);
            me->Relocate(new_x, new_y, 221, 2.24);
            me->CombatStop();
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SendMovementFlagUpdate();
        }
        
        me->Respawn();

    }
    
    void OnSpellFinish(Unit* caster, uint32 spellId, Unit* target, bool ok)
    {
        me->CombatStop();
        
        if (spellId != SPELL_MOVE_1 && spellId != SPELL_MOVE_2 && spellId != SPELL_MOVE_3 && spellId != SPELL_MOVE_4 && 
                spellId != SPELL_MOVE_5 && spellId != SPELL_MOVE_6 && spellId != SPELL_MOVE_7 && spellId != SPELL_CHANGE_FACING && !LockInMovement) {
            me->SetOrientation(startingOrientation);
            me->SendMovementFlagUpdate();
        }
    }

    std::list<Unit*> FindPossibleMoveUnits()
    {
        CellPair p(Trinity::ComputeCellPair(me->GetPositionX(), me->GetPositionY()));
        Cell cell(p);
        cell.data.Part.reserved = ALL_DISTRICT;
        cell.SetNoCreate();

        std::list<Unit*> pList;
        std::list<Unit*> returnList;
        
        float range = GetStrafeLenght(me->GetEntry())*SEARCH_RANGE;

        Trinity::AllCreaturesOfEntryInRange u_check(me, TRIGGER_ID, range);
        Trinity::UnitListSearcher<Trinity::AllCreaturesOfEntryInRange> searcher(pList, u_check);
        TypeContainerVisitor<Trinity::UnitListSearcher<Trinity::AllCreaturesOfEntryInRange>, GridTypeMapContainer >  grid_unit_searcher(searcher);

        cell.Visit(p, grid_unit_searcher, *(me->GetMap()));
    
        for(std::list<Unit *>::iterator itr = pList.begin(); itr != pList.end(); itr++)
        {
            if ((*itr)->GetEntry() != TRIGGER_ID || ((move_triggerAI*)((Creature*)(*itr))->AI())->onMarker != NULL)// || !me->isInFront((*itr), 8.0f, 2*M_PI/6)) // TODO: Need better check to exclude triggers not in front or not at strafe.
                continue;
                
            if (!((move_triggerAI*)((*itr)->ToCreature()->AI()))->IsMovementAllowed(me))
                continue;
    
            returnList.push_back((*itr));
        }
    
        pList.clear();
        return returnList;
    }
    
    std::list<Unit*> FindNearestEnemies()
    {
        CellPair p(Trinity::ComputeCellPair(me->GetPositionX(), me->GetPositionY()));
        Cell cell(p);
        cell.data.Part.reserved = ALL_DISTRICT;
        cell.SetNoCreate();

        std::list<Unit*> pList;
        std::list<Unit*> returnList;
        
        float range = 5.0f;
        
        Trinity::AnyUnitInObjectRangeCheck u_check(me, range);
        Trinity::UnitListSearcher<Trinity::AnyUnitInObjectRangeCheck> searcher(pList, u_check);
        TypeContainerVisitor<Trinity::UnitListSearcher<Trinity::AnyUnitInObjectRangeCheck>, GridTypeMapContainer >  grid_unit_searcher(searcher);

        cell.Visit(p, grid_unit_searcher, *(me->GetMap()));

        for (std::list<Unit *>::iterator itr = pList.begin(); itr != pList.end(); itr++) {
            if ((*itr)->GetEntry() == TRIGGER_ID)
                continue;
                
            if ((*itr)->IsFriendlyTo(me))
                continue;
                
            returnList.push_back((*itr));
        }
        
        pList.clear();
        return returnList;
    }
    
    int GetStrafeLenght(uint32 Entry)
    {
        if (Entry == NPC_QUEEN_H || Entry == NPC_QUEEN_A)
            return 2;
        
        if (Entry == NPC_KNIGHT_H || Entry == NPC_KNIGHT_A)
            return 2;
        
        return 1;
    }
    
    bool IsLockedInMovement()
    {
        return LockInMovement;
    }

    void UpdateAI(const uint32 diff)
    {
        me->SetReactState(REACT_PASSIVE);
        
        if (!pInstance)
            return;
        
        if (LinkCellTimer) {
            if (LinkCellTimer <= diff) {
                me->CastSpell(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), SPELL_MOVE_1, true);
                if (Creature* marker = me->FindNearestCreature(TRIGGER_ID, 2.0f, true))
                    start_marker = marker;
                startingOrientation = me->GetOrientation();
                LinkCellTimer = 0;
            }
            else
                LinkCellTimer -= diff;
        }

        if (pInstance->GetData(DATA_CHESS_EVENT) == DONE || pInstance->GetData(DATA_CHESS_EVENT) == FAIL) {
            if(me->isInCombat())
                me->CombatStop();            

            if(me->isCharmed()) {
                if (Unit* charmer = me->GetCharmer())
                    charmer->RemoveAurasDueToSpell(30019);
                me->RemoveCharmedOrPossessedBy(me->GetCharmer());
            }

            if(me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE))
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            
            if (ReturnToHome) {
                float x, y, z, o;
                me->GetHomePosition(x, y, z, o);
                me->Relocate(x, y, z, o);
                me->Respawn();
                me->CastSpell(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), SPELL_MOVE_1, true);
                //ResetTriggers();
                ReturnToHome = false;
            }
        }
        
        if (pInstance->GetData(DATA_CHESS_EVENT) != IN_PROGRESS)
            return;

        if (!InGame)
            return;
            
        if (LockInMovement)
            return;
            
        ReturnToHome = true;
        bool moved = false;
        
        if (me->IsNonMeleeSpellCasted(false))
            return;

        if (me->getVictim() && me->GetExactDistance2d(me->getVictim()->GetPositionX(), me->getVictim()->GetPositionY() >= NPC_ATTACK_RADIUS))
            me->CombatStop();

        if (!me->isCharmed()) {
            if (!me->getVictim()) {
                // Check for possible moves
                if (NextMove_Timer < diff) {
                    if (rand()%100 <= 30) {
                        PossibleMoveUnits = FindPossibleMoveUnits();
                        
                        if (!PossibleMoveUnits.empty()) {
                            std::list<Unit*>::iterator i = PossibleMoveUnits.begin();
                            advance (i ,rand()%PossibleMoveUnits.size());
                            
                            end_marker = (Creature*)(*i);
                            
                            if (((move_triggerAI*)end_marker->AI())->onMarker != NULL && !((move_triggerAI*)end_marker->AI())->EndMarker) {             
                                for(uint8 x = 0; x<PossibleMoveUnits.size(); x++) {
                                    i = PossibleMoveUnits.begin();
                                    advance(i, x);
                                    
                                    end_marker = (Creature*)(*i);
                                    
                                    if(!((move_triggerAI*)end_marker->AI())->EndMarker) {
                                        //DoCast((*i), SPELL_MOVE_1, true);
                                        me->CastSpell((*i)->GetPositionX(), (*i)->GetPositionY(), (*i)->GetPositionZ(), SPELL_MOVE_1, true);
                                        moved = true;
                                        break;
                                    }
                                }
                            }
                            else {
                                //DoCast((*i), SPELL_MOVE_1, true);
                                me->CastSpell((*i)->GetPositionX(), (*i)->GetPositionY(), (*i)->GetPositionZ(), SPELL_MOVE_1, true);
                                moved = true;
                            }
                        
                            //start_marker = end_marker;
                        }
                        else {
                            float x, y, z;
                            me->GetNearPoint(me, x, y, z, 0, 5.0f, M_PI);
                            me->CastSpell(x, y, me->GetPositionZ(), SPELL_CHANGE_FACING, true);
                        }
                    }
                    
                    NextMove_Timer = 8000 + rand()%3000;
                }
                else
                    NextMove_Timer -= diff;
                    
                // Face nearest enemy
                if (!moved && !LockInMovement && !me->getVictim()) {
                    if (CheckNearEnemiesTimer <= diff) {
                        uint32 tmpTimer = 800;
                        std::list<Unit*> enemies = FindNearestEnemies();
                        if (!enemies.empty()) {
                            std::list<Unit*>::iterator i = enemies.begin();
                            advance(i, rand()%enemies.size());
                            
                            me->CastSpell((*i)->GetPositionX(), (*i)->GetPositionY(), (*i)->GetPositionZ(), SPELL_CHANGE_FACING, true);
                            AttackStart(*i);
                            moved = true;
                            tmpTimer = 5000 + rand()%2000;
                        }
                        
                        CheckNearEnemiesTimer = tmpTimer;
                    }
                    else
                        CheckNearEnemiesTimer -= diff;
                }
            }
                
            switch(me->GetEntry()) {
            case NPC_BISHOP_A:
                if (Heal_Timer <= diff) {
                    if (Unit* target = DoSelectLowestHpFriendly(25, 5000))
                        DoCast(target, BISHOP_HEAL_A);
                
                    Heal_Timer = 15000 + rand()%5000;
                }
                else
                    Heal_Timer -= diff;
                break;
            case NPC_BISHOP_H:
                if (Heal_Timer <= diff) {
                    if (Unit* target = DoSelectLowestHpFriendly(25, 5000))
                        DoCast(target, BISHOP_HEAL_H);
                
                    Heal_Timer = 15000 + rand()%5000;
                }
                else
                    Heal_Timer -= diff;
                break;
            default:
                break;
            }
            
            if (me->getVictim()) {
                if (CheckForceMoveTimer <= diff) {
                    // Give it a 5% chance to move even if not in combat
                    if (rand()%100 < 4) {
                        uint32 kingEntry = (pInstance->GetData(CHESS_EVENT_TEAM) == ALLIANCE) ? NPC_KING_A : NPC_KING_H;
                        if (Creature* king = me->FindCreatureInGrid(kingEntry, 80.0f, true)) {
                            float x, y, z;
                            me->GetNearPoint(me, x, y, z, 0, 8.0f, me->GetAngle(king));
                            if (me->HasInArc(M_PI, king))
                                me->CastSpell(x, y, z, SPELL_MOVE_1, false);
                            else
                                me->CastSpell(x, y, z, SPELL_CHANGE_FACING, false);
                        }
                    }
                    
                    CheckForceMoveTimer = 1000;
                }
                else
                    CheckForceMoveTimer -= diff;
            }
        }

        if (!me->getVictim()) {
            potentialTarget = SelectTarget();
            if (potentialTarget && !LockInMovement && !((npc_chesspieceAI*)potentialTarget->AI())->IsLockedInMovement())
                AttackStart(potentialTarget);
        }
        
        if (me->getVictim())
            DoMeleeAttackIfReady();
    }
    
    void SpellHitTarget(Unit *target, const SpellEntry* spell)
    {
        if (target->GetEntry() != TRIGGER_ID || (spell->Id != SPELL_MOVE_1
           && spell->Id != SPELL_MOVE_2 && spell->Id != SPELL_MOVE_3 && spell->Id != SPELL_MOVE_4
           && spell->Id != SPELL_MOVE_5 && spell->Id != SPELL_MOVE_6 && spell->Id != SPELL_MOVE_7) )
            return;
            
        if (pInstance && pInstance->GetData(DATA_CHESS_EVENT) == IN_PROGRESS)
            LockInMovement = true;
        startingOrientation = me->GetOrientation();
        if (start_marker)
            start_marker->AI()->Reset();

        /*if (me->isCharmed()) {
            Creature* marker = (Creature*)target;
            if (marker && ((move_triggerAI*)marker->AI())->onMarker == me) {
                start_marker = end_marker;
                end_marker = (Creature*)target;
            }
        }*/
    }
};

struct npc_echo_of_medivhAI : public ScriptedAI
{
    npc_echo_of_medivhAI(Creature* c) : ScriptedAI(c)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
    }
    
    uint32 cheatTimer;
    
    ScriptedInstance* pInstance;
    
    void Aggro(Unit* who) {}
    
    void Reset()
    {
        cheatTimer = 80000 + rand()%20000;
        
        CellPair p(Trinity::ComputeCellPair(me->GetPositionX(), me->GetPositionY()));
        Cell cell(p);
        cell.data.Part.reserved = ALL_DISTRICT;
        cell.SetNoCreate();

        std::list<Unit*> pList;
        std::list<Unit*> finalList;
        
        float range = 80.0f;

        Trinity::AllCreaturesOfEntryInRange u_check(me, 22521, range);
        Trinity::UnitListSearcher<Trinity::AllCreaturesOfEntryInRange> searcher(pList, u_check);
        TypeContainerVisitor<Trinity::UnitListSearcher<Trinity::AllCreaturesOfEntryInRange>, GridTypeMapContainer >  grid_unit_searcher(searcher);

        cell.Visit(p, grid_unit_searcher, *(me->GetMap()));
    
        for(std::list<Unit *>::iterator itr = pList.begin(); itr != pList.end(); itr++) {
            if ((*itr)->GetEntry() == 22521)
                (*itr)->ToCreature()->DisappearAndDie();
        }
    }
    
    bool IsFriendlyPiece(uint32 entry)
    {
        if (!pInstance)
            return false;

        switch (entry) {
        case NPC_PAWN_H:
        case NPC_KNIGHT_H:
        case NPC_QUEEN_H:
        case NPC_BISHOP_H:
        case NPC_ROOK_H:
        case NPC_KING_H:
            return (pInstance->GetData(CHESS_EVENT_TEAM) == HORDE);
        case NPC_PAWN_A:
        case NPC_KNIGHT_A:
        case NPC_QUEEN_A:
        case NPC_BISHOP_A:
        case NPC_ROOK_A:
        case NPC_KING_A:
            return (pInstance->GetData(CHESS_EVENT_TEAM) == ALLIANCE);
        }
        
        return false;
    }
    
    void UpdateAI(uint32 const diff)
    {
        if (!pInstance)
            return;
            
        if (pInstance->GetData(DATA_CHESS_EVENT) != IN_PROGRESS)
            return;
        
        if (cheatTimer <= diff) {
            switch (rand()%3) {
            case 0: // Heal king
            {
                if (pInstance->GetData(CHESS_EVENT_TEAM) == ALLIANCE) {
                    if (Creature* king = me->FindCreatureInGrid(NPC_KING_H, 80.0f, true))
                        king->SetHealth(king->GetMaxHealth());
                }
                else {
                    if (Creature* king = me->FindCreatureInGrid(NPC_KING_A, 80.0f, true))
                        king->SetHealth(king->GetMaxHealth());
                }
                break;
            }
            case 1: // Fire
            {
                CellPair p(Trinity::ComputeCellPair(me->GetPositionX(), me->GetPositionY()));
                Cell cell(p);
                cell.data.Part.reserved = ALL_DISTRICT;
                cell.SetNoCreate();

                std::list<Unit*> pList;
                std::list<Unit*> finalList;
                
                float range = 80.0f;

                Trinity::AllCreaturesOfEntryInRange u_check(me, 22519, range);
                Trinity::UnitListSearcher<Trinity::AllCreaturesOfEntryInRange> searcher(pList, u_check);
                TypeContainerVisitor<Trinity::UnitListSearcher<Trinity::AllCreaturesOfEntryInRange>, GridTypeMapContainer >  grid_unit_searcher(searcher);

                cell.Visit(p, grid_unit_searcher, *(me->GetMap()));
            
                for(std::list<Unit *>::iterator itr = pList.begin(); itr != pList.end(); itr++)
                {
                    if ((*itr)->GetEntry() != 22519)
                        continue;
                        
                    if (((move_triggerAI*)((Creature*)(*itr))->AI())->onMarker == NULL)
                        continue;
                        
                    if (!IsFriendlyPiece(((move_triggerAI*)((Creature*)(*itr))->AI())->onMarker->GetEntry()))
                        continue;
            
                    finalList.push_back(((move_triggerAI*)((Creature*)(*itr))->AI())->onMarker);
                }
                
                Trinity::RandomResizeList(finalList, 3);
                for (std::list<Unit*>::iterator itr = finalList.begin(); itr != finalList.end(); itr++)
                    DoCast(*itr, 39345, true);

                break;
            }
            case 2: // Buff
            {
                CellPair p(Trinity::ComputeCellPair(me->GetPositionX(), me->GetPositionY()));
                Cell cell(p);
                cell.data.Part.reserved = ALL_DISTRICT;
                cell.SetNoCreate();

                std::list<Unit*> pList;
                std::list<Unit*> finalList;
                
                float range = 80.0f;

                Trinity::AllCreaturesOfEntryInRange u_check(me, 22519, range);
                Trinity::UnitListSearcher<Trinity::AllCreaturesOfEntryInRange> searcher(pList, u_check);
                TypeContainerVisitor<Trinity::UnitListSearcher<Trinity::AllCreaturesOfEntryInRange>, GridTypeMapContainer >  grid_unit_searcher(searcher);

                cell.Visit(p, grid_unit_searcher, *(me->GetMap()));
            
                for(std::list<Unit *>::iterator itr = pList.begin(); itr != pList.end(); itr++)
                {
                    if ((*itr)->GetEntry() != 22519)
                        continue;
                        
                    if (((move_triggerAI*)((Creature*)(*itr))->AI())->onMarker == NULL)
                        continue;
                        
                    if (IsFriendlyPiece(((move_triggerAI*)((Creature*)(*itr))->AI())->onMarker->GetEntry()))
                        continue;
            
                    finalList.push_back(((move_triggerAI*)((Creature*)(*itr))->AI())->onMarker);
                }
                
                Trinity::RandomResizeList(finalList, 1);                    
                for (std::list<Unit*>::iterator itr = finalList.begin(); itr != finalList.end(); itr++)
                    DoCast(*itr, 39339, true);

                break;
            }
            }
            
            cheatTimer = 80000 + rand()%20000;
        }
        else
            cheatTimer -= diff;
    }
};

bool GossipHello_npc_chesspiece(Player* player, Creature* creature)
{
    ScriptedInstance* pInstance = ((ScriptedInstance*)creature->GetInstanceData());

    if (pInstance && pInstance->GetData(DATA_CHESS_EVENT) == DONE)
        return true;
  
    if (player->HasAura(SPELL_RECENTLY_INGAME))
        return true;
        
    if (((npc_chesspieceAI*)creature->AI())->LockInMovement && pInstance && pInstance->GetData(DATA_CHESS_EVENT) == IN_PROGRESS)
        return true;

    if (player->GetTeam() == ALLIANCE && creature->getFaction() != A_FACTION)
        return true;

    if (player->GetTeam() == HORDE && creature->getFaction() != H_FACTION)
        return true;

    if (pInstance && pInstance->GetData(DATA_CHESS_EVENT) != IN_PROGRESS && creature->GetEntry() != NPC_KING_A && creature->GetEntry() != NPC_KING_H)
        return true;

    player->ADD_GOSSIP_ITEM(0, GOSSIP_POSSES, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
    player->SEND_GOSSIP_MENU(8990, creature->GetGUID());

    return true;
}

bool GossipSelect_npc_chesspiece(Player* player, Creature* creature, uint32 sender, uint32 action)
{
    if (action == GOSSIP_ACTION_INFO_DEF + 1) {
        player->CastSpell(creature, SPELL_POSSESS_CHESSPIECE, false);
        player->TeleportTo(532, playerTeleportPosition[0], playerTeleportPosition[1], playerTeleportPosition[2], playerTeleportPosition[3]);
    }

    player->CLOSE_GOSSIP_MENU();

    return true;
}

bool GossipHello_npc_echo_of_medivh(Player* player, Creature* creature)
{
    ScriptedInstance* pInstance = ((ScriptedInstance*)creature->GetInstanceData());
    
    if (pInstance->GetData(DATA_CHESS_EVENT) == FAIL)
        pInstance->SetData(DATA_CHESS_EVENT, NOT_STARTED);

    if (pInstance->GetData(DATA_CHESS_EVENT) == IN_PROGRESS)
        player->ADD_GOSSIP_ITEM(0, "Nous souhaitons recommencer.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);

    //player->ADD_GOSSIP_ITEM(0, EVENT_START, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
    player->SEND_GOSSIP_MENU(8990, creature->GetGUID());
    
    return true;
}

bool GossipSelect_npc_echo_of_medivh(Player* player, Creature* creature, uint32 sender, uint32 action)
{
    ScriptedInstance* pInstance = ((ScriptedInstance*)creature->GetInstanceData());
    
    if (!pInstance)
        return true;

    /*if (action == GOSSIP_ACTION_INFO_DEF + 1) {
        DoScriptText(SCRIPTTEXT_AT_EVENT_START, creature);
        pInstance->SetData(DATA_CHESS_EVENT, IN_PROGRESS);
        pInstance->SetData(CHESS_EVENT_TEAM, player->GetTeam());
        creature->GetMotionMaster()->MoveConfused();
    }*/
    
    if (action == GOSSIP_ACTION_INFO_DEF + 2) {
        pInstance->SetData(DATA_CHESS_EVENT, FAIL);
        creature->GetMotionMaster()->MoveIdle();
    }

    player->CLOSE_GOSSIP_MENU();
    
    return true;
}

CreatureAI* GetAI_npc_chesspiece(Creature* pCreature)
{
    return new npc_chesspieceAI(pCreature);
}

CreatureAI* GetAI_move_trigger(Creature* pCreature)
{
    return new move_triggerAI(pCreature);
}

CreatureAI* GetAI_npc_echo_of_medivh(Creature* pCreature)
{
    return new npc_echo_of_medivhAI(pCreature);
}

void AddSC_chess_event()
{
    Script* newscript;

    newscript = new Script;
    newscript->Name = "npc_chesspiece";
    newscript->GetAI = &GetAI_npc_chesspiece;
    newscript->pGossipHello = &GossipHello_npc_chesspiece;
    newscript->pGossipSelect = &GossipSelect_npc_chesspiece;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_echo_of_medivh";
    newscript->pGossipHello = &GossipHello_npc_echo_of_medivh;
    newscript->pGossipSelect = &GossipSelect_npc_echo_of_medivh;
    newscript->GetAI = &GetAI_npc_echo_of_medivh;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "chess_move_trigger";
    newscript->GetAI = &GetAI_move_trigger;
    newscript->RegisterSelf();
}
