/*
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 *
 * Copyright (C) 2008-2009 Trinity <http://www.trinitycore.org/>
 *
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

#include "MapManager.h"
#include "Player.h"
#include "GridNotifiers.h"
#include "WorldSession.h"
#include "Log.h"
#include "GridStates.h"
#include "CellImpl.h"
#include "InstanceData.h"
#include "Map.h"
#include "GridNotifiersImpl.h"
#include "Config/ConfigEnv.h"
#include "Transports.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "World.h"
#include "ScriptCalls.h"
#include "Group.h"
#include "MapRefManager.h"
#include "../scripts/ScriptMgr.h"

#include "MapInstanced.h"
#include "InstanceSaveMgr.h"
#include "VMapFactory.h"
#include "MoveMap.h"
#include "DynamicTree.h"
#include "BattleGround.h"

#define DEFAULT_GRID_EXPIRY     300
#define MAX_GRID_LOAD_TIME      50
#define MAX_CREATURE_ATTACK_RADIUS  (45.0f * sWorld.getRate(RATE_CREATURE_AGGRO))

GridState* si_GridStates[MAX_GRID_STATE];

Map::~Map()
{
    UnloadAll();
}

bool Map::ExistMap(uint32 mapid,int x,int y)
{
    int len = sWorld.GetDataPath().length()+strlen("maps/%03u%02u%02u.map")+1;
    char* tmp = new char[len];
    snprintf(tmp, len, (char *)(sWorld.GetDataPath()+"maps/%03u%02u%02u.map").c_str(),mapid,x,y);

    FILE *pf=fopen(tmp,"rb");

    if(!pf)
    {
        sLog.outError("Check existing of map file '%s': not exist!",tmp);
        delete[] tmp;
        return false;
    }

    map_fileheader header;
    fread(&header, sizeof(header), 1, pf);
    if (header.mapMagic != uint32(MAP_MAGIC) ||
        header.versionMagic != uint32(MAP_VERSION_MAGIC))
    {
        sLog.outError("Map file '%s' is non-compatible version (outdated?). Please, create new using ad.exe program.",tmp);
        delete [] tmp;
        fclose(pf);                                         //close file before return
        return false;
    }

    delete [] tmp;
    fclose(pf);
    return true;
}

bool Map::ExistVMap(uint32 mapid,int x,int y)
{
    if(VMAP::IVMapManager* vmgr = VMAP::VMapFactory::createOrGetVMapManager())
    {
        if(vmgr->isMapLoadingEnabled())
        {
                                                            // x and y are swapped !! => fixed now
            bool exists = vmgr->existsMap((sWorld.GetDataPath()+ "vmaps").c_str(),  mapid, x,y);
            if(!exists)
            {
                std::string name = vmgr->getDirFileName(mapid,x,y);
                sLog.outError("VMap file '%s' is missing or point to wrong version vmap file, redo vmaps with latest vmap_assembler.exe program", (sWorld.GetDataPath()+"vmaps/"+name).c_str());
                return false;
            }
        }
    }

    return true;
}

void Map::LoadVMap(int x,int y)
{
                                                            // x and y are swapped !!
    int vmapLoadResult = VMAP::VMapFactory::createOrGetVMapManager()->loadMap((sWorld.GetDataPath()+ "vmaps").c_str(),  GetId(), x,y);
    switch(vmapLoadResult)
    {
        case VMAP::VMAP_LOAD_RESULT_OK:
            sLog.outDetail("VMAP loaded name:%s, id:%d, x:%d, y:%d (vmap rep.: x:%d, y:%d)", GetMapName(), GetId(), x,y, x,y);
            break;
        case VMAP::VMAP_LOAD_RESULT_ERROR:
            sLog.outDetail("Could not load VMAP name:%s, id:%d, x:%d, y:%d (vmap rep.: x:%d, y:%d)", GetMapName(), GetId(), x,y, x,y);
            break;
        case VMAP::VMAP_LOAD_RESULT_IGNORED:
            DEBUG_LOG("Ignored VMAP name:%s, id:%d, x:%d, y:%d (vmap rep.: x:%d, y:%d)", GetMapName(), GetId(), x,y, x,y);
            break;
    }
}

void Map::LoadMap(uint32 mapid, uint32 instanceid, int x,int y)
{
    if( instanceid != 0)
    {
        if(GridMaps[x][y])
            return;

        Map* baseMap = const_cast<Map*>(MapManager::Instance().GetBaseMap(mapid));

        // load gridmap for base map
        if (!baseMap->GridMaps[x][y])
            baseMap->EnsureGridCreated(GridPair(63-x,63-y));

//+++        if (!baseMap->GridMaps[x][y])  don't check for GridMaps[gx][gy], we need the management for vmaps
//            return;

        ((MapInstanced*)(baseMap))->AddGridMapReference(GridPair(x,y));
        GridMaps[x][y] = baseMap->GridMaps[x][y];
        return;
    }

    //map already load, delete it before reloading (Is it necessary? Do we really need the ability the reload maps during runtime?)
    if(GridMaps[x][y])
    {
        sLog.outDetail("Unloading already loaded map %u before reloading.",mapid);
        delete (GridMaps[x][y]);
        GridMaps[x][y]=NULL;
    }

    // map file name
    char *tmp=NULL;
    // Pihhan: dataPath length + "maps/" + 3+2+2+ ".map" length may be > 32 !
    int len = sWorld.GetDataPath().length()+strlen("maps/%03u%02u%02u.map")+1;
    tmp = new char[len];
    snprintf(tmp, len, (char *)(sWorld.GetDataPath()+"maps/%03u%02u%02u.map").c_str(),mapid,x,y);
    sLog.outDetail("Loading map %s",tmp);
    // loading data
    GridMaps[x][y] = new GridMap();
    if (!GridMaps[x][y]->loadData(tmp))
    {
        sLog.outError("Error loading map file: \n %s\n", tmp);
    }
    delete [] tmp;
}

void Map::LoadMapAndVMap(uint32 mapid, uint32 instanceid, int x,int y)
{
    LoadMap(mapid,instanceid,x,y);
    if(instanceid == 0) {
        LoadVMap(x, y);                                     // Only load the data for the base map
        // load navmesh
        MMAP::MMapFactory::createOrGetMMapManager()->loadMap(mapid, x, y);
    }
}

void Map::InitStateMachine()
{
    si_GridStates[GRID_STATE_INVALID] = new InvalidState;
    si_GridStates[GRID_STATE_ACTIVE] = new ActiveState;
    si_GridStates[GRID_STATE_IDLE] = new IdleState;
    si_GridStates[GRID_STATE_REMOVAL] = new RemovalState;
}

void Map::DeleteStateMachine()
{
    delete si_GridStates[GRID_STATE_INVALID];
    delete si_GridStates[GRID_STATE_ACTIVE];
    delete si_GridStates[GRID_STATE_IDLE];
    delete si_GridStates[GRID_STATE_REMOVAL];
}

Map::Map(uint32 id, time_t expiry, uint32 InstanceId, uint8 SpawnMode)
   : i_mapEntry (sMapStore.LookupEntry(id)), i_spawnMode(SpawnMode),
   i_id(id), i_InstanceId(InstanceId), m_unloadTimer(0), i_gridExpiry(expiry),
   m_VisibleDistance(DEFAULT_VISIBILITY_DISTANCE),
   m_activeNonPlayersIter(m_activeNonPlayers.end())
   , i_lock(true)
{
    for(unsigned int idx=0; idx < MAX_NUMBER_OF_GRIDS; ++idx)
    {
        for(unsigned int j=0; j < MAX_NUMBER_OF_GRIDS; ++j)
        {
            //z code
            GridMaps[idx][j] =NULL;
            setNGrid(NULL, idx, j);
        }
    }

    Map::InitVisibilityDistance();
}

void Map::InitVisibilityDistance()
{
    //init visibility for continents
    m_VisibleDistance = sWorld.GetMaxVisibleDistanceOnContinents();
}

// Template specialization of utility methods
template<class T>
void Map::AddToGrid(T* obj, NGridType *grid, Cell const& cell)
{
    (*grid)(cell.CellX(), cell.CellY()).template AddGridObject<T>(obj);
}

template<>
void Map::AddToGrid(Player* obj, NGridType *grid, Cell const& cell)
{
    (*grid)(cell.CellX(), cell.CellY()).AddWorldObject(obj);
}

template<>
void Map::AddToGrid(Corpse *obj, NGridType *grid, Cell const& cell)
{
    // add to world object registry in grid
    if(obj->GetType()!=CORPSE_BONES)
    {
        (*grid)(cell.CellX(), cell.CellY()).AddWorldObject(obj);
    }
    // add to grid object store
    else
    {
        (*grid)(cell.CellX(), cell.CellY()).AddGridObject(obj);
    }
}

template<>
void Map::AddToGrid(Creature* obj, NGridType *grid, Cell const& cell)
{
    // add to world object registry in grid
    if(obj->isPet() || obj->IsTempWorldObject)
    {
        (*grid)(cell.CellX(), cell.CellY()).AddWorldObject<Creature>(obj);
    }
    // add to grid object store
    else
    {
        (*grid)(cell.CellX(), cell.CellY()).AddGridObject<Creature>(obj);
    }
    obj->SetCurrentCell(cell);
}

template<>
void Map::AddToGrid(DynamicObject* obj, NGridType *grid, Cell const& cell)
{
    if(obj->isActiveObject()) // only farsight
        (*grid)(cell.CellX(), cell.CellY()).AddWorldObject<DynamicObject>(obj);
    else
        (*grid)(cell.CellX(), cell.CellY()).AddGridObject<DynamicObject>(obj);
}

template<class T>
void Map::RemoveFromGrid(T* obj, NGridType *grid, Cell const& cell)
{
    (*grid)(cell.CellX(), cell.CellY()).template RemoveGridObject<T>(obj);
}

template<>
void Map::RemoveFromGrid(Player* obj, NGridType *grid, Cell const& cell)
{
    (*grid)(cell.CellX(), cell.CellY()).RemoveWorldObject(obj);
}

template<>
void Map::RemoveFromGrid(Corpse *obj, NGridType *grid, Cell const& cell)
{
    // remove from world object registry in grid
    if(obj->GetType()!=CORPSE_BONES)
    {
        (*grid)(cell.CellX(), cell.CellY()).RemoveWorldObject(obj);
    }
    // remove from grid object store
    else
    {
        (*grid)(cell.CellX(), cell.CellY()).RemoveGridObject(obj);
    }
}

template<>
void Map::RemoveFromGrid(Creature* obj, NGridType *grid, Cell const& cell)
{
    // remove from world object registry in grid
    if(obj->isPet() || obj->IsTempWorldObject)
    {
        (*grid)(cell.CellX(), cell.CellY()).RemoveWorldObject<Creature>(obj);
    }
    // remove from grid object store
    else
    {
        (*grid)(cell.CellX(), cell.CellY()).RemoveGridObject<Creature>(obj);
    }
}

template<>
void Map::RemoveFromGrid(DynamicObject* obj, NGridType *grid, Cell const& cell)
{
    if(obj->isActiveObject()) // only farsight
        (*grid)(cell.CellX(), cell.CellY()).RemoveWorldObject<DynamicObject>(obj);
    else
        (*grid)(cell.CellX(), cell.CellY()).RemoveGridObject<DynamicObject>(obj);
}

template<class T>
void Map::SwitchGridContainers(T* obj, bool on)
{
    CellPair p = Trinity::ComputeCellPair(obj->GetPositionX(), obj->GetPositionY());
    if(p.x_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP || p.y_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP)
    {
        sLog.outError("Map::SwitchGridContainers: Object " I64FMT " have invalid coordinates X:%f Y:%f grid cell [%u:%u]", obj->GetGUID(), obj->GetPositionX(), obj->GetPositionY(), p.x_coord, p.y_coord);
        return;
    }

    Cell cell(p);
    if( !loaded(GridPair(cell.data.Part.grid_x, cell.data.Part.grid_y)))
        return;

    DEBUG_LOG("Switch object " I64FMT " from grid[%u,%u] %u", obj->GetGUID(), cell.data.Part.grid_x, cell.data.Part.grid_y, on);
    NGridType *ngrid = getNGrid(cell.GridX(), cell.GridY());
    assert( ngrid != NULL);

    GridType &grid = (*ngrid)(cell.CellX(), cell.CellY());

    if(on)
    {
        grid.RemoveGridObject<T>(obj);
        grid.AddWorldObject<T>(obj);
    }
    else
    {
        grid.RemoveWorldObject<T>(obj);
        grid.AddGridObject<T>(obj);
    }
    obj->IsTempWorldObject = on;
}

template void Map::SwitchGridContainers(Creature *, bool);
template void Map::SwitchGridContainers(DynamicObject *, bool);

template<class T>
void Map::DeleteFromWorld(T* obj)
{
    // Note: In case resurrectable corpse and pet its removed from global lists in own destructor
    delete obj;
}

template<class T>
void Map::AddNotifier(T*)
{
}

template<>
void Map::AddNotifier(Player* obj)
{
    obj->m_Notified = false;
    obj->m_IsInNotifyList = false;
    AddUnitToNotify(obj);
}

template<>
void Map::AddNotifier(Creature* obj)
{
    obj->m_Notified = false;
    obj->m_IsInNotifyList = false;
    AddUnitToNotify(obj);
}

void
Map::EnsureGridCreated(const GridPair &p)
{
    if(!getNGrid(p.x_coord, p.y_coord))
    {
        Guard guard(*this);
        if(!getNGrid(p.x_coord, p.y_coord))
        {
            setNGrid(new NGridType(p.x_coord*MAX_NUMBER_OF_GRIDS + p.y_coord, p.x_coord, p.y_coord, i_gridExpiry, sWorld.getConfig(CONFIG_GRID_UNLOAD)),
                p.x_coord, p.y_coord);

            // build a linkage between this map and NGridType
            buildNGridLinkage(getNGrid(p.x_coord, p.y_coord));

            getNGrid(p.x_coord, p.y_coord)->SetGridState(GRID_STATE_IDLE);

            //z coord
            int gx=63-p.x_coord;
            int gy=63-p.y_coord;

            if(!GridMaps[gx][gy])
                Map::LoadMapAndVMap(i_id,i_InstanceId,gx,gy);
        }
    }
}

void
Map::EnsureGridLoaded(const Cell &cell, Player *player)
{
    EnsureGridCreated(GridPair(cell.GridX(), cell.GridY()));
    NGridType *grid = getNGrid(cell.GridX(), cell.GridY());

    assert(grid != NULL);
    if (!isGridObjectDataLoaded(cell.GridX(), cell.GridY()))
    {
        if (player)
        {
            player->SendDelayResponse(MAX_GRID_LOAD_TIME);
            DEBUG_LOG("Player %s enter cell[%u,%u] triggers of loading grid[%u,%u] on map %u", player->GetName(), cell.CellX(), cell.CellY(), cell.GridX(), cell.GridY(), i_id);
        }
        else
        {
            DEBUG_LOG("Active object nearby triggers of loading grid [%u,%u] on map %u", cell.GridX(), cell.GridY(), i_id);
        }

        setGridObjectDataLoaded(true, cell.GridX(), cell.GridY());
        ObjectGridLoader loader(*grid, this, cell);
        loader.LoadN();

        // Add resurrectable corpses to world object list in grid
        ObjectAccessor::Instance().AddCorpsesToGrid(GridPair(cell.GridX(),cell.GridY()),(*grid)(cell.CellX(), cell.CellY()), this);
        Balance();

        ResetGridExpiry(*getNGrid(cell.GridX(), cell.GridY()), 0.1f);
        grid->SetGridState(GRID_STATE_ACTIVE);
    }
    
    if(player)
        AddToGrid(player,grid,cell);
}

void Map::LoadGrid(float x, float y)
{
    CellPair pair = Trinity::ComputeCellPair(x, y);
    Cell cell(pair);
    EnsureGridLoaded(cell);
}

bool Map::Add(Player *player)
{
    player->GetMapRef().link(this, player);

    player->SetInstanceId(GetInstanceId());

    // update player state for other player and visa-versa
    CellPair p = Trinity::ComputeCellPair(player->GetPositionX(), player->GetPositionY());
    Cell cell(p);
    EnsureGridLoaded(cell, player);
    player->AddToWorld();

    SendInitSelf(player);
    SendInitTransports(player);

    player->m_clientGUIDs.clear();
    //AddNotifier(player);

    return true;
}

template<class T>
void
Map::Add(T *obj)
{
    CellPair p = Trinity::ComputeCellPair(obj->GetPositionX(), obj->GetPositionY());

    assert(obj);

    if(p.x_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP || p.y_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP)
    {
        sLog.outError("Map::Add: Object " I64FMTD " have invalid coordinates X:%f Y:%f grid cell [%u:%u]", obj->GetGUID(), obj->GetPositionX(), obj->GetPositionY(), p.x_coord, p.y_coord);
        return;
    }

    Cell cell(p);
    if(obj->isActiveObject())
        EnsureGridLoaded(cell);
    else
        EnsureGridCreated(GridPair(cell.GridX(), cell.GridY()));

    NGridType *grid = getNGrid(cell.GridX(), cell.GridY());
    assert( grid != NULL);

    AddToGrid(obj,grid,cell);
    obj->AddToWorld();
    
    if(obj->isActiveObject())
        AddToActive(obj);

    DEBUG_LOG("Object %u enters grid[%u,%u]", GUID_LOPART(obj->GetGUID()), cell.GridX(), cell.GridY());

    UpdateObjectVisibility(obj,cell,p);

    //AddNotifier(obj);
}

void Map::MessageBroadcast(Player *player, WorldPacket *msg, bool to_self, bool to_possessor)
{
    CellPair p = Trinity::ComputeCellPair(player->GetPositionX(), player->GetPositionY());

    if(p.x_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP || p.y_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP)
    {
        sLog.outError("Map::MessageBroadcast: Player (GUID: %u) have invalid coordinates X:%f Y:%f grid cell [%u:%u]", player->GetGUIDLow(), player->GetPositionX(), player->GetPositionY(), p.x_coord, p.y_coord);
        return;
    }

    Cell cell(p);
    cell.data.Part.reserved = ALL_DISTRICT;

    if( !loaded(GridPair(cell.data.Part.grid_x, cell.data.Part.grid_y)))
        return;

    Trinity::MessageDeliverer post_man(*player, msg, to_possessor, to_self);
    TypeContainerVisitor<Trinity::MessageDeliverer, WorldTypeMapContainer > message(post_man);
    cell.Visit(p, message, *this, *player, GetVisibilityDistance());
}

void Map::MessageBroadcast(WorldObject *obj, WorldPacket *msg, bool to_possessor)
{
    CellPair p = Trinity::ComputeCellPair(obj->GetPositionX(), obj->GetPositionY());

    if(p.x_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP || p.y_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP)
    {
        sLog.outError("Map::MessageBroadcast: Object " I64FMTD " have invalid coordinates X:%f Y:%f grid cell [%u:%u]", obj->GetGUID(), obj->GetPositionX(), obj->GetPositionY(), p.x_coord, p.y_coord);
        return;
    }

    Cell cell(p);
    cell.data.Part.reserved = ALL_DISTRICT;
    cell.SetNoCreate();

    if( !loaded(GridPair(cell.data.Part.grid_x, cell.data.Part.grid_y)))
        return;

    Trinity::ObjectMessageDeliverer post_man(*obj, msg, to_possessor);
    TypeContainerVisitor<Trinity::ObjectMessageDeliverer, WorldTypeMapContainer > message(post_man);
    cell.Visit(p, message, *this, *obj, GetVisibilityDistance());
}

void Map::MessageDistBroadcast(Player *player, WorldPacket *msg, float dist, bool to_self, bool to_possessor, bool own_team_only)
{
    CellPair p = Trinity::ComputeCellPair(player->GetPositionX(), player->GetPositionY());

    if(p.x_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP || p.y_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP)
    {
        sLog.outError("Map::MessageBroadcast: Player (GUID: %u) have invalid coordinates X:%f Y:%f grid cell [%u:%u]", player->GetGUIDLow(), player->GetPositionX(), player->GetPositionY(), p.x_coord, p.y_coord);
        return;
    }

    Cell cell(p);
    cell.data.Part.reserved = ALL_DISTRICT;

    if( !loaded(GridPair(cell.data.Part.grid_x, cell.data.Part.grid_y)))
        return;

    Trinity::MessageDistDeliverer post_man(*player, msg, to_possessor, dist, to_self, own_team_only);
    TypeContainerVisitor<Trinity::MessageDistDeliverer , WorldTypeMapContainer > message(post_man);
    cell.Visit(p, message, *this, *player, dist);
}

void Map::MessageDistBroadcast(WorldObject *obj, WorldPacket *msg, float dist, bool to_possessor)
{
    CellPair p = Trinity::ComputeCellPair(obj->GetPositionX(), obj->GetPositionY());

    if(p.x_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP || p.y_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP)
    {
        sLog.outError("Map::MessageBroadcast: Object " I64FMTD " have invalid coordinates X:%f Y:%f grid cell [%u:%u]", obj->GetGUID(), obj->GetPositionX(), obj->GetPositionY(), p.x_coord, p.y_coord);
        return;
    }

    Cell cell(p);
    cell.data.Part.reserved = ALL_DISTRICT;
    cell.SetNoCreate();

    if( !loaded(GridPair(cell.data.Part.grid_x, cell.data.Part.grid_y)))
        return;

    Trinity::ObjectMessageDistDeliverer post_man(*obj, msg, to_possessor, dist);
    TypeContainerVisitor<Trinity::ObjectMessageDistDeliverer, WorldTypeMapContainer > message(post_man);
    cell.Visit(p, message, *this, *obj, dist);
}

bool Map::loaded(const GridPair &p) const
{
    return ( getNGrid(p.x_coord, p.y_coord) && isGridObjectDataLoaded(p.x_coord, p.y_coord));
}

void Map::RelocationNotify()
{
    //Move backlog to notify list
    for(std::vector<uint64>::iterator iter = i_unitsToNotifyBacklog.begin(); iter != i_unitsToNotifyBacklog.end(); ++iter)
    {
        if(Unit *unit = ObjectAccessor::GetObjectInWorld(*iter, (Unit*)NULL))
        {
            i_unitsToNotify.push_back(unit);
        }
    }
    i_unitsToNotifyBacklog.clear();

    //Notify
    for(std::vector<Unit*>::iterator iter = i_unitsToNotify.begin(); iter != i_unitsToNotify.end(); ++iter)
    {
        Unit *unit = *iter;
        if(unit->m_Notified || !unit->IsInWorld() || unit->GetMapId() != GetId())
            continue;

        unit->m_Notified = true;
        unit->m_IsInNotifyList = false;

        float dist = abs(unit->GetPositionX() - unit->oldX) + abs(unit->GetPositionY() - unit->oldY);
        if(dist > 10.0f)
        {
            Trinity::VisibleChangesNotifier notifier(*unit);
            VisitWorld(unit->oldX, unit->oldY, GetVisibilityDistance(), notifier);
            dist = 0;
        }

        if(unit->GetTypeId() == TYPEID_PLAYER)
        {
            Trinity::PlayerRelocationNotifier notifier(*(unit->ToPlayer()));
            VisitAll(unit->GetPositionX(), unit->GetPositionY(), unit->GetMap()->GetVisibilityDistance() + dist, notifier);
            notifier.Notify();
        }
        else
        {
            Trinity::CreatureRelocationNotifier notifier(*(unit->ToCreature()));
            VisitAll(unit->GetPositionX(), unit->GetPositionY(), unit->GetMap()->GetVisibilityDistance() + dist, notifier);
        }
    }
    for(std::vector<Unit*>::iterator iter = i_unitsToNotify.begin(); iter != i_unitsToNotify.end(); ++iter)
    {
        (*iter)->m_Notified = false;
    }
    i_unitsToNotify.clear();
}

void Map::AddUnitToNotify(Unit* u)
{
    if(u->m_IsInNotifyList)
        return;

    u->m_IsInNotifyList = true;
    u->oldX = u->GetPositionX();
    u->oldY = u->GetPositionY();

    if(i_lock)
        i_unitsToNotifyBacklog.push_back(u->GetGUID());
    else
        i_unitsToNotify.push_back(u);
}

void Map::Update(const uint32 &t_diff)
{
    i_lock = false;
    
    _dynamicTree.update(t_diff);
    /// update worldsessions for existing players
    for(m_mapRefIter = m_mapRefManager.begin(); m_mapRefIter != m_mapRefManager.end(); ++m_mapRefIter)
    {
        Player* plr = m_mapRefIter->getSource();
        if(plr && plr->IsInWorld())
        {
            //plr->Update(t_diff);
            WorldSession * pSession = plr->GetSession();
            MapSessionFilter updater(pSession);

            pSession->Update(t_diff, updater);
        }
    }

    resetMarkedCells();

    Trinity::ObjectUpdater updater(t_diff);
    // for creature
    TypeContainerVisitor<Trinity::ObjectUpdater, GridTypeMapContainer  > grid_object_update(updater);
    // for pets
    TypeContainerVisitor<Trinity::ObjectUpdater, WorldTypeMapContainer > world_object_update(updater);

    // the player iterator is stored in the map object
    // to make sure calls to Map::Remove don't invalidate it
    for(m_mapRefIter = m_mapRefManager.begin(); m_mapRefIter != m_mapRefManager.end(); ++m_mapRefIter)
    {
        Player* plr = m_mapRefIter->getSource();

        if(!plr->IsInWorld())
            continue;

        CellPair standing_cell(Trinity::ComputeCellPair(plr->GetPositionX(), plr->GetPositionY()));

        // Check for correctness of standing_cell, it also avoids problems with update_cell
        if (standing_cell.x_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP || standing_cell.y_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP)
            continue;

        // the overloaded operators handle range checking
        // so ther's no need for range checking inside the loop
        CellPair begin_cell(standing_cell), end_cell(standing_cell);
        CellArea area = Cell::CalculateCellArea(*plr, GetVisibilityDistance());
        area.ResizeBorders(begin_cell, end_cell);

        for(uint32 x = begin_cell.x_coord; x <= end_cell.x_coord; ++x)
        {
            for(uint32 y = begin_cell.y_coord; y <= end_cell.y_coord; ++y)
            {
                // marked cells are those that have been visited
                // don't visit the same cell twice
                uint32 cell_id = (y * TOTAL_NUMBER_OF_CELLS_PER_MAP) + x;
                if(!isCellMarked(cell_id))
                {
                    markCell(cell_id);
                    CellPair pair(x,y);
                    Cell cell(pair);
                    cell.data.Part.reserved = CENTER_DISTRICT;
                    //cell.SetNoCreate();
                    cell.Visit(pair, grid_object_update,  *this);
                    cell.Visit(pair, world_object_update, *this);
                }
            }
        }
    }

    // non-player active objects
    if(!m_activeNonPlayers.empty())
    {
        for(m_activeNonPlayersIter = m_activeNonPlayers.begin(); m_activeNonPlayersIter != m_activeNonPlayers.end();)
        {
            // skip not in world
            WorldObject* obj = *m_activeNonPlayersIter;

            // step before processing, in this case if Map::Remove remove next object we correctly
            // step to next-next, and if we step to end() then newly added objects can wait next update.
            ++m_activeNonPlayersIter;

            if(!obj->IsInWorld())
                continue;

            // Update bindsight players
            if(obj->isType(TYPEMASK_UNIT))
            {
                if(!((Unit*)obj)->GetSharedVisionList().empty()) {
                    for(SharedVisionList::const_iterator itr = ((Unit*)obj)->GetSharedVisionList().begin(); itr != ((Unit*)obj)->GetSharedVisionList().end(); ++itr)
                    {
                        if(!*itr)
                        {
                            sLog.outError("unit %u has invalid shared vision player, list size %u", obj->GetEntry(), ((Unit*)obj)->GetSharedVisionList().size());
                            continue;
                        }
                        Trinity::PlayerVisibilityNotifier notifier(**itr);
                        VisitAll(obj->GetPositionX(), obj->GetPositionY(), World::GetMaxVisibleDistanceForObject(), notifier);
                        notifier.Notify();
                    }
                }
            }
            else if(obj->GetTypeId() == TYPEID_DYNAMICOBJECT)
            {
                if(Unit *caster = ((DynamicObject*)obj)->GetCaster())
                    if(caster->GetTypeId() == TYPEID_PLAYER && caster->GetUInt64Value(PLAYER_FARSIGHT) == obj->GetGUID())
                    {
                        Trinity::PlayerVisibilityNotifier notifier(*(caster->ToPlayer()));
                        VisitAll(obj->GetPositionX(), obj->GetPositionY(), GetVisibilityDistance(), notifier);
                        notifier.Notify();
                    }
            }

            CellPair standing_cell(Trinity::ComputeCellPair(obj->GetPositionX(), obj->GetPositionY()));

            // Check for correctness of standing_cell, it also avoids problems with update_cell
            if (standing_cell.x_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP || standing_cell.y_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP)
                continue;

            // the overloaded operators handle range checking
            // so ther's no need for range checking inside the loop
            CellPair begin_cell(standing_cell), end_cell(standing_cell);
            begin_cell << 1; begin_cell -= 1;               // upper left
            end_cell >> 1; end_cell += 1;                   // lower right

            for(uint32 x = begin_cell.x_coord; x <= end_cell.x_coord; ++x)
            {
                for(uint32 y = begin_cell.y_coord; y <= end_cell.y_coord; ++y)
                {
                    // marked cells are those that have been visited
                    // don't visit the same cell twice
                    uint32 cell_id = (y * TOTAL_NUMBER_OF_CELLS_PER_MAP) + x;
                    if(!isCellMarked(cell_id))
                    {
                        markCell(cell_id);
                        CellPair pair(x,y);
                        Cell cell(pair);
                        cell.data.Part.reserved = CENTER_DISTRICT;
                        //cell.SetNoCreate();
                        cell.Visit(pair, grid_object_update,  *this);
                        cell.Visit(pair, world_object_update, *this);
                    }
                }
            }
        }
    }

    i_lock = true;

    MoveAllCreaturesInMoveList();
    RelocationNotify();
    RemoveAllObjectsInRemoveList();

    // Don't unload grids if it's battleground, since we may have manually added GOs,creatures, those doesn't load from DB at grid re-load !
    // This isn't really bother us, since as soon as we have instanced BG-s, the whole map unloads as the BG gets ended
    if (IsBattleGroundOrArena())
        return;

    for (GridRefManager<NGridType>::iterator i = GridRefManager<NGridType>::begin(); i != GridRefManager<NGridType>::end();)
    {
        NGridType *grid = i->getSource();
        GridInfo *info = i->getSource()->getGridInfoRef();
        ++i;                                                // The update might delete the map and we need the next map before the iterator gets invalid
        assert(grid->GetGridState() >= 0 && grid->GetGridState() < MAX_GRID_STATE);
        si_GridStates[grid->GetGridState()]->Update(*this, *grid, *info, grid->getX(), grid->getY(), t_diff);
    }
}

void Map::Remove(Player *player, bool remove)
{
    // this may be called during Map::Update
    // after decrement+unlink, ++m_mapRefIter will continue correctly
    // when the first element of the list is being removed
    // nocheck_prev will return the padding element of the RefManager
    // instead of NULL in the case of prev
    if(m_mapRefIter == player->GetMapRef())
        m_mapRefIter = m_mapRefIter->nocheck_prev();
    player->GetMapRef().unlink();
    CellPair p = Trinity::ComputeCellPair(player->GetPositionX(), player->GetPositionY());
    if(p.x_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP || p.y_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP)
    {
        // invalid coordinates
        player->RemoveFromWorld();

        if( remove)
            DeleteFromWorld(player);

        return;
    }

    Cell cell(p);

    if( !getNGrid(cell.data.Part.grid_x, cell.data.Part.grid_y))
    {
        sLog.outError("Map::Remove() i_grids was NULL x:%d, y:%d",cell.data.Part.grid_x,cell.data.Part.grid_y);
        return;
    }

    DEBUG_LOG("Remove player %s from grid[%u,%u]", player->GetName(), cell.GridX(), cell.GridY());
    NGridType *grid = getNGrid(cell.GridX(), cell.GridY());
    assert(grid != NULL);

    player->RemoveFromWorld();
    RemoveFromGrid(player,grid,cell);

    SendRemoveTransports(player);
    UpdateObjectVisibility(player,cell,p);

    if( remove)
        DeleteFromWorld(player);
}

bool Map::RemoveBones(uint64 guid, float x, float y)
{
    if (IsRemovalGrid(x, y))
    {
        Corpse * corpse = ObjectAccessor::Instance().GetObjectInWorld(GetId(), x, y, guid, (Corpse*)NULL);
        if(corpse && corpse->GetTypeId() == TYPEID_CORPSE && corpse->GetType() == CORPSE_BONES)
            corpse->DeleteBonesFromWorld();
        else
            return false;
    }
    return true;
}

template<class T>
void
Map::Remove(T *obj, bool remove)
{
    CellPair p = Trinity::ComputeCellPair(obj->GetPositionX(), obj->GetPositionY());
    if(p.x_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP || p.y_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP)
    {
        sLog.outError("Map::Remove: Object " I64FMT " have invalid coordinates X:%f Y:%f grid cell [%u:%u]", obj->GetGUID(), obj->GetPositionX(), obj->GetPositionY(), p.x_coord, p.y_coord);
        return;
    }

    Cell cell(p);
    if( !loaded(GridPair(cell.data.Part.grid_x, cell.data.Part.grid_y)))
        return;

    DEBUG_LOG("Remove object " I64FMT " from grid[%u,%u]", obj->GetGUID(), cell.data.Part.grid_x, cell.data.Part.grid_y);
    NGridType *grid = getNGrid(cell.GridX(), cell.GridY());
    assert( grid != NULL);

    obj->RemoveFromWorld();
    if(obj->isActiveObject())
        RemoveFromActive(obj);
    RemoveFromGrid(obj,grid,cell);

    UpdateObjectVisibility(obj,cell,p);

    if( remove)
    {
        // if option set then object already saved at this moment
        if(!sWorld.getConfig(CONFIG_SAVE_RESPAWN_TIME_IMMEDIATELY))
            obj->SaveRespawnTime();
        DeleteFromWorld(obj);
    }
}

void
Map::PlayerRelocation(Player *player, float x, float y, float z, float orientation)
{
    assert(player);

    CellPair old_val = Trinity::ComputeCellPair(player->GetPositionX(), player->GetPositionY());
    CellPair new_val = Trinity::ComputeCellPair(x, y);

    Cell old_cell(old_val);
    Cell new_cell(new_val);
    new_cell |= old_cell;
    bool same_cell = (new_cell == old_cell);

    player->Relocate(x, y, z, orientation);

    if( old_cell.DiffGrid(new_cell) || old_cell.DiffCell(new_cell))
    {
        DEBUG_LOG("Player %s relocation grid[%u,%u]cell[%u,%u]->grid[%u,%u]cell[%u,%u]", player->GetName(), old_cell.GridX(), old_cell.GridY(), old_cell.CellX(), old_cell.CellY(), new_cell.GridX(), new_cell.GridY(), new_cell.CellX(), new_cell.CellY());

        // update player position for group at taxi flight
        if(player->GetGroup() && player->isInFlight())
            player->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_POSITION);

        NGridType* oldGrid = getNGrid(old_cell.GridX(), old_cell.GridY());
        RemoveFromGrid(player, oldGrid,old_cell);
        if( !old_cell.DiffGrid(new_cell))
            AddToGrid(player, oldGrid,new_cell);
        else
            EnsureGridLoaded(new_cell, player);
    }

    AddUnitToNotify(player);

    NGridType* newGrid = getNGrid(new_cell.GridX(), new_cell.GridY());
    if( !same_cell && newGrid->GetGridState()!= GRID_STATE_ACTIVE)
    {
        ResetGridExpiry(*newGrid, 0.1f);
        newGrid->SetGridState(GRID_STATE_ACTIVE);
    }
}

void
Map::CreatureRelocation(Creature *creature, float x, float y, float z, float ang)
{
    assert(CheckGridIntegrity(creature,false));

    Cell old_cell = creature->GetCurrentCell();

    CellPair new_val = Trinity::ComputeCellPair(x, y);
    Cell new_cell(new_val);

    // delay creature move for grid/cell to grid/cell moves
    if( old_cell.DiffCell(new_cell) || old_cell.DiffGrid(new_cell))
    {
        #ifdef TRINITY_DEBUG
        if((sLog.getLogFilter() & LOG_FILTER_CREATURE_MOVES)==0)
            sLog.outDebug("Creature (GUID: %u Entry: %u) added to moving list from grid[%u,%u]cell[%u,%u] to grid[%u,%u]cell[%u,%u].", creature->GetGUIDLow(), creature->GetEntry(), old_cell.GridX(), old_cell.GridY(), old_cell.CellX(), old_cell.CellY(), new_cell.GridX(), new_cell.GridY(), new_cell.CellX(), new_cell.CellY());
        #endif
        AddCreatureToMoveList(creature,x,y,z,ang);
        // in diffcell/diffgrid case notifiers called at finishing move creature in Map::MoveAllCreaturesInMoveList
    }
    else
    {
        creature->Relocate(x, y, z, ang);
        AddUnitToNotify(creature);
    }
    assert(CheckGridIntegrity(creature,true));
}

void Map::AddCreatureToMoveList(Creature *c, float x, float y, float z, float ang)
{
    if(!c)
        return;

    i_creaturesToMove[c] = CreatureMover(x,y,z,ang);
}

void Map::MoveAllCreaturesInMoveList()
{
    while(!i_creaturesToMove.empty())
    {
        // get data and remove element;
        CreatureMoveList::iterator iter = i_creaturesToMove.begin();
        Creature* c = iter->first;
        CreatureMover cm = iter->second;
        i_creaturesToMove.erase(iter);

        // calculate cells
        CellPair new_val = Trinity::ComputeCellPair(cm.x, cm.y);
        Cell new_cell(new_val);

        // do move or do move to respawn or remove creature if previous all fail
        if(CreatureCellRelocation(c,new_cell))
        {
            // update pos
            c->Relocate(cm.x, cm.y, cm.z, cm.ang);
            //CreatureRelocationNotify(c,new_cell,new_cell.cellPair());
            AddUnitToNotify(c);
        }
        else
        {
            // if creature can't be move in new cell/grid (not loaded) move it to repawn cell/grid
            // creature coordinates will be updated and notifiers send
            if(!CreatureRespawnRelocation(c))
            {
                // ... or unload (if respawn grid also not loaded)
                #ifdef TRINITY_DEBUG
                if((sLog.getLogFilter() & LOG_FILTER_CREATURE_MOVES)==0)
                    sLog.outDebug("Creature (GUID: %u Entry: %u) can't be move to unloaded respawn grid.",c->GetGUIDLow(),c->GetEntry());
                #endif
                c->CleanupsBeforeDelete();
                AddObjectToRemoveList(c);
            }
        }
    }
}

bool Map::CreatureCellRelocation(Creature *c, Cell new_cell)
{
    Cell const& old_cell = c->GetCurrentCell();
    if(!old_cell.DiffGrid(new_cell))                       // in same grid
    {
        // if in same cell then none do
        if(old_cell.DiffCell(new_cell))
        {
            #ifdef TRINITY_DEBUG
            if((sLog.getLogFilter() & LOG_FILTER_CREATURE_MOVES)==0)
                sLog.outDebug("Creature (GUID: %u Entry: %u) moved in grid[%u,%u] from cell[%u,%u] to cell[%u,%u].", c->GetGUIDLow(), c->GetEntry(), old_cell.GridX(), old_cell.GridY(), old_cell.CellX(), old_cell.CellY(), new_cell.CellX(), new_cell.CellY());
            #endif

            RemoveFromGrid(c,getNGrid(old_cell.GridX(), old_cell.GridY()),old_cell);
            AddToGrid(c,getNGrid(new_cell.GridX(), new_cell.GridY()),new_cell);
        }
        else
        {
            #ifdef TRINITY_DEBUG
            if((sLog.getLogFilter() & LOG_FILTER_CREATURE_MOVES)==0)
                sLog.outDebug("Creature (GUID: %u Entry: %u) move in same grid[%u,%u]cell[%u,%u].", c->GetGUIDLow(), c->GetEntry(), old_cell.GridX(), old_cell.GridY(), old_cell.CellX(), old_cell.CellY());
            #endif
        }

        return true;
    }

    // in diff. grids but active creature
    if(c->isActiveObject())
    {
        EnsureGridLoaded(new_cell);

        #ifdef TRINITY_DEBUG
        if((sLog.getLogFilter() & LOG_FILTER_CREATURE_MOVES)==0)
            sLog.outDebug("Active creature (GUID: %u Entry: %u) moved from grid[%u,%u]cell[%u,%u] to grid[%u,%u]cell[%u,%u].", c->GetGUIDLow(), c->GetEntry(), old_cell.GridX(), old_cell.GridY(), old_cell.CellX(), old_cell.CellY(), new_cell.GridX(), new_cell.GridY(), new_cell.CellX(), new_cell.CellY());
        #endif

        RemoveFromGrid(c,getNGrid(old_cell.GridX(), old_cell.GridY()),old_cell);
        AddToGrid(c,getNGrid(new_cell.GridX(), new_cell.GridY()),new_cell);

        return true;
    }

    // in diff. loaded grid normal creature
    if(loaded(GridPair(new_cell.GridX(), new_cell.GridY())))
    {
        #ifdef TRINITY_DEBUG
        if((sLog.getLogFilter() & LOG_FILTER_CREATURE_MOVES)==0)
            sLog.outDebug("Creature (GUID: %u Entry: %u) moved from grid[%u,%u]cell[%u,%u] to grid[%u,%u]cell[%u,%u].", c->GetGUIDLow(), c->GetEntry(), old_cell.GridX(), old_cell.GridY(), old_cell.CellX(), old_cell.CellY(), new_cell.GridX(), new_cell.GridY(), new_cell.CellX(), new_cell.CellY());
        #endif

        RemoveFromGrid(c,getNGrid(old_cell.GridX(), old_cell.GridY()),old_cell);
        EnsureGridCreated(GridPair(new_cell.GridX(), new_cell.GridY()));
        AddToGrid(c,getNGrid(new_cell.GridX(), new_cell.GridY()),new_cell);

        return true;
    }

    // fail to move: normal creature attempt move to unloaded grid
    #ifdef TRINITY_DEBUG
    if((sLog.getLogFilter() & LOG_FILTER_CREATURE_MOVES)==0)
        sLog.outDebug("Creature (GUID: %u Entry: %u) attempt move from grid[%u,%u]cell[%u,%u] to unloaded grid[%u,%u]cell[%u,%u].", c->GetGUIDLow(), c->GetEntry(), old_cell.GridX(), old_cell.GridY(), old_cell.CellX(), old_cell.CellY(), new_cell.GridX(), new_cell.GridY(), new_cell.CellX(), new_cell.CellY());
    #endif
    return false;
}

bool Map::CreatureRespawnRelocation(Creature *c)
{
    float resp_x, resp_y, resp_z, resp_o;
    c->GetRespawnCoord(resp_x, resp_y, resp_z, &resp_o);

    CellPair resp_val = Trinity::ComputeCellPair(resp_x, resp_y);
    Cell resp_cell(resp_val);

    c->CombatStop();
    c->GetMotionMaster()->Clear();

    #ifdef TRINITY_DEBUG
    if((sLog.getLogFilter() & LOG_FILTER_CREATURE_MOVES)==0)
        sLog.outDebug("Creature (GUID: %u Entry: %u) will moved from grid[%u,%u]cell[%u,%u] to respawn grid[%u,%u]cell[%u,%u].", c->GetGUIDLow(), c->GetEntry(), c->GetCurrentCell().GridX(), c->GetCurrentCell().GridY(), c->GetCurrentCell().CellX(), c->GetCurrentCell().CellY(), resp_cell.GridX(), resp_cell.GridY(), resp_cell.CellX(), resp_cell.CellY());
    #endif

    // teleport it to respawn point (like normal respawn if player see)
    if(CreatureCellRelocation(c,resp_cell))
    {
        c->Relocate(resp_x, resp_y, resp_z, resp_o);
        c->GetMotionMaster()->Initialize();                 // prevent possible problems with default move generators
        //CreatureRelocationNotify(c,resp_cell,resp_cell.cellPair());
        AddUnitToNotify(c);
        return true;
    }
    else
        return false;
}

bool Map::UnloadGrid(const uint32 &x, const uint32 &y, bool unloadAll)
{
    NGridType *grid = getNGrid(x, y);
    assert( grid != NULL);

    {
        if(!unloadAll && ActiveObjectsNearGrid(x, y))
             return false;

        ObjectGridUnloader unloader(*grid);

        if(!unloadAll)
        {
            // Finish creature moves, remove and delete all creatures with delayed remove before moving to respawn grids
            // Must know real mob position before move
            MoveAllCreaturesInMoveList();

            // move creatures to respawn grids if this is diff.grid or to remove list
            unloader.MoveToRespawnN();

            // Finish creature moves, remove and delete all creatures with delayed remove before unload
            MoveAllCreaturesInMoveList();
        }

        ObjectGridCleaner cleaner(*grid);
        cleaner.CleanN();

        RemoveAllObjectsInRemoveList();

        unloader.UnloadN();

        assert(i_objectsToRemove.empty());

        delete grid;
        setNGrid(NULL, x, y);
    }
    int gx=63-x;
    int gy=63-y;

    // delete grid map, but don't delete if it is from parent map (and thus only reference)
    //+++if (GridMaps[gx][gy]) don't check for GridMaps[gx][gy], we might have to unload vmaps
    {
        if (i_InstanceId == 0)
        {
            if (GridMaps[gx][gy])
            {
                GridMaps[gx][gy]->unloadData();
                delete GridMaps[gx][gy];
            }
            VMAP::VMapFactory::createOrGetVMapManager()->unloadMap(GetId(), gx, gy);
            MMAP::MMapFactory::createOrGetMMapManager()->unloadMap(GetId(), gx, gy);
        }
        else
            ((MapInstanced*)(MapManager::Instance().GetBaseMap(i_id)))->RemoveGridMapReference(GridPair(gx, gy));
        GridMaps[gx][gy] = NULL;
    }
    DEBUG_LOG("Unloading grid[%u,%u] for map %u finished", x,y, i_id);
    return true;
}

void Map::UnloadAll()
{
    // clear all delayed moves, useless anyway do this moves before map unload.
    i_creaturesToMove.clear();

    for (GridRefManager<NGridType>::iterator i = GridRefManager<NGridType>::begin(); i != GridRefManager<NGridType>::end();)
    {
        NGridType &grid(*i->getSource());
        ++i;
        UnloadGrid(grid.getX(), grid.getY(), true);       // deletes the grid and removes it from the GridRefManager
    }
}

//*****************************
// Grid function
//*****************************
GridMap::GridMap()
{
    m_flags = 0;
    // Area data
    m_gridArea = 0;
    m_area_map = NULL;
    // Height level data
    m_gridHeight = INVALID_HEIGHT;
    m_gridGetHeight = &GridMap::getHeightFromFlat;
    m_V9 = NULL;
    m_V8 = NULL;
    // Liquid data
    m_liquidType    = 0;
    m_liquid_offX   = 0;
    m_liquid_offY   = 0;
    m_liquid_width  = 0;
    m_liquid_height = 0;
    m_liquidLevel = INVALID_HEIGHT;
    m_liquid_type = NULL;
    m_liquid_map  = NULL;
    m_gridIntHeightMultiplier = 0.0f;
}

GridMap::~GridMap()
{
    unloadData();
}

bool GridMap::loadData(char *filename)
{
    // Unload old data if exist
    unloadData();

    map_fileheader header;
    // Not return error if file not found
    FILE *in = fopen(filename, "rb");
    if (!in)
        return true;
    fread(&header, sizeof(header),1,in);
    if (header.mapMagic == uint32(MAP_MAGIC) &&
        header.versionMagic == uint32(MAP_VERSION_MAGIC))
    {
        // loadup area data
        if (header.areaMapOffset && !loadAreaData(in, header.areaMapOffset, header.areaMapSize))
        {
            sLog.outError("Error loading map area data\n");
            fclose(in);
            return false;
        }
        // loadup height data
        if (header.heightMapOffset && !loadHeightData(in, header.heightMapOffset, header.heightMapSize))
        {
            sLog.outError("Error loading map height data\n");
            fclose(in);
            return false;
        }
        // loadup liquid data
        if (header.liquidMapOffset && !loadLiquidData(in, header.liquidMapOffset, header.liquidMapSize))
        {
            sLog.outError("Error loading map liquids data\n");
            fclose(in);
            return false;
        }
        fclose(in);
        return true;
    }
    sLog.outError("Map file '%s' is a non-compatible version (outdated?). Please, create new using the ad.exe program.", filename);
    fclose(in);
    return false;
}

void GridMap::unloadData()
{
    delete[] m_area_map;
    delete[] m_V9;
    delete[] m_V8;
    delete[] m_liquid_type;
    delete[] m_liquid_map;
    m_area_map = NULL;
    m_V9 = NULL;
    m_V8 = NULL;
    m_liquid_type = NULL;
    m_liquid_map  = NULL;
    m_gridGetHeight = &GridMap::getHeightFromFlat;
}

bool GridMap::loadAreaData(FILE *in, uint32 offset, uint32 /*size*/)
{
    map_areaHeader header;
    fseek(in, offset, SEEK_SET);
    fread(&header, sizeof(header), 1, in);
    if (header.fourcc != uint32(MAP_AREA_MAGIC))
        return false;

    m_gridArea = header.gridArea;
    if (!(header.flags & MAP_AREA_NO_AREA))
    {
        m_area_map = new uint16 [16*16];
        fread(m_area_map, sizeof(uint16), 16*16, in);
    }
    return true;
}

