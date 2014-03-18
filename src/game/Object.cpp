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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "Common.h"
#include "SharedDefines.h"
#include "WorldPacket.h"
#include "Opcodes.h"
#include "Log.h"
#include "World.h"
#include "Object.h"
#include "Creature.h"
#include "Player.h"
#include "Totem.h"
#include "ObjectMgr.h"
#include "WorldSession.h"
#include "UpdateData.h"
#include "UpdateMask.h"
#include "Util.h"
#include "MapManager.h"
#include "ObjectAccessor.h"
#include "Log.h"
#include "Transports.h"
#include "TargetedMovementGenerator.h"
#include "WaypointMovementGenerator.h"
#include "VMapFactory.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CreatureAINew.h"

#include "TemporarySummon.h"
#include "DynamicTree.h"

uint32 GuidHigh2TypeId(uint32 guid_hi)
{
    switch(guid_hi)
    {
        case HIGHGUID_ITEM:         return TYPEID_ITEM;
        //case HIGHGUID_CONTAINER:    return TYPEID_CONTAINER; HIGHGUID_CONTAINER==HIGHGUID_ITEM currently
        case HIGHGUID_UNIT:         return TYPEID_UNIT;
        case HIGHGUID_PET:          return TYPEID_UNIT;
        case HIGHGUID_PLAYER:       return TYPEID_PLAYER;
        case HIGHGUID_GAMEOBJECT:   return TYPEID_GAMEOBJECT;
        case HIGHGUID_DYNAMICOBJECT:return TYPEID_DYNAMICOBJECT;
        case HIGHGUID_CORPSE:       return TYPEID_CORPSE;
        case HIGHGUID_MO_TRANSPORT: return TYPEID_GAMEOBJECT;
    }
    return 10;                                              // unknown
}

Object::Object( )
{
    m_objectTypeId      = TYPEID_OBJECT;
    m_objectType        = TYPEMASK_OBJECT;

    m_uint32Values      = 0;
    m_uint32Values_mirror = 0;
    m_valuesCount       = 0;
    m_updateFlag        = 0;

    m_inWorld           = false;
    m_objectUpdated     = false;

    m_PackGUID.clear();
    m_PackGUID.appendPackGUID(0);
}

Object::~Object( )
{
    //if(m_objectUpdated)
    //    ObjectAccessor::Instance().RemoveUpdateObject(this);

    if(m_uint32Values)
    {
        if(IsInWorld())
        {
            ///- Do NOT call RemoveFromWorld here, if the object is a player it will crash
            sLog.outError("Object::~Object - guid=" I64FMTD ", typeid=%d deleted but still in world!!", GetGUID(), GetTypeId());
            assert(false);
        }

        assert(!m_objectUpdated);

        //DEBUG_LOG("Object desctr 1 check (%p)",(void*)this);
        delete [] m_uint32Values;
        delete [] m_uint32Values_mirror;
        m_uint32Values = NULL;
        m_uint32Values_mirror = NULL;
        //DEBUG_LOG("Object desctr 2 check (%p)",(void*)this);
    }
}

void Object::_InitValues()
{
    m_uint32Values = new uint32[ m_valuesCount ];
    memset(m_uint32Values, 0, m_valuesCount*sizeof(uint32));

    m_uint32Values_mirror = new uint32[ m_valuesCount ];
    memset(m_uint32Values_mirror, 0, m_valuesCount*sizeof(uint32));

    m_objectUpdated = false;
}

void Object::_Create( uint32 guidlow, uint32 entry, HighGuid guidhigh )
{
    if(!m_uint32Values) _InitValues();

    uint64 guid = MAKE_NEW_GUID(guidlow, entry, guidhigh);  // required more changes to make it working
    SetUInt64Value( OBJECT_FIELD_GUID, guid );
    SetUInt32Value( OBJECT_FIELD_TYPE, m_objectType );
    m_PackGUID.clear();
    m_PackGUID.appendPackGUID(GetGUID());
}

void Object::BuildMovementUpdateBlock(UpdateData * data, uint32 flags ) const
{
    ByteBuffer buf(500);

    buf << uint8( UPDATETYPE_MOVEMENT );
    buf << GetGUID();

    _BuildMovementUpdate(&buf, flags, 0x00000000);

    data->AddUpdateBlock(buf);
}

void Object::BuildCreateUpdateBlockForPlayer(UpdateData *data, Player *target) const
{
    if(!target)
    {
        return;
    }

    uint8  updatetype = UPDATETYPE_CREATE_OBJECT;
    uint8  flags      = m_updateFlag;
    uint32 flags2     = 0;

    /** lower flag1 **/
    if(target == this)                                      // building packet for oneself
        flags |= UPDATEFLAG_SELF;

    if(flags & UPDATEFLAG_HASPOSITION)
    {
        // UPDATETYPE_CREATE_OBJECT2 dynamic objects, corpses...
        if(isType(TYPEMASK_DYNAMICOBJECT) || isType(TYPEMASK_CORPSE) || isType(TYPEMASK_PLAYER))
            updatetype = UPDATETYPE_CREATE_OBJECT2;

        // UPDATETYPE_CREATE_OBJECT2 for pets...
        if(target->GetPetGUID() == GetGUID())
            updatetype = UPDATETYPE_CREATE_OBJECT2;

        // UPDATETYPE_CREATE_OBJECT2 for some gameobject types...
        if(isType(TYPEMASK_GAMEOBJECT))
        {
            switch(((GameObject*)this)->GetGoType())
            {
                case GAMEOBJECT_TYPE_TRAP:
                case GAMEOBJECT_TYPE_DUEL_ARBITER:
                case GAMEOBJECT_TYPE_FLAGSTAND:
                case GAMEOBJECT_TYPE_FLAGDROP:
                    updatetype = UPDATETYPE_CREATE_OBJECT2;
                    break;
                case GAMEOBJECT_TYPE_TRANSPORT:
                    flags |= UPDATEFLAG_TRANSPORT;
                    break;
            }
        }
    }

    //sLog.outDebug("BuildCreateUpdate: update-type: %u, object-type: %u got flags: %X, flags2: %X", updatetype, m_objectTypeId, flags, flags2);

    ByteBuffer buf(500);
    buf << (uint8)updatetype;
    //buf.append(GetPackGUID());    //client crashes when using this
    buf << (uint8)0xFF << GetGUID();
    buf << (uint8)m_objectTypeId;

    _BuildMovementUpdate(&buf, flags, flags2);

    UpdateMask updateMask;
    updateMask.SetCount( m_valuesCount );
    _SetCreateBits( &updateMask, target );
    _BuildValuesUpdate(updatetype, &buf, &updateMask, target );
    data->AddUpdateBlock(buf);
}

void Object::BuildUpdate(UpdateDataMapType &update_players)
{
    ObjectAccessor::_buildUpdateObject(this,update_players);
    ClearUpdateMask(true);
}

void Object::SendUpdateToPlayer(Player* player)
{
    // send update to another players
    SendUpdateObjectToAllExcept(player);

    // send create update to player
    UpdateData upd;
    WorldPacket packet;

    BuildCreateUpdateBlockForPlayer(&upd, player);
    upd.BuildPacket(&packet);
    player->GetSession()->SendPacket(&packet);

    // now object updated/(create updated)
}

void Object::BuildValuesUpdateBlockForPlayer(UpdateData *data, Player *target) const
{
    ByteBuffer buf(500);

    buf << (uint8) UPDATETYPE_VALUES;
    //buf.append(GetPackGUID());    //client crashes when using this. but not have crash in debug mode
    buf << (uint8)0xFF;
    buf << GetGUID();

    UpdateMask updateMask;
    updateMask.SetCount( m_valuesCount );

    _SetUpdateBits( &updateMask, target );
    _BuildValuesUpdate(UPDATETYPE_VALUES, &buf, &updateMask, target );

    data->AddUpdateBlock(buf);
}

void Object::BuildOutOfRangeUpdateBlock(UpdateData * data) const
{
    data->AddOutOfRangeGUID(GetGUID());
}

void Object::DestroyForPlayer(Player *target) const
{
    ASSERT(target);

    WorldPacket data(SMSG_DESTROY_OBJECT, 8);
    data << GetGUID();
    target->GetSession()->SendPacket( &data );
}

