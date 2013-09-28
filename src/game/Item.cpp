/*
 * Copyright (C) 2005-2008 MaNGOS <http://www.mangosproject.org/>
 *
 * Copyright (C) 2008 Trinity <http://www.trinitycore.org/>
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

#include "Common.h"
#include "Item.h"
#include "ObjectMgr.h"
#include "WorldPacket.h"
#include "Database/DatabaseEnv.h"
#include "ItemEnchantmentMgr.h"

void AddItemsSetItem(Player*player,Item *item)
{
    ItemPrototype const *proto = item->GetProto();
    uint32 setid = proto->ItemSet;

    ItemSetEntry const *set = sItemSetStore.LookupEntry(setid);

    if(!set)
    {
        sLog.outErrorDb("Item set %u for item (id %u) not found, mods not applied.",setid,proto->ItemId);
        return;
    }

    if( set->required_skill_id && player->GetSkillValue(set->required_skill_id) < set->required_skill_value )
        return;

    ItemSetEffect *eff = NULL;

    for(size_t x = 0; x < player->ItemSetEff.size(); ++x)
    {
        if(player->ItemSetEff[x] && player->ItemSetEff[x]->setid == setid)
        {
            eff = player->ItemSetEff[x];
            break;
        }
    }

    if(!eff)
    {
        eff = new ItemSetEffect;
        memset(eff,0,sizeof(ItemSetEffect));
        eff->setid = setid;

        size_t x = 0;
        for(; x < player->ItemSetEff.size(); x++)
            if(!player->ItemSetEff[x])
                break;

        if(x < player->ItemSetEff.size())
            player->ItemSetEff[x]=eff;
        else
            player->ItemSetEff.push_back(eff);
    }

    ++eff->item_count;

    for(uint32 x=0;x<8;x++)
    {
        if(!set->spells [x])
            continue;
        //not enough for  spell
        if(set->items_to_triggerspell[x] > eff->item_count)
            continue;

        uint32 z=0;
        for(;z<8;z++)
            if(eff->spells[z] && eff->spells[z]->Id==set->spells[x])
                break;

        if(z < 8)
            continue;

        //new spell
        for(uint32 y=0;y<8;y++)
        {
            if(!eff->spells[y])                             // free slot
            {
                SpellEntry const *spellInfo = spellmgr.LookupSpell(set->spells[x]);
                if(!spellInfo)
                {
                    sLog.outError("WORLD: unknown spell id %u in items set %u effects", set->spells[x],setid);
                    break;
                }

                // spell casted only if fit form requirement, in other case will casted at form change
                player->ApplyEquipSpell(spellInfo,NULL,true);
                eff->spells[y] = spellInfo;
                break;
            }
        }
    }
}

void RemoveItemsSetItem(Player*player,ItemPrototype const *proto)
{
    uint32 setid = proto->ItemSet;

    ItemSetEntry const *set = sItemSetStore.LookupEntry(setid);

    if(!set)
    {
        sLog.outErrorDb("Item set #%u for item #%u not found, mods not removed.",setid,proto->ItemId);
        return;
    }

    ItemSetEffect *eff = NULL;
    size_t setindex = 0;
    for(;setindex < player->ItemSetEff.size(); setindex++)
    {
        if(player->ItemSetEff[setindex] && player->ItemSetEff[setindex]->setid == setid)
        {
            eff = player->ItemSetEff[setindex];
            break;
        }
    }

    // can be in case now enough skill requirement for set appling but set has been appliend when skill requirement not enough
    if(!eff)
        return;

    --eff->item_count;

    for(uint32 x=0;x<8;x++)
    {
        if(!set->spells[x])
            continue;

        // enough for spell
        if(set->items_to_triggerspell[x] <= eff->item_count)
            continue;

        for(uint32 z=0;z<8;z++)
        {
            if(eff->spells[z] && eff->spells[z]->Id==set->spells[x])
            {
                // spell can be not active if not fit form requirement
                player->ApplyEquipSpell(eff->spells[z],NULL,false);
                eff->spells[z]=NULL;
                break;
            }
        }
    }

    if(!eff->item_count)                                    //all items of a set were removed
    {
        assert(eff == player->ItemSetEff[setindex]);
        delete eff;
        player->ItemSetEff[setindex] = NULL;
    }
}

bool ItemCanGoIntoBag(ItemPrototype const *pProto, ItemPrototype const *pBagProto)
{
    if(!pProto || !pBagProto)
        return false;

    switch(pBagProto->Class)
    {
        case ITEM_CLASS_CONTAINER:
            switch(pBagProto->SubClass)
            {
                case ITEM_SUBCLASS_CONTAINER:
                    return true;
                case ITEM_SUBCLASS_SOUL_CONTAINER:
                    if(!(pProto->BagFamily & BAG_FAMILY_MASK_SOUL_SHARDS))
                        return false;
                    return true;
                case ITEM_SUBCLASS_HERB_CONTAINER:
                    if(!(pProto->BagFamily & BAG_FAMILY_MASK_HERBS))
                        return false;
                    return true;
                case ITEM_SUBCLASS_ENCHANTING_CONTAINER:
                    if(!(pProto->BagFamily & BAG_FAMILY_MASK_ENCHANTING_SUPP))
                        return false;
                    return true;
                case ITEM_SUBCLASS_MINING_CONTAINER:
                    if(!(pProto->BagFamily & BAG_FAMILY_MASK_MINING_SUPP))
                        return false;
                    return true;
                case ITEM_SUBCLASS_ENGINEERING_CONTAINER:
                    if(!(pProto->BagFamily & BAG_FAMILY_MASK_ENGINEERING_SUPP))
                        return false;
                    return true;
                case ITEM_SUBCLASS_GEM_CONTAINER:
                    if(!(pProto->BagFamily & BAG_FAMILY_MASK_GEMS))
                        return false;
                    return true;
                case ITEM_SUBCLASS_LEATHERWORKING_CONTAINER:
                    if(!(pProto->BagFamily & BAG_FAMILY_MASK_LEATHERWORKING_SUPP))
                        return false;
                    return true;
                default:
                    return false;
            }
        case ITEM_CLASS_QUIVER:
            switch(pBagProto->SubClass)
            {
                case ITEM_SUBCLASS_QUIVER:
                    if(!(pProto->BagFamily & BAG_FAMILY_MASK_ARROWS))
                        return false;
                    return true;
                case ITEM_SUBCLASS_AMMO_POUCH:
                    if(!(pProto->BagFamily & BAG_FAMILY_MASK_BULLETS))
                        return false;
                    return true;
                default:
                    return false;
            }
    }
    return false;
}

Item::Item( )
{
    m_objectType |= TYPEMASK_ITEM;
    m_objectTypeId = TYPEID_ITEM;
                                                            // 2.3.2 - 0x18
    m_updateFlag = (UPDATEFLAG_LOWGUID | UPDATEFLAG_HIGHGUID);

    m_valuesCount = ITEM_END;
    m_slot = 0;
    uState = ITEM_NEW;
    uQueuePos = -1;
    m_container = NULL;
    m_lootGenerated = false;
    mb_in_trade = false;
}

bool Item::Create( uint32 guidlow, uint32 itemid, Player const* owner)
{
    Object::_Create( guidlow, 0, HIGHGUID_ITEM );

    SetEntry(itemid);
    SetFloatValue(OBJECT_FIELD_SCALE_X, 1.0f);

    SetUInt64Value(ITEM_FIELD_OWNER, owner ? owner->GetGUID() : 0);
    SetUInt64Value(ITEM_FIELD_CONTAINED, owner ? owner->GetGUID() : 0);

    ItemPrototype const *itemProto = objmgr.GetItemPrototype(itemid);
    if(!itemProto)
        return false;

    SetUInt32Value(ITEM_FIELD_STACK_COUNT, 1);
    SetUInt32Value(ITEM_FIELD_MAXDURABILITY, itemProto->MaxDurability);
    SetUInt32Value(ITEM_FIELD_DURABILITY, itemProto->MaxDurability);

    for(int i = 0; i < 5; ++i)
        SetSpellCharges(i,itemProto->Spells[i].SpellCharges);

    SetUInt32Value(ITEM_FIELD_FLAGS, itemProto->Flags);
    SetUInt32Value(ITEM_FIELD_DURATION, abs(itemProto->Duration));

    return true;
}

void Item::UpdateDuration(Player* owner, uint32 diff)
{
    if (!GetUInt32Value(ITEM_FIELD_DURATION))
        return;

    if (GetUInt32Value(ITEM_FIELD_DURATION)<=diff)
    {
        owner->DestroyItem(GetBagSlot(), GetSlot(), true);
        return;
    }

    SetUInt32Value(ITEM_FIELD_DURATION, GetUInt32Value(ITEM_FIELD_DURATION) - diff);
    SetState(ITEM_CHANGED);                                 // save new time in database
}

void Item::SaveToDB(SQLTransaction trans)
{
    uint32 guid = GetGUIDLow();
    switch (uState)
    {
        case ITEM_NEW:
        {
            std::ostringstream ss;
            ss << "REPLACE INTO item_instance (`guid`,`owner_guid`,`data`, \
                  `template`, `container_guid`, `creator`,`gift_creator`, `stacks`, `duration`, `spell1_charges`, `spell2_charges`, \
                  `spell3_charges`, `spell4_charges`, `spell5_charges`, `flags`, `enchant1_id`, `enchant1_duration`, `enchant1_charges`,\
                  `enchant2_id`, `enchant2_duration`, `enchant2_charges`, `enchant3_id`, `enchant3_duration`, `enchant3_charges`,\
                  `enchant4_id`, `enchant4_duration`, `enchant4_charges`, `enchant5_id`, `enchant5_duration`, `enchant5_charges`, \
                  `enchant6_id`, `enchant6_duration`, `enchant6_charges`, `enchant7_id`, `enchant7_duration`, `enchant7_charges`, \
                  `enchant8_id`, `enchant8_duration`, `enchant8_charges`, `enchant9_id`, `enchant9_duration`, `enchant9_charges`, \
                  `enchant10_id`, `enchant10_duration`,`enchant10_charges`, `enchant11_id`, `enchant11_duration`, `enchant11_charges`,\
                  `property_seed`, `random_prop_id`,  `text_id`, `durability`, `max_durability`, `num_slots`) VALUES (" 
                  << guid << "," << GUID_LOPART(GetOwnerGUID()) << ",'";
            for(uint16 i = 0; i < m_valuesCount; i++ )
                ss << GetUInt32Value(i) << " ";
            ss << "','";

            ss << GetUInt32Value(OBJECT_FIELD_ENTRY) << "','"; //`template`
            ss << GetUInt64Value(ITEM_FIELD_CONTAINED) << "','"; // `container_guid`
            ss << GetUInt64Value(ITEM_FIELD_CREATOR) << "','"; // `creator`
            ss << GetUInt64Value(ITEM_FIELD_GIFTCREATOR) << "','"; // `gift_creator`
            ss << GetUInt32Value(ITEM_FIELD_STACK_COUNT) << "','"; // `stacks`
            ss << GetUInt32Value(ITEM_FIELD_DURATION) << "','"; // `duration`
            for(uint16 i = 0; i < 5; i++)
                ss << GetUInt32Value(ITEM_FIELD_SPELL_CHARGES+i) << "','"; // `spell1_charges` -> `spell5_charges`
            ss << GetUInt32Value(ITEM_FIELD_FLAGS) << "','"; // `flags`
            for(uint16 i = 0; i < 33; i++)
                ss << GetUInt32Value(ITEM_FIELD_ENCHANTMENT+i) << "','"; // `enchant1_id`,`enchant1_duration`,`enchant1_charges` -> `enchant11_id` ... `enchant11_charges`
            ss << GetUInt32Value(ITEM_FIELD_PROPERTY_SEED) << "','"; // `property_seed`
            ss << GetUInt32Value(ITEM_FIELD_RANDOM_PROPERTIES_ID) << "','"; // `random_prop_id`
            ss << GetUInt32Value(ITEM_FIELD_ITEM_TEXT_ID) << "','"; // `text_id`
            ss << GetUInt32Value(ITEM_FIELD_DURABILITY) << "','"; // `durability`
            ss << GetUInt32Value(ITEM_FIELD_MAXDURABILITY) << "','"; // `max_durability`
            //ss << GetUInt32Value(CONTAINER_FIELD_NUM_SLOTS) << "','"; // `num_slots` ?
            ss << "0";

            ss << "' )";
            trans->Append(ss.str().c_str());
             //`template`, `container_guid`, `creator`, `gift_creator`, `stacks`, `duration`, `spell1_charges`, `spell2_charges`, `spell3_charges`, `spell4_charges`, `spell5_charges`, `flags`, `enchant1_id`, `enchant1_duration`, `enchant1_charges`, `enchant2_id`, `enchant2_duration`, `enchant2_charges`, `enchant3_id`, `enchant3_duration`, `enchant3_charges`, `enchant4_id`, `enchant4_duration`, `enchant4_charges`, `enchant5_id`, `enchant5_duration`, `enchant5_charges`, `enchant6_id`, `enchant6_duration`, `enchant6_charges`, `enchant7_id`, `enchant7_duration`, `enchant7_charges`, `enchant8_id`, `enchant8_duration`, `enchant8_charges`, `enchant9_id`, `enchant9_duration`, `enchant9_charges`, `enchant10_id`, `enchant10_duration`, `enchant10_charges`, `enchant11_id`, `enchant11_duration`, `enchant11_charges`, `property_seed`, `random_prop_id`, `text_id`, `durability`, `max_durability`, `num_slots`
        } break;
        case ITEM_CHANGED:
        {
            std::ostringstream ss;
            ss << "UPDATE item_instance SET data = '";
            for(uint16 i = 0; i < m_valuesCount; i++ )
                ss << GetUInt32Value(i) << " ";
            ss << "', owner_guid = '" << GUID_LOPART(GetOwnerGUID()) << "',";
                
            ss << "`template` = '" << GetUInt32Value(OBJECT_FIELD_ENTRY) << "',";
            ss << "`container_guid` = '" << GetUInt64Value(ITEM_FIELD_CONTAINED) << "',";
            ss << "`creator` = '" << GetUInt64Value(ITEM_FIELD_CREATOR) << "',";
            ss << "`gift_creator` = '" << GetUInt64Value(ITEM_FIELD_GIFTCREATOR) << "',";
            ss << "`stacks` = '" << GetUInt32Value(ITEM_FIELD_STACK_COUNT) << "',";
            ss << "`duration` = '" << GetUInt32Value(ITEM_FIELD_DURATION) << "',"; 
            ss << "`spell1_charges` = '" << GetUInt32Value(ITEM_FIELD_SPELL_CHARGES) << "',";
            ss << "`spell2_charges` = '" << GetUInt32Value(ITEM_FIELD_SPELL_CHARGES +1) << "',";
            ss << "`spell3_charges` = '" << GetUInt32Value(ITEM_FIELD_SPELL_CHARGES +2) << "',";
            ss << "`spell4_charges` = '" << GetUInt32Value(ITEM_FIELD_SPELL_CHARGES +3) << "',";
            ss << "`spell5_charges` = '" << GetUInt32Value(ITEM_FIELD_SPELL_CHARGES +4) << "',";
            ss << "`flags` = '" << GetUInt32Value(ITEM_FIELD_FLAGS) << "',"; 

            ss << "`enchant1_id` = '"       << GetUInt32Value(ITEM_FIELD_ENCHANTMENT +0) << "',";
            ss << "`enchant1_duration` = '" << GetUInt32Value(ITEM_FIELD_ENCHANTMENT +1) << "',";
            ss << "`enchant1_charges` = '"  << GetUInt32Value(ITEM_FIELD_ENCHANTMENT +2) << "',";
            ss << "`enchant2_id` = '"       << GetUInt32Value(ITEM_FIELD_ENCHANTMENT +3) << "',";
            ss << "`enchant2_duration` = '" << GetUInt32Value(ITEM_FIELD_ENCHANTMENT +4) << "',";
            ss << "`enchant2_charges` = '"  << GetUInt32Value(ITEM_FIELD_ENCHANTMENT +5) << "',";
            ss << "`enchant3_id` = '"       << GetUInt32Value(ITEM_FIELD_ENCHANTMENT +6) << "',";
            ss << "`enchant3_duration` = '" << GetUInt32Value(ITEM_FIELD_ENCHANTMENT +7) << "',";
            ss << "`enchant3_charges` = '"  << GetUInt32Value(ITEM_FIELD_ENCHANTMENT +8) << "',";
            ss << "`enchant4_id` = '"       << GetUInt32Value(ITEM_FIELD_ENCHANTMENT +9) << "',";
            ss << "`enchant4_duration` = '" << GetUInt32Value(ITEM_FIELD_ENCHANTMENT +10) << "',";
            ss << "`enchant4_charges` = '"  << GetUInt32Value(ITEM_FIELD_ENCHANTMENT +11) << "',";
            ss << "`enchant5_id` = '"       << GetUInt32Value(ITEM_FIELD_ENCHANTMENT +12) << "',";
            ss << "`enchant5_duration` = '" << GetUInt32Value(ITEM_FIELD_ENCHANTMENT +13) << "',";
            ss << "`enchant5_charges` = '"  << GetUInt32Value(ITEM_FIELD_ENCHANTMENT +14) << "',";
            ss << "`enchant6_id` = '"       << GetUInt32Value(ITEM_FIELD_ENCHANTMENT +15) << "',";
            ss << "`enchant6_duration` = '" << GetUInt32Value(ITEM_FIELD_ENCHANTMENT +16) << "',";
            ss << "`enchant6_charges` = '"  << GetUInt32Value(ITEM_FIELD_ENCHANTMENT +17) << "',";
            ss << "`enchant7_id` = '"       << GetUInt32Value(ITEM_FIELD_ENCHANTMENT +18) << "',";
            ss << "`enchant7_duration` = '" << GetUInt32Value(ITEM_FIELD_ENCHANTMENT +19) << "',";
            ss << "`enchant7_charges` = '"  << GetUInt32Value(ITEM_FIELD_ENCHANTMENT +20) << "',";
            ss << "`enchant8_id` = '"       << GetUInt32Value(ITEM_FIELD_ENCHANTMENT +21) << "',";
            ss << "`enchant8_duration` = '" << GetUInt32Value(ITEM_FIELD_ENCHANTMENT +22) << "',";
            ss << "`enchant8_charges` = '"  << GetUInt32Value(ITEM_FIELD_ENCHANTMENT +23) << "',";
            ss << "`enchant9_id` = '"       << GetUInt32Value(ITEM_FIELD_ENCHANTMENT +24) << "',";
            ss << "`enchant9_duration` = '" << GetUInt32Value(ITEM_FIELD_ENCHANTMENT +25) << "',";
            ss << "`enchant9_charges` = '"  << GetUInt32Value(ITEM_FIELD_ENCHANTMENT +26) << "',";
            ss << "`enchant10_id` = '"       << GetUInt32Value(ITEM_FIELD_ENCHANTMENT +27) << "',";
            ss << "`enchant10_duration` = '" << GetUInt32Value(ITEM_FIELD_ENCHANTMENT +28) << "',";
            ss << "`enchant10_charges` = '"  << GetUInt32Value(ITEM_FIELD_ENCHANTMENT +29) << "',";
            ss << "`enchant11_id` = '"       << GetUInt32Value(ITEM_FIELD_ENCHANTMENT +30) << "',";
            ss << "`enchant11_duration` = '" << GetUInt32Value(ITEM_FIELD_ENCHANTMENT +31) << "',";
            ss << "`enchant11_charges` = '"  << GetUInt32Value(ITEM_FIELD_ENCHANTMENT +32) << "',";

            ss << "`property_seed` = '" << GetUInt32Value(ITEM_FIELD_PROPERTY_SEED) << "',";
            ss << "`random_prop_id` = '" << GetUInt32Value(ITEM_FIELD_RANDOM_PROPERTIES_ID) << "',";
            ss << "`text_id` = '" << GetUInt32Value(ITEM_FIELD_ITEM_TEXT_ID) << "',";
            ss << "`durability` = '" << GetUInt32Value(ITEM_FIELD_DURABILITY) << "',";
            ss << "`max_durability` = '" << GetUInt32Value(ITEM_FIELD_MAXDURABILITY) << "'";
            //`num_slots` ?

            ss << " WHERE `guid` = '" << guid << "'";

            trans->Append( ss.str().c_str() );

            if(HasFlag(ITEM_FIELD_FLAGS, ITEM_FLAGS_WRAPPED))
                trans->PAppend("UPDATE character_gifts SET guid = '%u' WHERE item_guid = '%u'", GUID_LOPART(GetOwnerGUID()),GetGUIDLow());
        } break;
        case ITEM_REMOVED:
        {
            if (GetUInt32Value(ITEM_FIELD_ITEM_TEXT_ID) > 0 )
                trans->PAppend("DELETE FROM item_text WHERE id = '%u'", GetUInt32Value(ITEM_FIELD_ITEM_TEXT_ID));
            trans->PAppend("DELETE FROM item_instance WHERE guid = '%u'", guid);
            if(HasFlag(ITEM_FIELD_FLAGS, ITEM_FLAGS_WRAPPED))
                trans->PAppend("DELETE FROM character_gifts WHERE item_guid = '%u'", GetGUIDLow());
            delete this;
            return;
        }
        case ITEM_UNCHANGED:
            break;
    }
    SetState(ITEM_UNCHANGED);
}

bool Item::LoadFromDB(uint32 guid, uint64 owner_guid, QueryResult *result)
{
    // create item before any checks for store correct guid
    // and allow use "FSetState(ITEM_REMOVED); SaveToDB();" for deleting item from DB
    Object::_Create(guid, 0, HIGHGUID_ITEM);

    bool delete_result = false;
    if(!result)
    {
        result = CharacterDatabase.PQuery("SELECT data FROM item_instance WHERE guid = '%u'", guid);
        delete_result = true;
    }

    if (!result)
    {
        sLog.outError("ERROR: Item (GUID: %u owner: %u) not found in table `item_instance`, can't load. ",guid,GUID_LOPART(owner_guid));
        return false;
    }

    Field *fields = result->Fetch();

    if(!LoadValues(fields[0].GetString()))
    {
        sLog.outError("ERROR: Item #%d have broken data in `data` field. Can't be loaded.",guid);
        if (delete_result) delete result;
        return false;
    }

    bool need_save = false;                                 // need explicit save data at load fixes

    // overwrite possible wrong/corrupted guid
    uint64 new_item_guid = MAKE_NEW_GUID(guid,0, HIGHGUID_ITEM);
    if(GetUInt64Value(OBJECT_FIELD_GUID) != new_item_guid)
    {
        SetUInt64Value(OBJECT_FIELD_GUID, MAKE_NEW_GUID(guid,0, HIGHGUID_ITEM));
        need_save = true;
    }

    if (delete_result) delete result;

    ItemPrototype const* proto = GetProto();
    if(!proto)
        return false;

    // recalculate suffix factor
    if(GetItemRandomPropertyId() < 0)
    {
        if(UpdateItemSuffixFactor())
            need_save = true;
    }

    // Remove bind flag for items vs NO_BIND set
    if (IsSoulBound() && proto->Bonding == NO_BIND)
    {
        ApplyModFlag(ITEM_FIELD_FLAGS,ITEM_FLAGS_BINDED, false);
        need_save = true;
    }

    // update duration if need, and remove if not need
    if((proto->Duration==0) != (GetUInt32Value(ITEM_FIELD_DURATION)==0))
    {
        SetUInt32Value(ITEM_FIELD_DURATION,abs(proto->Duration));
        need_save = true;
    }

    // set correct owner
    if(owner_guid != 0 && GetOwnerGUID() != owner_guid)
    {
        SetOwnerGUID(owner_guid);
        need_save = true;
    }

    if(need_save)                                           // normal item changed state set not work at loading
    {
        std::ostringstream ss;
        ss << "UPDATE item_instance SET data = '";
        for(uint16 i = 0; i < m_valuesCount; i++ )
            ss << GetUInt32Value(i) << " ";
        ss << "', owner_guid = '" << GUID_LOPART(GetOwnerGUID()) << "' WHERE guid = '" << guid << "'";

        CharacterDatabase.Execute( ss.str().c_str() );
    }

    return true;
}

void Item::DeleteFromDB()
{
    CharacterDatabase.PExecute("DELETE FROM item_instance WHERE guid = '%u'",GetGUIDLow());
}

void Item::DeleteFromInventoryDB(SQLTransaction trans)
{
    trans->PAppend("DELETE FROM character_inventory WHERE item = '%u'",GetGUIDLow());
}

ItemPrototype const *Item::GetProto() const
{
    return objmgr.GetItemPrototype(GetEntry());
}

Player* Item::GetOwner()const
{
    return objmgr.GetPlayer(GetOwnerGUID());
}

uint32 Item::GetSkill()
{
    const static uint32 item_weapon_skills[MAX_ITEM_SUBCLASS_WEAPON] =
    {
        SKILL_AXES,     SKILL_2H_AXES,  SKILL_BOWS,          SKILL_GUNS,      SKILL_MACES,
        SKILL_2H_MACES, SKILL_POLEARMS, SKILL_SWORDS,        SKILL_2H_SWORDS, 0,
        SKILL_STAVES,   0,              0,                   SKILL_UNARMED,   0,
        SKILL_DAGGERS,  SKILL_THROWN,   SKILL_ASSASSINATION, SKILL_CROSSBOWS, SKILL_WANDS,
        SKILL_FISHING
    };

    const static uint32 item_armor_skills[MAX_ITEM_SUBCLASS_ARMOR] =
    {
        0,SKILL_CLOTH,SKILL_LEATHER,SKILL_MAIL,SKILL_PLATE_MAIL,0,SKILL_SHIELD,0,0,0
    };

    ItemPrototype const* proto = GetProto();

    switch (proto->Class)
    {
        case ITEM_CLASS_WEAPON:
            if( proto->SubClass >= MAX_ITEM_SUBCLASS_WEAPON )
                return 0;
            else
                return item_weapon_skills[proto->SubClass];

        case ITEM_CLASS_ARMOR:
            if( proto->SubClass >= MAX_ITEM_SUBCLASS_ARMOR )
                return 0;
            else
                return item_armor_skills[proto->SubClass];

        default:
            return 0;
    }
}

uint32 Item::GetSpell()
{
    ItemPrototype const* proto = GetProto();

    switch (proto->Class)
    {
        case ITEM_CLASS_WEAPON:
            switch (proto->SubClass)
            {
                case ITEM_SUBCLASS_WEAPON_AXE:     return  196;
                case ITEM_SUBCLASS_WEAPON_AXE2:    return  197;
                case ITEM_SUBCLASS_WEAPON_BOW:     return  264;
                case ITEM_SUBCLASS_WEAPON_GUN:     return  266;
                case ITEM_SUBCLASS_WEAPON_MACE:    return  198;
                case ITEM_SUBCLASS_WEAPON_MACE2:   return  199;
                case ITEM_SUBCLASS_WEAPON_POLEARM: return  200;
                case ITEM_SUBCLASS_WEAPON_SWORD:   return  201;
                case ITEM_SUBCLASS_WEAPON_SWORD2:  return  202;
                case ITEM_SUBCLASS_WEAPON_STAFF:   return  227;
                case ITEM_SUBCLASS_WEAPON_DAGGER:  return 1180;
                case ITEM_SUBCLASS_WEAPON_THROWN:  return 2567;
                case ITEM_SUBCLASS_WEAPON_SPEAR:   return 3386;
                case ITEM_SUBCLASS_WEAPON_CROSSBOW:return 5011;
                case ITEM_SUBCLASS_WEAPON_WAND:    return 5009;
                default: return 0;
            }
        case ITEM_CLASS_ARMOR:
            switch(proto->SubClass)
            {
                case ITEM_SUBCLASS_ARMOR_CLOTH:    return 9078;
                case ITEM_SUBCLASS_ARMOR_LEATHER:  return 9077;
                case ITEM_SUBCLASS_ARMOR_MAIL:     return 8737;
                case ITEM_SUBCLASS_ARMOR_PLATE:    return  750;
                case ITEM_SUBCLASS_ARMOR_SHIELD:   return 9116;
                default: return 0;
            }
    }
    return 0;
}

int32 Item::GenerateItemRandomPropertyId(uint32 item_id)
{
    ItemPrototype const *itemProto = sItemStorage.LookupEntry<ItemPrototype>(item_id);

    if(!itemProto)
        return 0;

    // item must have one from this field values not null if it can have random enchantments
    if((!itemProto->RandomProperty) && (!itemProto->RandomSuffix))
        return 0;

    // item can have not null only one from field values
    if((itemProto->RandomProperty) && (itemProto->RandomSuffix))
    {
        sLog.outErrorDb("Item template %u have RandomProperty==%u and RandomSuffix==%u, but must have one from field =0",itemProto->ItemId,itemProto->RandomProperty,itemProto->RandomSuffix);
        return 0;
    }

    // RandomProperty case
    if(itemProto->RandomProperty)
    {
        uint32 randomPropId = GetItemEnchantMod(itemProto->RandomProperty);
        ItemRandomPropertiesEntry const *random_id = sItemRandomPropertiesStore.LookupEntry(randomPropId);
        if(!random_id)
        {
            sLog.outErrorDb("Enchantment id #%u used but it doesn't have records in 'ItemRandomProperties.dbc'",randomPropId);
            return 0;
        }

        return random_id->ID;
    }
    // RandomSuffix case
    else
    {
        uint32 randomPropId = GetItemEnchantMod(itemProto->RandomSuffix);
        ItemRandomSuffixEntry const *random_id = sItemRandomSuffixStore.LookupEntry(randomPropId);
        if(!random_id)
        {
            sLog.outErrorDb("Enchantment id #%u used but it doesn't have records in sItemRandomSuffixStore.",randomPropId);
            return 0;
        }

        return -int32(random_id->ID);
    }
}

void Item::SetItemRandomProperties(int32 randomPropId)
{
    if(!randomPropId)
        return;

    if(randomPropId > 0)
    {
        ItemRandomPropertiesEntry const *item_rand = sItemRandomPropertiesStore.LookupEntry(randomPropId);
        if(item_rand)
        {
            if(GetInt32Value(ITEM_FIELD_RANDOM_PROPERTIES_ID) != int32(item_rand->ID))
            {
                SetInt32Value(ITEM_FIELD_RANDOM_PROPERTIES_ID,item_rand->ID);
                SetState(ITEM_CHANGED);
            }
            for(uint32 i = PROP_ENCHANTMENT_SLOT_2; i < PROP_ENCHANTMENT_SLOT_2 + 3; ++i)
                SetEnchantment(EnchantmentSlot(i),item_rand->enchant_id[i - PROP_ENCHANTMENT_SLOT_2],0,0);
        }
    }
    else
    {
        ItemRandomSuffixEntry const *item_rand = sItemRandomSuffixStore.LookupEntry(-randomPropId);
        if(item_rand)
        {
            if( GetInt32Value(ITEM_FIELD_RANDOM_PROPERTIES_ID) != -int32(item_rand->ID) ||
                !GetItemSuffixFactor())
            {
                SetInt32Value(ITEM_FIELD_RANDOM_PROPERTIES_ID,-int32(item_rand->ID));
                UpdateItemSuffixFactor();
                SetState(ITEM_CHANGED);
            }

            for(uint32 i = PROP_ENCHANTMENT_SLOT_0; i < PROP_ENCHANTMENT_SLOT_0 + 3; ++i)
                SetEnchantment(EnchantmentSlot(i),item_rand->enchant_id[i - PROP_ENCHANTMENT_SLOT_0],0,0);
        }
    }
}

bool Item::UpdateItemSuffixFactor()
{
    uint32 suffixFactor = GenerateEnchSuffixFactor(GetEntry());
    if(GetItemSuffixFactor()==suffixFactor)
        return false;
    SetUInt32Value(ITEM_FIELD_PROPERTY_SEED,suffixFactor);
    return true;
}

void Item::SetState(ItemUpdateState state, Player *forplayer)
{
    if (uState == ITEM_NEW && state == ITEM_REMOVED)
    {
        // pretend the item never existed
        RemoveFromUpdateQueueOf(forplayer);
        delete this;
        return;
    }

    if (state != ITEM_UNCHANGED)
    {
        // new items must stay in new state until saved
        if (uState != ITEM_NEW) uState = state;
        AddToUpdateQueueOf(forplayer);
    }
    else
    {
        // unset in queue
        // the item must be removed from the queue manually
        uQueuePos = -1;
        uState = ITEM_UNCHANGED;
    }
}

void Item::AddToUpdateQueueOf(Player *player)
{
    if (IsInUpdateQueue()) return;

    if (!player)
    {
        player = GetOwner();
        if (!player)
        {
            sLog.outError("Item::AddToUpdateQueueOf - GetPlayer didn't find a player matching owner's guid (%u)!", GUID_LOPART(GetOwnerGUID()));
            return;
        }
    }

    if (player->GetGUID() != GetOwnerGUID())
    {
        sLog.outError("Item::AddToUpdateQueueOf - Owner's guid (%u) and player's guid (%u) don't match!", GUID_LOPART(GetOwnerGUID()), player->GetGUIDLow());
        return;
    }

    if (player->m_itemUpdateQueueBlocked) return;

    player->m_itemUpdateQueue.push_back(this);
    uQueuePos = player->m_itemUpdateQueue.size()-1;
}

void Item::RemoveFromUpdateQueueOf(Player *player)
{
    if (!IsInUpdateQueue()) return;

    if (!player)
    {
        player = GetOwner();
        if (!player)
        {
            sLog.outError("Item::RemoveFromUpdateQueueOf - GetPlayer didn't find a player matching owner's guid (%u)!", GUID_LOPART(GetOwnerGUID()));
            return;
        }
    }

    if (player->GetGUID() != GetOwnerGUID())
    {
        sLog.outError("Item::RemoveFromUpdateQueueOf - Owner's guid (%u) and player's guid (%u) don't match!", GUID_LOPART(GetOwnerGUID()), player->GetGUIDLow());
        return;
    }

    if (player->m_itemUpdateQueueBlocked) return;

    player->m_itemUpdateQueue[uQueuePos] = NULL;
    uQueuePos = -1;
}

uint8 Item::GetBagSlot() const
{
    return m_container ? m_container->GetSlot() : uint8(INVENTORY_SLOT_BAG_0);
}

bool Item::IsEquipped() const
{
    return !IsInBag() && m_slot < EQUIPMENT_SLOT_END;
}

bool Item::CanBeTraded() const
{
    if(IsSoulBound())
        return false;
    if(IsBag() && (Player::IsBagPos(GetPos()) || !((Bag const*)this)->IsEmpty()) )
        return false;

    if(Player* owner = GetOwner())
    {
        if(owner->CanUnequipItem(GetPos(),false) !=  EQUIP_ERR_OK )
            return false;
        if(owner->GetLootGUID()==GetGUID())
            return false;
    }

    if (IsBoundByEnchant())
        return false;

    return true;
}

bool Item::IsBoundByEnchant() const
{
    // Check all enchants for soulbound
    for(uint32 enchant_slot = PERM_ENCHANTMENT_SLOT; enchant_slot < MAX_ENCHANTMENT_SLOT; ++enchant_slot)
    {
        uint32 enchant_id = GetEnchantmentId(EnchantmentSlot(enchant_slot));
        if(!enchant_id)
            continue;

        SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
        if(!enchantEntry)
            continue;

        if(enchantEntry->slot & ENCHANTMENT_CAN_SOULBOUND)
            return true;
    }
    return false;
}

bool Item::IsFitToSpellRequirements(SpellEntry const* spellInfo) const
{
    ItemPrototype const* proto = GetProto();

    if (spellInfo->EquippedItemClass != -1)                 // -1 == any item class
    {
        if(spellInfo->EquippedItemClass != int32(proto->Class))
            return false;                                   //  wrong item class

        if(spellInfo->EquippedItemSubClassMask != 0)        // 0 == any subclass
        {
            if((spellInfo->EquippedItemSubClassMask & (1 << proto->SubClass)) == 0)
                return false;                               // subclass not present in mask
        }
    }

    if(spellInfo->EquippedItemInventoryTypeMask != 0)       // 0 == any inventory type
    {
        if((spellInfo->EquippedItemInventoryTypeMask  & (1 << proto->InventoryType)) == 0)
            return false;                                   // inventory type not present in mask
    }

    return true;
}

void Item::SetEnchantment(EnchantmentSlot slot, uint32 id, uint32 duration, uint32 charges)
{
    // Better lost small time at check in comparison lost time at item save to DB.
    if((GetEnchantmentId(slot) == id) && (GetEnchantmentDuration(slot) == duration) && (GetEnchantmentCharges(slot) == charges))
        return;

    SetUInt32Value(ITEM_FIELD_ENCHANTMENT + slot*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_ID_OFFSET,id);
    SetUInt32Value(ITEM_FIELD_ENCHANTMENT + slot*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_DURATION_OFFSET,duration);
    SetUInt32Value(ITEM_FIELD_ENCHANTMENT + slot*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_CHARGES_OFFSET,charges);
    SetState(ITEM_CHANGED);
}

void Item::SetEnchantmentDuration(EnchantmentSlot slot, uint32 duration)
{
    if(GetEnchantmentDuration(slot) == duration)
        return;

    SetUInt32Value(ITEM_FIELD_ENCHANTMENT + slot*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_DURATION_OFFSET,duration);
    SetState(ITEM_CHANGED);
}

void Item::SetEnchantmentCharges(EnchantmentSlot slot, uint32 charges)
{
    if(GetEnchantmentCharges(slot) == charges)
        return;

    SetUInt32Value(ITEM_FIELD_ENCHANTMENT + slot*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_CHARGES_OFFSET,charges);
    SetState(ITEM_CHANGED);
}

void Item::ClearEnchantment(EnchantmentSlot slot)
{
    if(!GetEnchantmentId(slot))
        return;

    for(uint8 x = 0; x < 3; ++x)
        SetUInt32Value(ITEM_FIELD_ENCHANTMENT + slot*MAX_ENCHANTMENT_OFFSET + x, 0);
    SetState(ITEM_CHANGED);
}

bool Item::GemsFitSockets() const
{
    bool fits = true;
    for(uint32 enchant_slot = SOCK_ENCHANTMENT_SLOT; enchant_slot < SOCK_ENCHANTMENT_SLOT+3; ++enchant_slot)
    {
        uint8 SocketColor = GetProto()->Socket[enchant_slot-SOCK_ENCHANTMENT_SLOT].Color;

        uint32 enchant_id = GetEnchantmentId(EnchantmentSlot(enchant_slot));
        if(!enchant_id)
        {
            if(SocketColor) fits &= false;
            continue;
        }

        SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
        if(!enchantEntry)
        {
            if(SocketColor) fits &= false;
            continue;
        }

        uint8 GemColor = 0;

        uint32 gemid = enchantEntry->GemID;
        if(gemid)
        {
            ItemPrototype const* gemProto = sItemStorage.LookupEntry<ItemPrototype>(gemid);
            if(gemProto)
            {
                GemPropertiesEntry const* gemProperty = sGemPropertiesStore.LookupEntry(gemProto->GemProperties);
                if(gemProperty)
                    GemColor = gemProperty->color;
            }
        }

        fits &= (GemColor & SocketColor) ? true : false;
    }
    return fits;
}

uint8 Item::GetGemCountWithID(uint32 GemID) const
{
    uint8 count = 0;
    for(uint32 enchant_slot = SOCK_ENCHANTMENT_SLOT; enchant_slot < SOCK_ENCHANTMENT_SLOT+3; ++enchant_slot)
    {
        uint32 enchant_id = GetEnchantmentId(EnchantmentSlot(enchant_slot));
        if(!enchant_id)
            continue;

        SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
        if(!enchantEntry)
            continue;

        if(GemID == enchantEntry->GemID)
            ++count;
    }
    return count;
}

bool Item::IsLimitedToAnotherMapOrZone( uint32 cur_mapId, uint32 cur_zoneId) const
{
    ItemPrototype const* proto = GetProto();
    return proto && (proto->Map && proto->Map != cur_mapId || proto->Area && proto->Area != cur_zoneId );
}

// Though the client has the information in the item's data field,
// we have to send SMSG_ITEM_TIME_UPDATE to display the remaining
// time.
void Item::SendTimeUpdate(Player* owner)
{
    if (!GetUInt32Value(ITEM_FIELD_DURATION))
        return;

    WorldPacket data(SMSG_ITEM_TIME_UPDATE, (8+4));
    data << (uint64)GetGUID();
    data << (uint32)GetUInt32Value(ITEM_FIELD_DURATION);
    owner->GetSession()->SendPacket(&data);
}

Item* Item::CreateItem( uint32 item, uint32 count, Player const* player )
{
    if ( count < 1 )
        return NULL;                                        //don't create item at zero count

    ItemPrototype const *pProto = objmgr.GetItemPrototype( item );
    if( pProto )
    {
        if ( count > pProto->Stackable )
            count = pProto->Stackable;

        assert(count !=0 && "pProto->Stackable==0 but checked at loading already");

        Item *pItem = NewItemOrBag( pProto );
        if( pItem->Create(objmgr.GenerateLowGuid(HIGHGUID_ITEM), item, player) )
        {
            pItem->SetCount( count );
            return pItem;
        }
        else
            delete pItem;
    }
    return NULL;
}

Item* Item::CloneItem( uint32 count, Player const* player ) const
{
    Item* newItem = CreateItem( GetEntry(), count, player );
    if(!newItem)
        return NULL;

    newItem->SetUInt32Value( ITEM_FIELD_CREATOR,      GetUInt32Value( ITEM_FIELD_CREATOR ) );
    newItem->SetUInt32Value( ITEM_FIELD_GIFTCREATOR,  GetUInt32Value( ITEM_FIELD_GIFTCREATOR ) );
    newItem->SetUInt32Value( ITEM_FIELD_FLAGS,        GetUInt32Value( ITEM_FIELD_FLAGS ) );
    newItem->SetUInt32Value( ITEM_FIELD_DURATION,     GetUInt32Value( ITEM_FIELD_DURATION ) );
    newItem->SetItemRandomProperties(GetItemRandomPropertyId());
    return newItem;
}