bool  GridMap::loadHeightData(FILE *in, uint32 offset, uint32 /*size*/)
{
    map_heightHeader header;
    fseek(in, offset, SEEK_SET);
    fread(&header, sizeof(header), 1, in);
    if (header.fourcc != uint32(MAP_HEIGHT_MAGIC))
        return false;

    m_gridHeight = header.gridHeight;
    if (!(header.flags & MAP_HEIGHT_NO_HEIGHT))
    {
        if ((header.flags & MAP_HEIGHT_AS_INT16))
        {
            m_uint16_V9 = new uint16 [129*129];
            m_uint16_V8 = new uint16 [128*128];
            fread(m_uint16_V9, sizeof(uint16), 129*129, in);
            fread(m_uint16_V8, sizeof(uint16), 128*128, in);
            m_gridIntHeightMultiplier = (header.gridMaxHeight - header.gridHeight) / 65535;
            m_gridGetHeight = &GridMap::getHeightFromUint16;
        }
        else if ((header.flags & MAP_HEIGHT_AS_INT8))
        {
            m_uint8_V9 = new uint8 [129*129];
            m_uint8_V8 = new uint8 [128*128];
            fread(m_uint8_V9, sizeof(uint8), 129*129, in);
            fread(m_uint8_V8, sizeof(uint8), 128*128, in);
            m_gridIntHeightMultiplier = (header.gridMaxHeight - header.gridHeight) / 255;
            m_gridGetHeight = &GridMap::getHeightFromUint8;
        }
        else
        {
            m_V9 = new float [129*129];
            m_V8 = new float [128*128];
            fread(m_V9, sizeof(float), 129*129, in);
            fread(m_V8, sizeof(float), 128*128, in);
            m_gridGetHeight = &GridMap::getHeightFromFloat;
        }
    }
    else
        m_gridGetHeight = &GridMap::getHeightFromFlat;
    return true;
}