void Object::_BuildMovementUpdate(ByteBuffer * data, uint8 flags, uint32 flags2 ) const
{
    *data << (uint8)flags;                                  // update flags

    // 0x20
    if (flags & UPDATEFLAG_LIVING)
    {
        switch(GetTypeId())
        {
            case TYPEID_UNIT:
            {
                flags2 = ((Unit*)this)->GetUnitMovementFlags();
                flags2 &= ~MOVEMENTFLAG_ONTRANSPORT;
                flags2 &= ~MOVEMENTFLAG_SPLINE_ENABLED;
            }
            break;
            case TYPEID_PLAYER:
            {
                Player *player = const_cast<Player*>(ToPlayer());

                flags2 = player->GetUnitMovementFlags();

                if(player->GetTransport())
                    flags2 |= MOVEMENTFLAG_ONTRANSPORT;
                else
                    flags2 &= ~MOVEMENTFLAG_ONTRANSPORT;

                // remove unknown, unused etc flags for now
                flags2 &= ~MOVEMENTFLAG_SPLINE_ENABLED;            // will be set manually

                if(player->isInFlight())
                {
                    WPAssert(player->GetMotionMaster()->GetCurrentMovementGeneratorType() == FLIGHT_MOTION_TYPE);
                    flags2 = (MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_SPLINE_ENABLED);
                }
            }
            break;
        }

        *data << uint32(flags2);                            // movement flags
        *data << uint8(0);                                  // unk 2.3.0
        *data << uint32(getMSTime());                       // time (in milliseconds)
    }

    // 0x40
    if (flags & UPDATEFLAG_HASPOSITION)
    {
        // 0x02
        if((flags & UPDATEFLAG_TRANSPORT) && ((GameObject*)this)->GetGoType() == GAMEOBJECT_TYPE_MO_TRANSPORT)
        {
            *data << (float)0;
            *data << (float)0;
            *data << (float)0;
            *data << ((WorldObject *)this)->GetOrientation();
        }
        else
        {
            *data << ((WorldObject *)this)->GetPositionX();
            *data << ((WorldObject *)this)->GetPositionY();
            *data << ((WorldObject *)this)->GetPositionZ();
            *data << ((WorldObject *)this)->GetOrientation();
        }
    }

    // 0x20
    if(flags & UPDATEFLAG_LIVING)
    {
        // 0x00000200
        if(flags2 & MOVEMENTFLAG_ONTRANSPORT)
        {
            if(GetTypeId() == TYPEID_PLAYER)
            {
                *data << (uint64)(this->ToPlayer())->GetTransport()->GetGUID();
                *data << (float)(this->ToPlayer())->GetTransOffsetX();
                *data << (float)(this->ToPlayer())->GetTransOffsetY();
                *data << (float)(this->ToPlayer())->GetTransOffsetZ();
                *data << (float)(this->ToPlayer())->GetTransOffsetO();
                *data << (uint32)(this->ToPlayer())->GetTransTime();
            }
            //TrinIty currently not have support for other than player on transport
        }

        // 0x02200000
        if(flags2 & (MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING2))
        {
            if(GetTypeId() == TYPEID_PLAYER)
                *data << (float)(this->ToPlayer())->m_movementInfo.s_pitch;
            else
                *data << (float)0;                          // is't part of movement packet, we must store and send it...
        }

        if(GetTypeId() == TYPEID_PLAYER)
            *data << (uint32)(this->ToPlayer())->m_movementInfo.fallTime;
        else
            *data << (uint32)0;                             // last fall time

        // 0x00001000
        if(flags2 & MOVEMENTFLAG_JUMPING)
        {
            if(GetTypeId() == TYPEID_PLAYER)
            {
                *data << (float)(this->ToPlayer())->m_movementInfo.j_unk;
                *data << (float)(this->ToPlayer())->m_movementInfo.j_sinAngle;
                *data << (float)(this->ToPlayer())->m_movementInfo.j_cosAngle;
                *data << (float)(this->ToPlayer())->m_movementInfo.j_xyspeed;
            }
            else
            {
                *data << (float)0;
                *data << (float)0;
                *data << (float)0;
                *data << (float)0;
            }
        }

        // 0x04000000
        if(flags2 & MOVEMENTFLAG_SPLINE_ELEVATION)
        {
            if(GetTypeId() == TYPEID_PLAYER)
                *data << (float)(this->ToPlayer())->m_movementInfo.u_unk1;
            else
                *data << (float)0;
        }

        *data << ((Unit*)this)->GetSpeed( MOVE_WALK );
        *data << ((Unit*)this)->GetSpeed( MOVE_RUN );
        *data << ((Unit*)this)->GetSpeed( MOVE_SWIM_BACK );
        *data << ((Unit*)this)->GetSpeed( MOVE_SWIM );
        *data << ((Unit*)this)->GetSpeed( MOVE_RUN_BACK );
        *data << ((Unit*)this)->GetSpeed( MOVE_FLIGHT );
        *data << ((Unit*)this)->GetSpeed( MOVE_FLIGHT_BACK );
        *data << ((Unit*)this)->GetSpeed( MOVE_TURN_RATE );

        // 0x08000000
        if(flags2 & MOVEMENTFLAG_SPLINE_ENABLED)
        {
            if(GetTypeId() != TYPEID_PLAYER)
            {
                sLog.outError("_BuildMovementUpdate: MOVEMENTFLAG_SPLINE2 for non-player");
                return;
            }

            Player *player = const_cast<Player*>(ToPlayer());

            if(!player->isInFlight())
            {
                sLog.outError("_BuildMovementUpdate: MOVEMENTFLAG_SPLINE2 but not in flight");
                return;
            }

            WPAssert(player->GetMotionMaster()->GetCurrentMovementGeneratorType() == FLIGHT_MOTION_TYPE);

            FlightPathMovementGenerator *fmg = (FlightPathMovementGenerator*)(player->GetMotionMaster()->top());

            uint32 flags3 = 0x00000300;

            *data << uint32(flags3);                        // splines flag?

            if(flags3 & 0x10000)                            // probably x,y,z coords there
            {
                *data << (float)0;
                *data << (float)0;
                *data << (float)0;
            }

            if(flags3 & 0x20000)                            // probably guid there
            {
                *data << uint64(0);
            }

            if(flags3 & 0x40000)                            // may be orientation
            {
                *data << (float)0;
            }

            Path &path = fmg->GetPath();

            float x, y, z;
            (this->ToPlayer())->GetPosition(x, y, z);

            uint32 inflighttime = uint32(path.GetPassedLength(fmg->GetCurrentNode(), x, y, z) * 32);
            uint32 traveltime = uint32(path.GetTotalLength() * 32);

            *data << uint32(inflighttime);                  // passed move time?
            *data << uint32(traveltime);                    // full move time?
            *data << uint32(0);                             // ticks count?

            uint32 poscount = uint32(path.Size());

            *data << uint32(poscount);                      // points count

            for(uint32 i = 0; i < poscount; ++i)
            {
                *data << path.GetNodes()[i].x;
                *data << path.GetNodes()[i].y;
                *data << path.GetNodes()[i].z;
            }

            /*for(uint32 i = 0; i < poscount; i++)
            {
                // path points
                *data << (float)0;
                *data << (float)0;
                *data << (float)0;
            }*/

            *data << path.GetNodes()[poscount-1].x;
            *data << path.GetNodes()[poscount-1].y;
            *data << path.GetNodes()[poscount-1].z;

            // target position (path end)
            /**data << ((Unit*)this)->GetPositionX();
             *data << ((Unit*)this)->GetPositionY();
             *data << ((Unit*)this)->GetPositionZ();*/
        }
    }

    // 0x8
    if(flags & UPDATEFLAG_LOWGUID)
    {
        switch(GetTypeId())
        {
            case TYPEID_OBJECT:
            case TYPEID_ITEM:
            case TYPEID_CONTAINER:
            case TYPEID_GAMEOBJECT:
            case TYPEID_DYNAMICOBJECT:
            case TYPEID_CORPSE:
                *data << uint32(GetGUIDLow());              // GetGUIDLow()
                break;
            case TYPEID_UNIT:
                *data << uint32(0x0000000B);                // unk, can be 0xB or 0xC
                break;
            case TYPEID_PLAYER:
                if(flags & UPDATEFLAG_SELF)
                    *data << uint32(0x00000015);            // unk, can be 0x15 or 0x22
                else
                    *data << uint32(0x00000008);            // unk, can be 0x7 or 0x8
                break;
            default:
                *data << uint32(0x00000000);                // unk
                break;
        }
    }

    // 0x10
    if(flags & UPDATEFLAG_HIGHGUID)
    {
        switch(GetTypeId())
        {
            case TYPEID_OBJECT:
            case TYPEID_ITEM:
            case TYPEID_CONTAINER:
            case TYPEID_GAMEOBJECT:
            case TYPEID_DYNAMICOBJECT:
            case TYPEID_CORPSE:
                *data << uint32(GetGUIDHigh());             // GetGUIDHigh()
                break;
            default:
                *data << uint32(0x00000000);                // unk
                break;
        }
    }

    // 0x4
    if(flags & UPDATEFLAG_FULLGUID)
    {
        *data << uint8(0);                                  // packed guid (probably target guid)
    }

    // 0x2
    if(flags & UPDATEFLAG_TRANSPORT)
    {
        *data << uint32(getMSTime());                       // ms time
    }
}

