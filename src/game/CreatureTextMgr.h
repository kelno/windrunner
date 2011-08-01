/*
* Copyright (C) 2008-2010 Trinity <http://www.trinitycore.org/>
*
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

#ifndef TRINITY_CREATURE_TEXT_MGR_H
#define TRINITY_CREATURE_TEXT_MGR_H

#include "Creature.h"
#include "SharedDefines.h"

/*
N O T E S

*/

struct CreatureTextEntry
{
    uint32 entry;
    uint8 group;
    uint8 id;
    std::string text;
    ChatType type;
    Language lang;
    float probability;
    Emote emote;
    uint32 duration;
    uint32 sound;
};


typedef std::vector<CreatureTextEntry> CreatureTextGroup; //texts in a group
typedef UNORDERED_MAP<uint32, CreatureTextGroup> CreatureTextHolder; //groups for a creature
typedef UNORDERED_MAP<uint32, CreatureTextHolder> CreatureTextMap; //all creatures

class CreatureTextMgr
{
    //friend class ACE_Singleton<CreatureTextMgr, ACE_Null_Mutex>;
    public:
        CreatureTextMgr() {};
        ~CreatureTextMgr() {};
        void LoadCreatureTexts();
        CreatureTextMap const& GetTextMap() const { return mTextMap; }

    private:
        CreatureTextMap mTextMap;
};

#define sCreatureTextMgr Trinity::Singleton<CreatureTextMgr>::Instance()
#endif