bool  GridMap::loadLiquidData(FILE *in, uint32 offset, uint32 /*size*/)
{
    map_liquidHeader header;
    fseek(in, offset, SEEK_SET);
    fread(&header, sizeof(header), 1, in);
    if (header.fourcc != uint32(MAP_LIQUID_MAGIC))
        return false;

    m_liquidType   = header.liquidType;
    m_liquid_offX  = header.offsetX;
    m_liquid_offY  = header.offsetY;
    m_liquid_width = header.width;
    m_liquid_height= header.height;
    m_liquidLevel  = header.liquidLevel;

    if (!(header.flags & MAP_LIQUID_NO_TYPE))
    {
        m_liquid_type = new uint8 [16*16];
        fread(m_liquid_type, sizeof(uint8), 16*16, in);
    }
    if (!(header.flags & MAP_LIQUID_NO_HEIGHT))
    {
        m_liquid_map = new float [m_liquid_width*m_liquid_height];
        fread(m_liquid_map, sizeof(float), m_liquid_width*m_liquid_height, in);
    }
    return true;
}

uint16 GridMap::getArea(float x, float y)
{
    if (!m_area_map)
        return m_gridArea;

    x = 16 * (32 - x/SIZE_OF_GRIDS);
    y = 16 * (32 - y/SIZE_OF_GRIDS);
    int lx = (int)x & 15;
    int ly = (int)y & 15;
    return m_area_map[lx*16 + ly];
}