void Object::_BuildValuesUpdate(uint8 updatetype, ByteBuffer * data, UpdateMask *updateMask, Player *target) const
{
    if(!target)
        return;

    bool IsActivateToQuest = false;
    if (updatetype == UPDATETYPE_CREATE_OBJECT || updatetype == UPDATETYPE_CREATE_OBJECT2)
    {
        if (isType(TYPEMASK_GAMEOBJECT) && !((GameObject*)this)->IsTransport())
        {
            if ( ((GameObject*)this)->ActivateToQuest(target) || target->isGameMaster())
            {
                IsActivateToQuest = true;
                updateMask->SetBit(GAMEOBJECT_DYN_FLAGS);
            }
            if (GetUInt32Value(GAMEOBJECT_ARTKIT))
                updateMask->SetBit(GAMEOBJECT_ARTKIT);
        }
    }
    else                                                    //case UPDATETYPE_VALUES
    {
        if (isType(TYPEMASK_GAMEOBJECT) && !((GameObject*)this)->IsTransport())
        {
            if ( ((GameObject*)this)->ActivateToQuest(target) || target->isGameMaster())
            {
                IsActivateToQuest = true;
            }
            updateMask->SetBit(GAMEOBJECT_DYN_FLAGS);
            updateMask->SetBit(GAMEOBJECT_ANIMPROGRESS);
        }
    }

    WPAssert(updateMask && updateMask->GetCount() == m_valuesCount);

    *data << (uint8)updateMask->GetBlockCount();
    updateMask->AppendToPacket(data);

    // 2 specialized loops for speed optimization in non-unit case
    if(isType(TYPEMASK_UNIT))                               // unit (creature/player) case
    {
        for( uint16 index = 0; index < m_valuesCount; index ++ )
        {
            if( updateMask->GetBit( index ) )
            {
                // remove custom flag before send
                if( index == UNIT_NPC_FLAGS )
                    *data << uint32(m_uint32Values[ index ] & ~(UNIT_NPC_FLAG_GUARD + UNIT_NPC_FLAG_OUTDOORPVP));
                // FIXME: Some values at server stored in float format but must be sent to client in uint32 format
                else if(index >= UNIT_FIELD_BASEATTACKTIME && index <= UNIT_FIELD_RANGEDATTACKTIME)
                {
                    // convert from float to uint32 and send
                    *data << uint32(m_floatValues[ index ] < 0 ? 0 : m_floatValues[ index ]);
                }
                // there are some float values which may be negative or can't get negative due to other checks
                else if((index >= UNIT_FIELD_NEGSTAT0   && index <= UNIT_FIELD_NEGSTAT4) ||
                    (index >= UNIT_FIELD_RESISTANCEBUFFMODSPOSITIVE  && index <= (UNIT_FIELD_RESISTANCEBUFFMODSPOSITIVE + 6)) ||
                    (index >= UNIT_FIELD_RESISTANCEBUFFMODSNEGATIVE  && index <= (UNIT_FIELD_RESISTANCEBUFFMODSNEGATIVE + 6)) ||
                    (index >= UNIT_FIELD_POSSTAT0   && index <= UNIT_FIELD_POSSTAT4))
                {
                    *data << uint32(m_floatValues[ index ]);
                }
                // Gamemasters should be always able to select units - remove not selectable flag
                else if(index == UNIT_FIELD_FLAGS && target->isGameMaster())
                {
                    *data << (m_uint32Values[ index ] & ~UNIT_FLAG_NOT_SELECTABLE);
                }
                // use modelid_a if not gm, _h if gm for CREATURE_FLAG_EXTRA_TRIGGER creatures
                else if(index == UNIT_FIELD_DISPLAYID && GetTypeId() == TYPEID_UNIT)
                {
                    const CreatureInfo* cinfo = (this->ToCreature())->GetCreatureInfo();
                    if(cinfo->flags_extra & CREATURE_FLAG_EXTRA_TRIGGER)
                    {
                        if(target->isGameMaster() && target->GetSession()->GetGroupId() != GMGROUP_VIDEO)
                        {
                            if(cinfo->Modelid_A2)
                                *data << cinfo->Modelid_A1;
                            else
                                *data << 17519; // world invisible trigger's model
                        }
                        else
                        {
                            if(cinfo->Modelid_A2)
                                *data << cinfo->Modelid_A2;
                            else
                                *data << 11686; // world invisible trigger's model
                        }
                    }
                    else
                        *data << m_uint32Values[ index ];
                }
                // hide lootable animation for unallowed players
                else if(index == UNIT_DYNAMIC_FLAGS && GetTypeId() == TYPEID_UNIT)
                {
                    Creature *creature = const_cast<Creature*>(ToCreature());
                    if(!target->isAllowedToLoot(creature))
                        *data << (m_uint32Values[ index ] & ~UNIT_DYNFLAG_LOOTABLE);
                    else
                        *data << (m_uint32Values[ index ] & ~UNIT_DYNFLAG_OTHER_TAGGER);
                }
                // FG: pretend that OTHER players in own group are friendly ("blue")
                else if(index == UNIT_FIELD_BYTES_2 || index == UNIT_FIELD_FACTIONTEMPLATE)
                {
                bool ch = false;
                    if(target->GetTypeId() == TYPEID_PLAYER && GetTypeId() == TYPEID_PLAYER && target != this)
                    {
                    if(target->IsInSameGroupWith(this->ToPlayer()) || target->IsInSameRaidWith(this->ToPlayer()))
                    {
                        if(index == UNIT_FIELD_BYTES_2)
                        {
                            DEBUG_LOG("-- VALUES_UPDATE: Sending '%s' the blue-group-fix from '%s' (flag)", target->GetName(), (this->ToPlayer())->GetName());
                            *data << ( m_uint32Values[ index ] & ((UNIT_BYTE2_FLAG_SANCTUARY | UNIT_BYTE2_FLAG_AURAS | UNIT_BYTE2_FLAG_UNK5) << 8) ); // this flag is at uint8 offset 1 !!

                            ch = true;
                        }
                        else if(index == UNIT_FIELD_FACTIONTEMPLATE)
                        {
                            FactionTemplateEntry const *ft1, *ft2;
                            ft1 = (this->ToPlayer())->getFactionTemplateEntry();
                            ft2 = (target->ToPlayer())->getFactionTemplateEntry();
                            if(ft1 && ft2 && !ft1->IsFriendlyTo(*ft2))
                            {
                                uint32 faction = (target->ToPlayer())->getFaction(); // pretend that all other HOSTILE players have own faction, to allow follow, heal, rezz (trade wont work)
                                DEBUG_LOG("-- VALUES_UPDATE: Sending '%s' the blue-group-fix from '%s' (faction %u)", target->GetName(), (this->ToPlayer())->GetName(), faction);
                                *data << uint32(faction);
                                ch = true;
                            }
                        }
                    }
                    }
                    if(!ch)
                        *data << m_uint32Values[ index ];
                }
                else
                {
                    // send in current format (float as float, uint32 as uint32)
                    *data << m_uint32Values[ index ];
                }
            }
        }
    }
    else if(isType(TYPEMASK_GAMEOBJECT))                    // gameobject case
    {
        for( uint16 index = 0; index < m_valuesCount; index ++ )
        {
            if( updateMask->GetBit( index ) )
            {
                // send in current format (float as float, uint32 as uint32)
                if ( index == GAMEOBJECT_DYN_FLAGS )
                {
                    if(IsActivateToQuest )
                    {
                        switch(((GameObject*)this)->GetGoType())
                        {
                            case GAMEOBJECT_TYPE_CHEST:
                                *data << uint32(9);         // enable quest object. Represent 9, but 1 for client before 2.3.0
                                break;
                            case GAMEOBJECT_TYPE_GOOBER:
                                *data << uint32(1);
                                break;
                            default:
                                *data << uint32(0);         // unknown. not happen.
                                break;
                        }
                    }
                    else
                        *data << uint32(0);                 // disable quest object
                }
                else
                    *data << m_uint32Values[ index ];       // other cases
            }
        }
    }
    else                                                    // other objects case (no special index checks)
    {
        for( uint16 index = 0; index < m_valuesCount; index ++ )
        {
            if( updateMask->GetBit( index ) )
            {
                // send in current format (float as float, uint32 as uint32)
                *data << m_uint32Values[ index ];
            }
        }
    }
}

void Object::ClearUpdateMask(bool remove)
{
    for( uint16 index = 0; index < m_valuesCount; index ++ )
    {
        if(m_uint32Values_mirror[index]!= m_uint32Values[index])
            m_uint32Values_mirror[index] = m_uint32Values[index];
    }
    if(m_objectUpdated)
    {
        if(remove)
            ObjectAccessor::Instance().RemoveUpdateObject(this);
        m_objectUpdated = false;
    }
}

// Send current value fields changes to all viewers
void Object::SendUpdateObjectToAllExcept(Player* exceptPlayer)
{
    // changes will be send in create packet
    if(!IsInWorld())
        return;

    // nothing do
    if(!m_objectUpdated)
        return;

    ObjectAccessor::UpdateObject(this,exceptPlayer);
}

bool Object::LoadValues(const char* data)
{
    if(!m_uint32Values) _InitValues();

    Tokens tokens = StrSplit(data, " ");

    if(tokens.size() != m_valuesCount)
        return false;

    Tokens::iterator iter;
    int index;
    for (iter = tokens.begin(), index = 0; index < m_valuesCount; ++iter, ++index)
    {
        m_uint32Values[index] = atol((*iter).c_str());
    }

    return true;
}

void Object::_SetUpdateBits(UpdateMask *updateMask, Player* /*target*/) const
{
    for( uint16 index = 0; index < m_valuesCount; index ++ )
    {
        if(m_uint32Values_mirror[index]!= m_uint32Values[index])
            updateMask->SetBit(index);
    }
    
    // always update this field to prevent problems with shapeshifting
    if (GetTypeId() == TYPEID_PLAYER)
        updateMask->SetBit(UNIT_FIELD_BYTES_2);
}

void Object::_SetCreateBits(UpdateMask *updateMask, Player* /*target*/) const
{
    for( uint16 index = 0; index < m_valuesCount; index++ )
    {
        if(GetUInt32Value(index) != 0)
            updateMask->SetBit(index);
    }
}

void Object::SetInt32Value( uint16 index, int32 value )
{
    ASSERT( index < m_valuesCount || PrintIndexError( index , true ) );

    if(m_int32Values[ index ] != value)
    {
        m_int32Values[ index ] = value;

        if(m_inWorld)
        {
            if(!m_objectUpdated)
            {
                ObjectAccessor::Instance().AddUpdateObject(this);
                m_objectUpdated = true;
            }
        }
    }
}

void Object::SetUInt32Value( uint16 index, uint32 value )
{
    ASSERT( index < m_valuesCount || PrintIndexError( index , true ) );

    if(m_uint32Values[ index ] != value)
    {
        m_uint32Values[ index ] = value;

        if(m_inWorld)
        {
            if(!m_objectUpdated)
            {
                ObjectAccessor::Instance().AddUpdateObject(this);
                m_objectUpdated = true;
            }
        }
    }
}

void Object::SetUInt64Value( uint16 index, const uint64 &value )
{
    ASSERT( index + 1 < m_valuesCount || PrintIndexError( index , true ) );
    if(*((uint64*)&(m_uint32Values[ index ])) != value)
    {
        m_uint32Values[ index ] = *((uint32*)&value);
        m_uint32Values[ index + 1 ] = *(((uint32*)&value) + 1);

        if(m_inWorld)
        {
            if(!m_objectUpdated)
            {
                ObjectAccessor::Instance().AddUpdateObject(this);
                m_objectUpdated = true;
            }
        }
    }
}

void Object::SetFloatValue( uint16 index, float value )
{
    ASSERT( index < m_valuesCount || PrintIndexError( index , true ) );

    if(m_floatValues[ index ] != value)
    {
        m_floatValues[ index ] = value;

        if(m_inWorld)
        {
            if(!m_objectUpdated)
            {
                ObjectAccessor::Instance().AddUpdateObject(this);
                m_objectUpdated = true;
            }
        }
    }
}

void Object::SetByteValue( uint16 index, uint8 offset, uint8 value )
{
    ASSERT( index < m_valuesCount || PrintIndexError( index , true ) );

    if(offset > 4)
    {
        sLog.outError("Object::SetByteValue: wrong offset %u", offset);
        return;
    }

    if(uint8(m_uint32Values[ index ] >> (offset * 8)) != value)
    {
        m_uint32Values[ index ] &= ~uint32(uint32(0xFF) << (offset * 8));
        m_uint32Values[ index ] |= uint32(uint32(value) << (offset * 8));

        if(m_inWorld)
        {
            if(!m_objectUpdated)
            {
                ObjectAccessor::Instance().AddUpdateObject(this);
                m_objectUpdated = true;
            }
        }
    }
}

