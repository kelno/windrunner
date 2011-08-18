/*
 * Copyright (C) 2005-2011 MaNGOS <http://getmangos.com/>
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

#include "MoveMap.h"
#include "Map.h"
#include "Creature.h"
#include "PathFinder.h"
#include "Log.h"

#include "../recastnavigation/Detour/Include/DetourCommon.h"

////////////////// PathInfo //////////////////
PathInfo::PathInfo(const Unit* owner, const float destX, const float destY, const float destZ,
                   bool useStraightPath, bool forceDest) :
    m_polyLength(0), m_type(PATHFIND_BLANK),
    m_useStraightPath(useStraightPath), m_forceDestination(forceDest),
    m_sourceUnit(owner), m_navMesh(NULL), m_navMeshQuery(NULL)
{
    PathNode endPoint(destX, destY, destZ);
    setEndPosition(endPoint);

    float x,y,z;
    m_sourceUnit->GetPosition(x, y, z);
    PathNode startPoint(x, y, z);
    setStartPosition(startPoint);

    uint32 mapId = m_sourceUnit->GetMapId();
    if (MMAP::MMapFactory::IsPathfindingEnabled(mapId))
    {
        MMAP::MMapManager* mmap = MMAP::MMapFactory::createOrGetMMapManager();
        m_navMesh = mmap->GetNavMesh(mapId);
        m_navMeshQuery = mmap->GetNavMeshQuery(mapId, m_sourceUnit->GetInstanceId());
        if (m_navMesh != m_navMeshQuery->getNavMesh())
            sLog.outError("NAVMESH: navmesh from pathinfo and from nmquery don't match. MapId %d, instanceId %d", mapId, m_sourceUnit->GetInstanceId());
    }

    createFilter();

    if (m_navMesh && m_navMeshQuery && !m_sourceUnit->hasUnitState(UNIT_STAT_IGNORE_PATHFINDING) &&
        Trinity::IsValidMapCoord(m_sourceUnit->GetPositionX(), m_sourceUnit->GetPositionY(), m_sourceUnit->GetPositionZ()))
    {
        BuildPolyPath(startPoint, endPoint);
    }
    else
    {
        BuildShortcut();
        m_type = PathType(PATHFIND_NORMAL | PATHFIND_NOT_USING_PATH);
    }
}

PathInfo::~PathInfo() {}

bool PathInfo::Update(const float destX, const float destY, const float destZ,
                      bool useStraightPath, bool forceDest)
{
    PathNode newDest(destX, destY, destZ);
    PathNode oldDest = getEndPosition();
    setEndPosition(newDest);

    float x, y, z;
    m_sourceUnit->GetPosition(x, y, z);
    PathNode newStart(x, y, z);
    PathNode oldStart = getStartPosition();
    setStartPosition(newStart);

    m_useStraightPath = useStraightPath;
    m_forceDestination = forceDest;

    // make sure navMesh works - we can run on map w/o mmap
    if (!m_navMesh || !m_navMeshQuery || m_sourceUnit->hasUnitState(UNIT_STAT_IGNORE_PATHFINDING))
    {
        BuildShortcut();
        m_type = PathType(PATHFIND_NORMAL | PATHFIND_NOT_USING_PATH);
        return true;
    }

    if (m_navMeshQuery->getNavMesh() != m_navMesh)
    {
        sLog.outError("NAVMESH: PathInfo::Update: navmesh from pathinfo and from nmquery don't match. Attempting to fix.");

        MMAP::MMapManager* mmap = MMAP::MMapFactory::createOrGetMMapManager();
        const dtNavMesh* navMesh_tmp = mmap->GetNavMesh(m_sourceUnit->GetMapId());
        const dtNavMeshQuery* navMeshQuery_tmp = mmap->GetNavMeshQuery(m_sourceUnit->GetMapId(), m_sourceUnit->GetInstanceId());

        if (m_navMeshQuery != navMeshQuery_tmp && navMeshQuery_tmp->getNavMesh()->getMaxTiles() != 0) {
            m_navMeshQuery = navMeshQuery_tmp;
            m_navMesh = navMeshQuery_tmp->getNavMesh();
            sLog.outError("NAVMESH: PathInfo::Update: it sounds like it is fixed. If it crashes, then it's not.");
        } else {
            sLog.outError("NAVMESH: PathInfo::Update: cannot fix. Falling back to shortcut method.");
            m_navMesh = NULL;
            m_navMeshQuery = NULL;
            BuildShortcut();
            m_type = PathType(PATHFIND_NORMAL | PATHFIND_NOT_USING_PATH);
            return true;
        }
    }

    updateFilter();

    // check if destination moved - if not we can optimize something here
    // we are following old, precalculated path?
    float dist = m_sourceUnit->GetObjectSize();
    if (inRange(oldDest, newDest, dist, dist) && m_pathPoints.size() > 2)
    {
        // our target is not moving - we just coming closer
        // we are moving on precalculated path - enjoy the ride

        m_pathPoints.crop(1, 0);
        setNextPosition(m_pathPoints[1]);

        return false;
    }
    else
    {
        // target moved, so we need to update the poly path
        BuildPolyPath(newStart, newDest);
        return true;
    }
}

dtPolyRef PathInfo::getPathPolyByPosition(dtPolyRef *polyPath, uint32 polyPathSize, const float* point, float *distance)
{
    if (!polyPath || !polyPathSize)
        return INVALID_POLYREF;

    dtPolyRef nearestPoly = INVALID_POLYREF;
    float minDist2d = FLT_MAX;
    float minDist3d = 0.0f;

    for (uint32 i = 0; i < polyPathSize; ++i)
    {
        assert(polyPath[i] != INVALID_POLYREF);

        float closestPoint[VERTEX_SIZE];
        if (DT_SUCCESS != m_navMeshQuery->closestPointOnPoly(polyPath[i], point, closestPoint))
            continue;

        float d = dtVdist2DSqr(point, closestPoint);
        if (d < minDist2d)
        {
            minDist2d = d;
            nearestPoly = m_pathPolyRefs[i];
            minDist3d = dtVdistSqr(point, closestPoint);
        }

        if(minDist2d < 1.0f) // shortcut out - close enough for us
            break;
    }

    if (distance)
        *distance = dtSqrt(minDist3d);

    return (minDist2d < 4.0f) ? nearestPoly : INVALID_POLYREF;
}

dtPolyRef PathInfo::getPolyByLocation(const float* point, float *distance)
{
    // first we check the current path
    // if the current path doesn't contain the current poly,
    // we need to use the expensive navMesh.findNearestPoly
    dtPolyRef polyRef = getPathPolyByPosition(m_pathPolyRefs, m_polyLength, point, distance);
    if(polyRef != INVALID_POLYREF)
        return polyRef;

    // we don't have it in our old path
    // try to get it by findNearestPoly()
    // first try with low search box
    float extents[VERTEX_SIZE] = {3.0f, 5.0f, 3.0f};    // bounds of poly search area
    float closestPoint[VERTEX_SIZE] = {0.0f, 0.0f, 0.0f};
    dtStatus result = m_navMeshQuery->findNearestPoly(point, extents, &m_filter, &polyRef, closestPoint);
    if(DT_SUCCESS == result && polyRef != INVALID_POLYREF)
    {
        *distance = dtVdist(closestPoint, point);
        return polyRef;
    }

    // still nothing ..
    // try with bigger search box
    extents[1] = 200.0f;
    result = m_navMeshQuery->findNearestPoly(point, extents, &m_filter, &polyRef, closestPoint);
    if(DT_SUCCESS == result && polyRef != INVALID_POLYREF)
    {
        *distance = dtVdist(closestPoint, point);
        return polyRef;
    }

    return INVALID_POLYREF;
}

void PathInfo::BuildPolyPath(PathNode startPos, PathNode endPos)
{
    // *** getting start/end poly logic ***

    float distToStartPoly, distToEndPoly;
    float startPoint[VERTEX_SIZE] = {startPos.y, startPos.z, startPos.x};
    float endPoint[VERTEX_SIZE] = {endPos.y, endPos.z, endPos.x};

    dtPolyRef startPoly = getPolyByLocation(startPoint, &distToStartPoly);
    dtPolyRef endPoly = getPolyByLocation(endPoint, &distToEndPoly);

    // we have a hole in our mesh
    // make shortcut path and mark it as NOPATH ( with flying exception )
    // its up to caller how he will use this info
    if (startPoly == INVALID_POLYREF || endPoly == INVALID_POLYREF)
    {
        BuildShortcut();
        m_type = (m_sourceUnit->GetTypeId() == TYPEID_UNIT && ((Creature*)m_sourceUnit)->canFly())
                    ? PathType(PATHFIND_NORMAL | PATHFIND_NOT_USING_PATH) : PATHFIND_NOPATH;
        return;
    }

    // we may need a better number here
    bool farFromPoly = (distToStartPoly > 7.0f || distToEndPoly > 7.0f);
    if (farFromPoly)
    {

        bool buildShotrcut = false;
        if (m_sourceUnit->GetTypeId() == TYPEID_UNIT)
        {
            Creature* owner = (Creature*)m_sourceUnit;

            PathNode p = (distToStartPoly > 7.0f) ? startPos : endPos;
            if (m_sourceUnit->GetBaseMap()->IsUnderWater(p.x, p.y, p.z))
            {
                if (owner->canSwim())
                    buildShotrcut = true;
            }
            else
            {
                if (owner->canFly())
                    buildShotrcut = true;
            }
        }

        if (buildShotrcut)
        {
            BuildShortcut();
            m_type = PathType(PATHFIND_NORMAL | PATHFIND_NOT_USING_PATH);
            return;
        }
        else
        {
            float closestPoint[VERTEX_SIZE];
            // we may want to use closestPointOnPolyBoundary instead
            if (DT_SUCCESS == m_navMeshQuery->closestPointOnPoly(endPoly, endPoint, closestPoint))
            {
                dtVcopy(endPoint, closestPoint);
                setActualEndPosition(PathNode(endPoint[2],endPoint[0],endPoint[1]));
            }

            m_type = PATHFIND_INCOMPLETE;
        }
    }

    // *** poly path generating logic ***

    // start and end are on same polygon
    // just need to move in straight line
    if (startPoly == endPoly)
    {

        BuildShortcut();

        m_pathPolyRefs[0] = startPoly;
        m_polyLength = 1;

        m_type = farFromPoly ? PATHFIND_INCOMPLETE : PATHFIND_NORMAL;
        return;
    }

    // look for startPoly/endPoly in current path
    // TODO: we can merge it with getPathPolyByPosition() loop
    bool startPolyFound = false;
    bool endPolyFound = false;
    uint32 pathStartIndex, pathEndIndex;

    if (m_polyLength)
    {
        for (pathStartIndex = 0; pathStartIndex < m_polyLength; ++pathStartIndex)
        {
            // here to carch few bugs
            assert(m_pathPolyRefs[pathStartIndex] != INVALID_POLYREF);
            
            if (m_pathPolyRefs[pathStartIndex] == startPoly)
            {
                startPolyFound = true;
                break;
            }
        }

        for (pathEndIndex = m_polyLength-1; pathEndIndex > pathStartIndex; --pathEndIndex)
            if (m_pathPolyRefs[pathEndIndex] == endPoly)
            {
                endPolyFound = true;
                break;
            }
    }

    if (startPolyFound && endPolyFound)
    {

        // we moved along the path and the target did not move out of our old poly-path
        // our path is a simple subpath case, we have all the data we need
        // just "cut" it out

        m_polyLength = pathEndIndex - pathStartIndex + 1;
        memmove(m_pathPolyRefs, m_pathPolyRefs+pathStartIndex, m_polyLength*sizeof(dtPolyRef));
    }
    else if (startPolyFound && !endPolyFound)
    {

        // we are moving on the old path but target moved out
        // so we have atleast part of poly-path ready

        m_polyLength -= pathStartIndex;

        // try to adjust the suffix of the path instead of recalculating entire length
        // at given interval the target cannot get too far from its last location
        // thus we have less poly to cover
        // sub-path of optimal path is optimal

        // take ~80% of the original length
        // TODO : play with the values here
        uint32 prefixPolyLength = uint32(m_polyLength*0.8f + 0.5f);
        memmove(m_pathPolyRefs, m_pathPolyRefs+pathStartIndex, prefixPolyLength*sizeof(dtPolyRef));

        dtPolyRef suffixStartPoly = m_pathPolyRefs[prefixPolyLength-1];

        // we need any point on our suffix start poly to generate poly-path, so we need last poly in prefix data
        float suffixEndPoint[VERTEX_SIZE];
        if (DT_SUCCESS != m_navMeshQuery->closestPointOnPoly(suffixStartPoly, endPoint, suffixEndPoint))
        {
            // suffixStartPoly is invalid somehow, or the navmesh is broken => error state
            sLog.outError("%u's Path Build failed: invalid polyRef in path", m_sourceUnit->GetGUID());

            BuildShortcut();
            m_type = PATHFIND_NOPATH;
            return;
        }

        // generate suffix
        uint32 suffixPolyLength = 0;
        dtStatus dtResult = m_navMeshQuery->findPath(
                                suffixStartPoly,    // start polygon
                                endPoly,            // end polygon
                                suffixEndPoint,     // start position
                                endPoint,           // end position
                                &m_filter,            // polygon search filter
                                m_pathPolyRefs + prefixPolyLength - 1,    // [out] path
                                (int*)&suffixPolyLength,
                                MAX_PATH_LENGTH-prefixPolyLength);   // max number of polygons in output path

        if (!suffixPolyLength || dtResult != DT_SUCCESS)
        {
            // this is probably an error state, but we'll leave it
            // and hopefully recover on the next Update
            // we still need to copy our preffix
            sLog.outError("%u's Path Build failed: 0 length path", m_sourceUnit->GetGUID());
        }

        // new path = prefix + suffix - overlap
        m_polyLength = prefixPolyLength + suffixPolyLength - 1;
    }
    else
    {

        // either we have no path at all -> first run
        // or something went really wrong -> we aren't moving along the path to the target
        // just generate new path

        // free and invalidate old path data
        clear();

        dtStatus dtResult = m_navMeshQuery->findPath(
                startPoly,          // start polygon
                endPoly,            // end polygon
                startPoint,         // start position
                endPoint,           // end position
                &m_filter,           // polygon search filter
                m_pathPolyRefs,     // [out] path
                (int*)&m_polyLength,
                MAX_PATH_LENGTH);   // max number of polygons in output path

        if (!m_polyLength || dtResult != DT_SUCCESS)
        {
            // only happens if we passed bad data to findPath(), or navmesh is messed up
            sLog.outError("%u's Path Build failed: 0 length path", m_sourceUnit->GetGUID());
            BuildShortcut();
            m_type = PATHFIND_NOPATH;
            return;
        }
    }

    // by now we know what type of path we can get
    if (m_pathPolyRefs[m_polyLength - 1] == endPoly && !(m_type & PATHFIND_INCOMPLETE))
        m_type = PATHFIND_NORMAL;
    else
        m_type = PATHFIND_INCOMPLETE;

    // generate the point-path out of our up-to-date poly-path
    BuildPointPath(startPoint, endPoint);
}

void PathInfo::BuildPointPath(float *startPoint, float *endPoint)
{
    float pathPoints[MAX_POINT_PATH_LENGTH*VERTEX_SIZE];
    uint32 pointCount = 0;
    dtStatus dtResult = DT_FAILURE;
    bool usedOffmesh = false;
    if (m_useStraightPath)
    {
        dtResult = m_navMeshQuery->findStraightPath(
                startPoint,         // start position
                endPoint,           // end position
                m_pathPolyRefs,     // current path
                m_polyLength,       // lenth of current path
                pathPoints,         // [out] path corner points
                NULL,               // [out] flags
                NULL,               // [out] shortened path
                (int*)&pointCount,
                MAX_POINT_PATH_LENGTH);   // maximum number of points/polygons to use
    }
    else
    {
        dtResult = findSmoothPath(
                startPoint,         // start position
                endPoint,           // end position
                m_pathPolyRefs,     // current path
                m_polyLength,       // length of current path
                pathPoints,         // [out] path corner points
                (int*)&pointCount,
                usedOffmesh,
                MAX_POINT_PATH_LENGTH);    // maximum number of points
    }

    if (pointCount < 2 || dtResult != DT_SUCCESS)
    {
        // only happens if pass bad data to findStraightPath or navmesh is broken
        // single point paths can be generated here
        // TODO : check the exact cases
        BuildShortcut();
        m_type = PATHFIND_NOPATH;
        return;
    }

    // we need to flash our poly path to prevent it being used as subpath next cycle
    // in case of off mesh connection was used
    if(usedOffmesh)
        m_polyLength = 0;

    m_pathPoints.resize(pointCount);
    for (uint32 i = 0; i < pointCount; ++i)
        m_pathPoints.set(i, PathNode(pathPoints[i*VERTEX_SIZE+2], pathPoints[i*VERTEX_SIZE], pathPoints[i*VERTEX_SIZE+1]));

    // first point is always our current location - we need the next one
    setNextPosition(m_pathPoints[1]);
    setActualEndPosition(m_pathPoints[pointCount-1]);

    // force the given destination, if needed
    if(m_forceDestination &&
        (!(m_type & PATHFIND_NORMAL) || !inRange(getEndPosition(), getActualEndPosition(), 1.0f, 1.0f)))
    {
        setActualEndPosition(getEndPosition());
        BuildShortcut();
        m_type = PathType(PATHFIND_NORMAL | PATHFIND_NOT_USING_PATH);
        return;
    }
}

void PathInfo::BuildShortcut()
{
    clear();

    // make two point path, our curr pos is the start, and dest is the end
    m_pathPoints.resize(2);

    // set start and a default next position
    m_pathPoints.set(0, getStartPosition());
    m_pathPoints.set(1, getActualEndPosition());

    setNextPosition(getActualEndPosition());
    m_type = PATHFIND_SHORTCUT;
}

void PathInfo::createFilter()
{
    unsigned short includeFlags = 0;
    unsigned short excludeFlags = 0;

    if (m_sourceUnit->GetTypeId() == TYPEID_UNIT)
    {
        Creature* creature = (Creature*)m_sourceUnit;
        if (creature->canWalk())
            includeFlags |= NAV_GROUND;          // walk

        // creatures don't take environmental damage
        if (creature->canSwim())
            includeFlags |= (NAV_WATER | NAV_MAGMA | NAV_SLIME);           // swim
    }
    else if (m_sourceUnit->GetTypeId() == TYPEID_PLAYER)
    {
        // perfect support not possible, just stay 'safe'
        includeFlags |= (NAV_GROUND | NAV_WATER);
    }

    m_filter.setIncludeFlags(includeFlags);
    m_filter.setExcludeFlags(excludeFlags);

    updateFilter();
}

void PathInfo::updateFilter()
{
    // allow creatures to cheat and use different movement types if they are moved
    // forcefully into terrain they can't normally move in
    if (m_sourceUnit->IsInWater() || m_sourceUnit->IsUnderWater())
    {
        unsigned short includedFlags = m_filter.getIncludeFlags();
        includedFlags |= getNavTerrain(m_sourceUnit->GetPositionX(),
                                       m_sourceUnit->GetPositionY(),
                                       m_sourceUnit->GetPositionZ());

        m_filter.setIncludeFlags(includedFlags);
    }
}

NavTerrain PathInfo::getNavTerrain(float x, float y, float z)
{
    LiquidData data;
    m_sourceUnit->GetBaseMap()->getLiquidStatus(x, y, z, MAP_ALL_LIQUIDS, &data);

    switch (data.type)
    {
        case MAP_LIQUID_TYPE_WATER:
        case MAP_LIQUID_TYPE_OCEAN:
            return NAV_WATER;
        case MAP_LIQUID_TYPE_MAGMA:
            return NAV_MAGMA;
        case MAP_LIQUID_TYPE_SLIME:
            return NAV_SLIME;
        default:
            return NAV_GROUND;
    }
}

uint32 PathInfo::fixupCorridor(dtPolyRef* path, const uint32 npath, const uint32 maxPath,
                               const dtPolyRef* visited, const uint32 nvisited)
{
    int32 furthestPath = -1;
    int32 furthestVisited = -1;

    // Find furthest common polygon.
    for (int32 i = npath-1; i >= 0; --i)
    {
        bool found = false;
        for (int32 j = nvisited-1; j >= 0; --j)
        {
            if (path[i] == visited[j])
            {
                furthestPath = i;
                furthestVisited = j;
                found = true;
            }
        }
        if (found)
            break;
    }

    // If no intersection found just return current path.
    if (furthestPath == -1 || furthestVisited == -1)
        return npath;

    // Concatenate paths.

    // Adjust beginning of the buffer to include the visited.
    uint32 req = nvisited - furthestVisited;
    uint32 orig = uint32(furthestPath+1) < npath ? furthestPath+1 : npath;
    uint32 size = npath-orig > 0 ? npath-orig : 0;
    if (req+size > maxPath)
        size = maxPath-req;

    if (size)
        memmove(path+req, path+orig, size*sizeof(dtPolyRef));

    // Store visited
    for (uint32 i = 0; i < req; ++i)
        path[i] = visited[(nvisited-1)-i];

    return req+size;
}

bool PathInfo::getSteerTarget(const float* startPos, const float* endPos,
                              const float minTargetDist, const dtPolyRef* path, const uint32 pathSize,
                              float* steerPos, unsigned char& steerPosFlag, dtPolyRef& steerPosRef)
{
    // Find steer target.
    static const uint32 MAX_STEER_POINTS = 3;
    float steerPath[MAX_STEER_POINTS*VERTEX_SIZE];
    unsigned char steerPathFlags[MAX_STEER_POINTS];
    dtPolyRef steerPathPolys[MAX_STEER_POINTS];
    uint32 nsteerPath = 0;
    dtStatus dtResult = m_navMeshQuery->findStraightPath(startPos, endPos, path, pathSize,
                                                steerPath, steerPathFlags, steerPathPolys, (int*)&nsteerPath, MAX_STEER_POINTS);
    if (!nsteerPath || DT_SUCCESS != dtResult)
        return false;

    // Find vertex far enough to steer to.
    uint32 ns = 0;
    while (ns < nsteerPath)
    {
        // Stop at Off-Mesh link or when point is further than slop away.
        if ((steerPathFlags[ns] & DT_STRAIGHTPATH_OFFMESH_CONNECTION) ||
            !inRangeYZX(&steerPath[ns*VERTEX_SIZE], startPos, minTargetDist, 1000.0f))
            break;
        ns++;
    }
    // Failed to find good point to steer to.
    if (ns >= nsteerPath)
        return false;

    dtVcopy(steerPos, &steerPath[ns*VERTEX_SIZE]);
    steerPos[1] = startPos[1];  // keep Z value
    steerPosFlag = steerPathFlags[ns];
    steerPosRef = steerPathPolys[ns];

    return true;
}

dtStatus PathInfo::findSmoothPath(const float* startPos, const float* endPos,
                                     const dtPolyRef* polyPath, const uint32 polyPathSize,
                                     float* smoothPath, int* smoothPathSize, bool &usedOffmesh, const uint32 maxSmoothPathSize)
{
    assert(polyPathSize <= MAX_PATH_LENGTH);
    *smoothPathSize = 0;
    uint32 nsmoothPath = 0;
    usedOffmesh = false;

    dtPolyRef polys[MAX_PATH_LENGTH];
    memcpy(polys, polyPath, sizeof(dtPolyRef)*polyPathSize);
    uint32 npolys = polyPathSize;

    float iterPos[VERTEX_SIZE], targetPos[VERTEX_SIZE];
    if(DT_SUCCESS != m_navMeshQuery->closestPointOnPolyBoundary(polys[0], startPos, iterPos))
        return DT_FAILURE;

    if(DT_SUCCESS != m_navMeshQuery->closestPointOnPolyBoundary(polys[npolys-1], endPos, targetPos))
        return DT_FAILURE;

    dtVcopy(&smoothPath[nsmoothPath*VERTEX_SIZE], iterPos);
    nsmoothPath++;

    // Move towards target a small advancement at a time until target reached or
    // when ran out of memory to store the path.
    while (npolys && nsmoothPath < maxSmoothPathSize)
    {
        // Find location to steer towards.
        float steerPos[VERTEX_SIZE];
        unsigned char steerPosFlag;
        dtPolyRef steerPosRef = INVALID_POLYREF;

        if (!getSteerTarget(iterPos, targetPos, SMOOTH_PATH_SLOP, polys, npolys, steerPos, steerPosFlag, steerPosRef))
            break;

        bool endOfPath = (steerPosFlag & DT_STRAIGHTPATH_END);
        bool offMeshConnection = (steerPosFlag & DT_STRAIGHTPATH_OFFMESH_CONNECTION);

        // Find movement delta.
        float delta[VERTEX_SIZE];
        dtVsub(delta, steerPos, iterPos);
        float len = dtSqrt(dtVdot(delta,delta));
        // If the steer target is end of path or off-mesh link, do not move past the location.
        if ((endOfPath || offMeshConnection) && len < SMOOTH_PATH_STEP_SIZE)
            len = 1.0f;
        else
            len = SMOOTH_PATH_STEP_SIZE / len;

        float moveTgt[VERTEX_SIZE];
        dtVmad(moveTgt, iterPos, delta, len);

        // Move
        float result[VERTEX_SIZE];
        const static uint32 MAX_VISIT_POLY = 16;
        dtPolyRef visited[MAX_VISIT_POLY];

        uint32 nvisited = 0;
        m_navMeshQuery->moveAlongSurface(polys[0], iterPos, moveTgt, &m_filter, result, visited, (int*)&nvisited, MAX_VISIT_POLY);
        npolys = fixupCorridor(polys, npolys, MAX_PATH_LENGTH, visited, nvisited);

        m_navMeshQuery->getPolyHeight(polys[0], result, &result[1]);
        dtVcopy(iterPos, result);

        // Handle end of path and off-mesh links when close enough.
        if (endOfPath && inRangeYZX(iterPos, steerPos, SMOOTH_PATH_SLOP, 2.0f))
        {
            // Reached end of path.
            dtVcopy(iterPos, targetPos);
            if (nsmoothPath < maxSmoothPathSize)
            {
                dtVcopy(&smoothPath[nsmoothPath*VERTEX_SIZE], iterPos);
                nsmoothPath++;
            }
            break;
        }
        else if (offMeshConnection && inRangeYZX(iterPos, steerPos, SMOOTH_PATH_SLOP, 2.0f))
        {
            // Reached off-mesh connection.
            usedOffmesh = true;

            // Advance the path up to and over the off-mesh connection.
            dtPolyRef prevRef = INVALID_POLYREF;
            dtPolyRef polyRef = polys[0];
            uint32 npos = 0;
            while (npos < npolys && polyRef != steerPosRef)
            {
                prevRef = polyRef;
                polyRef = polys[npos];
                npos++;
            }

            for (uint32 i = npos; i < npolys; ++i)
                polys[i-npos] = polys[i];

            npolys -= npos;

            // Handle the connection.
            float startPos[VERTEX_SIZE], endPos[VERTEX_SIZE];
            if (DT_SUCCESS == m_navMesh->getOffMeshConnectionPolyEndPoints(prevRef, polyRef, startPos, endPos))
            {
                if (nsmoothPath < maxSmoothPathSize)
                {
                    dtVcopy(&smoothPath[nsmoothPath*VERTEX_SIZE], startPos);
                    nsmoothPath++;
                }
                // Move position at the other side of the off-mesh link.
                dtVcopy(iterPos, endPos);
                m_navMeshQuery->getPolyHeight(polys[0], iterPos, &iterPos[1]);
            }
        }

        // Store results.
        if (nsmoothPath < maxSmoothPathSize)
        {
            dtVcopy(&smoothPath[nsmoothPath*VERTEX_SIZE], iterPos);
            nsmoothPath++;
        }
    }

    *smoothPathSize = nsmoothPath;

    // this is most likely loop
    return nsmoothPath < maxSmoothPathSize ? DT_SUCCESS : DT_FAILURE;
}