float  GridMap::getHeightFromFlat(float /*x*/, float /*y*/) const
{
    return m_gridHeight;
}

float  GridMap::getHeightFromFloat(float x, float y) const
{
    if (!m_V8 || !m_V9)
        return m_gridHeight;

    x = MAP_RESOLUTION * (32 - x/SIZE_OF_GRIDS);
    y = MAP_RESOLUTION * (32 - y/SIZE_OF_GRIDS);

    int x_int = (int)x;
    int y_int = (int)y;
    x -= x_int;
    y -= y_int;
    x_int&=(MAP_RESOLUTION - 1);
    y_int&=(MAP_RESOLUTION - 1);

    // Height stored as: h5 - its v8 grid, h1-h4 - its v9 grid
    // +--------------> X
    // | h1-------h2     Coordinates is:
    // | | \  1  / |     h1 0,0
    // | |  \   /  |     h2 0,1
    // | | 2  h5 3 |     h3 1,0
    // | |  /   \  |     h4 1,1
    // | | /  4  \ |     h5 1/2,1/2
    // | h3-------h4
    // V Y
    // For find height need
    // 1 - detect triangle
    // 2 - solve linear equation from triangle points
    // Calculate coefficients for solve h = a*x + b*y + c

    float a,b,c;
    // Select triangle:
    if (x+y < 1)
    {
        if (x > y)
        {
            // 1 triangle (h1, h2, h5 points)
            float h1 = m_V9[(x_int)*129 + y_int];
            float h2 = m_V9[(x_int+1)*129 + y_int];
            float h5 = 2 * m_V8[x_int*128 + y_int];
            a = h2-h1;
            b = h5-h1-h2;
            c = h1;
        }
        else
        {
            // 2 triangle (h1, h3, h5 points)
            float h1 = m_V9[x_int*129 + y_int  ];
            float h3 = m_V9[x_int*129 + y_int+1];
            float h5 = 2 * m_V8[x_int*128 + y_int];
            a = h5 - h1 - h3;
            b = h3 - h1;
            c = h1;
        }
    }
    else
    {
        if (x > y)
        {
            // 3 triangle (h2, h4, h5 points)
            float h2 = m_V9[(x_int+1)*129 + y_int  ];
            float h4 = m_V9[(x_int+1)*129 + y_int+1];
            float h5 = 2 * m_V8[x_int*128 + y_int];
            a = h2 + h4 - h5;
            b = h4 - h2;
            c = h5 - h4;
        }
        else
        {
            // 4 triangle (h3, h4, h5 points)
            float h3 = m_V9[(x_int)*129 + y_int+1];
            float h4 = m_V9[(x_int+1)*129 + y_int+1];
            float h5 = 2 * m_V8[x_int*128 + y_int];
            a = h4 - h3;
            b = h3 + h4 - h5;
            c = h5 - h4;
        }
    }
    // Calculate height
    return a * x + b * y + c;
}