void Object::SetUInt16Value( uint16 index, uint8 offset, uint16 value )
{
    ASSERT( index < m_valuesCount || PrintIndexError( index , true ) );

    if(offset > 2)
    {
        sLog.outError("Object::SetUInt16Value: wrong offset %u", offset);
        return;
    }

    if(uint8(m_uint32Values[ index ] >> (offset * 16)) != value)
    {
        m_uint32Values[ index ] &= ~uint32(uint32(0xFFFF) << (offset * 16));
        m_uint32Values[ index ] |= uint32(uint32(value) << (offset * 16));

        if(m_inWorld)
        {
            if(!m_objectUpdated)
            {
                ObjectAccessor::Instance().AddUpdateObject(this);
                m_objectUpdated = true;
            }
        }
    }
}

void Object::SetStatFloatValue( uint16 index, float value)
{
    if(value < 0)
        value = 0.0f;

    SetFloatValue(index, value);
}

void Object::SetStatInt32Value( uint16 index, int32 value)
{
    if(value < 0)
        value = 0;

    SetUInt32Value(index, uint32(value));
}

void Object::ApplyModUInt32Value(uint16 index, int32 val, bool apply)
{
    int32 cur = GetUInt32Value(index);
    cur += (apply ? val : -val);
    if(cur < 0)
        cur = 0;
    SetUInt32Value(index,cur);
}

void Object::ApplyModInt32Value(uint16 index, int32 val, bool apply)
{
    int32 cur = GetInt32Value(index);
    cur += (apply ? val : -val);
    SetInt32Value(index,cur);
}

void Object::ApplyModSignedFloatValue(uint16 index, float  val, bool apply)
{
    float cur = GetFloatValue(index);
    cur += (apply ? val : -val);
    SetFloatValue(index,cur);
}

void Object::ApplyModPositiveFloatValue(uint16 index, float  val, bool apply)
{
    float cur = GetFloatValue(index);
    cur += (apply ? val : -val);
    if(cur < 0)
        cur = 0;
    SetFloatValue(index,cur);
}

void Object::SetFlag( uint16 index, uint32 newFlag )
{
    ASSERT( index < m_valuesCount || PrintIndexError( index , true ) );
    uint32 oldval = m_uint32Values[ index ];
    uint32 newval = oldval | newFlag;

    if(oldval != newval)
    {
        m_uint32Values[ index ] = newval;

        if(m_inWorld)
        {
            if(!m_objectUpdated)
            {
                ObjectAccessor::Instance().AddUpdateObject(this);
                m_objectUpdated = true;
            }
        }
    }
}

void Object::RemoveFlag( uint16 index, uint32 oldFlag )
{
    ASSERT( index < m_valuesCount || PrintIndexError( index , true ) );
    uint32 oldval = m_uint32Values[ index ];
    uint32 newval = oldval & ~oldFlag;

    if(oldval != newval)
    {
        m_uint32Values[ index ] = newval;

        if(m_inWorld)
        {
            if(!m_objectUpdated)
            {
                ObjectAccessor::Instance().AddUpdateObject(this);
                m_objectUpdated = true;
            }
        }
    }
}

void Object::SetByteFlag( uint16 index, uint8 offset, uint8 newFlag )
{
    ASSERT( index < m_valuesCount || PrintIndexError( index , true ) );

    if(offset > 4)
    {
        sLog.outError("Object::SetByteFlag: wrong offset %u", offset);
        return;
    }

    if(!(uint8(m_uint32Values[ index ] >> (offset * 8)) & newFlag))
    {
        m_uint32Values[ index ] |= uint32(uint32(newFlag) << (offset * 8));

        if(m_inWorld)
        {
            if(!m_objectUpdated)
            {
                ObjectAccessor::Instance().AddUpdateObject(this);
                m_objectUpdated = true;
            }
        }
    }
}

void Object::RemoveByteFlag( uint16 index, uint8 offset, uint8 oldFlag )
{
    ASSERT( index < m_valuesCount || PrintIndexError( index , true ) );

    if(offset > 4)
    {
        sLog.outError("Object::RemoveByteFlag: wrong offset %u", offset);
        return;
    }

    if(uint8(m_uint32Values[ index ] >> (offset * 8)) & oldFlag)
    {
        m_uint32Values[ index ] &= ~uint32(uint32(oldFlag) << (offset * 8));

        if(m_inWorld)
        {
            if(!m_objectUpdated)
            {
                ObjectAccessor::Instance().AddUpdateObject(this);
                m_objectUpdated = true;
            }
        }
    }
}

bool Object::PrintIndexError(uint32 index, bool set) const
{
    sLog.outError("Attempt %s non-existed value field: %u (count: %u) for object typeid: %u type mask: %u",(set ? "set value to" : "get value from"),index,m_valuesCount,GetTypeId(),m_objectType);

    // assert must fail after function call
    return false;
}

WorldObject::WorldObject()
{
    m_positionX         = 0.0f;
    m_positionY         = 0.0f;
    m_positionZ         = 0.0f;
    m_orientation       = 0.0f;

    m_mapId             = 0;
    m_InstanceId        = 0;
    m_map               = NULL;

    m_name = "";

    m_groupLootTimer    = 0;
    lootingGroupLeaderGUID = 0;

    mSemaphoreTeleport  = false;

    m_isActive          = false;
    IsTempWorldObject   = false;
}

void WorldObject::SetWorldObject(bool on)
{
    if(!IsInWorld())
        return;
    
    GetMap()->AddObjectToSwitchList(this, on); 
}

void WorldObject::setActive( bool on )
{
    if(m_isActive == on)
        return;

    if(GetTypeId() == TYPEID_PLAYER)
        return;

    m_isActive = on;

    if(!IsInWorld())
        return;

    Map *map = FindMap();
    if(!map)
        return;

    if(on)
    {
        if(GetTypeId() == TYPEID_UNIT)
            map->AddToActive(this->ToCreature());
        else if(GetTypeId() == TYPEID_DYNAMICOBJECT)
            map->AddToActive((DynamicObject*)this);
    }
    else
    {
        if(GetTypeId() == TYPEID_UNIT)
            map->RemoveFromActive(this->ToCreature());
        else if(GetTypeId() == TYPEID_DYNAMICOBJECT)
            map->RemoveFromActive((DynamicObject*)this);
    }
}

void WorldObject::_Create( uint32 guidlow, HighGuid guidhigh, uint32 mapid )
{
    Object::_Create(guidlow, 0, guidhigh);

    m_mapId = mapid;
}

uint32 WorldObject::GetZoneId() const
{
    return MapManager::Instance().GetBaseMap(m_mapId)->GetZoneId(m_positionX,m_positionY,m_positionZ);
}

uint32 WorldObject::GetAreaId() const
{
    return MapManager::Instance().GetBaseMap(m_mapId)->GetAreaId(m_positionX,m_positionY,m_positionZ);
}

InstanceData* WorldObject::GetInstanceData()
{
    Map *map = GetMap();
    return map->IsDungeon() ? ((InstanceMap*)map)->GetInstanceData() : NULL;
}

                                                            //slow
float WorldObject::GetDistance(const WorldObject* obj) const
{
    float dx = GetPositionX() - obj->GetPositionX();
    float dy = GetPositionY() - obj->GetPositionY();
    float dz = GetPositionZ() - obj->GetPositionZ();
    float sizefactor = GetObjectSize() + obj->GetObjectSize();
    float dist = sqrt((dx*dx) + (dy*dy) + (dz*dz)) - sizefactor;
    return ( dist > 0 ? dist : 0);
}

float WorldObject::GetDistance2d(float x, float y) const
{
    float dx = GetPositionX() - x;
    float dy = GetPositionY() - y;
    float sizefactor = GetObjectSize();
    float dist = sqrt((dx*dx) + (dy*dy)) - sizefactor;
    return ( dist > 0 ? dist : 0);
}

float WorldObject::GetDistanceSqr(float x, float y, float z) const
{
    float dx = GetPositionX() - x;
    float dy = GetPositionY() - y;
    float dz = GetPositionZ() - z;
    float sizefactor = GetObjectSize();
    float dist = dx*dx+dy*dy+dz*dz-sizefactor;
    return (dist > 0 ? dist : 0);
}

float WorldObject::GetExactDistance2d(const WorldObject* obj) const
{
    return GetExactDistance2d(obj->GetPositionX(),obj->GetPositionY());
}

float WorldObject::GetExactDistance2d(const float x, const float y) const
{
    float dx = GetPositionX() - x;
    float dy = GetPositionY() - y;
    return sqrt((dx*dx) + (dy*dy));
}

float WorldObject::GetDistance(const float x, const float y, const float z) const
{
    float dx = GetPositionX() - x;
    float dy = GetPositionY() - y;
    float dz = GetPositionZ() - z;
    float sizefactor = GetObjectSize();
    float dist = sqrt((dx*dx) + (dy*dy) + (dz*dz)) - sizefactor;
    return ( dist > 0 ? dist : 0);
}

float WorldObject::GetExactDistance(const float x, const float y, const float z) const
{
    float dx = GetPositionX() - x;
    float dy = GetPositionY() - y;
    float dz = GetPositionZ() - z;
    float dist = sqrt((dx*dx) + (dy*dy) + (dz*dz));
    return ( dist > 0 ? dist : 0);
}

float WorldObject::GetDistanceSq(const float &x, const float &y, const float &z) const
{
    float dx = GetPositionX() - x;
    float dy = GetPositionY() - y;
    float dz = GetPositionZ() - z;
    return dx*dx + dy*dy + dz*dz;
}

float WorldObject::GetDistance2d(const WorldObject* obj) const
{
    float dx = GetPositionX() - obj->GetPositionX();
    float dy = GetPositionY() - obj->GetPositionY();
    float sizefactor = GetObjectSize() + obj->GetObjectSize();
    float dist = sqrt((dx*dx) + (dy*dy)) - sizefactor;
    return ( dist > 0 ? dist : 0);
}

float WorldObject::GetDistanceZ(const WorldObject* obj) const
{
    float dz = fabs(GetPositionZ() - obj->GetPositionZ());
    float sizefactor = GetObjectSize() + obj->GetObjectSize();
    float dist = dz - sizefactor;
    return ( dist > 0 ? dist : 0);
}

