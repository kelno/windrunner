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

#ifndef MANGOS_PATH_FINDER_H
#define MANGOS_PATH_FINDER_H

#include "Path.h"
#include "MoveMapSharedDefines.h"
#include "../recastnavigation/Detour/Include/DetourNavMesh.h"
#include "../recastnavigation/Detour/Include/DetourNavMeshQuery.h"

class Unit;

// 64*6.0f=384y  number_of_points*interval = max_path_len
// this is way more than actual evade range
// I think we can safely cut those down even more
#define MAX_PATH_LENGTH         64
#define MAX_POINT_PATH_LENGTH   64

#define SMOOTH_PATH_STEP_SIZE   6.0f
#define SMOOTH_PATH_SLOP        0.4f

#define VERTEX_SIZE       3
#define INVALID_POLYREF   0

enum PathType
{
    PATHFIND_BLANK          = 0x0000,   // path not built yet
    PATHFIND_NORMAL         = 0x0001,   // normal path
    PATHFIND_SHORTCUT       = 0x0002,   // travel through obstacles, terrain, air, etc (old behavior)
    PATHFIND_INCOMPLETE     = 0x0004,   // we have partial path to follow - getting closer to target
    PATHFIND_NOPATH         = 0x0008,   // no valid path at all or error in generating one
    PATHFIND_NOT_USING_PATH = 0x0010    // used when we are either flying/swiming or on map w/o mmaps
};

class PathInfo
{
    public:
        PathInfo(Unit const* owner, const float destX, const float destY, const float destZ,
                 bool useStraightPath = false, bool forceDest = false);
        ~PathInfo();

        // return value : true if new path was calculated
        bool Update(const float destX, const float destY, const float destZ,
                    bool useStraightPath = false, bool forceDest = false);

        inline void getStartPosition(float &x, float &y, float &z) { x = m_startPosition.x; y = m_startPosition.y; z = m_startPosition.z; }
        inline void getNextPosition(float &x, float &y, float &z) { x = m_nextPosition.x; y = m_nextPosition.y; z = m_nextPosition.z; }
        inline void getEndPosition(float &x, float &y, float &z) { x = m_endPosition.x; y = m_endPosition.y; z = m_endPosition.z; }
        inline void getActualEndPosition(float &x, float &y, float &z) { x = m_actualEndPosition.x; y = m_actualEndPosition.y; z = m_actualEndPosition.z; }

        inline PathNode getStartPosition() const { return m_startPosition; }
        inline PathNode getNextPosition() const { return m_nextPosition; }
        inline PathNode getEndPosition() const { return m_endPosition; }
        inline PathNode getActualEndPosition() const { return m_actualEndPosition; }

        inline PointPath& getFullPath() { return m_pathPoints; }
        inline PathType getPathType() const { return m_type; }
        
        void BuildShortcut();

    private:

        dtPolyRef       m_pathPolyRefs[MAX_PATH_LENGTH];   // array of detour polygon references
        uint32          m_polyLength;                      // number of polygons in the path

        PointPath       m_pathPoints;       // our actual (x,y,z) path to the target
        PathType        m_type;             // tells what kind of path this is

        bool            m_useStraightPath;  // type of path will be generated
        bool            m_forceDestination; // when set, we will always arrive at given point

        PathNode        m_startPosition;    // {x, y, z} of current location
        PathNode        m_nextPosition;     // {x, y, z} of next location on the path
        PathNode        m_endPosition;      // {x, y, z} of the destination
        PathNode        m_actualEndPosition;  // {x, y, z} of the closest possible point to given destination

        const Unit* const       m_sourceUnit;       // the unit that is moving
        const dtNavMesh*        m_navMesh;          // the nav mesh
        const dtNavMeshQuery*   m_navMeshQuery;     // the nav mesh query used to find the path

        dtQueryFilter m_filter;                     // use single filter for all movements, update it when needed

        inline void setNextPosition(PathNode point) { m_nextPosition = point; }
        inline void setStartPosition(PathNode point) { m_startPosition = point; }
        inline void setEndPosition(PathNode point) { m_actualEndPosition = point; m_endPosition = point; }
        inline void setActualEndPosition(PathNode point) { m_actualEndPosition = point; }

        inline void clear()
        {
            m_polyLength = 0;
            m_pathPoints.clear();
        }

        dtPolyRef getPathPolyByPosition(dtPolyRef *polyPath, uint32 polyPathSize, const float* point, float *distance = NULL);
        dtPolyRef getPolyByLocation(const float* point, float *distance);

        void BuildPolyPath(PathNode startPos, PathNode endPos);
        void BuildPointPath(float *startPoint, float *endPoint);

        NavTerrain getNavTerrain(float x, float y, float z);
        void createFilter();
        void updateFilter();

        // smooth path functions
        uint32 fixupCorridor(dtPolyRef* path, const uint32 npath, const uint32 maxPath,
                             const dtPolyRef* visited, const uint32 nvisited);
        bool getSteerTarget(const float* startPos, const float* endPos, const float minTargetDist,
                            const dtPolyRef* path, const uint32 pathSize, float* steerPos,
                            unsigned char& steerPosFlag, dtPolyRef& steerPosRef);
        dtStatus findSmoothPath(const float* startPos, const float* endPos,
                              const dtPolyRef* polyPath, const uint32 polyPathSize,
                              float* smoothPath, int* smoothPathSize, bool &usedOffmesh,
                              const uint32 smoothPathMaxSize);
};

inline bool inRangeYZX(const float* v1, const float* v2, const float r, const float h)
{
    const float dx = v2[0] - v1[0];
    const float dy = v2[1] - v1[1]; // elevation
    const float dz = v2[2] - v1[2];
    return (dx*dx + dz*dz) < r*r && fabsf(dy) < h;
}

inline bool inRange(const PathNode p1, const PathNode p2,
                    const float r, const float h)
{
    const float dx = p2.x - p1.x;
    const float dy = p2.y - p1.y;
    const float dz = p2.z - p1.z;
    return (dx*dx + dy*dy) < r*r && fabsf(dz) < h;
}

#endif