float  GridMap::getHeightFromUint8(float x, float y) const
{
    if (!m_uint8_V8 || !m_uint8_V9)
        return m_gridHeight;

    x = MAP_RESOLUTION * (32 - x/SIZE_OF_GRIDS);
    y = MAP_RESOLUTION * (32 - y/SIZE_OF_GRIDS);

    int x_int = (int)x;
    int y_int = (int)y;
    x -= x_int;
    y -= y_int;
    x_int&=(MAP_RESOLUTION - 1);
    y_int&=(MAP_RESOLUTION - 1);

    int32 a, b, c;
    uint8 *V9_h1_ptr = &m_uint8_V9[x_int*128 + x_int + y_int];
    if (x+y < 1)
    {
        if (x > y)
        {
            // 1 triangle (h1, h2, h5 points)
            int32 h1 = V9_h1_ptr[  0];
            int32 h2 = V9_h1_ptr[129];
            int32 h5 = 2 * m_uint8_V8[x_int*128 + y_int];
            a = h2-h1;
            b = h5-h1-h2;
            c = h1;
        }
        else
        {
            // 2 triangle (h1, h3, h5 points)
            int32 h1 = V9_h1_ptr[0];
            int32 h3 = V9_h1_ptr[1];
            int32 h5 = 2 * m_uint8_V8[x_int*128 + y_int];
            a = h5 - h1 - h3;
            b = h3 - h1;
            c = h1;
        }
    }
    else
    {
        if (x > y)
        {
            // 3 triangle (h2, h4, h5 points)
            int32 h2 = V9_h1_ptr[129];
            int32 h4 = V9_h1_ptr[130];
            int32 h5 = 2 * m_uint8_V8[x_int*128 + y_int];
            a = h2 + h4 - h5;
            b = h4 - h2;
            c = h5 - h4;
        }
        else
        {
            // 4 triangle (h3, h4, h5 points)
            int32 h3 = V9_h1_ptr[  1];
            int32 h4 = V9_h1_ptr[130];
            int32 h5 = 2 * m_uint8_V8[x_int*128 + y_int];
            a = h4 - h3;
            b = h3 + h4 - h5;
            c = h5 - h4;
        }
    }
    // Calculate height
    return (float)((a * x) + (b * y) + c)*m_gridIntHeightMultiplier + m_gridHeight;
}

float  GridMap::getHeightFromUint16(float x, float y) const
{
    if (!m_uint16_V8 || !m_uint16_V9)
        return m_gridHeight;

    x = MAP_RESOLUTION * (32 - x/SIZE_OF_GRIDS);
    y = MAP_RESOLUTION * (32 - y/SIZE_OF_GRIDS);

    int x_int = (int)x;
    int y_int = (int)y;
    x -= x_int;
    y -= y_int;
    x_int&=(MAP_RESOLUTION - 1);
    y_int&=(MAP_RESOLUTION - 1);

    int32 a, b, c;
    uint16 *V9_h1_ptr = &m_uint16_V9[x_int*128 + x_int + y_int];
    if (x+y < 1)
    {
        if (x > y)
        {
            // 1 triangle (h1, h2, h5 points)
            int32 h1 = V9_h1_ptr[  0];
            int32 h2 = V9_h1_ptr[129];
            int32 h5 = 2 * m_uint16_V8[x_int*128 + y_int];
            a = h2-h1;
            b = h5-h1-h2;
            c = h1;
        }
        else
        {
            // 2 triangle (h1, h3, h5 points)
            int32 h1 = V9_h1_ptr[0];
            int32 h3 = V9_h1_ptr[1];
            int32 h5 = 2 * m_uint16_V8[x_int*128 + y_int];
            a = h5 - h1 - h3;
            b = h3 - h1;
            c = h1;
        }
    }
    else
    {
        if (x > y)
        {
            // 3 triangle (h2, h4, h5 points)
            int32 h2 = V9_h1_ptr[129];
            int32 h4 = V9_h1_ptr[130];
            int32 h5 = 2 * m_uint16_V8[x_int*128 + y_int];
            a = h2 + h4 - h5;
            b = h4 - h2;
            c = h5 - h4;
        }
        else
        {
            // 4 triangle (h3, h4, h5 points)
            int32 h3 = V9_h1_ptr[  1];
            int32 h4 = V9_h1_ptr[130];
            int32 h5 = 2 * m_uint16_V8[x_int*128 + y_int];
            a = h4 - h3;
            b = h3 + h4 - h5;
            c = h5 - h4;
        }
    }
    // Calculate height
    return (float)((a * x) + (b * y) + c)*m_gridIntHeightMultiplier + m_gridHeight;
}
float  GridMap::getLiquidLevel(float x, float y)
{
    if (!m_liquid_map)
        return m_liquidLevel;

    x = MAP_RESOLUTION * (32 - x/SIZE_OF_GRIDS);
    y = MAP_RESOLUTION * (32 - y/SIZE_OF_GRIDS);

    int cx_int = ((int)x & (MAP_RESOLUTION-1)) - m_liquid_offY;
    int cy_int = ((int)y & (MAP_RESOLUTION-1)) - m_liquid_offX;

    if (cx_int < 0 || cx_int >=m_liquid_height)
        return INVALID_HEIGHT;
    if (cy_int < 0 || cy_int >=m_liquid_width)
        return INVALID_HEIGHT;

    return m_liquid_map[cx_int*m_liquid_width + cy_int];
}

uint8  GridMap::getTerrainType(float x, float y)
{
    if (!m_liquid_type)
        return m_liquidType;

    x = 16 * (32 - x/SIZE_OF_GRIDS);
    y = 16 * (32 - y/SIZE_OF_GRIDS);
    int lx = (int)x & 15;
    int ly = (int)y & 15;
    return m_liquid_type[lx*16 + ly];
}

// Get water state on map
inline ZLiquidStatus GridMap::getLiquidStatus(float x, float y, float z, uint8 ReqLiquidType, LiquidData *data)
{
    // Check water type (if no water return)
    if (!m_liquid_type && !m_liquidType)
        return LIQUID_MAP_NO_WATER;

    // Get cell
    float cx = MAP_RESOLUTION * (32 - x/SIZE_OF_GRIDS);
    float cy = MAP_RESOLUTION * (32 - y/SIZE_OF_GRIDS);

    int x_int = (int)cx & (MAP_RESOLUTION-1);
    int y_int = (int)cy & (MAP_RESOLUTION-1);

    // Check water type in cell
    uint8 type = m_liquid_type ? m_liquid_type[(x_int>>3)*16 + (y_int>>3)] : m_liquidType;
    if (type == 0)
        return LIQUID_MAP_NO_WATER;

    // Check req liquid type mask
    if (ReqLiquidType && !(ReqLiquidType&type))
        return LIQUID_MAP_NO_WATER;

    // Check water level:
    // Check water height map
    int lx_int = x_int - m_liquid_offY;
    int ly_int = y_int - m_liquid_offX;
    if (lx_int < 0 || lx_int >=m_liquid_height)
        return LIQUID_MAP_NO_WATER;
    if (ly_int < 0 || ly_int >=m_liquid_width)
        return LIQUID_MAP_NO_WATER;

    // Get water level
    float liquid_level = m_liquid_map ? m_liquid_map[lx_int*m_liquid_width + ly_int] : m_liquidLevel;
    // Get ground level (sub 0.2 for fix some errors)
    float ground_level = getHeight(x, y);

    // Check water level and ground level
    if (liquid_level < ground_level || z < ground_level - 2)
        return LIQUID_MAP_NO_WATER;

    // All ok in water -> store data
    if (data)
    {
        data->type  = type;
        data->level = liquid_level;
        data->depth_level = ground_level;
    }

    // For speed check as int values
    int delta = int((liquid_level - z) * 10);

    // Get position delta
    if (delta > 20)                   // Under water
        return LIQUID_MAP_UNDER_WATER;
    if (delta > 0)                   // In water
        return LIQUID_MAP_IN_WATER;
    if (delta > -1)                   // Walk on water
        return LIQUID_MAP_WATER_WALK;
                                      // Above water
    return LIQUID_MAP_ABOVE_WATER;
}

inline GridMap *Map::GetGrid(float x, float y)
{
    // half opt method
    int gx=(int)(32-x/SIZE_OF_GRIDS);                       //grid x
    int gy=(int)(32-y/SIZE_OF_GRIDS);                       //grid y

    // ensure GridMap is loaded
    EnsureGridCreated(GridPair(63-gx,63-gy));

    return GridMaps[gx][gy];
}

bool Map::getObjectHitPos(uint32 phasemask, float x1, float y1, float z1, float x2, float y2, float z2, float& rx, float& ry, float& rz, float modifyDist)
{
    Vector3 startPos = Vector3(x1, y1, z1);
    Vector3 dstPos = Vector3(x2, y2, z2);
    
    Vector3 resultPos;
    bool result = _dynamicTree.getObjectHitPos(phasemask, startPos, dstPos, resultPos, modifyDist);
    
    rx = resultPos.x;
    ry = resultPos.y;
    rz = resultPos.z;
    return result;
}

float Map::GetHeight(float x, float y, float z, bool pUseVmaps) const
{
    if (!MapManager::IsValidMapCoord(i_id, x, y, z))
        return 0;
    
    return std::max<float>(_GetHeight(x, y, z, pUseVmaps), _dynamicTree.getHeight(x, y, z));
}

float Map::_GetHeight(float x, float y, float z, bool pUseVmaps) const
{
    // find raw .map surface under Z coordinates
    float mapHeight;
    if (GridMap *gmap = const_cast<Map*>(this)->GetGrid(x, y))
    {
        float _mapheight = gmap->getHeight(x,y);

        // look from a bit higher pos to find the floor, ignore under surface case
        if (z + 2.0f > _mapheight)
            mapHeight = _mapheight;
        else
            mapHeight = VMAP_INVALID_HEIGHT_VALUE;
    }
    else
        mapHeight = VMAP_INVALID_HEIGHT_VALUE;

    float vmapHeight;
    if (pUseVmaps)
    {
        VMAP::IVMapManager* vmgr = VMAP::VMapFactory::createOrGetVMapManager();
        if (vmgr->isHeightCalcEnabled())
        {
            // look from a bit higher pos to find the floor
            vmapHeight = vmgr->getHeight(GetId(), x, y, z + 2.0f);
        }
        else
            vmapHeight = VMAP_INVALID_HEIGHT_VALUE;
    }
    else
        vmapHeight = VMAP_INVALID_HEIGHT_VALUE;

    // mapHeight set for any above raw ground Z or <= INVALID_HEIGHT
    // vmapheight set for any under Z value or <= INVALID_HEIGHT

    if (vmapHeight > INVALID_HEIGHT)
    {
        if (mapHeight > INVALID_HEIGHT)
        {
            // we have mapheight and vmapheight and must select more appropriate

            // we are already under the surface or vmap height above map heigt
            // or if the distance of the vmap height is less the land height distance
            if (z < mapHeight || vmapHeight > mapHeight || fabs(mapHeight-z) > fabs(vmapHeight-z))
                return vmapHeight;
            else
                return mapHeight;                           // better use .map surface height

        }
        else
            return vmapHeight;                              // we have only vmapHeight (if have)
    }
    else
    {
        if (!pUseVmaps)
            return mapHeight;                               // explicitly use map data (if have)
        else if (mapHeight > INVALID_HEIGHT && (z < mapHeight + 2 || z == MAX_HEIGHT))
            return mapHeight;                               // explicitly use map data if original z < mapHeight but map found (z+2 > mapHeight)
        else
            return VMAP_INVALID_HEIGHT_VALUE;               // we not have any height
    }
}