bool WorldObject::IsWithinDistInMap(const WorldObject* obj, const float dist2compare, const bool is3D) const
{
    if (!obj || !IsInMap(obj)) return false;

    float dx = GetPositionX() - obj->GetPositionX();
    float dy = GetPositionY() - obj->GetPositionY();
    float distsq = dx*dx + dy*dy;
    if(is3D)
    {
        float dz = GetPositionZ() - obj->GetPositionZ();
        distsq += dz*dz;
    }
    float sizefactor = GetObjectSize() + obj->GetObjectSize();
    float maxdist = dist2compare + sizefactor;

    return distsq < maxdist * maxdist;
}

bool WorldObject::IsWithinLOSInMap(const WorldObject* obj) const
{
    if (!IsInMap(obj)) return false;
    float ox,oy,oz;
    obj->GetPosition(ox,oy,oz);
    return IsWithinLOS(ox, oy, oz);
}

bool WorldObject::IsWithinLOS(const float ox, const float oy, const float oz ) const
{
    /*float x,y,z;
    GetPosition(x,y,z);
    VMAP::IVMapManager *vMapManager = VMAP::VMapFactory::createOrGetVMapManager();
    return vMapManager->isInLineOfSight(GetMapId(), x, y, z+2.0f, ox, oy, oz+2.0f);*/
    if (IsInWorld())
        return GetMap()->isInLineOfSight(GetPositionX(), GetPositionY(), GetPositionZ()+2.f, ox, oy, oz+2.f);
    
    return true;
}

bool WorldObject::GetDistanceOrder(WorldObject const* obj1, WorldObject const* obj2, bool is3D /* = true */) const
{
    float dx1 = GetPositionX() - obj1->GetPositionX();
    float dy1 = GetPositionY() - obj1->GetPositionY();
    float distsq1 = dx1*dx1 + dy1*dy1;
    if(is3D)
    {
        float dz1 = GetPositionZ() - obj1->GetPositionZ();
        distsq1 += dz1*dz1;
    }

    float dx2 = GetPositionX() - obj2->GetPositionX();
    float dy2 = GetPositionY() - obj2->GetPositionY();
    float distsq2 = dx2*dx2 + dy2*dy2;
    if(is3D)
    {
        float dz2 = GetPositionZ() - obj2->GetPositionZ();
        distsq2 += dz2*dz2;
    }

    return distsq1 < distsq2;
}

bool WorldObject::IsInRange(WorldObject const* obj, float minRange, float maxRange, bool is3D /* = true */) const
{
    float dx = GetPositionX() - obj->GetPositionX();
    float dy = GetPositionY() - obj->GetPositionY();
    float distsq = dx*dx + dy*dy;
    if (is3D)
    {
        float dz = GetPositionZ() - obj->GetPositionZ();
        distsq += dz*dz;
    }

    float sizefactor = GetObjectSize() + obj->GetObjectSize();

    // check only for real range
    if (minRange > 0.0f)
    {
        float mindist = minRange + sizefactor;
        if (distsq < mindist * mindist)
            return false;
    }

    float maxdist = maxRange + sizefactor;
    return distsq < maxdist * maxdist;
}

void WorldObject::GetRandomPoint( float x, float y, float z, float distance, float &rand_x, float &rand_y, float &rand_z) const
{
    if(distance==0)
    {
        rand_x = x;
        rand_y = y;
        rand_z = z;
        return;
    }

    // angle to face `obj` to `this`
    float angle = GetMap()->rand_norm()*2*M_PI;
    float new_dist = GetMap()->rand_norm()*distance;

    rand_x = x + new_dist * cos(angle);
    rand_y = y + new_dist * sin(angle);
    rand_z = z;

    Trinity::NormalizeMapCoord(rand_x);
    Trinity::NormalizeMapCoord(rand_y);
    UpdateAllowedPositionZ(rand_x,rand_y,rand_z);            // update to LOS height if available
}

void WorldObject::UpdateGroundPositionZ(float x, float y, float &z) const
{
    float new_z = MapManager::Instance().GetBaseMap(GetMapId())->GetHeight(x,y,z,true);
    if(new_z > INVALID_HEIGHT)
        z = new_z+ 0.05f;                                   // just to be sure that we are not a few pixel under the surface
}

void WorldObject::UpdateAllowedPositionZ(float x, float y, float &z) const
{
    switch (GetTypeId())
    {
        case TYPEID_UNIT:
        {
            // non fly unit don't must be in air
            // non swim unit must be at ground (mostly speedup, because it don't must be in water and water level check less fast
            if (!ToCreature()->canFly())
            {
                bool canSwim = ToCreature()->canSwim();
                float ground_z = z;
                float max_z = canSwim
                    ? GetBaseMap()->GetWaterOrGroundLevel(x, y, z, &ground_z, !ToUnit()->HasAuraType(SPELL_AURA_WATER_WALK))
                    : ((ground_z = GetBaseMap()->GetHeight(x, y, z, true)));
                if (max_z > INVALID_HEIGHT)
                {
                    if (z > max_z)
                        z = max_z;
                    else if (z < ground_z)
                        z = ground_z;
                }
            }
            else
            {
                float ground_z = GetBaseMap()->GetHeight(x, y, z, true);
                if (z < ground_z)
                    z = ground_z;
            }
            break;
        }
        case TYPEID_PLAYER:
        {
            // for server controlled moves player work same as creature (but it can always swim)
            if (!ToPlayer()->CanFly())
            {
                float ground_z = z;
                float max_z = GetBaseMap()->GetWaterOrGroundLevel(x, y, z, &ground_z, !ToUnit()->HasAuraType(SPELL_AURA_WATER_WALK));
                if (max_z > INVALID_HEIGHT)
                {
                    if (z > max_z)
                        z = max_z;
                    else if (z < ground_z)
                        z = ground_z;
                }
            }
            else
            {
                float ground_z = GetBaseMap()->GetHeight(x, y, z, true);
                if (z < ground_z)
                    z = ground_z;
            }
            break;
        }
        default:
        {
            float ground_z = GetBaseMap()->GetHeight(x, y, z, true);
            if (ground_z > INVALID_HEIGHT)
                z = ground_z;
            break;
        }
    }
}

bool WorldObject::IsPositionValid() const
{
    return Trinity::IsValidMapCoord(m_positionX,m_positionY,m_positionZ,m_orientation);
}

void WorldObject::MonsterSay(const char* text, uint32 language, uint64 TargetGuid)
{
    WorldPacket data(SMSG_MESSAGECHAT, 200);
    BuildMonsterChat(&data,CHAT_MSG_MONSTER_SAY,text,language,GetName(),TargetGuid);
    SendMessageToSetInRange(&data,sWorld.getConfig(CONFIG_LISTEN_RANGE_SAY),true);
}

void WorldObject::MonsterYell(const char* text, uint32 language, uint64 TargetGuid)
{
    WorldPacket data(SMSG_MESSAGECHAT, 200);
    BuildMonsterChat(&data,CHAT_MSG_MONSTER_YELL,text,language,GetName(),TargetGuid);
    SendMessageToSetInRange(&data,sWorld.getConfig(CONFIG_LISTEN_RANGE_YELL),true);
}

void WorldObject::MonsterTextEmote(const char* text, uint64 TargetGuid, bool IsBossEmote, float dist, bool IsServerEmote)
{
    WorldPacket data(SMSG_MESSAGECHAT, 200);
    BuildMonsterChat(&data,IsBossEmote ? CHAT_MSG_RAID_BOSS_EMOTE : CHAT_MSG_MONSTER_EMOTE,text,LANG_UNIVERSAL,GetName(),TargetGuid);

    float range;
    if (dist)
        range = dist;
    else
        range = sWorld.getConfig(CONFIG_LISTEN_RANGE_TEXTEMOTE);

    if (IsServerEmote)
        sWorld.SendGlobalMessage(&data);
    else
        SendMessageToSetInRange(&data,range,true);
}

void WorldObject::MonsterWhisper(const char* text, uint64 receiver, bool IsBossWhisper)
{
    Player *player = objmgr.GetPlayer(receiver);
    if(!player || !player->GetSession())
        return;

    WorldPacket data(SMSG_MESSAGECHAT, 200);
    BuildMonsterChat(&data,IsBossWhisper ? CHAT_MSG_RAID_BOSS_WHISPER : CHAT_MSG_MONSTER_WHISPER,text,LANG_UNIVERSAL,GetName(),receiver);

    player->GetSession()->SendPacket(&data);
}

void WorldObject::SendPlaySound(uint32 Sound, bool OnlySelf)
{
    WorldPacket data(SMSG_PLAY_SOUND, 4);
    data << Sound;
    if (OnlySelf && GetTypeId() == TYPEID_PLAYER )
        (this->ToPlayer())->GetSession()->SendPacket( &data );
    else
        SendMessageToSet( &data, true ); // ToSelf ignored in this case
}

void Object::ForceValuesUpdateAtIndex(uint32 i)
{
    m_uint32Values_mirror[i] = GetUInt32Value(i) + 1; // makes server think the field changed
    if(m_inWorld)
    {
        if(!m_objectUpdated)
        {
            ObjectAccessor::Instance().AddUpdateObject(this);
            m_objectUpdated = true;
        }
    }
}

namespace Trinity
{
    MessageChatLocaleCacheDo::MessageChatLocaleCacheDo(WorldObject const& obj, ChatMsg msgtype, int32 textId, uint32 language, uint64 targetGUID, float dist)
        : i_object(obj), i_msgtype(msgtype), i_textId(textId), i_language(language), i_targetGUID(targetGUID), i_dist(dist)
    {
    }

    MessageChatLocaleCacheDo::~MessageChatLocaleCacheDo()
    {
        for(int i = 0; i < i_data_cache.size(); ++i)
            delete i_data_cache[i];
    }

    void MessageChatLocaleCacheDo::operator()(Player* p)
    {
        // skip far away players
        if(p->GetDistance(&i_object) > i_dist)
            return;

        uint32 loc_idx = p->GetSession()->GetSessionDbLocaleIndex();
        uint32 cache_idx = loc_idx+1;
        WorldPacket* data;

        // create if not cached yet
        if(i_data_cache.size() < cache_idx+1 || !i_data_cache[cache_idx])
        {
            if(i_data_cache.size() < cache_idx+1)
                i_data_cache.resize(cache_idx+1);

            char const* text = objmgr.GetTrinityString(i_textId,loc_idx);

            data = new WorldPacket(SMSG_MESSAGECHAT, 200);

            // TODO: i_object.GetName() also must be localized?
            i_object.BuildMonsterChat(data,i_msgtype,text,i_language,i_object.GetNameForLocaleIdx(loc_idx),i_targetGUID);

            i_data_cache[cache_idx] = data;
        }
        else
            data = i_data_cache[cache_idx];

        p->SendDirectMessage(data);
    }

