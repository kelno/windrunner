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

/*
TODO:
 - Implement in instance_karazhan dead chess_piece count per side to allow Medivh to cheat when is loosing.(Or different terms was used as a treshold for cheat ?)
 - Set proper position on left or right site from chess board for killed units.
 - Half turn if no trigger in arc (facing external side of the board)
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

typedef struct boardCell
{
    uint64 triggerGUID;
    uint64 pieceGUID;
    uint32 pieceEntry;
    uint8 row;
    uint8 col;
    
    void setData(uint64 _triggerGUID, uint8 _row, uint8 _col)
    {
        triggerGUID = _triggerGUID;
        row = _row;
        col = _col;
    }
    
    void reset()
    {
        pieceGUID = 0;
        pieceEntry = 0;
    }
    
    void setPiece(Creature* piece)
    {
        pieceGUID = piece->GetGUID();
        pieceEntry = piece->GetEntry();
    }
} BoardCell;

typedef enum gamePhase
{
    NOTSTARTED      = 0,
    PVE_WARMUP      = 1, // Medivh has been spoken too but king isn't controlled yet
    INPROGRESS_PVE  = 2,
    FAILED          = 4,
    PVE_FINISHED    = 5,
    PVP_WARMUP      = 6,
    INPROGRESS_PVP  = 7  // Get back to PVE_FINISHED after that
} GamePhase;

GamePhase chessPhase = NOTSTARTED;

typedef enum chessOrientationType
{
    ORI_SE  = 0,    // Horde start
    ORI_SW  = 1,
    ORI_NW  = 2,    // Alliance start
    ORI_NE  = 3
} ChessOrientationType;

float orientations[4] = { 3.809080, 2.235102, 0.661124, 5.385472 };

/*
x = case (0, 0) - y = case (7, 7)
    --------------------
        HORDE PIECES  y

X

     x  ALLY  PIECES
    --------------------

*/

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

enum deadTeam
{
    DEAD_ALLIANCE   = 0,
    DEAD_HORDE      = 1
};

