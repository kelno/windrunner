/*
 * Copyright (C) 2005-2010 MaNGOS <http://getmangos.com/>
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

#ifndef _VMAPDEFINITIONS_H
#define _VMAPDEFINITIONS_H
#include <cstring>

#define LIQUID_TILE_SIZE (533.333f / 128.f)

namespace VMAP
{
    //=====================================
    #define MAX_CAN_FALL_DISTANCE 10.0f
    const char VMAP_MAGIC[] = "VMAP_3.0";

    class VMapDefinitions
    {
        public:
            static float getMaxCanFallDistance() { return MAX_CAN_FALL_DISTANCE; }
    };

    //======================================

    // defined in TileAssembler.cpp currently...
    bool readChunk(FILE *rf, char *dest, const char *compare, uint32 len);
}

#ifndef NO_CORE_FUNCS
    #include "Errors.h"
    #include "Log.h"
    #define ERROR_LOG(...) sLog.outError(__VA_ARGS__);
    #define DETAIL_LOG(...) sLog.outDetail(__VA_ARGS__);
#else
    #include <assert.h>
    #define ASSERT(x) assert(x)
    #define DEBUG_LOG(...) do{ printf(__VA_ARGS__); printf("\n"); } while(0)
    #define DETAIL_LOG(...) do{ printf(__VA_ARGS__); printf("\n"); } while(0)
    #define ERROR_LOG(...) do{ printf("ERROR:"); printf(__VA_ARGS__); printf("\n"); } while(0)
#endif

#endif // _VMAPDEFINITIONS_H