    CreatureTextLocaleDo::CreatureTextLocaleDo(WorldObject& source, WorldPacket* data_en, WorldPacket* data_fr, float dist)
        : i_source(source), i_data_en(data_en), i_data_fr(data_fr), i_dist(dist)
    {
    }
        
    void CreatureTextLocaleDo::operator()(Player* p)
    {
        if (p->GetDistance(&i_source) > i_dist)
            return;
                
        if (p->GetSession()->GetSessionDbLocaleIndex() == objmgr.GetIndexForLocale(LOCALE_frFR))
            p->SendDirectMessage(i_data_fr);
        else
            p->SendDirectMessage(i_data_en);
    }
}

void WorldObject::MonsterSay(int32 textId, uint32 language, uint64 TargetGuid)
{
    CellPair p = Trinity::ComputeCellPair(GetPositionX(), GetPositionY());

    Cell cell(p);
    cell.data.Part.reserved = ALL_DISTRICT;
    cell.SetNoCreate();

    Trinity::MessageChatLocaleCacheDo say_do(*this, CHAT_MSG_MONSTER_SAY, textId,language,TargetGuid,sWorld.getConfig(CONFIG_LISTEN_RANGE_SAY));
    Trinity::PlayerWorker<Trinity::MessageChatLocaleCacheDo> say_worker(say_do);
    TypeContainerVisitor<Trinity::PlayerWorker<Trinity::MessageChatLocaleCacheDo>, WorldTypeMapContainer > message(say_worker);
    cell.Visit(p, message, *GetMap(), *this, sWorld.getConfig(CONFIG_LISTEN_RANGE_SAY));
}

void WorldObject::MonsterYell(int32 textId, uint32 language, uint64 TargetGuid)
{
    CellPair p = Trinity::ComputeCellPair(GetPositionX(), GetPositionY());

    Cell cell(p);
    cell.data.Part.reserved = ALL_DISTRICT;
    cell.SetNoCreate();

    Trinity::MessageChatLocaleCacheDo say_do(*this, CHAT_MSG_MONSTER_YELL, textId,language,TargetGuid,sWorld.getConfig(CONFIG_LISTEN_RANGE_YELL));
    Trinity::PlayerWorker<Trinity::MessageChatLocaleCacheDo> say_worker(say_do);
    TypeContainerVisitor<Trinity::PlayerWorker<Trinity::MessageChatLocaleCacheDo>, WorldTypeMapContainer > message(say_worker);
    cell.Visit(p, message, *GetMap(), *this, sWorld.getConfig(CONFIG_LISTEN_RANGE_YELL));
}

void WorldObject::MonsterTextEmote(int32 textId, uint64 TargetGuid, bool IsBossEmote)
{
    CellPair p = Trinity::ComputeCellPair(GetPositionX(), GetPositionY());

    Cell cell(p);
    cell.data.Part.reserved = ALL_DISTRICT;
    cell.SetNoCreate();

    Trinity::MessageChatLocaleCacheDo say_do(*this, IsBossEmote ? CHAT_MSG_RAID_BOSS_EMOTE : CHAT_MSG_MONSTER_EMOTE, textId,LANG_UNIVERSAL,TargetGuid,sWorld.getConfig(CONFIG_LISTEN_RANGE_TEXTEMOTE));
    Trinity::PlayerWorker<Trinity::MessageChatLocaleCacheDo> say_worker(say_do);
    TypeContainerVisitor<Trinity::PlayerWorker<Trinity::MessageChatLocaleCacheDo>, WorldTypeMapContainer > message(say_worker);
    cell.Visit(p, message, *GetMap(), *this, sWorld.getConfig(CONFIG_LISTEN_RANGE_TEXTEMOTE));
}

void WorldObject::MonsterWhisper(int32 textId, uint64 receiver, bool IsBossWhisper)
{
    Player *player = objmgr.GetPlayer(receiver);
    if(!player || !player->GetSession())
        return;

    uint32 loc_idx = player->GetSession()->GetSessionDbLocaleIndex();
    char const* text = objmgr.GetTrinityString(textId,loc_idx);

    WorldPacket data(SMSG_MESSAGECHAT, 200);
    BuildMonsterChat(&data,IsBossWhisper ? CHAT_MSG_RAID_BOSS_WHISPER : CHAT_MSG_MONSTER_WHISPER,text,LANG_UNIVERSAL,GetName(),receiver);

    player->GetSession()->SendPacket(&data);
}

void WorldObject::BuildMonsterChat(WorldPacket *data, uint8 msgtype, char const* text, uint32 language, char const* name, uint64 targetGuid) const
{
    //bool pre = (msgtype==CHAT_MSG_MONSTER_EMOTE || msgtype==CHAT_MSG_RAID_BOSS_EMOTE);
    bool pre = (msgtype == CHAT_MSG_MONSTER_EMOTE);
    
    char const* targetName = "";
    // Generate target name in case of creature
    if (targetGuid && !IS_PLAYER_GUID(targetGuid)) {
        uint32 loc_idx = objmgr.GetDBCLocaleIndex();
        if (Map* map = GetMap()) {
            if (Creature* target = map->GetCreatureInMap(targetGuid))
                targetName = target->GetName();
        }
    }

    *data << (uint8)msgtype;
    *data << (uint32)language;
    *data << (uint64)GetGUID();
    *data << (uint32)0;                                     //2.1.0
    *data << (uint32)(strlen(name)+1);
    *data << name;
    *data << (uint64)targetGuid;                            //Unit Target
    if( targetGuid && !IS_PLAYER_GUID(targetGuid) )
    {
        *data << (uint32)strlen(targetName)+1;                                        // target name length
        *data << targetName;                          // target name
    }
    *data << (uint32)(strlen(text)+1+(pre?3:0));
    if(pre)
        data->append("%s ",3);
    *data << text;
    *data << (uint8)0;                                      // ChatTag
}

void WorldObject::BuildHeartBeatMsg(WorldPacket *data) const
{
    //Heartbeat message cannot be used for non-units
    if (!isType(TYPEMASK_UNIT))
        return;

    data->Initialize(MSG_MOVE_HEARTBEAT, 32);
    data->append(GetPackGUID());
    *data << uint32(((Unit*)this)->GetUnitMovementFlags()); // movement flags
    *data << uint8(0);                                      // 2.3.0
    *data << getMSTime();                                   // time
    *data << m_positionX;
    *data << m_positionY;
    *data << m_positionZ;
    *data << m_orientation;
    *data << uint32(0);
}

void WorldObject::BuildTeleportAckMsg(WorldPacket *data, float x, float y, float z, float ang) const
{
    //TeleportAck message cannot be used for non-units
    if (!isType(TYPEMASK_UNIT))
        return;

    data->Initialize(MSG_MOVE_TELEPORT_ACK, 41);
    data->append(GetPackGUID());
    *data << uint32(0);                                     // this value increments every time
    *data << uint32(((Unit*)this)->GetUnitMovementFlags()); // movement flags
    *data << uint8(0);                                      // 2.3.0
    *data << getMSTime();                                   // time
    *data << x;
    *data << y;
    *data << z;
    *data << ang;
    *data << uint32(0);
}

void WorldObject::SendMessageToSet(WorldPacket *data, bool /*fake*/, bool bToPossessor)
{
    Map* map = MapManager::Instance().GetMap(m_mapId, this);
    if (!map)
        return;

    MapManager::Instance().GetMap(m_mapId, this)->MessageBroadcast(this, data, bToPossessor);
}

void WorldObject::SendMessageToSetInRange(WorldPacket *data, float dist, bool /*bToSelf*/, bool bToPossessor)
{
    MapManager::Instance().GetMap(m_mapId, this)->MessageDistBroadcast(this, data, dist, bToPossessor);
}

void WorldObject::SendObjectDeSpawnAnim(uint64 guid)
{
    WorldPacket data(SMSG_GAMEOBJECT_DESPAWN_ANIM, 8);
    data << guid;
    SendMessageToSet(&data, true);
}

Map* WorldObject::_getMap()
{
    return m_map = MapManager::Instance().GetMap(GetMapId(), this);
}

Map* WorldObject::_findMap()
{
    return m_map = MapManager::Instance().FindMap(GetMapId(), GetInstanceId());
}

Map const* WorldObject::GetBaseMap() const
{
    return MapManager::Instance().GetBaseMap(GetMapId());
}

void WorldObject::AddObjectToRemoveList()
{
    assert(m_uint32Values);

    Map* map = FindMap();
    if(!map)
    {
        sLog.outError("Object (TypeId: %u Entry: %u GUID: %u) at attempt add to move list not have valid map (Id: %u).",GetTypeId(),GetEntry(),GetGUIDLow(),GetMapId());
        return;
    }

    map->AddObjectToRemoveList(this);
}

Creature* WorldObject::SummonCreature(uint32 id, float x, float y, float z, float ang,TempSummonType spwtype,uint32 despwtime)
{
    TemporarySummon* pCreature = new TemporarySummon(GetGUID());

    uint32 team = 0;
    if (GetTypeId()==TYPEID_PLAYER)
        team = (this->ToPlayer())->GetTeam();

    if (!pCreature->Create(objmgr.GenerateLowGuid(HIGHGUID_UNIT,true), GetMap(), id, team))
    {
        delete pCreature;
        return NULL;
    }

    if (x == 0.0f && y == 0.0f && z == 0.0f)
        GetClosePoint(x, y, z, pCreature->GetObjectSize());

    pCreature->Relocate(x, y, z, ang);

    if(!pCreature->IsPositionValid())
    {
        sLog.outError("ERROR: Creature (guidlow %d, entry %d) not summoned. Suggested coordinates isn't valid (X: %f Y: %f)",pCreature->GetGUIDLow(),pCreature->GetEntry(),pCreature->GetPositionX(),pCreature->GetPositionY());
        delete pCreature;
        return NULL;
    }

    pCreature->SetHomePosition(x, y, z, ang);
    pCreature->Summon(spwtype, despwtime);

    if(GetTypeId()==TYPEID_UNIT && (this->ToCreature())->IsAIEnabled) {
        (this->ToCreature())->AI()->JustSummoned(pCreature);
        if ((this->ToCreature())->getAI())
            (this->ToCreature())->getAI()->onSummon(pCreature);
        if(pCreature->getAI())
            pCreature->AI()->IsSummonedBy(this->ToCreature());
    }

    if((pCreature->GetCreatureInfo()->flags_extra & CREATURE_FLAG_EXTRA_TRIGGER) && pCreature->m_spells[0])
    {
        if(GetTypeId() == TYPEID_UNIT || GetTypeId() == TYPEID_PLAYER)
            pCreature->setFaction(((Unit*)this)->getFaction());
        pCreature->CastSpell(pCreature, pCreature->m_spells[0], false, 0, 0, GetGUID());
    }

    //return the creature therewith the summoner has access to it
    return pCreature;
}