struct npc_echo_of_medivhAI : public ScriptedAI
{
    npc_echo_of_medivhAI(Creature* c) : ScriptedAI(c)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
    }

    BoardCell* board[8][8];
    
    uint32 cheatTimer;
    uint32 deadCount[2];
    
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
    
    void RemoveCheats() {} // TODO: fires and buffs
    
    void SetupBoard()
    {
        // Cleanup needed?
        if (Creature* trigger = me->FindNearestCreature(TRIGGER_ID, 15.0f, true)) {
            for (uint8 row = 0; row < 8; row++) {
                for (uint8 col = 0; col < 8; col++) {
                    if (Creature* cellTrigger = Creature::GetCreature(*me, board[row][col]->triggerGUID))
                        cellTrigger->ForcedDespawn();
                }
                
                delete[] board[row];
            }
            
            delete[] board;
        }

        for (uint8 row = 0; row < 8; row++) {
            for (uint8 col = 0; col < 8; col++) {
                BoardCell* cell = new BoardCell;
                board[row][col] = cell;

                if (Creature* trigger = me->SummonCreature(TRIGGER_ID, (-11108.099609 + (3.49f * col) + (4.4f * row)), (-1872.910034 - (4.4f * col) + (3.45f * row)), 220.667f, 0, TEMPSUMMON_MANUAL_DESPAWN, 0)) {
                    cell->setData(trigger->GetGUID(), row, col);
                    HandleCellInitialData(row, col, trigger, cell);
                }
            }
        }
        
        deadCount[DEAD_ALLIANCE] = 0;
        deadCount[DEAD_HORDE] = 0;
    }
    
    void HandleCellInitialData(uint8 row, uint8 col, Creature* trigger, BoardCell* cell)
    {
        switch (row) {
        case 0: // Alliance first row
            switch (col) {
            case 0:
            case 7: // Rook
                if (Creature* rook = trigger->FindNearestCreature(NPC_ROOK_A, 40.0f, true))
                    cell->setPiece(rook);
                break;
            case 1:
            case 6: // Knight
                if (Creature* knight = trigger->FindNearestCreature(NPC_KNIGHT_A, 40.0f, true))
                    cell->setPiece(knight);
                break;
            case 2:
            case 5: // Bishop
                if (Creature* bishop = trigger->FindNearestCreature(NPC_BISHOP_A, 40.0f, true))
                    cell->setPiece(bishop);
                break;
            case 3: // Queen
                if (Creature* queen = trigger->FindNearestCreature(NPC_QUEEN_A, 40.0f, true))
                    cell->setPiece(queen);
                break;
            case 4: // King
                if (Creature* king = trigger->FindNearestCreature(NPC_KING_A, 40.0f, true))
                    cell->setPiece(king);
                break;
            }
            break;
        case 1: // Alliance second row
            // All pawns
            if (Creature* pawn = trigger->FindNearestCreature(NPC_PAWN_A, 40.0f, true))
                cell->setPiece(pawn);
            break;
        case 6: // Horde second row
            // All pawns
            if (Creature* pawn = trigger->FindNearestCreature(NPC_PAWN_H, 40.0f, true))
                cell->setPiece(pawn);
            break;
        case 7: // Horde first row
            switch (col) {
            case 0:
            case 7: // Rook
                if (Creature* rook = trigger->FindNearestCreature(NPC_ROOK_H, 40.0f, true))
                    cell->setPiece(rook);
                break;
            case 1:
            case 6: // Knight
                if (Creature* knight = trigger->FindNearestCreature(NPC_KNIGHT_H, 40.0f, true))
                    cell->setPiece(knight);
                break;
            case 2:
            case 5: // Bishop
                if (Creature* bishop = trigger->FindNearestCreature(NPC_BISHOP_H, 40.0f, true))
                    cell->setPiece(bishop);
                break;
            case 3: // Queen
                if (Creature* queen = trigger->FindNearestCreature(NPC_QUEEN_H, 40.0f, true))
                    cell->setPiece(queen);
                break;
            case 4: // King
                if (Creature* king = trigger->FindNearestCreature(NPC_KING_H, 40.0f, true))
                    cell->setPiece(king);
                break;
            }
            break;
        default:
            //sLog.outString("Default for %u %u", row, col);
            cell->reset();
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
    
    void ReceiveEmote(Player* /*player*/, uint32 /*text_emote*/)
    {
        for (uint8 row = 0; row < 8; row++) {
            for (uint8 col = 0; col < 8; col++) {
                sLog.outString("%u %u: "I64FMTD, row, col, board[row][col]->triggerGUID);
            }
        }
    }
    
    void HandlePieceDeath(Creature* piece)
    {
        switch (piece->getFaction()) {
        case A_FACTION:
        {
            float baseX = -11078.116211;
            float baseY = -1908.443115;
            float deltaX = 2.148438;
            float deltaY = 1.723755;
            float extraX = 2.416992;
            float extraY = -2.889649;
            
            float offset = 1.3f * (deadCount[DEAD_ALLIANCE] % 8);

            float finalX = baseX + offset * deltaX + (deadCount[DEAD_ALLIANCE] >= 8 ? 1 : 0) * extraX;
            float finalY = baseY + offset * deltaY + (deadCount[DEAD_ALLIANCE] >= 8 ? 1 : 0) * extraY;
            piece->Relocate(finalX, finalY, 221, orientations[ORI_SW]);
            ++deadCount[DEAD_ALLIANCE];

            piece->CombatStop();
            piece->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            piece->SendMovementFlagUpdate();
            piece->setDeathState(JUST_ALIVED);
            piece->SetHealth(piece->GetMaxHealth());
            break;
        }
        case H_FACTION:
            float baseX = -11081.393555;
            float baseY = -1844.194092;
            float deltaX = -2.148438;
            float deltaY = -1.723755;
            float extraX = -2.416992;
            float extraY = 2.889649;
            
            float offset = 1.3f * (deadCount[DEAD_ALLIANCE] % 8);

            float finalX = baseX + offset * deltaX + (deadCount[DEAD_ALLIANCE] >= 8 ? 1 : 0) * extraX;
            float finalY = baseY + offset * deltaY + (deadCount[DEAD_ALLIANCE] >= 8 ? 1 : 0) * extraY;
            piece->Relocate(finalX, finalY, 221, orientations[ORI_NE]);
            ++deadCount[DEAD_ALLIANCE];

            piece->CombatStop();
            piece->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            piece->SendMovementFlagUpdate();
            piece->setDeathState(JUST_ALIVED);
            piece->SetHealth(piece->GetMaxHealth());
            break;
        }
    }
    
    int HandlePieceRotate(Creature* piece, uint64 trigger)
    {
        uint8 pieceRow = 8, pieceCol = 8, targetRow, targetCol;
        for (uint8 row = 0; row < 8; row++) {
            for (uint8 col = 0; col < 8; col++) {
                BoardCell* cell = board[row][col];
                if (cell->triggerGUID == trigger) {
                    targetRow = row;
                    targetCol = col;
                }
                if (cell->pieceGUID == piece->GetGUID()) {
                    pieceRow = row;
                    pieceCol = col;
                }
            }
        }

        int8 deltaRow = pieceRow - targetRow;
        int8 deltaCol = pieceCol - targetCol;
        
        if (fabs(deltaRow) + fabs(deltaCol) > 1)
            return -1;

        if (deltaRow == 1 && deltaCol == 0)
            return ORI_SE;
        if (deltaRow == 0 && deltaCol == 1)
            return ORI_SW;
        if (deltaRow == -1 && deltaCol == 0)
            return ORI_NW;
        if (deltaRow == 0 && deltaCol == -1)
            return ORI_NE;
    }
    
    bool HandlePieceMove(Creature* piece, uint64 trigger)
    {
        bool res = false;
        uint8 oldCol = 8, oldRow = 8, newCol, newRow;
        for (uint8 row = 0; row < 8; row++) {
            for (uint8 col = 0; col < 8; col++) {
                BoardCell* cell = board[row][col];
                if (cell->triggerGUID == trigger) {
                    if (cell->pieceGUID) {
                        res = false;
                    }
                    else {
                        newCol = col;
                        newRow = row;
                        res = true;
                    }
                    break;
                }
                if (cell->pieceGUID == piece->GetGUID()) {
                    oldCol = col;
                    oldRow = row;
                }
            }
        }
        
        if (res) {
            uint8 deltaRow = fabs(oldRow - newRow);
            uint8 deltaCol = fabs(oldCol - newCol);
            switch (piece->GetEntry()) {
            case NPC_PAWN_H:
            case NPC_PAWN_A:
            case NPC_BISHOP_H:
            case NPC_BISHOP_A:
            case NPC_ROOK_H:
            case NPC_ROOK_A:
            case NPC_KING_H:
            case NPC_KING_A:
                if (deltaRow > 1 || deltaCol > 1)
                    res = false;
                break;
            case NPC_QUEEN_H:
            case NPC_QUEEN_A:
                if (deltaRow > 3 || deltaCol > 3) // FIXME: not correct
                    res = false;
                break;
            case NPC_KNIGHT_H:
            case NPC_KNIGHT_A:
                if (deltaRow > 2 || deltaCol > 2) // FIXME: not correct
                    res = false;
                break;
            default:
                break;
            }
        }
        
        if (res) {
            board[newRow][newCol]->triggerGUID = trigger;
            board[newRow][newCol]->setPiece(piece);
            if (oldRow != 8 && oldCol != 8)
                board[oldRow][oldCol]->reset();
        }

        return res;
    }
    
    void HandleCheat()
    {
        switch (rand()%3) {
        case 0: // Heal king
        {
            if (pInstance->GetData(CHESS_EVENT_TEAM) == ALLIANCE) {
                if (Creature* king = me->FindCreatureInGrid(NPC_KING_H, 80.0f, true)) {
                    if (king->isAlive())
                        king->SetHealth(king->GetMaxHealth());
                }
            }
            else {
                if (Creature* king = me->FindCreatureInGrid(NPC_KING_A, 80.0f, true)) {
                    if (king->isAlive())
                        king->SetHealth(king->GetMaxHealth());
                }
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
                    
                /*if (((move_triggerAI*)((Creature*)(*itr))->AI())->onMarker == NULL)
                    continue;
                    
                if (!IsFriendlyPiece(((move_triggerAI*)((Creature*)(*itr))->AI())->onMarker->GetEntry()))
                    continue;
                    
                if ((*itr)->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE))
                    continue;
        
                finalList.push_back(((move_triggerAI*)((Creature*)(*itr))->AI())->onMarker);*/
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
                    
                /*if (((move_triggerAI*)((Creature*)(*itr))->AI())->onMarker == NULL)
                    continue;
                    
                if (IsFriendlyPiece(((move_triggerAI*)((Creature*)(*itr))->AI())->onMarker->GetEntry()))
                    continue;
                    
                if ((*itr)->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE))
                    continue;
        
                finalList.push_back(((move_triggerAI*)((Creature*)(*itr))->AI())->onMarker);*/
            }
            
            Trinity::RandomResizeList(finalList, 1);                    
            for (std::list<Unit*>::iterator itr = finalList.begin(); itr != finalList.end(); itr++)
                DoCast(*itr, 39339, true);

            break;
        }
        }
    }
    
    void UpdateAI(uint32 const diff)
    {
        if (!pInstance)
            return;
            
        if (pInstance->GetData(DATA_CHESS_EVENT) != IN_PROGRESS)
            return;
        
        if (cheatTimer <= diff) {
            HandleCheat();
            
            cheatTimer = 80000 + rand()%20000;
        }
        else
            cheatTimer -= diff;
    }
};

struct npc_chesspieceAI : public Scripted_NoMovementAI
{
    npc_chesspieceAI(Creature *c) : Scripted_NoMovementAI(c)
    {
        pInstance = ((ScriptedInstance*)me->GetInstanceData());
    }
    
    ScriptedInstance* pInstance;
    
    bool ReturnToHome;
    bool InGame;
    bool CanMove;
    bool LockInMovement;
    
    uint32 Heal_Timer;
    uint32 NextMove_Timer;
    uint32 CheckForceMoveTimer;
    uint32 CheckNearEnemiesTimer;
    
    uint64 MedivhGUID;
    
    float destX, destY;

    std::list<Unit *> PossibleMoveUnits;
    
    ChessOrientationType currentOrientation;
    
    void EnterEvadeMode()
    {
        // Just stay in place
        me->RemoveAllAuras();
        Reset();
    }
    
    void Aggro(Unit* pWho)
    {
        Unit* npc_medivh = Unit::GetUnit(*me, MedivhGUID);
        
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
    
        me->ApplySpellImmune(0, IMMUNITY_ID, 39331, true);
        
        switch (me->getFaction()) {
        case A_FACTION:
            currentOrientation = ORI_NW;
            break;
        case H_FACTION:
            currentOrientation = ORI_SE;
            break;
        }
        
        MedivhGUID = pInstance->GetData64(DATA_IMAGE_OF_MEDIVH);
    }
    
    void MovementInform(uint32 MovementType, uint32 Data)
    {
        if (MovementType != POINT_MOTION_TYPE)
            return;
            
        LockInMovement = false;
        
        me->SetOrientation(orientations[currentOrientation]);
        me->Relocate(destX, destY, 220.667f);
        me->SendMovementFlagUpdate();
    }
    
    void AttackStart(Unit* who) {}
    
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
            
        MedivhGUID = pInstance->GetData64(DATA_IMAGE_OF_MEDIVH);

        if (!charmer || charmer->GetTypeId() != TYPEID_PLAYER)
            return;
            
        if (apply)
            me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
        else
            me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);

        if (pInstance->GetData(CHESS_EVENT_TEAM) == ALLIANCE)
            me->setFaction(A_FACTION);
        else
            me->setFaction(H_FACTION);
    }
    
    void JustDied(Unit* pKiller)
    { 
        Creature* medivh = Creature::GetCreature(*me, pInstance ? pInstance->GetData64(DATA_IMAGE_OF_MEDIVH) : 0);
        ((npc_echo_of_medivhAI*)medivh->AI())->HandlePieceDeath(me);
        
        if (me->isCharmed()) {
            if (Unit* charmer = me->GetCharmer())
                charmer->RemoveAurasDueToSpell(30019);
            me->RemoveCharmedOrPossessedBy(me->GetCharmer());
        }

        /*if (npc_medivh && pInstance->GetData(CHESS_EVENT_TEAM) == HORDE) {
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
                if (Creature* medivh = Creature::GetCreature(*me, pInstance->GetData64(DATA_IMAGE_OF_MEDIVH))) {
                    medivh->GetMotionMaster()->MoveIdle();
                    medivh->AI()->Reset();
                }
                break;
            case NPC_KING_A:
                DoScriptText(SCRIPTTEXT_PLAYER_WIN, npc_medivh);
                pInstance->SetData(DATA_CHESS_EVENT, DONE);
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
                if (Creature* medivh = Creature::GetCreature(*me, pInstance->GetData64(DATA_IMAGE_OF_MEDIVH))) {
                    medivh->GetMotionMaster()->MoveIdle();
                    medivh->AI()->Reset();
                }
                break;
            case NPC_KING_H:
                DoScriptText(SCRIPTTEXT_PLAYER_WIN, npc_medivh);
                pInstance->SetData(DATA_CHESS_EVENT, DONE);
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
        
        me->Respawn();*/

    }
    
    void OnSpellFinish(Unit* caster, uint32 spellId, Unit* target, bool ok)
    {
        me->CombatStop();
        
        if (spellId != SPELL_MOVE_1 && spellId != SPELL_MOVE_2 && spellId != SPELL_MOVE_3 && spellId != SPELL_MOVE_4 && 
                spellId != SPELL_MOVE_5 && spellId != SPELL_MOVE_6 && spellId != SPELL_MOVE_7 && spellId != SPELL_CHANGE_FACING && !LockInMovement) {
            me->SetOrientation(orientations[currentOrientation]);
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
            /*if ((*itr)->GetEntry() != TRIGGER_ID || ((move_triggerAI*)((Creature*)(*itr))->AI())->onMarker != NULL)// || !me->isInFront((*itr), 8.0f, 2*M_PI/6)) // TODO: Need better check to exclude triggers not in front or not at strafe.
                continue;
                
            if (!((move_triggerAI*)((*itr)->ToCreature()->AI()))->IsMovementAllowed(me))
                continue;*/
    
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
    
    Creature* GetTarget()
    {
        Creature* target = Creature::GetCreature(*me, me->GetUInt64Value(UNIT_FIELD_TARGET));
        return target;
    }
    
    void MoveInLineOfSight(Unit* who) {}

    void UpdateAI(const uint32 diff)
    {
        me->SetReactState(REACT_PASSIVE);
        
        if (!pInstance)
            return;

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
                ReturnToHome = false;
            }
        }
        
        if (pInstance->GetData(DATA_CHESS_EVENT) != IN_PROGRESS)
            return;
            
        if (LockInMovement && me->GetMotionMaster()->GetCurrentMovementGeneratorType() != POINT_MOTION_TYPE)
            LockInMovement = false;

        if (!InGame)
            return;
            
        if (LockInMovement)
            return;
            
        ReturnToHome = true;
        bool moved = false;
        
        if (me->IsNonMeleeSpellCasted(false))
            return;

        if (me->isInCombat() && GetTarget() && me->GetExactDistance2d(GetTarget()->GetPositionX(), GetTarget()->GetPositionY()) >= NPC_ATTACK_RADIUS)
            me->CombatStop();

        if (!me->isCharmed()) {
            if (!me->isInCombat()) {
                // Check for possible moves
                if (NextMove_Timer < diff) {
                    if (rand()%100 <= 30) {
                        PossibleMoveUnits = FindPossibleMoveUnits();
                        
                        if (!PossibleMoveUnits.empty()) {
                            std::list<Unit*>::iterator i = PossibleMoveUnits.begin();
                            advance (i ,rand()%PossibleMoveUnits.size());
                            
                            /*if (((move_triggerAI*)end_marker->AI())->onMarker != NULL && !((move_triggerAI*)end_marker->AI())->EndMarker) {             
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
                            }*/
                        
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
                if (!moved && !LockInMovement && !me->isInCombat()) {
                    if (CheckNearEnemiesTimer <= diff) {
                        uint32 tmpTimer = 800;
                        std::list<Unit*> enemies;
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
            
            if (me->isInCombat()) {
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
                    
                    CheckForceMoveTimer = 5000;
                }
                else
                    CheckForceMoveTimer -= diff;
            }
        }
        
        if (me->isInCombat())
            DoMeleeAttackIfReady();
    }
    
    void SpellHitTarget(Unit *target, const SpellEntry* spell)
    {
        if (target->GetEntry() != TRIGGER_ID)
            return;

        if ((spell->Id == SPELL_MOVE_1
           || spell->Id == SPELL_MOVE_2 || spell->Id == SPELL_MOVE_3 || spell->Id == SPELL_MOVE_4
           || spell->Id == SPELL_MOVE_5 || spell->Id == SPELL_MOVE_6 || spell->Id == SPELL_MOVE_7)) {
            if (Creature* medivh = Creature::GetCreature(*me, MedivhGUID)) {
                if (((npc_echo_of_medivhAI*)medivh->AI())->HandlePieceMove(me, target->GetGUID())) {
                    destX = target->GetPositionX();
                    destY = target->GetPositionY();
                    me->GetMotionMaster()->MovePoint(0, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ());
                    target->CastSpell(target, SPELL_MOVE_MARKER, false);
                }
            }
        }
        else if (spell->Id == SPELL_CHANGE_FACING) {
            if (Creature* medivh = Creature::GetCreature(*me, MedivhGUID)) {
                int result = ((npc_echo_of_medivhAI*)medivh->AI())->HandlePieceRotate(me, target->GetGUID());
                if (result != -1) {
                    me->SetOrientation(orientations[result]);
                    me->SendMovementFlagUpdate();
                }
            }
        }
    }
};

bool GossipHello_npc_chesspiece(Player* player, Creature* creature)
{
    if (player->HasAura(SPELL_RECENTLY_INGAME))
        return true;

    if (player->GetTeam() == ALLIANCE && creature->getFaction() != A_FACTION)
        return true;

    if (player->GetTeam() == HORDE && creature->getFaction() != H_FACTION)
        return true;
        
    bool ok = true;
    
    switch (creature->GetEntry()) {
    case NPC_PAWN_H:
    case NPC_PAWN_A:
    case NPC_KNIGHT_H:
    case NPC_KNIGHT_A:
    case NPC_QUEEN_H:
    case NPC_QUEEN_A:
    case NPC_BISHOP_H:
    case NPC_BISHOP_A:
    case NPC_ROOK_H:
    case NPC_ROOK_A:
        if (chessPhase != INPROGRESS_PVE && chessPhase != INPROGRESS_PVP)
            ok = false;
            
        break;
    case NPC_KING_H:
    case NPC_KING_A:
        if (chessPhase != INPROGRESS_PVE && chessPhase != INPROGRESS_PVP && chessPhase != PVE_WARMUP && chessPhase != PVP_WARMUP)
            ok = false;
            
        break;
    default:
        ok = false;
        break;
    }

    if (ok) {
        player->ADD_GOSSIP_ITEM(0, GOSSIP_POSSES, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        player->SEND_GOSSIP_MENU(8990, creature->GetGUID());
    }

    return true;
}

bool GossipSelect_npc_chesspiece(Player* player, Creature* creature, uint32 sender, uint32 action)
{
    if (action == GOSSIP_ACTION_INFO_DEF + 1) {
        player->CastSpell(creature, SPELL_POSSESS_CHESSPIECE, false);
        player->TeleportTo(532, playerTeleportPosition[0], playerTeleportPosition[1], playerTeleportPosition[2], playerTeleportPosition[3]);
        
        if (chessPhase == PVE_WARMUP)
            chessPhase = INPROGRESS_PVE;
        else if (chessPhase == PVP_WARMUP)
            chessPhase = INPROGRESS_PVP;
    }

    player->CLOSE_GOSSIP_MENU();

    return true;
}

enum medivhGossipOptions
{
    MEDIVH_GOSSIP_START_PVE,
    MEDIVH_GOSSIP_RESTART,
    MEDIVH_GOSSIP_START_PVP
};

bool GossipHello_npc_echo_of_medivh(Player* player, Creature* creature)
{
    ScriptedInstance* pInstance = ((ScriptedInstance*)creature->GetInstanceData());
    
    if (!pInstance)
        return true;

    if (chessPhase == FAILED)
        chessPhase = NOTSTARTED;
    
    if (pInstance->GetData(DATA_CHESS_EVENT) == DONE && chessPhase == NOTSTARTED)
        chessPhase = PVE_FINISHED;

    if (chessPhase == NOTSTARTED)
        player->ADD_GOSSIP_ITEM(0, "Nous souhaitons jouer une partie contre vous !", GOSSIP_SENDER_MAIN, MEDIVH_GOSSIP_START_PVE);

    if (chessPhase == INPROGRESS_PVE || chessPhase == INPROGRESS_PVP)
        player->ADD_GOSSIP_ITEM(0, "Nous souhaitons recommencer.", GOSSIP_SENDER_MAIN, MEDIVH_GOSSIP_RESTART);
        
    if (chessPhase == PVE_FINISHED)
        player->ADD_GOSSIP_ITEM(0, "Nous souhaitons jouer entre nous.", GOSSIP_SENDER_MAIN, MEDIVH_GOSSIP_START_PVP);

    player->SEND_GOSSIP_MENU(8990, creature->GetGUID());
    
    return true;
}

bool GossipSelect_npc_echo_of_medivh(Player* player, Creature* creature, uint32 sender, uint32 action)
{
    ScriptedInstance* pInstance = ((ScriptedInstance*)creature->GetInstanceData());
    
    if (!pInstance)
        return true;
        
    pInstance->SetData(CHESS_EVENT_TEAM, player->GetTeam());
    
    switch (action) {
    case MEDIVH_GOSSIP_START_PVE:
        chessPhase = PVE_WARMUP;
        ((npc_echo_of_medivhAI*)(creature->AI()))->SetupBoard();
        break;
    case MEDIVH_GOSSIP_RESTART:
        chessPhase = FAILED;
        pInstance->SetData(DATA_CHESS_REINIT_PIECES, 0);
        break;
    case MEDIVH_GOSSIP_START_PVP:
        chessPhase = PVP_WARMUP;
        ((npc_echo_of_medivhAI*)(creature->AI()))->SetupBoard();
        break;
    default:
        sLog.outError("Chess event: unknown action %u", action);
        break;
    }

    player->CLOSE_GOSSIP_MENU();
    
    return true;
}

struct chess_move_triggerAI : public Scripted_NoMovementAI
{
    chess_move_triggerAI(Creature* c) : Scripted_NoMovementAI(c) {}
    
    void Aggro(Unit* who) {}
    
    void SpellHit(Unit* caster, const SpellEntry* spell)
    {
        
    }
};

CreatureAI* GetAI_npc_chesspiece(Creature* pCreature)
{
    return new npc_chesspieceAI(pCreature);
}

CreatureAI* GetAI_npc_echo_of_medivh(Creature* pCreature)
{
    return new npc_echo_of_medivhAI(pCreature);
}

CreatureAI* GetAI_chess_move_trigger(Creature* creature)
{
    return new chess_move_triggerAI(creature);
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
    newscript->GetAI = &GetAI_chess_move_trigger;
    newscript->RegisterSelf();
}