inline bool IsOutdoorWMO(uint32 mogpFlags, int32 adtId, int32 rootId, int32 groupId, WMOAreaTableEntry const* wmoEntry, AreaTableEntry const* atEntry, uint32 mapId)
{
    // If this flag is set we are outdoors and can mount up
    if (mogpFlags & 0x8000)
        return true;
    
    if (mogpFlags == 0x809 || mogpFlags == 0x849) // From observations, these zones are actually outdoor
        return true;
    
    // If flag 0x800 is set and we are in non-flyable areas we cannot mount up even if we are physically outdoors
    if (mapId != 530 && (mogpFlags & 0x800))
        return false;
    
    // If this flag is set we are physically outdoors, mounting up is allowed if previous check failed
    return mogpFlags & 0x8;
}

bool Map::IsOutdoors(float x, float y, float z) const
{
    uint32 mogpFlags;
    int32 adtId, rootId, groupId;

    // no wmo found? -> outside by default
    if(!GetAreaInfo(x, y, z, mogpFlags, adtId, rootId, groupId))
        return true;

    AreaTableEntry const* atEntry = 0;
    WMOAreaTableEntry const* wmoEntry = 0;
    /*WMOAreaTableEntry const* wmoEntry= GetWMOAreaTableEntryByTripple(rootId, adtId, groupId);
    if(wmoEntry)
    {
        DEBUG_LOG("Got WMOAreaTableEntry! flag %u, areaid %u", wmoEntry->Flags, wmoEntry->areaId);
        atEntry = GetAreaEntryByAreaID(wmoEntry->areaId);
    }*/
    return IsOutdoorWMO(mogpFlags, adtId, rootId, groupId, wmoEntry, atEntry, i_mapEntry->MapID);
}

bool Map::GetAreaInfo(float x, float y, float z, uint32 &flags, int32 &adtId, int32 &rootId, int32 &groupId) const
{
    float vmap_z = z;
    VMAP::IVMapManager* vmgr = VMAP::VMapFactory::createOrGetVMapManager();
    if (vmgr->getAreaInfo(GetId(), x, y, vmap_z, flags, adtId, rootId, groupId))
    {
        // check if there's terrain between player height and object height
        if(GridMap *gmap = const_cast<Map*>(this)->GetGrid(x, y))
        {
            float _mapheight = gmap->getHeight(x,y);
            // z + 2.0f condition taken from GetHeight(), not sure if it's such a great choice...
            if(z + 2.0f > _mapheight &&  _mapheight > vmap_z)
                return false;
        }
        return true;
    }
    return false;
}

uint16 Map::GetAreaFlag(float x, float y, float z, bool *isOutdoors) const
{
    uint32 mogpFlags;
    int32 adtId, rootId, groupId;
    WMOAreaTableEntry const* wmoEntry = 0;
    AreaTableEntry const* atEntry = 0;
    bool haveAreaInfo = false;

    if (GetAreaInfo(x, y, z, mogpFlags, adtId, rootId, groupId))
    {
        haveAreaInfo = true;
        if (wmoEntry = GetWMOAreaTableEntryByTripple(rootId, adtId, groupId))
            atEntry = GetAreaEntryByAreaID(wmoEntry->areaId);
    }

    uint16 areaflag;

    if (atEntry)
        areaflag = atEntry->exploreFlag;
    else
    {
        if (GridMap *gmap = const_cast<Map*>(this)->GetGrid(x, y))
            areaflag = gmap->getArea(x, y);
        // this used while not all *.map files generated (instances)
        else
            areaflag = GetAreaFlagByMapId(i_mapEntry->MapID);
    }

    if (isOutdoors)
    {
        if (haveAreaInfo)
            *isOutdoors = IsOutdoorWMO(mogpFlags, adtId, rootId, groupId, wmoEntry, atEntry, i_mapEntry->MapID);
        else
            *isOutdoors = true;
    }
    return areaflag;
}

uint8 Map::GetTerrainType(float x, float y) const
{
    if (GridMap *gmap = const_cast<Map*>(this)->GetGrid(x, y))
        return gmap->getTerrainType(x, y);
    else
        return 0;
}

ZLiquidStatus Map::getLiquidStatus(float x, float y, float z, uint8 ReqLiquidType, LiquidData *data) const
{
    ZLiquidStatus result = LIQUID_MAP_NO_WATER;
    VMAP::IVMapManager* vmgr = VMAP::VMapFactory::createOrGetVMapManager();
    float liquid_level, ground_level = INVALID_HEIGHT;
    uint32 liquid_type;

    if (!Trinity::IsValidMapCoord(x, y, z, 0))
        return LIQUID_MAP_NO_WATER;

    if (vmgr->GetLiquidLevel(GetId(), x, y, z, ReqLiquidType, liquid_level, ground_level, liquid_type))
    {
       // sLog.outDebug("getLiquidStatus(): vmap liquid level: %f ground: %f type: %u", liquid_level, ground_level, liquid_type);
        // Check water level and ground level
        if (liquid_level > ground_level && z > ground_level - 2)
        {
            // All ok in water -> store data
            if (data)
            {
                data->type  = liquid_type;
                data->level = liquid_level;
                data->depth_level = ground_level;
            }

            // For speed check as int values
            int delta = int((liquid_level - z) * 10);

            // Get position delta
            if (delta > 20)                   // Under water
                return LIQUID_MAP_UNDER_WATER;
            if (delta > 0 )                   // In water
                return LIQUID_MAP_IN_WATER;
            if (delta > -1)                   // Walk on water
                return LIQUID_MAP_WATER_WALK;
            result = LIQUID_MAP_ABOVE_WATER;
        }
    }

    if(GridMap* gmap = const_cast<Map*>(this)->GetGrid(x, y))
    {
        LiquidData map_data;
        ZLiquidStatus map_result = gmap->getLiquidStatus(x, y, z, ReqLiquidType, &map_data);
        // Not override LIQUID_MAP_ABOVE_WATER with LIQUID_MAP_NO_WATER:
        if (map_result != LIQUID_MAP_NO_WATER && (map_data.level > ground_level))
        {
            if (data)
                *data = map_data;
            return map_result;
        }
    }
    return result;
}

float Map::GetWaterLevel(float x, float y) const
{
    if (GridMap* gmap = const_cast<Map*>(this)->GetGrid(x, y))
        return gmap->getLiquidLevel(x, y);
    else
        return 0;
}

uint32 Map::GetAreaId(uint16 areaflag,uint32 map_id)
{
    AreaTableEntry const *entry = GetAreaEntryByAreaFlagAndMap(areaflag,map_id);

    if (entry)
        return entry->ID;
    else
        return 0;
}

uint32 Map::GetZoneId(uint16 areaflag,uint32 map_id)
{
    AreaTableEntry const *entry = GetAreaEntryByAreaFlagAndMap(areaflag,map_id);

    if( entry)
        return ( entry->zone != 0) ? entry->zone : entry->ID;
    else
        return 0;
}

bool Map::isInLineOfSight(float x1, float y1, float z1, float x2, float y2, float z2, uint32 phasemask) const
{
    return VMAP::VMapFactory::createOrGetVMapManager()->isInLineOfSight(GetId(), x1, y1, z1, x2, y2, z2)
            && _dynamicTree.isInLineOfSight(x1, y1, z1, x2, y2, z2, phasemask);
}

bool Map::IsInWater(float x, float y, float pZ, LiquidData *data) const
{
    // Check surface in x, y point for liquid
    if (const_cast<Map*>(this)->GetGrid(x, y))
    {
        LiquidData liquid_status;
        LiquidData *liquid_ptr = data ? data : &liquid_status;
        if (getLiquidStatus(x, y, pZ, MAP_ALL_LIQUIDS, liquid_ptr))
                return true;
    }
    return false;
}

bool Map::IsUnderWater(float x, float y, float z) const
{
    if (const_cast<Map*>(this)->GetGrid(x, y))
    {
        if (getLiquidStatus(x, y, z, MAP_LIQUID_TYPE_WATER|MAP_LIQUID_TYPE_OCEAN)&LIQUID_MAP_UNDER_WATER)
            return true;
    }
    return false;
}

bool Map::CheckGridIntegrity(Creature* c, bool moved) const
{
    Cell const& cur_cell = c->GetCurrentCell();

    CellPair xy_val = Trinity::ComputeCellPair(c->GetPositionX(), c->GetPositionY());
    Cell xy_cell(xy_val);
    if(xy_cell != cur_cell)
    {
        sLog.outDebug("Creature (GUIDLow: %u) X: %f Y: %f (%s) in grid[%u,%u]cell[%u,%u] instead grid[%u,%u]cell[%u,%u]",
            c->GetGUIDLow(),
            c->GetPositionX(),c->GetPositionY(),(moved ? "final" : "original"),
            cur_cell.GridX(), cur_cell.GridY(), cur_cell.CellX(), cur_cell.CellY(),
            xy_cell.GridX(),  xy_cell.GridY(),  xy_cell.CellX(),  xy_cell.CellY());
        return true;                                        // not crash at error, just output error in debug mode
    }

    return true;
}

const char* Map::GetMapName() const
{
    return i_mapEntry ? i_mapEntry->name[sWorld.GetDefaultDbcLocale()] : "UNNAMEDMAP\x0";
}

void Map::UpdateObjectVisibility( WorldObject* obj, Cell cell, CellPair cellpair)
{
    cell.data.Part.reserved = ALL_DISTRICT;
    cell.SetNoCreate();
    Trinity::VisibleChangesNotifier notifier(*obj);
    TypeContainerVisitor<Trinity::VisibleChangesNotifier, WorldTypeMapContainer > player_notifier(notifier);
    cell.Visit(cellpair, player_notifier, *this, *obj, GetVisibilityDistance());
}

void Map::SendInitSelf( Player * player)
{
    sLog.outDetail("Creating player data for himself %u", player->GetGUIDLow());

    UpdateData data;

    bool hasTransport = false;

    // attach to player data current transport data
    if(Transport* transport = player->GetTransport())
    {
        hasTransport = true;
        transport->BuildCreateUpdateBlockForPlayer(&data, player);
    }

    // build data for self presence in world at own client (one time for map)
    player->BuildCreateUpdateBlockForPlayer(&data, player);

    // build other passengers at transport also (they always visible and marked as visible and will not send at visibility update at add to map
    if(Transport* transport = player->GetTransport())
    {
        for(Transport::PlayerSet::const_iterator itr = transport->GetPassengers().begin();itr!=transport->GetPassengers().end();++itr)
        {
            if(player!=(*itr) && player->HaveAtClient(*itr))
            {
                hasTransport = true;
                (*itr)->BuildCreateUpdateBlockForPlayer(&data, player);
            }
        }
    }

    WorldPacket packet;
    data.BuildPacket(&packet, hasTransport);
    player->GetSession()->SendPacket(&packet);
}

void Map::SendInitTransports( Player * player)
{
    // Hack to send out transports
    MapManager::TransportMap& tmap = MapManager::Instance().m_TransportsByMap;

    // no transports at map
    if (tmap.find(player->GetMapId()) == tmap.end())
        return;

    UpdateData transData;

    MapManager::TransportSet& tset = tmap[player->GetMapId()];

    bool hasTransport = false;

    for (MapManager::TransportSet::iterator i = tset.begin(); i != tset.end(); ++i)
    {
        if((*i) != player->GetTransport())                  // send data for current transport in other place
        {
            hasTransport = true;
            (*i)->BuildCreateUpdateBlockForPlayer(&transData, player);
        }
    }

    WorldPacket packet;
    transData.BuildPacket(&packet, hasTransport);
    player->GetSession()->SendPacket(&packet);
}

void Map::SendRemoveTransports( Player * player)
{
    // Hack to send out transports
    MapManager::TransportMap& tmap = MapManager::Instance().m_TransportsByMap;

    // no transports at map
    if (tmap.find(player->GetMapId()) == tmap.end())
        return;

    UpdateData transData;

    MapManager::TransportSet& tset = tmap[player->GetMapId()];

    // except used transport
    for (MapManager::TransportSet::iterator i = tset.begin(); i != tset.end(); ++i)
        if(player->GetTransport() != (*i))
            (*i)->BuildOutOfRangeUpdateBlock(&transData);

    WorldPacket packet;
    transData.BuildPacket(&packet);
    player->GetSession()->SendPacket(&packet);
}

inline void Map::setNGrid(NGridType *grid, uint32 x, uint32 y)
{
    if(x >= MAX_NUMBER_OF_GRIDS || y >= MAX_NUMBER_OF_GRIDS)
    {
        sLog.outError("map::setNGrid() Invalid grid coordinates found: %d, %d!",x,y);
        assert(false);
    }
    i_grids[x][y] = grid;
}

void Map::DoDelayedMovesAndRemoves()
{
    MoveAllCreaturesInMoveList();
    RemoveAllObjectsInRemoveList();
}

void Map::AddObjectToRemoveList(WorldObject *obj)
{
    assert(obj->GetMapId()==GetId() && obj->GetInstanceId()==GetInstanceId());

    i_objectsToRemove.insert(obj);
    //sLog.outDebug("Object (GUID: %u TypeId: %u) added to removing list.",obj->GetGUIDLow(),obj->GetTypeId());
}