Pet* Player::SummonPet(uint32 entry, float x, float y, float z, float ang, PetType petType, uint32 duration)
{
    Pet* pet = new Pet(petType);

    if(petType == SUMMON_PET && pet->LoadPetFromDB(this, entry))
    {
        // Remove Demonic Sacrifice auras (known pet)
        Unit::AuraList const& auraClassScripts = GetAurasByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
        for(Unit::AuraList::const_iterator itr = auraClassScripts.begin();itr!=auraClassScripts.end();)
        {
            if((*itr)->GetModifier()->m_miscvalue==2228)
            {
                RemoveAurasDueToSpell((*itr)->GetId());
                itr = auraClassScripts.begin();
            }
            else
                ++itr;
        }

        if(duration > 0)
            pet->SetDuration(duration);

        return NULL;
    }

    // petentry==0 for hunter "call pet" (current pet summoned if any)
    if(!entry)
    {
        delete pet;
        return NULL;
    }

    Map *map = GetMap();
    uint32 pet_number = objmgr.GeneratePetNumber();
    if(!pet->Create(objmgr.GenerateLowGuid(HIGHGUID_PET), map, entry, pet_number))
    {
        sLog.outError("no such creature entry %u", entry);
        delete pet;
        return NULL;
    }

    pet->Relocate(x, y, z, ang);

    if(!pet->IsPositionValid())
    {
        sLog.outError("ERROR: Pet (guidlow %d, entry %d) not summoned. Suggested coordinates isn't valid (X: %f Y: %f)",pet->GetGUIDLow(),pet->GetEntry(),pet->GetPositionX(),pet->GetPositionY());
        delete pet;
        return NULL;
    }

    pet->SetOwnerGUID(GetGUID());
    pet->SetCreatorGUID(GetGUID());
    pet->SetUInt32Value(UNIT_FIELD_FACTIONTEMPLATE, getFaction());

    // this enables pet details window (Shift+P)
    pet->GetCharmInfo()->SetPetNumber(pet_number, false);

    pet->AIM_Initialize();

    map->Add(pet->ToCreature());

    pet->setPowerType(POWER_MANA);
    pet->SetUInt32Value(UNIT_NPC_FLAGS , 0);
    pet->SetUInt32Value(UNIT_FIELD_BYTES_1,0);
    pet->InitStatsForLevel(getLevel());

    switch(petType)
    {
        case GUARDIAN_PET:
        case POSSESSED_PET:
            pet->SetUInt32Value(UNIT_FIELD_FLAGS,0);
            AddGuardian(pet);
            break;
        case SUMMON_PET:
            pet->SetUInt32Value(UNIT_FIELD_BYTES_0, 2048);
            pet->SetUInt32Value(UNIT_FIELD_PETEXPERIENCE, 0);
            pet->SetUInt32Value(UNIT_FIELD_PETNEXTLEVELEXP, 1000);
            pet->SetHealth(pet->GetMaxHealth());
            pet->SetPower(POWER_MANA, pet->GetMaxPower(POWER_MANA));
            pet->InitPetCreateSpells();
            pet->SavePetToDB(PET_SAVE_AS_CURRENT);
            SetPet(pet);
            PetSpellInitialize();
            break;
    }

    if(petType == SUMMON_PET)
    {
        // Remove Demonic Sacrifice auras (known pet)
        Unit::AuraList const& auraClassScripts = GetAurasByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
        for(Unit::AuraList::const_iterator itr = auraClassScripts.begin();itr!=auraClassScripts.end();)
        {
            if((*itr)->GetModifier()->m_miscvalue==2228)
            {
                RemoveAurasDueToSpell((*itr)->GetId());
                itr = auraClassScripts.begin();
            }
            else
                ++itr;
        }
    }

    if(duration > 0)
        pet->SetDuration(duration);

    return pet;
}

Pet* Unit::SummonPet(uint32 entry, float x, float y, float z, float ang, uint32 duration)
{
    PetType petType = SUMMON_PET;
    Pet* pet = new Pet(petType);

    // petentry==0 for hunter "call pet" (current pet summoned if any)
    if(!entry)
    {
        delete pet;
        return NULL;
    }

    Map *map = GetMap();
    uint32 pet_number = objmgr.GeneratePetNumber();
    if(!pet->Create(objmgr.GenerateLowGuid(HIGHGUID_PET), map, entry, pet_number))
    {
        sLog.outError("no such creature entry %u", entry);
        delete pet;
        return NULL;
    }

    pet->Relocate(x, y, z, ang);

    if(!pet->IsPositionValid())
    {
        sLog.outError("ERROR: Pet (guidlow %d, entry %d) not summoned. Suggested coordinates isn't valid (X: %f Y: %f)",pet->GetGUIDLow(),pet->GetEntry(),pet->GetPositionX(),pet->GetPositionY());
        delete pet;
        return NULL;
    }

    pet->SetOwnerGUID(GetGUID());
    pet->SetCreatorGUID(GetGUID());
    pet->SetUInt32Value(UNIT_FIELD_FACTIONTEMPLATE, getFaction());

    pet->AIM_Initialize();

    map->Add(pet->ToCreature());

    pet->setPowerType(POWER_MANA);
    pet->SetUInt32Value(UNIT_NPC_FLAGS , 0);
    pet->SetUInt32Value(UNIT_FIELD_BYTES_1,0);
    pet->InitStatsForLevel(getLevel());

    switch(petType)
    {
        case SUMMON_PET:
            pet->SetUInt32Value(UNIT_FIELD_BYTES_0, 2048);
            pet->SetUInt32Value(UNIT_FIELD_PETEXPERIENCE, 0);
            pet->SetUInt32Value(UNIT_FIELD_PETNEXTLEVELEXP, 1000);
            pet->SetHealth(pet->GetMaxHealth());
            pet->SetPower(POWER_MANA, pet->GetMaxPower(POWER_MANA));
            pet->InitPetCreateSpells();
            SetPet(pet);
            break;
    }

    if(duration > 0)
        pet->SetDuration(duration);

    return pet;
}

GameObject* WorldObject::SummonGameObject(uint32 entry, float x, float y, float z, float ang, float rotation0, float rotation1, float rotation2, float rotation3, uint32 respawnTime)
{
    if(!IsInWorld())
        return NULL;

    GameObjectInfo const* goinfo = objmgr.GetGameObjectInfo(entry);
    if(!goinfo)
    {
        sLog.outErrorDb("Gameobject template %u not found in database!", entry);
        return NULL;
    }
    Map *map = GetMap();
    GameObject *go = new GameObject();
    if(!go->Create(objmgr.GenerateLowGuid(HIGHGUID_GAMEOBJECT,true),entry,map,x,y,z,ang,rotation0,rotation1,rotation2,rotation3,100,1))
    {
        delete go;
        return NULL;
    }
    go->SetRespawnTime(respawnTime);
    if(GetTypeId()==TYPEID_PLAYER || GetTypeId()==TYPEID_UNIT) //not sure how to handle this
        ((Unit*)this)->AddGameObject(go);
    else
        go->SetSpawnedByDefault(false);
    map->Add(go);

    return go;
}

Creature* WorldObject::SummonTrigger(float x, float y, float z, float ang, uint32 duration, CreatureAI* (*GetAI)(Creature*))
{
    TempSummonType summonType = (duration == 0) ? TEMPSUMMON_DEAD_DESPAWN : TEMPSUMMON_TIMED_DESPAWN;
    Creature* summon = SummonCreature(WORLD_TRIGGER, x, y, z, ang, summonType, duration);
    if(!summon)
        return NULL;

    //summon->SetName(GetName());
    if(GetTypeId()==TYPEID_PLAYER || GetTypeId()==TYPEID_UNIT)
    {
        summon->setFaction(((Unit*)this)->getFaction());
        summon->SetLevel(((Unit*)this)->getLevel());
    }

    if(GetAI)
        summon->AIM_Initialize(GetAI(summon));
    return summon;
}

Creature* WorldObject::FindNearestCreature(uint32 entry, float range, bool alive)
{
       Creature *creature = NULL;
       Trinity::NearestCreatureEntryWithLiveStateInObjectRangeCheck checker(*this, entry, alive, range);
       Trinity::CreatureLastSearcher<Trinity::NearestCreatureEntryWithLiveStateInObjectRangeCheck> searcher(creature, checker);
       VisitNearbyObject(range, searcher);
       return creature;
}

GameObject* WorldObject::FindNearestGameObject(uint32 entry, float range)
{
       GameObject *go = NULL;
       Trinity::NearestGameObjectEntryInObjectRangeCheck checker(*this, entry, range);
       Trinity::GameObjectLastSearcher<Trinity::NearestGameObjectEntryInObjectRangeCheck> searcher(go, checker);
       VisitNearbyGridObject(range, searcher);
       return go;
}

Player* WorldObject::FindNearestPlayer(float range)
{
       Player* pl = NULL;
       Trinity::AnyPlayerInObjectRangeCheck checker(this, range);
       Trinity::PlayerSearcher<Trinity::AnyPlayerInObjectRangeCheck> searcher(pl, checker);
       VisitNearbyObject(range, searcher);
       return pl;
}

void WorldObject::GetNearPoint2D(float &x, float &y, float distance2d, float absAngle ) const
{
    x = GetPositionX() + (GetObjectSize() + distance2d) * cos(absAngle);
    y = GetPositionY() + (GetObjectSize() + distance2d) * sin(absAngle);

    Trinity::NormalizeMapCoord(x);
    Trinity::NormalizeMapCoord(y);
}

void WorldObject::GetNearPoint(WorldObject const* searcher, float &x, float &y, float &z, float searcher_size, float distance2d, float absAngle ) const
{
    GetNearPoint2D(x,y,distance2d+searcher_size,absAngle);
    z = GetPositionZ();
    UpdateAllowedPositionZ(x, y, z);
}

void WorldObject::GetGroundPoint(float &x, float &y, float &z, float dist, float angle)
{
    angle += GetOrientation();
    x += dist * cos(angle);
    y += dist * sin(angle);
    Trinity::NormalizeMapCoord(x);
    Trinity::NormalizeMapCoord(y);
    UpdateGroundPositionZ(x, y, z);
}

void WorldObject::GetFirstCollisionPosition(float& x, float& y, float& z, float dist, float angle, bool keepZ)
{
    GetPosition(x,y,z);
    MovePositionToFirstCollision(x, y, z, dist, angle, keepZ);
}

void WorldObject::MovePositionToFirstCollision(float& x, float& y, float& z, float dist, float angle, bool keepZ)
{
    angle += GetOrientation();
    float destx, desty, destz, ground, floor;
    z += 2.0f;
    destx = x + dist * std::cos(angle);
    desty = y + dist * std::sin(angle);

    // Prevent invalid coordinates here, position is unchanged
    if (!Trinity::IsValidMapCoord(destx, desty))
    {
        sLog.outError("WorldObject::MovePositionToFirstCollision invalid coordinates X: %f and Y: %f were passed!", destx, desty);
        return;
    }

    if(keepZ)
        destz = z;
    else
    {
        ground = GetMap()->GetHeight(destx, desty, MAX_HEIGHT, true);
        floor = GetMap()->GetHeight(destx, desty, z, true);
        destz = fabs(ground - z) <= fabs(floor - z) ? ground : floor;
    }

    bool col = VMAP::VMapFactory::createOrGetVMapManager()->getObjectHitPos(GetMapId(), x, y, z+0.5f, destx, desty, destz+0.5f, destx, desty, destz, -0.5f);

    // collision occured
    if (col)
    {
        // move back a bit
        destx -= CONTACT_DISTANCE * std::cos(angle);
        desty -= CONTACT_DISTANCE * std::sin(angle);
        dist = sqrt((x - destx)*(x - destx) + (y - desty)*(y - desty));
    }

    // check dynamic collision
    col = GetMap()->getObjectHitPos(0, x, y, z+0.5f, destx, desty, destz+0.5f, destx, desty, destz, -0.5f);

    // Collided with a gameobject
    if (col)
    {
        destx -= CONTACT_DISTANCE * std::cos(angle);
        desty -= CONTACT_DISTANCE * std::sin(angle);
        dist = sqrt((x - destx)*(x - destx) + (y - desty)*(y - desty));
    }

    float step = dist/10.0f;

    for (uint8 j = 0; j < 10; ++j)
    {
        // do not allow too big z changes
        if (fabs(z - destz) > 6)
        {
            destx -= step * std::cos(angle);
            desty -= step * std::sin(angle);
            ground = GetMap()->GetHeight(destx, desty, MAX_HEIGHT, true);
            floor = GetMap()->GetHeight(destx, desty, z, true);
            destz = fabs(ground - z) <= fabs(floor - z) ? ground : floor;
        }
        // we have correct destz now
        else
        {
            x = destx;
            y = desty;
            z = destz;
            break;
        }
    }

    Trinity::NormalizeMapCoord(x);
    Trinity::NormalizeMapCoord(y);
    UpdateAllowedPositionZ(x, y, z);
    //pos.SetOrientation(GetOrientation());
}

bool Position::HasInArc(float arc, const Position *obj, float border) const
{
    // always have self in arc
    if (obj == this)
        return true;

    // move arc to range 0.. 2*pi
    arc = Trinity::NormalizeOrientation(arc);

    float angle = GetAngle(obj);
    angle -= m_orientation;

    // move angle to range -pi ... +pi
    angle = Trinity::NormalizeOrientation(angle);
    if (angle > M_PI)
        angle -= 2.0f*M_PI;

    float lborder = -1 * (arc/border);                        // in range -pi..0
    float rborder = (arc/border);                             // in range 0..pi
    return ((angle >= lborder) && (angle <= rborder));
}

bool Position::HasInLine(const Unit* const target, float width) const
{
    if (!HasInArc(float(M_PI), target))
        return false;
    width += target->GetObjectSize();
    float angle = GetRelativeAngle(target);
    return fabs(sin(angle)) * GetExactDist2d(target->GetPositionX(), target->GetPositionY()) < width;
}

bool WorldObject::isInFront(WorldObject const* target,  float arc) const
{
    return HasInArc(arc, target);
}

bool WorldObject::isInBack(WorldObject const* target, float arc) const
{
    return !HasInArc(2 * M_PI - arc, target);
}

std::string Position::ToString() const
{
    std::stringstream sstr;
    sstr << "X: " << m_positionX << " Y: " << m_positionY << " Z: " << m_positionZ << " O: " << m_orientation;
    return sstr.str();
}

ByteBuffer &operator>>(ByteBuffer& buf, Position::PositionXYZOStreamer const & streamer)
{
    float x, y, z, o;
    buf >> x >> y >> z >> o;
    streamer.m_pos->Relocate(x, y, z, o);
    return buf;
}
ByteBuffer & operator<<(ByteBuffer& buf, Position::PositionXYZStreamer const & streamer)
{
    float x, y, z;
    streamer.m_pos->GetPosition(x, y, z);
    buf << x << y << z;
    return buf;
}

ByteBuffer &operator>>(ByteBuffer& buf, Position::PositionXYZStreamer const & streamer)
{
    float x, y, z;
    buf >> x >> y >> z;
    streamer.m_pos->Relocate(x, y, z);
    return buf;
}

ByteBuffer & operator<<(ByteBuffer& buf, Position::PositionXYZOStreamer const & streamer)
{
    float x, y, z, o;
    streamer.m_pos->GetPosition(x, y, z, o);
    buf << x << y << z << o;
    return buf;
}

void Position::RelocateOffset(const Position & offset)
{
    m_positionX = GetPositionX() + (offset.GetPositionX() * cos(GetOrientation()) + offset.GetPositionY() * sin(GetOrientation() + M_PI));
    m_positionY = GetPositionY() + (offset.GetPositionY() * cos(GetOrientation()) + offset.GetPositionX() * sin(GetOrientation()));
    m_positionZ = GetPositionZ() + offset.GetPositionZ();
    m_orientation = GetOrientation() + offset.GetOrientation();
}

void Position::GetPositionOffsetTo(const Position & endPos, Position & retOffset) const
{
    float dx = endPos.GetPositionX() - GetPositionX();
    float dy = endPos.GetPositionY() - GetPositionY();

    retOffset.m_positionX = dx * cos(GetOrientation()) + dy * sin(GetOrientation());
    retOffset.m_positionY = dy * cos(GetOrientation()) - dx * sin(GetOrientation());
    retOffset.m_positionZ = endPos.GetPositionZ() - GetPositionZ();
    retOffset.m_orientation = endPos.GetOrientation() - GetOrientation();
}

float Position::GetAngle(const Position *obj) const
{
    if (!obj) return 0;
    return GetAngle(obj->GetPositionX(), obj->GetPositionY());
}

// Return angle in range 0..2*pi
float Position::GetAngle(const float x, const float y) const
{
    float dx = x - GetPositionX();
    float dy = y - GetPositionY();

    float ang = atan2(dy, dx);
    ang = (ang >= 0) ? ang : 2 * M_PI + ang;
    return ang;
}

void Position::GetSinCos(const float x, const float y, float &vsin, float &vcos) const
{
    float dx = GetPositionX() - x;
    float dy = GetPositionY() - y;

    if (dx < 0.001f && dy < 0.001f)
    {
        float angle = (float)rand_norm()*static_cast<float>(2*M_PI);
        vcos = cos(angle);
        vsin = sin(angle);
    }
    else
    {
        float dist = sqrt((dx*dx) + (dy*dy));
        vcos = dx / dist;
        vsin = dy / dist;
    }
}

bool Position::IsPositionValid() const
{
    return Trinity::IsValidMapCoord(m_positionX, m_positionY, m_positionZ, m_orientation);
}

void WorldObject::GetCreatureListWithEntryInGrid(std::list<Creature*>& lList, uint32 uiEntry, float fMaxSearchRange)
{
    CellPair pair(Trinity::ComputeCellPair(this->GetPositionX(), this->GetPositionY()));
    Cell cell(pair);
    cell.data.Part.reserved = ALL_DISTRICT;
    cell.SetNoCreate();

    Trinity::AllCreaturesOfEntryInRange check((Unit const*)this, uiEntry, fMaxSearchRange);
    Trinity::CreatureListSearcher<Trinity::AllCreaturesOfEntryInRange> searcher(lList, check);
    TypeContainerVisitor<Trinity::CreatureListSearcher<Trinity::AllCreaturesOfEntryInRange>, GridTypeMapContainer> visitor(searcher);

    cell.Visit(pair, visitor, *(this->GetMap()));
}

void WorldObject::GetGameObjectListWithEntryInGrid(std::list<GameObject*>& lList, uint32 uiEntry, float fMaxSearchRange)
{
    CellPair pair(Trinity::ComputeCellPair(this->GetPositionX(), this->GetPositionY()));
    Cell cell(pair);
    cell.data.Part.reserved = ALL_DISTRICT;
    cell.SetNoCreate();

    Trinity::AllGameObjectsWithEntryInRange check(this, uiEntry, fMaxSearchRange);
    Trinity::GameObjectListSearcher<Trinity::AllGameObjectsWithEntryInRange> searcher(lList, check);
    TypeContainerVisitor<Trinity::GameObjectListSearcher<Trinity::AllGameObjectsWithEntryInRange>, GridTypeMapContainer> visitor(searcher);

    cell.Visit(pair, visitor, *(this->GetMap()));
}