void Map::AddObjectToSwitchList(WorldObject *obj, bool on)
{
    assert(obj->GetMapId()==GetId() && obj->GetInstanceId()==GetInstanceId());

    std::map<WorldObject*, bool>::iterator itr = i_objectsToSwitch.find(obj);
    if(itr == i_objectsToSwitch.end())
        i_objectsToSwitch.insert(itr, std::make_pair(obj, on));
    else if(itr->second != on)
        i_objectsToSwitch.erase(itr);
    else
        assert(false);
}

void Map::RemoveAllObjectsInRemoveList()
{
    while(!i_objectsToSwitch.empty())
    {
        std::map<WorldObject*, bool>::iterator itr = i_objectsToSwitch.begin();
        WorldObject *obj = itr->first;
        bool on = itr->second;
        i_objectsToSwitch.erase(itr);

        switch(obj->GetTypeId())
        {
        case TYPEID_UNIT:
            if(!(obj->ToCreature())->isPet())
                SwitchGridContainers(obj->ToCreature(), on);
            break;
        }
    }

    //sLog.outDebug("Object remover 1 check.");
    while(!i_objectsToRemove.empty())
    {
        std::set<WorldObject*>::iterator itr = i_objectsToRemove.begin();
        WorldObject* obj = *itr;

        switch(obj->GetTypeId())
        {
            case TYPEID_CORPSE:
            {
                Corpse* corpse = ObjectAccessor::Instance().GetCorpse(*obj, obj->GetGUID());
                if (!corpse)
                    sLog.outError("Try delete corpse/bones %u that not in map", obj->GetGUIDLow());
                else
                    Remove(corpse,true);
                break;
            }
        case TYPEID_DYNAMICOBJECT:
            Remove((DynamicObject*)obj,true);
            break;
        case TYPEID_GAMEOBJECT:
            Remove((GameObject*)obj,true);
            break;
        case TYPEID_UNIT:
            // in case triggered sequence some spell can continue casting after prev CleanupsBeforeDelete call
            // make sure that like sources auras/etc removed before destructor start
            (obj->ToCreature())->CleanupsBeforeDelete ();
            Remove(obj->ToCreature(),true);
            break;
        default:
            sLog.outError("Non-grid object (TypeId: %u) in grid object removing list, ignored.",obj->GetTypeId());
            break;
        }

        i_objectsToRemove.erase(itr);
    }
    //sLog.outDebug("Object remover 2 check.");
}

uint32 Map::GetPlayersCountExceptGMs() const
{
    uint32 count = 0;
    for(MapRefManager::const_iterator itr = m_mapRefManager.begin(); itr != m_mapRefManager.end(); ++itr)
        if(!itr->getSource()->isGameMaster())
            ++count;
    return count;
}

void Map::SendToPlayers(WorldPacket const* data) const
{
    for(MapRefManager::const_iterator itr = m_mapRefManager.begin(); itr != m_mapRefManager.end(); ++itr)
        itr->getSource()->GetSession()->SendPacket(data);
}

bool Map::ActiveObjectsNearGrid(uint32 x, uint32 y) const
{
    assert(x < MAX_NUMBER_OF_GRIDS);
    assert(y < MAX_NUMBER_OF_GRIDS);

    CellPair cell_min(x*MAX_NUMBER_OF_CELLS, y*MAX_NUMBER_OF_CELLS);
    CellPair cell_max(cell_min.x_coord + MAX_NUMBER_OF_CELLS, cell_min.y_coord+MAX_NUMBER_OF_CELLS);

    //we must find visible range in cells so we unload only non-visible cells...
    float viewDist = GetVisibilityDistance();
    int cell_range = (int)ceilf(viewDist / SIZE_OF_GRID_CELL) + 1;

    cell_min << cell_range;
    cell_min -= cell_range;
    cell_max >> cell_range;
    cell_max += cell_range;

    for(MapRefManager::const_iterator iter = m_mapRefManager.begin(); iter != m_mapRefManager.end(); ++iter)
    {
        Player* plr = iter->getSource();

        CellPair p = Trinity::ComputeCellPair(plr->GetPositionX(), plr->GetPositionY());
        if( (cell_min.x_coord <= p.x_coord && p.x_coord <= cell_max.x_coord) &&
            (cell_min.y_coord <= p.y_coord && p.y_coord <= cell_max.y_coord))
            return true;
    }

    for(ActiveNonPlayers::const_iterator iter = m_activeNonPlayers.begin(); iter != m_activeNonPlayers.end(); ++iter)
    {
        WorldObject* obj = *iter;

        CellPair p = Trinity::ComputeCellPair(obj->GetPositionX(), obj->GetPositionY());
        if( (cell_min.x_coord <= p.x_coord && p.x_coord <= cell_max.x_coord) &&
            (cell_min.y_coord <= p.y_coord && p.y_coord <= cell_max.y_coord))
            return true;
    }

    return false;
}

void Map::AddToActive( Creature* c)
{
    AddToActiveHelper(c);

    // also not allow unloading spawn grid to prevent creating creature clone at load
    if(!c->isPet() && c->GetDBTableGUIDLow())
    {
        float x,y,z;
        c->GetRespawnCoord(x,y,z);
        GridPair p = Trinity::ComputeGridPair(x, y);
        if(getNGrid(p.x_coord, p.y_coord))
            getNGrid(p.x_coord, p.y_coord)->incUnloadActiveLock();
        else
        {
            GridPair p2 = Trinity::ComputeGridPair(c->GetPositionX(), c->GetPositionY());
            sLog.outError("Active creature (GUID: %u Entry: %u) added to grid[%u,%u] but spawn grid[%u,%u] not loaded.",
                c->GetGUIDLow(), c->GetEntry(), p.x_coord, p.y_coord, p2.x_coord, p2.y_coord);
        }
    }
}

void Map::RemoveFromActive( Creature* c)
{
    RemoveFromActiveHelper(c);

    // also allow unloading spawn grid
    if(!c->isPet() && c->GetDBTableGUIDLow())
    {
        float x,y,z;
        c->GetRespawnCoord(x,y,z);
        GridPair p = Trinity::ComputeGridPair(x, y);
        if(getNGrid(p.x_coord, p.y_coord))
            getNGrid(p.x_coord, p.y_coord)->decUnloadActiveLock();
        else
        {
            GridPair p2 = Trinity::ComputeGridPair(c->GetPositionX(), c->GetPositionY());
            sLog.outError("Active creature (GUID: %u Entry: %u) removed from grid[%u,%u] but spawn grid[%u,%u] not loaded.",
                c->GetGUIDLow(), c->GetEntry(), p.x_coord, p.y_coord, p2.x_coord, p2.y_coord);
        }
    }
}

Creature* Map::GetCreature(uint64 guid)
{
    Creature * ret = NULL;
    if(IS_CREATURE_GUID(guid))
        ret = ObjectAccessor::GetObjectInWorld(guid, (Creature*)NULL);

    if(!ret)
        return NULL;

    if(ret->GetMapId() != GetId())
        return NULL;

    if(ret->GetInstanceId() != GetInstanceId())
        return NULL;

    return ret;
}

GameObject* Map::GetGameObject(uint64 guid)
{
    GameObject * ret = ObjectAccessor::GetObjectInWorld(guid, (GameObject*)NULL);
    if(!ret)
        return NULL;
    if(ret->GetMapId() != GetId())
        return NULL;
    if(ret->GetInstanceId() != GetInstanceId())
        return NULL;
    return ret;
}

void Map::AddCreatureToPool(Creature *cre, uint32 poolId)
{
    CreaturePoolMember::iterator itr = m_cpmembers.find(poolId);
    if (itr == m_cpmembers.end()) {
        std::vector<Creature*> newVect;
        newVect.push_back(cre);
        m_cpmembers[poolId] = newVect;
    } else {
        itr->second.push_back(cre);
    }
}

void Map::RemoveCreatureFromPool(Creature *cre, uint32 poolId)
{
    CreaturePoolMember::iterator itr = m_cpmembers.find(poolId);
    if (itr != m_cpmembers.end()) {
        std::vector<Creature*> membersVect = itr->second;
        int i, vsize = membersVect.size();
        for (i = 0; i < vsize; i++) {
            if (membersVect[i] == cre) {
                membersVect.erase(membersVect.begin()+i);
                return;
            }
        }
        sLog.outError("Creature %u could not be removed from pool %u", cre->GetDBTableGUIDLow(), poolId);
    } else {
        sLog.outError("Pool %u not found for creature %u", poolId, cre->GetDBTableGUIDLow());
    }
}

bool Map::SupportsHeroicMode(const MapEntry* mapEntry)
{
    if(!mapEntry)
        return false;
    if (mapEntry->resetTimeHeroic)
        return true;

    const InstanceTemplateAddon* instTempAddon = objmgr.GetInstanceTemplateAddon(mapEntry->MapID);
    if(instTempAddon && instTempAddon->forceHeroicEnabled)
        return true;

    return false;    
}

std::vector<Creature*> Map::GetAllCreaturesFromPool(uint32 poolId)
{
    CreaturePoolMember::iterator itr = m_cpmembers.find(poolId);
    if (itr != m_cpmembers.end())
        return itr->second;

    std::vector<Creature*> emptyVect;
    return emptyVect;
}

template void Map::Add(Corpse *);
template void Map::Add(Creature *);
template void Map::Add(GameObject *);
template void Map::Add(DynamicObject *);

template void Map::Remove(Corpse *,bool);
template void Map::Remove(Creature *,bool);
template void Map::Remove(GameObject *, bool);
template void Map::Remove(DynamicObject *, bool);

/* ******* Dungeon Instance Maps ******* */

InstanceMap::InstanceMap(uint32 id, time_t expiry, uint32 InstanceId, uint8 SpawnMode)
  : Map(id, expiry, InstanceId, SpawnMode), i_data(NULL),
    m_resetAfterUnload(false), m_unloadWhenEmpty(false)
{
    //lets initialize visibility distance for dungeons
    InstanceMap::InitVisibilityDistance();

    // the timer is started by default, and stopped when the first player joins
    // this make sure it gets unloaded if for some reason no player joins
    m_unloadTimer = std::max(sWorld.getConfig(CONFIG_INSTANCE_UNLOAD_DELAY), (uint32)MIN_UNLOAD_DELAY);
}

InstanceMap::~InstanceMap()
{
    if(i_data)
    {
        delete i_data;
        i_data = NULL;
    }
    
    // unload instance specific navigation data
    MMAP::MMapFactory::createOrGetMMapManager()->unloadMapInstance(GetId(), GetInstanceId());
}

void InstanceMap::InitVisibilityDistance()
{
    //init visibility distance for instances
    m_VisibleDistance = sWorld.GetMaxVisibleDistanceInInstances();
}

/*
    Do map specific checks to see if the player can enter
*/
bool InstanceMap::CanEnter(Player *player)
{
    if(player->GetMapRef().getTarget() == this)
    {
        sLog.outError("InstanceMap::CanEnter - player %s(%u) already in map %d,%d,%d!", player->GetName(), player->GetGUIDLow(), GetId(), GetInstanceId(), GetSpawnMode());
//        assert(false);
        return false;
    }

    // cannot enter if the instance is full (player cap), GMs don't count
    InstanceTemplate const* iTemplate = objmgr.GetInstanceTemplate(GetId());
    if (!player->isGameMaster() && GetPlayersCountExceptGMs() >= iTemplate->maxPlayers)
    {
        sLog.outDetail("MAP: Instance '%u' of map '%s' cannot have more than '%u' players. Player '%s' rejected", GetInstanceId(), GetMapName(), iTemplate->maxPlayers, player->GetName());
        player->SendTransferAborted(GetId(), TRANSFER_ABORT_MAX_PLAYERS);
        return false;
    }

    // cannot enter while players in the instance are in combat
    Group *pGroup = player->GetGroup();
    if(!player->isGameMaster() && pGroup && pGroup->InCombatToInstance(GetInstanceId()) && player->GetMapId() != GetId())
    {
        player->SendTransferAborted(GetId(), TRANSFER_ABORT_ZONE_IN_COMBAT);
        return false;
    }

    return Map::CanEnter(player);
}

/*
    Do map specific checks and add the player to the map if successful.
*/
bool InstanceMap::Add(Player *player)
{
    // TODO: Not sure about checking player level: already done in HandleAreaTriggerOpcode
    // GMs still can teleport player in instance.
    // Is it needed?

    {
        Guard guard(*this);
        if(!CanEnter(player))
            return false;

        // get or create an instance save for the map
        InstanceSave *mapSave = sInstanceSaveManager.GetInstanceSave(GetInstanceId());
        if(!mapSave)
        {
            sLog.outDetail("InstanceMap::Add: creating instance save for map %d spawnmode %d with instance id %d", GetId(), GetSpawnMode(), GetInstanceId());
            mapSave = sInstanceSaveManager.AddInstanceSave(GetId(), GetInstanceId(), GetSpawnMode(), 0, true);
        }

        // check for existing instance binds
        InstancePlayerBind *playerBind = player->GetBoundInstance(GetId(), GetSpawnMode());
        if(playerBind && playerBind->perm)
        {
            // cannot enter other instances if bound permanently
            if(playerBind->save != mapSave)
            {
                sLog.outError("InstanceMap::Add: player %s(%d) is permanently bound to instance %d,%d,%d,%d,%d,%d but he is being put in instance %d,%d,%d,%d,%d,%d", player->GetName(), player->GetGUIDLow(), playerBind->save->GetMapId(), playerBind->save->GetInstanceId(), playerBind->save->GetDifficulty(), playerBind->save->GetPlayerCount(), playerBind->save->GetGroupCount(), playerBind->save->CanReset(), mapSave->GetMapId(), mapSave->GetInstanceId(), mapSave->GetDifficulty(), mapSave->GetPlayerCount(), mapSave->GetGroupCount(), mapSave->CanReset());
                assert(false);
            }
        }
        else
        {
            Group *pGroup = player->GetGroup();
            if(pGroup)
            {
                // solo saves should be reset when entering a group
                InstanceGroupBind *groupBind = pGroup->GetBoundInstance(GetId(), GetSpawnMode());
                if(playerBind)
                {
                    sLog.outError("InstanceMap::Add: player %s(%d) is being put in instance %d,%d,%d,%d,%d,%d but he is in group %d and is bound to instance %d,%d,%d,%d,%d,%d!", player->GetName(), player->GetGUIDLow(), mapSave->GetMapId(), mapSave->GetInstanceId(), mapSave->GetDifficulty(), mapSave->GetPlayerCount(), mapSave->GetGroupCount(), mapSave->CanReset(), GUID_LOPART(pGroup->GetLeaderGUID()), playerBind->save->GetMapId(), playerBind->save->GetInstanceId(), playerBind->save->GetDifficulty(), playerBind->save->GetPlayerCount(), playerBind->save->GetGroupCount(), playerBind->save->CanReset());
                    if(groupBind) sLog.outError("InstanceMap::Add: the group is bound to instance %d,%d,%d,%d,%d,%d", groupBind->save->GetMapId(), groupBind->save->GetInstanceId(), groupBind->save->GetDifficulty(), groupBind->save->GetPlayerCount(), groupBind->save->GetGroupCount(), groupBind->save->CanReset());
                    sLog.outError("InstanceMap::Add: do not let player %s enter instance otherwise crash will happen", player->GetName());
                    return false;
                    //player->UnbindInstance(GetId(), GetSpawnMode());
                    //assert(false);
                }
                // bind to the group or keep using the group save
                if(!groupBind)
                    pGroup->BindToInstance(mapSave, false);
                else
                {
                    // cannot jump to a different instance without resetting it
                    if(groupBind->save != mapSave)
                    {
                        sLog.outError("InstanceMap::Add: player %s(%d) is being put in instance %d,%d,%d but he is in group %d which is bound to instance %d,%d,%d!", player->GetName(), player->GetGUIDLow(), mapSave->GetMapId(), mapSave->GetInstanceId(), mapSave->GetDifficulty(), GUID_LOPART(pGroup->GetLeaderGUID()), groupBind->save->GetMapId(), groupBind->save->GetInstanceId(), groupBind->save->GetDifficulty());
                        if(mapSave)
                            sLog.outError("MapSave players: %d, group count: %d", mapSave->GetPlayerCount(), mapSave->GetGroupCount());
                        else
                            sLog.outError("MapSave NULL");
                        if(groupBind->save)
                            sLog.outError("GroupBind save players: %d, group count: %d", groupBind->save->GetPlayerCount(), groupBind->save->GetGroupCount());
                        else
                            sLog.outError("GroupBind save NULL");
                        assert(false);
                    }
                    // if the group/leader is permanently bound to the instance
                    // players also become permanently bound when they enter
                    if(groupBind->perm)
                    {
                        WorldPacket data(SMSG_INSTANCE_SAVE_CREATED, 4);
                        data << uint32(0);
                        player->GetSession()->SendPacket(&data);
                        player->BindToInstance(mapSave, true);
                    }
                }
            }
            else
            {
                // set up a solo bind or continue using it
                if(!playerBind)
                    player->BindToInstance(mapSave, false);
                else
                    // cannot jump to a different instance without resetting it
                    assert(playerBind->save == mapSave);
            }
        }

        if(i_data) i_data->OnPlayerEnter(player);
        // for normal instances cancel the reset schedule when the
        // first player enters (no players yet)
        SetResetSchedule(false);

        player->SendInitWorldStates();
        sLog.outDetail("MAP: Player '%s' entered the instance '%u' of map '%s'", player->GetName(), GetInstanceId(), GetMapName());
        // initialize unload state
        m_unloadTimer = 0;
        m_resetAfterUnload = false;
        m_unloadWhenEmpty = false;
    }
    
    // Remove auras that cannot be present in instance
    player->RemoveAurasWithCustomAttribute(SPELL_ATTR_CU_REMOVE_ON_INSTANCE_ENTER);

    // this will acquire the same mutex so it cannot be in the previous block
    Map::Add(player);
    return true;
}

void InstanceMap::Update(const uint32& t_diff)
{
    Map::Update(t_diff);

    if(i_data)
        i_data->Update(t_diff);
}

void InstanceMap::Remove(Player *player, bool remove)
{
    sLog.outDetail("MAP: Removing player '%s' from instance '%u' of map '%s' before relocating to other map", player->GetName(), GetInstanceId(), GetMapName());
    //if last player set unload timer
    if(!m_unloadTimer && m_mapRefManager.getSize() == 1)
        m_unloadTimer = m_unloadWhenEmpty ? MIN_UNLOAD_DELAY : std::max(sWorld.getConfig(CONFIG_INSTANCE_UNLOAD_DELAY), (uint32)MIN_UNLOAD_DELAY);
    Map::Remove(player, remove);
    // for normal instances schedule the reset after all players have left
    SetResetSchedule(true);
}

Creature * Map::GetCreatureInMap(uint64 guid)
{
    Creature * obj = HashMapHolder<Creature>::Find(guid);
    if(obj && obj->GetInstanceId() != GetInstanceId()) obj = NULL;
    return obj;
}

GameObject * Map::GetGameObjectInMap(uint64 guid)
{
    GameObject * obj = HashMapHolder<GameObject>::Find(guid);
    if(obj && obj->GetInstanceId() != GetInstanceId()) obj = NULL;
    return obj;
}

void InstanceMap::CreateInstanceData(bool load)
{
    if(i_data != NULL)
        return;

    InstanceTemplate const* mInstance = objmgr.GetInstanceTemplate(GetId());
    if (mInstance)
    {
        i_script_id = mInstance->script_id;
        i_data = sScriptMgr.CreateInstanceData(this);
    }

    if(!i_data)
        return;

    i_data->Initialize();

    if(load)
    {
        // TODO: make a global storage for this
        QueryResult* result = CharacterDatabase.PQuery("SELECT data FROM instance WHERE map = '%u' AND id = '%u'", GetId(), i_InstanceId);
        if (result)
        {
            Field* fields = result->Fetch();
            const char* data = fields[0].GetString();
            if(data)
                i_data->Load(data);

            delete result;
        }
    }
}

/*
    Returns true if there are no players in the instance
*/
bool InstanceMap::Reset(uint8 method)
{
    // note: since the map may not be loaded when the instance needs to be reset
    // the instance must be deleted from the DB by InstanceSaveManager

    if(HavePlayers())
    {
        if(method == INSTANCE_RESET_ALL)
        {
            // notify the players to leave the instance so it can be reset
            for(MapRefManager::iterator itr = m_mapRefManager.begin(); itr != m_mapRefManager.end(); ++itr)
                itr->getSource()->SendResetFailedNotify(GetId());
        }
        else
        {
            if(method == INSTANCE_RESET_GLOBAL)
            {
                // set the homebind timer for players inside (1 minute)
                for(MapRefManager::iterator itr = m_mapRefManager.begin(); itr != m_mapRefManager.end(); ++itr)
                    itr->getSource()->m_InstanceValid = false;
            }

            // the unload timer is not started
            // instead the map will unload immediately after the players have left
            m_unloadWhenEmpty = true;
            m_resetAfterUnload = true;
        }
    }
    else
    {
        // unloaded at next update
        m_unloadTimer = MIN_UNLOAD_DELAY;
        m_resetAfterUnload = true;
    }

    return m_mapRefManager.isEmpty();
}

void InstanceMap::PermBindAllPlayers(Player *player)
{
    InstanceSave *save = sInstanceSaveManager.GetInstanceSave(GetInstanceId());
    if(!save)
    {
        sLog.outError("Cannot bind players, no instance save available for map!\n");
        return;
    }

    Group *group = player->GetGroup();
    // group members outside the instance group don't get bound
    for(MapRefManager::iterator itr = m_mapRefManager.begin(); itr != m_mapRefManager.end(); ++itr)
    {
        Player* plr = itr->getSource();
        // players inside an instance cannot be bound to other instances
        // some players may already be permanently bound, in this case nothing happens
        InstancePlayerBind *bind = plr->GetBoundInstance(save->GetMapId(), save->GetDifficulty());
        if(!bind || !bind->perm)
        {
            plr->BindToInstance(save, true);
            WorldPacket data(SMSG_INSTANCE_SAVE_CREATED, 4);
            data << uint32(0);
            plr->GetSession()->SendPacket(&data);
        }

        // if the leader is not in the instance the group will not get a perm bind
        if(group && group->GetLeaderGUID() == plr->GetGUID())
            group->BindToInstance(save, true);
    }
}

time_t InstanceMap::GetResetTime()
{
    InstanceSave *save = sInstanceSaveManager.GetInstanceSave(GetInstanceId());
    return save ? save->GetDifficulty() : DIFFICULTY_NORMAL;
}

void InstanceMap::UnloadAll()
{
    if(HavePlayers())
    {
        sLog.outError("InstanceMap::UnloadAll: there are still players in the instance at unload, should not happen!");
        for(MapRefManager::iterator itr = m_mapRefManager.begin(); itr != m_mapRefManager.end(); ++itr)
        {
            Player* plr = itr->getSource();
            plr->TeleportTo(plr->m_homebindMapId, plr->m_homebindX, plr->m_homebindY, plr->m_homebindZ, plr->GetOrientation());
        }
    }

    if(m_resetAfterUnload == true)
        objmgr.DeleteRespawnTimeForInstance(GetInstanceId());

    Map::UnloadAll();
}

void InstanceMap::SendResetWarnings(uint32 timeLeft) const
{
    for(MapRefManager::const_iterator itr = m_mapRefManager.begin(); itr != m_mapRefManager.end(); ++itr)
        itr->getSource()->SendInstanceResetWarning(GetId(), timeLeft);
}

void InstanceMap::SetResetSchedule(bool on)
{
    // only for normal instances
    // the reset time is only scheduled when there are no payers inside
    // it is assumed that the reset time will rarely (if ever) change while the reset is scheduled
    if(!HavePlayers() && !IsRaid() && !IsHeroic())
    {
        InstanceSave *save = sInstanceSaveManager.GetInstanceSave(GetInstanceId());
        if(!save) sLog.outError("InstanceMap::SetResetSchedule: cannot turn schedule %s, no save available for instance %d of %d", on ? "on" : "off", GetInstanceId(), GetId());
        else sInstanceSaveManager.ScheduleReset(on, save->GetResetTime(), InstanceSaveManager::InstResetEvent(0, GetId(), GetInstanceId()));
    }
}

/* ******* Battleground Instance Maps ******* */

BattleGroundMap::BattleGroundMap(uint32 id, time_t expiry, uint32 InstanceId)
  : Map(id, expiry, InstanceId, DIFFICULTY_NORMAL)
{
    //lets initialize visibility distance for BG/Arenas
    BattleGroundMap::InitVisibilityDistance();
}

BattleGroundMap::~BattleGroundMap()
{
}

void BattleGroundMap::InitVisibilityDistance()
{
    //init visibility distance for BG/Arenas
    m_VisibleDistance = sWorld.GetMaxVisibleDistanceInBGArenas();
}

bool BattleGroundMap::CanEnter(Player * player)
{
    if(player->GetMapRef().getTarget() == this)
    {
        sLog.outError("BGMap::CanEnter - player %u already in map!", player->GetGUIDLow());
        assert(false);
        return false;
    }

    if(player->GetBattleGroundId() != GetInstanceId())
        return false;

    // player number limit is checked in bgmgr, no need to do it here

    return Map::CanEnter(player);
}

bool BattleGroundMap::Add(Player * player)
{
    {
        Guard guard(*this);
        if(!CanEnter(player))
            return false;
        // reset instance validity, battleground maps do not homebind
        player->m_InstanceValid = true;
    }
    return Map::Add(player);
}

void BattleGroundMap::Remove(Player *player, bool remove)
{
	if (player && player->isSpectator() && !player->isSpectateCanceled())
	{
	    if (GetBG())
	        GetBG()->RemoveSpectator(player->GetGUID());

	    if (player->isSpectator())
	        player->SetSpectate(false);
	}

    sLog.outDetail("MAP: Removing player '%s' from bg '%u' of map '%s' before relocating to other map", player->GetName(), GetInstanceId(), GetMapName());
    Map::Remove(player, remove);
}

void BattleGroundMap::SetUnload()
{
    m_unloadTimer = MIN_UNLOAD_DELAY;
}

void BattleGroundMap::UnloadAll()
{
    while(HavePlayers())
    {
        if(Player * plr = m_mapRefManager.getFirst()->getSource())
        {
            plr->TeleportTo(plr->m_homebindMapId, plr->m_homebindX, plr->m_homebindY, plr->m_homebindZ, plr->GetOrientation());
            // TeleportTo removes the player from this map (if the map exists) -> calls BattleGroundMap::Remove -> invalidates the iterator.
            // just in case, remove the player from the list explicitly here as well to prevent a possible infinite loop
            // note that this remove is not needed if the code works well in other places
            plr->GetMapRef().unlink();
        }
    }

    Map::UnloadAll();
}

/*--------------------------TRINITY-------------------------*/

