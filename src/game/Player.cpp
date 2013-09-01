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
#include "Language.h"
#include "Database/DatabaseEnv.h"
#include "Log.h"
#include "Opcodes.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "UpdateMask.h"
#include "Player.h"
#include "SkillDiscovery.h"
#include "QuestDef.h"
#include "GossipDef.h"
#include "UpdateData.h"
#include "Channel.h"
#include "ChannelMgr.h"
#include "MapManager.h"
#include "MapInstanced.h"
#include "InstanceSaveMgr.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "ObjectMgr.h"
#include "ObjectAccessor.h"
#include "CreatureAI.h"
#include "Formulas.h"
#include "Group.h"
#include "Guild.h"
#include "Pet.h"
#include "SpellAuras.h"
#include "Util.h"
#include "Transports.h"
#include "Weather.h"
#include "BattleGround.h"
#include "BattleGroundAV.h"
#include "BattleGroundMgr.h"
#include "OutdoorPvP.h"
#include "OutdoorPvPMgr.h"
#include "ArenaTeam.h"
#include "Chat.h"
#include "Database/AsyncDatabaseImpl.h"
#include "Spell.h"
#include "SocialMgr.h"
#include "GameEvent.h"
#include "Config/ConfigEnv.h"
#include "ScriptedInstance.h"
#include "ConditionMgr.h"
#include "SpectatorAddon.h"
#include "IRCMgr.h"

#include <cmath>
#include <setjmp.h>

#define ZONE_UPDATE_INTERVAL 1000

#define PLAYER_SKILL_INDEX(x)       (PLAYER_SKILL_INFO_1_1 + ((x)*3))
#define PLAYER_SKILL_VALUE_INDEX(x) (PLAYER_SKILL_INDEX(x)+1)
#define PLAYER_SKILL_BONUS_INDEX(x) (PLAYER_SKILL_INDEX(x)+2)

#define SKILL_VALUE(x)         PAIR32_LOPART(x)
#define SKILL_MAX(x)           PAIR32_HIPART(x)
#define MAKE_SKILL_VALUE(v, m) MAKE_PAIR32(v,m)

#define SKILL_TEMP_BONUS(x)    int16(PAIR32_LOPART(x))
#define SKILL_PERM_BONUS(x)    int16(PAIR32_HIPART(x))
#define MAKE_SKILL_BONUS(t, p) MAKE_PAIR32(t,p)

#ifdef UNIX
jmp_buf __jmp_env;
void __segv_handler(int)
{
    siglongjmp(__jmp_env, 1);
}
#endif

enum CharacterFlags
{
    CHARACTER_FLAG_NONE                 = 0x00000000,
    CHARACTER_FLAG_UNK1                 = 0x00000001,
    CHARACTER_FLAG_UNK2                 = 0x00000002,
    CHARACTER_LOCKED_FOR_TRANSFER       = 0x00000004,
    CHARACTER_FLAG_UNK4                 = 0x00000008,
    CHARACTER_FLAG_UNK5                 = 0x00000010,
    CHARACTER_FLAG_UNK6                 = 0x00000020,
    CHARACTER_FLAG_UNK7                 = 0x00000040,
    CHARACTER_FLAG_UNK8                 = 0x00000080,
    CHARACTER_FLAG_UNK9                 = 0x00000100,
    CHARACTER_FLAG_UNK10                = 0x00000200,
    CHARACTER_FLAG_HIDE_HELM            = 0x00000400,
    CHARACTER_FLAG_HIDE_CLOAK           = 0x00000800,
    CHARACTER_FLAG_UNK13                = 0x00001000,
    CHARACTER_FLAG_GHOST                = 0x00002000,
    CHARACTER_FLAG_RENAME               = 0x00004000,
    CHARACTER_FLAG_UNK16                = 0x00008000,
    CHARACTER_FLAG_UNK17                = 0x00010000,
    CHARACTER_FLAG_UNK18                = 0x00020000,
    CHARACTER_FLAG_UNK19                = 0x00040000,
    CHARACTER_FLAG_UNK20                = 0x00080000,
    CHARACTER_FLAG_UNK21                = 0x00100000,
    CHARACTER_FLAG_UNK22                = 0x00200000,
    CHARACTER_FLAG_UNK23                = 0x00400000,
    CHARACTER_FLAG_UNK24                = 0x00800000,
    CHARACTER_FLAG_LOCKED_BY_BILLING    = 0x01000000,
    CHARACTER_FLAG_DECLINED             = 0x02000000,
    CHARACTER_FLAG_UNK27                = 0x04000000,
    CHARACTER_FLAG_UNK28                = 0x08000000,
    CHARACTER_FLAG_UNK29                = 0x10000000,
    CHARACTER_FLAG_UNK30                = 0x20000000,
    CHARACTER_FLAG_UNK31                = 0x40000000,
    CHARACTER_FLAG_UNK32                = 0x80000000
};

// corpse reclaim times
#define DEATH_EXPIRE_STEP (5*MINUTE)
#define MAX_DEATH_COUNT 3

static uint32 copseReclaimDelay[MAX_DEATH_COUNT] = { 30, 60, 120 };

//== PlayerTaxi ================================================

PlayerTaxi::PlayerTaxi()
{
    // Taxi nodes
    memset(m_taximask, 0, sizeof(m_taximask));
}

void PlayerTaxi::InitTaxiNodesForLevel(uint32 race, uint32 level)
{
    // capital and taxi hub masks
    switch(race)
    {
        case RACE_HUMAN:    SetTaximaskNode(2);  break;     // Human
        case RACE_ORC:      SetTaximaskNode(23); break;     // Orc
        case RACE_DWARF:    SetTaximaskNode(6);  break;     // Dwarf
        case RACE_NIGHTELF: SetTaximaskNode(26);
                            SetTaximaskNode(27); break;     // Night Elf
        case RACE_UNDEAD_PLAYER: SetTaximaskNode(11); break;// Undead
        case RACE_TAUREN:   SetTaximaskNode(22); break;     // Tauren
        case RACE_GNOME:    SetTaximaskNode(6);  break;     // Gnome
        case RACE_TROLL:    SetTaximaskNode(23); break;     // Troll
        case RACE_BLOODELF: SetTaximaskNode(82); break;     // Blood Elf
        case RACE_DRAENEI:  SetTaximaskNode(94); break;     // Draenei
    }
    // new continent starting masks (It will be accessible only at new map)
    switch(Player::TeamForRace(race))
    {
        case ALLIANCE: SetTaximaskNode(100); break;
        case HORDE:    SetTaximaskNode(99);  break;
    }
    // level dependent taxi hubs
    if(level>=68)
        SetTaximaskNode(213);                               //Shattered Sun Staging Area
}

void PlayerTaxi::LoadTaxiMask(const char* data)
{
    Tokens tokens = StrSplit(data, " ");

    int index;
    Tokens::iterator iter;
    for (iter = tokens.begin(), index = 0;
        (index < TaxiMaskSize) && (iter != tokens.end()); ++iter, ++index)
    {
        // load and set bits only for existed taxi nodes
        m_taximask[index] = sTaxiNodesMask[index] & uint32(atol((*iter).c_str()));
    }
}

void PlayerTaxi::AppendTaximaskTo( ByteBuffer& data, bool all )
{
    if(all)
    {
        for (uint8 i=0; i<TaxiMaskSize; i++)
            data << uint32(sTaxiNodesMask[i]);              // all existed nodes
    }
    else
    {
        for (uint8 i=0; i<TaxiMaskSize; i++)
            data << uint32(m_taximask[i]);                  // known nodes
    }
}

bool PlayerTaxi::LoadTaxiDestinationsFromString( const std::string& values )
{
    ClearTaxiDestinations();

    Tokens tokens = StrSplit(values," ");

    for(Tokens::iterator iter = tokens.begin(); iter != tokens.end(); ++iter)
    {
        uint32 node = uint32(atol(iter->c_str()));
        AddTaxiDestination(node);
    }

    if(m_TaxiDestinations.empty())
        return true;

    // Check integrity
    if(m_TaxiDestinations.size() < 2)
        return false;

    for(size_t i = 1; i < m_TaxiDestinations.size(); ++i)
    {
        uint32 cost;
        uint32 path;
        objmgr.GetTaxiPath(m_TaxiDestinations[i-1],m_TaxiDestinations[i],path,cost);
        if(!path)
            return false;
    }

    return true;
}

std::string PlayerTaxi::SaveTaxiDestinationsToString()
{
    if(m_TaxiDestinations.empty())
        return "";

    std::ostringstream ss;

    for(size_t i=0; i < m_TaxiDestinations.size(); ++i)
        ss << m_TaxiDestinations[i] << " ";

    return ss.str();
}

uint32 PlayerTaxi::GetCurrentTaxiPath() const
{
    if(m_TaxiDestinations.size() < 2)
        return 0;

    uint32 path;
    uint32 cost;

    objmgr.GetTaxiPath(m_TaxiDestinations[0],m_TaxiDestinations[1],path,cost);

    return path;
}

//== Player ====================================================

const int32 Player::ReputationRank_Length[MAX_REPUTATION_RANK] = {36000, 3000, 3000, 3000, 6000, 12000, 21000, 1000};

UpdateMask Player::updateVisualBits;

Player::Player (WorldSession *session): Unit()
{
    m_transport = 0;

    m_speakTime = 0;
    m_speakCount = 0;

    m_objectType |= TYPEMASK_PLAYER;
    m_objectTypeId = TYPEID_PLAYER;

    m_valuesCount = PLAYER_END;

    m_session = session;

    m_divider = 0;

    m_ExtraFlags = 0;

    // players always accept
    if(GetSession()->GetSecurity() == SEC_PLAYER)
        SetAcceptWhispers(true);

    m_curSelection = 0;
    m_lootGuid = 0;

    m_comboTarget = 0;
    m_comboPoints = 0;

    m_usedTalentCount = 0;

    m_regenTimer = 0;
    m_weaponChangeTimer = 0;

    m_zoneUpdateId = 0;
    m_zoneUpdateTimer = 0;

    m_areaUpdateId = 0;

    m_nextSave = sWorld.getConfig(CONFIG_INTERVAL_SAVE);

    // randomize first save time in range [CONFIG_INTERVAL_SAVE] around [CONFIG_INTERVAL_SAVE]
    // this must help in case next save after mass player load after server startup
    m_nextSave = GetMap()->urand(m_nextSave/2,m_nextSave*3/2);

    clearResurrectRequestData();

    m_SpellModRemoveCount = 0;

    memset(m_items, 0, sizeof(Item*)*PLAYER_SLOTS_COUNT);

    m_social = NULL;

    // group is initialized in the reference constructor
    SetGroupInvite(NULL);
    m_groupUpdateMask = 0;
    m_auraUpdateMask = 0;

    duel = NULL;

    m_GuildIdInvited = 0;
    m_ArenaTeamIdInvited = 0;

    m_atLoginFlags = AT_LOGIN_NONE;

    m_dontMove = false;

    pTrader = 0;
    ClearTrade();

    m_cinematic = 0;

    PlayerTalkClass = new PlayerMenu( GetSession() );
    m_currentBuybackSlot = BUYBACK_SLOT_START;

    for ( int aX = 0 ; aX < 8 ; aX++ )
        m_Tutorials[ aX ] = 0x00;
    m_TutorialsChanged = false;

    m_DailyQuestChanged = false;
    m_lastDailyQuestTime = 0;

    for (uint8 i=0; i<MAX_TIMERS; i++)
        m_MirrorTimer[i] = DISABLED_MIRROR_TIMER;

    m_MirrorTimerFlags = UNDERWATER_NONE;
    m_MirrorTimerFlagsLast = UNDERWATER_NONE;
    m_isInWater = false;
    m_drunkTimer = 0;
    m_drunk = 0;
    m_restTime = 0;
    m_deathTimer = 0;
    m_deathExpireTime = 0;
	m_deathTime = 0;

    m_swingErrorMsg = 0;

    m_DetectInvTimer = 1000;

    m_bgBattleGroundID = 0;
    for (int j=0; j < PLAYER_MAX_BATTLEGROUND_QUEUES; j++)
    {
        m_bgBattleGroundQueueID[j].bgQueueType  = 0;
        m_bgBattleGroundQueueID[j].invitedToInstance = 0;
    }
    m_bgTeam = 0;

    m_logintime = time(NULL);
    m_Last_tick = m_logintime;
    m_WeaponProficiency = 0;
    m_ArmorProficiency = 0;
    m_canParry = false;
    m_canBlock = false;
    m_canDualWield = false;
    m_ammoDPS = 0.0f;

    m_temporaryUnsummonedPetNumber = 0;
    //cache for UNIT_CREATED_BY_SPELL to allow
    //returning reagents for temporarily removed pets
    //when dying/logging out
    m_oldpetspell = 0;

    ////////////////////Rest System/////////////////////
    time_inn_enter=0;
    inn_pos_mapid=0;
    inn_pos_x=0;
    inn_pos_y=0;
    inn_pos_z=0;
    m_rest_bonus=0;
    rest_type=REST_TYPE_NO;
    ////////////////////Rest System/////////////////////

    //movement anticheat
    m_anti_lastmovetime = 0;   //last movement time
    m_anti_transportGUID = 0;  //current transport GUID
    m_anti_NextLenCheck = 0;
    m_anti_MovedLen = 0.0f;
    m_anti_beginfalltime = 0;  //alternative falling begin time
    m_anti_lastalarmtime = 0;    //last time when alarm generated
    m_anti_alarmcount = 0;       //alarm counter
    m_anti_TeleTime = 0;
    m_CanFly=false;
    /////////////////////////////////

    m_mailsLoaded = false;
    m_mailsUpdated = false;
    unReadMails = 0;
    m_nextMailDelivereTime = 0;

    m_resetTalentsCost = 0;
    m_resetTalentsTime = 0;
    m_itemUpdateQueueBlocked = false;

    for (int i = 0; i < MAX_MOVE_TYPE; ++i)
        m_forced_speed_changes[i] = 0;

    m_stableSlots = 0;

    /////////////////// Instance System /////////////////////

    m_HomebindTimer = 0;
    m_InstanceValid = true;
    m_dungeonDifficulty = DIFFICULTY_NORMAL;

    for (int i = 0; i < BASEMOD_END; i++)
    {
        m_auraBaseMod[i][FLAT_MOD] = 0.0f;
        m_auraBaseMod[i][PCT_MOD] = 1.0f;
    }

    // Honor System
    m_lastHonorUpdateTime = time(NULL);

    // Player summoning
    m_summon_expire = 0;
    m_summon_mapid = 0;
    m_summon_x = 0.0f;
    m_summon_y = 0.0f;
    m_summon_z = 0.0f;
    m_invite_summon = false;

    //Default movement to run mode
    m_unit_movement_flags = 0;

    m_miniPet = 0;
    m_bgAfkReportedTimer = 0;
    m_contestedPvPTimer = 0;

    m_KnockedBack = false;

    m_declinedname = NULL;

    m_isActive = true;

    m_farsightVision = false;
    
    // Experience Blocking
    m_isXpBlocked = false;
    
    // Spirit of Redemption
    m_spiritRedemptionKillerGUID = 0;

    m_globalCooldowns.clear();
    m_kickatnextupdate = false;
    m_swdBackfireDmg = 0;
    
    m_ConditionErrorMsgId = 0;
    
    m_lastOpenLockKey = 0;
    
    _attackersCheckTime = 0;
    
    m_bPassOnGroupLoot = false;

    spectatorFlag = false;
    spectateCanceled = false;
    spectateFrom = NULL;
    
    _lastSpamAlert = 0;
    lastLagReport = 0;
}

Player::~Player ()
{
    CleanupsBeforeDelete();

    // it must be unloaded already in PlayerLogout and accessed only for loggined player
    //m_social = NULL;

    // Note: buy back item already deleted from DB when player was saved
    for(int i = 0; i < PLAYER_SLOTS_COUNT; ++i)
    {
        if(m_items[i])
            delete m_items[i];
    }
    CleanupChannels();

    for (PlayerSpellMap::const_iterator itr = m_spells.begin(); itr != m_spells.end(); ++itr)
        delete itr->second;

    //all mailed items should be deleted, also all mail should be deallocated
    for (PlayerMails::iterator itr =  m_mail.begin(); itr != m_mail.end();++itr)
        delete *itr;

    for (ItemMap::iterator iter = mMitems.begin(); iter != mMitems.end(); ++iter)
        delete iter->second;                                //if item is duplicated... then server may crash ... but that item should be deallocated

    delete PlayerTalkClass;

    if (m_transport)
    {
        m_transport->RemovePassenger(this);
    }

    for(size_t x = 0; x < ItemSetEff.size(); x++)
        if(ItemSetEff[x])
            delete ItemSetEff[x];

    // clean up player-instance binds, may unload some instance saves
    for(uint8 i = 0; i < TOTAL_DIFFICULTIES; i++)
        for(BoundInstancesMap::iterator itr = m_boundInstances[i].begin(); itr != m_boundInstances[i].end(); ++itr)
            itr->second.save->RemovePlayer(this);

    delete m_declinedname;
}

void Player::CleanupsBeforeDelete()
{
    if(m_uint32Values)                                      // only for fully created Object
    {
        TradeCancel(false);
        DuelComplete(DUEL_INTERUPTED);
    }
    Unit::CleanupsBeforeDelete();
}

bool Player::Create( uint32 guidlow, const std::string& name, uint8 race, uint8 class_, uint8 gender, uint8 skin, uint8 face, uint8 hairStyle, uint8 hairColor, uint8 facialHair, uint8 outfitId )
{
    //FIXME: outfitId not used in player creating

    Object::_Create(guidlow, 0, HIGHGUID_PLAYER);

    m_name = name;

    PlayerInfo const* info = objmgr.GetPlayerInfo(race, class_);
    if(!info)
    {
        sLog.outError("Player have incorrect race/class pair. Can't be loaded.");
        return false;
    }

    for (int i = 0; i < PLAYER_SLOTS_COUNT; i++)
        m_items[i] = NULL;

    m_race = race;
    m_class = class_;
    m_gender = gender;

    SetMapId(info->mapId);
    Relocate(info->positionX,info->positionY,info->positionZ);

    ChrClassesEntry const* cEntry = sChrClassesStore.LookupEntry(class_);
    if(!cEntry)
    {
        sLog.outError("Class %u not found in DBC (Wrong DBC files?)",class_);
        return false;
    }

    uint8 powertype = cEntry->powerType;

    uint32 unitfield;

    switch(powertype)
    {
        case POWER_ENERGY:
        case POWER_MANA:
            unitfield = 0x00000000;
            break;
        case POWER_RAGE:
            unitfield = 0x00110000;
            break;
        default:
            sLog.outError("Invalid default powertype %u for player (class %u)",powertype,class_);
            return false;
    }

    SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS, DEFAULT_WORLD_OBJECT_SIZE );
    SetFloatValue(UNIT_FIELD_COMBATREACH, DEFAULT_COMBAT_REACH );

    switch(gender)
    {
        case GENDER_FEMALE:
            SetDisplayId(info->displayId_f );
            SetNativeDisplayId(info->displayId_f );
            break;
        case GENDER_MALE:
            SetDisplayId(info->displayId_m );
            SetNativeDisplayId(info->displayId_m );
            break;
        default:
            sLog.outError("Invalid gender %u for player",gender);
            return false;
            break;
    }

    setFactionForRace(m_race);

    uint32 RaceClassGender = ( race ) | ( class_ << 8 ) | ( gender << 16 );

    SetUInt32Value(UNIT_FIELD_BYTES_0, ( RaceClassGender | ( powertype << 24 ) ) );
    SetUInt32Value(UNIT_FIELD_BYTES_1, unitfield);
    SetByteValue(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_SANCTUARY | UNIT_BYTE2_FLAG_UNK5 );
    SetUInt32Value(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE );
    SetFloatValue(UNIT_MOD_CAST_SPEED, 1.0f);               // fix cast time showed in spell tooltip on client

                                                            //-1 is default value
    SetInt32Value(PLAYER_FIELD_WATCHED_FACTION_INDEX, uint32(-1));

    SetUInt32Value(PLAYER_BYTES, (skin | (face << 8) | (hairStyle << 16) | (hairColor << 24)));
    SetUInt32Value(PLAYER_BYTES_2, (facialHair | (0x00 << 8) | (0x00 << 16) | (0x02 << 24)));
    SetByteValue(PLAYER_BYTES_3, 0, gender);

    SetUInt32Value( PLAYER_GUILDID, 0 );
    SetUInt32Value( PLAYER_GUILDRANK, 0 );
    SetUInt32Value( PLAYER_GUILD_TIMESTAMP, 0 );

    SetUInt64Value( PLAYER_FIELD_KNOWN_TITLES, 0 );        // 0=disabled
    SetUInt32Value( PLAYER_CHOSEN_TITLE, 0 );
    SetUInt32Value( PLAYER_FIELD_KILLS, 0 );
    SetUInt32Value( PLAYER_FIELD_LIFETIME_HONORABLE_KILLS, 0 );
    SetUInt32Value( PLAYER_FIELD_TODAY_CONTRIBUTION, 0 );
    SetUInt32Value( PLAYER_FIELD_YESTERDAY_CONTRIBUTION, 0 );

    // set starting level
    if (GetSession()->GetSecurity() >= SEC_MODERATOR)
        SetUInt32Value (UNIT_FIELD_LEVEL, sWorld.getConfig(CONFIG_START_GM_LEVEL));
    else
        SetUInt32Value (UNIT_FIELD_LEVEL, sWorld.getConfig(CONFIG_START_PLAYER_LEVEL));

    SetUInt32Value (PLAYER_FIELD_COINAGE, sWorld.getConfig(CONFIG_START_PLAYER_MONEY));
    SetUInt32Value (PLAYER_FIELD_HONOR_CURRENCY, sWorld.getConfig(CONFIG_START_HONOR_POINTS));
    SetUInt32Value (PLAYER_FIELD_ARENA_CURRENCY, sWorld.getConfig(CONFIG_START_ARENA_POINTS));

    // start with every map explored
    if(sWorld.getConfig(CONFIG_START_ALL_EXPLORED))
    {
        for (uint8 i=0; i<64; i++)
            SetFlag(PLAYER_EXPLORED_ZONES_1+i,0xFFFFFFFF);
    }

    //Reputations if "StartAllReputation" is enabled, -- TODO: Fix this in a better way
    if(sWorld.getConfig(CONFIG_START_ALL_REP))
        SetAtLoginFlag(AT_LOGIN_ALL_REP);
        
    // Played time
    m_Last_tick = time(NULL);
    m_Played_time[0] = 0;
    m_Played_time[1] = 0;

    // base stats and related field values
    InitStatsForLevel();
    InitTaxiNodesForLevel();
    InitTalentForLevel();
    InitPrimaryProffesions();                               // to max set before any spell added

    // apply original stats mods before spell loading or item equipment that call before equip _RemoveStatsMods()
    UpdateMaxHealth();                                      // Update max Health (for add bonus from stamina)
    SetHealth(GetMaxHealth());
    if (getPowerType()==POWER_MANA)
    {
        UpdateMaxPower(POWER_MANA);                         // Update max Mana (for add bonus from intellect)
        SetPower(POWER_MANA,GetMaxPower(POWER_MANA));
    }

    // original spells
    learnDefaultSpells(true);

    // original action bar
    std::list<uint16>::const_iterator action_itr[4];
    for(int i=0; i<4; i++)
        action_itr[i] = info->action[i].begin();

    for (; action_itr[0]!=info->action[0].end() && action_itr[1]!=info->action[1].end();)
    {
        uint16 taction[4];
        for(int i=0; i<4 ;i++)
            taction[i] = (*action_itr[i]);

	addActionButton((uint8)taction[0], taction[1], (uint8)taction[2], (uint8)taction[3]);

        for(int i=0; i<4 ;i++)
            ++action_itr[i];
    }

    // original items
    CharStartOutfitEntry const* oEntry = NULL;
    for (uint32 i = 1; i < sCharStartOutfitStore.GetNumRows(); ++i)
    {
        if(CharStartOutfitEntry const* entry = sCharStartOutfitStore.LookupEntry(i))
        {
            if(entry->RaceClassGender == RaceClassGender)
            {
                oEntry = entry;
                break;
            }
        }
    }

    if(oEntry)
    {
        for(int j = 0; j < MAX_OUTFIT_ITEMS; ++j)
        {
            if(oEntry->ItemId[j] <= 0)
                continue;

            uint32 item_id = oEntry->ItemId[j];

            ItemPrototype const* iProto = objmgr.GetItemPrototype(item_id);
            if(!iProto)
            {
                sLog.outErrorDb("Initial item id %u (race %u class %u) from CharStartOutfit.dbc not listed in `item_template`, ignoring.",item_id,getRace(),getClass());
                continue;
            }

            uint32 count = iProto->Stackable;               // max stack by default (mostly 1)
            if(iProto->Class==ITEM_CLASS_CONSUMABLE && iProto->SubClass==ITEM_SUBCLASS_FOOD)
            {
                switch(iProto->Spells[0].SpellCategory)
                {
                    case SPELL_CATEGORY_FOOD:                                // food
                        if(iProto->Stackable > 4)
                            count = 4;
                        break;
                    case SPELL_CATEGORY_DRINK:                                // drink
                        if(iProto->Stackable > 2)
                            count = 2;
                        break;
                }
            }

            StoreNewItemInBestSlots(item_id, count);
        }
    }

    for (PlayerCreateInfoItems::const_iterator item_id_itr = info->item.begin(); item_id_itr!=info->item.end(); ++item_id_itr++)
        StoreNewItemInBestSlots(item_id_itr->item_id, item_id_itr->item_amount);

    // bags and main-hand weapon must equipped at this moment
    // now second pass for not equipped (offhand weapon/shield if it attempt equipped before main-hand weapon)
    // or ammo not equipped in special bag
    for(int i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; i++)
    {
        if(Item* pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i ))
        {
            uint16 eDest;
            // equip offhand weapon/shield if it attempt equipped before main-hand weapon
            uint8 msg = CanEquipItem( NULL_SLOT, eDest, pItem, false );
            if( msg == EQUIP_ERR_OK )
            {
                RemoveItem(INVENTORY_SLOT_BAG_0, i,true);
                EquipItem( eDest, pItem, true);
            }
            // move other items to more appropriate slots (ammo not equipped in special bag)
            else
            {
                ItemPosCountVec sDest;
                msg = CanStoreItem( NULL_BAG, NULL_SLOT, sDest, pItem, false );
                if( msg == EQUIP_ERR_OK )
                {
                    RemoveItem(INVENTORY_SLOT_BAG_0, i,true);
                    pItem = StoreItem( sDest, pItem, true);
                }

                // if  this is ammo then use it
                uint8 msg = CanUseAmmo( pItem->GetProto()->ItemId );
                if( msg == EQUIP_ERR_OK )
                    SetAmmo( pItem->GetProto()->ItemId );
            }
        }
    }
    // all item positions resolved

    m_lastGenderChange = 0;

    if (sWorld.getConfig(CONFIG_ARENASERVER_ENABLED))
    {
        if(class_ == CLASS_HUNTER)
        { // Pet spells
            uint32 spellsId [119] = {5149,883,1515,6991,2641,982,17254,737,17262,24424,26184,3530,26185,35303,311,26184,17263,7370,35299,35302,17264,1749,231,2441,23111,2976,23111,17266,2981,17262,24609,2976,26094,2982,298,1747,17264,24608,26189,24454,23150,24581,2977,1267,1748,26065,24455,1751,17265,23146,17267,23112,17265,2310,23100,24451,175,24607,2315,2981,24641,25013,25014,17263,3667,24584,3667,2975,23146,25015,1749,26185,1750,35388,17266,24607,25016,23149,24588,23149,295,27361,26202,35306,2619,2977,16698,3666,3666,24582,23112,26202,1751,16698,24582,17268,24599,24589,25017,35391,3489,28343,35307,27347,27349,353,24599,35324,27347,35348,27348,17268,27348,27346,24845,27361,2751,24632,35308 };
            for (int i = 0; i < 119; i++)
                addSpell(spellsId[i],true);
        }

        //class specific spells/skills from recuperation data
        int faction = (GetTeam() == ALLIANCE) ? 1 : 2;
        QueryResult* query = WorldDatabase.PQuery("SELECT command FROM recups_data WHERE classe = %u AND (faction = %u OR faction = 0)", class_, faction);
        if (query) {
            do {
                Field* fields = query->Fetch();
                std::string tempstr = fields[0].GetString();

                {
                    std::vector<std::string> v, vline;
                    std::vector<std::string>::iterator i;

                    int cutAt;
                    while ((cutAt = tempstr.find_first_of(";")) != tempstr.npos) {
                        if (cutAt > 0) {
                            vline.push_back(tempstr.substr(0, cutAt));
                        }
                        tempstr = tempstr.substr(cutAt + 1);
                    }

                    if (tempstr.length() > 0) {
                        vline.push_back(tempstr);
                    }

                    for (i = vline.begin(); i != vline.end(); i++) {
                        v.clear();
                        tempstr = *i;
                        while ((cutAt = tempstr.find_first_of(" ")) != tempstr.npos) {
                            if (cutAt > 0) {
                                v.push_back(tempstr.substr(0, cutAt));
                            }
                            tempstr = tempstr.substr(cutAt + 1);
                        }

                        if (tempstr.length() > 0) {
                            v.push_back(tempstr);
                        }

                        if (v[0] == "learn") {
                            uint32 spell = atol(v[1].c_str());
                            SpellEntry const* spellInfo = spellmgr.LookupSpell(spell);
                            if (!spellInfo || !SpellMgr::IsSpellValid(spellInfo, m_session->GetPlayer())) {
                                continue;
                            }

                            if (!HasSpell(spell))
                                addSpell(spell,true);
                        } else if (v[0] == "setskill") {
                            /* skill, v[1] == skill ID */
                            int32 skill = atoi(v[1].c_str());
                            if (skill <= 0) {
                                continue;
                            }

                            int32 maxskill = 375;

                            SkillLineEntry const* sl = sSkillLineStore.LookupEntry(skill);
                            if (!sl) {
                                continue;
                            }

                            if (!GetSkillValue(skill)) {
                                continue;
                            }

                            SetSkill(skill, 375, maxskill);
                        }
                    }
                }
            } while (query->NextRow());
        } else {
            sLog.outError("Player creation : failed to get data from recups_data to add initial spells/skills");
        }

        SetSkill(129,375,375); //first aid
        addSpell(27028,true); //first aid spell
        addSpell(27033,true); //bandage
        addSpell(28029,true); //master ench
        SetSkill(333,375,375); //max it
        addSpell(23803,true);//  [Ench. d'arme (Esprit renforc�) frFR] 
        addSpell(34002,true); // [Ench. de brassards (Assaut) frFR]
        addSpell(25080,true); // [Ench. de gants (Agilit� excellente) frFR]
        addSpell(34091,true); //mount 280 
    
        //Pala mounts
        if(class_ == CLASS_PALADIN)
        {
            if(GetTeam() == ALLIANCE) {
                addSpell(23214,true); //60
                addSpell(13819,true); //40
            } else {
                addSpell(34767,true); //60
                addSpell(34769,true); //40
            }
        }
    }

    return true;
}

bool Player::StoreNewItemInBestSlots(uint32 titem_id, uint32 titem_amount)
{
    // attempt equip by one
    while(titem_amount > 0)
    {
        uint16 eDest;
        uint8 msg = CanEquipNewItem( NULL_SLOT, eDest, titem_id, false );
        if( msg != EQUIP_ERR_OK )
            break;

        EquipNewItem( eDest, titem_id, true);
        AutoUnequipOffhandIfNeed();
        --titem_amount;
    }

    if(titem_amount == 0)
        return true;                                        // equipped

    // attempt store
    ItemPosCountVec sDest;
    // store in main bag to simplify second pass (special bags can be not equipped yet at this moment)
    uint8 msg = CanStoreNewItem( INVENTORY_SLOT_BAG_0, NULL_SLOT, sDest, titem_id, titem_amount );
    if( msg == EQUIP_ERR_OK )
    {
        StoreNewItem( sDest, titem_id, true, Item::GenerateItemRandomPropertyId(titem_id) );
        return true;                                        // stored
    }

    // item can't be added
    sLog.outError("STORAGE: Can't equip or store initial item %u for race %u class %u , error msg = %u",titem_id,getRace(),getClass(),msg);
    return false;
}

uint32 Player::GetEquipedItemsLevelSum()
{
                        /* Head  Neck  Should. Back   Chest  Waist   MH     OH    Ranged Hands  Wrist  Legs   Feet */
    uint16 posTab[13] = { 65280, 65281, 65282, 65294, 65284, 65288, 65295, 65296, 65297, 65289, 65285, 65286, 65287 };
    uint32 levelSum = 0;
    
    for (uint8 i = 0; i < 13; i++) {
        Item* item = GetItemByPos(posTab[i]);
        if (!item)
            continue;
        
        levelSum += item->GetProto()->ItemLevel;
    }
    
    return levelSum;
}

void Player::SendMirrorTimer(MirrorTimerType Type, uint32 MaxValue, uint32 CurrentValue, int32 Regen)
{
    if (int(MaxValue) == DISABLED_MIRROR_TIMER)
    {
        if (int(CurrentValue) != DISABLED_MIRROR_TIMER)
            StopMirrorTimer(Type);
        return;
    }

    WorldPacket data(SMSG_START_MIRROR_TIMER, (21));
    data << (uint32)Type;
    data << CurrentValue;
    data << MaxValue;
    data << Regen;
    data << (uint8)0;
    data << (uint32)0;                                      // spell id
    GetSession()->SendPacket(&data);
}

void Player::StopMirrorTimer(MirrorTimerType Type)
{
    m_MirrorTimer[Type] = DISABLED_MIRROR_TIMER;

    WorldPacket data(SMSG_STOP_MIRROR_TIMER, 4);
    data << (uint32)Type;
    GetSession()->SendPacket(&data);
}

void Player::EnvironmentalDamage(EnviromentalDamage type, uint32 damage)
{
    if (!isAlive() || isGameMaster() || isSpectator())
        return;

    // Absorb, resist some environmental damage type
    uint32 absorb = 0;
    uint32 resist = 0;

    if (type == DAMAGE_LAVA)
        CalcAbsorbResist(this, SPELL_SCHOOL_MASK_FIRE, DIRECT_DAMAGE, damage, &absorb, &resist, 0);
    else if (type == DAMAGE_SLIME)
        CalcAbsorbResist(this, SPELL_SCHOOL_MASK_NATURE, DIRECT_DAMAGE, damage, &absorb, &resist, 0);

    damage-=absorb+resist;

    WorldPacket data(SMSG_ENVIRONMENTALDAMAGELOG, (21));
    data << uint64(GetGUID());
    data << uint8(type != DAMAGE_FALL_TO_VOID ? type : DAMAGE_FALL);
    data << uint32(damage);
    data << uint32(absorb);
    data << uint32(resist);
    SendMessageToSet(&data, true);

    DealDamage(this, damage, NULL, SELF_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);

    if (type==DAMAGE_FALL && !isAlive())                     // DealDamage not apply item durability loss at self damage
    {
        DEBUG_LOG("We are fall to death, loosing 10 percents durability");
        DurabilityLossAll(0.10f,false);
        // durability lost message
        WorldPacket data(SMSG_DURABILITY_DAMAGE_DEATH, 0);
        GetSession()->SendPacket(&data);
    }
}

int32 Player::getMaxTimer(MirrorTimerType timer)
{
    switch (timer)
    {
        case FATIGUE_TIMER:
            return MINUTE*IN_MILLISECONDS;
        case BREATH_TIMER:
        {
            if (!isAlive() || HasAuraType(SPELL_AURA_WATER_BREATHING) || GetSession()->GetSecurity() >= sWorld.getConfig(CONFIG_DISABLE_BREATHING))
                return DISABLED_MIRROR_TIMER;
            int32 UnderWaterTime = MINUTE*IN_MILLISECONDS;
            AuraList const& mModWaterBreathing = GetAurasByType(SPELL_AURA_MOD_WATER_BREATHING);
            for (AuraList::const_iterator i = mModWaterBreathing.begin(); i != mModWaterBreathing.end(); ++i)
                UnderWaterTime = uint32(UnderWaterTime * (100.0f + (*i)->GetModifierValue()) / 100.0f);
            return UnderWaterTime;
        }
        case FIRE_TIMER:
        {
            if (!isAlive())
                return DISABLED_MIRROR_TIMER;
            return IN_MILLISECONDS;
        }
        default:
            return 0;
    }
}

void Player::UpdateMirrorTimers()
{
    // Desync flags for update on next HandleDrowning
    if (m_MirrorTimerFlags)
        m_MirrorTimerFlagsLast = ~m_MirrorTimerFlags;
}

void Player::HandleDrowning(uint32 time_diff)
{
    if (!m_MirrorTimerFlags)
        return;

    // In water
    if (m_MirrorTimerFlags & UNDERWATER_INWATER)
    {
        // Breath timer not activated - activate it
        if (m_MirrorTimer[BREATH_TIMER] == DISABLED_MIRROR_TIMER)
        {
            m_MirrorTimer[BREATH_TIMER] = getMaxTimer(BREATH_TIMER);
            SendMirrorTimer(BREATH_TIMER, m_MirrorTimer[BREATH_TIMER], m_MirrorTimer[BREATH_TIMER], -1);
        }
        else                                                              // If activated - do tick
        {
            m_MirrorTimer[BREATH_TIMER]-=time_diff;
            // Timer limit - need deal damage
            if (m_MirrorTimer[BREATH_TIMER] < 0)
            {
                m_MirrorTimer[BREATH_TIMER]+= 1*IN_MILLISECONDS;
                // Calculate and deal damage
                // TODO: Check this formula
                uint32 damage = GetMaxHealth() / 5 + GetMap()->urand(0, getLevel()-1);
                EnvironmentalDamage(DAMAGE_DROWNING, damage);
            }
            else if (!(m_MirrorTimerFlagsLast & UNDERWATER_INWATER))      // Update time in client if need
                SendMirrorTimer(BREATH_TIMER, getMaxTimer(BREATH_TIMER), m_MirrorTimer[BREATH_TIMER], -1);
        }
    }
    else if (m_MirrorTimer[BREATH_TIMER] != DISABLED_MIRROR_TIMER)        // Regen timer
    {
        int32 UnderWaterTime = getMaxTimer(BREATH_TIMER);
        // Need breath regen
        m_MirrorTimer[BREATH_TIMER]+=10*time_diff;
        if (m_MirrorTimer[BREATH_TIMER] >= UnderWaterTime || !isAlive())
            StopMirrorTimer(BREATH_TIMER);
        else if (m_MirrorTimerFlagsLast & UNDERWATER_INWATER)
            SendMirrorTimer(BREATH_TIMER, UnderWaterTime, m_MirrorTimer[BREATH_TIMER], 10);
    }

    // In dark water
    if (m_MirrorTimerFlags & UNDERWATER_INDARKWATER)
    {
        // Fatigue timer not activated - activate it
        if (m_MirrorTimer[FATIGUE_TIMER] == DISABLED_MIRROR_TIMER)
        {
            m_MirrorTimer[FATIGUE_TIMER] = getMaxTimer(FATIGUE_TIMER);
            SendMirrorTimer(FATIGUE_TIMER, m_MirrorTimer[FATIGUE_TIMER], m_MirrorTimer[FATIGUE_TIMER], -1);
        }
        else
        {
            m_MirrorTimer[FATIGUE_TIMER]-=time_diff;
            // Timer limit - need deal damage or teleport ghost to graveyard
            if (m_MirrorTimer[FATIGUE_TIMER] < 0)
            {
                m_MirrorTimer[FATIGUE_TIMER]+= 1*IN_MILLISECONDS;
                if (isAlive())                                            // Calculate and deal damage
                {
                    uint32 damage = GetMaxHealth() / 5 + GetMap()->urand(0, getLevel()-1);
                    EnvironmentalDamage(DAMAGE_EXHAUSTED, damage);
                }
                else if (HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST))       // Teleport ghost to graveyard
                    RepopAtGraveyard();
            }
            else if (!(m_MirrorTimerFlagsLast & UNDERWATER_INDARKWATER))
                SendMirrorTimer(FATIGUE_TIMER, getMaxTimer(FATIGUE_TIMER), m_MirrorTimer[FATIGUE_TIMER], -1);
        }
    }
    else if (m_MirrorTimer[FATIGUE_TIMER] != DISABLED_MIRROR_TIMER)       // Regen timer
    {
        int32 DarkWaterTime = getMaxTimer(FATIGUE_TIMER);
        m_MirrorTimer[FATIGUE_TIMER]+=10*time_diff;
        if (m_MirrorTimer[FATIGUE_TIMER] >= DarkWaterTime || !isAlive())
            StopMirrorTimer(FATIGUE_TIMER);
        else if (m_MirrorTimerFlagsLast & UNDERWATER_INDARKWATER)
            SendMirrorTimer(FATIGUE_TIMER, DarkWaterTime, m_MirrorTimer[FATIGUE_TIMER], 10);
    }

    if (m_MirrorTimerFlags & (UNDERWATER_INLAVA|UNDERWATER_INSLIME))
    {
        // Breath timer not activated - activate it
        if (m_MirrorTimer[FIRE_TIMER] == DISABLED_MIRROR_TIMER)
            m_MirrorTimer[FIRE_TIMER] = getMaxTimer(FIRE_TIMER);
        else
        {
            m_MirrorTimer[FIRE_TIMER]-=time_diff;
            if (m_MirrorTimer[FIRE_TIMER] < 0)
            {
                m_MirrorTimer[FIRE_TIMER]+= 1*IN_MILLISECONDS;
                // Calculate and deal damage
                // TODO: Check this formula
                uint32 damage = GetMap()->urand(600, 700);
                if (m_MirrorTimerFlags&UNDERWATER_INLAVA)
                    EnvironmentalDamage(DAMAGE_LAVA, damage);
                else
                    EnvironmentalDamage(DAMAGE_SLIME, damage);
            }
        }
    }
    else
        m_MirrorTimer[FIRE_TIMER] = DISABLED_MIRROR_TIMER;

    // Recheck timers flag
    m_MirrorTimerFlags&=~UNDERWATER_EXIST_TIMERS;
    for (uint8 i = 0; i< MAX_TIMERS; ++i)
        if (m_MirrorTimer[i] != DISABLED_MIRROR_TIMER)
        {
            m_MirrorTimerFlags|=UNDERWATER_EXIST_TIMERS;
            break;
        }
    m_MirrorTimerFlagsLast = m_MirrorTimerFlags;
}

///The player sobers by 256 every 10 seconds
void Player::HandleSobering()
{
    m_drunkTimer = 0;

    uint32 drunk = (m_drunk <= 256) ? 0 : (m_drunk - 256);
    SetDrunkValue(drunk);
}

DrunkenState Player::GetDrunkenstateByValue(uint16 value)
{
    if(value >= 23000)
        return DRUNKEN_SMASHED;
    if(value >= 12800)
        return DRUNKEN_DRUNK;
    if(value & 0xFFFE)
        return DRUNKEN_TIPSY;
    return DRUNKEN_SOBER;
}

void Player::SetDrunkValue(uint16 newDrunkenValue, uint32 itemId)
{
    uint32 oldDrunkenState = Player::GetDrunkenstateByValue(m_drunk);

    m_drunk = newDrunkenValue;
    SetUInt32Value(PLAYER_BYTES_3,(GetUInt32Value(PLAYER_BYTES_3) & 0xFFFF0001) | (m_drunk & 0xFFFE));

    uint32 newDrunkenState = Player::GetDrunkenstateByValue(m_drunk);

    // special drunk invisibility detection
    if(newDrunkenState >= DRUNKEN_DRUNK)
        m_detectInvisibilityMask |= (1<<6);
    else
        m_detectInvisibilityMask &= ~(1<<6);

    if(newDrunkenState == oldDrunkenState)
        return;

    WorldPacket data(SMSG_CROSSED_INEBRIATION_THRESHOLD, (8+4+4));
    data << uint64(GetGUID());
    data << uint32(newDrunkenState);
    data << uint32(itemId);

    SendMessageToSet(&data, true);
}

void Player::Update( uint32 p_time )
{
    if(!IsInWorld())
        return;

/*    signal(SIGSEGV, __segv_handler);
    if (sigsetjmp(__jmp_env, 1) == 1) {
        sLog.outError("CRASH: Player guid %ld, account id %ld caused crash, kicking it.", GetGUIDLow(), m_session ? m_session->GetAccountId() : 0);
        signal(SIGSEGV, SIG_DFL);
        if (!m_session)
            raise(SIGSEGV);
        m_session->LogoutPlayer(false);
        return;
    }*/

    if (m_kickatnextupdate && m_session) {
        m_kickatnextupdate = false;
        m_session->LogoutPlayer(false);
        return;
    }

    // undelivered mail
    if(m_nextMailDelivereTime && m_nextMailDelivereTime <= time(NULL))
    {
        SendNewMail();
        ++unReadMails;

        // It will be recalculate at mailbox open (for unReadMails important non-0 until mailbox open, it also will be recalculated)
        m_nextMailDelivereTime = 0;
    }

    for(std::map<uint32, uint32>::iterator itr = m_globalCooldowns.begin(); itr != m_globalCooldowns.end(); ++itr)
        if(itr->second)
        {
            if(itr->second > p_time)
                itr->second -= p_time;
            else
                itr->second = 0;
        }

    Unit::Update( p_time );

    time_t now = time (NULL);

    UpdatePvPFlag(now);

    UpdateContestedPvP(p_time);

    UpdateDuelFlag(now);

    CheckDuelDistance(now);

    UpdateAfkReport(now);

    if(isCharmed())
    {
        if(Unit *charmer = GetCharmer())
            if(charmer->GetTypeId() == TYPEID_UNIT && charmer->isAlive())
                UpdateCharmedAI();
    }

    // Update items that have just a limited lifetime
    if (now>m_Last_tick)
        UpdateItemDuration(uint32(now- m_Last_tick));

    if (!m_timedquests.empty())
    {
        std::set<uint32>::iterator iter = m_timedquests.begin();
        while (iter != m_timedquests.end())
        {
            QuestStatusData& q_status = mQuestStatus[*iter];
            if( q_status.m_timer <= p_time )
            {
                uint32 quest_id  = *iter;
                ++iter;                                     // current iter will be removed in FailTimedQuest
                FailTimedQuest( quest_id );
            }
            else
            {
                q_status.m_timer -= p_time;
                if (q_status.uState != QUEST_NEW) q_status.uState = QUEST_CHANGED;
                ++iter;
            }
        }
    }

    if (hasUnitState(UNIT_STAT_MELEE_ATTACKING) && !hasUnitState(UNIT_STAT_CASTING))
    {
        if(Unit *pVictim = getVictim())
        {
            // default combat reach 10
            // TODO add weapon,skill check

            if (isAttackReady(BASE_ATTACK))
            {
                if(!IsWithinMeleeRange(pVictim))
                {
                    setAttackTimer(BASE_ATTACK,100);
                    if(m_swingErrorMsg != 1)                // send single time (client auto repeat)
                    {
                        SendAttackSwingNotInRange();
                        m_swingErrorMsg = 1;
                    }
                }
                //120 degrees of radiant range
                else if( !HasInArc( 2*M_PI/3, pVictim ))
                {
                    setAttackTimer(BASE_ATTACK,100);
                    if(m_swingErrorMsg != 2)                // send single time (client auto repeat)
                    {
                        SendAttackSwingBadFacingAttack();
                        m_swingErrorMsg = 2;
                    }
                }
                else
                {
                    m_swingErrorMsg = 0;                    // reset swing error state

                    // prevent base and off attack in same time, delay attack at 0.2 sec
                    if(haveOffhandWeapon())
                    {
                        uint32 off_att = getAttackTimer(OFF_ATTACK);
                        if(off_att < ATTACK_DISPLAY_DELAY)
                            setAttackTimer(OFF_ATTACK,ATTACK_DISPLAY_DELAY);
                    }
                    AttackerStateUpdate(pVictim, BASE_ATTACK);
                    resetAttackTimer(BASE_ATTACK);
                }
            }

            if ( haveOffhandWeapon() && isAttackReady(OFF_ATTACK))
            {
                if(!IsWithinMeleeRange(pVictim))
                {
                    setAttackTimer(OFF_ATTACK,100);
                }
                else if( !HasInArc( 2*M_PI/3, pVictim ))
                {
                    setAttackTimer(OFF_ATTACK,100);
                }
                else
                {
                    // prevent base and off attack in same time, delay attack at 0.2 sec
                    uint32 base_att = getAttackTimer(BASE_ATTACK);
                    if(base_att < ATTACK_DISPLAY_DELAY)
                        setAttackTimer(BASE_ATTACK,ATTACK_DISPLAY_DELAY);
                    // do attack
                    AttackerStateUpdate(pVictim, OFF_ATTACK);
                    resetAttackTimer(OFF_ATTACK);
                }
            }

            /*Unit *owner = pVictim->GetOwner();
            Unit *u = owner ? owner : pVictim;
            if(u->IsPvP() && (!duel || duel->opponent != u))
            {
                UpdatePvP(true);
                RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_ENTER_PVP_COMBAT);
            }*/
        }
    }

    if(HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_RESTING))
    {
        if(roll_chance_i(3) && GetTimeInnEnter() > 0)       //freeze update
        {
            int time_inn = time(NULL)-GetTimeInnEnter();
            if (time_inn >= 10)                             //freeze update
            {
                float bubble = 0.125*sWorld.getRate(RATE_REST_INGAME);
                                                            //speed collect rest bonus (section/in hour)
                SetRestBonus( GetRestBonus()+ time_inn*((float)GetUInt32Value(PLAYER_NEXT_LEVEL_XP)/72000)*bubble );
                UpdateInnerTime(time(NULL));
            }
        }
    }

    if(m_regenTimer > 0)
    {
        if(p_time >= m_regenTimer)
            m_regenTimer = 0;
        else
            m_regenTimer -= p_time;
    }

    if (m_weaponChangeTimer > 0)
    {
        if(p_time >= m_weaponChangeTimer)
            m_weaponChangeTimer = 0;
        else
            m_weaponChangeTimer -= p_time;
    }

    if (m_zoneUpdateTimer > 0)
    {
        if(p_time >= m_zoneUpdateTimer)
        {
            uint32 newzone = GetZoneId();
            if( m_zoneUpdateId != newzone )
                UpdateZone(newzone);                        // also update area
            else
            {
                // use area updates as well
                // needed for free far all arenas for example
                uint32 newarea = GetAreaId();
                if( m_areaUpdateId != newarea )
                    UpdateArea(newarea);

                m_zoneUpdateTimer = ZONE_UPDATE_INTERVAL;
            }
        }
        else
            m_zoneUpdateTimer -= p_time;
    }

    if (isAlive())
    {
        RegenerateAll();
    }

    if (m_deathState == JUST_DIED)
    {
        KillPlayer();
    }

    if(m_nextSave > 0)
    {
        if(p_time >= m_nextSave)
        {
            // m_nextSave reseted in SaveToDB call
            SaveToDB();
            sLog.outDetail("Player '%s' (GUID: %u) saved", GetName(), GetGUIDLow());
        }
        else
        {
            m_nextSave -= p_time;
        }
    }

    //Handle Water/drowning
    HandleDrowning(p_time);

    //Handle detect stealth players
    if (m_DetectInvTimer > 0)
    {
        if (p_time >= m_DetectInvTimer)
        {
            m_DetectInvTimer = 3000;
            HandleStealthedUnitsDetection();
        }
        else
            m_DetectInvTimer -= p_time;
    }

    // Played time
    if (now > m_Last_tick)
    {
        uint32 elapsed = uint32(now - m_Last_tick);
        m_Played_time[0] += elapsed;                        // Total played time
        m_Played_time[1] += elapsed;                        // Level played time
        m_Last_tick = now;
    }

    if (m_drunk)
    {
        m_drunkTimer += p_time;

        if (m_drunkTimer > 10000)
            HandleSobering();
    }

    // not auto-free ghost from body in instances
    if(m_deathTimer > 0  && !GetBaseMap()->Instanceable())
    {
        if(p_time >= m_deathTimer)
        {
            m_deathTimer = 0;
            BuildPlayerRepop();
            RepopAtGraveyard();
        }
        else
            m_deathTimer -= p_time;
    }

    UpdateEnchantTime(p_time);
    UpdateHomebindTime(p_time);

    // group update
    SendUpdateToOutOfRangeGroupMembers();
    
   /* if (now > _attackersCheckTime) {
        HostilReference* ref = getHostilRefManager().getFirst();
        while (ref) {
            HostilReference* nextRef = ref->next();
            if (Unit* target = ref->getSourceUnit()) {
                if (!this->canSeeOrDetect(target, false, true) || IsFriendlyTo(target) || !target->isInCombat())
                    _removeAttacker(target);
            }

            ref = nextRef;
        }
        
        _attackersCheckTime = now + 5;
    }*/

    Pet* pet = GetPet();
    if(pet && !IsWithinDistInMap(pet, OWNER_MAX_DISTANCE) && !pet->isPossessed())
    {
        RemovePet(pet, PET_SAVE_NOT_IN_SLOT, true);
        return;
    }
    
    /*if (GetMapId() == 564 && GetPositionZ() <= 180.0f && GetPositionZ() >= 160.0f) {
        if (ScriptedInstance* pInstance = ((ScriptedInstance*)GetInstanceData())) {
            if (pInstance->GetData(14) == IN_PROGRESS && isAlive())
                TeleportTo(GetMapId(), GetPositionX(), GetPositionY(), 195.0f, GetOrientation());
        }
    }*/
}

void Player::setDeathState(DeathState s)
{
    uint32 ressSpellId = 0;

    bool cur = isAlive();

    if(s == JUST_DIED)
    {
        if(!cur)
        {
            sLog.outError("setDeathState: attempt to kill a dead player %s(%d)", GetName(), GetGUIDLow());
            return;
        }

        // send spectate addon message
        if (HaveSpectators())
        {
            SpectatorAddonMsg msg;
            msg.SetPlayer(GetName());
            msg.SetStatus(false);
            SendSpectatorAddonMsgToBG(msg);
        }

        // drunken state is cleared on death
        SetDrunkValue(0);
        // lost combo points at any target (targeted combo points clear in Unit::setDeathState)
        ClearComboPoints();

        clearResurrectRequestData();

        // remove form before other mods to prevent incorrect stats calculation
        RemoveAurasDueToSpell(m_ShapeShiftFormSpellId);

        //FIXME: is pet dismissed at dying or releasing spirit? if second, add setDeathState(DEAD) to HandleRepopRequestOpcode and define pet unsummon here with (s == DEAD)
        RemovePet(NULL, PET_SAVE_NOT_IN_SLOT, true);

        // remove uncontrolled pets
        RemoveMiniPet();
        RemoveGuardians();

        // save value before aura remove in Unit::setDeathState
        ressSpellId = GetUInt32Value(PLAYER_SELF_RES_SPELL);

        // passive spell
        if(!ressSpellId)
            ressSpellId = GetResurrectionSpellId();
    }
    Unit::setDeathState(s);

    // restore resurrection spell id for player after aura remove
    if(s == JUST_DIED && cur && ressSpellId)
        SetUInt32Value(PLAYER_SELF_RES_SPELL, ressSpellId);

    if(isAlive() && !cur)
    {
        //clear aura case after resurrection by another way (spells will be applied before next death)
        SetUInt32Value(PLAYER_SELF_RES_SPELL, 0);

        // restore default warrior stance
        if(getClass()== CLASS_WARRIOR)
            CastSpell(this,SPELL_ID_PASSIVE_BATTLE_STANCE,true);
    }
}

void Player::SetSelection(uint64 guid)
{
    m_curSelection = guid;
    SetUInt64Value(UNIT_FIELD_TARGET, guid);
}

bool Player::BuildEnumData( QueryResult * result, WorldPacket * p_data )
{
    //          0                1                2                3                      4                      5                      6               7                     8
    //  "SELECT characters.guid, characters.data, characters.name, characters.position_x, characters.position_y, characters.position_z, characters.map, characters.totaltime, characters.leveltime, "
    //   9                    10                   11                     12                   13                    14               15                16                 17
    //  "characters.at_login, character_pet.entry, character_pet.modelid, character_pet.level, guild_member.guildid, characters.race, characters.class, characters.gender, characters.playerBytes, "
    //   18                       19                      20                21             22                23
    //  "characters.playerBytes2, characters.playerFlags, characters.level, characters.xp, characters.money, characters.equipmentCache "
    
    Field *fields = result->Fetch();
        
    uint32 guid = fields[0].GetUInt32();
    uint8 pRace = fields[14].GetUInt8();
    uint8 pClass = fields[15].GetUInt8();
    
    PlayerInfo const *info = objmgr.GetPlayerInfo(pRace, pClass);
    if(!info)
    {
        sLog.outError("Player %u have incorrect race/class pair. Don't build enum.", guid);
        return false;
    }
    
    *p_data << uint64(MAKE_NEW_GUID(guid, 0, HIGHGUID_PLAYER));
    *p_data << fields[2].GetString();                           // name
    *p_data << uint8(pRace);                                    // race
    *p_data << uint8(pClass);                                   // class
    *p_data << uint8(fields[16].GetUInt8());                    // gender

    uint32 playerBytes = fields[17].GetUInt32();
    *p_data << uint8(playerBytes);                              // skin
    *p_data << uint8(playerBytes >> 8);                         // face
    *p_data << uint8(playerBytes >> 16);                        // hair style
    *p_data << uint8(playerBytes >> 24);                        // hair color

    uint32 playerBytes2 = fields[18].GetUInt32();
    *p_data << uint8(playerBytes2 & 0xFF);                      // facial hair
    
    *p_data << uint8(fields[20].GetUInt8());                    // level
    
    // do not use GetMap! it will spawn a new instance since the bound instances are not loaded
    uint32 zoneId = MapManager::Instance().GetZoneId(fields[6].GetUInt32(), fields[3].GetFloat(),fields[4].GetFloat(), fields[5].GetFloat());
    *p_data << zoneId;
    *p_data << uint32(fields[6].GetUInt32());                   // map

    *p_data << fields[3].GetFloat();                            // x
    *p_data << fields[4].GetFloat();                            // y
    *p_data << fields[5].GetFloat();                            // z

    *p_data << (result ? result->Fetch()[13].GetUInt32() : 0);

    uint32 char_flags = 0;
    uint32 playerFlags = fields[19].GetUInt32();
    uint32 atLoginFlags = fields[9].GetUInt32();
    if (playerFlags & PLAYER_FLAGS_HIDE_HELM)
        char_flags |= CHARACTER_FLAG_HIDE_HELM;
    if (playerFlags & PLAYER_FLAGS_HIDE_CLOAK)
        char_flags |= CHARACTER_FLAG_HIDE_CLOAK;
    if (playerFlags & PLAYER_FLAGS_GHOST)
        char_flags |= CHARACTER_FLAG_GHOST;
    if (atLoginFlags & AT_LOGIN_RENAME)
        char_flags |= CHARACTER_FLAG_RENAME;
    // always send the flag if declined names aren't used
    // to let the client select a default method of declining the name
    if(!sWorld.getConfig(CONFIG_DECLINED_NAMES_USED) || (result && result->Fetch()[14].GetCppString() != ""))
        char_flags |= CHARACTER_FLAG_DECLINED;

    *p_data << (uint32)char_flags;                          // character flags

    *p_data << (uint8)1;                                    // unknown

    // Pets info
    {
        uint32 petDisplayId = 0;
        uint32 petLevel   = 0;
        uint32 petFamily  = 0;

        // show pet at selection character in character list  only for non-ghost character
        if(result && !(playerFlags & PLAYER_FLAGS_GHOST) && (pClass == CLASS_WARLOCK || pClass == CLASS_HUNTER))
        {
            Field* fields = result->Fetch();

            uint32 entry = fields[10].GetUInt32();
            CreatureInfo const* cInfo = sCreatureStorage.LookupEntry<CreatureInfo>(entry);
            if(cInfo)
            {
                petDisplayId = fields[11].GetUInt32();
                petLevel     = fields[12].GetUInt32();
                petFamily    = cInfo->family;
            }
        }

        *p_data << (uint32)petDisplayId;
        *p_data << (uint32)petLevel;
        *p_data << (uint32)petFamily;
    }

    Tokens equipCache = StrSplit(fields[23].GetCppString(), " ");
    for (uint8 slot = 0; slot < 38; slot++)
    {
        /*uint32 visualbase = PLAYER_VISIBLE_ITEM_1_0 + (slot * MAX_VISIBLE_ITEM_OFFSET);
        uint32 item_id = GetUInt32Value(visualbase);
        const ItemPrototype * proto = objmgr.GetItemPrototype(item_id);
        SpellItemEnchantmentEntry const *enchant = NULL;

        for(uint8 enchantSlot = PERM_ENCHANTMENT_SLOT; enchantSlot<=TEMP_ENCHANTMENT_SLOT; enchantSlot++)
        {
            uint32 enchantId = GetUInt32Value(visualbase+1+enchantSlot);
            if(enchant = sSpellItemEnchantmentStore.LookupEntry(enchantId))
                break;
        }*/
        
        uint32 itemId = GetUInt32ValueFromArray(equipCache, slot);
        slot++;     // Next field is enchantmentId
        const ItemPrototype *proto = objmgr.GetItemPrototype(itemId);
        SpellItemEnchantmentEntry const *enchant = NULL;

        uint32 enchants = GetUInt32ValueFromArray(equipCache, slot);
        for (uint8 enchantSlot = PERM_ENCHANTMENT_SLOT; enchantSlot <= TEMP_ENCHANTMENT_SLOT; ++enchantSlot)
        {
            // values stored in 2 uint16
            uint32 enchantId = 0x0000FFFF & (enchants >> enchantSlot*16);
            if (!enchantId)
                continue;

            enchant = sSpellItemEnchantmentStore.LookupEntry(enchantId);
            if (enchant)
                break;
        }

        if (proto != NULL)
        {
            *p_data << (uint32)proto->DisplayInfoID;
            *p_data << (uint8)proto->InventoryType;
            *p_data << (uint32)(enchant?enchant->aura_id:0);
        }
        else
        {
            *p_data << (uint32)0;
            *p_data << (uint8)0;
            *p_data << (uint32)0;                           // enchant?
        }
    }
    
    *p_data << (uint32)0;                                   // first bag display id
    *p_data << (uint8)0;                                    // first bag inventory type
    *p_data << (uint32)0;                                   // enchant?
    
    return true;
}

bool Player::ToggleAFK()
{
    ToggleFlag(PLAYER_FLAGS, PLAYER_FLAGS_AFK);

    bool state = HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_AFK);

    // afk player not allowed in battleground
    if(state && InBattleGround() && !InArena())
        LeaveBattleground();

    return state;
}

bool Player::ToggleDND()
{
    ToggleFlag(PLAYER_FLAGS, PLAYER_FLAGS_DND);

    return HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_DND);
}

uint8 Player::chatTag() const
{
    // it's bitmask
    // 0x8 - ??
    // 0x4 - gm
    // 0x2 - dnd
    // 0x1 - afk
    if(isGMChat())
        return 4;
    else if(isDND())
        return 3;
    if(isAFK())
        return 1;
    else
        return 0;
}

bool Player::TeleportTo(uint32 mapid, float x, float y, float z, float orientation, uint32 options)
{
    if(!MapManager::IsValidMapCoord(mapid, x, y, z, orientation))
    {
        sLog.outError("TeleportTo: invalid map %d or absent instance template.", mapid);
        return false;
    }

    if((GetSession()->GetSecurity() < SEC_GAMEMASTER) && !sWorld.IsAllowedMap(mapid))
    {
        sLog.outError("Player %s tried to enter a forbidden map", GetName());
        return false;
    }

    // preparing unsummon pet if lost (we must get pet before teleportation or will not find it later)
    Pet* pet = GetPet();

    MapEntry const* mEntry = sMapStore.LookupEntry(mapid);

    // don't let enter battlegrounds without assigned battleground id (for example through areatrigger)...
    // don't let gm level > 1 either
    if(!InBattleGround() && mEntry->IsBattleGroundOrArena())
        return false;

    // 449 - Champions' Hall (Alliance) // 450 - Hall of Legends (Horde)
    if(mapid == 449 && GetTeam()==HORDE)
    {
        GetSession()->SendNotification(LANG_NO_ENTER_CHAMPIONS_HALL);
        return false;
    }

    if(mapid == 450 && GetTeam() == ALLIANCE)
    {
        GetSession()->SendNotification(LANG_NO_ENTER_HALL_OF_LEGENDS);
        return false;
    }

    // client without expansion support
    if(GetSession()->Expansion() < mEntry->Expansion())
    {
        if(GetTransport())
            RepopAtGraveyard();                             // teleport to near graveyard if on transport, looks blizz like :)

        SendTransferAborted(mapid, TRANSFER_ABORT_INSUF_EXPAN_LVL1);

        return false;                                       // normal client can't teleport to this map...
    }

    // if we were on a transport, leave
    if (!(options & TELE_TO_NOT_LEAVE_TRANSPORT) && m_transport)
    {
        m_transport->RemovePassenger(this);
        m_transport = NULL;
        m_movementInfo.t_x = 0.0f;
        m_movementInfo.t_y = 0.0f;
        m_movementInfo.t_z = 0.0f;
        m_movementInfo.t_o = 0.0f;
        m_movementInfo.t_time = 0;
    }

    SetSemaphoreTeleport(true);

    // The player was ported to another map and looses the duel immediatly.
    // We have to perform this check before the teleport, otherwise the
    // ObjectAccessor won't find the flag.
    if (duel && GetMapId()!=mapid)
    {
        GameObject* obj = ObjectAccessor::GetGameObject(*this, GetUInt64Value(PLAYER_DUEL_ARBITER));
        if (obj)
            DuelComplete(DUEL_FLED);
    }

    // reset movement flags at teleport, because player will continue move with these flags after teleport
    SetUnitMovementFlags(0);

    if ((GetMapId() == mapid) && (!m_transport))
    {
        // prepare zone change detect
        uint32 old_zone = GetZoneId();

        // near teleport
        if(!GetSession()->PlayerLogout())
        {
            WorldPacket data;
            BuildTeleportAckMsg(&data, x, y, z, orientation);
            GetSession()->SendPacket(&data);
            SetPosition( x, y, z, orientation, true);
        }
        else
            // this will be used instead of the current location in SaveToDB
            m_teleport_dest = WorldLocation(mapid, x, y, z, orientation);
        SetFallInformation(0, z);

        //BuildHeartBeatMsg(&data);
        //SendMessageToSet(&data, true);
        if (!(options & TELE_TO_NOT_UNSUMMON_PET))
        {
            //same map, only remove pet if out of range
            if(pet && !IsWithinDistInMap(pet, OWNER_MAX_DISTANCE))
            {
                if(pet->isControlled() && !pet->isTemporarySummoned() )
                    m_temporaryUnsummonedPetNumber = pet->GetCharmInfo()->GetPetNumber();
                else
                    m_temporaryUnsummonedPetNumber = 0;

                RemovePet(pet, PET_SAVE_NOT_IN_SLOT);
            }
        }

        if(!(options & TELE_TO_NOT_LEAVE_COMBAT))
            CombatStop();

        if (!(options & TELE_TO_NOT_UNSUMMON_PET))
        {
            // resummon pet
            if(pet && m_temporaryUnsummonedPetNumber)
            {
                Pet* NewPet = new Pet;
                if(!NewPet->LoadPetFromDB(this, 0, m_temporaryUnsummonedPetNumber, true))
                    delete NewPet;

                m_temporaryUnsummonedPetNumber = 0;
            }
        }

        if(!GetSession()->PlayerLogout())
        {
            // don't reset teleport semaphore while logging out, otherwise m_teleport_dest won't be used in Player::SaveToDB
            SetSemaphoreTeleport(false);

            UpdateZone(GetZoneId());
        }

        // new zone
        if(old_zone != GetZoneId())
        {
            // honorless target
            if(pvpInfo.inHostileArea)
                CastSpell(this, 2479, true);
        }
    }
    else
    {
        // far teleport to another map
        Map* oldmap = IsInWorld() ? GetMap() : NULL;
        // check if we can enter before stopping combat / removing pet / totems / interrupting spells

        // Check enter rights before map getting to avoid creating instance copy for player
        // this check not dependent from map instance copy and same for all instance copies of selected map
        if (!MapManager::Instance().CanPlayerEnter(mapid, this))
        {
            SetSemaphoreTeleport(false);
            return false;
        }

        // If the map is not created, assume it is possible to enter it.
        // It will be created in the WorldPortAck.
        Map *map = MapManager::Instance().FindMap(mapid);
        if (!map ||  map->CanEnter(this))
        {
            SetSelection(0);

            CombatStop();

            ResetContestedPvP();

            // remove player from battleground on far teleport (when changing maps)
            if(BattleGround const* bg = GetBattleGround())
            {
                // Note: at battleground join battleground id set before teleport
                // and we already will found "current" battleground
                // just need check that this is targeted map or leave
                if(bg->GetMapId() != mapid)
                    LeaveBattleground(false);                   // don't teleport to entry point
            }

            // remove pet on map change
            if (pet)
            {
                //leaving map -> delete pet right away (doing this later will cause problems)
                if(pet->isControlled() && !pet->isTemporarySummoned())
                    m_temporaryUnsummonedPetNumber = pet->GetCharmInfo()->GetPetNumber();
                else
                    m_temporaryUnsummonedPetNumber = 0;

                RemovePet(pet, PET_SAVE_NOT_IN_SLOT);
            }

            // remove all dyn objects
            RemoveAllDynObjects();

            // stop spellcasting
            // not attempt interrupt teleportation spell at caster teleport
            if(!(options & TELE_TO_SPELL))
                if(IsNonMeleeSpellCasted(true))
                    InterruptNonMeleeSpells(true);

            if(!GetSession()->PlayerLogout())
            {
                // send transfer packets
                WorldPacket data(SMSG_TRANSFER_PENDING, (4+4+4));
                data << uint32(mapid);
                if (m_transport)
                {
                    data << m_transport->GetEntry() << GetMapId();
                }
                GetSession()->SendPacket(&data);

                data.Initialize(SMSG_NEW_WORLD, (20));
                if (m_transport)
                {
                    data << (uint32)mapid << m_movementInfo.t_x << m_movementInfo.t_y << m_movementInfo.t_z << m_movementInfo.t_o;
                }
                else
                {
                    data << (uint32)mapid << (float)x << (float)y << (float)z << (float)orientation;
                }
                GetSession()->SendPacket( &data );
                SendSavedInstances();

                // remove from old map now
                if(oldmap) oldmap->Remove(this, false);
            }

            // new final coordinates
            float final_x = x;
            float final_y = y;
            float final_z = z;
            float final_o = orientation;

            if(m_transport)
            {
                final_x += m_movementInfo.t_x;
                final_y += m_movementInfo.t_y;
                final_z += m_movementInfo.t_z;
                final_o += m_movementInfo.t_o;
            }

            m_teleport_dest = WorldLocation(mapid, final_x, final_y, final_z, final_o);
            SetFallInformation(0, final_z);
            // if the player is saved before worldportack (at logout for example)
            // this will be used instead of the current location in SaveToDB

            RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_CHANGE_MAP);

            // move packet sent by client always after far teleport
            // SetPosition(final_x, final_y, final_z, final_o, true);
            SetDontMove(true);

            // code for finish transfer to new map called in WorldSession::HandleMoveWorldportAckOpcode at client packet
        }
        else
            return false;
    }
    m_anti_TeleTime=time(NULL);
    return true;
}

void Player::AddToWorld()
{
    ///- Do not add/remove the player from the object storage
    ///- It will crash when updating the ObjectAccessor
    ///- The player should only be added when logging in
    Unit::AddToWorld();

    for(int i = PLAYER_SLOT_START; i < PLAYER_SLOT_END; i++)
    {
        if(m_items[i])
            m_items[i]->AddToWorld();
    }
    
    if (HasAura(45717))
        CastSpell(this, 45917, true);
        
    if (m_session->IsMailChanged()) {
        m_session->SendNotification(LANG_WARNING_MAIL_CHANGED);
        ChatHandler(this).SendSysMessage("**********************************************************************************");
        ChatHandler(this).SendSysMessage(LANG_WARNING_MAIL_CHANGED);
        ChatHandler(this).SendSysMessage("**********************************************************************************");
    }
}

void Player::RemoveFromWorld()
{
    // cleanup
    if(IsInWorld())
    {
        ///- Release charmed creatures, unsummon totems and remove pets/guardians
        StopCastingCharm();
        StopCastingBindSight();
        UnsummonAllTotems();
        RemoveMiniPet();
        RemoveGuardians();
    }

    for(int i = PLAYER_SLOT_START; i < PLAYER_SLOT_END; i++)
    {
        if(m_items[i])
            m_items[i]->RemoveFromWorld();
    }

    if (isSpectator())
    	SetSpectate(false);

    ///- Do not add/remove the player from the object storage
    ///- It will crash when updating the ObjectAccessor
    ///- The player should only be removed when logging out
    Unit::RemoveFromWorld();
}

void Player::RewardRage( uint32 damage, uint32 weaponSpeedHitFactor, bool attacker )
{
    float addRage;

    float rageconversion = ((0.0091107836 * getLevel()*getLevel())+3.225598133*getLevel())+4.2652911;

    if(attacker)
    {
        addRage = ((damage/rageconversion*7.5 + weaponSpeedHitFactor)/2);

        // talent who gave more rage on attack
        addRage *= 1.0f + GetTotalAuraModifier(SPELL_AURA_MOD_RAGE_FROM_DAMAGE_DEALT) / 100.0f;
    }
    else
    {
        addRage = damage/rageconversion*2.5;

        // Berserker Rage effect
        if(HasAura(18499,0))
            addRage *= 1.3;
    }

    addRage *= sWorld.getRate(RATE_POWER_RAGE_INCOME);

    ModifyPower(POWER_RAGE, uint32(addRage*10));
}

void Player::RegenerateAll()
{
    if (m_regenTimer != 0)
        return;
    uint32 regenDelay = 2000;

    // Not in combat or they have regeneration
    if( !isInCombat() || HasAuraType(SPELL_AURA_MOD_REGEN_DURING_COMBAT) ||
        HasAuraType(SPELL_AURA_MOD_HEALTH_REGEN_IN_COMBAT) || IsPolymorphed() )
    {
        RegenerateHealth();
        if (!isInCombat() && !HasAuraType(SPELL_AURA_INTERRUPT_REGEN))
            Regenerate(POWER_RAGE);
    }

    Regenerate( POWER_ENERGY );

    Regenerate( POWER_MANA );

    m_regenTimer = regenDelay;
}

void Player::Regenerate(Powers power)
{
    uint32 curValue = GetPower(power);
    uint32 maxValue = GetMaxPower(power);

    float addvalue = 0.0f;

    switch (power)
    {
        case POWER_MANA:
        {
            bool recentCast = IsUnderLastManaUseEffect();
            float ManaIncreaseRate = sWorld.getRate(RATE_POWER_MANA);
            if (recentCast)
            {
                // Trinity Updates Mana in intervals of 2s, which is correct
                addvalue = GetFloatValue(PLAYER_FIELD_MOD_MANA_REGEN_INTERRUPT) *  ManaIncreaseRate * 2.00f;
            }
            else
            {
                addvalue = GetFloatValue(PLAYER_FIELD_MOD_MANA_REGEN) * ManaIncreaseRate * 2.00f;
            }
        }   break;
        case POWER_RAGE:                                    // Regenerate rage
        {
            float RageDecreaseRate = sWorld.getRate(RATE_POWER_RAGE_LOSS);
            addvalue = 30 * RageDecreaseRate;               // 3 rage by tick
        }   break;
        case POWER_ENERGY:                                  // Regenerate energy (rogue)
            addvalue = 20;
            break;
        case POWER_FOCUS:
        case POWER_HAPPINESS:
            break;
    }

    // Mana regen calculated in Player::UpdateManaRegen()
    // Exist only for POWER_MANA, POWER_ENERGY, POWER_FOCUS auras
    if(power != POWER_MANA)
    {
        AuraList const& ModPowerRegenPCTAuras = GetAurasByType(SPELL_AURA_MOD_POWER_REGEN_PERCENT);
        for(AuraList::const_iterator i = ModPowerRegenPCTAuras.begin(); i != ModPowerRegenPCTAuras.end(); ++i)
            if ((*i)->GetModifier()->m_miscvalue == power)
                addvalue *= ((*i)->GetModifierValue() + 100) / 100.0f;
    }

    if (power != POWER_RAGE)
    {
        curValue += uint32(addvalue);
        if (curValue > maxValue)
            curValue = maxValue;
    }
    else
    {
        if(curValue <= uint32(addvalue))
            curValue = 0;
        else
            curValue -= uint32(addvalue);
    }
    SetPower(power, curValue);
}

void Player::RegenerateHealth()
{
    uint32 curValue = GetHealth();
    uint32 maxValue = GetMaxHealth();

    if (curValue >= maxValue) return;

    float HealthIncreaseRate = sWorld.getRate(RATE_HEALTH);

    float addvalue = 0.0f;

    // polymorphed case
    if ( IsPolymorphed() )
        addvalue = GetMaxHealth()/3;
    // normal regen case (maybe partly in combat case)
    else if (!isInCombat() || HasAuraType(SPELL_AURA_MOD_REGEN_DURING_COMBAT) )
    {
        addvalue = OCTRegenHPPerSpirit()* HealthIncreaseRate;
        if (!isInCombat())
        {
            AuraList const& mModHealthRegenPct = GetAurasByType(SPELL_AURA_MOD_HEALTH_REGEN_PERCENT);
            for(AuraList::const_iterator i = mModHealthRegenPct.begin(); i != mModHealthRegenPct.end(); ++i)
                addvalue *= (100.0f + (*i)->GetModifierValue()) / 100.0f;
        }
        else if(HasAuraType(SPELL_AURA_MOD_REGEN_DURING_COMBAT))
            addvalue *= GetTotalAuraModifier(SPELL_AURA_MOD_REGEN_DURING_COMBAT) / 100.0f;

        if(!IsStandState())
            addvalue *= 1.5;
    }

    // always regeneration bonus (including combat)
    addvalue += GetTotalAuraModifier(SPELL_AURA_MOD_HEALTH_REGEN_IN_COMBAT);

    if(addvalue < 0)
        addvalue = 0;

    ModifyHealth(int32(addvalue));
}

bool Player::CanInteractWithNPCs(bool alive) const
{
    if(alive && !isAlive())
        return false;
    if(isInFlight())
        return false;

    return true;
}

bool Player::IsUnderWater() const
{
    return IsInWater() &&
        GetPositionZ() < (MapManager::Instance().GetBaseMap(GetMapId())->GetWaterLevel(GetPositionX(),GetPositionY())-2);
}

void Player::SetInWater(bool apply)
{
    if(m_isInWater==apply)
        return;

    //define player in water by opcodes
    //move player's guid into HateOfflineList of those mobs
    //which can't swim and move guid back into ThreatList when
    //on surface.
    //TODO: exist also swimming mobs, and function must be symmetric to enter/leave water
    m_isInWater = apply;

    // remove auras that need water/land
    RemoveAurasWithInterruptFlags(apply ? AURA_INTERRUPT_FLAG_NOT_ABOVEWATER : AURA_INTERRUPT_FLAG_NOT_UNDERWATER);

    getHostilRefManager().updateThreatTables();
}

void Player::SetGameMaster(bool on)
{
    if(on)
    {
        m_ExtraFlags |= PLAYER_EXTRA_GM_ON;
        setFaction(35);
        SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_GM);

        RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_FFA_PVP);
        ResetContestedPvP();

        getHostilRefManager().setOnlineOfflineState(false);
        CombatStop();
    }
    else
    {
        m_ExtraFlags &= ~ PLAYER_EXTRA_GM_ON;
        setFactionForRace(getRace());
        RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_GM);

        // restore FFA PvP Server state
        if(sWorld.IsFFAPvPRealm())
            SetFlag(PLAYER_FLAGS,PLAYER_FLAGS_FFA_PVP);

        // restore FFA PvP area state, remove not allowed for GM mounts
        UpdateArea(m_areaUpdateId);

        getHostilRefManager().setOnlineOfflineState(true);
    }

    //ObjectAccessor::UpdateVisibilityForPlayer(this);
    SetToNotify();
}

void Player::SetGMVisible(bool on)
{
    uint32 transparence_spell;
    if (GetSession()->GetSecurity() == SEC_MODERATOR)
        transparence_spell = 37801; //Transparency 25%
    else
        transparence_spell = 37800; //Transparency 50%


    if(on)
    {
        m_ExtraFlags &= ~PLAYER_EXTRA_GM_INVISIBLE;         //remove flag

        // Reapply stealth/invisibility if active or show if not any
        if(HasAuraType(SPELL_AURA_MOD_STEALTH))
            SetVisibility(VISIBILITY_GROUP_STEALTH);
        //else if(HasAuraType(SPELL_AURA_MOD_INVISIBILITY))
        //    SetVisibility(VISIBILITY_GROUP_INVISIBILITY);
        else
            SetVisibility(VISIBILITY_ON);

        RemoveAurasDueToSpell(transparence_spell);
    }
    else
    {
        m_ExtraFlags |= PLAYER_EXTRA_GM_INVISIBLE;          //add flag

        SetAcceptWhispers(false);
        SetGameMaster(true);

        SetVisibility(VISIBILITY_OFF);

        SpellEntry const* spellproto = spellmgr.LookupSpell(transparence_spell); //Transparency 50%
        if (spellproto)
        {
            Aura* aura = CreateAura(spellproto, 0, NULL, this, NULL);
            if (aura)
            {
                aura->SetDeathPersistent(true);
                aura->SetNegative();
                AddAura(aura);
            }
        }
    }
}

bool Player::IsGroupVisibleFor(Player* p) const
{
    switch(sWorld.getConfig(CONFIG_GROUP_VISIBILITY))
    {
        default: return IsInSameGroupWith(p);
        case 1:  return IsInSameRaidWith(p);
        case 2:  return GetTeam()==p->GetTeam();
    }
}

bool Player::IsInSameGroupWith(Player const* p) const
{
    return  p==this || GetGroup() != NULL &&
        GetGroup() == p->GetGroup() &&
        GetGroup()->SameSubGroup(this->ToPlayer(), p->ToPlayer());
}

///- If the player is invited, remove him. If the group if then only 1 person, disband the group.
/// \todo Shouldn't we also check if there is no other invitees before disbanding the group?
void Player::UninviteFromGroup()
{
    Group* group = GetGroupInvite();
    if(!group)
        return;

    group->RemoveInvite(this);

    if(group->GetMembersCount() <= 1)                   // group has just 1 member => disband
    {
        if(group->IsCreated())
        {
            group->Disband(true);
            objmgr.RemoveGroup(group);
        }
        else
            group->RemoveAllInvites();

        delete group;
    }
}

void Player::RemoveFromGroup(Group* group, uint64 guid)
{
    if(group)
    {
        if (group->RemoveMember(guid, 0) <= 1)
        {
            // group->Disband(); already disbanded in RemoveMember
            objmgr.RemoveGroup(group);
            delete group;
            // removemember sets the player's group pointer to NULL
        }
    }
}

void Player::SendLogXPGain(uint32 GivenXP, Unit* victim, uint32 RestXP)
{
    WorldPacket data(SMSG_LOG_XPGAIN, 21);
    data << uint64(victim ? victim->GetGUID() : 0);         // guid
    data << uint32(GivenXP+RestXP);                         // given experience
    data << uint8(victim ? 0 : 1);                          // 00-kill_xp type, 01-non_kill_xp type
    if(victim)
    {
        data << uint32(GivenXP);                            // experience without rested bonus
        data << float(1);                                   // 1 - none 0 - 100% group bonus output
    }
    data << uint8(0);                                       // new 2.4.0
    GetSession()->SendPacket(&data);
}

void Player::GiveXP(uint32 xp, Unit* victim)
{
    if ( xp < 1 )
        return;

    if(!isAlive())
        return;
        
    // Experience Blocking
    if (m_isXpBlocked)
        return;

    uint32 level = getLevel();

    // XP to money conversion processed in Player::RewardQuest
    if(level >= sWorld.getConfig(CONFIG_MAX_PLAYER_LEVEL))
        return;

    // handle SPELL_AURA_MOD_XP_PCT auras
    Unit::AuraList const& ModXPPctAuras = GetAurasByType(SPELL_AURA_MOD_XP_PCT);
    for(Unit::AuraList::const_iterator i = ModXPPctAuras.begin();i != ModXPPctAuras.end(); ++i)
        xp = uint32(xp*(1.0f + (*i)->GetModifierValue() / 100.0f));
        
    Unit::AuraList const& DummyAuras = GetAurasByType(SPELL_AURA_DUMMY);
    for(Unit::AuraList::const_iterator i = DummyAuras.begin();i != DummyAuras.end(); ++i)
        if((*i)->GetId() == 32098 || (*i)->GetId() == 32096)
        {
           uint32 area_id = GetAreaId();
           if(area_id == 3483 || area_id == 3535 || area_id == 3562 || area_id == 3713)
                   xp = uint32(xp*(1.0f + 5.0f / 100.0f));
        }


    // XP resting bonus for kill
    uint32 rested_bonus_xp = victim ? GetXPRestBonus(xp) : 0;

    SendLogXPGain(xp,victim,rested_bonus_xp);

    uint32 curXP = GetUInt32Value(PLAYER_XP);
    uint32 nextLvlXP = GetUInt32Value(PLAYER_NEXT_LEVEL_XP);
    uint32 newXP = curXP + xp + rested_bonus_xp;

    while( newXP >= nextLvlXP && level < sWorld.getConfig(CONFIG_MAX_PLAYER_LEVEL) )
    {
        newXP -= nextLvlXP;

        if ( level < sWorld.getConfig(CONFIG_MAX_PLAYER_LEVEL) )
            GiveLevel(level + 1);

        level = getLevel();
        nextLvlXP = GetUInt32Value(PLAYER_NEXT_LEVEL_XP);
    }

    SetUInt32Value(PLAYER_XP, newXP);
}

// Update player to next level
// Current player experience not update (must be update by caller)
void Player::GiveLevel(uint32 level)
{
    if ( level == getLevel() )
        return;

    PlayerLevelInfo info;
    objmgr.GetPlayerLevelInfo(getRace(),getClass(),level,&info);

    PlayerClassLevelInfo classInfo;
    objmgr.GetPlayerClassLevelInfo(getClass(),level,&classInfo);

    // send levelup info to client
    WorldPacket data(SMSG_LEVELUP_INFO, (4+4+MAX_POWERS*4+MAX_STATS*4));
    data << uint32(level);
    data << uint32(int32(classInfo.basehealth) - int32(GetCreateHealth()));
    // for(int i = 0; i < MAX_POWERS; ++i)                  // Powers loop (0-6)
    data << uint32(int32(classInfo.basemana)   - int32(GetCreateMana()));
    data << uint32(0);
    data << uint32(0);
    data << uint32(0);
    data << uint32(0);
    // end for
    for(int i = STAT_STRENGTH; i < MAX_STATS; ++i)          // Stats loop (0-4)
        data << uint32(int32(info.stats[i]) - GetCreateStat(Stats(i)));

    GetSession()->SendPacket(&data);

    SetUInt32Value(PLAYER_NEXT_LEVEL_XP, Trinity::XP::xp_to_level(level));

    //update level, max level of skills
    if(getLevel()!= level)
        m_Played_time[1] = 0;                               // Level Played Time reset
    SetLevel(level);
    UpdateSkillsForLevel ();

    // save base values (bonuses already included in stored stats
    for(int i = STAT_STRENGTH; i < MAX_STATS; ++i)
        SetCreateStat(Stats(i), info.stats[i]);

    SetCreateHealth(classInfo.basehealth);
    SetCreateMana(classInfo.basemana);

    InitTalentForLevel();
    InitTaxiNodesForLevel();

    UpdateAllStats();

    if(sWorld.getConfig(CONFIG_ALWAYS_MAXSKILL)) // Max weapon skill when leveling up
        UpdateSkillsToMaxSkillsForLevel();

    // set current level health and mana/energy to maximum after applying all mods.
    SetHealth(GetMaxHealth());
    SetPower(POWER_MANA, GetMaxPower(POWER_MANA));
    SetPower(POWER_ENERGY, GetMaxPower(POWER_ENERGY));
    if(GetPower(POWER_RAGE) > GetMaxPower(POWER_RAGE))
        SetPower(POWER_RAGE, GetMaxPower(POWER_RAGE));
    SetPower(POWER_FOCUS, 0);
    SetPower(POWER_HAPPINESS, 0);

    // give level to summoned pet
    Pet* pet = GetPet();
    if(pet && pet->getPetType()==SUMMON_PET)
        pet->GivePetLevel(level);
}

void Player::InitTalentForLevel()
{
    uint32 level = getLevel();
    // talents base at level diff ( talents = level - 9 but some can be used already)
    if(level < 10)
    {
        // Remove all talent points
        if(m_usedTalentCount > 0)                           // Free any used talents
        {
            resetTalents(true);
            SetFreeTalentPoints(0);
        }
    }
    else
    {
        uint32 talentPointsForLevel = uint32((level-9)*sWorld.getRate(RATE_TALENT));
        // if used more that have then reset
        if(m_usedTalentCount > talentPointsForLevel)
        {
            if (GetSession()->GetSecurity() < SEC_ADMINISTRATOR)
                resetTalents(true);
            else
                SetFreeTalentPoints(0);
        }
        // else update amount of free points
        else
            SetFreeTalentPoints(talentPointsForLevel-m_usedTalentCount);
    }
}

void Player::InitStatsForLevel(bool reapplyMods)
{
    if(reapplyMods)                                         //reapply stats values only on .reset stats (level) command
        _RemoveAllStatBonuses();

    PlayerClassLevelInfo classInfo;
    objmgr.GetPlayerClassLevelInfo(getClass(),getLevel(),&classInfo);

    PlayerLevelInfo info;
    objmgr.GetPlayerLevelInfo(getRace(),getClass(),getLevel(),&info);

    SetUInt32Value(PLAYER_FIELD_MAX_LEVEL, sWorld.getConfig(CONFIG_MAX_PLAYER_LEVEL) );
    SetUInt32Value(PLAYER_NEXT_LEVEL_XP, Trinity::XP::xp_to_level(getLevel()));

    UpdateSkillsForLevel ();

    // set default cast time multiplier
    SetFloatValue(UNIT_MOD_CAST_SPEED, 1.0f);

    // reset size before reapply auras
    SetFloatValue(OBJECT_FIELD_SCALE_X,1.0f);

    // save base values (bonuses already included in stored stats
    for(int i = STAT_STRENGTH; i < MAX_STATS; ++i)
        SetCreateStat(Stats(i), info.stats[i]);

    for(int i = STAT_STRENGTH; i < MAX_STATS; ++i)
        SetStat(Stats(i), info.stats[i]);

    SetCreateHealth(classInfo.basehealth);

    //set create powers
    SetCreateMana(classInfo.basemana);

    SetArmor(int32(m_createStats[STAT_AGILITY]*2));

    InitStatBuffMods();

    //reset rating fields values
    for(uint16 index = PLAYER_FIELD_COMBAT_RATING_1; index < PLAYER_FIELD_COMBAT_RATING_1 + MAX_COMBAT_RATING; ++index)
        SetUInt32Value(index, 0);

    SetUInt32Value(PLAYER_FIELD_MOD_HEALING_DONE_POS,0);
    for (int i = 0; i < 7; i++)
    {
        SetUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_NEG+i, 0);
        SetUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS+i, 0);
        SetFloatValue(PLAYER_FIELD_MOD_DAMAGE_DONE_PCT+i, 1.00f);
    }

    //reset attack power, damage and attack speed fields
    SetFloatValue(UNIT_FIELD_BASEATTACKTIME, 2000.0f );
    SetFloatValue(UNIT_FIELD_BASEATTACKTIME + 1, 2000.0f ); // offhand attack time
    SetFloatValue(UNIT_FIELD_RANGEDATTACKTIME, 2000.0f );

    SetFloatValue(UNIT_FIELD_MINDAMAGE, 0.0f );
    SetFloatValue(UNIT_FIELD_MAXDAMAGE, 0.0f );
    SetFloatValue(UNIT_FIELD_MINOFFHANDDAMAGE, 0.0f );
    SetFloatValue(UNIT_FIELD_MAXOFFHANDDAMAGE, 0.0f );
    SetFloatValue(UNIT_FIELD_MINRANGEDDAMAGE, 0.0f );
    SetFloatValue(UNIT_FIELD_MAXRANGEDDAMAGE, 0.0f );

    SetUInt32Value(UNIT_FIELD_ATTACK_POWER,            0 );
    SetUInt32Value(UNIT_FIELD_ATTACK_POWER_MODS,       0 );
    SetFloatValue(UNIT_FIELD_ATTACK_POWER_MULTIPLIER,0.0f);
    SetUInt32Value(UNIT_FIELD_RANGED_ATTACK_POWER,     0 );
    SetUInt32Value(UNIT_FIELD_RANGED_ATTACK_POWER_MODS,0 );
    SetFloatValue(UNIT_FIELD_RANGED_ATTACK_POWER_MULTIPLIER,0.0f);

    // Base crit values (will be recalculated in UpdateAllStats() at loading and in _ApplyAllStatBonuses() at reset
    SetFloatValue(PLAYER_CRIT_PERCENTAGE,0.0f);
    SetFloatValue(PLAYER_OFFHAND_CRIT_PERCENTAGE,0.0f);
    SetFloatValue(PLAYER_RANGED_CRIT_PERCENTAGE,0.0f);

    // Init spell schools (will be recalculated in UpdateAllStats() at loading and in _ApplyAllStatBonuses() at reset
    for (uint8 i = 0; i < 7; ++i)
        SetFloatValue(PLAYER_SPELL_CRIT_PERCENTAGE1+i, 0.0f);

    SetFloatValue(PLAYER_PARRY_PERCENTAGE, 0.0f);
    SetFloatValue(PLAYER_BLOCK_PERCENTAGE, 0.0f);
    SetUInt32Value(PLAYER_SHIELD_BLOCK, 0);

    // Dodge percentage
    SetFloatValue(PLAYER_DODGE_PERCENTAGE, 0.0f);

    // set armor (resistance 0) to original value (create_agility*2)
    SetArmor(int32(m_createStats[STAT_AGILITY]*2));
    SetResistanceBuffMods(SpellSchools(0), true, 0.0f);
    SetResistanceBuffMods(SpellSchools(0), false, 0.0f);
    // set other resistance to original value (0)
    for (int i = 1; i < MAX_SPELL_SCHOOL; i++)
    {
        SetResistance(SpellSchools(i), 0);
        SetResistanceBuffMods(SpellSchools(i), true, 0.0f);
        SetResistanceBuffMods(SpellSchools(i), false, 0.0f);
    }

    SetUInt32Value(PLAYER_FIELD_MOD_TARGET_RESISTANCE,0);
    SetUInt32Value(PLAYER_FIELD_MOD_TARGET_PHYSICAL_RESISTANCE,0);
    for(int i = 0; i < MAX_SPELL_SCHOOL; ++i)
    {
        SetFloatValue(UNIT_FIELD_POWER_COST_MODIFIER+i,0.0f);
        SetFloatValue(UNIT_FIELD_POWER_COST_MULTIPLIER+i,0.0f);
    }
    // Init data for form but skip reapply item mods for form
    InitDataForForm(reapplyMods);

    // save new stats
    for (int i = POWER_MANA; i < MAX_POWERS; i++)
        SetMaxPower(Powers(i),  uint32(GetCreatePowers(Powers(i))));

    SetMaxHealth(classInfo.basehealth);                     // stamina bonus will applied later

    // cleanup mounted state (it will set correctly at aura loading if player saved at mount.
    SetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID, 0);

    // cleanup unit flags (will be re-applied if need at aura load).
    RemoveFlag( UNIT_FIELD_FLAGS,
        UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_DISABLE_MOVE | UNIT_FLAG_NOT_ATTACKABLE_1 |
        UNIT_FLAG_PET_IN_COMBAT  | UNIT_FLAG_SILENCED     | UNIT_FLAG_PACIFIED         |
        UNIT_FLAG_DISABLE_ROTATE | UNIT_FLAG_IN_COMBAT    | UNIT_FLAG_DISARMED         |
        UNIT_FLAG_CONFUSED       | UNIT_FLAG_FLEEING      | UNIT_FLAG_NOT_SELECTABLE   |
        UNIT_FLAG_SKINNABLE      | UNIT_FLAG_MOUNT        | UNIT_FLAG_TAXI_FLIGHT      );
    SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE );   // must be set

    // cleanup player flags (will be re-applied if need at aura load), to avoid have ghost flag without ghost aura, for example.
    RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_AFK | PLAYER_FLAGS_DND | PLAYER_FLAGS_GM | PLAYER_FLAGS_GHOST | PLAYER_FLAGS_FFA_PVP);

    SetByteValue(UNIT_FIELD_BYTES_1, 2, 0x00);              // one form stealth modified bytes

    // restore if need some important flags
    SetUInt32Value(PLAYER_FIELD_BYTES2, 0 );                // flags empty by default

    if(reapplyMods)                                         //reapply stats values only on .reset stats (level) command
        _ApplyAllStatBonuses();

    // set current level health and mana/energy to maximum after applying all mods.
    SetHealth(GetMaxHealth());
    SetPower(POWER_MANA, GetMaxPower(POWER_MANA));
    SetPower(POWER_ENERGY, GetMaxPower(POWER_ENERGY));
    if(GetPower(POWER_RAGE) > GetMaxPower(POWER_RAGE))
        SetPower(POWER_RAGE, GetMaxPower(POWER_RAGE));
    SetPower(POWER_FOCUS, 0);
    SetPower(POWER_HAPPINESS, 0);
}

void Player::SendInitialSpells()
{
    uint16 spellCount = 0;

    WorldPacket data(SMSG_INITIAL_SPELLS, (1+2+4*m_spells.size()+2+m_spellCooldowns.size()*(2+2+2+4+4)));
    data << uint8(0);

    size_t countPos = data.wpos();
    data << uint16(spellCount);                             // spell count placeholder

    for (PlayerSpellMap::const_iterator itr = m_spells.begin(); itr != m_spells.end(); ++itr)
    {
        if(itr->second->state == PLAYERSPELL_REMOVED)
            continue;

        if(!itr->second->active || itr->second->disabled)
            continue;

        data << uint16(itr->first);
        data << uint16(0);                                  // it's not slot id

        spellCount +=1;
    }

    data.put<uint16>(countPos,spellCount);                  // write real count value

    uint16 spellCooldowns = m_spellCooldowns.size();
    data << uint16(spellCooldowns);
    for(SpellCooldowns::const_iterator itr=m_spellCooldowns.begin(); itr!=m_spellCooldowns.end(); ++itr)
    {
        SpellEntry const *sEntry = spellmgr.LookupSpell(itr->first);
        if(!sEntry)
            continue;

        data << uint16(itr->first);

        time_t cooldown = 0;
        time_t curTime = time(NULL);
        if(itr->second.end > curTime)
            cooldown = (itr->second.end-curTime)*1000;

        data << uint16(itr->second.itemid);                 // cast item id
        data << uint16(sEntry->Category);                   // spell category
        if(sEntry->Category)                                // may be wrong, but anyway better than nothing...
        {
            data << uint32(0);                              // cooldown
            data << uint32(cooldown);                       // category cooldown
        }
        else
        {
            data << uint32(cooldown);                       // cooldown
            data << uint32(0);                              // category cooldown
        }
    }

    GetSession()->SendPacket(&data);

    sLog.outDetail( "CHARACTER: Sent Initial Spells" );
}

void Player::RemoveMail(uint32 id)
{
    for(PlayerMails::iterator itr = m_mail.begin(); itr != m_mail.end();++itr)
    {
        if ((*itr)->messageID == id)
        {
            //do not delete item, because Player::removeMail() is called when returning mail to sender.
            m_mail.erase(itr);
            return;
        }
    }
}

void Player::SendMailResult(uint32 mailId, uint32 mailAction, uint32 mailError, uint32 equipError, uint32 item_guid, uint32 item_count)
{
    WorldPacket data(SMSG_SEND_MAIL_RESULT, (4+4+4+(mailError == MAIL_ERR_BAG_FULL?4:(mailAction == MAIL_ITEM_TAKEN?4+4:0))));
    data << (uint32) mailId;
    data << (uint32) mailAction;
    data << (uint32) mailError;
    if ( mailError == MAIL_ERR_BAG_FULL )
        data << (uint32) equipError;
    else if( mailAction == MAIL_ITEM_TAKEN )
    {
        data << (uint32) item_guid;                         // item guid low?
        data << (uint32) item_count;                        // item count?
    }
    GetSession()->SendPacket(&data);
}

void Player::SendNewMail()
{
    // deliver undelivered mail
    WorldPacket data(SMSG_RECEIVED_MAIL, 4);
    data << (uint32) 0;
    GetSession()->SendPacket(&data);
}

void Player::UpdateNextMailTimeAndUnreads()
{
    // calculate next delivery time (min. from non-delivered mails
    // and recalculate unReadMail
    time_t cTime = time(NULL);
    m_nextMailDelivereTime = 0;
    unReadMails = 0;
    for(PlayerMails::iterator itr = m_mail.begin(); itr != m_mail.end(); ++itr)
    {
        if((*itr)->deliver_time > cTime)
        {
            if(!m_nextMailDelivereTime || m_nextMailDelivereTime > (*itr)->deliver_time)
                m_nextMailDelivereTime = (*itr)->deliver_time;
        }
        else if(((*itr)->checked & MAIL_CHECK_MASK_READ) == 0)
            ++unReadMails;
    }
}

void Player::AddNewMailDeliverTime(time_t deliver_time)
{
    if(deliver_time <= time(NULL))                          // ready now
    {
        ++unReadMails;
        SendNewMail();
    }
    else                                                    // not ready and no have ready mails
    {
        if(!m_nextMailDelivereTime || m_nextMailDelivereTime > deliver_time)
            m_nextMailDelivereTime =  deliver_time;
    }
}

bool Player::addSpell(uint32 spell_id, bool active, bool learning, bool loading, uint16 slot_id, bool disabled)
{
    SpellEntry const *spellInfo = spellmgr.LookupSpell(spell_id);
    if (!spellInfo)
    {
        // do character spell book cleanup (all characters)
        if(loading && !learning)                            // spell load case
        {
            sLog.outError("Player::addSpell: Non-existed in SpellStore spell #%u request, deleting for all characters in `character_spell`.",spell_id);
            CharacterDatabase.PExecute("DELETE FROM character_spell WHERE spell = '%u'",spell_id);
        }
        else
            sLog.outError("Player::addSpell: Non-existed in SpellStore spell #%u request.",spell_id);

        return false;
    }

    if(!SpellMgr::IsSpellValid(spellInfo,this,false))
    {
        // do character spell book cleanup (all characters)
        if(loading && !learning)                            // spell load case
        {
            sLog.outError("Player::addSpell: Broken spell #%u learning not allowed, deleting for all characters in `character_spell`.",spell_id);
            CharacterDatabase.PExecute("DELETE FROM character_spell WHERE spell = '%u'",spell_id);
        }
        else
            sLog.outError("Player::addSpell: Broken spell #%u learning not allowed.",spell_id);

        return false;
    }

    PlayerSpellState state = learning ? PLAYERSPELL_NEW : PLAYERSPELL_UNCHANGED;

    bool disabled_case = false;
    bool superceded_old = false;

    PlayerSpellMap::iterator itr = m_spells.find(spell_id);
    if (itr != m_spells.end())
    {
        // update active state for known spell
        if(itr->second->active != active && itr->second->state != PLAYERSPELL_REMOVED && !itr->second->disabled)
        {
            itr->second->active = active;

            // loading && !learning == explicitly load from DB and then exist in it already and set correctly
            if(loading && !learning)
                itr->second->state = PLAYERSPELL_UNCHANGED;
            else if(itr->second->state != PLAYERSPELL_NEW)
                itr->second->state = PLAYERSPELL_CHANGED;

            if(!active)
            {
                WorldPacket data(SMSG_REMOVED_SPELL, 4);
                data << uint16(spell_id);
                GetSession()->SendPacket(&data);
            }
            return active;                                  // learn (show in spell book if active now)
        }

        if(itr->second->disabled != disabled && itr->second->state != PLAYERSPELL_REMOVED)
        {
            if(itr->second->state != PLAYERSPELL_NEW)
                itr->second->state = PLAYERSPELL_CHANGED;
            itr->second->disabled = disabled;

            if(disabled)
                return false;

            disabled_case = true;
        }
        else switch(itr->second->state)
        {
            case PLAYERSPELL_UNCHANGED:                     // known saved spell
                return false;
            case PLAYERSPELL_REMOVED:                       // re-learning removed not saved spell
            {
                delete itr->second;
                m_spells.erase(itr);
                state = PLAYERSPELL_CHANGED;
                break;                                      // need re-add
            }
            default:                                        // known not saved yet spell (new or modified)
            {
                // can be in case spell loading but learned at some previous spell loading
                if(loading && !learning)
                    itr->second->state = PLAYERSPELL_UNCHANGED;

                return false;
            }
        }
    }
    
    if (loading && !learning) {
        bool sendPacket = false;
        // TEMP HACK: Remove wrongly added racial spells after spell templates introduction
        switch (spell_id) {
        case 25046:
            if (m_class != CLASS_ROGUE) {
                disabled_case = true;
                sendPacket = true;
            }
            break;
        case 28730:
            if (m_class == CLASS_ROGUE) {
                disabled_case = true;
                sendPacket = true;
            }
            break;
        case 20575:
            if (m_class != CLASS_WARLOCK) {
                disabled_case = true;
                sendPacket = true;
            }
            break;
        case 20576:
            if (m_class != CLASS_HUNTER) {
                disabled_case = true;
                sendPacket = true;
            }
            break;
        case 21563:
            if (m_class == CLASS_WARLOCK || m_class == CLASS_HUNTER) {
                disabled_case = true;
                sendPacket = true;
            }
            break;
        case 13819:
            if (m_team == HORDE) {
                disabled_case = true;
                sendPacket = true;
            }
            break;
        case 33697:
            if (m_class != CLASS_SHAMAN) {
                disabled_case = true;
                sendPacket = true;
            }
            break;
        case 20572:
            if (m_class != CLASS_WARRIOR && m_class != CLASS_HUNTER && m_class != CLASS_ROGUE) {
                disabled_case = true;
                sendPacket = true;
            }
            break;
        case 33702:
            if (m_class != CLASS_MAGE && m_class != CLASS_WARLOCK) {
                disabled_case = true;
                sendPacket = true;
            }
            break;
        }
        
        if (sendPacket) {
            if (!active) {
                WorldPacket data(SMSG_REMOVED_SPELL, 4);
                data << uint16(spell_id);
                GetSession()->SendPacket(&data);
            }
            return false; 
        }
    }

    if(!disabled_case) // skip new spell adding if spell already known (disabled spells case)
    {
        // talent: unlearn all other talent ranks (high and low)
        if(TalentSpellPos const* talentPos = GetTalentSpellPos(spell_id))
        {
            if(TalentEntry const *talentInfo = sTalentStore.LookupEntry( talentPos->talent_id ))
            {
                for(int i=0; i <5; ++i)
                {
                    // skip learning spell and no rank spell case
                    uint32 rankSpellId = talentInfo->RankID[i];
                    if(!rankSpellId || rankSpellId==spell_id)
                        continue;

                    // skip unknown ranks
                    if(!HasSpell(rankSpellId))
                        continue;

                    removeSpell(rankSpellId);
                }
            }
        }
        // non talent spell: learn low ranks (recursive call)
        else if(uint32 prev_spell = spellmgr.GetPrevSpellInChain(spell_id))
        {
            if(loading)                                     // at spells loading, no output, but allow save
                addSpell(prev_spell,active,true,loading,SPELL_WITHOUT_SLOT_ID,disabled);
            else                                            // at normal learning
                learnSpell(prev_spell);
        }

        PlayerSpell *newspell = new PlayerSpell;
        newspell->active = active;
        newspell->state = state;
        newspell->disabled = disabled;

        // replace spells in action bars and spellbook to bigger rank if only one spell rank must be accessible
        if(newspell->active && !newspell->disabled && !SpellMgr::canStackSpellRanks(spellInfo) && spellmgr.GetSpellRank(spellInfo->Id) != 0)
        {
            for( PlayerSpellMap::iterator itr = m_spells.begin(); itr != m_spells.end(); ++itr )
            {
                if(itr->second->state == PLAYERSPELL_REMOVED) continue;
                SpellEntry const *i_spellInfo = spellmgr.LookupSpell(itr->first);
                if(!i_spellInfo) continue;

                if( spellmgr.IsRankSpellDueToSpell(spellInfo,itr->first) )
                {
                    if(itr->second->active)
                    {
                        if(spellmgr.IsHighRankOfSpell(spell_id,itr->first))
                        {
                            if(!loading)                    // not send spell (re-/over-)learn packets at loading
                            {
                                WorldPacket data(SMSG_SUPERCEDED_SPELL, (4));
                                data << uint16(itr->first);
                                data << uint16(spell_id);
                                GetSession()->SendPacket( &data );
                            }

                            // mark old spell as disable (SMSG_SUPERCEDED_SPELL replace it in client by new)
                            itr->second->active = false;
                            itr->second->state = PLAYERSPELL_CHANGED;
                            superceded_old = true;          // new spell replace old in action bars and spell book.
                        }
                        else if(spellmgr.IsHighRankOfSpell(itr->first,spell_id))
                        {
                            if(!loading)                    // not send spell (re-/over-)learn packets at loading
                            {
                                WorldPacket data(SMSG_SUPERCEDED_SPELL, (4));
                                data << uint16(spell_id);
                                data << uint16(itr->first);
                                GetSession()->SendPacket( &data );
                            }

                            // mark new spell as disable (not learned yet for client and will not learned)
                            newspell->active = false;
                            if(newspell->state != PLAYERSPELL_NEW)
                                newspell->state = PLAYERSPELL_CHANGED;
                        }
                    }
                }
            }
        }

        uint16 tmpslot=slot_id;

        if (tmpslot == SPELL_WITHOUT_SLOT_ID)
        {
            uint16 maxid = 0;
            PlayerSpellMap::iterator itr;
            for (itr = m_spells.begin(); itr != m_spells.end(); ++itr)
            {
                if(itr->second->state == PLAYERSPELL_REMOVED)
                    continue;
                if (itr->second->slotId > maxid)
                    maxid = itr->second->slotId;
            }
            tmpslot = maxid + 1;
        }

        newspell->slotId = tmpslot;
        m_spells[spell_id] = newspell;

        // return false if spell disabled
        if (newspell->disabled)
            return false;
    }

    uint32 talentCost = GetTalentSpellCost(spell_id);

    // cast talents with SPELL_EFFECT_LEARN_SPELL (other dependent spells will learned later as not auto-learned)
    // note: all spells with SPELL_EFFECT_LEARN_SPELL isn't passive
    if( talentCost > 0 && IsSpellHaveEffect(spellInfo,SPELL_EFFECT_LEARN_SPELL) )
    {
        // ignore stance requirement for talent learn spell (stance set for spell only for client spell description show)
        CastSpell(this, spell_id, true);
    }
    // also cast passive spells (including all talents without SPELL_EFFECT_LEARN_SPELL) with additional checks
    else if (IsPassiveSpell(spell_id))
    {
        // if spell doesn't require a stance or the player is in the required stance
        if( ( !spellInfo->Stances &&
            spell_id != 5420 && spell_id != 5419 && spell_id != 7376 &&
            spell_id != 7381 && spell_id != 21156 && spell_id != 21009 &&
            spell_id != 21178 && spell_id != 33948 && spell_id != 40121 ) ||
            m_form != 0 && (spellInfo->Stances & (1<<(m_form-1))) ||
            (spell_id ==  5420 && m_form == FORM_TREE) ||
            (spell_id ==  5419 && m_form == FORM_TRAVEL) ||
            (spell_id ==  7376 && m_form == FORM_DEFENSIVESTANCE) ||
            (spell_id ==  7381 && m_form == FORM_BERSERKERSTANCE) ||
            (spell_id == 21156 && m_form == FORM_BATTLESTANCE)||
            (spell_id == 21178 && (m_form == FORM_BEAR || m_form == FORM_DIREBEAR) ) ||
            (spell_id == 33948 && m_form == FORM_FLIGHT) ||
            (spell_id == 40121 && m_form == FORM_FLIGHT_EPIC) )
                                                            //Check CasterAuraStates
            if (!spellInfo->CasterAuraState || HasAuraState(AuraState(spellInfo->CasterAuraState)))
                CastSpell(this, spell_id, true);
    }
    else if( IsSpellHaveEffect(spellInfo,SPELL_EFFECT_SKILL_STEP) )
    {
        CastSpell(this, spell_id, true);
        return false;
    }

    // update used talent points count
    m_usedTalentCount += talentCost;

    // update free primary prof.points (if any, can be none in case GM .learn prof. learning)
    if(uint32 freeProfs = GetFreePrimaryProffesionPoints())
    {
        if(spellmgr.IsPrimaryProfessionFirstRankSpell(spell_id))
            SetFreePrimaryProffesions(freeProfs-1);
    }

    // add dependent skills
    uint16 maxskill     = GetMaxSkillValueForLevel();

    SpellLearnSkillNode const* spellLearnSkill = spellmgr.GetSpellLearnSkill(spell_id);

    if(spellLearnSkill)
    {
        uint32 skill_value = GetPureSkillValue(spellLearnSkill->skill);
        uint32 skill_max_value = GetPureMaxSkillValue(spellLearnSkill->skill);

        if(skill_value < spellLearnSkill->value)
            skill_value = spellLearnSkill->value;

        uint32 new_skill_max_value = spellLearnSkill->maxvalue == 0 ? maxskill : spellLearnSkill->maxvalue;

        if(skill_max_value < new_skill_max_value)
            skill_max_value =  new_skill_max_value;

        SetSkill(spellLearnSkill->skill,skill_value,skill_max_value);
    }
    else
    {
        // not ranked skills
        SkillLineAbilityMap::const_iterator lower = spellmgr.GetBeginSkillLineAbilityMap(spell_id);
        SkillLineAbilityMap::const_iterator upper = spellmgr.GetEndSkillLineAbilityMap(spell_id);

        for(SkillLineAbilityMap::const_iterator _spell_idx = lower; _spell_idx != upper; ++_spell_idx)
        {
            SkillLineEntry const *pSkill = sSkillLineStore.LookupEntry(_spell_idx->second->skillId);
            if(!pSkill)
                continue;

            if(HasSkill(pSkill->id))
                continue;

            if(_spell_idx->second->learnOnGetSkill == ABILITY_LEARNED_ON_GET_RACE_OR_CLASS_SKILL ||
                // poison special case, not have ABILITY_LEARNED_ON_GET_RACE_OR_CLASS_SKILL
                pSkill->id==SKILL_POISONS && _spell_idx->second->max_value==0 ||
                // lockpicking special case, not have ABILITY_LEARNED_ON_GET_RACE_OR_CLASS_SKILL
                pSkill->id==SKILL_LOCKPICKING && _spell_idx->second->max_value==0 )
            {
                switch(GetSkillRangeType(pSkill,_spell_idx->second->racemask!=0))
                {
                    case SKILL_RANGE_LANGUAGE:
                        SetSkill(pSkill->id, 300, 300 );
                        break;
                    case SKILL_RANGE_LEVEL:
                        SetSkill(pSkill->id, 1, GetMaxSkillValueForLevel() );
                        break;
                    case SKILL_RANGE_MONO:
                        SetSkill(pSkill->id, 1, 1 );
                        break;
                    default:
                        break;
                }
            }
        }
    }

    // learn dependent spells
    SpellLearnSpellMap::const_iterator spell_begin = spellmgr.GetBeginSpellLearnSpell(spell_id);
    SpellLearnSpellMap::const_iterator spell_end   = spellmgr.GetEndSpellLearnSpell(spell_id);

    for(SpellLearnSpellMap::const_iterator itr = spell_begin; itr != spell_end; ++itr)
    {
        if(!itr->second.autoLearned)
        {
            if(loading)                                     // at spells loading, no output, but allow save
                addSpell(itr->second.spell,true,true,loading);
            else                                            // at normal learning
                learnSpell(itr->second.spell);
        }
    }

    // return true (for send learn packet) only if spell active (in case ranked spells) and not replace old spell
    return active && !disabled && !superceded_old;
}

void Player::learnSpell(uint32 spell_id)
{
    PlayerSpellMap::iterator itr = m_spells.find(spell_id);

    bool disabled = (itr != m_spells.end()) ? itr->second->disabled : false;
    bool active = disabled ? itr->second->active : true;

    bool learning = addSpell(spell_id,active);

    // learn all disabled higher ranks (recursive)
    SpellChainNode const* node = spellmgr.GetSpellChainNode(spell_id);
    if (node)
    {
        PlayerSpellMap::iterator iter = m_spells.find(node->next);
        if (disabled && iter != m_spells.end() && iter->second->disabled )
            learnSpell(node->next);
    }

    // prevent duplicated entires in spell book
    if(!learning)
        return;

    WorldPacket data(SMSG_LEARNED_SPELL, 4);
    data << uint32(spell_id);
    GetSession()->SendPacket(&data);
}

void Player::removeSpell(uint32 spell_id, bool disabled)
{
    PlayerSpellMap::iterator itr = m_spells.find(spell_id);
    if (itr == m_spells.end())
        return;

    if(itr->second->state == PLAYERSPELL_REMOVED || disabled && itr->second->disabled)
        return;

    // unlearn non talent higher ranks (recursive)
    SpellChainNode const* node = spellmgr.GetSpellChainNode(spell_id);
    if (node)
    {
        if(HasSpell(node->next) && !GetTalentSpellPos(node->next))
        removeSpell(node->next,disabled);
    }
    //unlearn spells dependent from recently removed spells
    SpellsRequiringSpellMap const& reqMap = spellmgr.GetSpellsRequiringSpell();
    SpellsRequiringSpellMap::const_iterator itr2 = reqMap.find(spell_id);
    for (uint32 i=reqMap.count(spell_id);i>0;i--,itr2++)
        removeSpell(itr2->second,disabled);

    // removing
    WorldPacket data(SMSG_REMOVED_SPELL, 4);
    data << uint16(spell_id);
    GetSession()->SendPacket(&data);

    if (disabled)
    {
        itr->second->disabled = disabled;
        if(itr->second->state != PLAYERSPELL_NEW)
            itr->second->state = PLAYERSPELL_CHANGED;
    }
    else
    {
        if(itr->second->state == PLAYERSPELL_NEW)
        {
            delete itr->second;
            m_spells.erase(itr);
        }
        else
            itr->second->state = PLAYERSPELL_REMOVED;
    }

    RemoveAurasDueToSpell(spell_id);

    // remove pet auras
    if(PetAura const* petSpell = spellmgr.GetPetAura(spell_id))
        RemovePetAura(petSpell);

    // free talent points
    uint32 talentCosts = GetTalentSpellCost(spell_id);
    if(talentCosts > 0)
    {
        if(talentCosts < m_usedTalentCount)
            m_usedTalentCount -= talentCosts;
        else
            m_usedTalentCount = 0;
    }

    // update free primary prof.points (if not overflow setting, can be in case GM use before .learn prof. learning)
    if(spellmgr.IsPrimaryProfessionFirstRankSpell(spell_id))
    {
        uint32 freeProfs = GetFreePrimaryProffesionPoints()+1;
        if(freeProfs <= sWorld.getConfig(CONFIG_MAX_PRIMARY_TRADE_SKILL))
            SetFreePrimaryProffesions(freeProfs);
    }

    // remove dependent skill
    SpellLearnSkillNode const* spellLearnSkill = spellmgr.GetSpellLearnSkill(spell_id);
    if(spellLearnSkill)
    {
        uint32 prev_spell = spellmgr.GetPrevSpellInChain(spell_id);
        if(!prev_spell)                                     // first rank, remove skill
            SetSkill(spellLearnSkill->skill,0,0);
        else
        {
            // search prev. skill setting by spell ranks chain
            SpellLearnSkillNode const* prevSkill = spellmgr.GetSpellLearnSkill(prev_spell);
            while(!prevSkill && prev_spell)
            {
                prev_spell = spellmgr.GetPrevSpellInChain(prev_spell);
                prevSkill = spellmgr.GetSpellLearnSkill(spellmgr.GetFirstSpellInChain(prev_spell));
            }

            if(!prevSkill)                                  // not found prev skill setting, remove skill
                SetSkill(spellLearnSkill->skill,0,0);
            else                                            // set to prev. skill setting values
            {
                uint32 skill_value = GetPureSkillValue(prevSkill->skill);
                uint32 skill_max_value = GetPureMaxSkillValue(prevSkill->skill);

                if(skill_value >  prevSkill->value)
                    skill_value = prevSkill->value;

                uint32 new_skill_max_value = prevSkill->maxvalue == 0 ? GetMaxSkillValueForLevel() : prevSkill->maxvalue;

                if(skill_max_value > new_skill_max_value)
                    skill_max_value =  new_skill_max_value;

                SetSkill(prevSkill->skill,skill_value,skill_max_value);
            }
        }

    }
    else
    {
        // not ranked skills
        SkillLineAbilityMap::const_iterator lower = spellmgr.GetBeginSkillLineAbilityMap(spell_id);
        SkillLineAbilityMap::const_iterator upper = spellmgr.GetEndSkillLineAbilityMap(spell_id);

        for(SkillLineAbilityMap::const_iterator _spell_idx = lower; _spell_idx != upper; ++_spell_idx)
        {
            SkillLineEntry const *pSkill = sSkillLineStore.LookupEntry(_spell_idx->second->skillId);
            if(!pSkill)
                continue;

            if(_spell_idx->second->learnOnGetSkill == ABILITY_LEARNED_ON_GET_RACE_OR_CLASS_SKILL ||
                // poison special case, not have ABILITY_LEARNED_ON_GET_RACE_OR_CLASS_SKILL
                pSkill->id==SKILL_POISONS && _spell_idx->second->max_value==0 ||
                // lockpicking special case, not have ABILITY_LEARNED_ON_GET_RACE_OR_CLASS_SKILL
                pSkill->id==SKILL_LOCKPICKING && _spell_idx->second->max_value==0 )
            {
                // not reset skills for professions, class and racial abilities
                if( (pSkill->categoryId==SKILL_CATEGORY_SECONDARY || pSkill->categoryId==SKILL_CATEGORY_PROFESSION) &&
                    (IsProfessionSkill(pSkill->id) || _spell_idx->second->racemask!=0) )
                    continue;
                
                if (pSkill->categoryId == SKILL_CATEGORY_CLASS || pSkill->categoryId == SKILL_CATEGORY_WEAPON) // When do we need to reset this? I added this because it made faction-changed characters forget almost all their spells
                    continue;

                SetSkill(pSkill->id, 0, 0 );
            }
        }
    }

    // remove dependent spells
    SpellLearnSpellMap::const_iterator spell_begin = spellmgr.GetBeginSpellLearnSpell(spell_id);
    SpellLearnSpellMap::const_iterator spell_end   = spellmgr.GetEndSpellLearnSpell(spell_id);

    for(SpellLearnSpellMap::const_iterator itr2 = spell_begin; itr2 != spell_end; ++itr2)
        removeSpell(itr2->second.spell, disabled);
}

void Player::RemoveArenaSpellCooldowns()
{
    // remove cooldowns on spells that has < 15 min CD
    SpellCooldowns::iterator itr, next;
    // iterate spell cooldowns
    for(itr = m_spellCooldowns.begin();itr != m_spellCooldowns.end(); itr = next)
    {
        next = itr;
        ++next;
        SpellEntry const * entry = spellmgr.LookupSpell(itr->first);
        // check if spellentry is present and if the cooldown is less than 15 mins
        if( entry &&
            entry->RecoveryTime <= 15 * MINUTE * 1000 &&
            entry->CategoryRecoveryTime <= 15 * MINUTE * 1000 )
        {
            // notify player
            WorldPacket data(SMSG_CLEAR_COOLDOWN, (4+8));
            data << uint32(itr->first);
            data << GetGUID();
            GetSession()->SendPacket(&data);
            // remove cooldown
            m_spellCooldowns.erase(itr);
        }
    }
}

void Player::RemoveAllSpellCooldown()
{
    if(!m_spellCooldowns.empty())
    {
        for(SpellCooldowns::const_iterator itr = m_spellCooldowns.begin();itr != m_spellCooldowns.end(); ++itr)
        {
            WorldPacket data(SMSG_CLEAR_COOLDOWN, (4+8));
            data << uint32(itr->first);
            data << uint64(GetGUID());
            GetSession()->SendPacket(&data);
        }
        m_spellCooldowns.clear();
    }
}

void Player::_LoadSpellCooldowns(QueryResult *result)
{
    m_spellCooldowns.clear();

    //QueryResult *result = CharacterDatabase.PQuery("SELECT spell,item,time FROM character_spell_cooldown WHERE guid = '%u'",GetGUIDLow());

    if(result)
    {
        time_t curTime = time(NULL);

        do
        {
            Field *fields = result->Fetch();

            uint32 spell_id = fields[0].GetUInt32();
            uint32 item_id  = fields[1].GetUInt32();
            time_t db_time  = (time_t)fields[2].GetUInt64();

            if(!spellmgr.LookupSpell(spell_id))
            {
                sLog.outError("Player %u have unknown spell %u in `character_spell_cooldown`, skipping.",GetGUIDLow(),spell_id);
                continue;
            }

            // skip outdated cooldown
            if(db_time <= curTime)
                continue;

            AddSpellCooldown(spell_id, item_id, db_time);
        }
        while( result->NextRow() );

        delete result;
    }
}

void Player::_SaveSpellCooldowns(SQLTransaction trans)
{
    trans->PAppend("DELETE FROM character_spell_cooldown WHERE guid = '%u'", GetGUIDLow());

    time_t curTime = time(NULL);

    // remove outdated and save active
    for(SpellCooldowns::iterator itr = m_spellCooldowns.begin();itr != m_spellCooldowns.end();)
    {
        if(itr->second.end <= curTime)
            m_spellCooldowns.erase(itr++);
        else
        {
            trans->PAppend("INSERT INTO character_spell_cooldown (guid,spell,item,time) VALUES ('%u', '%u', '%u', '" I64FMTD "')", GetGUIDLow(), itr->first, itr->second.itemid, uint64(itr->second.end));
            ++itr;
        }
    }
}

uint32 Player::resetTalentsCost() const
{
    // The first time reset costs 1 gold
    if(m_resetTalentsCost < 1*GOLD)
        return 1*GOLD;
    // then 5 gold
    else if(m_resetTalentsCost < 5*GOLD)
        return 5*GOLD;
    // After that it increases in increments of 5 gold
    else if(m_resetTalentsCost < 10*GOLD)
        return 10*GOLD;
    else
    {
        uint32 months = (sWorld.GetGameTime() - m_resetTalentsTime)/MONTH;
        if(months > 0)
        {
            // This cost will be reduced by a rate of 5 gold per month
            int32 new_cost = int32(m_resetTalentsCost) - 5*GOLD*months;
            // to a minimum of 10 gold.
            return (new_cost < 10*GOLD ? 10*GOLD : new_cost);
        }
        else
        {
            // After that it increases in increments of 5 gold
            int32 new_cost = m_resetTalentsCost + 5*GOLD;
            // until it hits a cap of 50 gold.
            if(new_cost > 50*GOLD)
                new_cost = 50*GOLD;
            return new_cost;
        }
    }
}

bool Player::resetTalents(bool no_cost)
{
    // not need after this call
    if(HasAtLoginFlag(AT_LOGIN_RESET_TALENTS))
    {
        m_atLoginFlags = m_atLoginFlags & ~AT_LOGIN_RESET_TALENTS;
        CharacterDatabase.PExecute("UPDATE characters set at_login = at_login & ~ %u WHERE guid ='%u'", uint32(AT_LOGIN_RESET_TALENTS), GetGUIDLow());
    }

    uint32 level = getLevel();
    uint32 talentPointsForLevel = level < 10 ? 0 : uint32((level-9)*sWorld.getRate(RATE_TALENT));

    if (m_usedTalentCount == 0)
    {
        SetFreeTalentPoints(talentPointsForLevel);
        return false;
    }

    uint32 cost = 0;

    if(!no_cost && !sWorld.getConfig(CONFIG_NO_RESET_TALENT_COST))
    {
        cost = resetTalentsCost();

        if (GetMoney() < cost)
        {
            SendBuyError( BUY_ERR_NOT_ENOUGHT_MONEY, 0, 0, 0);
            return false;
        }
    }

    for (unsigned int i = 0; i < sTalentStore.GetNumRows(); i++)
    {
        TalentEntry const *talentInfo = sTalentStore.LookupEntry(i);

        if (!talentInfo) continue;

        TalentTabEntry const *talentTabInfo = sTalentTabStore.LookupEntry( talentInfo->TalentTab );

        if(!talentTabInfo)
            continue;

        // unlearn only talents for character class
        // some spell learned by one class as normal spells or know at creation but another class learn it as talent,
        // to prevent unexpected lost normal learned spell skip another class talents
        if( (getClassMask() & talentTabInfo->ClassMask) == 0 )
            continue;

        for (int j = 0; j < 5; j++)
        {
            for(PlayerSpellMap::iterator itr = GetSpellMap().begin(); itr != GetSpellMap().end();)
            {
                if(itr->second->state == PLAYERSPELL_REMOVED || itr->second->disabled)
                {
                    ++itr;
                    continue;
                }

                // remove learned spells (all ranks)
                uint32 itrFirstId = spellmgr.GetFirstSpellInChain(itr->first);

                // unlearn if first rank is talent or learned by talent
                if (itrFirstId == talentInfo->RankID[j] || spellmgr.IsSpellLearnToSpell(talentInfo->RankID[j],itrFirstId))
                {
                    removeSpell(itr->first,!IsPassiveSpell(itr->first));
                    itr = GetSpellMap().begin();
                    continue;
                }
                else
                    ++itr;
            }
        }
    }

    SetFreeTalentPoints(talentPointsForLevel);

    if(!no_cost)
    {
        ModifyMoney(-(int32)cost);

        m_resetTalentsCost = cost;
        m_resetTalentsTime = time(NULL);
    }

    //FIXME: remove pet before or after unlearn spells? for now after unlearn to allow removing of talent related, pet affecting auras
    RemovePet(NULL,PET_SAVE_NOT_IN_SLOT, true);

    return true;
}

bool Player::_removeSpell(uint16 spell_id)
{
    PlayerSpellMap::iterator itr = m_spells.find(spell_id);
    if (itr != m_spells.end())
    {
        delete itr->second;
        m_spells.erase(itr);
        return true;
    }
    return false;
}

Mail* Player::GetMail(uint32 id)
{
    for(PlayerMails::iterator itr = m_mail.begin(); itr != m_mail.end(); ++itr)
    {
        if ((*itr)->messageID == id)
        {
            return (*itr);
        }
    }
    return NULL;
}

void Player::_SetCreateBits(UpdateMask *updateMask, Player *target) const
{
    if(target == this)
    {
        Object::_SetCreateBits(updateMask, target);
    }
    else
    {
        for(uint16 index = 0; index < m_valuesCount; index++)
        {
            if(GetUInt32Value(index) != 0 && updateVisualBits.GetBit(index))
                updateMask->SetBit(index);
        }
    }
}

void Player::_SetUpdateBits(UpdateMask *updateMask, Player *target) const
{
    if(target == this)
    {
        Object::_SetUpdateBits(updateMask, target);
    }
    else
    {
        Object::_SetUpdateBits(updateMask, target);
        *updateMask &= updateVisualBits;
    }
}

void Player::InitVisibleBits()
{
    updateVisualBits.SetCount(PLAYER_END);

    // TODO: really implement OWNER_ONLY and GROUP_ONLY. Flags can be found in UpdateFields.h

    updateVisualBits.SetBit(OBJECT_FIELD_GUID);
    updateVisualBits.SetBit(OBJECT_FIELD_TYPE);
    updateVisualBits.SetBit(OBJECT_FIELD_SCALE_X);

    updateVisualBits.SetBit(UNIT_FIELD_CHARM);
    updateVisualBits.SetBit(UNIT_FIELD_CHARM+1);

    updateVisualBits.SetBit(UNIT_FIELD_SUMMON);
    updateVisualBits.SetBit(UNIT_FIELD_SUMMON+1);

    updateVisualBits.SetBit(UNIT_FIELD_CHARMEDBY);
    updateVisualBits.SetBit(UNIT_FIELD_CHARMEDBY+1);

    updateVisualBits.SetBit(UNIT_FIELD_TARGET);
    updateVisualBits.SetBit(UNIT_FIELD_TARGET+1);

    updateVisualBits.SetBit(UNIT_FIELD_CHANNEL_OBJECT);
    updateVisualBits.SetBit(UNIT_FIELD_CHANNEL_OBJECT+1);

    updateVisualBits.SetBit(UNIT_FIELD_HEALTH);
    updateVisualBits.SetBit(UNIT_FIELD_POWER1);
    updateVisualBits.SetBit(UNIT_FIELD_POWER2);
    updateVisualBits.SetBit(UNIT_FIELD_POWER3);
    updateVisualBits.SetBit(UNIT_FIELD_POWER4);
    updateVisualBits.SetBit(UNIT_FIELD_POWER5);

    updateVisualBits.SetBit(UNIT_FIELD_MAXHEALTH);
    updateVisualBits.SetBit(UNIT_FIELD_MAXPOWER1);
    updateVisualBits.SetBit(UNIT_FIELD_MAXPOWER2);
    updateVisualBits.SetBit(UNIT_FIELD_MAXPOWER3);
    updateVisualBits.SetBit(UNIT_FIELD_MAXPOWER4);
    updateVisualBits.SetBit(UNIT_FIELD_MAXPOWER5);

    updateVisualBits.SetBit(UNIT_FIELD_LEVEL);
    updateVisualBits.SetBit(UNIT_FIELD_FACTIONTEMPLATE);
    updateVisualBits.SetBit(UNIT_FIELD_BYTES_0);
    updateVisualBits.SetBit(UNIT_FIELD_FLAGS);
    updateVisualBits.SetBit(UNIT_FIELD_FLAGS_2);
    for(uint16 i = UNIT_FIELD_AURA; i < UNIT_FIELD_AURASTATE; ++i)
        updateVisualBits.SetBit(i);
    updateVisualBits.SetBit(UNIT_FIELD_AURASTATE);
    updateVisualBits.SetBit(UNIT_FIELD_BASEATTACKTIME);
    updateVisualBits.SetBit(UNIT_FIELD_BASEATTACKTIME + 1);
    updateVisualBits.SetBit(UNIT_FIELD_BOUNDINGRADIUS);
    updateVisualBits.SetBit(UNIT_FIELD_COMBATREACH);
    updateVisualBits.SetBit(UNIT_FIELD_DISPLAYID);
    updateVisualBits.SetBit(UNIT_FIELD_NATIVEDISPLAYID);
    updateVisualBits.SetBit(UNIT_FIELD_MOUNTDISPLAYID);
    updateVisualBits.SetBit(UNIT_FIELD_BYTES_1);
    updateVisualBits.SetBit(UNIT_FIELD_PETNUMBER);
    updateVisualBits.SetBit(UNIT_FIELD_PET_NAME_TIMESTAMP);
    updateVisualBits.SetBit(UNIT_DYNAMIC_FLAGS);
    updateVisualBits.SetBit(UNIT_CHANNEL_SPELL);
    updateVisualBits.SetBit(UNIT_MOD_CAST_SPEED);
    updateVisualBits.SetBit(UNIT_FIELD_BYTES_2);

    updateVisualBits.SetBit(PLAYER_DUEL_ARBITER);
    updateVisualBits.SetBit(PLAYER_DUEL_ARBITER+1);
    updateVisualBits.SetBit(PLAYER_FLAGS);
    updateVisualBits.SetBit(PLAYER_GUILDID);
    updateVisualBits.SetBit(PLAYER_GUILDRANK);
    updateVisualBits.SetBit(PLAYER_BYTES);
    updateVisualBits.SetBit(PLAYER_BYTES_2);
    updateVisualBits.SetBit(PLAYER_BYTES_3);
    updateVisualBits.SetBit(PLAYER_DUEL_TEAM);
    updateVisualBits.SetBit(PLAYER_GUILD_TIMESTAMP);

    // PLAYER_QUEST_LOG_x also visible bit on official (but only on party/raid)...
    for(uint16 i = PLAYER_QUEST_LOG_1_1; i < PLAYER_QUEST_LOG_25_2; i+=4)
        updateVisualBits.SetBit(i);

    //Players visible items are not inventory stuff
    //431) = 884 (0x374) = main weapon
    for(uint16 i = 0; i < EQUIPMENT_SLOT_END; i++)
    {
        // item creator
        updateVisualBits.SetBit(PLAYER_VISIBLE_ITEM_1_CREATOR + (i*MAX_VISIBLE_ITEM_OFFSET) + 0);
        updateVisualBits.SetBit(PLAYER_VISIBLE_ITEM_1_CREATOR + (i*MAX_VISIBLE_ITEM_OFFSET) + 1);

        uint16 visual_base = PLAYER_VISIBLE_ITEM_1_0 + (i*MAX_VISIBLE_ITEM_OFFSET);

        // item entry
        updateVisualBits.SetBit(visual_base + 0);

        // item enchantment IDs
        for(uint8 j = 0; j < MAX_INSPECTED_ENCHANTMENT_SLOT; ++j)
            updateVisualBits.SetBit(visual_base + 1 + j);

        // random properties
        updateVisualBits.SetBit(PLAYER_VISIBLE_ITEM_1_PROPERTIES + 0 + (i*MAX_VISIBLE_ITEM_OFFSET));
        updateVisualBits.SetBit(PLAYER_VISIBLE_ITEM_1_PROPERTIES + 1 + (i*MAX_VISIBLE_ITEM_OFFSET));
    }

    updateVisualBits.SetBit(PLAYER_CHOSEN_TITLE);
}

void Player::BuildCreateUpdateBlockForPlayer( UpdateData *data, Player *target ) const
{
    for(int i = 0; i < EQUIPMENT_SLOT_END; i++)
    {
        if(m_items[i] == NULL)
            continue;

        m_items[i]->BuildCreateUpdateBlockForPlayer( data, target );
    }

    if(target == this)
    {

        for(int i = INVENTORY_SLOT_BAG_START; i < BANK_SLOT_BAG_END; i++)
        {
            if(m_items[i] == NULL)
                continue;

            m_items[i]->BuildCreateUpdateBlockForPlayer( data, target );
        }
        for(int i = KEYRING_SLOT_START; i < KEYRING_SLOT_END; i++)
        {
            if(m_items[i] == NULL)
                continue;

            m_items[i]->BuildCreateUpdateBlockForPlayer( data, target );
        }
    }

    Unit::BuildCreateUpdateBlockForPlayer( data, target );
}

void Player::DestroyForPlayer( Player *target ) const
{
    Unit::DestroyForPlayer( target );

    for(int i = 0; i < INVENTORY_SLOT_BAG_END; i++)
    {
        if(m_items[i] == NULL)
            continue;

        m_items[i]->DestroyForPlayer( target );
    }

    if(target == this)
    {
        for(int i = INVENTORY_SLOT_BAG_START; i < BANK_SLOT_BAG_END; i++)
        {
            if(m_items[i] == NULL)
                continue;

            m_items[i]->DestroyForPlayer( target );
        }
        for(int i = KEYRING_SLOT_START; i < KEYRING_SLOT_END; i++)
        {
            if(m_items[i] == NULL)
                continue;

            m_items[i]->DestroyForPlayer( target );
        }
    }
}

bool Player::HasSpell(uint32 spell) const
{
    PlayerSpellMap::const_iterator itr = m_spells.find((uint16)spell);
    return (itr != m_spells.end() && itr->second->state != PLAYERSPELL_REMOVED && !itr->second->disabled);
}

bool Player::HasSpellButDisabled(uint32 spell) const
{
    PlayerSpellMap::const_iterator itr = m_spells.find((uint16)spell);
    return (itr != m_spells.end() && itr->second->state != PLAYERSPELL_REMOVED && itr->second->disabled);
}

TrainerSpellState Player::GetTrainerSpellState(TrainerSpell const* trainer_spell) const
{
    if (!trainer_spell)
        return TRAINER_SPELL_RED;

    if (!trainer_spell->spell)
        return TRAINER_SPELL_RED;

    // known spell
    if(HasSpell(trainer_spell->spell))
        return TRAINER_SPELL_GRAY;

    // check race/class requirement
    if(!IsSpellFitByClassAndRace(trainer_spell->spell))
        return TRAINER_SPELL_RED;

    // check level requirement
    if(getLevel() < trainer_spell->reqlevel)
        return TRAINER_SPELL_RED;

    if(SpellChainNode const* spell_chain = spellmgr.GetSpellChainNode(trainer_spell->spell))
    {
        // check prev.rank requirement
        if(spell_chain->prev && !HasSpell(spell_chain->prev))
            return TRAINER_SPELL_RED;
    }

    if(uint32 spell_req = spellmgr.GetSpellRequired(trainer_spell->spell))
    {
        // check additional spell requirement
        if(!HasSpell(spell_req))
            return TRAINER_SPELL_RED;
    }

    // check skill requirement
    if(trainer_spell->reqskill && GetBaseSkillValue(trainer_spell->reqskill) < trainer_spell->reqskillvalue)
        return TRAINER_SPELL_RED;

    // exist, already checked at loading
    SpellEntry const* spell = spellmgr.LookupSpell(trainer_spell->spell);

    // secondary prof. or not prof. spell
    uint32 skill = spell->EffectMiscValue[1];

    if(spell->Effect[1] != SPELL_EFFECT_SKILL || !IsPrimaryProfessionSkill(skill))
        return TRAINER_SPELL_GREEN;

    // check primary prof. limit
    if(spellmgr.IsPrimaryProfessionFirstRankSpell(spell->Id) && GetFreePrimaryProffesionPoints() == 0)
        return TRAINER_SPELL_RED;

    return TRAINER_SPELL_GREEN;
}

void Player::DeleteFromDB(uint64 playerguid, uint32 accountId, bool updateRealmChars)
{
    uint32 guid = GUID_LOPART(playerguid);

    // convert corpse to bones if exist (to prevent exiting Corpse in World without DB entry)
    // bones will be deleted by corpse/bones deleting thread shortly
    ObjectAccessor::Instance().ConvertCorpseForPlayer(playerguid);

    // remove from guild
    uint32 guildId = GetGuildIdFromDB(playerguid);
    if(guildId != 0)
    {
        Guild* guild = objmgr.GetGuildById(guildId);
        if(guild)
            guild->DelMember(guid);
    }

    // remove from arena teams
    uint32 at_id = GetArenaTeamIdFromDB(playerguid,ARENA_TEAM_2v2);
    if(at_id != 0)
    {
        ArenaTeam * at = objmgr.GetArenaTeamById(at_id);
        if(at)
            at->DelMember(playerguid);
    }
    at_id = GetArenaTeamIdFromDB(playerguid,ARENA_TEAM_3v3);
    if(at_id != 0)
    {
        ArenaTeam * at = objmgr.GetArenaTeamById(at_id);
        if(at)
            at->DelMember(playerguid);
    }
    at_id = GetArenaTeamIdFromDB(playerguid,ARENA_TEAM_5v5);
    if(at_id != 0)
    {
        ArenaTeam * at = objmgr.GetArenaTeamById(at_id);
        if(at)
            at->DelMember(playerguid);
    }

    // the player was uninvited already on logout so just remove from group
    QueryResult *resultGroup = CharacterDatabase.PQuery("SELECT leaderGuid FROM group_member WHERE memberGuid='%u'", guid);
    if(resultGroup)
    {
        uint64 leaderGuid = MAKE_NEW_GUID((*resultGroup)[0].GetUInt32(), 0, HIGHGUID_PLAYER);
        delete resultGroup;
        Group* group = objmgr.GetGroupByLeader(leaderGuid);
        if(group)
        {
            RemoveFromGroup(group, playerguid);
        }
    }

    // remove signs from petitions (also remove petitions if owner);
    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    RemovePetitionsAndSigns(playerguid, 10, trans);

    // return back all mails with COD and Item                 0  1              2      3       4          5     6
    QueryResult *resultMail = CharacterDatabase.PQuery("SELECT id,mailTemplateId,sender,subject,itemTextId,money,has_items FROM mail WHERE receiver='%u' AND has_items<>0 AND cod<>0", guid);
    if(resultMail)
    {
        do
        {
            Field *fields = resultMail->Fetch();

            uint32 mail_id       = fields[0].GetUInt32();
            uint16 mailTemplateId= fields[1].GetUInt16();
            uint32 sender        = fields[2].GetUInt32();
            std::string subject  = fields[3].GetCppString();
            uint32 itemTextId    = fields[4].GetUInt32();
            uint32 money         = fields[5].GetUInt32();
            bool has_items       = fields[6].GetBool();

            //we can return mail now
            //so firstly delete the old one
            trans->PAppend("DELETE FROM mail WHERE id = '%u'", mail_id);

            MailItemsInfo mi;
            if(has_items)
            {
                QueryResult *resultItems = CharacterDatabase.PQuery("SELECT item_guid,item_template FROM mail_items WHERE mail_id='%u'", mail_id);
                if(resultItems)
                {
                    do
                    {
                        Field *fields2 = resultItems->Fetch();

                        uint32 item_guidlow = fields2[0].GetUInt32();
                        uint32 item_template = fields2[1].GetUInt32();

                        ItemPrototype const* itemProto = objmgr.GetItemPrototype(item_template);
                        if(!itemProto)
                        {
                            trans->PAppend("DELETE FROM item_instance WHERE guid = '%u'", item_guidlow);
                            continue;
                        }

                        Item *pItem = NewItemOrBag(itemProto);
                        if(!pItem->LoadFromDB(item_guidlow, MAKE_NEW_GUID(guid, 0, HIGHGUID_PLAYER)))
                        {
                            pItem->FSetState(ITEM_REMOVED);
                            pItem->SaveToDB(trans);              // it also deletes item object !
                            continue;
                        }

                        mi.AddItem(item_guidlow, item_template, pItem);
                    }
                    while (resultItems->NextRow());

                    delete resultItems;
                }
            }

            trans->PAppend("DELETE FROM mail_items WHERE mail_id = '%u'", mail_id);

            uint32 pl_account = objmgr.GetPlayerAccountIdByGUID(MAKE_NEW_GUID(guid, 0, HIGHGUID_PLAYER));

            WorldSession::SendReturnToSender(MAIL_NORMAL, pl_account, guid, sender, subject, itemTextId, &mi, money, mailTemplateId);
        }
        while (resultMail->NextRow());

        delete resultMail;
    }

    // unsummon and delete for pets in world is not required: player deleted from CLI or character list with not loaded pet.
    // Get guids of character's pets, will deleted in transaction
    QueryResult *resultPets = CharacterDatabase.PQuery("SELECT id FROM character_pet WHERE owner = '%u'",guid);

    // NOW we can finally clear other DB data related to character
    if (resultPets)
    {
        do
        {
            Field *fields3 = resultPets->Fetch();
            uint32 petguidlow = fields3[0].GetUInt32();
            Pet::DeleteFromDB(petguidlow);
        } while (resultPets->NextRow());
        delete resultPets;
    }

    trans->PAppend("DELETE FROM characters WHERE guid = '%u'",guid);
    trans->PAppend("DELETE FROM character_declinedname WHERE guid = '%u'",guid);
    trans->PAppend("DELETE FROM character_action WHERE guid = '%u'",guid);
    trans->PAppend("DELETE FROM character_aura WHERE guid = '%u'",guid);
    trans->PAppend("DELETE FROM character_gifts WHERE guid = '%u'",guid);
    trans->PAppend("DELETE FROM character_homebind WHERE guid = '%u'",guid);
    trans->PAppend("DELETE FROM character_instance WHERE guid = '%u'",guid);
    trans->PAppend("DELETE FROM group_instance WHERE leaderGuid = '%u'",guid);
    trans->PAppend("DELETE FROM character_inventory WHERE guid = '%u'",guid);
    trans->PAppend("DELETE FROM character_queststatus WHERE guid = '%u'",guid);
    trans->PAppend("DELETE FROM character_reputation WHERE guid = '%u'",guid);
    trans->PAppend("DELETE FROM character_spell WHERE guid = '%u'",guid);
    trans->PAppend("DELETE FROM character_spell_cooldown WHERE guid = '%u'",guid);
    trans->PAppend("DELETE FROM gm_tickets WHERE playerGuid = '%u'", guid);
    trans->PAppend("DELETE FROM item_instance WHERE owner_guid = '%u'",guid);
    trans->PAppend("DELETE FROM character_social WHERE guid = '%u' OR friend='%u'",guid,guid);
    trans->PAppend("DELETE FROM mail WHERE receiver = '%u'",guid);
    trans->PAppend("DELETE FROM mail_items WHERE receiver = '%u'",guid);
    trans->PAppend("DELETE FROM character_pet WHERE owner = '%u'",guid);
    trans->PAppend("DELETE FROM character_pet_declinedname WHERE owner = '%u'",guid);
    trans->PAppend("DELETE FROM character_skills WHERE guid = '%u'",guid);
    CharacterDatabase.CommitTransaction(trans);

    //LoginDatabase.PExecute("UPDATE realmcharacters SET numchars = numchars - 1 WHERE acctid = %d AND realmid = %d", accountId, realmID);
    if(updateRealmChars) sWorld.UpdateRealmCharCount(accountId);
}

void Player::SetMovement(PlayerMovementType pType)
{
    WorldPacket data;
    switch(pType)
    {
        case MOVE_ROOT:       data.Initialize(SMSG_FORCE_MOVE_ROOT,   GetPackGUID().size()+4); break;
        case MOVE_UNROOT:     data.Initialize(SMSG_FORCE_MOVE_UNROOT, GetPackGUID().size()+4); break;
        case MOVE_WATER_WALK: data.Initialize(SMSG_MOVE_WATER_WALK,   GetPackGUID().size()+4); break;
        case MOVE_LAND_WALK:  data.Initialize(SMSG_MOVE_LAND_WALK,    GetPackGUID().size()+4); break;
        default:
            sLog.outError("Player::SetMovement: Unsupported move type (%d), data not sent to client.",pType);
            return;
    }
    data.append(GetPackGUID());
    data << uint32(0);
    GetSession()->SendPacket( &data );
}

/* Preconditions:
  - a resurrectable corpse must not be loaded for the player (only bones)
  - the player must be in world
*/
void Player::BuildPlayerRepop()
{
    if(getRace() == RACE_NIGHTELF)
        CastSpell(this, 20584, true);                       // auras SPELL_AURA_INCREASE_SPEED(+speed in wisp form), SPELL_AURA_INCREASE_SWIM_SPEED(+swim speed in wisp form), SPELL_AURA_TRANSFORM (to wisp form)
    CastSpell(this, 8326, true);                            // auras SPELL_AURA_GHOST, SPELL_AURA_INCREASE_SPEED(why?), SPELL_AURA_INCREASE_SWIM_SPEED(why?)

    // there must be SMSG.FORCE_RUN_SPEED_CHANGE, SMSG.FORCE_SWIM_SPEED_CHANGE, SMSG.MOVE_WATER_WALK
    // there must be SMSG.STOP_MIRROR_TIMER
    // there we must send 888 opcode

    // the player cannot have a corpse already, only bones which are not returned by GetCorpse
    if(GetCorpse())
    {
        sLog.outError("BuildPlayerRepop: player %s(%d) already has a corpse", GetName(), GetGUIDLow());
	return;
    }

    // create a corpse and place it at the player's location
    CreateCorpse();
    Corpse *corpse = GetCorpse();
    if(!corpse)
    {
        sLog.outError("Error creating corpse for Player %s [%u]", GetName(), GetGUIDLow());
        return;
    }
    GetMap()->Add(corpse);

    // convert player body to ghost
    SetHealth( 1 );

    SetMovement(MOVE_WATER_WALK);
    if(!GetSession()->isLogingOut())
        SetMovement(MOVE_UNROOT);

    // BG - remove insignia related
    RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SKINNABLE);

//    SendCorpseReclaimDelay();

    // to prevent cheating
    corpse->ResetGhostTime();

    StopMirrorTimers();                                     //disable timers(bars)

    SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS, (float)1.0);   //see radius of death player?

    SetByteValue(UNIT_FIELD_BYTES_1, 3, PLAYER_STATE_FLAG_ALWAYS_STAND);
}

void Player::SendDelayResponse(const uint32 ml_seconds)
{
    //FIXME: is this delay time arg really need? 50msec by default in code
    WorldPacket data( SMSG_QUERY_TIME_RESPONSE, 4+4 );
    data << (uint32)time(NULL);
    data << (uint32)0;
    GetSession()->SendPacket( &data );
}

void Player::ResurrectPlayer(float restore_percent, bool applySickness)
{
    WorldPacket data(SMSG_DEATH_RELEASE_LOC, 4*4);          // remove spirit healer position
    data << uint32(-1);
    data << float(0);
    data << float(0);
    data << float(0);
    GetSession()->SendPacket(&data);

    // send spectate addon message
    if (HaveSpectators())
    {
        SpectatorAddonMsg msg;
        msg.SetPlayer(GetName());
        msg.SetStatus(true);
        SendSpectatorAddonMsgToBG(msg);
    }

    // speed change, land walk

    // remove death flag + set aura
    SetByteValue(UNIT_FIELD_BYTES_1, 3, 0x00);
    if(getRace() == RACE_NIGHTELF)
        RemoveAurasDueToSpell(20584);                       // speed bonuses
    RemoveAurasDueToSpell(8326);                            // SPELL_AURA_GHOST

    setDeathState(ALIVE);

    SetMovement(MOVE_LAND_WALK);
    SetMovement(MOVE_UNROOT);

    m_deathTimer = 0;

    // set health/powers (0- will be set in caller)
    if(restore_percent>0.0f)
    {
        SetHealth(uint32(GetMaxHealth()*restore_percent));
        SetPower(POWER_MANA, uint32(GetMaxPower(POWER_MANA)*restore_percent));
        SetPower(POWER_RAGE, 0);
        SetPower(POWER_ENERGY, uint32(GetMaxPower(POWER_ENERGY)*restore_percent));
    }

    // update visibility
    //ObjectAccessor::UpdateVisibilityForPlayer(this);
    SetToNotify();

    // some items limited to specific map
    DestroyZoneLimitedItem( true, GetZoneId());

    if(!applySickness)
        return;

    //Characters from level 1-10 are not affected by resurrection sickness.
    //Characters from level 11-19 will suffer from one minute of sickness
    //for each level they are above 10.
    //Characters level 20 and up suffer from ten minutes of sickness.
    int32 startLevel = sWorld.getConfig(CONFIG_DEATH_SICKNESS_LEVEL);

    if(int32(getLevel()) >= startLevel)
    {
        // set resurrection sickness
        CastSpell(this,SPELL_ID_PASSIVE_RESURRECTION_SICKNESS,true);

        // not full duration
        if(int32(getLevel()) < startLevel+9)
        {
            int32 delta = (int32(getLevel()) - startLevel + 1)*MINUTE;

            for(int i =0; i < 3; ++i)
            {
                if(Aura* Aur = GetAura(SPELL_ID_PASSIVE_RESURRECTION_SICKNESS,i))
                {
                    Aur->SetAuraDuration(delta*1000);
                    Aur->UpdateAuraDuration();
                }
            }
        }
    }
    
    UpdateAreaDependentAuras(GetAreaId());
}

void Player::KillPlayer()
{
    SetMovement(MOVE_ROOT);

    StopMirrorTimers();                                     //disable timers(bars)

    setDeathState(CORPSE);
    //SetFlag( UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_IN_PVP );

    SetFlag(UNIT_DYNAMIC_FLAGS, 0x00);
    ApplyModFlag(PLAYER_FIELD_BYTES, PLAYER_FIELD_BYTE_RELEASE_TIMER, !sMapStore.LookupEntry(GetMapId())->Instanceable());

    // 6 minutes until repop at graveyard
    m_deathTimer = 6*MINUTE*1000;

	m_deathTime = time(NULL);

    UpdateCorpseReclaimDelay();                             // dependent at use SetDeathPvP() call before kill
    SendCorpseReclaimDelay();
    
    /* Sunwell/Kalecgos: death in spectral realm */
    if (GetMapId() == 580 && GetPositionZ() < -65)
		TeleportTo(GetMapId(), GetPositionX(), GetPositionY(), 53.079, GetOrientation());

    // don't create corpse at this moment, player might be falling

    // update visibility
    ObjectAccessor::UpdateObjectVisibility(this);
}

void Player::CreateCorpse()
{
    // prevent existence 2 corpse for player
    SpawnCorpseBones();

    uint32 _uf, _pb, _pb2, _cfb1, _cfb2;

    Corpse *corpse = new Corpse( (m_ExtraFlags & PLAYER_EXTRA_PVP_DEATH) ? CORPSE_RESURRECTABLE_PVP : CORPSE_RESURRECTABLE_PVE );
    SetPvPDeath(false);

    if(!corpse->Create(objmgr.GenerateLowGuid(HIGHGUID_CORPSE), this, GetMapId(), GetPositionX(),
        GetPositionY(), GetPositionZ(), GetOrientation()))
    {
        delete corpse;
        return;
    }

    _uf = GetUInt32Value(UNIT_FIELD_BYTES_0);
    _pb = GetUInt32Value(PLAYER_BYTES);
    _pb2 = GetUInt32Value(PLAYER_BYTES_2);

    uint8 race       = (uint8)(_uf);
    uint8 skin       = (uint8)(_pb);
    uint8 face       = (uint8)(_pb >> 8);
    uint8 hairstyle  = (uint8)(_pb >> 16);
    uint8 haircolor  = (uint8)(_pb >> 24);
    uint8 facialhair = (uint8)(_pb2);

    _cfb1 = ((0x00) | (race << 8) | (getGender() << 16) | (skin << 24));
    _cfb2 = ((face) | (hairstyle << 8) | (haircolor << 16) | (facialhair << 24));

    corpse->SetUInt32Value( CORPSE_FIELD_BYTES_1, _cfb1 );
    corpse->SetUInt32Value( CORPSE_FIELD_BYTES_2, _cfb2 );

    uint32 flags = CORPSE_FLAG_UNK2;
    if(HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_HIDE_HELM))
        flags |= CORPSE_FLAG_HIDE_HELM;
    if(HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_HIDE_CLOAK))
        flags |= CORPSE_FLAG_HIDE_CLOAK;
    if(InBattleGround() && !InArena())
        flags |= CORPSE_FLAG_LOOTABLE;                      // to be able to remove insignia
    corpse->SetUInt32Value( CORPSE_FIELD_FLAGS, flags );

    corpse->SetUInt32Value( CORPSE_FIELD_DISPLAY_ID, GetNativeDisplayId() );

    corpse->SetUInt32Value( CORPSE_FIELD_GUILD, GetGuildId() );

    uint32 iDisplayID;
    uint16 iIventoryType;
    uint32 _cfi;
    for (int i = 0; i < EQUIPMENT_SLOT_END; i++)
    {
        if(m_items[i])
        {
            iDisplayID = m_items[i]->GetProto()->DisplayInfoID;
            iIventoryType = (uint16)m_items[i]->GetProto()->InventoryType;

            _cfi =  (uint16(iDisplayID)) | (iIventoryType)<< 24;
            corpse->SetUInt32Value(CORPSE_FIELD_ITEM + i,_cfi);
        }
    }

    // we don't SaveToDB for players in battlegrounds so don't do it for corpses either
    const MapEntry *entry = sMapStore.LookupEntry(corpse->GetMapId());
    assert(entry);
    if(entry->map_type != MAP_BATTLEGROUND)
        corpse->SaveToDB();

    // register for player, but not show
    ObjectAccessor::Instance().AddCorpse(corpse);
}

void Player::SpawnCorpseBones()
{
    if(ObjectAccessor::Instance().ConvertCorpseForPlayer(GetGUID()))
        SaveToDB();                                         // prevent loading as ghost without corpse
}

Corpse* Player::GetCorpse() const
{
    return ObjectAccessor::Instance().GetCorpseForPlayerGUID(GetGUID());
}

void Player::DurabilityLossAll(double percent, bool inventory)
{
    for(int i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; i++)
        if(Item *pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i ))
            DurabilityLoss(pItem,percent);

    if(inventory)
    {
        // bags not have durability
        // for(int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; i++)

        for(int i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; i++)
            if(Item *pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i ))
                DurabilityLoss(pItem,percent);

        // keys not have durability
        //for(int i = KEYRING_SLOT_START; i < KEYRING_SLOT_END; i++)

        for(int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; i++)
            if(Bag* pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, i ))
                for(uint32 j = 0; j < pBag->GetBagSize(); j++)
                    if(Item* pItem = GetItemByPos( i, j ))
                        DurabilityLoss(pItem,percent);
    }
}

void Player::DurabilityLoss(Item* item, double percent)
{
    if(!item )
        return;

    uint32 pMaxDurability =  item ->GetUInt32Value(ITEM_FIELD_MAXDURABILITY);

    if(!pMaxDurability)
        return;

    uint32 pDurabilityLoss = uint32(pMaxDurability*percent);

    if(pDurabilityLoss < 1 )
        pDurabilityLoss = 1;

    DurabilityPointsLoss(item,pDurabilityLoss);
}

void Player::DurabilityPointsLossAll(int32 points, bool inventory)
{
    for(int i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; i++)
        if(Item *pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i ))
            DurabilityPointsLoss(pItem,points);

    if(inventory)
    {
        // bags not have durability
        // for(int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; i++)

        for(int i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; i++)
            if(Item *pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i ))
                DurabilityPointsLoss(pItem,points);

        // keys not have durability
        //for(int i = KEYRING_SLOT_START; i < KEYRING_SLOT_END; i++)

        for(int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; i++)
            if(Bag* pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, i ))
                for(uint32 j = 0; j < pBag->GetBagSize(); j++)
                    if(Item* pItem = GetItemByPos( i, j ))
                        DurabilityPointsLoss(pItem,points);
    }
}

void Player::DurabilityPointsLoss(Item* item, int32 points)
{
    int32 pMaxDurability = item->GetUInt32Value(ITEM_FIELD_MAXDURABILITY);
    int32 pOldDurability = item->GetUInt32Value(ITEM_FIELD_DURABILITY);
    int32 pNewDurability = pOldDurability - points;

    if (pNewDurability < 0)
        pNewDurability = 0;
    else if (pNewDurability > pMaxDurability)
        pNewDurability = pMaxDurability;

    if (pOldDurability != pNewDurability)
    {
        // modify item stats _before_ Durability set to 0 to pass _ApplyItemMods internal check
        if ( pNewDurability == 0 && pOldDurability > 0 && item->IsEquipped())
            _ApplyItemMods(item,item->GetSlot(), false);

        item->SetUInt32Value(ITEM_FIELD_DURABILITY, pNewDurability);

        // modify item stats _after_ restore durability to pass _ApplyItemMods internal check
        if ( pNewDurability > 0 && pOldDurability == 0 && item->IsEquipped())
            _ApplyItemMods(item,item->GetSlot(), true);

        item->SetState(ITEM_CHANGED, this);
    }
}

void Player::DurabilityPointLossForEquipSlot(EquipmentSlots slot)
{
    if(Item *pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, slot ))
        DurabilityPointsLoss(pItem,1);
}

uint32 Player::DurabilityRepairAll(bool cost, float discountMod, bool guildBank)
{
    uint32 TotalCost = 0;
    // equipped, backpack, bags itself
    for(int i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_ITEM_END; i++)
        TotalCost += DurabilityRepair(( (INVENTORY_SLOT_BAG_0 << 8) | i ),cost,discountMod, guildBank);

    // bank, buyback and keys not repaired

    // items in inventory bags
    for(int j = INVENTORY_SLOT_BAG_START; j < INVENTORY_SLOT_BAG_END; j++)
        for(int i = 0; i < MAX_BAG_SIZE; i++)
            TotalCost += DurabilityRepair(( (j << 8) | i ),cost,discountMod, guildBank);
    return TotalCost;
}

uint32 Player::DurabilityRepair(uint16 pos, bool cost, float discountMod, bool guildBank)
{
    Item* item = GetItemByPos(pos);

    uint32 TotalCost = 0;
    if(!item)
        return TotalCost;

    uint32 maxDurability = item->GetUInt32Value(ITEM_FIELD_MAXDURABILITY);
    if(!maxDurability)
        return TotalCost;

    uint32 curDurability = item->GetUInt32Value(ITEM_FIELD_DURABILITY);

    if(cost)
    {
        uint32 LostDurability = maxDurability - curDurability;
        if(LostDurability>0)
        {
            ItemPrototype const *ditemProto = item->GetProto();

            DurabilityCostsEntry const *dcost = sDurabilityCostsStore.LookupEntry(ditemProto->ItemLevel);
            if(!dcost)
            {
                sLog.outError("ERROR: RepairDurability: Wrong item lvl %u", ditemProto->ItemLevel);
                return TotalCost;
            }

            uint32 dQualitymodEntryId = (ditemProto->Quality+1)*2;
            DurabilityQualityEntry const *dQualitymodEntry = sDurabilityQualityStore.LookupEntry(dQualitymodEntryId);
            if(!dQualitymodEntry)
            {
                sLog.outError("ERROR: RepairDurability: Wrong dQualityModEntry %u", dQualitymodEntryId);
                return TotalCost;
            }

            uint32 dmultiplier = dcost->multiplier[ItemSubClassToDurabilityMultiplierId(ditemProto->Class,ditemProto->SubClass)];
            uint32 costs = uint32(LostDurability*dmultiplier*double(dQualitymodEntry->quality_mod));

            costs = uint32(costs * discountMod);

            if (costs==0)                                   //fix for ITEM_QUALITY_ARTIFACT
                costs = 1;

            if (guildBank)
            {
                if (GetGuildId()==0)
                {
                    DEBUG_LOG("You are not member of a guild");
                    return TotalCost;
                }

                Guild *pGuild = objmgr.GetGuildById(GetGuildId());
                if (!pGuild)
                    return TotalCost;

                if (!pGuild->HasRankRight(GetRank(), GR_RIGHT_WITHDRAW_REPAIR))
                {
                    DEBUG_LOG("You do not have rights to withdraw for repairs");
                    return TotalCost;
                }

                if (pGuild->GetMemberMoneyWithdrawRem(GetGUIDLow()) < costs)
                {
                    DEBUG_LOG("You do not have enough money withdraw amount remaining");
                    return TotalCost;
                }

                if (pGuild->GetGuildBankMoney() < costs)
                {
                    DEBUG_LOG("There is not enough money in bank");
                    return TotalCost;
                }

                SQLTransaction trans = CharacterDatabase.BeginTransaction();
                pGuild->MemberMoneyWithdraw(costs, GetGUIDLow(), trans);
                CharacterDatabase.CommitTransaction(trans);
                TotalCost = costs;
            }
            else if (GetMoney() < costs)
            {
                DEBUG_LOG("You do not have enough money");
                return TotalCost;
            }
            else
                ModifyMoney( -int32(costs) );
        }
    }

    item->SetUInt32Value(ITEM_FIELD_DURABILITY, maxDurability);
    item->SetState(ITEM_CHANGED, this);

    // reapply mods for total broken and repaired item if equipped
    if(IsEquipmentPos(pos) && !curDurability)
        _ApplyItemMods(item,pos & 255, true);
    return TotalCost;
}

void Player::RepopAtGraveyard()
{
    // note: this can be called also when the player is alive
    // for example from WorldSession::HandleMovementOpcodes

    AreaTableEntry const *zone = GetAreaEntryByAreaID(GetAreaId());

    // Such zones are considered unreachable as a ghost and the player must be automatically revived
    if(!isAlive() && zone && zone->flags & AREA_FLAG_NEED_FLY || GetTransport() || (zone && GetPositionZ() < zone->maxDepth) || (zone && zone->ID == 2257))
    {
        ResurrectPlayer(0.5f);
        SpawnCorpseBones();
    }

    WorldSafeLocsEntry const *ClosestGrave = NULL;

    // Special handle for battleground maps
    BattleGround *bg = sBattleGroundMgr.GetBattleGround(GetBattleGroundId());

    if(bg && (bg->GetTypeID() == BATTLEGROUND_AB || bg->GetTypeID() == BATTLEGROUND_EY || bg->GetTypeID() == BATTLEGROUND_AV || bg->GetTypeID() == BATTLEGROUND_WS))
        ClosestGrave = bg->GetClosestGraveYard(GetPositionX(), GetPositionY(), GetPositionZ(), GetTeam());
    else
        ClosestGrave = objmgr.GetClosestGraveYard( GetPositionX(), GetPositionY(), GetPositionZ(), GetMapId(), GetTeam() );

    // stop countdown until repop
    m_deathTimer = 0;

    // if no grave found, stay at the current location
    // and don't show spirit healer location
    if(ClosestGrave)
    {
        TeleportTo(ClosestGrave->map_id, ClosestGrave->x, ClosestGrave->y, ClosestGrave->z, GetOrientation());
        if(isDead())                                        // not send if alive, because it used in TeleportTo()
        {
            WorldPacket data(SMSG_DEATH_RELEASE_LOC, 4*4);  // show spirit healer position on minimap
            data << ClosestGrave->map_id;
            data << ClosestGrave->x;
            data << ClosestGrave->y;
            data << ClosestGrave->z;
            GetSession()->SendPacket(&data);
        }
    }
    else if (zone && GetPositionZ() < zone->maxDepth)
        TeleportTo(m_homebindMapId, m_homebindX, m_homebindY, m_homebindZ, GetOrientation());
}

void Player::JoinedChannel(Channel *c)
{
    m_channels.push_back(c);
}

void Player::LeftChannel(Channel *c)
{
    m_channels.remove(c);
}

void Player::CleanupChannels()
{
    while(!m_channels.empty())
    {
        Channel* ch = *m_channels.begin();
        m_channels.erase(m_channels.begin());               // remove from player's channel list
        ch->Leave(GetGUID(), false);                 // not send to client, not remove from player's channel list
        if (ChannelMgr* cMgr = channelMgr(GetTeam()))
            cMgr->LeftChannel(ch->GetName());               // deleted channel if empty
    }
}

void Player::UpdateLocalChannels(uint32 newZone )
{
    if(m_channels.empty())
        return;

    AreaTableEntry const* current_zone = GetAreaEntryByAreaID(newZone);
    if(!current_zone)
        return;

    ChannelMgr* cMgr = channelMgr(GetTeam());
    if(!cMgr)
        return;

    std::string current_zone_name = current_zone->area_name[GetSession()->GetSessionDbcLocale()];

    for(JoinedChannelsList::iterator i = m_channels.begin(), next; i != m_channels.end(); i = next)
    {
        next = i; ++next;

        // skip non built-in channels
        if(!(*i)->IsConstant())
            continue;

        ChatChannelsEntry const* ch = GetChannelEntryFor((*i)->GetChannelId());
        if(!ch)
            continue;

        if((ch->flags & 4) == 4)                            // global channel without zone name in pattern
            continue;

        //  new channel
        char new_channel_name_buf[100];
        snprintf(new_channel_name_buf,100,ch->pattern[m_session->GetSessionDbcLocale()],current_zone_name.c_str());
        Channel* new_channel = cMgr->GetJoinChannel(new_channel_name_buf,ch->ChannelID);

        if((*i)!=new_channel)
        {
            new_channel->Join(GetGUID(),"");                // will output Changed Channel: N. Name

            // leave old channel
            (*i)->Leave(GetGUID(),false);                   // not send leave channel, it already replaced at client
            std::string name = (*i)->GetName();             // store name, (*i)erase in LeftChannel
            LeftChannel(*i);                                // remove from player's channel list
            cMgr->LeftChannel(name);                        // delete if empty
        }
    }
}

void Player::LeaveLFGChannel()
{
    for(JoinedChannelsList::iterator i = m_channels.begin(); i != m_channels.end(); ++i )
    {
        if((*i)->IsLFG())
        {
            (*i)->Leave(GetGUID());
            break;
        }
    }
}

void Player::UpdateDefense()
{
    uint32 defense_skill_gain = sWorld.getConfig(CONFIG_SKILL_GAIN_DEFENSE);

    if(UpdateSkill(SKILL_DEFENSE,defense_skill_gain))
    {
        // update dependent from defense skill part
        UpdateDefenseBonusesMod();
    }
}

void Player::HandleBaseModValue(BaseModGroup modGroup, BaseModType modType, float amount, bool apply)
{
    if(modGroup >= BASEMOD_END || modType >= MOD_END)
    {
        sLog.outError("ERROR in HandleBaseModValue(): non existed BaseModGroup of wrong BaseModType!");
        return;
    }

    float val = 1.0f;

    switch(modType)
    {
        case FLAT_MOD:
            m_auraBaseMod[modGroup][modType] += apply ? amount : -amount;
            break;
        case PCT_MOD:
            if(amount <= -100.0f)
                amount = -200.0f;

            val = (100.0f + amount) / 100.0f;
            m_auraBaseMod[modGroup][modType] *= apply ? val : (1.0f/val);
            break;
    }

    if(!CanModifyStats())
        return;

    switch(modGroup)
    {
        case CRIT_PERCENTAGE:              UpdateCritPercentage(BASE_ATTACK);                          break;
        case RANGED_CRIT_PERCENTAGE:       UpdateCritPercentage(RANGED_ATTACK);                        break;
        case OFFHAND_CRIT_PERCENTAGE:      UpdateCritPercentage(OFF_ATTACK);                           break;
        case SHIELD_BLOCK_VALUE:           UpdateShieldBlockValue();                                   break;
        default: break;
    }
}

float Player::GetBaseModValue(BaseModGroup modGroup, BaseModType modType) const
{
    if(modGroup >= BASEMOD_END || modType > MOD_END)
    {
        sLog.outError("ERROR: trial to access non existed BaseModGroup or wrong BaseModType!");
        return 0.0f;
    }

    if(modType == PCT_MOD && m_auraBaseMod[modGroup][PCT_MOD] <= 0.0f)
        return 0.0f;

    return m_auraBaseMod[modGroup][modType];
}

float Player::GetTotalBaseModValue(BaseModGroup modGroup) const
{
    if(modGroup >= BASEMOD_END)
    {
        sLog.outError("ERROR: wrong BaseModGroup in GetTotalBaseModValue()!");
        return 0.0f;
    }

    if(m_auraBaseMod[modGroup][PCT_MOD] <= 0.0f)
        return 0.0f;

    return m_auraBaseMod[modGroup][FLAT_MOD] * m_auraBaseMod[modGroup][PCT_MOD];
}

uint32 Player::GetShieldBlockValue() const
{
    BaseModGroup modGroup = SHIELD_BLOCK_VALUE;

    float value = GetTotalBaseModValue(modGroup) + GetStat(STAT_STRENGTH)/20 - 1;

    value = (value < 0) ? 0 : value;

    return uint32(value);
}

float Player::GetMeleeCritFromAgility()
{
    uint32 level = getLevel();
    uint32 pclass = getClass();

    if (level>GT_MAX_LEVEL) level = GT_MAX_LEVEL;

    GtChanceToMeleeCritBaseEntry const *critBase  = sGtChanceToMeleeCritBaseStore.LookupEntry(pclass-1);
    GtChanceToMeleeCritEntry     const *critRatio = sGtChanceToMeleeCritStore.LookupEntry((pclass-1)*GT_MAX_LEVEL + level-1);
    if (critBase==NULL || critRatio==NULL)
        return 0.0f;

    float crit=critBase->base + GetStat(STAT_AGILITY)*critRatio->ratio;
    return crit*100.0f;
}

float Player::GetDodgeFromAgility()
{
    // Table for base dodge values
    float dodge_base[MAX_CLASSES] = {
         0.0075f,   // Warrior
         0.00652f,  // Paladin
        -0.0545f,   // Hunter
        -0.0059f,   // Rogue
         0.03183f,  // Priest
         0.0114f,   // DK
         0.0167f,   // Shaman
         0.034575f, // Mage
         0.02011f,  // Warlock
         0.0f,      // ??
        -0.0187f    // Druid
    };
    // Crit/agility to dodge/agility coefficient multipliers
    float crit_to_dodge[MAX_CLASSES] = {
         1.1f,      // Warrior
         1.0f,      // Paladin
         1.6f,      // Hunter
         2.0f,      // Rogue
         1.0f,      // Priest
         1.0f,      // DK?
         1.0f,      // Shaman
         1.0f,      // Mage
         1.0f,      // Warlock
         0.0f,      // ??
         1.7f       // Druid
    };

    uint32 level = getLevel();
    uint32 pclass = getClass();

    if (level>GT_MAX_LEVEL) level = GT_MAX_LEVEL;

    // Dodge per agility for most classes equal crit per agility (but for some classes need apply some multiplier)
    GtChanceToMeleeCritEntry  const *dodgeRatio = sGtChanceToMeleeCritStore.LookupEntry((pclass-1)*GT_MAX_LEVEL + level-1);
    if (dodgeRatio==NULL || pclass > MAX_CLASSES)
        return 0.0f;

    float dodge=dodge_base[pclass-1] + GetStat(STAT_AGILITY) * dodgeRatio->ratio * crit_to_dodge[pclass-1];
    return dodge*100.0f;
}

float Player::GetSpellCritFromIntellect()
{
    uint32 level = getLevel();
    uint32 pclass = getClass();

    if (level>GT_MAX_LEVEL) level = GT_MAX_LEVEL;

    GtChanceToSpellCritBaseEntry const *critBase  = sGtChanceToSpellCritBaseStore.LookupEntry(pclass-1);
    GtChanceToSpellCritEntry     const *critRatio = sGtChanceToSpellCritStore.LookupEntry((pclass-1)*GT_MAX_LEVEL + level-1);
    if (critBase==NULL || critRatio==NULL)
        return 0.0f;

    float crit=critBase->base + GetStat(STAT_INTELLECT)*critRatio->ratio;
    return crit*100.0f;
}

float Player::GetRatingCoefficient(CombatRating cr) const
{
    uint32 level = getLevel();

    if (level>GT_MAX_LEVEL) level = GT_MAX_LEVEL;

    GtCombatRatingsEntry const *Rating = sGtCombatRatingsStore.LookupEntry(cr*GT_MAX_LEVEL+level-1);
    if (Rating == NULL)
        return 1.0f;                                        // By default use minimum coefficient (not must be called)

    return Rating->ratio;
}

float Player::GetRatingBonusValue(CombatRating cr) const
{
    return float(GetUInt32Value(PLAYER_FIELD_COMBAT_RATING_1 + cr)) / GetRatingCoefficient(cr);
}

uint32 Player::GetMeleeCritDamageReduction(uint32 damage) const
{
    float melee  = GetRatingBonusValue(CR_CRIT_TAKEN_MELEE)*2.0f;
    if (melee>25.0f) melee = 25.0f;
    return uint32 (melee * damage /100.0f);
}

uint32 Player::GetRangedCritDamageReduction(uint32 damage) const
{
    float ranged = GetRatingBonusValue(CR_CRIT_TAKEN_RANGED)*2.0f;
    if (ranged>25.0f) ranged=25.0f;
    return uint32 (ranged * damage /100.0f);
}

uint32 Player::GetSpellCritDamageReduction(uint32 damage) const
{
    float spell = GetRatingBonusValue(CR_CRIT_TAKEN_SPELL)*2.0f;
    // In wow script resilience limited to 25%
    if (spell>25.0f)
        spell = 25.0f;
    return uint32 (spell * damage / 100.0f);
}

uint32 Player::GetDotDamageReduction(uint32 damage) const
{
    float spellDot = GetRatingBonusValue(CR_CRIT_TAKEN_SPELL);
    // Dot resilience not limited (limit it by 100%)
    if (spellDot > 100.0f)
        spellDot = 100.0f;
    return uint32 (spellDot * damage / 100.0f);
}

float Player::GetExpertiseDodgeOrParryReduction(WeaponAttackType attType) const
{
    switch (attType)
    {
        case BASE_ATTACK:
            return GetUInt32Value(PLAYER_EXPERTISE) / 4.0f;
        case OFF_ATTACK:
            return GetUInt32Value(PLAYER_OFFHAND_EXPERTISE) / 4.0f;
        default:
            break;
    }
    return 0.0f;
}

float Player::OCTRegenHPPerSpirit()
{
    uint32 level = getLevel();
    uint32 pclass = getClass();

    if (level>GT_MAX_LEVEL) level = GT_MAX_LEVEL;

    GtOCTRegenHPEntry     const *baseRatio = sGtOCTRegenHPStore.LookupEntry((pclass-1)*GT_MAX_LEVEL + level-1);
    GtRegenHPPerSptEntry  const *moreRatio = sGtRegenHPPerSptStore.LookupEntry((pclass-1)*GT_MAX_LEVEL + level-1);
    if (baseRatio==NULL || moreRatio==NULL)
        return 0.0f;

    // Formula from PaperDollFrame script
    float spirit = GetStat(STAT_SPIRIT);
    float baseSpirit = spirit;
    if (baseSpirit>50) baseSpirit = 50;
    float moreSpirit = spirit - baseSpirit;
    float regen = baseSpirit * baseRatio->ratio + moreSpirit * moreRatio->ratio;
    return regen;
}

float Player::OCTRegenMPPerSpirit()
{
    uint32 level = getLevel();
    uint32 pclass = getClass();

    if (level>GT_MAX_LEVEL) level = GT_MAX_LEVEL;

//    GtOCTRegenMPEntry     const *baseRatio = sGtOCTRegenMPStore.LookupEntry((pclass-1)*GT_MAX_LEVEL + level-1);
    GtRegenMPPerSptEntry  const *moreRatio = sGtRegenMPPerSptStore.LookupEntry((pclass-1)*GT_MAX_LEVEL + level-1);
    if (moreRatio==NULL)
        return 0.0f;

    // Formula get from PaperDollFrame script
    float spirit    = GetStat(STAT_SPIRIT);
    float regen     = spirit * moreRatio->ratio;
    return regen;
}

void Player::ApplyRatingMod(CombatRating cr, int32 value, bool apply)
{
    ApplyModUInt32Value(PLAYER_FIELD_COMBAT_RATING_1 + cr, value, apply);

    float RatingCoeffecient = GetRatingCoefficient(cr);
    float RatingChange = 0.0f;

    bool affectStats = CanModifyStats();

    switch (cr)
    {
        case CR_WEAPON_SKILL:                               // Implemented in Unit::RollMeleeOutcomeAgainst
        case CR_DEFENSE_SKILL:
            UpdateDefenseBonusesMod();
            break;
        case CR_DODGE:
            UpdateDodgePercentage();
            break;
        case CR_PARRY:
            UpdateParryPercentage();
            break;
        case CR_BLOCK:
            UpdateBlockPercentage();
            break;
        case CR_HIT_MELEE:
            RatingChange = value / RatingCoeffecient;
            m_modMeleeHitChance += apply ? RatingChange : -RatingChange;
            break;
        case CR_HIT_RANGED:
            RatingChange = value / RatingCoeffecient;
            m_modRangedHitChance += apply ? RatingChange : -RatingChange;
            break;
        case CR_HIT_SPELL:
            RatingChange = value / RatingCoeffecient;
            m_modSpellHitChance += apply ? RatingChange : -RatingChange;
            break;
        case CR_CRIT_MELEE:
            if(affectStats)
            {
                UpdateCritPercentage(BASE_ATTACK);
                UpdateCritPercentage(OFF_ATTACK);
            }
            break;
        case CR_CRIT_RANGED:
            if(affectStats)
                UpdateCritPercentage(RANGED_ATTACK);
            break;
        case CR_CRIT_SPELL:
            if(affectStats)
                UpdateAllSpellCritChances();
            break;
        case CR_HIT_TAKEN_MELEE:                            // Implemented in Unit::MeleeMissChanceCalc
        case CR_HIT_TAKEN_RANGED:
            break;
        case CR_HIT_TAKEN_SPELL:                            // Implemented in Unit::MagicSpellHitResult
            break;
        case CR_CRIT_TAKEN_MELEE:                           // Implemented in Unit::RollMeleeOutcomeAgainst (only for chance to crit)
        case CR_CRIT_TAKEN_RANGED:
            break;
        case CR_CRIT_TAKEN_SPELL:                           // Implemented in Unit::SpellCriticalBonus (only for chance to crit)
            break;
        case CR_HASTE_MELEE:
            RatingChange = value / RatingCoeffecient;
            ApplyAttackTimePercentMod(BASE_ATTACK,RatingChange,apply);
            ApplyAttackTimePercentMod(OFF_ATTACK,RatingChange,apply);
            break;
        case CR_HASTE_RANGED:
            RatingChange = value / RatingCoeffecient;
            ApplyAttackTimePercentMod(RANGED_ATTACK, RatingChange, apply);
            break;
        case CR_HASTE_SPELL:
            RatingChange = value / RatingCoeffecient;
            ApplyCastTimePercentMod(RatingChange,apply);
            break;
        case CR_WEAPON_SKILL_MAINHAND:                      // Implemented in Unit::RollMeleeOutcomeAgainst
        case CR_WEAPON_SKILL_OFFHAND:
        case CR_WEAPON_SKILL_RANGED:
            break;
        case CR_EXPERTISE:
            if(affectStats)
            {
                UpdateExpertise(BASE_ATTACK);
                UpdateExpertise(OFF_ATTACK);
            }
            break;
    }
}

void Player::SetRegularAttackTime()
{
    for(int i = 0; i < MAX_ATTACK; ++i)
    {
        Item *tmpitem = GetWeaponForAttack(WeaponAttackType(i));
        if(tmpitem && !tmpitem->IsBroken())
        {
            ItemPrototype const *proto = tmpitem->GetProto();
            if(proto->Delay)
                SetAttackTime(WeaponAttackType(i), proto->Delay);
            else
                SetAttackTime(WeaponAttackType(i), BASE_ATTACK_TIME);
        }
    }
}

//skill+step, checking for max value
bool Player::UpdateSkill(uint32 skill_id, uint32 step)
{
    if(!skill_id)
        return false;

    SkillStatusMap::iterator itr = mSkillStatus.find(skill_id);
    if(itr == mSkillStatus.end() || itr->second.uState == SKILL_DELETED)
        return false;
        
    uint32 valueIndex = PLAYER_SKILL_VALUE_INDEX(itr->second.pos);
    uint32 data = GetUInt32Value(valueIndex);
    uint32 value = SKILL_VALUE(data);
    uint32 max = SKILL_MAX(data);

    if ((!max) || (!value) || (value >= max))
        return false;

    if (value*512 < max*GetMap()->urand(0,512))
    {
        uint32 new_value = value+step;
        if(new_value > max)
            new_value = max;

        SetUInt32Value(valueIndex,MAKE_SKILL_VALUE(new_value,max));
        if(itr->second.uState != SKILL_NEW)
            itr->second.uState = SKILL_CHANGED;
            
        return true;
    }

    return false;
}

inline int SkillGainChance(uint32 SkillValue, uint32 GrayLevel, uint32 GreenLevel, uint32 YellowLevel)
{
    if ( SkillValue >= GrayLevel )
        return sWorld.getConfig(CONFIG_SKILL_CHANCE_GREY)*10;
    if ( SkillValue >= GreenLevel )
        return sWorld.getConfig(CONFIG_SKILL_CHANCE_GREEN)*10;
    if ( SkillValue >= YellowLevel )
        return sWorld.getConfig(CONFIG_SKILL_CHANCE_YELLOW)*10;
    return sWorld.getConfig(CONFIG_SKILL_CHANCE_ORANGE)*10;
}

bool Player::UpdateCraftSkill(uint32 spellid)
{
    SkillLineAbilityMap::const_iterator lower = spellmgr.GetBeginSkillLineAbilityMap(spellid);
    SkillLineAbilityMap::const_iterator upper = spellmgr.GetEndSkillLineAbilityMap(spellid);

    for(SkillLineAbilityMap::const_iterator _spell_idx = lower; _spell_idx != upper; ++_spell_idx)
    {
        if(_spell_idx->second->skillId)
        {
            uint32 SkillValue = GetPureSkillValue(_spell_idx->second->skillId);

            // Alchemy Discoveries here
            SpellEntry const* spellEntry = spellmgr.LookupSpell(spellid);
            if(spellEntry && spellEntry->Mechanic==MECHANIC_DISCOVERY)
            {
                if(uint32 discoveredSpell = GetSkillDiscoverySpell(_spell_idx->second->skillId, spellid, this))
                    learnSpell(discoveredSpell);
            }

            uint32 craft_skill_gain = sWorld.getConfig(CONFIG_SKILL_GAIN_CRAFTING);

            return UpdateSkillPro(_spell_idx->second->skillId, SkillGainChance(SkillValue,
                _spell_idx->second->max_value,
                (_spell_idx->second->max_value + _spell_idx->second->min_value)/2,
                _spell_idx->second->min_value),
                craft_skill_gain);
        }
    }
    return false;
}

bool Player::UpdateGatherSkill(uint32 SkillId, uint32 SkillValue, uint32 RedLevel, uint32 Multiplicator )
{
    uint32 gathering_skill_gain = sWorld.getConfig(CONFIG_SKILL_GAIN_GATHERING);

    // For skinning and Mining chance decrease with level. 1-74 - no decrease, 75-149 - 2 times, 225-299 - 8 times
    switch (SkillId)
    {
        case SKILL_HERBALISM:
        case SKILL_LOCKPICKING:
        case SKILL_JEWELCRAFTING:
            return UpdateSkillPro(SkillId, SkillGainChance(SkillValue, RedLevel+100, RedLevel+50, RedLevel+25)*Multiplicator,gathering_skill_gain);
        case SKILL_SKINNING:
            if( sWorld.getConfig(CONFIG_SKILL_CHANCE_SKINNING_STEPS)==0)
                return UpdateSkillPro(SkillId, SkillGainChance(SkillValue, RedLevel+100, RedLevel+50, RedLevel+25)*Multiplicator,gathering_skill_gain);
            else
                return UpdateSkillPro(SkillId, (SkillGainChance(SkillValue, RedLevel+100, RedLevel+50, RedLevel+25)*Multiplicator) >> (SkillValue/sWorld.getConfig(CONFIG_SKILL_CHANCE_SKINNING_STEPS)), gathering_skill_gain);
        case SKILL_MINING:
            if (sWorld.getConfig(CONFIG_SKILL_CHANCE_MINING_STEPS)==0)
                return UpdateSkillPro(SkillId, SkillGainChance(SkillValue, RedLevel+100, RedLevel+50, RedLevel+25)*Multiplicator,gathering_skill_gain);
            else
                return UpdateSkillPro(SkillId, (SkillGainChance(SkillValue, RedLevel+100, RedLevel+50, RedLevel+25)*Multiplicator) >> (SkillValue/sWorld.getConfig(CONFIG_SKILL_CHANCE_MINING_STEPS)),gathering_skill_gain);
    }
    return false;
}

bool Player::UpdateFishingSkill()
{
    uint32 SkillValue = GetPureSkillValue(SKILL_FISHING);

    int32 chance = SkillValue < 75 ? 100 : 2500/(SkillValue-50);

    uint32 gathering_skill_gain = sWorld.getConfig(CONFIG_SKILL_GAIN_GATHERING);

    return UpdateSkillPro(SKILL_FISHING,chance*10,gathering_skill_gain);
}

bool Player::UpdateSkillPro(uint16 SkillId, int32 Chance, uint32 step)
{
    if ( !SkillId )
        return false;

    if(Chance <= 0)                                         // speedup in 0 chance case
        return false;

    SkillStatusMap::iterator itr = mSkillStatus.find(SkillId);
    if(itr == mSkillStatus.end() || itr->second.uState == SKILL_DELETED)
        return false;
        
    uint32 valueIndex = PLAYER_SKILL_VALUE_INDEX(itr->second.pos);
    
    uint32 data = GetUInt32Value(valueIndex);
    uint16 SkillValue = SKILL_VALUE(data);
    uint16 MaxValue   = SKILL_MAX(data);

    if ( !MaxValue || !SkillValue || SkillValue >= MaxValue )
        return false;

    int32 Roll = GetMap()->irand(1,1000);

    if ( Roll <= Chance )
    {
        uint32 new_value = SkillValue+step;
        if(new_value > MaxValue)
            new_value = MaxValue;

        SetUInt32Value(valueIndex,MAKE_SKILL_VALUE(new_value,MaxValue));
        if(itr->second.uState != SKILL_NEW)
            itr->second.uState = SKILL_CHANGED;
            
        return true;
    }

    return false;
}

void Player::UpdateWeaponSkill (WeaponAttackType attType)
{
    // no skill gain in pvp
    Unit *pVictim = getVictim();
    if (pVictim && pVictim->isCharmedOwnedByPlayerOrPlayer())
        return;

    if(IsInFeralForm())
        return;                                             // always maximized SKILL_FERAL_COMBAT in fact

    if(m_form == FORM_TREE)
        return;                                             // use weapon but not skill up

    uint32 weapon_skill_gain = sWorld.getConfig(CONFIG_SKILL_GAIN_WEAPON);

    switch(attType)
    {
        case BASE_ATTACK:
        {
            Item *tmpitem = GetWeaponForAttack(attType,true);

            if (!tmpitem)
                UpdateSkill(SKILL_UNARMED,weapon_skill_gain);
            else if(tmpitem->GetProto()->SubClass != ITEM_SUBCLASS_WEAPON_FISHING_POLE)
                UpdateSkill(tmpitem->GetSkill(),weapon_skill_gain);
            break;
        }
        case OFF_ATTACK:
        case RANGED_ATTACK:
        {
            Item *tmpitem = GetWeaponForAttack(attType,true);
            if (tmpitem)
                UpdateSkill(tmpitem->GetSkill(),weapon_skill_gain);
            break;
        }
    }
    UpdateAllCritPercentages();
}

void Player::UpdateCombatSkills(Unit *pVictim, WeaponAttackType attType, MeleeHitOutcome outcome, bool defence)
{
/* Not need, this checked on call this func from trigger system
    switch(outcome)
    {
        case MELEE_HIT_CRIT:
        case MELEE_HIT_DODGE:
        case MELEE_HIT_PARRY:
        case MELEE_HIT_BLOCK:
        case MELEE_HIT_BLOCK_CRIT:
            return;

        default:
            break;
    }
*/
    uint32 plevel = getLevel();                             // if defense than pVictim == attacker
    uint32 greylevel = Trinity::XP::GetGrayLevel(plevel);
    uint32 moblevel = pVictim->getLevelForTarget(this);
    if(moblevel < greylevel)
        return;

    if (moblevel > plevel + 5)
        moblevel = plevel + 5;

    uint32 lvldif = moblevel - greylevel;
    if(lvldif < 3)
        lvldif = 3;

    uint32 skilldif = 5 * plevel - (defence ? GetBaseDefenseSkillValue() : GetBaseWeaponSkillValue(attType));
    if(skilldif <= 0)
        return;

    float chance = float(3 * lvldif * skilldif) / plevel;
    if(!defence)
    {
        if(getClass() == CLASS_WARRIOR || getClass() == CLASS_ROGUE)
            chance *= 0.1f * GetStat(STAT_INTELLECT);
    }

    chance = chance < 1.0f ? 1.0f : chance;                 //minimum chance to increase skill is 1%

    if(roll_chance_f(chance))
    {
        if(defence)
            UpdateDefense();
        else
            UpdateWeaponSkill(attType);
    }
    else
        return;
}

void Player::ModifySkillBonus(uint32 skillid,int32 val, bool talent)
{
    SkillStatusMap::const_iterator itr = mSkillStatus.find(skillid);
    if(itr == mSkillStatus.end() || itr->second.uState == SKILL_DELETED)
        return;
        
    uint32 bonusIndex = PLAYER_SKILL_BONUS_INDEX(itr->second.pos);

    uint32 bonus_val = GetUInt32Value(bonusIndex);
    int16 temp_bonus = SKILL_TEMP_BONUS(bonus_val);
    int16 perm_bonus = SKILL_PERM_BONUS(bonus_val);

    if(talent)                                          // permanent bonus stored in high part
        SetUInt32Value(bonusIndex,MAKE_SKILL_BONUS(temp_bonus,perm_bonus+val));
    else
        SetUInt32Value(bonusIndex,MAKE_SKILL_BONUS(temp_bonus+val,perm_bonus));
}

void Player::UpdateSkillsForLevel()
{
    uint16 maxconfskill = sWorld.GetConfigMaxSkillValue();
    uint32 maxSkill = GetMaxSkillValueForLevel();

    bool alwaysMaxSkill = sWorld.getConfig(CONFIG_ALWAYS_MAX_SKILL_FOR_LEVEL);

    for(SkillStatusMap::iterator itr = mSkillStatus.begin(); itr != mSkillStatus.end(); ++itr)
    {
        if(itr->second.uState == SKILL_DELETED)
            continue;

        uint32 pskill = itr->first;
        SkillLineEntry const *pSkill = sSkillLineStore.LookupEntry(pskill);
        if (!pSkill)
            continue;

        if (GetSkillRangeType(pSkill,false) != SKILL_RANGE_LEVEL)
            continue;

        uint32 valueIndex = PLAYER_SKILL_VALUE_INDEX(itr->second.pos);
        uint32 data = GetUInt32Value(valueIndex);
        uint32 max = SKILL_MAX(data);
        uint32 val = SKILL_VALUE(data);

        /// update only level dependent max skill values
        if (max != 1)
        {
            /// maximize skill always
            if (alwaysMaxSkill)
            {
                SetUInt32Value(valueIndex, MAKE_SKILL_VALUE(maxSkill,maxSkill));
                if(itr->second.uState != SKILL_NEW)
                    itr->second.uState = SKILL_CHANGED;
            }
            else if(max != maxconfskill)                    /// update max skill value if current max skill not maximized
            {
                SetUInt32Value(valueIndex, MAKE_SKILL_VALUE(val,maxSkill));
                if(itr->second.uState != SKILL_NEW)
                    itr->second.uState = SKILL_CHANGED;
            }
        }
    }
}

void Player::UpdateSkillsToMaxSkillsForLevel()
{
    for(SkillStatusMap::iterator itr = mSkillStatus.begin(); itr != mSkillStatus.end(); ++itr)
    {
        if(itr->second.uState == SKILL_DELETED)
            continue;

        uint32 pskill = itr->first;
        if (IsProfessionOrRidingSkill(pskill))
            continue;
        uint32 valueIndex = PLAYER_SKILL_VALUE_INDEX(itr->second.pos);
        uint32 data = GetUInt32Value(valueIndex);
        uint32 max = SKILL_MAX(data);

        if (max > 1)
        {
            SetUInt32Value(valueIndex,MAKE_SKILL_VALUE(max,max));
            if(itr->second.uState != SKILL_NEW)
                itr->second.uState = SKILL_CHANGED;
        }
        if (pskill == SKILL_DEFENSE)
            UpdateDefenseBonusesMod();
    }
}

// This functions sets a skill line value (and adds if doesn't exist yet)
// To "remove" a skill line, set it's values to zero
void Player::SetSkill(uint32 id, uint16 currVal, uint16 maxVal)
{
    if(!id)
        return;

    SkillStatusMap::iterator itr = mSkillStatus.find(id);

    // Has skill
    if(itr != mSkillStatus.end() && itr->second.uState != SKILL_DELETED)
    {
        if(currVal) {
            SetUInt32Value(PLAYER_SKILL_VALUE_INDEX(itr->second.pos),MAKE_SKILL_VALUE(currVal,maxVal));
            if(itr->second.uState != SKILL_NEW)
                itr->second.uState = SKILL_CHANGED;
        }
        else                                                //remove
        {
            // clear skill fields
            SetUInt32Value(PLAYER_SKILL_INDEX(itr->second.pos),0);
            SetUInt32Value(PLAYER_SKILL_VALUE_INDEX(itr->second.pos),0);
            SetUInt32Value(PLAYER_SKILL_BONUS_INDEX(itr->second.pos),0);

            // mark as deleted or simply remove from map if not saved yet
            if(itr->second.uState != SKILL_NEW)
                itr->second.uState = SKILL_DELETED;
            else
                mSkillStatus.erase(itr);

            // remove spells that depend on this skill when removing the skill
            for (PlayerSpellMap::const_iterator itr = m_spells.begin(), next = m_spells.begin(); itr != m_spells.end(); itr = next)
            {
                ++next;
                if(itr->second->state == PLAYERSPELL_REMOVED)
                    continue;

                SkillLineAbilityMap::const_iterator lower = spellmgr.GetBeginSkillLineAbilityMap(itr->first);
                SkillLineAbilityMap::const_iterator upper = spellmgr.GetEndSkillLineAbilityMap(itr->first);

                for(SkillLineAbilityMap::const_iterator _spell_idx = lower; _spell_idx != upper; ++_spell_idx)
                {
                    if (_spell_idx->second->skillId == id)
                    {
                        // this may remove more than one spell (dependents)
                        removeSpell(itr->first);
                        next = m_spells.begin();
                        break;
                    }
                }
            }
        }
    }
    else if(currVal)                                        //add
    {
        for (int i=0; i < PLAYER_MAX_SKILLS; ++i) {
            if (!GetUInt32Value(PLAYER_SKILL_INDEX(i)))
            {
                SkillLineEntry const *pSkill = sSkillLineStore.LookupEntry(id);
                if (!pSkill)
                {
                    sLog.outError("Skill not found in SkillLineStore: skill #%u", id);
                    return;
                }
                // enable unlearn button for primary professions only
                if (pSkill->categoryId == SKILL_CATEGORY_PROFESSION)
                    SetUInt32Value(PLAYER_SKILL_INDEX(i), MAKE_PAIR32(id,1));
                else
                    SetUInt32Value(PLAYER_SKILL_INDEX(i), MAKE_PAIR32(id,0));
                SetUInt32Value(PLAYER_SKILL_VALUE_INDEX(i),MAKE_SKILL_VALUE(currVal,maxVal));

                // insert new entry or update if not deleted old entry yet
                if(itr != mSkillStatus.end())
                {
                    itr->second.pos = i;
                    itr->second.uState = SKILL_CHANGED;
                }
                else
                    mSkillStatus.insert(SkillStatusMap::value_type(id, SkillStatusData(i, SKILL_NEW)));

                // apply skill bonuses
                SetUInt32Value(PLAYER_SKILL_BONUS_INDEX(i),0);

                // temporary bonuses
                AuraList const& mModSkill = GetAurasByType(SPELL_AURA_MOD_SKILL);
                for (AuraList::const_iterator j = mModSkill.begin(); j != mModSkill.end(); ++j)
                    if ((*j)->GetMiscValue() == int32(id))
                        (*j)->ApplyModifier(true);

                // permanent bonuses
                AuraList const& mModSkillTalent = GetAurasByType(SPELL_AURA_MOD_SKILL_TALENT);
                for (AuraList::const_iterator j = mModSkillTalent.begin(); j != mModSkillTalent.end(); ++j)
                    if ((*j)->GetMiscValue() == int32(id))
                        (*j)->ApplyModifier(true);

                // Learn all spells for skill
                learnSkillRewardedSpells(id);
                return;
            }
        }
    }
}

bool Player::HasSkill(uint32 skill) const
{
    if(!skill)
        return false;

    SkillStatusMap::const_iterator itr = mSkillStatus.find(skill);
    return (itr != mSkillStatus.end() && itr->second.uState != SKILL_DELETED);
}

uint16 Player::GetSkillValue(uint32 skill) const
{
    if(!skill)
        return 0;

    SkillStatusMap::const_iterator itr = mSkillStatus.find(skill);
    if(itr == mSkillStatus.end() || itr->second.uState == SKILL_DELETED)
        return 0;

    uint32 bonus = GetUInt32Value(PLAYER_SKILL_BONUS_INDEX(itr->second.pos));

    int32 result = int32(SKILL_VALUE(GetUInt32Value(PLAYER_SKILL_VALUE_INDEX(itr->second.pos))));
    result += SKILL_TEMP_BONUS(bonus);
    result += SKILL_PERM_BONUS(bonus);
    return result < 0 ? 0 : result;
}

uint16 Player::GetMaxSkillValue(uint32 skill) const
{
    if(!skill)
        return 0;
    
    SkillStatusMap::const_iterator itr = mSkillStatus.find(skill);
    if(itr == mSkillStatus.end() || itr->second.uState == SKILL_DELETED)
        return 0;

    uint32 bonus = GetUInt32Value(PLAYER_SKILL_BONUS_INDEX(itr->second.pos));

    int32 result = int32(SKILL_MAX(GetUInt32Value(PLAYER_SKILL_VALUE_INDEX(itr->second.pos))));
    result += SKILL_TEMP_BONUS(bonus);
    result += SKILL_PERM_BONUS(bonus);
    return result < 0 ? 0 : result;
}

uint16 Player::GetPureMaxSkillValue(uint32 skill) const
{
    if(!skill)
        return 0;

    SkillStatusMap::const_iterator itr = mSkillStatus.find(skill);
    if(itr == mSkillStatus.end() || itr->second.uState == SKILL_DELETED)
        return 0;

    return SKILL_MAX(GetUInt32Value(PLAYER_SKILL_VALUE_INDEX(itr->second.pos)));
}

uint16 Player::GetBaseSkillValue(uint32 skill) const
{
    if(!skill)
        return 0;

    SkillStatusMap::const_iterator itr = mSkillStatus.find(skill);
    if(itr == mSkillStatus.end() || itr->second.uState == SKILL_DELETED)
        return 0;

    int32 result = int32(SKILL_VALUE(GetUInt32Value(PLAYER_SKILL_VALUE_INDEX(itr->second.pos))));
    result +=  SKILL_PERM_BONUS(GetUInt32Value(PLAYER_SKILL_BONUS_INDEX(itr->second.pos)));
    return result < 0 ? 0 : result;
}

uint16 Player::GetPureSkillValue(uint32 skill) const
{
    if(!skill)
        return 0;

    SkillStatusMap::const_iterator itr = mSkillStatus.find(skill);
    if(itr == mSkillStatus.end() || itr->second.uState == SKILL_DELETED)
        return 0;

    return SKILL_VALUE(GetUInt32Value(PLAYER_SKILL_VALUE_INDEX(itr->second.pos)));
}

int16 Player::GetSkillPermBonusValue(uint32 skill) const
{
    if (!skill)
        return 0;

    SkillStatusMap::const_iterator itr = mSkillStatus.find(skill);
    if(itr == mSkillStatus.end() || itr->second.uState == SKILL_DELETED)
        return 0;

    return SKILL_PERM_BONUS(GetUInt32Value(PLAYER_SKILL_BONUS_INDEX(itr->second.pos)));
}

int16 Player::GetSkillTempBonusValue(uint32 skill) const
{
    if (!skill)
        return 0;

    SkillStatusMap::const_iterator itr = mSkillStatus.find(skill);
    if(itr == mSkillStatus.end() || itr->second.uState == SKILL_DELETED)
        return 0;

    return SKILL_TEMP_BONUS(GetUInt32Value(PLAYER_SKILL_BONUS_INDEX(itr->second.pos)));
}

void Player::SendInitialActionButtons()
{
    sLog.outDetail( "Initializing Action Buttons for '%u'", GetGUIDLow() );

    WorldPacket data(SMSG_ACTION_BUTTONS, (MAX_ACTION_BUTTONS*4));
    for(int button = 0; button < MAX_ACTION_BUTTONS; ++button)
    {
        ActionButtonList::const_iterator itr = m_actionButtons.find(button);
        if(itr != m_actionButtons.end() && itr->second.uState != ACTIONBUTTON_DELETED)
        {
            data << uint16(itr->second.action);
            data << uint8(itr->second.misc);
            data << uint8(itr->second.type);
        }
        else
        {
            data << uint32(0);
        }
    }

    GetSession()->SendPacket( &data );
    sLog.outDetail( "Action Buttons for '%u' Initialized", GetGUIDLow() );
}

void Player::addActionButton(const uint8 button, const uint16 action, const uint8 type, const uint8 misc)
{
    if(button >= MAX_ACTION_BUTTONS)
    {
        sLog.outError( "Action %u not added into button %u for player %s: button must be < 132", action, button, GetName() );
        return;
    }

    // check cheating with adding non-known spells to action bar
    if(type==ACTION_BUTTON_SPELL)
    {
        if(!spellmgr.LookupSpell(action))
        {
            sLog.outError( "Action %u not added into button %u for player %s: spell not exist", action, button, GetName() );
            return;
        }

        if(!HasSpell(action))
        {
            sLog.outError( "Action %u not added into button %u for player %s: player don't known this spell", action, button, GetName() );
            return;
        }
    }

    ActionButtonList::iterator buttonItr = m_actionButtons.find(button);

    if (buttonItr==m_actionButtons.end())
    {                                                       // just add new button
        m_actionButtons[button] = ActionButton(action,type,misc);
    }
    else
    {                                                       // change state of current button
        ActionButtonUpdateState uState = buttonItr->second.uState;
        buttonItr->second = ActionButton(action,type,misc);
        if (uState != ACTIONBUTTON_NEW) buttonItr->second.uState = ACTIONBUTTON_CHANGED;
    };

    sLog.outDetail( "Player '%u' Added Action '%u' to Button '%u'", GetGUIDLow(), action, button );
}

void Player::removeActionButton(uint8 button)
{
    ActionButtonList::iterator buttonItr = m_actionButtons.find(button);
    if (buttonItr==m_actionButtons.end())
        return;

    if(buttonItr->second.uState==ACTIONBUTTON_NEW)
        m_actionButtons.erase(buttonItr);                   // new and not saved
    else
        buttonItr->second.uState = ACTIONBUTTON_DELETED;    // saved, will deleted at next save

    sLog.outDetail( "Action Button '%u' Removed from Player '%u'", button, GetGUIDLow() );
}

void Player::SetDontMove(bool dontMove)
{
    m_dontMove = dontMove;
}

bool Player::SetPosition(float x, float y, float z, float orientation, bool teleport)
{
    // prevent crash when a bad coord is sent by the client
    if(!Trinity::IsValidMapCoord(x,y,z,orientation))
    {
        sLog.outError("Player::SetPosition(%f, %f, %f, %f, %d) .. bad coordinates for player %u!",x,y,z,orientation,teleport,GetGUIDLow());
        return false;
    }

    Map *m = GetMap();

    const float old_x = GetPositionX();
    const float old_y = GetPositionY();
    const float old_z = GetPositionZ();
    const float old_r = GetOrientation();

    if( teleport || old_x != x || old_y != y || old_z != z || old_r != orientation )
    {
        if (teleport || old_x != x || old_y != y || old_z != z)
            RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_MOVE | AURA_INTERRUPT_FLAG_TURNING);
        else
            RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_TURNING);

        // move and update visible state if need
        m->PlayerRelocation(this, x, y, z, orientation);

        // reread after Map::Relocation
        m = GetMap();
        x = GetPositionX();
        y = GetPositionY();
        z = GetPositionZ();

        // group update
        if(GetGroup() && (old_x != x || old_y != y))
            SetGroupUpdateFlag(GROUP_UPDATE_FLAG_POSITION);
    }

    // code block for underwater state update
    UpdateUnderwaterState(m, x, y, z);

    CheckAreaExploreAndOutdoor();

    return true;
}

void Player::SaveRecallPosition()
{
    m_recallMap = GetMapId();
    m_recallX = GetPositionX();
    m_recallY = GetPositionY();
    m_recallZ = GetPositionZ();
    m_recallO = GetOrientation();
}

void Player::SendMessageToSet(WorldPacket *data, bool self, bool to_possessor)
{
    MapManager::Instance().GetMap(GetMapId(), this)->MessageBroadcast(this, data, self, to_possessor);
}

void Player::SendMessageToSetInRange(WorldPacket *data, float dist, bool self, bool to_possessor)
{
    MapManager::Instance().GetMap(GetMapId(), this)->MessageDistBroadcast(this, data, dist, self, to_possessor);
}

void Player::SendMessageToSetInRange(WorldPacket *data, float dist, bool self, bool to_possessor, bool own_team_only)
{
    MapManager::Instance().GetMap(GetMapId(), this)->MessageDistBroadcast(this, data, dist, self, to_possessor, own_team_only);
}

void Player::SendDirectMessage(WorldPacket *data)
{
    GetSession()->SendPacket(data);
}

void Player::CheckAreaExploreAndOutdoor()
{
    if (!isAlive())
        return;

    if (isInFlight())
        return;

    bool isOutdoor;
    uint16 areaFlag = GetBaseMap()->GetAreaFlag(GetPositionX(),GetPositionY(),GetPositionZ(), &isOutdoor);

    if (sWorld.getConfig(CONFIG_VMAP_INDOOR_CHECK) && !isOutdoor)
        RemoveAurasWithAttribute(SPELL_ATTR_OUTDOORS_ONLY);
    else if (isOutdoor) {
        // Check if we need to reapply outdoor only passive spells
        const PlayerSpellMap& sp_list = GetSpellMap();
        for (PlayerSpellMap::const_iterator itr = sp_list.begin(); itr != sp_list.end(); ++itr) {
            if (itr->second->state == PLAYERSPELL_REMOVED)
                continue;
            SpellEntry const* spellInfo = spellmgr.LookupSpell(itr->first);
            if (!spellInfo || !IsNeedCastSpellAtOutdoor(spellInfo) || HasAura(itr->first))
                continue;
            
            if (GetErrorAtShapeshiftedCast(spellInfo, m_form) == 0)
                CastSpell(this, itr->first, true, NULL);
        }
    }

    if (areaFlag==0xffff)
        return;
    int offset = areaFlag / 32;

    if(offset >= 128)
    {
        sLog.outError("ERROR: Wrong area flag %u in map data for (X: %f Y: %f) point to field PLAYER_EXPLORED_ZONES_1 + %u ( %u must be < 64 ).",areaFlag,GetPositionX(),GetPositionY(),offset,offset);
        return;
    }

    uint32 val = (uint32)(1 << (areaFlag % 32));
    uint32 currFields = GetUInt32Value(PLAYER_EXPLORED_ZONES_1 + offset);

    if( !(currFields & val) )
    {
        SetUInt32Value(PLAYER_EXPLORED_ZONES_1 + offset, (uint32)(currFields | val));

        AreaTableEntry const *p = GetAreaEntryByAreaFlagAndMap(areaFlag,GetMapId());
        if(!p)
        {
            sLog.outError("PLAYER: Player %u discovered unknown area (x: %f y: %f map: %u", GetGUIDLow(), GetPositionX(),GetPositionY(),GetMapId());
        }
        else if(p->area_level > 0)
        {
            uint32 area = p->ID;
            if (getLevel() >= sWorld.getConfig(CONFIG_MAX_PLAYER_LEVEL))
            {
                SendExplorationExperience(area,0);
            }
            else
            {
                int32 diff = int32(getLevel()) - p->area_level;
                uint32 XP = 0;
                if (diff < -5)
                {
                    if (hasCustomXpRate())
                        XP = uint32(objmgr.GetBaseXP(getLevel()+5)*m_customXp);
                    else
                        XP = uint32(objmgr.GetBaseXP(getLevel()+5)*sWorld.getRate(RATE_XP_EXPLORE));
                }
                else if (diff > 5)
                {
                    int32 exploration_percent = (100-((diff-5)*5));
                    if (exploration_percent > 100)
                        exploration_percent = 100;
                    else if (exploration_percent < 0)
                        exploration_percent = 0;

                    if (hasCustomXpRate())
                        XP = uint32(objmgr.GetBaseXP(p->area_level)*exploration_percent/100*m_customXp);
                    else
                        XP = uint32(objmgr.GetBaseXP(p->area_level)*exploration_percent/100*sWorld.getRate(RATE_XP_EXPLORE));
                }
                else
                {
                    if (hasCustomXpRate())
                        XP = uint32(objmgr.GetBaseXP(p->area_level)*m_customXp);
                    else
                        XP = uint32(objmgr.GetBaseXP(p->area_level)*sWorld.getRate(RATE_XP_EXPLORE));
                }

                GiveXP( XP, NULL );
                SendExplorationExperience(area,XP);
            }
            sLog.outDetail("PLAYER: Player %u discovered a new area: %u", GetGUIDLow(), area);
        }
    }
}

uint32 Player::TeamForRace(uint8 race)
{
    ChrRacesEntry const* rEntry = sChrRacesStore.LookupEntry(race);
    if(!rEntry)
    {
        sLog.outError("Race %u not found in DBC: wrong DBC files?",uint32(race));
        return ALLIANCE;
    }

    switch(rEntry->TeamID)
    {
        case 7: return ALLIANCE;
        case 1: return HORDE;
    }

    sLog.outError("Race %u have wrong team id in DBC: wrong DBC files?",uint32(race),rEntry->TeamID);
    return ALLIANCE;
}

uint32 Player::getFactionForRace(uint8 race)
{
    ChrRacesEntry const* rEntry = sChrRacesStore.LookupEntry(race);
    if(!rEntry)
    {
        sLog.outError("Race %u not found in DBC: wrong DBC files?",uint32(race));
        return 0;
    }

    return rEntry->FactionID;
}

void Player::setFactionForRace(uint8 race)
{
    m_team = TeamForRace(race);
    setFaction( getFactionForRace(race) );
}

void Player::UpdateReputation() const
{
    sLog.outDetail( "WORLD: Player::UpdateReputation" );

    for(FactionStateList::const_iterator itr = m_factions.begin(); itr != m_factions.end(); ++itr)
    {
        SendFactionState(&(itr->second));
    }
}

void Player::SendFactionState(FactionState const* faction) const
{
    if(faction->Flags & FACTION_FLAG_VISIBLE)               //If faction is visible then update it
    {
        WorldPacket data(SMSG_SET_FACTION_STANDING, (16));  // last check 2.4.0
        data << (float) 0;                                  // unk 2.4.0
        data << (uint32) 1;                                 // count
        // for
        data << (uint32) faction->ReputationListID;
        data << (uint32) faction->Standing;
        // end for
        GetSession()->SendPacket(&data);
    }
}

void Player::SendInitialReputations()
{
    WorldPacket data(SMSG_INITIALIZE_FACTIONS, (4+128*5));
    data << uint32 (0x00000080);

    RepListID a = 0;

    for (FactionStateList::const_iterator itr = m_factions.begin(); itr != m_factions.end(); ++itr)
    {
        // fill in absent fields
        for (; a != itr->first; a++)
        {
            data << uint8  (0x00);
            data << uint32 (0x00000000);
        }

        // fill in encountered data
        data << uint8  (itr->second.Flags);
        data << uint32 (itr->second.Standing);

        ++a;
    }

    // fill in absent fields
    for (; a != 128; a++)
    {
        data << uint8  (0x00);
        data << uint32 (0x00000000);
    }

    GetSession()->SendPacket(&data);
}

FactionState const* Player::GetFactionState( FactionEntry const* factionEntry) const
{
    FactionStateList::const_iterator itr = m_factions.find(factionEntry->reputationListID);
    if (itr != m_factions.end())
        return &itr->second;

    return NULL;
}

void Player::SetFactionAtWar(FactionState* faction, bool atWar)
{
    // not allow declare war to own faction
    if(atWar && (faction->Flags & FACTION_FLAG_PEACE_FORCED) )
        return;

    // already set
    if(((faction->Flags & FACTION_FLAG_AT_WAR) != 0) == atWar)
        return;

    if( atWar )
        faction->Flags |= FACTION_FLAG_AT_WAR;
    else
        faction->Flags &= ~FACTION_FLAG_AT_WAR;

    faction->Changed = true;
}

void Player::SetFactionInactive(FactionState* faction, bool inactive)
{
    // always invisible or hidden faction can't be inactive
    if(inactive && ((faction->Flags & (FACTION_FLAG_INVISIBLE_FORCED|FACTION_FLAG_HIDDEN)) || !(faction->Flags & FACTION_FLAG_VISIBLE) ) )
        return;

    // already set
    if(((faction->Flags & FACTION_FLAG_INACTIVE) != 0) == inactive)
        return;

    if(inactive)
        faction->Flags |= FACTION_FLAG_INACTIVE;
    else
        faction->Flags &= ~FACTION_FLAG_INACTIVE;

    faction->Changed = true;
}

void Player::SetFactionVisibleForFactionTemplateId(uint32 FactionTemplateId)
{
    FactionTemplateEntry const*factionTemplateEntry = sFactionTemplateStore.LookupEntry(FactionTemplateId);

    if(!factionTemplateEntry)
        return;

    SetFactionVisibleForFactionId(factionTemplateEntry->faction);
}

void Player::SetFactionVisibleForFactionId(uint32 FactionId)
{
    FactionEntry const *factionEntry = sFactionStore.LookupEntry(FactionId);
    if(!factionEntry)
        return;

    if(factionEntry->reputationListID < 0)
        return;

    FactionStateList::iterator itr = m_factions.find(factionEntry->reputationListID);
    if (itr == m_factions.end())
        return;

    SetFactionVisible(&itr->second);
}

void Player::SetFactionVisible(FactionState* faction)
{
    // always invisible or hidden faction can't be make visible
    if(faction->Flags & (FACTION_FLAG_INVISIBLE_FORCED|FACTION_FLAG_HIDDEN))
        return;

    // already set
    if(faction->Flags & FACTION_FLAG_VISIBLE)
        return;

    faction->Flags |= FACTION_FLAG_VISIBLE;
    faction->Changed = true;

    if(!m_session->PlayerLoading())
    {
        // make faction visible in reputation list at client
        WorldPacket data(SMSG_SET_FACTION_VISIBLE, 4);
        data << faction->ReputationListID;
        GetSession()->SendPacket(&data);
    }
}

void Player::SetInitialFactions()
{
    for(unsigned int i = 1; i < sFactionStore.GetNumRows(); i++)
    {
        FactionEntry const *factionEntry = sFactionStore.LookupEntry(i);

        if( factionEntry && (factionEntry->reputationListID >= 0))
        {
            FactionState newFaction;
            newFaction.ID = factionEntry->ID;
            newFaction.ReputationListID = factionEntry->reputationListID;
            newFaction.Standing = 0;
            newFaction.Flags = GetDefaultReputationFlags(factionEntry);
            newFaction.Changed = true;
            newFaction.Deleted = false;

            m_factions[newFaction.ReputationListID] = newFaction;
        }
    }
}

uint32 Player::GetDefaultReputationFlags(const FactionEntry *factionEntry) const
{
    if (!factionEntry)
        return 0;

    uint32 raceMask = getRaceMask();
    uint32 classMask = getClassMask();
    for (int i=0; i < 4; i++)
    {
        if( (factionEntry->BaseRepRaceMask[i] & raceMask) &&
            (factionEntry->BaseRepClassMask[i]==0 ||
            (factionEntry->BaseRepClassMask[i] & classMask) ) )
            return factionEntry->ReputationFlags[i];
    }
    return 0;
}

int32 Player::GetBaseReputation(const FactionEntry *factionEntry) const
{
    if (!factionEntry)
        return 0;

    uint32 raceMask = getRaceMask();
    uint32 classMask = getClassMask();
    for (int i=0; i < 4; i++)
    {
        if( (factionEntry->BaseRepRaceMask[i] & raceMask) &&
            (factionEntry->BaseRepClassMask[i]==0 ||
            (factionEntry->BaseRepClassMask[i] & classMask) ) )
            return factionEntry->BaseRepValue[i];
    }

    // in faction.dbc exist factions with (RepListId >=0, listed in character reputation list) with all BaseRepRaceMask[i]==0
    return 0;
}

int32 Player::GetReputation(uint32 faction_id) const
{
    FactionEntry const *factionEntry = sFactionStore.LookupEntry(faction_id);

    if (!factionEntry)
    {
        sLog.outError("Player::GetReputation: Can't get reputation of %s for unknown faction (faction template id) #%u.",GetName(), faction_id);
        return 0;
    }

    return GetReputation(factionEntry);
}

int32 Player::GetReputation(const FactionEntry *factionEntry) const
{
    // Faction without recorded reputation. Just ignore.
    if(!factionEntry)
        return 0;

    FactionStateList::const_iterator itr = m_factions.find(factionEntry->reputationListID);
    if (itr != m_factions.end())
        return GetBaseReputation(factionEntry) + itr->second.Standing;

    return 0;
}

ReputationRank Player::GetReputationRank(uint32 faction) const
{
    FactionEntry const*factionEntry = sFactionStore.LookupEntry(faction);
    if(!factionEntry)
        return MIN_REPUTATION_RANK;

    return GetReputationRank(factionEntry);
}

ReputationRank Player::ReputationToRank(int32 standing) const
{
    int32 Limit = Reputation_Cap + 1;
    for (int i = MAX_REPUTATION_RANK-1; i >= MIN_REPUTATION_RANK; --i)
    {
        Limit -= ReputationRank_Length[i];
        if (standing >= Limit )
            return ReputationRank(i);
    }
    return MIN_REPUTATION_RANK;
}

ReputationRank Player::GetReputationRank(const FactionEntry *factionEntry) const
{
    int32 Reputation = GetReputation(factionEntry);
    return ReputationToRank(Reputation);
}

ReputationRank Player::GetBaseReputationRank(const FactionEntry *factionEntry) const
{
    int32 Reputation = GetBaseReputation(factionEntry);
    return ReputationToRank(Reputation);
}

bool Player::ModifyFactionReputation(uint32 FactionTemplateId, int32 DeltaReputation)
{
    FactionTemplateEntry const* factionTemplateEntry = sFactionTemplateStore.LookupEntry(FactionTemplateId);

    if(!factionTemplateEntry)
    {
        sLog.outError("Player::ModifyFactionReputation: Can't update reputation of %s for unknown faction (faction template id) #%u.", GetName(), FactionTemplateId);
        return false;
    }

    FactionEntry const *factionEntry = sFactionStore.LookupEntry(factionTemplateEntry->faction);

    // Faction without recorded reputation. Just ignore.
    if(!factionEntry)
        return false;

    return ModifyFactionReputation(factionEntry, DeltaReputation);
}

bool Player::ModifyFactionReputation(FactionEntry const* factionEntry, int32 standing)
{
    SimpleFactionsList const* flist = GetFactionTeamList(factionEntry->ID);
    if (flist)
    {
        bool res = false;
        for (SimpleFactionsList::const_iterator itr = flist->begin();itr != flist->end();++itr)
        {
            FactionEntry const *factionEntryCalc = sFactionStore.LookupEntry(*itr);
            if(factionEntryCalc)
                res = ModifyOneFactionReputation(factionEntryCalc, standing);
        }
        return res;
    }
    else
        return ModifyOneFactionReputation(factionEntry, standing);
}

bool Player::ModifyOneFactionReputation(FactionEntry const* factionEntry, int32 standing)
{
    FactionStateList::iterator itr = m_factions.find(factionEntry->reputationListID);
    if (itr != m_factions.end())
    {
        int32 BaseRep = GetBaseReputation(factionEntry);
        int32 new_rep = BaseRep + itr->second.Standing + standing;

        if (new_rep > Reputation_Cap)
            new_rep = Reputation_Cap;
        else
        if (new_rep < Reputation_Bottom)
            new_rep = Reputation_Bottom;

        if(ReputationToRank(new_rep) <= REP_HOSTILE)
            SetFactionAtWar(&itr->second,true);

        itr->second.Standing = new_rep - BaseRep;
        itr->second.Changed = true;

        SetFactionVisible(&itr->second);

        for( int i = 0; i < MAX_QUEST_LOG_SIZE; i++ )
        {
            if(uint32 questid = GetQuestSlotQuestId(i))
            {
                Quest const* qInfo = objmgr.GetQuestTemplate(questid);
                if( qInfo && qInfo->GetRepObjectiveFaction() == factionEntry->ID )
                {
                    QuestStatusData& q_status = mQuestStatus[questid];
                    if( q_status.m_status == QUEST_STATUS_INCOMPLETE )
                    {
                        if(GetReputation(factionEntry) >= qInfo->GetRepObjectiveValue())
                            if ( CanCompleteQuest( questid ) )
                                CompleteQuest( questid );
                    }
                    else if( q_status.m_status == QUEST_STATUS_COMPLETE )
                    {
                        if(GetReputation(factionEntry) < qInfo->GetRepObjectiveValue())
                            IncompleteQuest( questid );
                    }
                }
            }
        }

        SendFactionState(&(itr->second));

        return true;
    }
    return false;
}

bool Player::SetFactionReputation(uint32 FactionTemplateId, int32 standing)
{
    FactionTemplateEntry const* factionTemplateEntry = sFactionTemplateStore.LookupEntry(FactionTemplateId);

    if(!factionTemplateEntry)
    {
        sLog.outError("Player::SetFactionReputation: Can't set reputation of %s for unknown faction (faction template id) #%u.", GetName(), FactionTemplateId);
        return false;
    }

    FactionEntry const *factionEntry = sFactionStore.LookupEntry(factionTemplateEntry->faction);

    // Faction without recorded reputation. Just ignore.
    if(!factionEntry)
        return false;

    return SetFactionReputation(factionEntry, standing);
}

bool Player::SetFactionReputation(FactionEntry const* factionEntry, int32 standing)
{
    SimpleFactionsList const* flist = GetFactionTeamList(factionEntry->ID);
    if (flist)
    {
        bool res = false;
        for (SimpleFactionsList::const_iterator itr = flist->begin();itr != flist->end();++itr)
        {
            FactionEntry const *factionEntryCalc = sFactionStore.LookupEntry(*itr);
            if(factionEntryCalc)
                res = SetOneFactionReputation(factionEntryCalc, standing);
        }
        return res;
    }
    else
        return SetOneFactionReputation(factionEntry, standing);
}

bool Player::SetOneFactionReputation(FactionEntry const* factionEntry, int32 standing)
{
    FactionStateList::iterator itr = m_factions.find(factionEntry->reputationListID);
    if (itr != m_factions.end())
    {
        if (standing > Reputation_Cap)
            standing = Reputation_Cap;
        else
        if (standing < Reputation_Bottom)
            standing = Reputation_Bottom;

        int32 BaseRep = GetBaseReputation(factionEntry);
        itr->second.Standing = standing - BaseRep;
        itr->second.Changed = true;

        SetFactionVisible(&itr->second);

        if(ReputationToRank(standing) <= REP_HOSTILE)
            SetFactionAtWar(&itr->second,true);

        SendFactionState(&(itr->second));
        return true;
    }
    return false;
}

void Player::SwapFactionReputation(uint32 factionId1, uint32 factionId2)
{
    FactionEntry const* factionEntry1 = sFactionStore.LookupEntry(factionId1);
    FactionEntry const* factionEntry2 = sFactionStore.LookupEntry(factionId2);
    
    FactionState* state1 = (FactionState*) GetFactionState(factionEntry1);
    FactionState* state2 = (FactionState*) GetFactionState(factionEntry2);
    
   
    if (!state1 || !state2) {
        sLog.outError("Player::SwapFactionReputation: Attempt to swap a faction with a non-existing FactionEntry");
        return;
    }
    
    FactionState derefState1 = *state1;
    FactionState derefState1Cpy = *state1;
    FactionState derefState2 = *state2;
    
    derefState1.Standing = derefState2.Standing;
    derefState1.Flags = derefState2.Flags;
    derefState1.Changed = true;
    
    derefState2.Standing = derefState1Cpy.Standing;
    derefState2.Flags = derefState1Cpy.Flags;
    derefState2.Changed = true;
    
    m_factions[factionEntry1->reputationListID] = derefState2;
    m_factions[factionEntry2->reputationListID] = derefState1;
}

void Player::DropFactionReputation(uint32 factionId)
{
    FactionEntry const* factionEntry = sFactionStore.LookupEntry(factionId);
    if (!factionEntry) {
        sLog.outError("Player::SwapFactionReputation: Attempt to drop a faction with a non-existing FactionEntry");
        return;
    }
    
    FactionState* state = (FactionState*) GetFactionState(factionEntry);
    state->Changed = true;
    state->Deleted = true;
}

//Calculate total reputation percent player gain with quest/creature level
int32 Player::CalculateReputationGain(uint32 creatureOrQuestLevel, int32 rep, bool for_quest)
{
    // for grey creature kill received 20%, in other case 100.
    int32 percent = (!for_quest && (creatureOrQuestLevel <= Trinity::XP::GetGrayLevel(getLevel()))) ? 20 : 100;

    int32 repMod = GetTotalAuraModifier(SPELL_AURA_MOD_REPUTATION_GAIN);

    percent += rep > 0 ? repMod : -repMod;

    if(percent <=0)
        return 0;

    return int32(sWorld.getRate(RATE_REPUTATION_GAIN)*rep*percent/100);
}

//Calculates how many reputation points player gains in victim's enemy factions
void Player::RewardReputation(Unit *pVictim, float rate)
{
    if(!pVictim || pVictim->GetTypeId() == TYPEID_PLAYER)
        return;

    if((pVictim->ToCreature())->IsReputationGainDisabled())
        return;

    ReputationOnKillEntry const* Rep = objmgr.GetReputationOnKilEntry((pVictim->ToCreature())->GetCreatureInfo()->Entry);

    if(!Rep)
        return;
        
    Unit::AuraList const& DummyAuras = GetAurasByType(SPELL_AURA_DUMMY);
    for(Unit::AuraList::const_iterator i = DummyAuras.begin();i != DummyAuras.end(); ++i)
    if((*i)->GetId() == 32098 || (*i)->GetId() == 32096)
    {
        uint32 area_id = GetAreaId();
        if(area_id == 3483 || area_id == 3535 || area_id == 3562 || area_id == 3713)
            rate = rate*(1.0f + 25.0f / 100.0f);
    }

    if(Rep->repfaction1 && (!Rep->team_dependent || GetTeam()==ALLIANCE))
    {
        int32 donerep1 = CalculateReputationGain(pVictim->getLevel(),Rep->repvalue1,false);
        donerep1 = int32(donerep1*rate);
        FactionEntry const *factionEntry1 = sFactionStore.LookupEntry(Rep->repfaction1);
        uint32 current_reputation_rank1 = GetReputationRank(factionEntry1);
        if(factionEntry1 && current_reputation_rank1 <= Rep->reputation_max_cap1)
            ModifyFactionReputation(factionEntry1, donerep1);

        // Wiki: Team factions value divided by 2
        if(Rep->is_teamaward1)
        {
            FactionEntry const *team1_factionEntry = sFactionStore.LookupEntry(factionEntry1->team);
            if(team1_factionEntry)
                ModifyFactionReputation(team1_factionEntry, donerep1 / 2);
        }
    }

    if(Rep->repfaction2 && (!Rep->team_dependent || GetTeam()==HORDE))
    {
        int32 donerep2 = CalculateReputationGain(pVictim->getLevel(),Rep->repvalue2,false);
        donerep2 = int32(donerep2*rate);
        FactionEntry const *factionEntry2 = sFactionStore.LookupEntry(Rep->repfaction2);
        uint32 current_reputation_rank2 = GetReputationRank(factionEntry2);
        if(factionEntry2 && current_reputation_rank2 <= Rep->reputation_max_cap2)
            ModifyFactionReputation(factionEntry2, donerep2);

        // Wiki: Team factions value divided by 2
        if(Rep->is_teamaward2)
        {
            FactionEntry const *team2_factionEntry = sFactionStore.LookupEntry(factionEntry2->team);
            if(team2_factionEntry)
                ModifyFactionReputation(team2_factionEntry, donerep2 / 2);
        }
    }
}

//Calculate how many reputation points player gain with the quest
void Player::RewardReputation(Quest const *pQuest)
{
    // quest reputation reward/loss
    for(int i = 0; i < QUEST_REPUTATIONS_COUNT; ++i)
    {
        if(pQuest->RewRepFaction[i] && pQuest->RewRepValue[i] )
        {
            int32 rep = CalculateReputationGain(pQuest->GetQuestLevel(),pQuest->RewRepValue[i],true);
            FactionEntry const* factionEntry = sFactionStore.LookupEntry(pQuest->RewRepFaction[i]);
            if(factionEntry)
                ModifyFactionReputation(factionEntry, rep);
        }
    }

    // TODO: implement reputation spillover
}

void Player::UpdateArenaFields(void)
{
    /* arena calcs go here */
}

void Player::UpdateHonorFields()
{
    /// called when rewarding honor and at each save
    uint64 now = time(NULL);
    uint64 today = uint64(time(NULL) / DAY) * DAY;

    if(m_lastHonorUpdateTime < today)
    {
        uint64 yesterday = today - DAY;

        uint16 kills_today = PAIR32_LOPART(GetUInt32Value(PLAYER_FIELD_KILLS));

        // update yesterday's contribution
        if(m_lastHonorUpdateTime >= yesterday )
        {
            SetUInt32Value(PLAYER_FIELD_YESTERDAY_CONTRIBUTION, GetUInt32Value(PLAYER_FIELD_TODAY_CONTRIBUTION));

            // this is the first update today, reset today's contribution
            SetUInt32Value(PLAYER_FIELD_TODAY_CONTRIBUTION, 0);
            SetUInt32Value(PLAYER_FIELD_KILLS, MAKE_PAIR32(0,kills_today));
        }
        else
        {
            // no honor/kills yesterday or today, reset
            SetUInt32Value(PLAYER_FIELD_YESTERDAY_CONTRIBUTION, 0);
            SetUInt32Value(PLAYER_FIELD_KILLS, 0);
        }
    }

    m_lastHonorUpdateTime = now;
}

///Calculate the amount of honor gained based on the victim
///and the size of the group for which the honor is divided
///An exact honor value can also be given (overriding the calcs)
bool Player::RewardHonor(Unit *uVictim, uint32 groupsize, float honor, bool pvptoken)
{
    // do not reward honor in arenas, but enable onkill spellproc
    if(InArena())
    {
        if(!uVictim || uVictim == this || uVictim->GetTypeId() != TYPEID_PLAYER)
            return false;

        if( GetBGTeam() == (uVictim->ToPlayer())->GetBGTeam() )
            return false;

        return true;
    }

    // 'Inactive' this aura prevents the player from gaining honor points and battleground tokens
    if(GetDummyAura(SPELL_AURA_PLAYER_INACTIVE))
        return false;

    uint64 victim_guid = 0;
    uint32 victim_rank = 0;
    time_t now = time(NULL);

    // need call before fields update to have chance move yesterday data to appropriate fields before today data change.
    UpdateHonorFields();

    // do not reward honor in arenas, but return true to enable onkill spellproc
    if(InBattleGround() && GetBattleGround() && GetBattleGround()->isArena())
        return true;

    if(honor <= 0)
    {
        if(!uVictim || uVictim == this || uVictim->HasAuraType(SPELL_AURA_NO_PVP_CREDIT))
            return false;

        victim_guid = uVictim->GetGUID();

        if( uVictim->GetTypeId() == TYPEID_PLAYER )
        {
            Player *pVictim = uVictim->ToPlayer();

            if( GetTeam() == pVictim->GetTeam() && !sWorld.IsFFAPvPRealm() )
                return false;

            float f = 1;                                    //need for total kills (?? need more info)
            uint32 k_grey = 0;
            uint32 k_level = getLevel();
            uint32 v_level = pVictim->getLevel();

            {
                // PLAYER_CHOSEN_TITLE VALUES DESCRIPTION
                //  [0]      Just name
                //  [1..14]  Alliance honor titles and player name
                //  [15..28] Horde honor titles and player name
                //  [29..38] Other title and player name
                //  [39+]    Nothing
                uint32 victim_title = pVictim->GetUInt32Value(PLAYER_CHOSEN_TITLE);
                                                            // Get Killer titles, CharTitlesEntry::bit_index

                // Ranks:
                //  title[1..14]  -> rank[5..18]
                //  title[15..28] -> rank[5..18]
                //  title[other]  -> 0
                if (victim_title == 0)
                    victim_guid = 0;                        // Don't show HK: <rank> message, only log.
                else if (victim_title < HKRANKMAX)
                    victim_rank = victim_title + 4;
                else if (victim_title < (2*HKRANKMAX-1))
                	victim_rank = victim_title - (HKRANKMAX-1) + 4;
                else
                    victim_guid = 0;                        // Don't show HK: <rank> message, only log.
            }

            if(k_level <= 5)
                k_grey = 0;
            else if( k_level <= 39 )
                k_grey = k_level - 5 - k_level/10;
            else
                k_grey = k_level - 1 - k_level/5;

            if(v_level<=k_grey)
                return false;

            float diff_level = (k_level == k_grey) ? 1 : ((float(v_level) - float(k_grey)) / (float(k_level) - float(k_grey)));

            int32 v_rank =1;                                //need more info

            honor = ((f * diff_level * (190 + v_rank*10))/6);
            honor *= ((float)k_level) / 70.0f;              //factor of dependence on levels of the killer

            // count the number of playerkills in one day
            ApplyModUInt32Value(PLAYER_FIELD_KILLS, 1, true);
            // and those in a lifetime
            ApplyModUInt32Value(PLAYER_FIELD_LIFETIME_HONORABLE_KILLS, 1, true);

            UpdateKnownTitles();
        }
        else
        {
            Creature *cVictim = uVictim->ToCreature();

            if (!cVictim->isRacialLeader())
                return false;

            honor = 100;                                    // ??? need more info
            victim_rank = 19;                               // HK: Leader
        }
    }

    if (uVictim != NULL)
    {
        honor *= sWorld.getRate(RATE_HONOR);

        if(groupsize > 1)
            honor /= groupsize;

        honor *= (((float)GetMap()->urand(8,12))/10);                 // approx honor: 80% - 120% of real honor
    }

    // honor - for show honor points in log
    // victim_guid - for show victim name in log
    // victim_rank [1..4]  HK: <dishonored rank>
    // victim_rank [5..19] HK: <alliance\horde rank>
    // victim_rank [0,20+] HK: <>
    WorldPacket data(SMSG_PVP_CREDIT,4+8+4);
    data << (uint32) honor;
    data << (uint64) victim_guid;
    data << (uint32) victim_rank;

    GetSession()->SendPacket(&data);

    // add honor points
    ModifyHonorPoints(int32(honor));

    ApplyModUInt32Value(PLAYER_FIELD_TODAY_CONTRIBUTION, uint32(honor), true);

    if( sWorld.getConfig(CONFIG_PVP_TOKEN_ENABLE) && pvptoken )
    {
        if(!uVictim || uVictim == this || uVictim->HasAuraType(SPELL_AURA_NO_PVP_CREDIT))
            return true;

        if(uVictim->GetTypeId() == TYPEID_PLAYER)
        {
            // Check if allowed to receive it in current map
            uint8 MapType = sWorld.getConfig(CONFIG_PVP_TOKEN_MAP_TYPE);
            if( (MapType == 1 && !InBattleGround() && !HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_FFA_PVP))
                || (MapType == 2 && !HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_FFA_PVP))
                || (MapType == 3 && !InBattleGround()) )
                return true;

            uint32 ZoneId = sWorld.getConfig(CONFIG_PVP_TOKEN_ZONE_ID);
            if (ZoneId && m_zoneUpdateId != ZoneId)
                return true;

            uint32 noSpaceForCount = 0;
            uint32 itemId = sWorld.getConfig(CONFIG_PVP_TOKEN_ID);
            int32 count = sWorld.getConfig(CONFIG_PVP_TOKEN_COUNT);

            // check space and find places
            ItemPosCountVec dest;
            uint8 msg = CanStoreNewItem( NULL_BAG, NULL_SLOT, dest, itemId, count, &noSpaceForCount );
            if( msg != EQUIP_ERR_OK )   // convert to possible store amount
                count = noSpaceForCount;

            if( count == 0 || dest.empty()) // can't add any
            {
                // -- TODO: Send to mailbox if no space
                ChatHandler(this).PSendSysMessage("You don't have any space in your bags for a token.");
                return true;
            }

            Item* item = StoreNewItem( dest, itemId, true, Item::GenerateItemRandomPropertyId(itemId));
            SendNewItem(item,count,true,false);
            ChatHandler(this).PSendSysMessage("You have been awarded a token for slaying another player.");
        }
    }

    return true;
}

void Player::ModifyHonorPoints( int32 value )
{
    if(value < 0)
    {
        if (GetHonorPoints() > sWorld.getConfig(CONFIG_MAX_HONOR_POINTS))
            SetUInt32Value(PLAYER_FIELD_HONOR_CURRENCY, sWorld.getConfig(CONFIG_MAX_HONOR_POINTS) + value);
        else
            SetUInt32Value(PLAYER_FIELD_HONOR_CURRENCY, GetHonorPoints() > uint32(-value) ? GetHonorPoints() + value : 0);
    }
    else
        SetUInt32Value(PLAYER_FIELD_HONOR_CURRENCY, GetHonorPoints() < sWorld.getConfig(CONFIG_MAX_HONOR_POINTS) - value ? GetHonorPoints() + value : sWorld.getConfig(CONFIG_MAX_HONOR_POINTS));
}

void Player::ModifyArenaPoints( int32 value )
{
    if(value < 0)
    {
        if (GetArenaPoints() > sWorld.getConfig(CONFIG_MAX_ARENA_POINTS))
            SetUInt32Value(PLAYER_FIELD_ARENA_CURRENCY, sWorld.getConfig(CONFIG_MAX_ARENA_POINTS) + value);
        else
            SetUInt32Value(PLAYER_FIELD_ARENA_CURRENCY, GetArenaPoints() > uint32(-value) ? GetArenaPoints() + value : 0);
    }
    else
        SetUInt32Value(PLAYER_FIELD_ARENA_CURRENCY, GetArenaPoints() < sWorld.getConfig(CONFIG_MAX_ARENA_POINTS) - value ? GetArenaPoints() + value : sWorld.getConfig(CONFIG_MAX_ARENA_POINTS));
}

void Player::ForceNameUpdateInArenaTeams(uint64 guid, std::string newname)
{
    if (ArenaTeam *team2 = objmgr.GetArenaTeamById(GetArenaTeamIdFromDB(guid, ARENA_TEAM_2v2)))
        team2->ModifyMemberName(guid, newname);
    if (ArenaTeam *team3 = objmgr.GetArenaTeamById(GetArenaTeamIdFromDB(guid, ARENA_TEAM_3v3)))
        team3->ModifyMemberName(guid, newname);
    if (ArenaTeam *team5 = objmgr.GetArenaTeamById(GetArenaTeamIdFromDB(guid, ARENA_TEAM_5v5)))
        team5->ModifyMemberName(guid, newname);
}

uint32 Player::GetGuildIdFromDB(uint64 guid)
{
    std::ostringstream ss;
    ss<<"SELECT guildid FROM guild_member WHERE guid='"<<guid<<"'";
    QueryResult *result = CharacterDatabase.Query( ss.str().c_str() );
    if( result )
    {
        uint32 v = result->Fetch()[0].GetUInt32();
        delete result;
        return v;
    }
    else
        return 0;
}

uint32 Player::GetRankFromDB(uint64 guid)
{
    std::ostringstream ss;
    ss<<"SELECT rank FROM guild_member WHERE guid='"<<guid<<"'";
    QueryResult *result = CharacterDatabase.Query( ss.str().c_str() );
    if( result )
    {
        uint32 v = result->Fetch()[0].GetUInt32();
        delete result;
        return v;
    }
    else
        return 0;
}

uint32 Player::GetArenaTeamIdFromDB(uint64 guid, uint8 type)
{
    QueryResult *result = CharacterDatabase.PQuery("SELECT arena_team_member.arenateamid FROM arena_team_member JOIN arena_team ON arena_team_member.arenateamid = arena_team.arenateamid WHERE guid='%u' AND type='%u' LIMIT 1", GUID_LOPART(guid), type);
    if(!result)
        return 0;

    uint32 id = (*result)[0].GetUInt32();
    delete result;
    return id;
}

uint32 Player::GetZoneIdFromDB(uint64 guid)
{
    std::ostringstream ss;

    ss<<"SELECT zone FROM characters WHERE guid='"<<GUID_LOPART(guid)<<"'";
    QueryResult *result = CharacterDatabase.Query( ss.str().c_str() );
    if (!result)
        return 0;
    Field* fields = result->Fetch();
    uint32 zone = fields[0].GetUInt32();
    delete result;

    if (!zone)
    {
        // stored zone is zero, use generic and slow zone detection
        ss.str("");
        ss<<"SELECT map,position_x,position_y,position_z FROM characters WHERE guid='"<<GUID_LOPART(guid)<<"'";
        result = CharacterDatabase.Query(ss.str().c_str());
        if( !result )
            return 0;
        fields = result->Fetch();
        uint32 map  = fields[0].GetUInt32();
        float posx = fields[1].GetFloat();
        float posy = fields[2].GetFloat();
        float posz = fields[3].GetFloat();
        delete result;

        zone = MapManager::Instance().GetZoneId(map,posx,posy,posz);

        ss.str("");
        ss << "UPDATE characters SET zone='"<<zone<<"' WHERE guid='"<<GUID_LOPART(guid)<<"'";
        CharacterDatabase.Execute(ss.str().c_str());
    }

    return zone;
}

uint32 Player::GetLevelFromDB(uint64 guid)
{
    QueryResult *result = CharacterDatabase.PQuery( "SELECT level FROM characters WHERE guid='%u'", GUID_LOPART(guid) );
    if (!result)
        return 0;
        
    Field* fields = result->Fetch();
    uint32 level = fields[0].GetUInt32();
    delete result;

    return level;
}

void Player::UpdateArea(uint32 newArea)
{
    // FFA_PVP flags are area and not zone id dependent
    // so apply them accordingly
    m_areaUpdateId    = newArea;

    AreaTableEntry const* area = GetAreaEntryByAreaID(newArea);

    if(area && ((area->flags & AREA_FLAG_ARENA) || (World::IsZoneFFA(area->ID)) || (area->ID == 3775))) // Hack
    {
        if(!isGameMaster())
            SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_FFA_PVP);
    }
    else
    {
        // remove ffa flag only if not ffapvp realm
        // removal in sanctuaries and capitals is handled in zone update
        if(HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_FFA_PVP) && !sWorld.IsFFAPvPRealm())
            RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_FFA_PVP);
    }

    UpdateAreaDependentAuras(newArea);
}

void Player::UpdateZone(uint32 newZone)
{
    if(sWorld.getConfig(CONFIG_ARENASERVER_ENABLED) //bring back the escapers !
        && newZone != 616  //Hyjal arena zone
        && GetAreaId() != 19 //Zul Gurub arena zone
        && !InBattleGround()
        && !IsBeingTeleported()
        && !isGameMaster())
    {
        TeleportToArenaZone(ShouldGoToSecondaryArenaZone());
        return;
    }

    uint32 oldZoneId  = m_zoneUpdateId;
    m_zoneUpdateId    = newZone;
    m_zoneUpdateTimer = ZONE_UPDATE_INTERVAL;

    // zone changed, so area changed as well, update it
    UpdateArea(GetAreaId());

    AreaTableEntry const* zone = GetAreaEntryByAreaID(newZone);
    if(!zone)
        return;

    // inform outdoor pvp
    if(oldZoneId != m_zoneUpdateId)
    {
        sOutdoorPvPMgr.HandlePlayerLeaveZone(this, oldZoneId);
        sOutdoorPvPMgr.HandlePlayerEnterZone(this, m_zoneUpdateId);
    }

    if (sWorld.getConfig(CONFIG_WEATHER))
    {
        Weather *wth = sWorld.FindWeather(zone->ID);
        if(wth)
        {
            wth->SendWeatherUpdateToPlayer(this);
        }
        else
        {
            if(!sWorld.AddWeather(zone->ID))
            {
                // send fine weather packet to remove old zone's weather
                Weather::SendFineWeatherUpdateToPlayer(this);
            }
        }
    }

    SetPvPZone(sWorld.getConfig(CONFIG_PVP_ZONE_ENABLE) && GetZoneId() == sWorld.getConfig(CONFIG_PVP_ZONE_ID));

    pvpInfo.inHostileArea = 
        (GetTeam() == ALLIANCE && zone->team == AREATEAM_HORDE) ||
        (GetTeam() == HORDE    && zone->team == AREATEAM_ALLY)  ||
        (!isInPvPZone() && sWorld.IsPvPRealm() && zone->team == AREATEAM_NONE)  ||
        InBattleGround();                                   // overwrite for battlegrounds, maybe batter some zone flags but current known not 100% fit to this

    if(pvpInfo.inHostileArea)                               // in hostile area
    {
        if(!IsPvP() || pvpInfo.endTimer != 0)
            UpdatePvP(true, true);
    }
    else                                                    // in friendly area
    {
        if(IsPvP() && (isInPvPZone() || (!HasFlag(PLAYER_FLAGS,PLAYER_FLAGS_IN_PVP) && pvpInfo.endTimer == 0)) )
            pvpInfo.endTimer = time(0);                     // start toggle-off
    }

    if(zone->flags & AREA_FLAG_SANCTUARY || (World::IsZoneSanctuary(zone->ID)))
        // in sanctuary
    {
        SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_SANCTUARY);
        if(sWorld.IsFFAPvPRealm())
            RemoveFlag(PLAYER_FLAGS,PLAYER_FLAGS_FFA_PVP);
    }
    else
    {
        RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_SANCTUARY);
    }

    if(zone->flags & AREA_FLAG_CAPITAL)                     // in capital city
    {
        SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_RESTING);
        SetRestType(REST_TYPE_IN_CITY);
        InnEnter(time(0),GetMapId(),0,0,0);

        if(sWorld.IsFFAPvPRealm())
            RemoveFlag(PLAYER_FLAGS,PLAYER_FLAGS_FFA_PVP);
    }
    else                                                    // anywhere else
    {
        if(HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_RESTING))     // but resting (walk from city or maybe in tavern or leave tavern recently)
        {
            if(GetRestType()==REST_TYPE_IN_TAVERN)          // has been in tavern. Is still in?
            {
                if(GetMapId()!=GetInnPosMapId() || sqrt((GetPositionX()-GetInnPosX())*(GetPositionX()-GetInnPosX())+(GetPositionY()-GetInnPosY())*(GetPositionY()-GetInnPosY())+(GetPositionZ()-GetInnPosZ())*(GetPositionZ()-GetInnPosZ()))>40)
                {
                    RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_RESTING);
                    SetRestType(REST_TYPE_NO);

                    if(sWorld.IsFFAPvPRealm())
                        SetFlag(PLAYER_FLAGS,PLAYER_FLAGS_FFA_PVP);
                }
            }
            else                                            // not in tavern (leave city then)
            {
                RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_RESTING);
                SetRestType(REST_TYPE_NO);

                // Set player to FFA PVP when not in rested environment.
                if(sWorld.IsFFAPvPRealm())
                    SetFlag(PLAYER_FLAGS,PLAYER_FLAGS_FFA_PVP);
            }
        }
    }

    // remove items with area/map limitations (delete only for alive player to allow back in ghost mode)
    // if player resurrected at teleport this will be applied in resurrect code
    if(isAlive())
        DestroyZoneLimitedItem( true, newZone );

    // recent client version not send leave/join channel packets for built-in local channels
    UpdateLocalChannels( newZone );

    // group update
    if(GetGroup())
        SetGroupUpdateFlag(GROUP_UPDATE_FLAG_ZONE);

    UpdateZoneDependentAuras(newZone);
}

//If players are too far way of duel flag... then player loose the duel
void Player::CheckDuelDistance(time_t currTime)
{
    if(!duel)
        return;

    uint64 duelFlagGUID = GetUInt64Value(PLAYER_DUEL_ARBITER);
    GameObject* obj = ObjectAccessor::GetGameObject(*this, duelFlagGUID);
    if(!obj)
        return;

    if(duel->outOfBound == 0)
    {
        if(!IsWithinDistInMap(obj, 65))
        {
            duel->outOfBound = currTime;

            WorldPacket data(SMSG_DUEL_OUTOFBOUNDS, 0);
            GetSession()->SendPacket(&data);
        }
    }
    else
    {
        if(IsWithinDistInMap(obj, 65))
        {
            duel->outOfBound = 0;

            WorldPacket data(SMSG_DUEL_INBOUNDS, 0);
            GetSession()->SendPacket(&data);
        }
        else if(currTime >= (duel->outOfBound+10))
        {
            DuelComplete(DUEL_FLED);
        }
    }
}

bool Player::IsOutdoorPvPActive()
{
    return (isAlive() && !HasInvisibilityAura() && !HasStealthAura() && (HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_IN_PVP) || sWorld.IsPvPRealm())  && !HasUnitMovementFlag(MOVEMENTFLAG_FLYING2) && !isInFlight());
}

void Player::DuelComplete(DuelCompleteType type)
{
    // duel not requested
    if(!duel)
        return;

    WorldPacket data(SMSG_DUEL_COMPLETE, (1));
    data << (uint8)((type != DUEL_INTERUPTED) ? 1 : 0);
    GetSession()->SendPacket(&data);
    duel->opponent->GetSession()->SendPacket(&data);

    if(type != DUEL_INTERUPTED)
    {
        data.Initialize(SMSG_DUEL_WINNER, (1+20));          // we guess size
        data << (uint8)((type==DUEL_WON) ? 0 : 1);          // 0 = just won; 1 = fled
        data << duel->opponent->GetName();
        data << GetName();
        SendMessageToSet(&data,true);
    }

    // cool-down duel spell
    /*data.Initialize(SMSG_SPELL_COOLDOWN, 17);

    data<<GetGUID();
    data<<uint8(0x0);

    data<<(uint32)7266;
    data<<uint32(0x0);
    GetSession()->SendPacket(&data);
    data.Initialize(SMSG_SPELL_COOLDOWN, 17);
    data<<duel->opponent->GetGUID();
    data<<uint8(0x0);
    data<<(uint32)7266;
    data<<uint32(0x0);
    duel->opponent->GetSession()->SendPacket(&data);*/

    //Remove Duel Flag object
    GameObject* obj = ObjectAccessor::GetGameObject(*this, GetUInt64Value(PLAYER_DUEL_ARBITER));
    if(obj)
        duel->initiator->RemoveGameObject(obj,true);

    /* remove auras */
    std::vector<uint32> auras2remove;
    AuraMap const& vAuras = duel->opponent->GetAuras();
    for (AuraMap::const_iterator i = vAuras.begin(); i != vAuras.end(); ++i)
    {
        if (!i->second->IsPositive() && i->second->GetCasterGUID() == GetGUID() && i->second->GetAuraApplyTime() >= duel->startTime)
            auras2remove.push_back(i->second->GetId());
    }

    for(size_t i=0; i<auras2remove.size(); i++)
        duel->opponent->RemoveAurasDueToSpell(auras2remove[i]);

    auras2remove.clear();
    AuraMap const& auras = GetAuras();
    for (AuraMap::const_iterator i = auras.begin(); i != auras.end(); ++i)
    {
        if (!i->second->IsPositive() && i->second->GetCasterGUID() == duel->opponent->GetGUID() && i->second->GetAuraApplyTime() >= duel->startTime)
            auras2remove.push_back(i->second->GetId());
    }
    for(size_t i=0; i<auras2remove.size(); i++)
        RemoveAurasDueToSpell(auras2remove[i]);

    // cleanup combo points
    if(GetComboTarget()==duel->opponent->GetGUID())
        ClearComboPoints();
    else if(GetComboTarget()==duel->opponent->GetPetGUID())
        ClearComboPoints();

    if(duel->opponent->GetComboTarget()==GetGUID())
        duel->opponent->ClearComboPoints();
    else if(duel->opponent->GetComboTarget()==GetPetGUID())
        duel->opponent->ClearComboPoints();

    // Honor points after duel (the winner) - ImpConfig
    if(uint32 amount = sWorld.getConfig(CONFIG_HONOR_AFTER_DUEL))
        duel->opponent->RewardHonor(NULL,1,amount);

    // Refresh in PvPZone
    if(isInPvPZone())
    {
        SetHealth(GetMaxHealth());
        if(Pet* pet = GetPet())
            pet->SetHealth(pet->GetMaxHealth());
        if(getPowerType() == POWER_MANA || getClass() == CLASS_DRUID)
            SetPower(POWER_MANA,GetMaxPower(POWER_MANA));

        duel->opponent->SetHealth(duel->opponent->GetMaxHealth());
        if(Pet* pet = duel->opponent->GetPet())
            pet->SetHealth(pet->GetMaxHealth());
        if(duel->opponent->getPowerType() == POWER_MANA || getClass() == CLASS_DRUID)
            duel->opponent->SetPower(POWER_MANA,duel->opponent->GetMaxPower(POWER_MANA));
    }

    //cleanups
    SetUInt64Value(PLAYER_DUEL_ARBITER, 0);
    SetUInt32Value(PLAYER_DUEL_TEAM, 0);
    duel->opponent->SetUInt64Value(PLAYER_DUEL_ARBITER, 0);
    duel->opponent->SetUInt32Value(PLAYER_DUEL_TEAM, 0);

    delete duel->opponent->duel;
    duel->opponent->duel = NULL;
    delete duel;
    duel = NULL;
}

//---------------------------------------------------------//

void Player::_ApplyItemMods(Item *item, uint8 slot,bool apply)
{
    if(slot >= INVENTORY_SLOT_BAG_END || !item)
        return;

    // not apply mods for broken item
    if(item->IsBroken() && apply)
        return;

    ItemPrototype const *proto = item->GetProto();

    if(!proto)
        return;

    sLog.outDetail("applying mods for item %u ",item->GetGUIDLow());

    uint32 attacktype = Player::GetAttackBySlot(slot);
    if(attacktype < MAX_ATTACK)
        _ApplyWeaponDependentAuraMods(item,WeaponAttackType(attacktype),apply);

    _ApplyItemBonuses(proto,slot,apply);

    if( slot==EQUIPMENT_SLOT_RANGED )
        _ApplyAmmoBonuses();

    ApplyItemEquipSpell(item,apply);
    ApplyEnchantment(item, apply);

    if(proto->Socket[0].Color)                              //only (un)equipping of items with sockets can influence metagems, so no need to waste time with normal items
        CorrectMetaGemEnchants(slot, apply);
}

void Player::_ApplyItemBonuses(ItemPrototype const *proto,uint8 slot,bool apply)
{
    if(slot >= INVENTORY_SLOT_BAG_END || !proto)
        return;

    for (int i = 0; i < 10; i++)
    {
        float val = float (proto->ItemStat[i].ItemStatValue);

        if(val==0)
            continue;

        switch (proto->ItemStat[i].ItemStatType)
        {
            case ITEM_MOD_MANA:
                HandleStatModifier(UNIT_MOD_MANA, BASE_VALUE, float(val), apply);
                break;
            case ITEM_MOD_HEALTH:                           // modify HP
                HandleStatModifier(UNIT_MOD_HEALTH, BASE_VALUE, float(val), apply);
                break;
            case ITEM_MOD_AGILITY:                          // modify agility
                HandleStatModifier(UNIT_MOD_STAT_AGILITY, BASE_VALUE, float(val), apply);
                ApplyStatBuffMod(STAT_AGILITY, float(val), apply);
                break;
            case ITEM_MOD_STRENGTH:                         //modify strength
                HandleStatModifier(UNIT_MOD_STAT_STRENGTH, BASE_VALUE, float(val), apply);
                ApplyStatBuffMod(STAT_STRENGTH, float(val), apply);
                break;
            case ITEM_MOD_INTELLECT:                        //modify intellect
                HandleStatModifier(UNIT_MOD_STAT_INTELLECT, BASE_VALUE, float(val), apply);
                ApplyStatBuffMod(STAT_INTELLECT, float(val), apply);
                break;
            case ITEM_MOD_SPIRIT:                           //modify spirit
                HandleStatModifier(UNIT_MOD_STAT_SPIRIT, BASE_VALUE, float(val), apply);
                ApplyStatBuffMod(STAT_SPIRIT, float(val), apply);
                break;
            case ITEM_MOD_STAMINA:                          //modify stamina
                HandleStatModifier(UNIT_MOD_STAT_STAMINA, BASE_VALUE, float(val), apply);
                ApplyStatBuffMod(STAT_STAMINA, float(val), apply);
                break;
            case ITEM_MOD_DEFENSE_SKILL_RATING:
                ApplyRatingMod(CR_DEFENSE_SKILL, int32(val), apply);
                break;
            case ITEM_MOD_DODGE_RATING:
                ApplyRatingMod(CR_DODGE, int32(val), apply);
                break;
            case ITEM_MOD_PARRY_RATING:
                ApplyRatingMod(CR_PARRY, int32(val), apply);
                break;
            case ITEM_MOD_BLOCK_RATING:
                ApplyRatingMod(CR_BLOCK, int32(val), apply);
                break;
            case ITEM_MOD_HIT_MELEE_RATING:
                ApplyRatingMod(CR_HIT_MELEE, int32(val), apply);
                break;
            case ITEM_MOD_HIT_RANGED_RATING:
                ApplyRatingMod(CR_HIT_RANGED, int32(val), apply);
                break;
            case ITEM_MOD_HIT_SPELL_RATING:
                ApplyRatingMod(CR_HIT_SPELL, int32(val), apply);
                break;
            case ITEM_MOD_CRIT_MELEE_RATING:
                ApplyRatingMod(CR_CRIT_MELEE, int32(val), apply);
                break;
            case ITEM_MOD_CRIT_RANGED_RATING:
                ApplyRatingMod(CR_CRIT_RANGED, int32(val), apply);
                break;
            case ITEM_MOD_CRIT_SPELL_RATING:
                ApplyRatingMod(CR_CRIT_SPELL, int32(val), apply);
                break;
            case ITEM_MOD_HIT_TAKEN_MELEE_RATING:
                ApplyRatingMod(CR_HIT_TAKEN_MELEE, int32(val), apply);
                break;
            case ITEM_MOD_HIT_TAKEN_RANGED_RATING:
                ApplyRatingMod(CR_HIT_TAKEN_RANGED, int32(val), apply);
                break;
            case ITEM_MOD_HIT_TAKEN_SPELL_RATING:
                ApplyRatingMod(CR_HIT_TAKEN_SPELL, int32(val), apply);
                break;
            case ITEM_MOD_CRIT_TAKEN_MELEE_RATING:
                ApplyRatingMod(CR_CRIT_TAKEN_MELEE, int32(val), apply);
                break;
            case ITEM_MOD_CRIT_TAKEN_RANGED_RATING:
                ApplyRatingMod(CR_CRIT_TAKEN_RANGED, int32(val), apply);
                break;
            case ITEM_MOD_CRIT_TAKEN_SPELL_RATING:
                ApplyRatingMod(CR_CRIT_TAKEN_SPELL, int32(val), apply);
                break;
            case ITEM_MOD_HASTE_MELEE_RATING:
                ApplyRatingMod(CR_HASTE_MELEE, int32(val), apply);
                break;
            case ITEM_MOD_HASTE_RANGED_RATING:
                ApplyRatingMod(CR_HASTE_RANGED, int32(val), apply);
                break;
            case ITEM_MOD_HASTE_SPELL_RATING:
                ApplyRatingMod(CR_HASTE_SPELL, int32(val), apply);
                break;
            case ITEM_MOD_HIT_RATING:
                ApplyRatingMod(CR_HIT_MELEE, int32(val), apply);
                ApplyRatingMod(CR_HIT_RANGED, int32(val), apply);
                ApplyRatingMod(CR_HIT_SPELL, int32(val), apply);
                break;
            case ITEM_MOD_CRIT_RATING:
                ApplyRatingMod(CR_CRIT_MELEE, int32(val), apply);
                ApplyRatingMod(CR_CRIT_RANGED, int32(val), apply);
                ApplyRatingMod(CR_CRIT_SPELL, int32(val), apply);
                break;
            case ITEM_MOD_HIT_TAKEN_RATING:
                ApplyRatingMod(CR_HIT_TAKEN_MELEE, int32(val), apply);
                ApplyRatingMod(CR_HIT_TAKEN_RANGED, int32(val), apply);
                ApplyRatingMod(CR_HIT_TAKEN_SPELL, int32(val), apply);
                break;
            case ITEM_MOD_CRIT_TAKEN_RATING:
                ApplyRatingMod(CR_CRIT_TAKEN_MELEE, int32(val), apply);
                ApplyRatingMod(CR_CRIT_TAKEN_RANGED, int32(val), apply);
                ApplyRatingMod(CR_CRIT_TAKEN_SPELL, int32(val), apply);
                break;
            case ITEM_MOD_RESILIENCE_RATING:
                ApplyRatingMod(CR_CRIT_TAKEN_MELEE, int32(val), apply);
                ApplyRatingMod(CR_CRIT_TAKEN_RANGED, int32(val), apply);
                ApplyRatingMod(CR_CRIT_TAKEN_SPELL, int32(val), apply);
                break;
            case ITEM_MOD_HASTE_RATING:
                ApplyRatingMod(CR_HASTE_MELEE, int32(val), apply);
                ApplyRatingMod(CR_HASTE_RANGED, int32(val), apply);
                ApplyRatingMod(CR_HASTE_SPELL, int32(val), apply);
                break;
            case ITEM_MOD_EXPERTISE_RATING:
                ApplyRatingMod(CR_EXPERTISE, int32(val), apply);
                break;
        }
    }

    if (proto->Armor)
        HandleStatModifier(UNIT_MOD_ARMOR, BASE_VALUE, float(proto->Armor), apply);

    if (proto->Block)
        HandleBaseModValue(SHIELD_BLOCK_VALUE, FLAT_MOD, float(proto->Block), apply);

    if (proto->HolyRes)
        HandleStatModifier(UNIT_MOD_RESISTANCE_HOLY, BASE_VALUE, float(proto->HolyRes), apply);

    if (proto->FireRes)
        HandleStatModifier(UNIT_MOD_RESISTANCE_FIRE, BASE_VALUE, float(proto->FireRes), apply);

    if (proto->NatureRes)
        HandleStatModifier(UNIT_MOD_RESISTANCE_NATURE, BASE_VALUE, float(proto->NatureRes), apply);

    if (proto->FrostRes)
        HandleStatModifier(UNIT_MOD_RESISTANCE_FROST, BASE_VALUE, float(proto->FrostRes), apply);

    if (proto->ShadowRes)
        HandleStatModifier(UNIT_MOD_RESISTANCE_SHADOW, BASE_VALUE, float(proto->ShadowRes), apply);

    if (proto->ArcaneRes)
        HandleStatModifier(UNIT_MOD_RESISTANCE_ARCANE, BASE_VALUE, float(proto->ArcaneRes), apply);

    WeaponAttackType attType = BASE_ATTACK;
    float damage = 0.0f;

    if( slot == EQUIPMENT_SLOT_RANGED && (
        proto->InventoryType == INVTYPE_RANGED || proto->InventoryType == INVTYPE_THROWN ||
        proto->InventoryType == INVTYPE_RANGEDRIGHT ))
    {
        attType = RANGED_ATTACK;
    }
    else if(slot==EQUIPMENT_SLOT_OFFHAND)
    {
        attType = OFF_ATTACK;
    }

    if (proto->Damage[0].DamageMin > 0 )
    {
        damage = apply ? proto->Damage[0].DamageMin : BASE_MINDAMAGE;
        SetBaseWeaponDamage(attType, MINDAMAGE, damage);
        //sLog.outError("applying mindam: assigning %f to weapon mindamage, now is: %f", damage, GetWeaponDamageRange(attType, MINDAMAGE));
    }

    if (proto->Damage[0].DamageMax  > 0 )
    {
        damage = apply ? proto->Damage[0].DamageMax : BASE_MAXDAMAGE;
        SetBaseWeaponDamage(attType, MAXDAMAGE, damage);
    }

    if(!IsUseEquipedWeapon(slot==EQUIPMENT_SLOT_MAINHAND))
        return;

    if (proto->Delay)
    {
        if(slot == EQUIPMENT_SLOT_RANGED)
            SetAttackTime(RANGED_ATTACK, apply ? proto->Delay: BASE_ATTACK_TIME);
        else if(slot==EQUIPMENT_SLOT_MAINHAND)
            SetAttackTime(BASE_ATTACK, apply ? proto->Delay: BASE_ATTACK_TIME);
        else if(slot==EQUIPMENT_SLOT_OFFHAND)
            SetAttackTime(OFF_ATTACK, apply ? proto->Delay: BASE_ATTACK_TIME);
    }

    if(CanModifyStats() && (damage || proto->Delay))
        UpdateDamagePhysical(attType);
}

void Player::_ApplyWeaponDependentAuraMods(Item *item,WeaponAttackType attackType,bool apply)
{
    AuraList const& auraCritList = GetAurasByType(SPELL_AURA_MOD_CRIT_PERCENT);
    for(AuraList::const_iterator itr = auraCritList.begin(); itr!=auraCritList.end();++itr)
        _ApplyWeaponDependentAuraCritMod(item,attackType,*itr,apply);

    AuraList const& auraDamageFlatList = GetAurasByType(SPELL_AURA_MOD_DAMAGE_DONE);
    for(AuraList::const_iterator itr = auraDamageFlatList.begin(); itr!=auraDamageFlatList.end();++itr)
        _ApplyWeaponDependentAuraDamageMod(item,attackType,*itr,apply);

    AuraList const& auraDamagePCTList = GetAurasByType(SPELL_AURA_MOD_DAMAGE_PERCENT_DONE);
    for(AuraList::const_iterator itr = auraDamagePCTList.begin(); itr!=auraDamagePCTList.end();++itr)
        _ApplyWeaponDependentAuraDamageMod(item,attackType,*itr,apply);
}

void Player::_ApplyWeaponDependentAuraCritMod(Item *item, WeaponAttackType attackType, Aura* aura, bool apply)
{
    // generic not weapon specific case processes in aura code
    if(aura->GetSpellProto()->EquippedItemClass == -1)
        return;

    BaseModGroup mod = BASEMOD_END;
    switch(attackType)
    {
        case BASE_ATTACK:   mod = CRIT_PERCENTAGE;        break;
        case OFF_ATTACK:    mod = OFFHAND_CRIT_PERCENTAGE;break;
        case RANGED_ATTACK: mod = RANGED_CRIT_PERCENTAGE; break;
        default: return;
    }

    if (item->IsFitToSpellRequirements(aura->GetSpellProto()))
    {
        HandleBaseModValue(mod, FLAT_MOD, float (aura->GetModifierValue()), apply);
    }
}

void Player::_ApplyWeaponDependentAuraDamageMod(Item *item, WeaponAttackType attackType, Aura* aura, bool apply)
{
    // ignore spell mods for not wands
    Modifier const* modifier = aura->GetModifier();
    if((modifier->m_miscvalue & SPELL_SCHOOL_MASK_NORMAL)==0 && (getClassMask() & CLASSMASK_WAND_USERS)==0)
        return;

    // generic not weapon specific case processes in aura code
    if(aura->GetSpellProto()->EquippedItemClass == -1)
        return;

    UnitMods unitMod = UNIT_MOD_END;
    switch(attackType)
    {
        case BASE_ATTACK:   unitMod = UNIT_MOD_DAMAGE_MAINHAND; break;
        case OFF_ATTACK:    unitMod = UNIT_MOD_DAMAGE_OFFHAND;  break;
        case RANGED_ATTACK: unitMod = UNIT_MOD_DAMAGE_RANGED;   break;
        default: return;
    }

    UnitModifierType unitModType = TOTAL_VALUE;
    switch(modifier->m_auraname)
    {
        case SPELL_AURA_MOD_DAMAGE_DONE:         unitModType = TOTAL_VALUE; break;
        case SPELL_AURA_MOD_DAMAGE_PERCENT_DONE: unitModType = TOTAL_PCT;   break;
        default: return;
    }

    if (item->IsFitToSpellRequirements(aura->GetSpellProto()))
    {
        HandleStatModifier(unitMod, unitModType, float(aura->GetModifierValue()),apply);
    }
}

void Player::ApplyItemEquipSpell(Item *item, bool apply, bool form_change)
{
    if(!item)
        return;

    ItemPrototype const *proto = item->GetProto();
    if(!proto)
        return;

    for (int i = 0; i < 5; i++)
    {
        _Spell const& spellData = proto->Spells[i];

        // no spell
        if(!spellData.SpellId )
            continue;

        // wrong triggering type
        if(apply && spellData.SpellTrigger != ITEM_SPELLTRIGGER_ON_EQUIP)
            continue;

        // check if it is valid spell
        SpellEntry const* spellproto = spellmgr.LookupSpell(spellData.SpellId);
        if(!spellproto)
            continue;

        ApplyEquipSpell(spellproto,item,apply,form_change);
    }
}

void Player::ApplyEquipSpell(SpellEntry const* spellInfo, Item* item, bool apply, bool form_change)
{
    if(apply)
    {
        // Cannot be used in this stance/form
        if(GetErrorAtShapeshiftedCast(spellInfo, m_form)!=0)
            return;

        if(form_change)                                     // check aura active state from other form
        {
            bool found = false;
            for (int k=0; k < 3; ++k)
            {
                spellEffectPair spair = spellEffectPair(spellInfo->Id, k);
                for (AuraMap::iterator iter = m_Auras.lower_bound(spair); iter != m_Auras.upper_bound(spair); ++iter)
                {
                    if(!item || iter->second->GetCastItemGUID() == item->GetGUID())
                    {
                        found = true;
                        break;
                    }
                }
                if(found)
                    break;
            }

            if(found)                                       // and skip re-cast already active aura at form change
                return;
        }

        DEBUG_LOG("WORLD: cast %s Equip spellId - %i", (item ? "item" : "itemset"), spellInfo->Id);

        CastSpell(this,spellInfo,true,item);
    }
    else
    {
        if(form_change)                                     // check aura compatibility
        {
            // Cannot be used in this stance/form
            if(GetErrorAtShapeshiftedCast(spellInfo, m_form)==0)
                return;                                     // and remove only not compatible at form change
        }

        if(item)
            RemoveAurasDueToItemSpell(item,spellInfo->Id);  // un-apply all spells , not only at-equipped
        else
            RemoveAurasDueToSpell(spellInfo->Id);           // un-apply spell (item set case)
    }
}

void Player::UpdateEquipSpellsAtFormChange()
{
    for (int i = 0; i < INVENTORY_SLOT_BAG_END; i++)
    {
        if(m_items[i] && !m_items[i]->IsBroken())
        {
            ApplyItemEquipSpell(m_items[i],false,true);     // remove spells that not fit to form
            ApplyItemEquipSpell(m_items[i],true,true);      // add spells that fit form but not active
        }
    }

    // item set bonuses not dependent from item broken state
    for(size_t setindex = 0; setindex < ItemSetEff.size(); ++setindex)
    {
        ItemSetEffect* eff = ItemSetEff[setindex];
        if(!eff)
            continue;

        for(uint32 y=0;y<8; ++y)
        {
            SpellEntry const* spellInfo = eff->spells[y];
            if(!spellInfo)
                continue;

            ApplyEquipSpell(spellInfo,NULL,false,true);       // remove spells that not fit to form
            ApplyEquipSpell(spellInfo,NULL,true,true);        // add spells that fit form but not active
        }
    }
}
void Player::CastItemCombatSpell(Unit *target, WeaponAttackType attType, uint32 procVictim, uint32 procEx, SpellEntry const *spellInfo)
{

    if(spellInfo && ( (spellInfo->Attributes & SPELL_ATTR_STOP_ATTACK_TARGET) ||
      (spellInfo->DmgClass == SPELL_DAMAGE_CLASS_MAGIC || spellInfo->DmgClass == SPELL_DAMAGE_CLASS_NONE)) )
        return;

    if(!target || !target->isAlive() || target == this)
        return;

    for(int i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; i++)
    {
        // If usable, try to cast item spell
        if (Item * item = (this->ToPlayer())->GetItemByPos(INVENTORY_SLOT_BAG_0,i))
            if(!item->IsBroken())
                if (ItemPrototype const *proto = item->GetProto())
                {
                    // Additional check for weapons
                    if (proto->Class==ITEM_CLASS_WEAPON)
                    {
                        // offhand item cannot proc from main hand hit etc
                        EquipmentSlots slot;
                        switch (attType)
                        {
                            case BASE_ATTACK:   slot = EQUIPMENT_SLOT_MAINHAND; break;
                            case OFF_ATTACK:    slot = EQUIPMENT_SLOT_OFFHAND;  break;
                            case RANGED_ATTACK: slot = EQUIPMENT_SLOT_RANGED;   break;
                            default: slot = EQUIPMENT_SLOT_END; break;
                        }
                        if (slot != i)
                            continue;
                        // Check if item is useable (forms or disarm)
                        if (attType == BASE_ATTACK)
                        {
                            if (!(this->ToPlayer())->IsUseEquipedWeapon(true))
                                continue;
                        }
                        else
                        {
                            if ((this->ToPlayer())->IsInFeralForm())
                                continue;
                        }
                    }
                    (this->ToPlayer())->CastItemCombatSpell(target, attType, procVictim, procEx, item, proto, spellInfo);
                }
    }
}

void Player::CastItemCombatSpell(Unit *target, WeaponAttackType attType, uint32 procVictim, uint32 procEx, Item *item, ItemPrototype const * proto, SpellEntry const *spell)
{
    // Can do effect if any damage done to target
    if (procVictim & PROC_FLAG_TAKEN_ANY_DAMAGE)
    {
        for (int i = 0; i < 5; i++)
        {
            _Spell const& spellData = proto->Spells[i];

            // no spell
            if(!spellData.SpellId )
                continue;

            // wrong triggering type
            if(spellData.SpellTrigger != ITEM_SPELLTRIGGER_CHANCE_ON_HIT)
                continue;

            SpellEntry const *spellInfo = spellmgr.LookupSpell(spellData.SpellId);
            if(!spellInfo)
            {
                sLog.outError("WORLD: unknown Item spellid %i", spellData.SpellId);
                continue;
            }

            // not allow proc extra attack spell at extra attack
            if( m_extraAttacks && IsSpellHaveEffect(spellInfo, SPELL_EFFECT_ADD_EXTRA_ATTACKS) )
                return;

            float chance = spellInfo->procChance;

            if(spellData.SpellPPMRate)
            {
                uint32 WeaponSpeed = GetAttackTime(attType);
                chance = GetPPMProcChance(WeaponSpeed, spellData.SpellPPMRate);
            }
            else if(chance > 100.0f)
            {
                chance = GetWeaponProcChance();
            }

            if (roll_chance_f(chance))
                CastSpell(target, spellInfo->Id, true, item);
        }
    }

    // item combat enchantments
    for(int e_slot = 0; e_slot < MAX_ENCHANTMENT_SLOT; ++e_slot)
    {
        uint32 enchant_id = item->GetEnchantmentId(EnchantmentSlot(e_slot));
        SpellItemEnchantmentEntry const *pEnchant = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
        if(!pEnchant) continue;
        for (int s = 0; s < 3; ++s)
        {
            if(pEnchant->type[s] != ITEM_ENCHANTMENT_TYPE_COMBAT_SPELL)
            {
                uint32 FTSpellId=0;
                switch(pEnchant->spellid[s])
                {
					// Flametongue Weapon
                    case 10400:  FTSpellId = 8026; break; // Rank 1
                    case 15567: FTSpellId = 8028; break; // Rank 2
                    case 15568: FTSpellId = 8029; break; // Rank 3
                    case 15569: FTSpellId = 10445; break; // Rank 4
                    case 16311: FTSpellId = 16343; break; // Rank 5
                    case 16312: FTSpellId = 16344; break; // Rank 6
                    case 16313: FTSpellId = 25488; break; // Rank 7
                }
                if (FTSpellId)
                    CastSpell(target, FTSpellId, true, item);
                continue;
            }

            SpellEnchantProcEntry const* entry =  spellmgr.GetSpellEnchantProcEvent(enchant_id);
            if (entry && entry->procEx)
            {
                // Check hit/crit/dodge/parry requirement
                if((entry->procEx & procEx) == 0)
                    continue;
            }
            else
            {
                // Can do effect if any damage done to target
                if (!(procVictim & PROC_FLAG_TAKEN_ANY_DAMAGE))
                    continue;
            }

            SpellEntry const *spellInfo = spellmgr.LookupSpell(pEnchant->spellid[s]);
            if (!spellInfo)
            {
                sLog.outError("Player::CastItemCombatSpell Enchant %i, cast unknown spell %i", pEnchant->ID, pEnchant->spellid[s]);
                continue;
            }

            // do not allow proc windfury totem from yellow attacks
            if(spell && spellInfo->SpellFamilyName == SPELLFAMILY_SHAMAN && spellInfo->SpellFamilyFlags & 0x200000000LL)
                return;

            // not allow proc extra attack spell at extra attack
            if( m_extraAttacks && IsSpellHaveEffect(spellInfo, SPELL_EFFECT_ADD_EXTRA_ATTACKS) )
                return;

            float chance = pEnchant->amount[s] != 0 ? float(pEnchant->amount[s]) : GetWeaponProcChance();

            if (entry && entry->PPMChance)
            {
                uint32 WeaponSpeed = GetAttackTime(attType);
                chance = GetPPMProcChance(WeaponSpeed, entry->PPMChance);
            }
            else if (entry && entry->customChance)
                chance = entry->customChance;

            // Apply spell mods
            ApplySpellMod(pEnchant->spellid[s],SPELLMOD_CHANCE_OF_SUCCESS,chance);

            if (roll_chance_f(chance))
            {
                if(IsPositiveSpell(pEnchant->spellid[s]))
                    CastSpell(this, pEnchant->spellid[s], true, item);
                else
                    CastSpell(target, pEnchant->spellid[s], true, item);
            }
        }
    }
}

void Player::_RemoveAllItemMods()
{
    for (int i = 0; i < INVENTORY_SLOT_BAG_END; i++)
    {
        if(m_items[i])
        {
            ItemPrototype const *proto = m_items[i]->GetProto();
            if(!proto)
                continue;

            // item set bonuses not dependent from item broken state
            if(proto->ItemSet)
                RemoveItemsSetItem(this,proto);

            if(m_items[i]->IsBroken())
                continue;

            ApplyItemEquipSpell(m_items[i],false);
            ApplyEnchantment(m_items[i], false);
        }
    }

    for (int i = 0; i < INVENTORY_SLOT_BAG_END; i++)
    {
        if(m_items[i])
        {
            if(m_items[i]->IsBroken())
                continue;
            ItemPrototype const *proto = m_items[i]->GetProto();
            if(!proto)
                continue;

            uint32 attacktype = Player::GetAttackBySlot(i);
            if(attacktype < MAX_ATTACK)
                _ApplyWeaponDependentAuraMods(m_items[i],WeaponAttackType(attacktype),false);

            _ApplyItemBonuses(proto,i, false);

            if( i == EQUIPMENT_SLOT_RANGED )
                _ApplyAmmoBonuses();
        }
    }
}

void Player::_ApplyAllItemMods()
{
    for (int i = 0; i < INVENTORY_SLOT_BAG_END; i++)
    {
        if(m_items[i])
        {
            if(m_items[i]->IsBroken())
                continue;

            ItemPrototype const *proto = m_items[i]->GetProto();
            if(!proto)
                continue;

            uint32 attacktype = Player::GetAttackBySlot(i);
            if(attacktype < MAX_ATTACK)
                _ApplyWeaponDependentAuraMods(m_items[i],WeaponAttackType(attacktype),true);

            _ApplyItemBonuses(proto,i, true);

            if( i == EQUIPMENT_SLOT_RANGED )
                _ApplyAmmoBonuses();
        }
    }

    for (int i = 0; i < INVENTORY_SLOT_BAG_END; i++)
    {
        if(m_items[i])
        {
            ItemPrototype const *proto = m_items[i]->GetProto();
            if(!proto)
                continue;

            // item set bonuses not dependent from item broken state
            if(proto->ItemSet)
                AddItemsSetItem(this,m_items[i]);

            if(m_items[i]->IsBroken())
                continue;

            ApplyItemEquipSpell(m_items[i],true);
            ApplyEnchantment(m_items[i], true);
        }
    }
}

void Player::_ApplyAmmoBonuses()
{
    // check ammo
    uint32 ammo_id = GetUInt32Value(PLAYER_AMMO_ID);
    if(!ammo_id)
        return;

    float currentAmmoDPS;

    ItemPrototype const *ammo_proto = objmgr.GetItemPrototype( ammo_id );
    if( !ammo_proto || ammo_proto->Class!=ITEM_CLASS_PROJECTILE || !CheckAmmoCompatibility(ammo_proto))
        currentAmmoDPS = 0.0f;
    else
        currentAmmoDPS = ammo_proto->Damage[0].DamageMin;

    if(currentAmmoDPS == GetAmmoDPS())
        return;

    m_ammoDPS = currentAmmoDPS;

    if(CanModifyStats())
        UpdateDamagePhysical(RANGED_ATTACK);
}

bool Player::CheckAmmoCompatibility(const ItemPrototype *ammo_proto) const
{
    if(!ammo_proto)
        return false;

    // check ranged weapon
    Item *weapon = GetWeaponForAttack( RANGED_ATTACK );
    if(!weapon  || weapon->IsBroken() )
        return false;

    ItemPrototype const* weapon_proto = weapon->GetProto();
    if(!weapon_proto || weapon_proto->Class!=ITEM_CLASS_WEAPON )
        return false;

    // check ammo ws. weapon compatibility
    switch(weapon_proto->SubClass)
    {
        case ITEM_SUBCLASS_WEAPON_BOW:
        case ITEM_SUBCLASS_WEAPON_CROSSBOW:
            if(ammo_proto->SubClass!=ITEM_SUBCLASS_ARROW)
                return false;
            break;
        case ITEM_SUBCLASS_WEAPON_GUN:
            if(ammo_proto->SubClass!=ITEM_SUBCLASS_BULLET)
                return false;
            break;
        default:
            return false;
    }

    return true;
}

/*  If in a battleground a player dies, and an enemy removes the insignia, the player's bones is lootable
    Called by remove insignia spell effect    */
void Player::RemovedInsignia(Player* looterPlr)
{
    if (!GetBattleGroundId())
        return;

    // If not released spirit, do it !
    if(m_deathTimer > 0)
    {
        m_deathTimer = 0;
        BuildPlayerRepop();
        RepopAtGraveyard();
    }

    Corpse *corpse = GetCorpse();
    if (!corpse)
        return;

    // We have to convert player corpse to bones, not to be able to resurrect there
    // SpawnCorpseBones isn't handy, 'cos it saves player while he in BG
    Corpse *bones = ObjectAccessor::Instance().ConvertCorpseForPlayer(GetGUID(),true);
    if (!bones)
        return;

    // Now we must make bones lootable, and send player loot
    bones->SetFlag(CORPSE_FIELD_DYNAMIC_FLAGS, CORPSE_DYNFLAG_LOOTABLE);

    // We store the level of our player in the gold field
    // We retrieve this information at Player::SendLoot()
    bones->loot.gold = getLevel();
    bones->lootRecipient = looterPlr;
    looterPlr->SendLoot(bones->GetGUID(), LOOT_INSIGNIA);
}

/*Loot type MUST be
1-corpse, go
2-skinning
3-Fishing
*/

void Player::SendLootRelease( uint64 guid )
{
    WorldPacket data( SMSG_LOOT_RELEASE_RESPONSE, (8+1) );
    data << uint64(guid) << uint8(1);
    SendDirectMessage( &data );
}

void Player::SendLoot(uint64 guid, LootType loot_type)
{
    Loot    *loot = 0;
    PermissionTypes permission = ALL_PERMISSION;

    // release old loot
    if(uint64 lguid = GetLootGUID())
        GetSession()->DoLootRelease(lguid);

    if (IS_GAMEOBJECT_GUID(guid))
    {
        GameObject *go =
            ObjectAccessor::GetGameObject(*this, guid);

        // not check distance for GO in case owned GO (fishing bobber case, for example)
        // And permit out of range GO with no owner in case fishing hole
        if (!go || (loot_type != LOOT_FISHINGHOLE && (loot_type != LOOT_FISHING || go->GetOwnerGUID() != GetGUID()) && !go->IsInRange(GetPositionX(), GetPositionY(), GetPositionZ(),INTERACTION_DISTANCE)))
        {
            SendLootRelease(guid);
            return;
        }

        loot = &go->loot;

        if(go->getLootState() == GO_READY)
        {
            uint32 lootid =  go->GetLootId();

            //TODO: fix this big hack
            if((go->GetEntry() == BG_AV_OBJECTID_MINE_N || go->GetEntry() == BG_AV_OBJECTID_MINE_S))
                if( BattleGround *bg = GetBattleGround())
                    if(bg->GetTypeID() == BATTLEGROUND_AV)
                        if(!(((BattleGroundAV*)bg)->PlayerCanDoMineQuest(go->GetEntry(),GetTeam())))
                        {
                            SendLootRelease(guid);
                            return;
                        }

            if(lootid)
            {
                loot->clear();
                loot->FillLoot(lootid, LootTemplates_Gameobject, this);

                //if chest apply 2.1.x rules
                if((go->GetGoType() == GAMEOBJECT_TYPE_CHEST)&&(go->GetGOInfo()->chest.groupLootRules))
                {
                    if(Group* group = this->GetGroup())
                    {
                        group->UpdateLooterGuid((WorldObject*)go, true);

                        switch (group->GetLootMethod())
                        {
                            case GROUP_LOOT:
                                // we dont use a recipient, because any char at the correct distance can open a chest
                                group->GroupLoot(this->GetGUID(), loot, (WorldObject*) go);
                                break;
                            case NEED_BEFORE_GREED:
                                group->NeedBeforeGreed(this->GetGUID(), loot, (WorldObject*) go);
                                break;
                            case MASTER_LOOT:
                                group->MasterLoot(this->GetGUID(), loot, (WorldObject*) go);
                                break;
                            default:
                                break;
                        }
                    }
                }
            }

            if(loot_type == LOOT_FISHING)
                go->getFishLoot(loot);

            go->SetLootState(GO_ACTIVATED);
        }
    }
    else if (IS_ITEM_GUID(guid))
    {
        Item *item = GetItemByGuid( guid );

        if (!item)
        {
            SendLootRelease(guid);
            return;
        }

        if(loot_type == LOOT_DISENCHANTING)
        {
            loot = &item->loot;

            if(!item->m_lootGenerated)
            {
                item->m_lootGenerated = true;
                loot->clear();
                loot->FillLoot(item->GetProto()->DisenchantID, LootTemplates_Disenchant, this);
            }
        }
        else if(loot_type == LOOT_PROSPECTING)
        {
            loot = &item->loot;

            if(!item->m_lootGenerated)
            {
                item->m_lootGenerated = true;
                loot->clear();
                loot->FillLoot(item->GetEntry(), LootTemplates_Prospecting, this);
            }
        }
        else
        {
            loot = &item->loot;

            if(!item->m_lootGenerated)
            {
                item->m_lootGenerated = true;
                loot->clear();
                loot->FillLoot(item->GetEntry(), LootTemplates_Item, this);

                loot->generateMoneyLoot(item->GetProto()->MinMoneyLoot,item->GetProto()->MaxMoneyLoot);
            }
        }
    }
    else if (IS_CORPSE_GUID(guid))                          // remove insignia
    {
        Corpse *bones = ObjectAccessor::GetCorpse(*this, guid);

        if (!bones || !((loot_type == LOOT_CORPSE) || (loot_type == LOOT_INSIGNIA)) || (bones->GetType() != CORPSE_BONES) )
        {
            SendLootRelease(guid);
            return;
        }

        loot = &bones->loot;

        if (!bones->lootForBody)
        {
            bones->lootForBody = true;
            uint32 pLevel = bones->loot.gold;
            bones->loot.clear();
            if(GetBattleGround()->GetTypeID() == BATTLEGROUND_AV)
                loot->FillLoot(1, LootTemplates_Creature, this);
            // It may need a better formula
            // Now it works like this: lvl10: ~6copper, lvl70: ~9silver
            bones->loot.gold = (uint32)( GetMap()->urand(50, 150) * 0.016f * pow( ((float)pLevel)/5.76f, 2.5f) * sWorld.getRate(RATE_DROP_MONEY) );
        }

        if (bones->lootRecipient != this)
            permission = NONE_PERMISSION;
    }
    else
    {
        Creature *creature = ObjectAccessor::GetCreature(*this, guid);

        // must be in range and creature must be alive for pickpocket and must be dead for another loot
        if (!creature || creature->isAlive()!=(loot_type == LOOT_PICKPOCKETING) || !creature->IsWithinDistInMap(this,INTERACTION_DISTANCE))
        {
            SendLootRelease(guid);
            return;
        }

        if(loot_type == LOOT_PICKPOCKETING && IsFriendlyTo(creature))
        {
            SendLootRelease(guid);
            return;
        }
        
        if (creature->isWorldBoss() && !creature->IsAllowedToLoot(GetGUIDLow()))
        {
            SendLootRelease(guid);
            return;
        }

        loot   = &creature->loot;

        if(loot_type == LOOT_PICKPOCKETING)
        {
            if ( !creature->lootForPickPocketed )
            {
                creature->lootForPickPocketed = true;
                loot->clear();

                if (uint32 lootid = creature->GetCreatureInfo()->pickpocketLootId)
                    loot->FillLoot(lootid, LootTemplates_Pickpocketing, this);

                // Generate extra money for pick pocket loot
                const uint32 a = GetMap()->urand(0, creature->getLevel()/2);
                const uint32 b = GetMap()->urand(0, getLevel()/2);
                loot->gold = uint32(10 * (a + b) * sWorld.getRate(RATE_DROP_MONEY));
            }
        }
        else
        {
            // the player whose group may loot the corpse
            Player *recipient = creature->GetLootRecipient();
            if (!recipient)
            {
                creature->SetLootRecipient(this);
                recipient = this;
            }

            if (creature->lootForPickPocketed)
            {
                creature->lootForPickPocketed = false;
                loot->clear();
            }

            if(!creature->lootForBody)
            {
                creature->lootForBody = true;
                loot->clear();

                if (uint32 lootid = creature->GetCreatureInfo()->lootid)
                    loot->FillLoot(lootid, LootTemplates_Creature, recipient);

                loot->generateMoneyLoot(creature->GetCreatureInfo()->mingold,creature->GetCreatureInfo()->maxgold);

                if(Group* group = recipient->GetGroup())
                {
                    group->UpdateLooterGuid(creature,true);

                    switch (group->GetLootMethod())
                    {
                        case GROUP_LOOT:
                            // GroupLoot delete items over threshold (threshold even not implemented), and roll them. Items with quality<threshold, round robin
                            group->GroupLoot(recipient->GetGUID(), loot, creature);
                            break;
                        case NEED_BEFORE_GREED:
                            group->NeedBeforeGreed(recipient->GetGUID(), loot, creature);
                            break;
                        case MASTER_LOOT:
                            group->MasterLoot(recipient->GetGUID(), loot, creature);
                            break;
                        default:
                            break;
                    }
                }
            }

            // possible only if creature->lootForBody && loot->empty() at spell cast check
            if (loot_type == LOOT_SKINNING)
            {
                loot->clear();
                loot->FillLoot(creature->GetCreatureInfo()->SkinLootId, LootTemplates_Skinning, this);
            }
            // set group rights only for loot_type != LOOT_SKINNING
            else
            {
                if(Group* group = GetGroup())
                {
                    if( group == recipient->GetGroup() )
                    {
                        if(group->GetLootMethod() == FREE_FOR_ALL)
                            permission = ALL_PERMISSION;
                        else if(group->GetLooterGuid() == GetGUID())
                        {
                            if(group->GetLootMethod() == MASTER_LOOT)
                                permission = MASTER_PERMISSION;
                            else
                                permission = ALL_PERMISSION;
                        }
                        else
                            permission = GROUP_PERMISSION;
                    }
                    else
                        permission = NONE_PERMISSION;
                }
                else if(recipient == this)
                    permission = ALL_PERMISSION;
                else
                    permission = NONE_PERMISSION;
            }
        }
    }

    SetLootGUID(guid);

    QuestItemList *q_list = 0;
    if (permission != NONE_PERMISSION)
    {
        QuestItemMap const& lootPlayerQuestItems = loot->GetPlayerQuestItems();
        QuestItemMap::const_iterator itr = lootPlayerQuestItems.find(GetGUIDLow());
        if (itr == lootPlayerQuestItems.end())
            q_list = loot->FillQuestLoot(this);
        else
            q_list = itr->second;
    }

    QuestItemList *ffa_list = 0;
    if (permission != NONE_PERMISSION)
    {
        QuestItemMap const& lootPlayerFFAItems = loot->GetPlayerFFAItems();
        QuestItemMap::const_iterator itr = lootPlayerFFAItems.find(GetGUIDLow());
        if (itr == lootPlayerFFAItems.end())
            ffa_list = loot->FillFFALoot(this);
        else
            ffa_list = itr->second;
    }

    QuestItemList *conditional_list = 0;
    if (permission != NONE_PERMISSION)
    {
        QuestItemMap const& lootPlayerNonQuestNonFFAConditionalItems = loot->GetPlayerNonQuestNonFFAConditionalItems();
        QuestItemMap::const_iterator itr = lootPlayerNonQuestNonFFAConditionalItems.find(GetGUIDLow());
        if (itr == lootPlayerNonQuestNonFFAConditionalItems.end())
            conditional_list = loot->FillNonQuestNonFFAConditionalLoot(this);
        else
            conditional_list = itr->second;
    }

    // LOOT_PICKPOCKETING, LOOT_PROSPECTING, LOOT_DISENCHANTING and LOOT_INSIGNIA unsupported by client, sending LOOT_SKINNING instead
    if(loot_type == LOOT_PICKPOCKETING || loot_type == LOOT_DISENCHANTING || loot_type == LOOT_PROSPECTING || loot_type == LOOT_INSIGNIA)
        loot_type = LOOT_SKINNING;

    if(loot_type == LOOT_FISHINGHOLE)
        loot_type = LOOT_FISHING;

    WorldPacket data(SMSG_LOOT_RESPONSE, (9+50));           // we guess size

    data << uint64(guid);
    data << uint8(loot_type);
    data << LootView(*loot, q_list, ffa_list, conditional_list, this, permission);

    SendDirectMessage(&data);

    // add 'this' player as one of the players that are looting 'loot'
    if (permission != NONE_PERMISSION)
        loot->AddLooter(GetGUID());

    if ( loot_type == LOOT_CORPSE && !IS_ITEM_GUID(guid) )
        SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_LOOTING);
}

void Player::SendNotifyLootMoneyRemoved()
{
    WorldPacket data(SMSG_LOOT_CLEAR_MONEY, 0);
    GetSession()->SendPacket( &data );
}

void Player::SendNotifyLootItemRemoved(uint8 lootSlot)
{
    WorldPacket data(SMSG_LOOT_REMOVED, 1);
    data << uint8(lootSlot);
    GetSession()->SendPacket( &data );
}

void Player::SendUpdateWorldState(uint32 Field, uint32 Value)
{
    WorldPacket data(SMSG_UPDATE_WORLD_STATE, 8);
    data << Field;
    data << Value;
    GetSession()->SendPacket(&data);
}

void Player::SendInitWorldStates(bool forceZone, uint32 forceZoneId)
{
    // data depends on zoneid/mapid...
    BattleGround* bg = GetBattleGround();
    uint16 NumberOfFields = 0;
    uint32 mapid = GetMapId();
    uint32 zoneid;
    if(forceZone)
        zoneid = forceZoneId;
    else
        zoneid = GetZoneId();
    OutdoorPvP * pvp = sOutdoorPvPMgr.GetOutdoorPvPToZoneId(zoneid);
    uint32 areaid = GetAreaId();

    // may be exist better way to do this...
    switch(zoneid)
    {
        case 0:
        case 1:
        case 4:
        case 8:
        case 10:
        case 11:
        case 12:
        case 36:
        case 38:
        case 40:
        case 41:
        case 51:
        case 267:
        case 1519:
        case 1537:
        case 2257:
        case 2918:
            NumberOfFields = 6;
            break;
        case 139:
            NumberOfFields = 39;
            break;
        case 1377:
            NumberOfFields = 13;
            break;
        case 2597:
            NumberOfFields = 81;
            break;
        case 3277:
            NumberOfFields = 14;
            break;
        case 3358:
        case 3820:
            NumberOfFields = 38;
            break;
        case 3483:
            NumberOfFields = 25;
            break;
        case 3518:
            NumberOfFields = 37;
            break;
        case 3519:
            NumberOfFields = 36;
            break;
        case 3521:
            NumberOfFields = 35;
            break;
        case 3698:
        case 3702:
        case 3968:
            NumberOfFields = 9;
            break;
        case 3703:
            NumberOfFields = 9;
            break;
        default:
            NumberOfFields = 10;
            break;
    }

    WorldPacket data(SMSG_INIT_WORLD_STATES, (4+4+4+2+(NumberOfFields*8)));
    data << uint32(mapid);                                  // mapid
    data << uint32(zoneid);                                 // zone id
    data << uint32(areaid);                                 // area id, new 2.1.0
    data << uint16(NumberOfFields);                         // count of uint64 blocks
    data << uint32(0x8d8) << uint32(0x0);                   // 1
    data << uint32(0x8d7) << uint32(0x0);                   // 2
    data << uint32(0x8d6) << uint32(0x0);                   // 3
    data << uint32(0x8d5) << uint32(0x0);                   // 4
    data << uint32(0x8d4) << uint32(0x0);                   // 5
    data << uint32(0x8d3) << uint32(0x0);                   // 6
    if(mapid == 530)                                        // Outland
    {
        data << uint32(0x9bf) << uint32(0x0);               // 7
        data << uint32(0x9bd) << uint32(0xF);               // 8
        data << uint32(0x9bb) << uint32(0xF);               // 9
    }
    switch(zoneid)
    {
        case 1:
        case 11:
        case 12:
        case 38:
        case 40:
        case 51:
        case 1519:
        case 1537:
        case 2257:
            break;
        case 139: // EPL
            {
                if(pvp && pvp->GetTypeId() == OUTDOOR_PVP_EP)
                    pvp->FillInitialWorldStates(data);
                else
                {
                    data << uint32(0x97a) << uint32(0x0); // 10 2426
                    data << uint32(0x917) << uint32(0x0); // 11 2327
                    data << uint32(0x918) << uint32(0x0); // 12 2328
                    data << uint32(0x97b) << uint32(0x32); // 13 2427
                    data << uint32(0x97c) << uint32(0x32); // 14 2428
                    data << uint32(0x933) << uint32(0x1); // 15 2355
                    data << uint32(0x946) << uint32(0x0); // 16 2374
                    data << uint32(0x947) << uint32(0x0); // 17 2375
                    data << uint32(0x948) << uint32(0x0); // 18 2376
                    data << uint32(0x949) << uint32(0x0); // 19 2377
                    data << uint32(0x94a) << uint32(0x0); // 20 2378
                    data << uint32(0x94b) << uint32(0x0); // 21 2379
                    data << uint32(0x932) << uint32(0x0); // 22 2354
                    data << uint32(0x934) << uint32(0x0); // 23 2356
                    data << uint32(0x935) << uint32(0x0); // 24 2357
                    data << uint32(0x936) << uint32(0x0); // 25 2358
                    data << uint32(0x937) << uint32(0x0); // 26 2359
                    data << uint32(0x938) << uint32(0x0); // 27 2360
                    data << uint32(0x939) << uint32(0x1); // 28 2361
                    data << uint32(0x930) << uint32(0x1); // 29 2352
                    data << uint32(0x93a) << uint32(0x0); // 30 2362
                    data << uint32(0x93b) << uint32(0x0); // 31 2363
                    data << uint32(0x93c) << uint32(0x0); // 32 2364
                    data << uint32(0x93d) << uint32(0x0); // 33 2365
                    data << uint32(0x944) << uint32(0x0); // 34 2372
                    data << uint32(0x945) << uint32(0x0); // 35 2373
                    data << uint32(0x931) << uint32(0x1); // 36 2353
                    data << uint32(0x93e) << uint32(0x0); // 37 2366
                    data << uint32(0x931) << uint32(0x1); // 38 2367 ??  grey horde not in dbc! send for consistency's sake, and to match field count
                    data << uint32(0x940) << uint32(0x0); // 39 2368
                    data << uint32(0x941) << uint32(0x0); // 7 2369
                    data << uint32(0x942) << uint32(0x0); // 8 2370
                    data << uint32(0x943) << uint32(0x0); // 9 2371
                }
            }
            break;
        case 1377: // Silithus
            {
                if(pvp && pvp->GetTypeId() == OUTDOOR_PVP_SI)
                    pvp->FillInitialWorldStates(data);
                else
                {
                    // states are always shown
                    data << uint32(2313) << uint32(0x0); // 7 ally silityst gathered
                    data << uint32(2314) << uint32(0x0); // 8 horde silityst gathered
                    data << uint32(2317) << uint32(0x0); // 9 max silithyst
                }
                // dunno about these... aq opening event maybe?
                data << uint32(2322) << uint32(0x0); // 10 sandworm N
                data << uint32(2323) << uint32(0x0); // 11 sandworm S
                data << uint32(2324) << uint32(0x0); // 12 sandworm SW
                data << uint32(2325) << uint32(0x0); // 13 sandworm E
            }
            break;
        case 2597:                                          // AV
            if (bg && bg->GetTypeID() == BATTLEGROUND_AV)
                bg->FillInitialWorldStates(data);
            else
            {
                data << uint32(0x7ae) << uint32(0x1);           // 7 snowfall n
                data << uint32(0x532) << uint32(0x1);           // 8 frostwolfhut hc
                data << uint32(0x531) << uint32(0x0);           // 9 frostwolfhut ac
                data << uint32(0x52e) << uint32(0x0);           // 10 stormpike firstaid a_a
                data << uint32(0x571) << uint32(0x0);           // 11 east frostwolf tower horde assaulted -unused
                data << uint32(0x570) << uint32(0x0);           // 12 west frostwolf tower horde assaulted - unused
                data << uint32(0x567) << uint32(0x1);           // 13 frostwolfe c
                data << uint32(0x566) << uint32(0x1);           // 14 frostwolfw c
                data << uint32(0x550) << uint32(0x1);           // 15 irondeep (N) ally
                data << uint32(0x544) << uint32(0x0);           // 16 ice grave a_a
                data << uint32(0x536) << uint32(0x0);           // 17 stormpike grave h_c
                data << uint32(0x535) << uint32(0x1);           // 18 stormpike grave a_c
                data << uint32(0x518) << uint32(0x0);           // 19 stoneheart grave a_a
                data << uint32(0x517) << uint32(0x0);           // 20 stoneheart grave h_a
                data << uint32(0x574) << uint32(0x0);           // 21 1396 unk
                data << uint32(0x573) << uint32(0x0);           // 22 iceblood tower horde assaulted -unused
                data << uint32(0x572) << uint32(0x0);           // 23 towerpoint horde assaulted - unused
                data << uint32(0x56f) << uint32(0x0);           // 24 1391 unk
                data << uint32(0x56e) << uint32(0x0);           // 25 iceblood a
                data << uint32(0x56d) << uint32(0x0);           // 26 towerp a
                data << uint32(0x56c) << uint32(0x0);           // 27 frostwolfe a
                data << uint32(0x56b) << uint32(0x0);           // 28 froswolfw a
                data << uint32(0x56a) << uint32(0x1);           // 29 1386 unk
                data << uint32(0x569) << uint32(0x1);           // 30 iceblood c
                data << uint32(0x568) << uint32(0x1);           // 31 towerp c
                data << uint32(0x565) << uint32(0x0);           // 32 stoneh tower a
                data << uint32(0x564) << uint32(0x0);           // 33 icewing tower a
                data << uint32(0x563) << uint32(0x0);           // 34 dunn a
                data << uint32(0x562) << uint32(0x0);           // 35 duns a
                data << uint32(0x561) << uint32(0x0);           // 36 stoneheart bunker alliance assaulted - unused
                data << uint32(0x560) << uint32(0x0);           // 37 icewing bunker alliance assaulted - unused
                data << uint32(0x55f) << uint32(0x0);           // 38 dunbaldar south alliance assaulted - unused
                data << uint32(0x55e) << uint32(0x0);           // 39 dunbaldar north alliance assaulted - unused
                data << uint32(0x55d) << uint32(0x0);           // 40 stone tower d
                data << uint32(0x3c6) << uint32(0x0);           // 41 966 unk
                data << uint32(0x3c4) << uint32(0x0);           // 42 964 unk
                data << uint32(0x3c2) << uint32(0x0);           // 43 962 unk
                data << uint32(0x516) << uint32(0x1);           // 44 stoneheart grave a_c
                data << uint32(0x515) << uint32(0x0);           // 45 stonheart grave h_c
                data << uint32(0x3b6) << uint32(0x0);           // 46 950 unk
                data << uint32(0x55c) << uint32(0x0);           // 47 icewing tower d
                data << uint32(0x55b) << uint32(0x0);           // 48 dunn d
                data << uint32(0x55a) << uint32(0x0);           // 49 duns d
                data << uint32(0x559) << uint32(0x0);           // 50 1369 unk
                data << uint32(0x558) << uint32(0x0);           // 51 iceblood d
                data << uint32(0x557) << uint32(0x0);           // 52 towerp d
                data << uint32(0x556) << uint32(0x0);           // 53 frostwolfe d
                data << uint32(0x555) << uint32(0x0);           // 54 frostwolfw d
                data << uint32(0x554) << uint32(0x1);           // 55 stoneh tower c
                data << uint32(0x553) << uint32(0x1);           // 56 icewing tower c
                data << uint32(0x552) << uint32(0x1);           // 57 dunn c
                data << uint32(0x551) << uint32(0x1);           // 58 duns c
                data << uint32(0x54f) << uint32(0x0);           // 59 irondeep (N) horde
                data << uint32(0x54e) << uint32(0x0);           // 60 irondeep (N) ally
                data << uint32(0x54d) << uint32(0x1);           // 61 mine (S) neutral
                data << uint32(0x54c) << uint32(0x0);           // 62 mine (S) horde
                data << uint32(0x54b) << uint32(0x0);           // 63 mine (S) ally
                data << uint32(0x545) << uint32(0x0);           // 64 iceblood h_a
                data << uint32(0x543) << uint32(0x1);           // 65 iceblod h_c
                data << uint32(0x542) << uint32(0x0);           // 66 iceblood a_c
                data << uint32(0x540) << uint32(0x0);           // 67 snowfall h_a
                data << uint32(0x53f) << uint32(0x0);           // 68 snowfall a_a
                data << uint32(0x53e) << uint32(0x0);           // 69 snowfall h_c
                data << uint32(0x53d) << uint32(0x0);           // 70 snowfall a_c
                data << uint32(0x53c) << uint32(0x0);           // 71 frostwolf g h_a
                data << uint32(0x53b) << uint32(0x0);           // 72 frostwolf g a_a
                data << uint32(0x53a) << uint32(0x1);           // 73 frostwolf g h_c
                data << uint32(0x539) << uint32(0x0);           // 74 frostwolf g a_c
                data << uint32(0x538) << uint32(0x0);           // 75 stormpike grave h_a
                data << uint32(0x537) << uint32(0x0);           // 76 stormpike grave a_a
                data << uint32(0x534) << uint32(0x0);           // 77 frostwolf hut h_a
                data << uint32(0x533) << uint32(0x0);           // 78 frostwolf hut a_a
                data << uint32(0x530) << uint32(0x0);           // 79 stormpike first aid h_a
                data << uint32(0x52f) << uint32(0x0);           // 80 stormpike first aid h_c
                data << uint32(0x52d) << uint32(0x1);           // 81 stormpike first aid a_c
            }
            break;
        case 3277:                                          // WS
            if (bg && bg->GetTypeID() == BATTLEGROUND_WS)
                bg->FillInitialWorldStates(data);
            else
            {
                data << uint32(0x62d) << uint32(0x0);       // 7 1581 alliance flag captures
                data << uint32(0x62e) << uint32(0x0);       // 8 1582 horde flag captures
                data << uint32(0x609) << uint32(0x0);       // 9 1545 unk, set to 1 on alliance flag pickup...
                data << uint32(0x60a) << uint32(0x0);       // 10 1546 unk, set to 1 on horde flag pickup, after drop it's -1
                data << uint32(0x60b) << uint32(0x2);       // 11 1547 unk
                data << uint32(0x641) << uint32(0x3);       // 12 1601 unk (max flag captures?)
                data << uint32(0x922) << uint32(0x1);       // 13 2338 horde (0 - hide, 1 - flag ok, 2 - flag picked up (flashing), 3 - flag picked up (not flashing)
                data << uint32(0x923) << uint32(0x1);       // 14 2339 alliance (0 - hide, 1 - flag ok, 2 - flag picked up (flashing), 3 - flag picked up (not flashing)
            }
            break;
        case 3358:                                          // AB
            if (bg && bg->GetTypeID() == BATTLEGROUND_AB)
                bg->FillInitialWorldStates(data);
            else
            {
                data << uint32(0x6e7) << uint32(0x0);       // 7 1767 stables alliance
                data << uint32(0x6e8) << uint32(0x0);       // 8 1768 stables horde
                data << uint32(0x6e9) << uint32(0x0);       // 9 1769 unk, ST?
                data << uint32(0x6ea) << uint32(0x0);       // 10 1770 stables (show/hide)
                data << uint32(0x6ec) << uint32(0x0);       // 11 1772 farm (0 - horde controlled, 1 - alliance controlled)
                data << uint32(0x6ed) << uint32(0x0);       // 12 1773 farm (show/hide)
                data << uint32(0x6ee) << uint32(0x0);       // 13 1774 farm color
                data << uint32(0x6ef) << uint32(0x0);       // 14 1775 gold mine color, may be FM?
                data << uint32(0x6f0) << uint32(0x0);       // 15 1776 alliance resources
                data << uint32(0x6f1) << uint32(0x0);       // 16 1777 horde resources
                data << uint32(0x6f2) << uint32(0x0);       // 17 1778 horde bases
                data << uint32(0x6f3) << uint32(0x0);       // 18 1779 alliance bases
                data << uint32(0x6f4) << uint32(0x7d0);     // 19 1780 max resources (2000)
                data << uint32(0x6f6) << uint32(0x0);       // 20 1782 blacksmith color
                data << uint32(0x6f7) << uint32(0x0);       // 21 1783 blacksmith (show/hide)
                data << uint32(0x6f8) << uint32(0x0);       // 22 1784 unk, bs?
                data << uint32(0x6f9) << uint32(0x0);       // 23 1785 unk, bs?
                data << uint32(0x6fb) << uint32(0x0);       // 24 1787 gold mine (0 - horde contr, 1 - alliance contr)
                data << uint32(0x6fc) << uint32(0x0);       // 25 1788 gold mine (0 - conflict, 1 - horde)
                data << uint32(0x6fd) << uint32(0x0);       // 26 1789 gold mine (1 - show/0 - hide)
                data << uint32(0x6fe) << uint32(0x0);       // 27 1790 gold mine color
                data << uint32(0x700) << uint32(0x0);       // 28 1792 gold mine color, wtf?, may be LM?
                data << uint32(0x701) << uint32(0x0);       // 29 1793 lumber mill color (0 - conflict, 1 - horde contr)
                data << uint32(0x702) << uint32(0x0);       // 30 1794 lumber mill (show/hide)
                data << uint32(0x703) << uint32(0x0);       // 31 1795 lumber mill color color
                data << uint32(0x732) << uint32(0x1);       // 32 1842 stables (1 - uncontrolled)
                data << uint32(0x733) << uint32(0x1);       // 33 1843 gold mine (1 - uncontrolled)
                data << uint32(0x734) << uint32(0x1);       // 34 1844 lumber mill (1 - uncontrolled)
                data << uint32(0x735) << uint32(0x1);       // 35 1845 farm (1 - uncontrolled)
                data << uint32(0x736) << uint32(0x1);       // 36 1846 blacksmith (1 - uncontrolled)
                data << uint32(0x745) << uint32(0x2);       // 37 1861 unk
                data << uint32(0x7a3) << uint32(0x708);     // 38 1955 warning limit (1800)
            }
            break;
        case 3820:                                          // EY
            if (bg && bg->GetTypeID() == BATTLEGROUND_EY)
                bg->FillInitialWorldStates(data);
            else
            {
                data << uint32(0xac1) << uint32(0x0);       // 7  2753 Horde Bases
                data << uint32(0xac0) << uint32(0x0);       // 8  2752 Alliance Bases
                data << uint32(0xab6) << uint32(0x0);       // 9  2742 Mage Tower - Horde conflict
                data << uint32(0xab5) << uint32(0x0);       // 10 2741 Mage Tower - Alliance conflict
                data << uint32(0xab4) << uint32(0x0);       // 11 2740 Fel Reaver - Horde conflict
                data << uint32(0xab3) << uint32(0x0);       // 12 2739 Fel Reaver - Alliance conflict
                data << uint32(0xab2) << uint32(0x0);       // 13 2738 Draenei - Alliance conflict
                data << uint32(0xab1) << uint32(0x0);       // 14 2737 Draenei - Horde conflict
                data << uint32(0xab0) << uint32(0x0);       // 15 2736 unk // 0 at start
                data << uint32(0xaaf) << uint32(0x0);       // 16 2735 unk // 0 at start
                data << uint32(0xaad) << uint32(0x0);       // 17 2733 Draenei - Horde control
                data << uint32(0xaac) << uint32(0x0);       // 18 2732 Draenei - Alliance control
                data << uint32(0xaab) << uint32(0x1);       // 19 2731 Draenei uncontrolled (1 - yes, 0 - no)
                data << uint32(0xaaa) << uint32(0x0);       // 20 2730 Mage Tower - Alliance control
                data << uint32(0xaa9) << uint32(0x0);       // 21 2729 Mage Tower - Horde control
                data << uint32(0xaa8) << uint32(0x1);       // 22 2728 Mage Tower uncontrolled (1 - yes, 0 - no)
                data << uint32(0xaa7) << uint32(0x0);       // 23 2727 Fel Reaver - Horde control
                data << uint32(0xaa6) << uint32(0x0);       // 24 2726 Fel Reaver - Alliance control
                data << uint32(0xaa5) << uint32(0x1);       // 25 2725 Fel Reaver uncontrolled (1 - yes, 0 - no)
                data << uint32(0xaa4) << uint32(0x0);       // 26 2724 Boold Elf - Horde control
                data << uint32(0xaa3) << uint32(0x0);       // 27 2723 Boold Elf - Alliance control
                data << uint32(0xaa2) << uint32(0x1);       // 28 2722 Boold Elf uncontrolled (1 - yes, 0 - no)
                data << uint32(0xac5) << uint32(0x1);       // 29 2757 Flag (1 - show, 0 - hide) - doesn't work exactly this way!
                data << uint32(0xad2) << uint32(0x1);       // 30 2770 Horde top-stats (1 - show, 0 - hide) // 02 -> horde picked up the flag
                data << uint32(0xad1) << uint32(0x1);       // 31 2769 Alliance top-stats (1 - show, 0 - hide) // 02 -> alliance picked up the flag
                data << uint32(0xabe) << uint32(0x0);       // 32 2750 Horde resources
                data << uint32(0xabd) << uint32(0x0);       // 33 2749 Alliance resources
                data << uint32(0xa05) << uint32(0x8e);      // 34 2565 unk, constant?
                data << uint32(0xaa0) << uint32(0x0);       // 35 2720 Capturing progress-bar (100 -> empty (only grey), 0 -> blue|red (no grey), default 0)
                data << uint32(0xa9f) << uint32(0x0);       // 36 2719 Capturing progress-bar (0 - left, 100 - right)
                data << uint32(0xa9e) << uint32(0x0);       // 37 2718 Capturing progress-bar (1 - show, 0 - hide)
                data << uint32(0xc0d) << uint32(0x17b);     // 38 3085 unk
                // and some more ... unknown
            }
            break;
        // any of these needs change! the client remembers the prev setting!
        // ON EVERY ZONE LEAVE, RESET THE OLD ZONE'S WORLD STATE, BUT AT LEAST THE UI STUFF!
        case 3483:                                          // Hellfire Peninsula
            {
                if(pvp && pvp->GetTypeId() == OUTDOOR_PVP_HP)
                    pvp->FillInitialWorldStates(data);
                else
                {
                    data << uint32(0x9ba) << uint32(0x1);           // 10 // add ally tower main gui icon       // maybe should be sent only on login?
                    data << uint32(0x9b9) << uint32(0x1);           // 11 // add horde tower main gui icon      // maybe should be sent only on login?
                    data << uint32(0x9b5) << uint32(0x0);           // 12 // show neutral broken hill icon      // 2485
                    data << uint32(0x9b4) << uint32(0x1);           // 13 // show icon above broken hill        // 2484
                    data << uint32(0x9b3) << uint32(0x0);           // 14 // show ally broken hill icon         // 2483
                    data << uint32(0x9b2) << uint32(0x0);           // 15 // show neutral overlook icon         // 2482
                    data << uint32(0x9b1) << uint32(0x1);           // 16 // show the overlook arrow            // 2481
                    data << uint32(0x9b0) << uint32(0x0);           // 17 // show ally overlook icon            // 2480
                    data << uint32(0x9ae) << uint32(0x0);           // 18 // horde pvp objectives captured      // 2478
                    data << uint32(0x9ac) << uint32(0x0);           // 19 // ally pvp objectives captured       // 2476
                    data << uint32(2475)  << uint32(100); //: ally / horde slider grey area                              // show only in direct vicinity!
                    data << uint32(2474)  << uint32(50);  //: ally / horde slider percentage, 100 for ally, 0 for horde  // show only in direct vicinity!
                    data << uint32(2473)  << uint32(0);   //: ally / horde slider display                                // show only in direct vicinity!
                    data << uint32(0x9a8) << uint32(0x0);           // 20 // show the neutral stadium icon      // 2472
                    data << uint32(0x9a7) << uint32(0x0);           // 21 // show the ally stadium icon         // 2471
                    data << uint32(0x9a6) << uint32(0x1);           // 22 // show the horde stadium icon        // 2470
                }
            }
            break;
        case 3518:
            {
                if(pvp && pvp->GetTypeId() == OUTDOOR_PVP_NA)
                    pvp->FillInitialWorldStates(data);
                else
                {
                    data << uint32(2503) << uint32(0x0);    // 10
                    data << uint32(2502) << uint32(0x0);    // 11
                    data << uint32(2493) << uint32(0x0);    // 12
                    data << uint32(2491) << uint32(0x0);    // 13

                    data << uint32(2495) << uint32(0x0);    // 14
                    data << uint32(2494) << uint32(0x0);    // 15
                    data << uint32(2497) << uint32(0x0);    // 16

                    data << uint32(2762) << uint32(0x0);    // 17
                    data << uint32(2662) << uint32(0x0);    // 18
                    data << uint32(2663) << uint32(0x0);    // 19
                    data << uint32(2664) << uint32(0x0);    // 20

                    data << uint32(2760) << uint32(0x0);    // 21
                    data << uint32(2670) << uint32(0x0);    // 22
                    data << uint32(2668) << uint32(0x0);    // 23
                    data << uint32(2669) << uint32(0x0);    // 24

                    data << uint32(2761) << uint32(0x0);    // 25
                    data << uint32(2667) << uint32(0x0);    // 26
                    data << uint32(2665) << uint32(0x0);    // 27
                    data << uint32(2666) << uint32(0x0);    // 28

                    data << uint32(2763) << uint32(0x0);    // 29
                    data << uint32(2659) << uint32(0x0);    // 30
                    data << uint32(2660) << uint32(0x0);    // 31
                    data << uint32(2661) << uint32(0x0);    // 32

                    data << uint32(2671) << uint32(0x0);    // 33
                    data << uint32(2676) << uint32(0x0);    // 34
                    data << uint32(2677) << uint32(0x0);    // 35
                    data << uint32(2672) << uint32(0x0);    // 36
                    data << uint32(2673) << uint32(0x0);    // 37
                }
            }
            break;
        case 3519:                                          // Terokkar Forest
            {
                if(pvp && pvp->GetTypeId() == OUTDOOR_PVP_TF)
                    pvp->FillInitialWorldStates(data);
                else
                {
                    data << uint32(0xa41) << uint32(0x0);           // 10 // 2625 capture bar pos
                    data << uint32(0xa40) << uint32(0x14);          // 11 // 2624 capture bar neutral
                    data << uint32(0xa3f) << uint32(0x0);           // 12 // 2623 show capture bar
                    data << uint32(0xa3e) << uint32(0x0);           // 13 // 2622 horde towers controlled
                    data << uint32(0xa3d) << uint32(0x5);           // 14 // 2621 ally towers controlled
                    data << uint32(0xa3c) << uint32(0x0);           // 15 // 2620 show towers controlled
                    data << uint32(0xa88) << uint32(0x0);           // 16 // 2696 SE Neu
                    data << uint32(0xa87) << uint32(0x0);           // 17 // SE Horde
                    data << uint32(0xa86) << uint32(0x0);           // 18 // SE Ally
                    data << uint32(0xa85) << uint32(0x0);           // 19 //S Neu
                    data << uint32(0xa84) << uint32(0x0);           // 20 S Horde
                    data << uint32(0xa83) << uint32(0x0);           // 21 S Ally
                    data << uint32(0xa82) << uint32(0x0);           // 22 NE Neu
                    data << uint32(0xa81) << uint32(0x0);           // 23 NE Horde
                    data << uint32(0xa80) << uint32(0x0);           // 24 NE Ally
                    data << uint32(0xa7e) << uint32(0x0);           // 25 // 2686 N Neu
                    data << uint32(0xa7d) << uint32(0x0);           // 26 N Horde
                    data << uint32(0xa7c) << uint32(0x0);           // 27 N Ally
                    data << uint32(0xa7b) << uint32(0x0);           // 28 NW Ally
                    data << uint32(0xa7a) << uint32(0x0);           // 29 NW Horde
                    data << uint32(0xa79) << uint32(0x0);           // 30 NW Neutral
                    data << uint32(0x9d0) << uint32(0x5);           // 31 // 2512 locked time remaining seconds first digit
                    data << uint32(0x9ce) << uint32(0x0);           // 32 // 2510 locked time remaining seconds second digit
                    data << uint32(0x9cd) << uint32(0x0);           // 33 // 2509 locked time remaining minutes
                    data << uint32(0x9cc) << uint32(0x0);           // 34 // 2508 neutral locked time show
                    data << uint32(0xad0) << uint32(0x0);           // 35 // 2768 horde locked time show
                    data << uint32(0xacf) << uint32(0x1);           // 36 // 2767 ally locked time show
                }
            }
            break;
        case 3521:                                          // Zangarmarsh
            {
                if(pvp && pvp->GetTypeId() == OUTDOOR_PVP_ZM)
                    pvp->FillInitialWorldStates(data);
                else
                {
                    data << uint32(0x9e1) << uint32(0x0);           // 10 //2529
                    data << uint32(0x9e0) << uint32(0x0);           // 11
                    data << uint32(0x9df) << uint32(0x0);           // 12
                    data << uint32(0xa5d) << uint32(0x1);           // 13 //2653
                    data << uint32(0xa5c) << uint32(0x0);           // 14 //2652 east beacon neutral
                    data << uint32(0xa5b) << uint32(0x1);           // 15 horde
                    data << uint32(0xa5a) << uint32(0x0);           // 16 ally
                    data << uint32(0xa59) << uint32(0x1);           // 17 // 2649 Twin spire graveyard horde  12???
                    data << uint32(0xa58) << uint32(0x0);           // 18 ally     14 ???
                    data << uint32(0xa57) << uint32(0x0);           // 19 neutral  7???
                    data << uint32(0xa56) << uint32(0x0);           // 20 // 2646 west beacon neutral
                    data << uint32(0xa55) << uint32(0x1);           // 21 horde
                    data << uint32(0xa54) << uint32(0x0);           // 22 ally
                    data << uint32(0x9e7) << uint32(0x0);           // 23 // 2535
                    data << uint32(0x9e6) << uint32(0x0);           // 24
                    data << uint32(0x9e5) << uint32(0x0);           // 25
                    data << uint32(0xa00) << uint32(0x0);           // 26 // 2560
                    data << uint32(0x9ff) << uint32(0x1);           // 27
                    data << uint32(0x9fe) << uint32(0x0);           // 28
                    data << uint32(0x9fd) << uint32(0x0);           // 29
                    data << uint32(0x9fc) << uint32(0x1);           // 30
                    data << uint32(0x9fb) << uint32(0x0);           // 31
                    data << uint32(0xa62) << uint32(0x0);           // 32 // 2658
                    data << uint32(0xa61) << uint32(0x1);           // 33
                    data << uint32(0xa60) << uint32(0x1);           // 34
                    data << uint32(0xa5f) << uint32(0x0);           // 35
                }
            }
            break;
        case 3698:                                          // Nagrand Arena
            if (bg && bg->GetTypeID() == BATTLEGROUND_NA)
                bg->FillInitialWorldStates(data);
            else
            {
                data << uint32(0xa0f) << uint32(0x0);           // 7
                data << uint32(0xa10) << uint32(0x0);           // 8
                data << uint32(0xa11) << uint32(0x0);           // 9 show
            }
            break;
        case 3702:                                          // Blade's Edge Arena
            if (bg && bg->GetTypeID() == BATTLEGROUND_BE)
                bg->FillInitialWorldStates(data);
            else
            {
                data << uint32(0x9f0) << uint32(0x0);           // 7 gold
                data << uint32(0x9f1) << uint32(0x0);           // 8 green
                data << uint32(0x9f3) << uint32(0x0);           // 9 show
            }
            break;
        case 3968:                                          // Ruins of Lordaeron
            if (bg && bg->GetTypeID() == BATTLEGROUND_RL)
                bg->FillInitialWorldStates(data);
            else
            {
                data << uint32(0xbb8) << uint32(0x0);           // 7 gold
                data << uint32(0xbb9) << uint32(0x0);           // 8 green
                data << uint32(0xbba) << uint32(0x0);           // 9 show
            }
            break;
        case 3703:                                          // Shattrath City
            break;
        default:
            data << uint32(0x914) << uint32(0x0);           // 7
            data << uint32(0x913) << uint32(0x0);           // 8
            data << uint32(0x912) << uint32(0x0);           // 9
            data << uint32(0x915) << uint32(0x0);           // 10
            break;
    }
    GetSession()->SendPacket(&data);
}

uint32 Player::GetXPRestBonus(uint32 xp)
{
    uint32 rested_bonus = (uint32)GetRestBonus();           // xp for each rested bonus

    if(rested_bonus > xp)                                   // max rested_bonus == xp or (r+x) = 200% xp
        rested_bonus = xp;

    SetRestBonus( GetRestBonus() - rested_bonus);

    sLog.outDetail("Player gain %u xp (+ %u Rested Bonus). Rested points=%f",xp+rested_bonus,rested_bonus,GetRestBonus());
    return rested_bonus;
}

void Player::SetBindPoint(uint64 guid)
{
    WorldPacket data(SMSG_BINDER_CONFIRM, 8);
    data << uint64(guid);
    GetSession()->SendPacket( &data );
}

void Player::SendTalentWipeConfirm(uint64 guid)
{
    WorldPacket data(MSG_TALENT_WIPE_CONFIRM, (8+4));
    data << uint64(guid);
    uint32 cost = sWorld.getConfig(CONFIG_NO_RESET_TALENT_COST) ? 0 : resetTalentsCost();
    data << cost;
    GetSession()->SendPacket( &data );
}

void Player::SendPetSkillWipeConfirm()
{
    Pet* pet = GetPet();
    if(!pet)
        return;
    WorldPacket data(SMSG_PET_UNLEARN_CONFIRM, (8+4));
    data << pet->GetGUID();
    data << uint32(pet->resetTalentsCost());
    GetSession()->SendPacket( &data );
}

/*********************************************************/
/***                    STORAGE SYSTEM                 ***/
/*********************************************************/

void Player::SetVirtualItemSlot( uint8 i, Item* item)
{
    assert(i < 3);
    if(i < 2 && item)
    {
        if(!item->GetEnchantmentId(TEMP_ENCHANTMENT_SLOT))
            return;
        uint32 charges = item->GetEnchantmentCharges(TEMP_ENCHANTMENT_SLOT);
        if(charges == 0)
            return;
        if(charges > 1)
            item->SetEnchantmentCharges(TEMP_ENCHANTMENT_SLOT,charges-1);
        else if(charges <= 1)
        {
            ApplyEnchantment(item,TEMP_ENCHANTMENT_SLOT,false);
            item->ClearEnchantment(TEMP_ENCHANTMENT_SLOT);
        }
    }
}

void Player::SetSheath( uint32 sheathed )
{
    switch (sheathed)
    {
        case SHEATH_STATE_UNARMED:                          // no prepared weapon
            SetVirtualItemSlot(0,NULL);
            SetVirtualItemSlot(1,NULL);
            SetVirtualItemSlot(2,NULL);
            break;
        case SHEATH_STATE_MELEE:                            // prepared melee weapon
        {
            SetVirtualItemSlot(0,GetWeaponForAttack(BASE_ATTACK,true));
            SetVirtualItemSlot(1,GetWeaponForAttack(OFF_ATTACK,true));
            SetVirtualItemSlot(2,NULL);
        };  break;
        case SHEATH_STATE_RANGED:                           // prepared ranged weapon
            SetVirtualItemSlot(0,NULL);
            SetVirtualItemSlot(1,NULL);
            SetVirtualItemSlot(2,GetWeaponForAttack(RANGED_ATTACK,true));
            break;
        default:
            SetVirtualItemSlot(0,NULL);
            SetVirtualItemSlot(1,NULL);
            SetVirtualItemSlot(2,NULL);
            break;
    }
    SetByteValue(UNIT_FIELD_BYTES_2, 0, sheathed);          // this must visualize Sheath changing for other players...
}

uint8 Player::FindEquipSlot( ItemPrototype const* proto, uint32 slot, bool swap ) const
{
    uint8 pClass = getClass();

    uint8 slots[4];
    slots[0] = NULL_SLOT;
    slots[1] = NULL_SLOT;
    slots[2] = NULL_SLOT;
    slots[3] = NULL_SLOT;
    switch( proto->InventoryType )
    {
        case INVTYPE_HEAD:
            slots[0] = EQUIPMENT_SLOT_HEAD;
            break;
        case INVTYPE_NECK:
            slots[0] = EQUIPMENT_SLOT_NECK;
            break;
        case INVTYPE_SHOULDERS:
            slots[0] = EQUIPMENT_SLOT_SHOULDERS;
            break;
        case INVTYPE_BODY:
            slots[0] = EQUIPMENT_SLOT_BODY;
            break;
        case INVTYPE_CHEST:
            slots[0] = EQUIPMENT_SLOT_CHEST;
            break;
        case INVTYPE_ROBE:
            slots[0] = EQUIPMENT_SLOT_CHEST;
            break;
        case INVTYPE_WAIST:
            slots[0] = EQUIPMENT_SLOT_WAIST;
            break;
        case INVTYPE_LEGS:
            slots[0] = EQUIPMENT_SLOT_LEGS;
            break;
        case INVTYPE_FEET:
            slots[0] = EQUIPMENT_SLOT_FEET;
            break;
        case INVTYPE_WRISTS:
            slots[0] = EQUIPMENT_SLOT_WRISTS;
            break;
        case INVTYPE_HANDS:
            slots[0] = EQUIPMENT_SLOT_HANDS;
            break;
        case INVTYPE_FINGER:
            slots[0] = EQUIPMENT_SLOT_FINGER1;
            slots[1] = EQUIPMENT_SLOT_FINGER2;
            break;
        case INVTYPE_TRINKET:
            slots[0] = EQUIPMENT_SLOT_TRINKET1;
            slots[1] = EQUIPMENT_SLOT_TRINKET2;
            break;
        case INVTYPE_CLOAK:
            slots[0] =  EQUIPMENT_SLOT_BACK;
            break;
        case INVTYPE_WEAPON:
        {
            slots[0] = EQUIPMENT_SLOT_MAINHAND;

            // suggest offhand slot only if know dual wielding
            // (this will be replace mainhand weapon at auto equip instead unwonted "you don't known dual wielding" ...
            if(CanDualWield())
                slots[1] = EQUIPMENT_SLOT_OFFHAND;
        };break;
        case INVTYPE_SHIELD:
            slots[0] = EQUIPMENT_SLOT_OFFHAND;
            break;
        case INVTYPE_RANGED:
            slots[0] = EQUIPMENT_SLOT_RANGED;
            break;
        case INVTYPE_2HWEAPON:
            slots[0] = EQUIPMENT_SLOT_MAINHAND;
            break;
        case INVTYPE_TABARD:
            slots[0] = EQUIPMENT_SLOT_TABARD;
            break;
        case INVTYPE_WEAPONMAINHAND:
            slots[0] = EQUIPMENT_SLOT_MAINHAND;
            break;
        case INVTYPE_WEAPONOFFHAND:
            slots[0] = EQUIPMENT_SLOT_OFFHAND;
            break;
        case INVTYPE_HOLDABLE:
            slots[0] = EQUIPMENT_SLOT_OFFHAND;
            break;
        case INVTYPE_THROWN:
            slots[0] = EQUIPMENT_SLOT_RANGED;
            break;
        case INVTYPE_RANGEDRIGHT:
            slots[0] = EQUIPMENT_SLOT_RANGED;
            break;
        case INVTYPE_BAG:
            slots[0] = INVENTORY_SLOT_BAG_1;
            slots[1] = INVENTORY_SLOT_BAG_2;
            slots[2] = INVENTORY_SLOT_BAG_3;
            slots[3] = INVENTORY_SLOT_BAG_4;
            break;
        case INVTYPE_RELIC:
        {
            switch(proto->SubClass)
            {
                case ITEM_SUBCLASS_ARMOR_LIBRAM:
                    if (pClass == CLASS_PALADIN)
                        slots[0] = EQUIPMENT_SLOT_RANGED;
                    break;
                case ITEM_SUBCLASS_ARMOR_IDOL:
                    if (pClass == CLASS_DRUID)
                        slots[0] = EQUIPMENT_SLOT_RANGED;
                    break;
                case ITEM_SUBCLASS_ARMOR_TOTEM:
                    if (pClass == CLASS_SHAMAN)
                        slots[0] = EQUIPMENT_SLOT_RANGED;
                    break;
                case ITEM_SUBCLASS_ARMOR_MISC:
                    if (pClass == CLASS_WARLOCK)
                        slots[0] = EQUIPMENT_SLOT_RANGED;
                    break;
            }
            break;
        }
        default :
            return NULL_SLOT;
    }

    if( slot != NULL_SLOT )
    {
        if( swap || !GetItemByPos( INVENTORY_SLOT_BAG_0, slot ) )
        {
            for (int i = 0; i < 4; i++)
            {
                if ( slots[i] == slot )
                    return slot;
            }
        }
    }
    else
    {
        // search free slot at first
        for (int i = 0; i < 4; i++)
        {
            if ( slots[i] != NULL_SLOT && !GetItemByPos( INVENTORY_SLOT_BAG_0, slots[i] ) )
            {
                // in case 2hand equipped weapon offhand slot empty but not free
                if(slots[i]==EQUIPMENT_SLOT_OFFHAND)
                {
                    Item* mainItem = GetItemByPos( INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND );
                    if(!mainItem || mainItem->GetProto()->InventoryType != INVTYPE_2HWEAPON)
                        return slots[i];
                }
                else
                    return slots[i];
            }
        }

        // if not found free and can swap return first appropriate from used
        for (int i = 0; i < 4; i++)
        {
            if ( slots[i] != NULL_SLOT && swap )
                return slots[i];
        }
    }

    // no free position
    return NULL_SLOT;
}

uint8 Player::CanUnequipItems( uint32 item, uint32 count ) const
{
    Item *pItem;
    uint32 tempcount = 0;

    uint8 res = EQUIP_ERR_OK;

    for(int i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_BAG_END; i++)
    {
        pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
        if( pItem && pItem->GetEntry() == item )
        {
            uint8 ires = CanUnequipItem(INVENTORY_SLOT_BAG_0 << 8 | i, false);
            if(ires==EQUIP_ERR_OK)
            {
                tempcount += pItem->GetCount();
                if( tempcount >= count )
                    return EQUIP_ERR_OK;
            }
            else
                res = ires;
        }
    }
    for(int i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; i++)
    {
        pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
        if( pItem && pItem->GetEntry() == item )
        {
            tempcount += pItem->GetCount();
            if( tempcount >= count )
                return EQUIP_ERR_OK;
        }
    }
    for(int i = KEYRING_SLOT_START; i < KEYRING_SLOT_END; i++)
    {
        pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
        if( pItem && pItem->GetEntry() == item )
        {
            tempcount += pItem->GetCount();
            if( tempcount >= count )
                return EQUIP_ERR_OK;
        }
    }
    Bag *pBag;
    for(int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; i++)
    {
        pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, i );
        if( pBag )
        {
            for(uint32 j = 0; j < pBag->GetBagSize(); j++)
            {
                pItem = GetItemByPos( i, j );
                if( pItem && pItem->GetEntry() == item )
                {
                    tempcount += pItem->GetCount();
                    if( tempcount >= count )
                        return EQUIP_ERR_OK;
                }
            }
        }
    }

    // not found req. item count and have unequippable items
    return res;
}

uint32 Player::GetItemCount( uint32 item, bool inBankAlso, Item* skipItem ) const
{
    uint32 count = 0;
    for(int i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_ITEM_END; i++)
    {
        Item *pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
        if( pItem && pItem != skipItem &&  pItem->GetEntry() == item )
            count += pItem->GetCount();
    }
    for(int i = KEYRING_SLOT_START; i < KEYRING_SLOT_END; i++)
    {
        Item *pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
        if( pItem && pItem != skipItem && pItem->GetEntry() == item )
            count += pItem->GetCount();
    }
    for(int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; i++)
    {
        Bag* pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, i );
        if( pBag )
            count += pBag->GetItemCount(item,skipItem);
    }

    if(skipItem && skipItem->GetProto()->GemProperties)
    {
        for(int i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_ITEM_END; i++)
        {
            Item *pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
            if( pItem && pItem != skipItem && pItem->GetProto()->Socket[0].Color )
                count += pItem->GetGemCountWithID(item);
        }
    }

    if(inBankAlso)
    {
        for(int i = BANK_SLOT_ITEM_START; i < BANK_SLOT_ITEM_END; i++)
        {
            Item* pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
            if( pItem && pItem != skipItem && pItem->GetEntry() == item )
                count += pItem->GetCount();
        }
        for(int i = BANK_SLOT_BAG_START; i < BANK_SLOT_BAG_END; i++)
        {
            Bag* pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, i );
            if( pBag )
                count += pBag->GetItemCount(item,skipItem);
        }

        if(skipItem && skipItem->GetProto()->GemProperties)
        {
            for(int i = BANK_SLOT_ITEM_START; i < BANK_SLOT_ITEM_END; i++)
            {
                Item* pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
                if( pItem && pItem != skipItem && pItem->GetProto()->Socket[0].Color )
                    count += pItem->GetGemCountWithID(item);
            }
        }
    }

    return count;
}

Item* Player::GetItemByGuid( uint64 guid ) const
{
    for(int i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_ITEM_END; i++)
    {
        Item *pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
        if( pItem && pItem->GetGUID() == guid )
            return pItem;
    }
    for(int i = KEYRING_SLOT_START; i < KEYRING_SLOT_END; i++)
    {
        Item *pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
        if( pItem && pItem->GetGUID() == guid )
            return pItem;
    }

    for(int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; i++)
    {
        Bag *pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, i );
        if( pBag )
        {
            for(uint32 j = 0; j < pBag->GetBagSize(); j++)
            {
                Item* pItem = pBag->GetItemByPos( j );
                if( pItem && pItem->GetGUID() == guid )
                    return pItem;
            }
        }
    }
    for(int i = BANK_SLOT_BAG_START; i < BANK_SLOT_BAG_END; i++)
    {
        Bag *pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, i );
        if( pBag )
        {
            for(uint32 j = 0; j < pBag->GetBagSize(); j++)
            {
                Item* pItem = pBag->GetItemByPos( j );
                if( pItem && pItem->GetGUID() == guid )
                    return pItem;
            }
        }
    }

    return NULL;
}

Item* Player::GetItemByPos( uint16 pos ) const
{
    uint8 bag = pos >> 8;
    uint8 slot = pos & 255;
    return GetItemByPos( bag, slot );
}

Item* Player::GetItemByPos( uint8 bag, uint8 slot ) const
{
    if( bag == INVENTORY_SLOT_BAG_0 && ( slot < BANK_SLOT_BAG_END || slot >= KEYRING_SLOT_START && slot < KEYRING_SLOT_END ) )
        return m_items[slot];
    else if(bag >= INVENTORY_SLOT_BAG_START && bag < INVENTORY_SLOT_BAG_END
        || bag >= BANK_SLOT_BAG_START && bag < BANK_SLOT_BAG_END )
    {
        Bag *pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, bag );
        if ( pBag )
            return pBag->GetItemByPos(slot);
    }
    return NULL;
}

Item* Player::GetWeaponForAttack(WeaponAttackType attackType, bool useable) const
{
    uint16 slot;
    switch (attackType)
    {
        case BASE_ATTACK:   slot = EQUIPMENT_SLOT_MAINHAND; break;
        case OFF_ATTACK:    slot = EQUIPMENT_SLOT_OFFHAND;  break;
        case RANGED_ATTACK: slot = EQUIPMENT_SLOT_RANGED;   break;
        default: return NULL;
    }

    Item* item = GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
    if (!item || item->GetProto()->Class != ITEM_CLASS_WEAPON)
        return NULL;

    if(!useable)
        return item;

    if( item->IsBroken() || !IsUseEquipedWeapon(attackType==BASE_ATTACK) )
        return NULL;

    return item;
}

Item* Player::GetShield(bool useable) const
{
    Item* item = GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);
    if (!item || item->GetProto()->Class != ITEM_CLASS_ARMOR)
        return NULL;

    if(!useable)
        return item;

    if( item->IsBroken())
        return NULL;

    return item;
}

uint32 Player::GetAttackBySlot( uint8 slot )
{
    switch(slot)
    {
        case EQUIPMENT_SLOT_MAINHAND: return BASE_ATTACK;
        case EQUIPMENT_SLOT_OFFHAND:  return OFF_ATTACK;
        case EQUIPMENT_SLOT_RANGED:   return RANGED_ATTACK;
        default:                      return MAX_ATTACK;
    }
}

bool Player::HasBankBagSlot( uint8 slot ) const
{
    uint32 maxslot = GetByteValue(PLAYER_BYTES_2, 2) + BANK_SLOT_BAG_START;
    if( slot < maxslot )
        return true;
    return false;
}

bool Player::IsInventoryPos( uint8 bag, uint8 slot )
{
    if( bag == INVENTORY_SLOT_BAG_0 && slot == NULL_SLOT )
        return true;
    if( bag == INVENTORY_SLOT_BAG_0 && ( slot >= INVENTORY_SLOT_ITEM_START && slot < INVENTORY_SLOT_ITEM_END ) )
        return true;
    if( bag >= INVENTORY_SLOT_BAG_START && bag < INVENTORY_SLOT_BAG_END )
        return true;
    if( bag == INVENTORY_SLOT_BAG_0 && ( slot >= KEYRING_SLOT_START && slot < KEYRING_SLOT_END ) )
        return true;
    return false;
}

bool Player::IsEquipmentPos( uint8 bag, uint8 slot )
{
    if( bag == INVENTORY_SLOT_BAG_0 && ( slot < EQUIPMENT_SLOT_END ) )
        return true;
    if( bag == INVENTORY_SLOT_BAG_0 && ( slot >= INVENTORY_SLOT_BAG_START && slot < INVENTORY_SLOT_BAG_END ) )
        return true;
    return false;
}

bool Player::IsBankPos( uint8 bag, uint8 slot )
{
    if( bag == INVENTORY_SLOT_BAG_0 && ( slot >= BANK_SLOT_ITEM_START && slot < BANK_SLOT_ITEM_END ) )
        return true;
    if( bag == INVENTORY_SLOT_BAG_0 && ( slot >= BANK_SLOT_BAG_START && slot < BANK_SLOT_BAG_END ) )
        return true;
    if( bag >= BANK_SLOT_BAG_START && bag < BANK_SLOT_BAG_END )
        return true;
    return false;
}

bool Player::IsBagPos( uint16 pos )
{
    uint8 bag = pos >> 8;
    uint8 slot = pos & 255;
    if( bag == INVENTORY_SLOT_BAG_0 && ( slot >= INVENTORY_SLOT_BAG_START && slot < INVENTORY_SLOT_BAG_END ) )
        return true;
    if( bag == INVENTORY_SLOT_BAG_0 && ( slot >= BANK_SLOT_BAG_START && slot < BANK_SLOT_BAG_END ) )
        return true;
    return false;
}

bool Player::IsValidPos( uint8 bag, uint8 slot ) const
{
    // post selected
    if(bag == NULL_BAG)
        return true;

    if (bag == INVENTORY_SLOT_BAG_0)
    {
        // any post selected
        if (slot == NULL_SLOT)
            return true;

        // equipment
        if (slot < EQUIPMENT_SLOT_END)
            return true;

        // bag equip slots
        if (slot >= INVENTORY_SLOT_BAG_START && slot < INVENTORY_SLOT_BAG_END)
            return true;

        // backpack slots
        if (slot >= INVENTORY_SLOT_ITEM_START && slot < INVENTORY_SLOT_ITEM_END)
            return true;

        // keyring slots
        if (slot >= KEYRING_SLOT_START && slot < KEYRING_SLOT_END)
            return true;

        // bank main slots
        if (slot >= BANK_SLOT_ITEM_START && slot < BANK_SLOT_ITEM_END)
            return true;

        // bank bag slots
        if (slot >= BANK_SLOT_BAG_START && slot < BANK_SLOT_BAG_END)
            return true;

        return false;
    }

    // bag content slots
    if (bag >= INVENTORY_SLOT_BAG_START && bag < INVENTORY_SLOT_BAG_END)
    {
        Bag* pBag = (Bag*)GetItemByPos (INVENTORY_SLOT_BAG_0, bag);
        if(!pBag)
            return false;

        // any post selected
        if (slot == NULL_SLOT)
            return true;

        return slot < pBag->GetBagSize();
    }

    // bank bag content slots
    if( bag >= BANK_SLOT_BAG_START && bag < BANK_SLOT_BAG_END )
    {
        Bag* pBag = (Bag*)GetItemByPos (INVENTORY_SLOT_BAG_0, bag);
        if(!pBag)
            return false;

        // any post selected
        if (slot == NULL_SLOT)
            return true;

        return slot < pBag->GetBagSize();
    }

    // where this?
    return false;
}


bool Player::HasItemCount( uint32 item, uint32 count, bool inBankAlso ) const
{
    uint32 tempcount = 0;
    for(int i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_ITEM_END; i++)
    {
        Item *pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
        if( pItem && pItem->GetEntry() == item )
        {
            tempcount += pItem->GetCount();
            if( tempcount >= count )
                return true;
        }
    }
    for(int i = KEYRING_SLOT_START; i < KEYRING_SLOT_END; i++)
    {
        Item *pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
        if( pItem && pItem->GetEntry() == item )
        {
            tempcount += pItem->GetCount();
            if( tempcount >= count )
                return true;
        }
    }
    for(int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; i++)
    {
        if(Bag* pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, i ))
        {
            for(uint32 j = 0; j < pBag->GetBagSize(); j++)
            {
                Item* pItem = GetItemByPos( i, j );
                if( pItem && pItem->GetEntry() == item )
                {
                    tempcount += pItem->GetCount();
                    if( tempcount >= count )
                        return true;
                }
            }
        }
    }

    if(inBankAlso)
    {
        for(int i = BANK_SLOT_ITEM_START; i < BANK_SLOT_ITEM_END; i++)
        {
            Item *pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
            if( pItem && pItem->GetEntry() == item )
            {
                tempcount += pItem->GetCount();
                if( tempcount >= count )
                    return true;
            }
        }
        for(int i = BANK_SLOT_BAG_START; i < BANK_SLOT_BAG_END; i++)
        {
            if(Bag* pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, i ))
            {
                for(uint32 j = 0; j < pBag->GetBagSize(); j++)
                {
                    Item* pItem = GetItemByPos( i, j );
                    if( pItem && pItem->GetEntry() == item )
                    {
                        tempcount += pItem->GetCount();
                        if( tempcount >= count )
                            return true;
                    }
                }
            }
        }
    }

    return false;
}

uint32 Player::GetEmptyBagSlotsCount() const
{
    uint32 freeSlots = 0;
    
    for (int i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; i++) {
        Item* pItem = GetItemByPos(INVENTORY_SLOT_BAG_0, i);
        if (!pItem)
            ++freeSlots;
    }
    
    for (int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; i++) {
        if (Bag* pBag = (Bag*)GetItemByPos(INVENTORY_SLOT_BAG_0, i)) {
            for (uint32 j = 0; j < pBag->GetBagSize(); j++) {
                Item* pItem = GetItemByPos(i, j);
                if (!pItem)
                    ++freeSlots;
            }
        }
    }
    
    return freeSlots;
}

Item* Player::GetItemOrItemWithGemEquipped( uint32 item ) const
{
    Item *pItem;
    for(int i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; i++)
    {
        pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
        if( pItem && pItem->GetEntry() == item )
            return pItem;
    }

    ItemPrototype const *pProto = objmgr.GetItemPrototype(item);
    if (pProto && pProto->GemProperties)
    {
        for(int i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; i++)
        {
            pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
            if( pItem && pItem->GetProto()->Socket[0].Color )
            {
                if (pItem->GetGemCountWithID(item) > 0 )
                    return pItem;
            }
        }
    }

    return NULL;
}

uint8 Player::_CanTakeMoreSimilarItems(uint32 entry, uint32 count, Item* pItem, uint32* no_space_count ) const
{
    ItemPrototype const *pProto = objmgr.GetItemPrototype(entry);
    if( !pProto )
    {
        if(no_space_count)
            *no_space_count = count;
        return EQUIP_ERR_CANT_CARRY_MORE_OF_THIS;
    }

    if (pItem && pItem->m_lootGenerated)
        return EQUIP_ERR_ALREADY_LOOTED;

    // no maximum
    if(pProto->MaxCount == 0)
        return EQUIP_ERR_OK;

    uint32 curcount = GetItemCount(pProto->ItemId,true,pItem);

    if( curcount + count > pProto->MaxCount )
    {
        if(no_space_count)
            *no_space_count = count +curcount - pProto->MaxCount;
        return EQUIP_ERR_CANT_CARRY_MORE_OF_THIS;
    }

    return EQUIP_ERR_OK;
}

bool Player::HasItemTotemCategory( uint32 TotemCategory ) const
{
    Item *pItem;
    for (uint8 i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_ITEM_END; ++i) {
        pItem = GetItemByPos(INVENTORY_SLOT_BAG_0, i);
        if (pItem && IsTotemCategoryCompatibleWith(pItem->GetProto()->TotemCategory,TotemCategory, pItem))
            return true;
    }
    
    for (uint8 i = KEYRING_SLOT_START; i < KEYRING_SLOT_END; ++i) {
        pItem = GetItemByPos(INVENTORY_SLOT_BAG_0, i);
        if (pItem && IsTotemCategoryCompatibleWith(pItem->GetProto()->TotemCategory,TotemCategory, pItem))
            return true;
    }
    
    for(uint8 i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i) {
        if (Bag *pBag = (Bag*)GetItemByPos(INVENTORY_SLOT_BAG_0, i)) {
            for (uint32 j = 0; j < pBag->GetBagSize(); ++j) {
                pItem = GetItemByPos(i, j);
                if (pItem && IsTotemCategoryCompatibleWith(pItem->GetProto()->TotemCategory,TotemCategory, pItem))
                    return true;
            }
        }
    }
    return false;
}

uint8 Player::_CanStoreItem_InSpecificSlot( uint8 bag, uint8 slot, ItemPosCountVec &dest, ItemPrototype const *pProto, uint32& count, bool swap, Item* pSrcItem ) const
{
    Item* pItem2 = GetItemByPos( bag, slot );

    // ignore move item (this slot will be empty at move)
    if(pItem2==pSrcItem)
        pItem2 = NULL;

    uint32 need_space;

    // empty specific slot - check item fit to slot
    if( !pItem2 || swap )
    {
        if( bag == INVENTORY_SLOT_BAG_0 )
        {
            // keyring case
            if(slot >= KEYRING_SLOT_START && slot < KEYRING_SLOT_START+GetMaxKeyringSize() && !(pProto->BagFamily & BAG_FAMILY_MASK_KEYS))
                return EQUIP_ERR_ITEM_DOESNT_GO_INTO_BAG;

            // prevent cheating
            if(slot >= BUYBACK_SLOT_START && slot < BUYBACK_SLOT_END || slot >= PLAYER_SLOT_END)
                return EQUIP_ERR_ITEM_DOESNT_GO_INTO_BAG;
        }
        else
        {
            Bag* pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, bag );
            if( !pBag )
                return EQUIP_ERR_ITEM_DOESNT_GO_INTO_BAG;

            ItemPrototype const* pBagProto = pBag->GetProto();
            if( !pBagProto )
                return EQUIP_ERR_ITEM_DOESNT_GO_INTO_BAG;

            if( !ItemCanGoIntoBag(pProto,pBagProto) )
                return EQUIP_ERR_ITEM_DOESNT_GO_INTO_BAG;
        }

        // non empty stack with space
        need_space = pProto->Stackable;
    }
    // non empty slot, check item type
    else
    {
        // check item type
        if(pItem2->GetEntry() != pProto->ItemId)
            return EQUIP_ERR_ITEM_CANT_STACK;

        // check free space
        if(pItem2->GetCount() >= pProto->Stackable)
            return EQUIP_ERR_ITEM_CANT_STACK;

        need_space = pProto->Stackable - pItem2->GetCount();
    }

    if(need_space > count)
        need_space = count;

    ItemPosCount newPosition = ItemPosCount((bag << 8) | slot, need_space);
    if(!newPosition.isContainedIn(dest))
    {
        dest.push_back(newPosition);
        count -= need_space;
    }
    return EQUIP_ERR_OK;
}

uint8 Player::_CanStoreItem_InBag( uint8 bag, ItemPosCountVec &dest, ItemPrototype const *pProto, uint32& count, bool merge, bool non_specialized, Item* pSrcItem, uint8 skip_bag, uint8 skip_slot ) const
{
    // skip specific bag already processed in first called _CanStoreItem_InBag
    if(bag==skip_bag)
        return EQUIP_ERR_ITEM_DOESNT_GO_INTO_BAG;

    Bag* pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, bag );
    if( !pBag )
        return EQUIP_ERR_ITEM_DOESNT_GO_INTO_BAG;

    ItemPrototype const* pBagProto = pBag->GetProto();
    if( !pBagProto )
        return EQUIP_ERR_ITEM_DOESNT_GO_INTO_BAG;

    // specialized bag mode or non-specilized
    if( non_specialized != (pBagProto->Class == ITEM_CLASS_CONTAINER && pBagProto->SubClass == ITEM_SUBCLASS_CONTAINER) )
        return EQUIP_ERR_ITEM_DOESNT_GO_INTO_BAG;

    if( !ItemCanGoIntoBag(pProto,pBagProto) )
        return EQUIP_ERR_ITEM_DOESNT_GO_INTO_BAG;

    for(uint32 j = 0; j < pBag->GetBagSize(); j++)
    {
        // skip specific slot already processed in first called _CanStoreItem_InSpecificSlot
        if(j==skip_slot)
            continue;

        Item* pItem2 = GetItemByPos( bag, j );

        // ignore move item (this slot will be empty at move)
        if(pItem2==pSrcItem)
            pItem2 = NULL;

        // if merge skip empty, if !merge skip non-empty
        if((pItem2!=NULL)!=merge)
            continue;

        if( pItem2 )
        {
            if(pItem2->GetEntry() == pProto->ItemId && pItem2->GetCount() < pProto->Stackable )
            {
                uint32 need_space = pProto->Stackable - pItem2->GetCount();
                if(need_space > count)
                    need_space = count;

                ItemPosCount newPosition = ItemPosCount((bag << 8) | j, need_space);
                if(!newPosition.isContainedIn(dest))
                {
                    dest.push_back(newPosition);
                    count -= need_space;

                    if(count==0)
                        return EQUIP_ERR_OK;
                }
            }
        }
        else
        {
            uint32 need_space = pProto->Stackable;
            if(need_space > count)
                need_space = count;

            ItemPosCount newPosition = ItemPosCount((bag << 8) | j, need_space);
            if(!newPosition.isContainedIn(dest))
            {
                dest.push_back(newPosition);
                count -= need_space;

                if(count==0)
                    return EQUIP_ERR_OK;
            }
        }
    }
    return EQUIP_ERR_OK;
}

uint8 Player::_CanStoreItem_InInventorySlots( uint8 slot_begin, uint8 slot_end, ItemPosCountVec &dest, ItemPrototype const *pProto, uint32& count, bool merge, Item* pSrcItem, uint8 skip_bag, uint8 skip_slot ) const
{
    for(uint32 j = slot_begin; j < slot_end; j++)
    {
        // skip specific slot already processed in first called _CanStoreItem_InSpecificSlot
        if(INVENTORY_SLOT_BAG_0==skip_bag && j==skip_slot)
            continue;

        Item* pItem2 = GetItemByPos( INVENTORY_SLOT_BAG_0, j );

        // ignore move item (this slot will be empty at move)
        if(pItem2==pSrcItem)
            pItem2 = NULL;

        // if merge skip empty, if !merge skip non-empty
        if((pItem2!=NULL)!=merge)
            continue;

        if( pItem2 )
        {
            if(pItem2->GetEntry() == pProto->ItemId && pItem2->GetCount() < pProto->Stackable )
            {
                uint32 need_space = pProto->Stackable - pItem2->GetCount();
                if(need_space > count)
                    need_space = count;
                ItemPosCount newPosition = ItemPosCount((INVENTORY_SLOT_BAG_0 << 8) | j, need_space);
                if(!newPosition.isContainedIn(dest))
                {
                    dest.push_back(newPosition);
                    count -= need_space;

                    if(count==0)
                        return EQUIP_ERR_OK;
                }
            }
        }
        else
        {
            uint32 need_space = pProto->Stackable;
            if(need_space > count)
                need_space = count;

            ItemPosCount newPosition = ItemPosCount((INVENTORY_SLOT_BAG_0 << 8) | j, need_space);
            if(!newPosition.isContainedIn(dest))
            {
                dest.push_back(newPosition);
                count -= need_space;

                if(count==0)
                    return EQUIP_ERR_OK;
            }
        }
    }
    return EQUIP_ERR_OK;
}

uint8 Player::_CanStoreItem( uint8 bag, uint8 slot, ItemPosCountVec &dest, uint32 entry, uint32 count, Item *pItem, bool swap, uint32* no_space_count ) const
{
    ItemPrototype const *pProto = objmgr.GetItemPrototype(entry);
    if( !pProto )
    {
        if(no_space_count)
            *no_space_count = count;
        return swap ? EQUIP_ERR_ITEMS_CANT_BE_SWAPPED :EQUIP_ERR_ITEM_NOT_FOUND;
    }

    if(pItem && pItem->IsBindedNotWith(GetGUID()))
    {
        if(no_space_count)
            *no_space_count = count;
        return EQUIP_ERR_DONT_OWN_THAT_ITEM;
    }
    
    // Healthstones check
    /*static uint32 const itypes[6][3] = {
        { 5512,19004,19005},                        // Minor Healthstone
        { 5511,19006,19007},                        // Lesser Healthstone
        { 5509,19008,19009},                        // Healthstone
        { 5510,19010,19011},                        // Greater Healthstone
        { 9421,19012,19013},                        // Major Healthstone
        {22103,22104,22105}                         // Master Healthstone
    };
    bool isHealthstone = false;
    for (uint8 i = 0; i < 6 && !isHealthstone; i++) {
        for (uint8 j = 0; j < 3 && !isHealthstone; j++) {
            if (itypes[i][j] == entry)
                isHealthstone = true;
        }
    }
    if (isHealthstone) {
        for (uint8 i = 0; i < 6; i++) {
            for (uint8 j = 0; j < 3; j++) {
                if (HasItemCount(itypes[i][j], 1, true))
                    return EQUIP_ERR_CANT_CARRY_MORE_OF_THIS;
            }
        }
    }*/

    // check count of items (skip for auto move for same player from bank)
    uint32 no_similar_count = 0;                            // can't store this amount similar items
    uint8 res = _CanTakeMoreSimilarItems(entry,count,pItem,&no_similar_count);
    if(res!=EQUIP_ERR_OK)
    {
        if(count==no_similar_count)
        {
            if(no_space_count)
                *no_space_count = no_similar_count;
            return res;
        }
        count -= no_similar_count;
    }

    // in specific slot
    if( bag != NULL_BAG && slot != NULL_SLOT )
    {
        if(!IsValidPos(bag, slot))
            return EQUIP_ERR_ITEM_DOESNT_GO_TO_SLOT;
        res = _CanStoreItem_InSpecificSlot(bag,slot,dest,pProto,count,swap,pItem);
        if(res!=EQUIP_ERR_OK)
        {
            if(no_space_count)
                *no_space_count = count + no_similar_count;
            return res;
        }

        if(count==0)
        {
            if(no_similar_count==0)
                return EQUIP_ERR_OK;

            if(no_space_count)
                *no_space_count = count + no_similar_count;
            return EQUIP_ERR_CANT_CARRY_MORE_OF_THIS;
        }
    }

    // not specific slot or have space for partly store only in specific slot

    // in specific bag
    if( bag != NULL_BAG )
    {
        // search stack in bag for merge to
        if( pProto->Stackable > 1 )
        {
            if( bag == INVENTORY_SLOT_BAG_0 )               // inventory
            {
                res = _CanStoreItem_InInventorySlots(KEYRING_SLOT_START,KEYRING_SLOT_END,dest,pProto,count,true,pItem,bag,slot);
                if(res!=EQUIP_ERR_OK)
                {
                    if(no_space_count)
                        *no_space_count = count + no_similar_count;
                    return res;
                }

                if(count==0)
                {
                    if(no_similar_count==0)
                        return EQUIP_ERR_OK;

                    if(no_space_count)
                        *no_space_count = count + no_similar_count;
                    return EQUIP_ERR_CANT_CARRY_MORE_OF_THIS;
                }

                res = _CanStoreItem_InInventorySlots(INVENTORY_SLOT_ITEM_START,INVENTORY_SLOT_ITEM_END,dest,pProto,count,true,pItem,bag,slot);
                if(res!=EQUIP_ERR_OK)
                {
                    if(no_space_count)
                        *no_space_count = count + no_similar_count;
                    return res;
                }

                if(count==0)
                {
                    if(no_similar_count==0)
                        return EQUIP_ERR_OK;

                    if(no_space_count)
                        *no_space_count = count + no_similar_count;
                    return EQUIP_ERR_CANT_CARRY_MORE_OF_THIS;
                }
            }
            else                                            // equipped bag
            {
                // we need check 2 time (specialized/non_specialized), use NULL_BAG to prevent skipping bag
                res = _CanStoreItem_InBag(bag,dest,pProto,count,true,false,pItem,NULL_BAG,slot);
                if(res!=EQUIP_ERR_OK)
                    res = _CanStoreItem_InBag(bag,dest,pProto,count,true,true,pItem,NULL_BAG,slot);

                if(res!=EQUIP_ERR_OK)
                {
                    if(no_space_count)
                        *no_space_count = count + no_similar_count;
                    return res;
                }

                if(count==0)
                {
                    if(no_similar_count==0)
                        return EQUIP_ERR_OK;

                    if(no_space_count)
                        *no_space_count = count + no_similar_count;
                    return EQUIP_ERR_CANT_CARRY_MORE_OF_THIS;
                }
            }
        }

        // search free slot in bag for place to
        if( bag == INVENTORY_SLOT_BAG_0 )                   // inventory
        {
            // search free slot - keyring case
            if(pProto->BagFamily & BAG_FAMILY_MASK_KEYS)
            {
                uint32 keyringSize = GetMaxKeyringSize();
                res = _CanStoreItem_InInventorySlots(KEYRING_SLOT_START,KEYRING_SLOT_START+keyringSize,dest,pProto,count,false,pItem,bag,slot);
                if(res!=EQUIP_ERR_OK)
                {
                    if(no_space_count)
                        *no_space_count = count + no_similar_count;
                    return res;
                }

                if(count==0)
                {
                    if(no_similar_count==0)
                        return EQUIP_ERR_OK;

                    if(no_space_count)
                        *no_space_count = count + no_similar_count;
                    return EQUIP_ERR_CANT_CARRY_MORE_OF_THIS;
                }
            }

            res = _CanStoreItem_InInventorySlots(INVENTORY_SLOT_ITEM_START,INVENTORY_SLOT_ITEM_END,dest,pProto,count,false,pItem,bag,slot);
            if(res!=EQUIP_ERR_OK)
            {
                if(no_space_count)
                    *no_space_count = count + no_similar_count;
                return res;
            }

            if(count==0)
            {
                if(no_similar_count==0)
                    return EQUIP_ERR_OK;

                if(no_space_count)
                    *no_space_count = count + no_similar_count;
                return EQUIP_ERR_CANT_CARRY_MORE_OF_THIS;
            }
        }
        else                                                // equipped bag
        {
            res = _CanStoreItem_InBag(bag,dest,pProto,count,false,false,pItem,NULL_BAG,slot);
            if(res!=EQUIP_ERR_OK)
                res = _CanStoreItem_InBag(bag,dest,pProto,count,false,true,pItem,NULL_BAG,slot);

            if(res!=EQUIP_ERR_OK)
            {
                if(no_space_count)
                    *no_space_count = count + no_similar_count;
                return res;
            }

            if(count==0)
            {
                if(no_similar_count==0)
                    return EQUIP_ERR_OK;

                if(no_space_count)
                    *no_space_count = count + no_similar_count;
                return EQUIP_ERR_CANT_CARRY_MORE_OF_THIS;
            }
        }
    }

    // not specific bag or have space for partly store only in specific bag

    // search stack for merge to
    if( pProto->Stackable > 1 )
    {
        res = _CanStoreItem_InInventorySlots(KEYRING_SLOT_START,KEYRING_SLOT_END,dest,pProto,count,true,pItem,bag,slot);
        if(res!=EQUIP_ERR_OK)
        {
            if(no_space_count)
                *no_space_count = count + no_similar_count;
            return res;
        }

        if(count==0)
        {
            if(no_similar_count==0)
                return EQUIP_ERR_OK;

            if(no_space_count)
                *no_space_count = count + no_similar_count;
            return EQUIP_ERR_CANT_CARRY_MORE_OF_THIS;
        }

        res = _CanStoreItem_InInventorySlots(INVENTORY_SLOT_ITEM_START,INVENTORY_SLOT_ITEM_END,dest,pProto,count,true,pItem,bag,slot);
        if(res!=EQUIP_ERR_OK)
        {
            if(no_space_count)
                *no_space_count = count + no_similar_count;
            return res;
        }

        if(count==0)
        {
            if(no_similar_count==0)
                return EQUIP_ERR_OK;

            if(no_space_count)
                *no_space_count = count + no_similar_count;
            return EQUIP_ERR_CANT_CARRY_MORE_OF_THIS;
        }

        if( pProto->BagFamily )
        {
            for(int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; i++)
            {
                res = _CanStoreItem_InBag(i,dest,pProto,count,true,false,pItem,bag,slot);
                if(res!=EQUIP_ERR_OK)
                    continue;

                if(count==0)
                {
                    if(no_similar_count==0)
                        return EQUIP_ERR_OK;

                    if(no_space_count)
                        *no_space_count = count + no_similar_count;
                    return EQUIP_ERR_CANT_CARRY_MORE_OF_THIS;
                }
            }
        }

        for(int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; i++)
        {
            res = _CanStoreItem_InBag(i,dest,pProto,count,true,true,pItem,bag,slot);
            if(res!=EQUIP_ERR_OK)
                continue;

            if(count==0)
            {
                if(no_similar_count==0)
                    return EQUIP_ERR_OK;

                if(no_space_count)
                    *no_space_count = count + no_similar_count;
                return EQUIP_ERR_CANT_CARRY_MORE_OF_THIS;
            }
        }
    }

    // search free slot - special bag case
    if( pProto->BagFamily )
    {
        if(pProto->BagFamily & BAG_FAMILY_MASK_KEYS)
        {
            uint32 keyringSize = GetMaxKeyringSize();
            res = _CanStoreItem_InInventorySlots(KEYRING_SLOT_START,KEYRING_SLOT_START+keyringSize,dest,pProto,count,false,pItem,bag,slot);
            if(res!=EQUIP_ERR_OK)
            {
                if(no_space_count)
                    *no_space_count = count + no_similar_count;
                return res;
            }

            if(count==0)
            {
                if(no_similar_count==0)
                    return EQUIP_ERR_OK;

                if(no_space_count)
                    *no_space_count = count + no_similar_count;
                return EQUIP_ERR_CANT_CARRY_MORE_OF_THIS;
            }
        }

        for(int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; i++)
        {
            res = _CanStoreItem_InBag(i,dest,pProto,count,false,false,pItem,bag,slot);
            if(res!=EQUIP_ERR_OK)
                continue;

            if(count==0)
            {
                if(no_similar_count==0)
                    return EQUIP_ERR_OK;

                if(no_space_count)
                    *no_space_count = count + no_similar_count;
                return EQUIP_ERR_CANT_CARRY_MORE_OF_THIS;
            }
        }
    }

    // search free slot
    res = _CanStoreItem_InInventorySlots(INVENTORY_SLOT_ITEM_START,INVENTORY_SLOT_ITEM_END,dest,pProto,count,false,pItem,bag,slot);
    if(res!=EQUIP_ERR_OK)
    {
        if(no_space_count)
            *no_space_count = count + no_similar_count;
        return res;
    }

    if(count==0)
    {
        if(no_similar_count==0)
            return EQUIP_ERR_OK;

        if(no_space_count)
            *no_space_count = count + no_similar_count;
        return EQUIP_ERR_CANT_CARRY_MORE_OF_THIS;
    }

    for(int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; i++)
    {
        res = _CanStoreItem_InBag(i,dest,pProto,count,false,true,pItem,bag,slot);
        if(res!=EQUIP_ERR_OK)
            continue;

        if(count==0)
        {
            if(no_similar_count==0)
                return EQUIP_ERR_OK;

            if(no_space_count)
                *no_space_count = count + no_similar_count;
            return EQUIP_ERR_CANT_CARRY_MORE_OF_THIS;
        }
    }

    if(no_space_count)
        *no_space_count = count + no_similar_count;

    return EQUIP_ERR_INVENTORY_FULL;
}

//////////////////////////////////////////////////////////////////////////
uint8 Player::CanStoreItems( Item **pItems,int count) const
{
    Item    *pItem2;

    // fill space table
    int inv_slot_items[INVENTORY_SLOT_ITEM_END-INVENTORY_SLOT_ITEM_START];
    int inv_bags[INVENTORY_SLOT_BAG_END-INVENTORY_SLOT_BAG_START][MAX_BAG_SIZE];
    int inv_keys[KEYRING_SLOT_END-KEYRING_SLOT_START];

    memset(inv_slot_items,0,sizeof(int)*(INVENTORY_SLOT_ITEM_END-INVENTORY_SLOT_ITEM_START));
    memset(inv_bags,0,sizeof(int)*(INVENTORY_SLOT_BAG_END-INVENTORY_SLOT_BAG_START)*MAX_BAG_SIZE);
    memset(inv_keys,0,sizeof(int)*(KEYRING_SLOT_END-KEYRING_SLOT_START));

    for(int i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; i++)
    {
        pItem2 = GetItemByPos( INVENTORY_SLOT_BAG_0, i );

        if (pItem2 && !pItem2->IsInTrade())
        {
            inv_slot_items[i-INVENTORY_SLOT_ITEM_START] = pItem2->GetCount();
        }
    }

    for(int i = KEYRING_SLOT_START; i < KEYRING_SLOT_END; i++)
    {
        pItem2 = GetItemByPos( INVENTORY_SLOT_BAG_0, i );

        if (pItem2 && !pItem2->IsInTrade())
        {
            inv_keys[i-KEYRING_SLOT_START] = pItem2->GetCount();
        }
    }

    for(int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; i++)
    {
        if(Bag* pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, i ))
        {
            for(uint32 j = 0; j < pBag->GetBagSize(); j++)
            {
                pItem2 = GetItemByPos( i, j );
                if (pItem2 && !pItem2->IsInTrade())
                {
                    inv_bags[i-INVENTORY_SLOT_BAG_START][j] = pItem2->GetCount();
                }
            }
        }
    }

    // check free space for all items
    for (int k=0;k<count;k++)
    {
        Item  *pItem = pItems[k];

        // no item
        if (!pItem)  continue;

        ItemPrototype const *pProto = pItem->GetProto();

        // strange item
        if( !pProto )
            return EQUIP_ERR_ITEM_NOT_FOUND;

        // item it 'bind'
        if(pItem->IsBindedNotWith(GetGUID()))
            return EQUIP_ERR_DONT_OWN_THAT_ITEM;

        Bag *pBag;
        ItemPrototype const *pBagProto;

        // item is 'one item only'
        uint8 res = CanTakeMoreSimilarItems(pItem);
        if(res != EQUIP_ERR_OK)
            return res;

        // search stack for merge to
        if( pProto->Stackable > 1 )
        {
            bool b_found = false;

            for(int t = KEYRING_SLOT_START; t < KEYRING_SLOT_END; t++)
            {
                pItem2 = GetItemByPos( INVENTORY_SLOT_BAG_0, t );
                if( pItem2 && pItem2->GetEntry() == pItem->GetEntry() && inv_keys[t-KEYRING_SLOT_START] + pItem->GetCount() <= pProto->Stackable )
                {
                    inv_keys[t-KEYRING_SLOT_START] += pItem->GetCount();
                    b_found = true;
                    break;
                }
            }
            if (b_found) continue;

            for(int t = INVENTORY_SLOT_ITEM_START; t < INVENTORY_SLOT_ITEM_END; t++)
            {
                pItem2 = GetItemByPos( INVENTORY_SLOT_BAG_0, t );
                if( pItem2 && pItem2->GetEntry() == pItem->GetEntry() && inv_slot_items[t-INVENTORY_SLOT_ITEM_START] + pItem->GetCount() <= pProto->Stackable )
                {
                    inv_slot_items[t-INVENTORY_SLOT_ITEM_START] += pItem->GetCount();
                    b_found = true;
                    break;
                }
            }
            if (b_found) continue;

            for(int t = INVENTORY_SLOT_BAG_START; !b_found && t < INVENTORY_SLOT_BAG_END; t++)
            {
                pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, t );
                if( pBag )
                {
                    for(uint32 j = 0; j < pBag->GetBagSize(); j++)
                    {
                        pItem2 = GetItemByPos( t, j );
                        if( pItem2 && pItem2->GetEntry() == pItem->GetEntry() && inv_bags[t-INVENTORY_SLOT_BAG_START][j] + pItem->GetCount() <= pProto->Stackable )
                        {
                            inv_bags[t-INVENTORY_SLOT_BAG_START][j] += pItem->GetCount();
                            b_found = true;
                            break;
                        }
                    }
                }
            }
            if (b_found) continue;
        }

        // special bag case
        if( pProto->BagFamily )
        {
            bool b_found = false;
            if(pProto->BagFamily & BAG_FAMILY_MASK_KEYS)
            {
                uint32 keyringSize = GetMaxKeyringSize();
                for(uint32 t = KEYRING_SLOT_START; t < KEYRING_SLOT_START+keyringSize; ++t)
                {
                    if( inv_keys[t-KEYRING_SLOT_START] == 0 )
                    {
                        inv_keys[t-KEYRING_SLOT_START] = 1;
                        b_found = true;
                        break;
                    }
                }
            }

            if (b_found) continue;

            for(int t = INVENTORY_SLOT_BAG_START; !b_found && t < INVENTORY_SLOT_BAG_END; t++)
            {
                pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, t );
                if( pBag )
                {
                    pBagProto = pBag->GetProto();

                    // not plain container check
                    if( pBagProto && (pBagProto->Class != ITEM_CLASS_CONTAINER || pBagProto->SubClass != ITEM_SUBCLASS_CONTAINER) &&
                        ItemCanGoIntoBag(pProto,pBagProto) )
                    {
                        for(uint32 j = 0; j < pBag->GetBagSize(); j++)
                        {
                            if( inv_bags[t-INVENTORY_SLOT_BAG_START][j] == 0 )
                            {
                                inv_bags[t-INVENTORY_SLOT_BAG_START][j] = 1;
                                b_found = true;
                                break;
                            }
                        }
                    }
                }
            }
            if (b_found) continue;
        }

        // search free slot
        bool b_found = false;
        for(int t = INVENTORY_SLOT_ITEM_START; t < INVENTORY_SLOT_ITEM_END; t++)
        {
            if( inv_slot_items[t-INVENTORY_SLOT_ITEM_START] == 0 )
            {
                inv_slot_items[t-INVENTORY_SLOT_ITEM_START] = 1;
                b_found = true;
                break;
            }
        }
        if (b_found) continue;

        // search free slot in bags
        for(int t = INVENTORY_SLOT_BAG_START; !b_found && t < INVENTORY_SLOT_BAG_END; t++)
        {
            pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, t );
            if( pBag )
            {
                pBagProto = pBag->GetProto();

                // special bag already checked
                if( pBagProto && (pBagProto->Class != ITEM_CLASS_CONTAINER || pBagProto->SubClass != ITEM_SUBCLASS_CONTAINER))
                    continue;

                for(uint32 j = 0; j < pBag->GetBagSize(); j++)
                {
                    if( inv_bags[t-INVENTORY_SLOT_BAG_START][j] == 0 )
                    {
                        inv_bags[t-INVENTORY_SLOT_BAG_START][j] = 1;
                        b_found = true;
                        break;
                    }
                }
            }
        }

        // no free slot found?
        if (!b_found)
            return EQUIP_ERR_INVENTORY_FULL;
    }

    return EQUIP_ERR_OK;
}

//////////////////////////////////////////////////////////////////////////
uint8 Player::CanEquipNewItem( uint8 slot, uint16 &dest, uint32 item, bool swap ) const
{
    dest = 0;
    Item *pItem = Item::CreateItem( item, 1, this );
    if( pItem )
    {
        uint8 result = CanEquipItem(slot, dest, pItem, swap );
        delete pItem;
        return result;
    }

    return EQUIP_ERR_ITEM_NOT_FOUND;
}

uint8 Player::CanEquipItem( uint8 slot, uint16 &dest, Item *pItem, bool swap, bool not_loading ) const
{
    dest = 0;
    if( pItem )
    {
        ItemPrototype const *pProto = pItem->GetProto();
        if( pProto )
        {
            if(pItem->IsBindedNotWith(GetGUID()))
                return EQUIP_ERR_DONT_OWN_THAT_ITEM;

            // check count of items (skip for auto move for same player from bank)
            uint8 res = CanTakeMoreSimilarItems(pItem);
            if(res != EQUIP_ERR_OK)
                return res;

            // check this only in game
            if(not_loading)
            {
                // May be here should be more stronger checks; STUNNED checked
                // ROOT, CONFUSED, DISTRACTED, FLEEING this needs to be checked.
                if (hasUnitState(UNIT_STAT_STUNNED))
                    return EQUIP_ERR_YOU_ARE_STUNNED;

                // do not allow equipping gear except weapons, offhands, projectiles, relics in
                // - combat
                // - in-progress arenas
                if( !pProto->CanChangeEquipStateInCombat() )
                {
                    if( isInCombat() )
                        return EQUIP_ERR_NOT_IN_COMBAT;

                    if(BattleGround* bg = GetBattleGround())
                        if( bg->isArena() && bg->GetStatus() == STATUS_IN_PROGRESS )
                            return EQUIP_ERR_NOT_DURING_ARENA_MATCH;
                }

                if(isInCombat()&& pProto->Class == ITEM_CLASS_WEAPON && m_weaponChangeTimer != 0)
                    return EQUIP_ERR_CANT_DO_RIGHT_NOW;         // maybe exist better err

                if(IsNonMeleeSpellCasted(false))
                    return EQUIP_ERR_CANT_DO_RIGHT_NOW;
            }

            uint8 eslot = FindEquipSlot( pProto, slot, swap );
            if( eslot == NULL_SLOT )
                return EQUIP_ERR_ITEM_CANT_BE_EQUIPPED;

            uint8 msg = CanUseItem( pItem , not_loading );
            if( msg != EQUIP_ERR_OK )
                return msg;
            if( !swap && GetItemByPos( INVENTORY_SLOT_BAG_0, eslot ) )
                return EQUIP_ERR_NO_EQUIPMENT_SLOT_AVAILABLE;

            // check unique-equipped on item
            if (pProto->Flags & ITEM_FLAGS_UNIQUE_EQUIPPED)
            {
                // there is an equip limit on this item
                Item* tItem = GetItemOrItemWithGemEquipped(pProto->ItemId);
                if (tItem && (!swap || tItem->GetSlot() != eslot ) )
                    return EQUIP_ERR_ITEM_UNIQUE_EQUIPABLE;
            }

            // check unique-equipped on gems
            for(uint32 enchant_slot = SOCK_ENCHANTMENT_SLOT; enchant_slot < SOCK_ENCHANTMENT_SLOT+3; ++enchant_slot)
            {
                uint32 enchant_id = pItem->GetEnchantmentId(EnchantmentSlot(enchant_slot));
                if(!enchant_id)
                    continue;
                SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
                if(!enchantEntry)
                    continue;

                ItemPrototype const* pGem = objmgr.GetItemPrototype(enchantEntry->GemID);
                if(pGem && (pGem->Flags & ITEM_FLAGS_UNIQUE_EQUIPPED))
                {
                    Item* tItem = GetItemOrItemWithGemEquipped(enchantEntry->GemID);
                    if(tItem && (!swap || tItem->GetSlot() != eslot ))
                        return EQUIP_ERR_ITEM_UNIQUE_EQUIPABLE;
                }
            }

            // check unique-equipped special item classes
            if (pProto->Class == ITEM_CLASS_QUIVER)
            {
                for(int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
                {
                    if( Item* pBag = GetItemByPos( INVENTORY_SLOT_BAG_0, i ) )
                    {
                        if( ItemPrototype const* pBagProto = pBag->GetProto() )
                        {
                            if( pBagProto->Class==pProto->Class && (!swap || pBag->GetSlot() != eslot ) )
                            {
                                if(pBagProto->SubClass == ITEM_SUBCLASS_AMMO_POUCH)
                                    return EQUIP_ERR_CAN_EQUIP_ONLY1_AMMOPOUCH;
                                else
                                    return EQUIP_ERR_CAN_EQUIP_ONLY1_QUIVER;
                            }
                        }
                    }
                }
            }

            uint32 type = pProto->InventoryType;

            if(eslot == EQUIPMENT_SLOT_OFFHAND)
            {
                if( type == INVTYPE_WEAPON || type == INVTYPE_WEAPONOFFHAND )
                {
                    if(!CanDualWield())
                        return EQUIP_ERR_CANT_DUAL_WIELD;
                }

                Item *mainItem = GetItemByPos( INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND );
                if(mainItem)
                {
                    if(mainItem->GetProto()->InventoryType == INVTYPE_2HWEAPON)
                        return EQUIP_ERR_CANT_EQUIP_WITH_TWOHANDED;
                }
            }

            // equip two-hand weapon case (with possible unequip 2 items)
            if( type == INVTYPE_2HWEAPON )
            {
                if(eslot != EQUIPMENT_SLOT_MAINHAND)
                    return EQUIP_ERR_ITEM_CANT_BE_EQUIPPED;

                // offhand item must can be stored in inventory for offhand item and it also must be unequipped
                Item *offItem = GetItemByPos( INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND );
                ItemPosCountVec off_dest;
                if( offItem && (!not_loading ||
                    CanUnequipItem(uint16(INVENTORY_SLOT_BAG_0) << 8 | EQUIPMENT_SLOT_OFFHAND,false) !=  EQUIP_ERR_OK ||
                    CanStoreItem( NULL_BAG, NULL_SLOT, off_dest, offItem, false ) !=  EQUIP_ERR_OK ) )
                    return swap ? EQUIP_ERR_ITEMS_CANT_BE_SWAPPED : EQUIP_ERR_INVENTORY_FULL;
            }
            dest = ((INVENTORY_SLOT_BAG_0 << 8) | eslot);
            return EQUIP_ERR_OK;
        }
    }
    if( !swap )
        return EQUIP_ERR_ITEM_NOT_FOUND;
    else
        return EQUIP_ERR_ITEMS_CANT_BE_SWAPPED;
}

uint8 Player::CanUnequipItem( uint16 pos, bool swap ) const
{
    // Applied only to equipped items and bank bags
    if(!IsEquipmentPos(pos) && !IsBagPos(pos))
        return EQUIP_ERR_OK;

    Item* pItem = GetItemByPos(pos);

    // Applied only to existed equipped item
    if( !pItem )
        return EQUIP_ERR_OK;

    ItemPrototype const *pProto = pItem->GetProto();
    if( !pProto )
        return EQUIP_ERR_ITEM_NOT_FOUND;

    // do not allow unequipping gear except weapons, offhands, projectiles, relics in
    // - combat
    // - in-progress arenas
    if( !pProto->CanChangeEquipStateInCombat() )
    {
        if( isInCombat() )
            return EQUIP_ERR_NOT_IN_COMBAT;

        if(BattleGround* bg = GetBattleGround())
            if( bg->isArena() && bg->GetStatus() == STATUS_IN_PROGRESS )
                return EQUIP_ERR_NOT_DURING_ARENA_MATCH;
    }

    if(!swap && pItem->IsBag() && !((Bag*)pItem)->IsEmpty())
        return EQUIP_ERR_CAN_ONLY_DO_WITH_EMPTY_BAGS;

    return EQUIP_ERR_OK;
}

uint8 Player::CanBankItem( uint8 bag, uint8 slot, ItemPosCountVec &dest, Item *pItem, bool swap, bool not_loading ) const
{
    if( !pItem )
        return swap ? EQUIP_ERR_ITEMS_CANT_BE_SWAPPED : EQUIP_ERR_ITEM_NOT_FOUND;

    uint32 count = pItem->GetCount();

    ItemPrototype const *pProto = pItem->GetProto();
    if( !pProto )
        return swap ? EQUIP_ERR_ITEMS_CANT_BE_SWAPPED : EQUIP_ERR_ITEM_NOT_FOUND;

    if( pItem->IsBindedNotWith(GetGUID()) )
        return EQUIP_ERR_DONT_OWN_THAT_ITEM;

    // check count of items (skip for auto move for same player from bank)
    uint8 res = CanTakeMoreSimilarItems(pItem);
    if(res != EQUIP_ERR_OK)
        return res;

    // in specific slot
    if( bag != NULL_BAG && slot != NULL_SLOT )
    {
        if( pProto->InventoryType == INVTYPE_BAG )
        {
            Bag *pBag = (Bag*)pItem;
            if( pBag )
            {
                if( slot >= BANK_SLOT_BAG_START && slot < BANK_SLOT_BAG_END )
                {
                    if( !HasBankBagSlot( slot ) )
                        return EQUIP_ERR_MUST_PURCHASE_THAT_BAG_SLOT;
                    if( uint8 cantuse = CanUseItem( pItem, not_loading ) != EQUIP_ERR_OK )
                        return cantuse;
                }
                else
                {
                    if( !pBag->IsEmpty() )
                        return EQUIP_ERR_NONEMPTY_BAG_OVER_OTHER_BAG;
                }
            }
        }
        else
        {
            if( slot >= BANK_SLOT_BAG_START && slot < BANK_SLOT_BAG_END )
                return EQUIP_ERR_ITEM_DOESNT_GO_TO_SLOT;
        }

        res = _CanStoreItem_InSpecificSlot(bag,slot,dest,pProto,count,swap,pItem);
        if(res!=EQUIP_ERR_OK)
            return res;

        if(count==0)
            return EQUIP_ERR_OK;
    }

    // not specific slot or have space for partly store only in specific slot

    // in specific bag
    if( bag != NULL_BAG )
    {
        if( pProto->InventoryType == INVTYPE_BAG )
        {
            Bag *pBag = (Bag*)pItem;
            if( pBag && !pBag->IsEmpty() )
                return EQUIP_ERR_NONEMPTY_BAG_OVER_OTHER_BAG;
        }

        // search stack in bag for merge to
        if( pProto->Stackable > 1 )
        {
            if( bag == INVENTORY_SLOT_BAG_0 )
            {
                res = _CanStoreItem_InInventorySlots(BANK_SLOT_ITEM_START,BANK_SLOT_ITEM_END,dest,pProto,count,true,pItem,bag,slot);
                if(res!=EQUIP_ERR_OK)
                    return res;

                if(count==0)
                    return EQUIP_ERR_OK;
            }
            else
            {
                res = _CanStoreItem_InBag(bag,dest,pProto,count,true,false,pItem,NULL_BAG,slot);
                if(res!=EQUIP_ERR_OK)
                    res = _CanStoreItem_InBag(bag,dest,pProto,count,true,true,pItem,NULL_BAG,slot);

                if(res!=EQUIP_ERR_OK)
                    return res;

                if(count==0)
                    return EQUIP_ERR_OK;
            }
        }

        // search free slot in bag
        if( bag == INVENTORY_SLOT_BAG_0 )
        {
            res = _CanStoreItem_InInventorySlots(BANK_SLOT_ITEM_START,BANK_SLOT_ITEM_END,dest,pProto,count,false,pItem,bag,slot);
            if(res!=EQUIP_ERR_OK)
                return res;

            if(count==0)
                return EQUIP_ERR_OK;
        }
        else
        {
            res = _CanStoreItem_InBag(bag,dest,pProto,count,false,false,pItem,NULL_BAG,slot);
            if(res!=EQUIP_ERR_OK)
                res = _CanStoreItem_InBag(bag,dest,pProto,count,false,true,pItem,NULL_BAG,slot);

            if(res!=EQUIP_ERR_OK)
                return res;

            if(count==0)
                return EQUIP_ERR_OK;
        }
    }

    // not specific bag or have space for partly store only in specific bag

    // search stack for merge to
    if( pProto->Stackable > 1 )
    {
        // in slots
        res = _CanStoreItem_InInventorySlots(BANK_SLOT_ITEM_START,BANK_SLOT_ITEM_END,dest,pProto,count,true,pItem,bag,slot);
        if(res!=EQUIP_ERR_OK)
            return res;

        if(count==0)
            return EQUIP_ERR_OK;

        // in special bags
        if( pProto->BagFamily )
        {
            for(int i = BANK_SLOT_BAG_START; i < BANK_SLOT_BAG_END; i++)
            {
                res = _CanStoreItem_InBag(i,dest,pProto,count,true,false,pItem,bag,slot);
                if(res!=EQUIP_ERR_OK)
                    continue;

                if(count==0)
                    return EQUIP_ERR_OK;
            }
        }

        for(int i = BANK_SLOT_BAG_START; i < BANK_SLOT_BAG_END; i++)
        {
            res = _CanStoreItem_InBag(i,dest,pProto,count,true,true,pItem,bag,slot);
            if(res!=EQUIP_ERR_OK)
                continue;

            if(count==0)
                return EQUIP_ERR_OK;
        }
    }

    // search free place in special bag
    if( pProto->BagFamily )
    {
        for(int i = BANK_SLOT_BAG_START; i < BANK_SLOT_BAG_END; i++)
        {
            res = _CanStoreItem_InBag(i,dest,pProto,count,false,false,pItem,bag,slot);
            if(res!=EQUIP_ERR_OK)
                continue;

            if(count==0)
                return EQUIP_ERR_OK;
        }
    }

    // search free space
    res = _CanStoreItem_InInventorySlots(BANK_SLOT_ITEM_START,BANK_SLOT_ITEM_END,dest,pProto,count,false,pItem,bag,slot);
    if(res!=EQUIP_ERR_OK)
        return res;

    if(count==0)
        return EQUIP_ERR_OK;

    for(int i = BANK_SLOT_BAG_START; i < BANK_SLOT_BAG_END; i++)
    {
        res = _CanStoreItem_InBag(i,dest,pProto,count,false,true,pItem,bag,slot);
        if(res!=EQUIP_ERR_OK)
            continue;

        if(count==0)
            return EQUIP_ERR_OK;
    }
    return EQUIP_ERR_BANK_FULL;
}

uint8 Player::CanUseItem( Item *pItem, bool not_loading ) const
{
    if( pItem )
    {
        if( !isAlive() && not_loading )
            return EQUIP_ERR_YOU_ARE_DEAD;
        //if( isStunned() )
        //    return EQUIP_ERR_YOU_ARE_STUNNED;
        ItemPrototype const *pProto = pItem->GetProto();
        if( pProto )
        {
            if( pItem->IsBindedNotWith(GetGUID()) )
                return EQUIP_ERR_DONT_OWN_THAT_ITEM;
            if( (pProto->AllowableClass & getClassMask()) == 0 || (pProto->AllowableRace & getRaceMask()) == 0 )
                return EQUIP_ERR_YOU_CAN_NEVER_USE_THAT_ITEM;
            if( pItem->GetSkill() != 0  )
            {
                if( GetSkillValue( pItem->GetSkill() ) == 0 )
                    return EQUIP_ERR_NO_REQUIRED_PROFICIENCY;
            }
            if( pProto->RequiredSkill != 0  )
            {
                if( GetSkillValue( pProto->RequiredSkill ) == 0 )
                    return EQUIP_ERR_NO_REQUIRED_PROFICIENCY;
                else if( GetSkillValue( pProto->RequiredSkill ) < pProto->RequiredSkillRank )
                    return EQUIP_ERR_ERR_CANT_EQUIP_SKILL;
            }
            if( pProto->RequiredSpell != 0 && !HasSpell( pProto->RequiredSpell ) )
                return EQUIP_ERR_NO_REQUIRED_PROFICIENCY;
            if( pProto->RequiredReputationFaction && uint32(GetReputationRank(pProto->RequiredReputationFaction)) < pProto->RequiredReputationRank )
                return EQUIP_ERR_CANT_EQUIP_REPUTATION;
            if( getLevel() < pProto->RequiredLevel )
                return EQUIP_ERR_CANT_EQUIP_LEVEL_I;
            return EQUIP_ERR_OK;
        }
    }
    return EQUIP_ERR_ITEM_NOT_FOUND;
}

bool Player::CanUseItem( ItemPrototype const *pProto )
{
    // Used by group, function NeedBeforeGreed, to know if a prototype can be used by a player

    if( pProto )
    {
        if( (pProto->AllowableClass & getClassMask()) == 0 || (pProto->AllowableRace & getRaceMask()) == 0 )
            return false;
        if( pProto->RequiredSkill != 0  )
        {
            if( GetSkillValue( pProto->RequiredSkill ) == 0 )
                return false;
            else if( GetSkillValue( pProto->RequiredSkill ) < pProto->RequiredSkillRank )
                return false;
        }
        if( pProto->RequiredSpell != 0 && !HasSpell( pProto->RequiredSpell ) )
            return false;
        if( getLevel() < pProto->RequiredLevel )
            return false;
        return true;
    }
    return false;
}

uint8 Player::CanUseAmmo( uint32 item ) const
{
    if( !isAlive() )
        return EQUIP_ERR_YOU_ARE_DEAD;
    //if( isStunned() )
    //    return EQUIP_ERR_YOU_ARE_STUNNED;
    ItemPrototype const *pProto = objmgr.GetItemPrototype( item );
    if( pProto )
    {
        if( pProto->InventoryType!= INVTYPE_AMMO )
            return EQUIP_ERR_ONLY_AMMO_CAN_GO_HERE;
        if( (pProto->AllowableClass & getClassMask()) == 0 || (pProto->AllowableRace & getRaceMask()) == 0 )
            return EQUIP_ERR_YOU_CAN_NEVER_USE_THAT_ITEM;
        if( pProto->RequiredSkill != 0  )
        {
            if( GetSkillValue( pProto->RequiredSkill ) == 0 )
                return EQUIP_ERR_NO_REQUIRED_PROFICIENCY;
            else if( GetSkillValue( pProto->RequiredSkill ) < pProto->RequiredSkillRank )
                return EQUIP_ERR_ERR_CANT_EQUIP_SKILL;
        }
        if( pProto->RequiredSpell != 0 && !HasSpell( pProto->RequiredSpell ) )
            return EQUIP_ERR_NO_REQUIRED_PROFICIENCY;
        /*if( GetReputation() < pProto->RequiredReputation )
        return EQUIP_ERR_CANT_EQUIP_REPUTATION;
        */
        if( getLevel() < pProto->RequiredLevel )
            return EQUIP_ERR_CANT_EQUIP_LEVEL_I;

        // Requires No Ammo
        if(GetDummyAura(46699))
            return EQUIP_ERR_BAG_FULL6;

        return EQUIP_ERR_OK;
    }
    return EQUIP_ERR_ITEM_NOT_FOUND;
}

void Player::SetAmmo( uint32 item )
{
    if(!item)
        return;

    // already set
    if( GetUInt32Value(PLAYER_AMMO_ID) == item )
        return;

    // check ammo
    if(item)
    {
        uint8 msg = CanUseAmmo( item );
        if( msg != EQUIP_ERR_OK )
        {
            SendEquipError( msg, NULL, NULL );
            return;
        }
    }

    SetUInt32Value(PLAYER_AMMO_ID, item);

    _ApplyAmmoBonuses();
}

void Player::RemoveAmmo()
{
    SetUInt32Value(PLAYER_AMMO_ID, 0);

    m_ammoDPS = 0.0f;

    if(CanModifyStats())
        UpdateDamagePhysical(RANGED_ATTACK);
}

// Return stored item (if stored to stack, it can diff. from pItem). And pItem ca be deleted in this case.
Item* Player::StoreNewItem( ItemPosCountVec const& dest, uint32 item, bool update,int32 randomPropertyId )
{
    uint32 count = 0;
    for(ItemPosCountVec::const_iterator itr = dest.begin(); itr != dest.end(); ++itr)
        count += itr->count;

    Item *pItem = Item::CreateItem( item, count, this );
    if( pItem )
    {
        ItemAddedQuestCheck( item, count );
        if(randomPropertyId)
            pItem->SetItemRandomProperties(randomPropertyId);
        pItem = StoreItem( dest, pItem, update );
    }
    
    if (item == 31088)  // Tainted Core
        SetMovement(MOVE_ROOT);
        
    // If purple equipable item, save inventory immediately
    if (pItem && pItem->GetProto()->Quality >= ITEM_QUALITY_EPIC &&
        (pItem->GetProto()->Class == ITEM_CLASS_WEAPON || pItem->GetProto()->Class == ITEM_CLASS_ARMOR || pItem->GetProto()->Class == ITEM_CLASS_JUNK)) {
        SQLTransaction trans = CharacterDatabase.BeginTransaction();
        SaveInventoryAndGoldToDB(trans);
        CharacterDatabase.CommitTransaction(trans);
    }
    
    return pItem;
}

Item* Player::StoreItem( ItemPosCountVec const& dest, Item* pItem, bool update )
{
    if( !pItem )
        return NULL;

    Item* lastItem = pItem;

    for(ItemPosCountVec::const_iterator itr = dest.begin(); itr != dest.end(); )
    {
        uint16 pos = itr->pos;
        uint32 count = itr->count;

        ++itr;

        if(itr == dest.end())
        {
            lastItem = _StoreItem(pos,pItem,count,false,update);
            break;
        }

        lastItem = _StoreItem(pos,pItem,count,true,update);
    }

    return lastItem;
}

// Return stored item (if stored to stack, it can diff. from pItem). And pItem ca be deleted in this case.
Item* Player::_StoreItem( uint16 pos, Item *pItem, uint32 count, bool clone, bool update )
{
    if( !pItem )
        return NULL;

    uint8 bag = pos >> 8;
    uint8 slot = pos & 255;

    Item *pItem2 = GetItemByPos( bag, slot );

    if( !pItem2 )
    {
        if(clone)
            pItem = pItem->CloneItem(count,this);
        else
            pItem->SetCount(count);

        if(!pItem)
            return NULL;

        if( pItem->GetProto()->Bonding == BIND_WHEN_PICKED_UP ||
            pItem->GetProto()->Bonding == BIND_QUEST_ITEM ||
            pItem->GetProto()->Bonding == BIND_WHEN_EQUIPED && IsBagPos(pos) )
            pItem->SetBinding( true );

        if( bag == INVENTORY_SLOT_BAG_0 )
        {
            m_items[slot] = pItem;
            SetUInt64Value( (uint16)(PLAYER_FIELD_INV_SLOT_HEAD + (slot * 2) ), pItem->GetGUID() );
            pItem->SetUInt64Value( ITEM_FIELD_CONTAINED, GetGUID() );
            pItem->SetUInt64Value( ITEM_FIELD_OWNER, GetGUID() );

            pItem->SetSlot( slot );
            pItem->SetContainer( NULL );

            if( IsInWorld() && update )
            {
                pItem->AddToWorld();
                pItem->SendUpdateToPlayer( this );
            }

            pItem->SetState(ITEM_CHANGED, this);
        }
        else
        {
            Bag *pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, bag );
            if( pBag )
            {
                pBag->StoreItem( slot, pItem, update );
                if( IsInWorld() && update )
                {
                    pItem->AddToWorld();
                    pItem->SendUpdateToPlayer( this );
                }
                pItem->SetState(ITEM_CHANGED, this);
                pBag->SetState(ITEM_CHANGED, this);
            }
        }

        AddEnchantmentDurations(pItem);
        AddItemDurations(pItem);

        return pItem;
    }
    else
    {
        if( pItem2->GetProto()->Bonding == BIND_WHEN_PICKED_UP ||
            pItem2->GetProto()->Bonding == BIND_QUEST_ITEM ||
            pItem2->GetProto()->Bonding == BIND_WHEN_EQUIPED && IsBagPos(pos) )
            pItem2->SetBinding( true );

        pItem2->SetCount( pItem2->GetCount() + count );
        if( IsInWorld() && update )
            pItem2->SendUpdateToPlayer( this );

        if(!clone)
        {
            // delete item (it not in any slot currently)
            if( IsInWorld() && update )
            {
                pItem->RemoveFromWorld();
                pItem->DestroyForPlayer( this );
            }

            RemoveEnchantmentDurations(pItem);
            RemoveItemDurations(pItem);

            pItem->SetOwnerGUID(GetGUID());                 // prevent error at next SetState in case trade/mail/buy from vendor
            pItem->SetState(ITEM_REMOVED, this);
        }
        // AddItemDurations(pItem2); - pItem2 already have duration listed for player
        AddEnchantmentDurations(pItem2);

        pItem2->SetState(ITEM_CHANGED, this);

        return pItem2;
    }
}

Item* Player::EquipNewItem( uint16 pos, uint32 item, bool update )
{
    Item *pItem = Item::CreateItem( item, 1, this );
    if( pItem )
    {
        ItemAddedQuestCheck( item, 1 );
        Item * retItem = EquipItem( pos, pItem, update );

        return retItem;
    }
    return NULL;
}

Item* Player::EquipItem( uint16 pos, Item *pItem, bool update )
{
    if( pItem )
    {
        AddEnchantmentDurations(pItem);
        AddItemDurations(pItem);

        uint8 bag = pos >> 8;
        uint8 slot = pos & 255;

        Item *pItem2 = GetItemByPos( bag, slot );

        if( !pItem2 )
        {
            VisualizeItem( slot, pItem);

            if(isAlive())
            {
                ItemPrototype const *pProto = pItem->GetProto();

                // item set bonuses applied only at equip and removed at unequip, and still active for broken items
                if(pProto && pProto->ItemSet)
                    AddItemsSetItem(this,pItem);

                _ApplyItemMods(pItem, slot, true);

                if(pProto && isInCombat()&& pProto->Class == ITEM_CLASS_WEAPON && m_weaponChangeTimer == 0)
                {
                    uint32 cooldownSpell = SPELL_ID_WEAPON_SWITCH_COOLDOWN_1_5s;

                    if (getClass() == CLASS_ROGUE)
                        cooldownSpell = SPELL_ID_WEAPON_SWITCH_COOLDOWN_1_0s;

                    SpellEntry const* spellProto = spellmgr.LookupSpell(cooldownSpell);

                    if (!spellProto)
                        sLog.outError("Weapon switch cooldown spell %u couldn't be found in Spell.dbc", cooldownSpell);
                    else
                    {
                        m_weaponChangeTimer = spellProto->StartRecoveryTime;

                        WorldPacket data(SMSG_SPELL_COOLDOWN, 8+1+4);
                        data << uint64(GetGUID());
                        data << uint8(1);
                        data << uint32(cooldownSpell);
                        data << uint32(0);
                        GetSession()->SendPacket(&data);
                    }
                }
            }

            if( IsInWorld() && update )
            {
                pItem->AddToWorld();
                pItem->SendUpdateToPlayer( this );
            }

            ApplyEquipCooldown(pItem);

            if( slot == EQUIPMENT_SLOT_MAINHAND )
                UpdateExpertise(BASE_ATTACK);
            else if( slot == EQUIPMENT_SLOT_OFFHAND )
                UpdateExpertise(OFF_ATTACK);
        }
        else
        {
            pItem2->SetCount( pItem2->GetCount() + pItem->GetCount() );
            if( IsInWorld() && update )
                pItem2->SendUpdateToPlayer( this );

            // delete item (it not in any slot currently)
            //pItem->DeleteFromDB();
            if( IsInWorld() && update )
            {
                pItem->RemoveFromWorld();
                pItem->DestroyForPlayer( this );
            }

            RemoveEnchantmentDurations(pItem);
            RemoveItemDurations(pItem);

            pItem->SetOwnerGUID(GetGUID());                 // prevent error at next SetState in case trade/mail/buy from vendor
            pItem->SetState(ITEM_REMOVED, this);
            pItem2->SetState(ITEM_CHANGED, this);

            ApplyEquipCooldown(pItem2);

            return pItem2;
        }
    }

    return pItem;
}

void Player::QuickEquipItem( uint16 pos, Item *pItem)
{
    if( pItem )
    {
        AddEnchantmentDurations(pItem);
        AddItemDurations(pItem);

        uint8 slot = pos & 255;
        VisualizeItem( slot, pItem);

        if( IsInWorld() )
        {
            pItem->AddToWorld();
            pItem->SendUpdateToPlayer( this );
        }
    }
}

void Player::SetVisibleItemSlot(uint8 slot, Item *pItem)
{
    // PLAYER_VISIBLE_ITEM_i_CREATOR    // Size: 2
    // PLAYER_VISIBLE_ITEM_i_0          // Size: 12
    //    entry                         //      Size: 1
    //    inspected enchantments        //      Size: 6
    //    ?                             //      Size: 5
    // PLAYER_VISIBLE_ITEM_i_PROPERTIES // Size: 1 (property,suffix factor)
    // PLAYER_VISIBLE_ITEM_i_PAD        // Size: 1
    //                                  //     = 16

    if(pItem)
    {
        SetUInt64Value(PLAYER_VISIBLE_ITEM_1_CREATOR + (slot * MAX_VISIBLE_ITEM_OFFSET), pItem->GetUInt64Value(ITEM_FIELD_CREATOR));

        int VisibleBase = PLAYER_VISIBLE_ITEM_1_0 + (slot * MAX_VISIBLE_ITEM_OFFSET);
        SetUInt32Value(VisibleBase + 0, pItem->GetEntry());

        for(int i = 0; i < MAX_INSPECTED_ENCHANTMENT_SLOT; ++i)
            SetUInt32Value(VisibleBase + 1 + i, pItem->GetEnchantmentId(EnchantmentSlot(i)));

        // Use SetInt16Value to prevent set high part to FFFF for negative value
        SetInt16Value( PLAYER_VISIBLE_ITEM_1_PROPERTIES + (slot * MAX_VISIBLE_ITEM_OFFSET), 0, pItem->GetItemRandomPropertyId());
        SetUInt32Value(PLAYER_VISIBLE_ITEM_1_PROPERTIES + 1 + (slot * MAX_VISIBLE_ITEM_OFFSET), pItem->GetItemSuffixFactor());
    }
    else
    {
        SetUInt64Value(PLAYER_VISIBLE_ITEM_1_CREATOR + (slot * MAX_VISIBLE_ITEM_OFFSET), 0);

        int VisibleBase = PLAYER_VISIBLE_ITEM_1_0 + (slot * MAX_VISIBLE_ITEM_OFFSET);
        SetUInt32Value(VisibleBase + 0, 0);

        for(int i = 0; i < MAX_INSPECTED_ENCHANTMENT_SLOT; ++i)
            SetUInt32Value(VisibleBase + 1 + i, 0);

        SetUInt32Value(PLAYER_VISIBLE_ITEM_1_PROPERTIES + 0 + (slot * MAX_VISIBLE_ITEM_OFFSET), 0);
        SetUInt32Value(PLAYER_VISIBLE_ITEM_1_PROPERTIES + 1 + (slot * MAX_VISIBLE_ITEM_OFFSET), 0);
    }
}

void Player::VisualizeItem( uint8 slot, Item *pItem)
{
    if(!pItem)
        return;

    // check also  BIND_WHEN_PICKED_UP and BIND_QUEST_ITEM for .additem or .additemset case by GM (not binded at adding to inventory)
    if( pItem->GetProto()->Bonding == BIND_WHEN_EQUIPED || pItem->GetProto()->Bonding == BIND_WHEN_PICKED_UP || pItem->GetProto()->Bonding == BIND_QUEST_ITEM )
        pItem->SetBinding( true );

    m_items[slot] = pItem;
    SetUInt64Value( (uint16)(PLAYER_FIELD_INV_SLOT_HEAD + (slot * 2) ), pItem->GetGUID() );
    pItem->SetUInt64Value( ITEM_FIELD_CONTAINED, GetGUID() );
    pItem->SetUInt64Value( ITEM_FIELD_OWNER, GetGUID() );
    pItem->SetSlot( slot );
    pItem->SetContainer( NULL );

    if( slot < EQUIPMENT_SLOT_END )
        SetVisibleItemSlot(slot,pItem);

    pItem->SetState(ITEM_CHANGED, this);
}

void Player::RemoveItem( uint8 bag, uint8 slot, bool update )
{
    // note: removeitem does not actually change the item
    // it only takes the item out of storage temporarily
    // note2: if removeitem is to be used for delinking
    // the item must be removed from the player's updatequeue

    Item *pItem = GetItemByPos( bag, slot );
    if( pItem )
    {
        RemoveEnchantmentDurations(pItem);
        RemoveItemDurations(pItem);

        if( bag == INVENTORY_SLOT_BAG_0 )
        {
            if ( slot < INVENTORY_SLOT_BAG_END )
            {
                ItemPrototype const *pProto = pItem->GetProto();
                // item set bonuses applied only at equip and removed at unequip, and still active for broken items

                if(pProto && pProto->ItemSet)
                    RemoveItemsSetItem(this,pProto);

                _ApplyItemMods(pItem, slot, false);

                // remove item dependent auras and casts (only weapon and armor slots)
                if(slot < EQUIPMENT_SLOT_END)
                    RemoveItemDependentAurasAndCasts(pItem);

                // remove held enchantments
                if ( slot == EQUIPMENT_SLOT_MAINHAND )
                {
                    if (pItem->GetItemSuffixFactor())
                    {
                        pItem->ClearEnchantment(PROP_ENCHANTMENT_SLOT_3);
                        pItem->ClearEnchantment(PROP_ENCHANTMENT_SLOT_4);
                    }
                    else
                    {
                        pItem->ClearEnchantment(PROP_ENCHANTMENT_SLOT_0);
                        pItem->ClearEnchantment(PROP_ENCHANTMENT_SLOT_1);
                    }
                }
            }

            m_items[slot] = NULL;
            SetUInt64Value((uint16)(PLAYER_FIELD_INV_SLOT_HEAD + (slot*2)), 0);

            if ( slot < EQUIPMENT_SLOT_END )
                SetVisibleItemSlot(slot,NULL);
        }
        else
        {
            Bag *pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, bag );
            if( pBag )
                pBag->RemoveItem(slot, update);
        }
        pItem->SetUInt64Value( ITEM_FIELD_CONTAINED, 0 );
        // pItem->SetUInt64Value( ITEM_FIELD_OWNER, 0 ); not clear owner at remove (it will be set at store). This used in mail and auction code
        pItem->SetSlot( NULL_SLOT );
        if( IsInWorld() && update )
            pItem->SendUpdateToPlayer( this );

        if( slot == EQUIPMENT_SLOT_MAINHAND )
            UpdateExpertise(BASE_ATTACK);
        else if( slot == EQUIPMENT_SLOT_OFFHAND )
            UpdateExpertise(OFF_ATTACK);
    }
}

// Common operation need to remove item from inventory without delete in trade, auction, guild bank, mail....
void Player::MoveItemFromInventory(uint8 bag, uint8 slot, bool update)
{
    if(Item* it = GetItemByPos(bag,slot))
    {
        ItemRemovedQuestCheck(it->GetEntry(),it->GetCount());
        RemoveItem( bag,slot,update);
        it->RemoveFromUpdateQueueOf(this);
        if(it->IsInWorld())
        {
            it->RemoveFromWorld();
            it->DestroyForPlayer( this );
        }
    }
}

// Common operation need to add item from inventory without delete in trade, guild bank, mail....
void Player::MoveItemToInventory(ItemPosCountVec const& dest, Item* pItem, bool update, bool in_characterInventoryDB)
{
    // update quest counters
    ItemAddedQuestCheck(pItem->GetEntry(),pItem->GetCount());

    // store item
    Item* pLastItem = StoreItem( dest, pItem, update);

    // only set if not merged to existed stack (pItem can be deleted already but we can compare pointers any way)
    if(pLastItem==pItem)
    {
        // update owner for last item (this can be original item with wrong owner
        if(pLastItem->GetOwnerGUID() != GetGUID())
            pLastItem->SetOwnerGUID(GetGUID());

        // if this original item then it need create record in inventory
        // in case trade we already have item in other player inventory
        pLastItem->SetState(in_characterInventoryDB ? ITEM_CHANGED : ITEM_NEW, this);
    }
}

void Player::DestroyItem( uint8 bag, uint8 slot, bool update )
{
    Item *pItem = GetItemByPos( bag, slot );
    if( pItem )
    {
        // start from destroy contained items (only equipped bag can have its)
        if (pItem->IsBag() && pItem->IsEquipped())          // this also prevent infinity loop if empty bag stored in bag==slot
        {
            for (int i = 0; i < MAX_BAG_SIZE; i++)
                DestroyItem(slot,i,update);
        }
        
        
        if (pItem->GetEntry() == 31088)      // Vashj Tainted Core
            SetMovement(MOVE_UNROOT);

        if(pItem->HasFlag(ITEM_FIELD_FLAGS, ITEM_FLAGS_WRAPPED))
            CharacterDatabase.PExecute("DELETE FROM character_gifts WHERE item_guid = '%u'", pItem->GetGUIDLow());

        RemoveEnchantmentDurations(pItem);
        RemoveItemDurations(pItem);

        ItemRemovedQuestCheck( pItem->GetEntry(), pItem->GetCount() );

        if( bag == INVENTORY_SLOT_BAG_0 )
        {
            SetUInt64Value((uint16)(PLAYER_FIELD_INV_SLOT_HEAD + (slot*2)), 0);

            // equipment and equipped bags can have applied bonuses
            if ( slot < INVENTORY_SLOT_BAG_END )
            {
                ItemPrototype const *pProto = pItem->GetProto();

                // item set bonuses applied only at equip and removed at unequip, and still active for broken items
                if(pProto && pProto->ItemSet)
                    RemoveItemsSetItem(this,pProto);

                _ApplyItemMods(pItem, slot, false);
            }

            if ( slot < EQUIPMENT_SLOT_END )
            {
                // remove item dependent auras and casts (only weapon and armor slots)
                RemoveItemDependentAurasAndCasts(pItem);

                // equipment visual show
                SetVisibleItemSlot(slot,NULL);
            }

            m_items[slot] = NULL;
        }
        else if(Bag *pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, bag ))
            pBag->RemoveItem(slot, update);

        if( IsInWorld() && update )
        {
            pItem->RemoveFromWorld();
            pItem->DestroyForPlayer(this);
        }

        //pItem->SetOwnerGUID(0);
        pItem->SetUInt64Value( ITEM_FIELD_CONTAINED, 0 );
        pItem->SetSlot( NULL_SLOT );
        pItem->SetState(ITEM_REMOVED, this);
    }
}

void Player::DestroyItemCount( uint32 item, uint32 count, bool update, bool unequip_check, bool inBankAlso)
{
    Item *pItem;
    ItemPrototype const *pProto;
    uint32 remcount = 0;

    // in inventory
    for(int i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; i++)
    {
        pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
        if( pItem && pItem->GetEntry() == item )
        {
            if( pItem->GetCount() + remcount <= count )
            {
                // all items in inventory can unequipped
                remcount += pItem->GetCount();
                DestroyItem( INVENTORY_SLOT_BAG_0, i, update);

                if(remcount >=count)
                    return;
            }
            else
            {
                pProto = pItem->GetProto();
                ItemRemovedQuestCheck( pItem->GetEntry(), count - remcount );
                pItem->SetCount( pItem->GetCount() - count + remcount );
                if( IsInWorld() & update )
                    pItem->SendUpdateToPlayer( this );
                pItem->SetState(ITEM_CHANGED, this);
                return;
            }
        }
    }
    for(int i = KEYRING_SLOT_START; i < KEYRING_SLOT_END; i++)
    {
        pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
        if( pItem && pItem->GetEntry() == item )
        {
            if( pItem->GetCount() + remcount <= count )
            {
                // all keys can be unequipped
                remcount += pItem->GetCount();
                DestroyItem( INVENTORY_SLOT_BAG_0, i, update);

                if(remcount >=count)
                    return;
            }
            else
            {
                pProto = pItem->GetProto();
                ItemRemovedQuestCheck( pItem->GetEntry(), count - remcount );
                pItem->SetCount( pItem->GetCount() - count + remcount );
                if( IsInWorld() & update )
                    pItem->SendUpdateToPlayer( this );
                pItem->SetState(ITEM_CHANGED, this);
                return;
            }
        }
    }

    // in inventory bags
    for(int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; i++)
    {
        if(Bag *pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, i ))
        {
            for(uint32 j = 0; j < pBag->GetBagSize(); j++)
            {
                pItem = pBag->GetItemByPos(j);
                if( pItem && pItem->GetEntry() == item )
                {
                    // all items in bags can be unequipped
                    if( pItem->GetCount() + remcount <= count )
                    {
                        remcount += pItem->GetCount();
                        DestroyItem( i, j, update );

                        if(remcount >=count)
                            return;
                    }
                    else
                    {
                        pProto = pItem->GetProto();
                        ItemRemovedQuestCheck( pItem->GetEntry(), count - remcount );
                        pItem->SetCount( pItem->GetCount() - count + remcount );
                        if( IsInWorld() && update )
                            pItem->SendUpdateToPlayer( this );
                        pItem->SetState(ITEM_CHANGED, this);
                        return;
                    }
                }
            }
        }
    }

    // in equipment and bag list
    for(int i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_BAG_END; i++)
    {
        pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
        if( pItem && pItem->GetEntry() == item )
        {
            if( pItem->GetCount() + remcount <= count )
            {
                if(!unequip_check || CanUnequipItem(INVENTORY_SLOT_BAG_0 << 8 | i,false) == EQUIP_ERR_OK )
                {
                    remcount += pItem->GetCount();
                    DestroyItem( INVENTORY_SLOT_BAG_0, i, update);

                    if(remcount >=count)
                        return;
                }
            }
            else
            {
                pProto = pItem->GetProto();
                ItemRemovedQuestCheck( pItem->GetEntry(), count - remcount );
                pItem->SetCount( pItem->GetCount() - count + remcount );
                if( IsInWorld() & update )
                    pItem->SendUpdateToPlayer( this );
                pItem->SetState(ITEM_CHANGED, this);
                return;
            }
        }
    }
    
    if(inBankAlso)
    {
        for(int i = BANK_SLOT_ITEM_START; i < BANK_SLOT_ITEM_END; i++)
        {
            Item *pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
            if( pItem && pItem->GetEntry() == item )
            {
                if( pItem->GetCount() + remcount <= count )
                {
                    // all items in inventory can unequipped
                    remcount += pItem->GetCount();
                    DestroyItem( INVENTORY_SLOT_BAG_0, i, update);

                    if(remcount >=count)
                        return;
                }
                else
                {
                    pProto = pItem->GetProto();
                    ItemRemovedQuestCheck( pItem->GetEntry(), count - remcount );
                    pItem->SetCount( pItem->GetCount() - count + remcount );
                    if( IsInWorld() & update )
                        pItem->SendUpdateToPlayer( this );
                    pItem->SetState(ITEM_CHANGED, this);
                    return;
                }
            }
        }
        for(int i = BANK_SLOT_BAG_START; i < BANK_SLOT_BAG_END; i++)
        {
            if(Bag* pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, i ))
            {
                for(uint32 j = 0; j < pBag->GetBagSize(); j++)
                {
                    pItem = pBag->GetItemByPos(j);
                    if( pItem && pItem->GetEntry() == item )
                    {
                        // all items in bags can be unequipped
                        if( pItem->GetCount() + remcount <= count )
                        {
                            remcount += pItem->GetCount();
                            DestroyItem( i, j, update );

                            if(remcount >=count)
                                return;
                        }
                        else
                        {
                            pProto = pItem->GetProto();
                            ItemRemovedQuestCheck( pItem->GetEntry(), count - remcount );
                            pItem->SetCount( pItem->GetCount() - count + remcount );
                            if( IsInWorld() && update )
                                pItem->SendUpdateToPlayer( this );
                            pItem->SetState(ITEM_CHANGED, this);
                            return;
                        }
                    }
                }
            }
        }
    }
}

void Player::DestroyZoneLimitedItem( bool update, uint32 new_zone )
{
    // in inventory
    for(int i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; i++)
    {
        Item* pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
        if( pItem && pItem->IsLimitedToAnotherMapOrZone(GetMapId(),new_zone) )
            DestroyItem( INVENTORY_SLOT_BAG_0, i, update);
    }
    for(int i = KEYRING_SLOT_START; i < KEYRING_SLOT_END; i++)
    {
        Item* pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
        if( pItem && pItem->IsLimitedToAnotherMapOrZone(GetMapId(),new_zone) )
            DestroyItem( INVENTORY_SLOT_BAG_0, i, update);
    }

    // in inventory bags
    for(int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; i++)
    {
        Bag* pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, i );
        if( pBag )
        {
            for(uint32 j = 0; j < pBag->GetBagSize(); j++)
            {
                Item* pItem = pBag->GetItemByPos(j);
                if( pItem && pItem->IsLimitedToAnotherMapOrZone(GetMapId(),new_zone) )
                    DestroyItem( i, j, update);
            }
        }
    }

    // in equipment and bag list
    for(int i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_BAG_END; i++)
    {
        Item* pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
        if( pItem && pItem->IsLimitedToAnotherMapOrZone(GetMapId(),new_zone) )
            DestroyItem( INVENTORY_SLOT_BAG_0, i, update);
    }
}

void Player::DestroyConjuredItems( bool update )
{
    // used when entering arena
    // destroys all conjured items
    // in inventory
    for(int i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; i++)
    {
        Item* pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
        if( pItem && pItem->GetProto() &&
            (pItem->GetProto()->Class == ITEM_CLASS_CONSUMABLE) &&
            (pItem->GetProto()->Flags & ITEM_FLAGS_CONJURED) )
            DestroyItem( INVENTORY_SLOT_BAG_0, i, update);
    }

    // in inventory bags
    for(int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; i++)
    {
        Bag* pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, i );
        if( pBag )
        {
            for(uint32 j = 0; j < pBag->GetBagSize(); j++)
            {
                Item* pItem = pBag->GetItemByPos(j);
                if( pItem && pItem->GetProto() &&
                    (pItem->GetProto()->Class == ITEM_CLASS_CONSUMABLE) &&
                    (pItem->GetProto()->Flags & ITEM_FLAGS_CONJURED) )
                    DestroyItem( i, j, update);
            }
        }
    }

    // in equipment and bag list
    for(int i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_BAG_END; i++)
    {
        Item* pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
        if( pItem && pItem->GetProto() &&
            (pItem->GetProto()->Class == ITEM_CLASS_CONSUMABLE) &&
            (pItem->GetProto()->Flags & ITEM_FLAGS_CONJURED) )
            DestroyItem( INVENTORY_SLOT_BAG_0, i, update);
    }
}

void Player::DestroyItemCount( Item* pItem, uint32 &count, bool update )
{
    if(!pItem)
        return;

    if( pItem->GetCount() <= count )
    {
        count-= pItem->GetCount();

        DestroyItem( pItem->GetBagSlot(),pItem->GetSlot(), update);
    }
    else
    {
        ItemRemovedQuestCheck( pItem->GetEntry(), count);
        pItem->SetCount( pItem->GetCount() - count );
        count = 0;
        if( IsInWorld() & update )
            pItem->SendUpdateToPlayer( this );
        pItem->SetState(ITEM_CHANGED, this);
    }
}

void Player::SwapItems(uint32 item1, uint32 item2)
{
    uint32 count = GetItemCount(item1, true);
    if (count != 0) {
        DestroyItemCount(item1, count, true, false, true);
        ItemPosCountVec dest;
        uint8 msg = CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, item2, count, false);
        if (msg == EQUIP_ERR_OK)
            StoreNewItem(dest, item2, count, true);
        else {
            if (Item* newItem = Item::CreateItem(item2, count, this)) {
                SQLTransaction trans = CharacterDatabase.BeginTransaction();
                newItem->SaveToDB(trans);
                CharacterDatabase.CommitTransaction(trans);

                MailItemsInfo mi;
                mi.AddItem(newItem->GetGUIDLow(), newItem->GetEntry(), newItem);
                std::string subject = GetSession()->GetTrinityString(LANG_NOT_EQUIPPED_ITEM);
                WorldSession::SendMailTo(this, MAIL_NORMAL, MAIL_STATIONERY_GM, GetGUIDLow(), GetGUIDLow(), subject, 0, &mi, 0, 0, MAIL_CHECK_MASK_NONE);
            }
        }
    }
}

void Player::SplitItem( uint16 src, uint16 dst, uint32 count )
{
    uint8 srcbag = src >> 8;
    uint8 srcslot = src & 255;

    uint8 dstbag = dst >> 8;
    uint8 dstslot = dst & 255;

    Item *pSrcItem = GetItemByPos( srcbag, srcslot );
    if( !pSrcItem )
    {
        SendEquipError( EQUIP_ERR_ITEM_NOT_FOUND, pSrcItem, NULL );
        return;
    }

    // not let split all items (can be only at cheating)
    if(pSrcItem->GetCount() == count)
    {
        SendEquipError( EQUIP_ERR_COULDNT_SPLIT_ITEMS, pSrcItem, NULL );
        return;
    }

    // not let split more existed items (can be only at cheating)
    if(pSrcItem->GetCount() < count)
    {
        SendEquipError( EQUIP_ERR_TRIED_TO_SPLIT_MORE_THAN_COUNT, pSrcItem, NULL );
        return;
    }

    if(pSrcItem->m_lootGenerated)                           // prevent split looting item (item
    {
        //best error message found for attempting to split while looting
        SendEquipError( EQUIP_ERR_COULDNT_SPLIT_ITEMS, pSrcItem, NULL );
        return;
    }

    Item *pNewItem = pSrcItem->CloneItem( count, this );
    if( !pNewItem )
    {
        SendEquipError( EQUIP_ERR_ITEM_NOT_FOUND, pSrcItem, NULL );
        return;
    }

    if( IsInventoryPos( dst ) )
    {
        // change item amount before check (for unique max count check)
        pSrcItem->SetCount( pSrcItem->GetCount() - count );

        ItemPosCountVec dest;
        uint8 msg = CanStoreItem( dstbag, dstslot, dest, pNewItem, false );
        if( msg != EQUIP_ERR_OK )
        {
            delete pNewItem;
            pSrcItem->SetCount( pSrcItem->GetCount() + count );
            SendEquipError( msg, pSrcItem, NULL );
            return;
        }

        if( IsInWorld() )
            pSrcItem->SendUpdateToPlayer( this );
        pSrcItem->SetState(ITEM_CHANGED, this);
        StoreItem( dest, pNewItem, true);
    }
    else if( IsBankPos ( dst ) )
    {
        // change item amount before check (for unique max count check)
        pSrcItem->SetCount( pSrcItem->GetCount() - count );

        ItemPosCountVec dest;
        uint8 msg = CanBankItem( dstbag, dstslot, dest, pNewItem, false );
        if( msg != EQUIP_ERR_OK )
        {
            delete pNewItem;
            pSrcItem->SetCount( pSrcItem->GetCount() + count );
            SendEquipError( msg, pSrcItem, NULL );
            return;
        }

        if( IsInWorld() )
            pSrcItem->SendUpdateToPlayer( this );
        pSrcItem->SetState(ITEM_CHANGED, this);
        BankItem( dest, pNewItem, true);
    }
    else if( IsEquipmentPos ( dst ) )
    {
        // change item amount before check (for unique max count check), provide space for splitted items
        pSrcItem->SetCount( pSrcItem->GetCount() - count );

        uint16 dest;
        uint8 msg = CanEquipItem( dstslot, dest, pNewItem, false );
        if( msg != EQUIP_ERR_OK )
        {
            delete pNewItem;
            pSrcItem->SetCount( pSrcItem->GetCount() + count );
            SendEquipError( msg, pSrcItem, NULL );
            return;
        }

        if( IsInWorld() )
            pSrcItem->SendUpdateToPlayer( this );
        pSrcItem->SetState(ITEM_CHANGED, this);
        EquipItem( dest, pNewItem, true);
        AutoUnequipOffhandIfNeed();
    }
}

void Player::SwapItem( uint16 src, uint16 dst )
{
    uint8 srcbag = src >> 8;
    uint8 srcslot = src & 255;

    uint8 dstbag = dst >> 8;
    uint8 dstslot = dst & 255;

    Item *pSrcItem = GetItemByPos( srcbag, srcslot );
    Item *pDstItem = GetItemByPos( dstbag, dstslot );

    if( !pSrcItem )
        return;

    if(!isAlive() )
    {
        SendEquipError( EQUIP_ERR_YOU_ARE_DEAD, pSrcItem, pDstItem );
        return;
    }

    if(pSrcItem->m_lootGenerated)                           // prevent swap looting item
    {
        //best error message found for attempting to swap while looting
        SendEquipError( EQUIP_ERR_CANT_DO_RIGHT_NOW, pSrcItem, NULL );
        return;
    }

    // check unequip potability for equipped items and bank bags
    if(IsEquipmentPos ( src ) || IsBagPos ( src ))
    {
        // bags can be swapped with empty bag slots
        uint8 msg = CanUnequipItem( src, !IsBagPos ( src ) || IsBagPos ( dst ));
        if(msg != EQUIP_ERR_OK)
        {
            SendEquipError( msg, pSrcItem, pDstItem );
            return;
        }
    }

    // prevent put equipped/bank bag in self
    if( IsBagPos ( src ) && srcslot == dstbag)
    {
        SendEquipError( EQUIP_ERR_NONEMPTY_BAG_OVER_OTHER_BAG, pSrcItem, pDstItem );
        return;
    }
    
    // prevent equipping bag in the same slot from its inside
    if (IsBagPos(dst) && srcbag == dstslot)
    {
        SendEquipError(EQUIP_ERR_ITEMS_CANT_BE_SWAPPED, pSrcItem, pDstItem);
        return;
    }

    if( !pDstItem )
    {
        if( IsInventoryPos( dst ) )
        {
            ItemPosCountVec dest;
            uint8 msg = CanStoreItem( dstbag, dstslot, dest, pSrcItem, false );
            if( msg != EQUIP_ERR_OK )
            {
                SendEquipError( msg, pSrcItem, NULL );
                return;
            }

            RemoveItem(srcbag, srcslot, true);
            StoreItem( dest, pSrcItem, true);
        }
        else if( IsBankPos ( dst ) )
        {
            ItemPosCountVec dest;
            uint8 msg = CanBankItem( dstbag, dstslot, dest, pSrcItem, false);
            if( msg != EQUIP_ERR_OK )
            {
                SendEquipError( msg, pSrcItem, NULL );
                return;
            }

            RemoveItem(srcbag, srcslot, true);
            BankItem( dest, pSrcItem, true);
        }
        else if( IsEquipmentPos ( dst ) )
        {
            uint16 dest;
            uint8 msg = CanEquipItem( dstslot, dest, pSrcItem, false );
            if( msg != EQUIP_ERR_OK )
            {
                SendEquipError( msg, pSrcItem, NULL );
                return;
            }

            RemoveItem(srcbag, srcslot, true);
            EquipItem( dest, pSrcItem, true);
            AutoUnequipOffhandIfNeed();
        }
    }
    else                                                    // if (!pDstItem)
    {
        if(pDstItem->m_lootGenerated)                       // prevent swap looting item
        {
            //best error message found for attempting to swap while looting
            SendEquipError( EQUIP_ERR_CANT_DO_RIGHT_NOW, pDstItem, NULL );
            return;
        }

        // check unequip potability for equipped items and bank bags
        if(IsEquipmentPos ( dst ) || IsBagPos ( dst ))
        {
            // bags can be swapped with empty bag slots
            uint8 msg = CanUnequipItem( dst, !IsBagPos ( dst ) || IsBagPos ( src ) );
            if(msg != EQUIP_ERR_OK)
            {
                SendEquipError( msg, pSrcItem, pDstItem );
                return;
            }
        }

        // attempt merge to / fill target item
        {
            uint8 msg;
            ItemPosCountVec sDest;
            uint16 eDest;
            if( IsInventoryPos( dst ) )
                msg = CanStoreItem( dstbag, dstslot, sDest, pSrcItem, false );
            else if( IsBankPos ( dst ) )
                msg = CanBankItem( dstbag, dstslot, sDest, pSrcItem, false );
            else if( IsEquipmentPos ( dst ) )
                msg = CanEquipItem( dstslot, eDest, pSrcItem, false );
            else
                return;

            // can be merge/fill
            if(msg == EQUIP_ERR_OK)
            {
                if( pSrcItem->GetCount() + pDstItem->GetCount() <= pSrcItem->GetProto()->Stackable )
                {
                    RemoveItem(srcbag, srcslot, true);

                    if( IsInventoryPos( dst ) )
                        StoreItem( sDest, pSrcItem, true);
                    else if( IsBankPos ( dst ) )
                        BankItem( sDest, pSrcItem, true);
                    else if( IsEquipmentPos ( dst ) )
                    {
                        EquipItem( eDest, pSrcItem, true);
                        AutoUnequipOffhandIfNeed();
                    }
                }
                else
                {
                    pSrcItem->SetCount( pSrcItem->GetCount() + pDstItem->GetCount() - pSrcItem->GetProto()->Stackable );
                    pDstItem->SetCount( pSrcItem->GetProto()->Stackable );
                    pSrcItem->SetState(ITEM_CHANGED, this);
                    pDstItem->SetState(ITEM_CHANGED, this);
                    if( IsInWorld() )
                    {
                        pSrcItem->SendUpdateToPlayer( this );
                        pDstItem->SendUpdateToPlayer( this );
                    }
                }
                return;
            }
        }

        // impossible merge/fill, do real swap
        uint8 msg;

        // check src->dest move possibility
        ItemPosCountVec sDest;
        uint16 eDest;
        if( IsInventoryPos( dst ) )
            msg = CanStoreItem( dstbag, dstslot, sDest, pSrcItem, true );
        else if( IsBankPos( dst ) )
            msg = CanBankItem( dstbag, dstslot, sDest, pSrcItem, true );
        else if( IsEquipmentPos( dst ) )
        {
            msg = CanEquipItem( dstslot, eDest, pSrcItem, true );
            if( msg == EQUIP_ERR_OK )
                msg = CanUnequipItem( eDest, true );
        }

        if( msg != EQUIP_ERR_OK )
        {
            SendEquipError( msg, pSrcItem, pDstItem );
            return;
        }

        // check dest->src move possibility
        ItemPosCountVec sDest2;
        uint16 eDest2;
        if( IsInventoryPos( src ) )
            msg = CanStoreItem( srcbag, srcslot, sDest2, pDstItem, true );
        else if( IsBankPos( src ) )
            msg = CanBankItem( srcbag, srcslot, sDest2, pDstItem, true );
        else if( IsEquipmentPos( src ) )
        {
            msg = CanEquipItem( srcslot, eDest2, pDstItem, true);
            if( msg == EQUIP_ERR_OK )
                msg = CanUnequipItem( eDest2, true);
        }

        if( msg != EQUIP_ERR_OK )
        {
            SendEquipError( msg, pDstItem, pSrcItem );
            return;
        }

        // now do moves, remove...
        RemoveItem(dstbag, dstslot, false);
        RemoveItem(srcbag, srcslot, false);

        // add to dest
        if( IsInventoryPos( dst ) )
            StoreItem(sDest, pSrcItem, true);
        else if( IsBankPos( dst ) )
            BankItem(sDest, pSrcItem, true);
        else if( IsEquipmentPos( dst ) )
            EquipItem(eDest, pSrcItem, true);

        // add to src
        if( IsInventoryPos( src ) )
            StoreItem(sDest2, pDstItem, true);
        else if( IsBankPos( src ) )
            BankItem(sDest2, pDstItem, true);
        else if( IsEquipmentPos( src ) )
            EquipItem(eDest2, pDstItem, true);

        AutoUnequipOffhandIfNeed();
    }
}

void Player::AddItemToBuyBackSlot( Item *pItem )
{
    if( pItem )
    {
        uint32 slot = m_currentBuybackSlot;
        // if current back slot non-empty search oldest or free
        if(m_items[slot])
        {
            uint32 oldest_time = GetUInt32Value( PLAYER_FIELD_BUYBACK_TIMESTAMP_1 );
            uint32 oldest_slot = BUYBACK_SLOT_START;

            for(uint32 i = BUYBACK_SLOT_START+1; i < BUYBACK_SLOT_END; ++i )
            {
                // found empty
                if(!m_items[i])
                {
                    slot = i;
                    break;
                }

                uint32 i_time = GetUInt32Value( PLAYER_FIELD_BUYBACK_TIMESTAMP_1 + i - BUYBACK_SLOT_START);

                if(oldest_time > i_time)
                {
                    oldest_time = i_time;
                    oldest_slot = i;
                }
            }

            // find oldest
            slot = oldest_slot;
        }

        RemoveItemFromBuyBackSlot( slot, true );

        m_items[slot] = pItem;
        time_t base = time(NULL);
        uint32 etime = uint32(base - m_logintime + (30 * 3600));
        uint32 eslot = slot - BUYBACK_SLOT_START;

        SetUInt64Value( PLAYER_FIELD_VENDORBUYBACK_SLOT_1 + eslot * 2, pItem->GetGUID() );
        ItemPrototype const *pProto = pItem->GetProto();
        if( pProto )
            SetUInt32Value( PLAYER_FIELD_BUYBACK_PRICE_1 + eslot, pProto->SellPrice * pItem->GetCount() );
        else
            SetUInt32Value( PLAYER_FIELD_BUYBACK_PRICE_1 + eslot, 0 );
        SetUInt32Value( PLAYER_FIELD_BUYBACK_TIMESTAMP_1 + eslot, (uint32)etime );

        // move to next (for non filled list is move most optimized choice)
        if(m_currentBuybackSlot < BUYBACK_SLOT_END-1)
            ++m_currentBuybackSlot;
    }
}

Item* Player::GetItemFromBuyBackSlot( uint32 slot )
{
    if( slot >= BUYBACK_SLOT_START && slot < BUYBACK_SLOT_END )
        return m_items[slot];
    return NULL;
}

void Player::RemoveItemFromBuyBackSlot( uint32 slot, bool del )
{
    if( slot >= BUYBACK_SLOT_START && slot < BUYBACK_SLOT_END )
    {
        Item *pItem = m_items[slot];
        if( pItem )
        {
            pItem->RemoveFromWorld();
            if(del) pItem->SetState(ITEM_REMOVED, this);
        }

        m_items[slot] = NULL;

        uint32 eslot = slot - BUYBACK_SLOT_START;
        SetUInt64Value( PLAYER_FIELD_VENDORBUYBACK_SLOT_1 + eslot * 2, 0 );
        SetUInt32Value( PLAYER_FIELD_BUYBACK_PRICE_1 + eslot, 0 );
        SetUInt32Value( PLAYER_FIELD_BUYBACK_TIMESTAMP_1 + eslot, 0 );

        // if current backslot is filled set to now free slot
        if(m_items[m_currentBuybackSlot])
            m_currentBuybackSlot = slot;
    }
}

void Player::SendEquipError( uint8 msg, Item* pItem, Item *pItem2 )
{
    WorldPacket data( SMSG_INVENTORY_CHANGE_FAILURE, (msg == EQUIP_ERR_CANT_EQUIP_LEVEL_I ? 22 : 18) );
    data << uint8(msg);

    if(msg)
    {
        data << uint64(pItem ? pItem->GetGUID() : 0);
        data << uint64(pItem2 ? pItem2->GetGUID() : 0);
        data << uint8(0);                                   // not 0 there...

        if(msg == EQUIP_ERR_CANT_EQUIP_LEVEL_I)
        {
            uint32 level = 0;

            if(pItem)
                if(ItemPrototype const* proto =  pItem->GetProto())
                    level = proto->RequiredLevel;

            data << uint32(level);                          // new 2.4.0
        }
    }
    GetSession()->SendPacket(&data);
}

void Player::SendBuyError( uint8 msg, Creature* pCreature, uint32 item, uint32 param )
{
    WorldPacket data( SMSG_BUY_FAILED, (8+4+4+1) );
    data << uint64(pCreature ? pCreature->GetGUID() : 0);
    data << uint32(item);
    if( param > 0 )
        data << uint32(param);
    data << uint8(msg);
    GetSession()->SendPacket(&data);
}

void Player::SendSellError( uint8 msg, Creature* pCreature, uint64 guid, uint32 param )
{
    WorldPacket data( SMSG_SELL_ITEM,(8+8+(param?4:0)+1));  // last check 2.0.10
    data << uint64(pCreature ? pCreature->GetGUID() : 0);
    data << uint64(guid);
    if( param > 0 )
        data << uint32(param);
    data << uint8(msg);
    GetSession()->SendPacket(&data);
}

void Player::ClearTrade()
{
    tradeGold = 0;
    acceptTrade = false;
    for(int i = 0; i < TRADE_SLOT_COUNT; i++)
        tradeItems[i] = NULL_SLOT;
}

void Player::TradeCancel(bool sendback)
{
    if(pTrader)
    {
        // send yellow "Trade canceled" message to both traders
        WorldSession* ws;
        ws = GetSession();
        if(sendback)
            ws->SendCancelTrade();
        ws = pTrader->GetSession();
        if(!ws->PlayerLogout())
            ws->SendCancelTrade();

        // cleanup
        ClearTrade();
        pTrader->ClearTrade();
        // prevent loss of reference
        pTrader->pTrader = NULL;
        pTrader = NULL;
    }
}

void Player::UpdateItemDuration(uint32 time, bool realtimeonly)
{
    if(m_itemDuration.empty())
        return;

    for(ItemDurationList::iterator itr = m_itemDuration.begin();itr != m_itemDuration.end(); )
    {
        Item* item = *itr;
        ++itr;                                              // current element can be erased in UpdateDuration

        if (realtimeonly && item->GetProto()->Duration < 0 || !realtimeonly)
            item->UpdateDuration(this,time);
    }
}

void Player::UpdateEnchantTime(uint32 time)
{
    for(EnchantDurationList::iterator itr = m_enchantDuration.begin(),next;itr != m_enchantDuration.end();itr=next)
    {
        assert(itr->item);
        next=itr;
        if(!itr->item->GetEnchantmentId(itr->slot))
        {
            next = m_enchantDuration.erase(itr);
        }
        else if(itr->leftduration <= time)
        {
            ApplyEnchantment(itr->item,itr->slot,false,false);
            itr->item->ClearEnchantment(itr->slot);
            next = m_enchantDuration.erase(itr);
        }
        else if(itr->leftduration > time)
        {
            itr->leftduration -= time;
            ++next;
        }
    }
}

void Player::AddEnchantmentDurations(Item *item)
{
    for(int x=0;x<MAX_ENCHANTMENT_SLOT;++x)
    {
        if(!item->GetEnchantmentId(EnchantmentSlot(x)))
            continue;

        uint32 duration = item->GetEnchantmentDuration(EnchantmentSlot(x));
        if( duration > 0 )
            AddEnchantmentDuration(item,EnchantmentSlot(x),duration);
    }
}

void Player::RemoveEnchantmentDurations(Item *item)
{
    for(EnchantDurationList::iterator itr = m_enchantDuration.begin();itr != m_enchantDuration.end();)
    {
        if(itr->item == item)
        {
            // save duration in item
            item->SetEnchantmentDuration(EnchantmentSlot(itr->slot),itr->leftduration);
            itr = m_enchantDuration.erase(itr);
        }
        else
            ++itr;
    }
}

void Player::RemoveAllEnchantments(EnchantmentSlot slot, bool arena)
{
    // remove enchantments from equipped items first to clean up the m_enchantDuration list
    for(EnchantDurationList::iterator itr = m_enchantDuration.begin(),next;itr != m_enchantDuration.end();itr=next)
    {
        next = itr;
        if(itr->slot==slot)
        {
            if(arena && itr->item)
            {
                uint32 enchant_id = itr->item->GetEnchantmentId(slot);
                if(enchant_id)
                {
                    SpellItemEnchantmentEntry const *pEnchant = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
                    if(pEnchant && pEnchant->aura_id == ITEM_ENCHANTMENT_AURAID_POISON)
                    {
                        ++next;
                        continue;
                    }
                }
            }
            if(itr->item && itr->item->GetEnchantmentId(slot))
            {
                // remove from stats
                ApplyEnchantment(itr->item,slot,false,false);
                // remove visual
                itr->item->ClearEnchantment(slot);
            }
            // remove from update list
            next = m_enchantDuration.erase(itr);
        }
        else
            ++next;
    }

    // remove enchants from inventory items
    // NOTE: no need to remove these from stats, since these aren't equipped
    // in inventory
    for(int i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; i++)
    {
        Item* pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
        if (!pItem)
            continue;
        uint32 enchant_id = pItem->GetEnchantmentId(slot);
        if (enchant_id) {
            SpellItemEnchantmentEntry const *pEnchant = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
            if (arena && pEnchant && pEnchant->aura_id == ITEM_ENCHANTMENT_AURAID_POISON)
                continue;
            
            pItem->ClearEnchantment(slot);
        }
    }

    // in inventory bags
    for(int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; i++)
    {
        Bag* pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, i );
        if( pBag )
        {
            for(uint32 j = 0; j < pBag->GetBagSize(); j++)
            {
                Item* pItem = pBag->GetItemByPos(j);
                if (!pItem)
                    continue;
                uint32 enchant_id = pItem->GetEnchantmentId(slot);
                if (enchant_id) {
                    SpellItemEnchantmentEntry const *pEnchant = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
                    if (arena && pEnchant && pEnchant->aura_id == ITEM_ENCHANTMENT_AURAID_POISON)
                        continue;
                    
                    pItem->ClearEnchantment(slot);
                }
            }
        }
    }
}

// duration == 0 will remove item enchant
void Player::AddEnchantmentDuration(Item *item,EnchantmentSlot slot,uint32 duration)
{
    if(!item)
        return;

    if(slot >= MAX_ENCHANTMENT_SLOT)
        return;

    for(EnchantDurationList::iterator itr = m_enchantDuration.begin();itr != m_enchantDuration.end();++itr)
    {
        if(itr->item == item && itr->slot == slot)
        {
            itr->item->SetEnchantmentDuration(itr->slot,itr->leftduration);
            m_enchantDuration.erase(itr);
            break;
        }
    }
    if(item && duration > 0 )
    {
        GetSession()->SendItemEnchantTimeUpdate(GetGUID(), item->GetGUID(),slot,uint32(duration/1000));
        m_enchantDuration.push_back(EnchantDuration(item,slot,duration));
    }
}

void Player::ApplyEnchantment(Item *item,bool apply)
{
    for(uint32 slot = 0; slot < MAX_ENCHANTMENT_SLOT; ++slot)
        ApplyEnchantment(item, EnchantmentSlot(slot), apply);
}

void Player::ApplyEnchantment(Item *item,EnchantmentSlot slot,bool apply, bool apply_dur, bool ignore_condition)
{
    if(!item)
        return;

    if(!item->IsEquipped())
        return;

    if(slot >= MAX_ENCHANTMENT_SLOT)
        return;

    uint32 enchant_id = item->GetEnchantmentId(slot);
    if(!enchant_id)
        return;

    SpellItemEnchantmentEntry const *pEnchant = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
    if(!pEnchant)
        return;

    if(!ignore_condition && pEnchant->EnchantmentCondition && !(this->ToPlayer())->EnchantmentFitsRequirements(pEnchant->EnchantmentCondition, -1))
        return;

    for (int s=0; s<3; s++)
    {
        uint32 enchant_display_type = pEnchant->type[s];
        uint32 enchant_amount = pEnchant->amount[s];
        uint32 enchant_spell_id = pEnchant->spellid[s];

        switch(enchant_display_type)
        {
            case ITEM_ENCHANTMENT_TYPE_NONE:
                break;
            case ITEM_ENCHANTMENT_TYPE_COMBAT_SPELL:
                // processed in Player::CastItemCombatSpell
                break;
            case ITEM_ENCHANTMENT_TYPE_DAMAGE:
                if (item->GetSlot() == EQUIPMENT_SLOT_MAINHAND)
                    HandleStatModifier(UNIT_MOD_DAMAGE_MAINHAND, TOTAL_VALUE, float(enchant_amount), apply);
                else if (item->GetSlot() == EQUIPMENT_SLOT_OFFHAND)
                    HandleStatModifier(UNIT_MOD_DAMAGE_OFFHAND, TOTAL_VALUE, float(enchant_amount), apply);
                else if (item->GetSlot() == EQUIPMENT_SLOT_RANGED)
                    HandleStatModifier(UNIT_MOD_DAMAGE_RANGED, TOTAL_VALUE, float(enchant_amount), apply);
                break;
            case ITEM_ENCHANTMENT_TYPE_EQUIP_SPELL:
                if(enchant_spell_id)
                {
                    // Hack, Flametongue Weapon
                    if (enchant_spell_id==10400 || enchant_spell_id==15567 ||
                        enchant_spell_id==15568 || enchant_spell_id==15569 ||
                        enchant_spell_id==16311 || enchant_spell_id==16312 ||
                        enchant_spell_id==16313)
                    {
                        // processed in Player::CastItemCombatSpell
                        break;
                    }
                    else if(apply)
                    {
                        int32 basepoints = 0;
                        // Random Property Exist - try found basepoints for spell (basepoints depends from item suffix factor)
                        if (item->GetItemRandomPropertyId())
                        {
                            ItemRandomSuffixEntry const *item_rand = sItemRandomSuffixStore.LookupEntry(abs(item->GetItemRandomPropertyId()));
                            if (item_rand)
                            {
                                // Search enchant_amount
                                for (int k=0; k<3; k++)
                                {
                                    if(item_rand->enchant_id[k] == enchant_id)
                                    {
                                        basepoints = int32((item_rand->prefix[k]*item->GetItemSuffixFactor()) / 10000 );
                                        break;
                                    }
                                }
                            }
                        }
                        // Cast custom spell vs all equal basepoints getted from enchant_amount
                        if (basepoints)
                            CastCustomSpell(this,enchant_spell_id,&basepoints,&basepoints,&basepoints,true,item);
                        else
                            CastSpell(this,enchant_spell_id,true,item);
                    }
                    else
                        RemoveAurasDueToItemSpell(item,enchant_spell_id);
                }
                break;
            case ITEM_ENCHANTMENT_TYPE_RESISTANCE:
                if (!enchant_amount)
                {
                    ItemRandomSuffixEntry const *item_rand = sItemRandomSuffixStore.LookupEntry(abs(item->GetItemRandomPropertyId()));
                    if(item_rand)
                    {
                        for (int k=0; k<3; k++)
                        {
                            if(item_rand->enchant_id[k] == enchant_id)
                            {
                                enchant_amount = uint32((item_rand->prefix[k]*item->GetItemSuffixFactor()) / 10000 );
                                break;
                            }
                        }
                    }
                }

                HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + enchant_spell_id), TOTAL_VALUE, float(enchant_amount), apply);
                break;
            case ITEM_ENCHANTMENT_TYPE_STAT:
            {
                if (!enchant_amount)
                {
                    ItemRandomSuffixEntry const *item_rand_suffix = sItemRandomSuffixStore.LookupEntry(abs(item->GetItemRandomPropertyId()));
                    if(item_rand_suffix)
                    {
                        for (int k=0; k<3; k++)
                        {
                            if(item_rand_suffix->enchant_id[k] == enchant_id)
                            {
                                enchant_amount = uint32((item_rand_suffix->prefix[k]*item->GetItemSuffixFactor()) / 10000 );
                                break;
                            }
                        }
                    }
                }

                switch (enchant_spell_id)
                {
                    case ITEM_MOD_AGILITY:
                        HandleStatModifier(UNIT_MOD_STAT_AGILITY, TOTAL_VALUE, float(enchant_amount), apply);
                        ApplyStatBuffMod(STAT_AGILITY, enchant_amount, apply);
                        break;
                    case ITEM_MOD_STRENGTH:
                        HandleStatModifier(UNIT_MOD_STAT_STRENGTH, TOTAL_VALUE, float(enchant_amount), apply);
                        ApplyStatBuffMod(STAT_STRENGTH, enchant_amount, apply);
                        break;
                    case ITEM_MOD_INTELLECT:
                        HandleStatModifier(UNIT_MOD_STAT_INTELLECT, TOTAL_VALUE, float(enchant_amount), apply);
                        ApplyStatBuffMod(STAT_INTELLECT, enchant_amount, apply);
                        break;
                    case ITEM_MOD_SPIRIT:
                        HandleStatModifier(UNIT_MOD_STAT_SPIRIT, TOTAL_VALUE, float(enchant_amount), apply);
                        ApplyStatBuffMod(STAT_SPIRIT, enchant_amount, apply);
                        break;
                    case ITEM_MOD_STAMINA:
                        HandleStatModifier(UNIT_MOD_STAT_STAMINA, TOTAL_VALUE, float(enchant_amount), apply);
                        ApplyStatBuffMod(STAT_STAMINA, enchant_amount, apply);
                        break;
                    case ITEM_MOD_DEFENSE_SKILL_RATING:
                        (this->ToPlayer())->ApplyRatingMod(CR_DEFENSE_SKILL, enchant_amount, apply);
                        break;
                    case  ITEM_MOD_DODGE_RATING:
                        (this->ToPlayer())->ApplyRatingMod(CR_DODGE, enchant_amount, apply);
                        break;
                    case ITEM_MOD_PARRY_RATING:
                        (this->ToPlayer())->ApplyRatingMod(CR_PARRY, enchant_amount, apply);
                        break;
                    case ITEM_MOD_BLOCK_RATING:
                        (this->ToPlayer())->ApplyRatingMod(CR_BLOCK, enchant_amount, apply);
                        break;
                    case ITEM_MOD_HIT_MELEE_RATING:
                        (this->ToPlayer())->ApplyRatingMod(CR_HIT_MELEE, enchant_amount, apply);
                        break;
                    case ITEM_MOD_HIT_RANGED_RATING:
                        (this->ToPlayer())->ApplyRatingMod(CR_HIT_RANGED, enchant_amount, apply);
                        break;
                    case ITEM_MOD_HIT_SPELL_RATING:
                        (this->ToPlayer())->ApplyRatingMod(CR_HIT_SPELL, enchant_amount, apply);
                        break;
                    case ITEM_MOD_CRIT_MELEE_RATING:
                        (this->ToPlayer())->ApplyRatingMod(CR_CRIT_MELEE, enchant_amount, apply);
                        break;
                    case ITEM_MOD_CRIT_RANGED_RATING:
                        (this->ToPlayer())->ApplyRatingMod(CR_CRIT_RANGED, enchant_amount, apply);
                        break;
                    case ITEM_MOD_CRIT_SPELL_RATING:
                        (this->ToPlayer())->ApplyRatingMod(CR_CRIT_SPELL, enchant_amount, apply);
                        break;
//                    Values from ITEM_STAT_MELEE_HA_RATING to ITEM_MOD_HASTE_RANGED_RATING are never used
//                    in Enchantments
//                    case ITEM_MOD_HIT_TAKEN_MELEE_RATING:
//                        (this->ToPlayer())->ApplyRatingMod(CR_HIT_TAKEN_MELEE, enchant_amount, apply);
//                        break;
//                    case ITEM_MOD_HIT_TAKEN_RANGED_RATING:
//                        (this->ToPlayer())->ApplyRatingMod(CR_HIT_TAKEN_RANGED, enchant_amount, apply);
//                        break;
//                    case ITEM_MOD_HIT_TAKEN_SPELL_RATING:
//                        (this->ToPlayer())->ApplyRatingMod(CR_HIT_TAKEN_SPELL, enchant_amount, apply);
//                        break;
//                    case ITEM_MOD_CRIT_TAKEN_MELEE_RATING:
//                        (this->ToPlayer())->ApplyRatingMod(CR_CRIT_TAKEN_MELEE, enchant_amount, apply);
//                        break;
//                    case ITEM_MOD_CRIT_TAKEN_RANGED_RATING:
//                        (this->ToPlayer())->ApplyRatingMod(CR_CRIT_TAKEN_RANGED, enchant_amount, apply);
//                        break;
//                    case ITEM_MOD_CRIT_TAKEN_SPELL_RATING:
//                        (this->ToPlayer())->ApplyRatingMod(CR_CRIT_TAKEN_SPELL, enchant_amount, apply);
//                        break;
//                    case ITEM_MOD_HASTE_MELEE_RATING:
//                        (this->ToPlayer())->ApplyRatingMod(CR_HASTE_MELEE, enchant_amount, apply);
//                        break;
//                    case ITEM_MOD_HASTE_RANGED_RATING:
//                        (this->ToPlayer())->ApplyRatingMod(CR_HASTE_RANGED, enchant_amount, apply);
//                        break;
                    case ITEM_MOD_HASTE_SPELL_RATING:
                        (this->ToPlayer())->ApplyRatingMod(CR_HASTE_SPELL, enchant_amount, apply);
                        break;
                    case ITEM_MOD_HIT_RATING:
                        (this->ToPlayer())->ApplyRatingMod(CR_HIT_MELEE, enchant_amount, apply);
                        (this->ToPlayer())->ApplyRatingMod(CR_HIT_RANGED, enchant_amount, apply);
                        (this->ToPlayer())->ApplyRatingMod(CR_HIT_SPELL, enchant_amount, apply);
                        break;
                    case ITEM_MOD_CRIT_RATING:
                        (this->ToPlayer())->ApplyRatingMod(CR_CRIT_MELEE, enchant_amount, apply);
                        (this->ToPlayer())->ApplyRatingMod(CR_CRIT_RANGED, enchant_amount, apply);
                        (this->ToPlayer())->ApplyRatingMod(CR_CRIT_SPELL, enchant_amount, apply);
                        break;
//                    Values ITEM_MOD_HIT_TAKEN_RATING and ITEM_MOD_CRIT_TAKEN_RATING are never used in Enchantment
//                    case ITEM_MOD_HIT_TAKEN_RATING:
//                          (this->ToPlayer())->ApplyRatingMod(CR_HIT_TAKEN_MELEE, enchant_amount, apply);
//                          (this->ToPlayer())->ApplyRatingMod(CR_HIT_TAKEN_RANGED, enchant_amount, apply);
//                          (this->ToPlayer())->ApplyRatingMod(CR_HIT_TAKEN_SPELL, enchant_amount, apply);
//                        break;
//                    case ITEM_MOD_CRIT_TAKEN_RATING:
//                          (this->ToPlayer())->ApplyRatingMod(CR_CRIT_TAKEN_MELEE, enchant_amount, apply);
//                          (this->ToPlayer())->ApplyRatingMod(CR_CRIT_TAKEN_RANGED, enchant_amount, apply);
//                          (this->ToPlayer())->ApplyRatingMod(CR_CRIT_TAKEN_SPELL, enchant_amount, apply);
//                        break;
                    case ITEM_MOD_RESILIENCE_RATING:
                        (this->ToPlayer())->ApplyRatingMod(CR_CRIT_TAKEN_MELEE, enchant_amount, apply);
                        (this->ToPlayer())->ApplyRatingMod(CR_CRIT_TAKEN_RANGED, enchant_amount, apply);
                        (this->ToPlayer())->ApplyRatingMod(CR_CRIT_TAKEN_SPELL, enchant_amount, apply);
                        break;
                    case ITEM_MOD_HASTE_RATING:
                        (this->ToPlayer())->ApplyRatingMod(CR_HASTE_MELEE, enchant_amount, apply);
                        (this->ToPlayer())->ApplyRatingMod(CR_HASTE_RANGED, enchant_amount, apply);
                        (this->ToPlayer())->ApplyRatingMod(CR_HASTE_SPELL, enchant_amount, apply);
                        break;
                    case ITEM_MOD_EXPERTISE_RATING:
                        (this->ToPlayer())->ApplyRatingMod(CR_EXPERTISE, enchant_amount, apply);
                        break;
                    default:
                        break;
                }
                break;
            }
            case ITEM_ENCHANTMENT_TYPE_TOTEM:               // Shaman Rockbiter Weapon
            {
                if(getClass() == CLASS_SHAMAN)
                {
                    float addValue = 0.0f;
                    if(item->GetSlot() == EQUIPMENT_SLOT_MAINHAND)
                    {
                        addValue = float(enchant_amount * item->GetProto()->Delay/1000.0f);
                        HandleStatModifier(UNIT_MOD_DAMAGE_MAINHAND, TOTAL_VALUE, addValue, apply);
                    }
                    else if(item->GetSlot() == EQUIPMENT_SLOT_OFFHAND )
                    {
                        addValue = float(enchant_amount * item->GetProto()->Delay/1000.0f);
                        HandleStatModifier(UNIT_MOD_DAMAGE_OFFHAND, TOTAL_VALUE, addValue, apply);
                    }
                }
                break;
            }
            default:
                sLog.outError("Unknown item enchantment display type: %d",enchant_display_type);
                break;
        }                                                   /*switch(enchant_display_type)*/
    }                                                       /*for*/

    // visualize enchantment at player and equipped items
    if(slot < MAX_INSPECTED_ENCHANTMENT_SLOT)
    {
        int VisibleBase = PLAYER_VISIBLE_ITEM_1_0 + (item->GetSlot() * MAX_VISIBLE_ITEM_OFFSET);
        SetUInt32Value(VisibleBase + 1 + slot, apply? item->GetEnchantmentId(slot) : 0);
    }

    if(apply_dur)
    {
        if(apply)
        {
            // set duration
            uint32 duration = item->GetEnchantmentDuration(slot);
            if(duration > 0)
                AddEnchantmentDuration(item,slot,duration);
        }
        else
        {
            // duration == 0 will remove EnchantDuration
            AddEnchantmentDuration(item,slot,0);
        }
    }
}

void Player::SendEnchantmentDurations()
{
    for(EnchantDurationList::iterator itr = m_enchantDuration.begin();itr != m_enchantDuration.end();++itr)
    {
        GetSession()->SendItemEnchantTimeUpdate(GetGUID(), itr->item->GetGUID(),itr->slot,uint32(itr->leftduration)/1000);
    }
}

void Player::SendItemDurations()
{
    for(ItemDurationList::iterator itr = m_itemDuration.begin();itr != m_itemDuration.end();++itr)
    {
        (*itr)->SendTimeUpdate(this);
    }
}

void Player::SendNewItem(Item *item, uint32 count, bool received, bool created, bool broadcast)
{
    if(!item)                                               // prevent crash
        return;

                                                            // last check 2.0.10
    WorldPacket data( SMSG_ITEM_PUSH_RESULT, (8+4+4+4+1+4+4+4+4+4) );
    data << GetGUID();                                      // player GUID
    data << uint32(received);                               // 0=looted, 1=from npc
    data << uint32(created);                                // 0=received, 1=created
    data << uint32(1);                                      // always 0x01 (probably meant to be count of listed items)
    data << (uint8)item->GetBagSlot();                      // bagslot
                                                            // item slot, but when added to stack: 0xFFFFFFFF
    data << (uint32) ((item->GetCount()==count) ? item->GetSlot() : -1);
    data << uint32(item->GetEntry());                       // item id
    data << uint32(item->GetItemSuffixFactor());            // SuffixFactor
    data << uint32(item->GetItemRandomPropertyId());        // random item property id
    data << uint32(count);                                  // count of items
    data << GetItemCount(item->GetEntry());                 // count of items in inventory

    if (broadcast && GetGroup())
        GetGroup()->BroadcastPacket(&data, true);
    else
        GetSession()->SendPacket(&data);
}

/*********************************************************/
/***                    QUEST SYSTEM                   ***/
/*********************************************************/

void Player::PrepareQuestMenu( uint64 guid )
{
    Object *pObject;
    QuestRelations* pObjectQR;
    QuestRelations* pObjectQIR;
    Creature *pCreature = ObjectAccessor::GetCreature(*this, guid);
    if( pCreature )
    {
        pObject = (Object*)pCreature;
        pObjectQR  = &objmgr.mCreatureQuestRelations;
        pObjectQIR = &objmgr.mCreatureQuestInvolvedRelations;
    }
    else
    {
        GameObject *pGameObject = ObjectAccessor::GetGameObject(*this, guid);
        if( pGameObject )
        {
            pObject = (Object*)pGameObject;
            pObjectQR  = &objmgr.mGOQuestRelations;
            pObjectQIR = &objmgr.mGOQuestInvolvedRelations;
        }
        else
            return;
    }

    QuestMenu &qm = PlayerTalkClass->GetQuestMenu();
    qm.ClearMenu();

    for(QuestRelations::const_iterator i = pObjectQIR->lower_bound(pObject->GetEntry()); i != pObjectQIR->upper_bound(pObject->GetEntry()); ++i)
    {
        uint32 quest_id = i->second;
        QuestStatus status = GetQuestStatus( quest_id );
        if ( status == QUEST_STATUS_COMPLETE && !GetQuestRewardStatus( quest_id ) )
            qm.AddMenuItem(quest_id, DIALOG_STATUS_REWARD_REP);
        else if ( status == QUEST_STATUS_INCOMPLETE )
            qm.AddMenuItem(quest_id, DIALOG_STATUS_INCOMPLETE);
        else if (status == QUEST_STATUS_AVAILABLE )
            qm.AddMenuItem(quest_id, DIALOG_STATUS_CHAT);
    }

    for(QuestRelations::const_iterator i = pObjectQR->lower_bound(pObject->GetEntry()); i != pObjectQR->upper_bound(pObject->GetEntry()); ++i)
    {
        uint32 quest_id = i->second;
        Quest const* pQuest = objmgr.GetQuestTemplate(quest_id);
        if(!pQuest) continue;

        QuestStatus status = GetQuestStatus( quest_id );

        if (pQuest->IsAutoComplete() && CanTakeQuest(pQuest, false))
            qm.AddMenuItem(quest_id, DIALOG_STATUS_REWARD_REP);
        else if ( status == QUEST_STATUS_NONE && CanTakeQuest( pQuest, false ) ) {
            if (pCreature && pCreature->GetQuestPoolId()) {
                if (!sWorld.IsQuestInAPool(quest_id)) {
                    qm.AddMenuItem(quest_id, DIALOG_STATUS_CHAT);
                    continue;
                }
                // Quest is in a pool, check if it's current
                if (sWorld.GetCurrentQuestForPool(pCreature->GetQuestPoolId()) != quest_id)
                    continue;
                else 
                    qm.AddMenuItem(quest_id, DIALOG_STATUS_CHAT);
            }
            else    // No quest pool, just add it
                qm.AddMenuItem(quest_id, DIALOG_STATUS_AVAILABLE);
        }
    }
}

void Player::SendPreparedQuest( uint64 guid )
{
    QuestMenu& questMenu = PlayerTalkClass->GetQuestMenu();
    if( questMenu.Empty() )
        return;

    QuestMenuItem const& qmi0 = questMenu.GetItem( 0 );

    uint32 status = qmi0.m_qIcon;

    // single element case
    if ( questMenu.MenuItemCount() == 1 )
    {
        // Auto open -- maybe also should verify there is no greeting
        uint32 quest_id = qmi0.m_qId;
        Quest const* pQuest = objmgr.GetQuestTemplate(quest_id);
        if ( pQuest )
        {
            if( status == DIALOG_STATUS_REWARD_REP && !GetQuestRewardStatus( quest_id ) )
                PlayerTalkClass->SendQuestGiverRequestItems( pQuest, guid, CanRewardQuest(pQuest,false), true );
            else if( status == DIALOG_STATUS_INCOMPLETE )
                PlayerTalkClass->SendQuestGiverRequestItems( pQuest, guid, false, true );
            // Send completable on repeatable quest if player don't have quest
            else if( pQuest->IsRepeatable() && !pQuest->IsDaily() )
                PlayerTalkClass->SendQuestGiverRequestItems( pQuest, guid, CanCompleteRepeatableQuest(pQuest), true );
            else
                PlayerTalkClass->SendQuestGiverQuestDetails( pQuest, guid, true );
        }
    }
    // multiply entries
    else
    {
        QEmote qe;
        qe._Delay = 0;
        qe._Emote = 0;
        std::string title = "";
        Creature *pCreature = ObjectAccessor::GetCreature(*this, guid);
        if( pCreature )
        {
            uint32 textid = pCreature->GetNpcTextId();
            GossipText * gossiptext = objmgr.GetGossipText(textid);
            if( !gossiptext )
            {
                qe._Delay = 0;                              //TEXTEMOTE_MESSAGE;              //zyg: player emote
                qe._Emote = 0;                              //TEXTEMOTE_HELLO;                //zyg: NPC emote
                title = "";
            }
            else
            {
                qe = gossiptext->Options[0].Emotes[0];

                if(!gossiptext->Options[0].Text_0.empty())
                {
                    title = gossiptext->Options[0].Text_0;

                    int loc_idx = GetSession()->GetSessionDbLocaleIndex();
                    if (loc_idx >= 0)
                    {
                        NpcTextLocale const *nl = objmgr.GetNpcTextLocale(textid);
                        if (nl)
                        {
                            if (nl->Text_0[0].size() > loc_idx && !nl->Text_0[0][loc_idx].empty())
                                title = nl->Text_0[0][loc_idx];
                        }
                    }
                }
                else
                {
                    title = gossiptext->Options[0].Text_1;

                    int loc_idx = GetSession()->GetSessionDbLocaleIndex();
                    if (loc_idx >= 0)
                    {
                        NpcTextLocale const *nl = objmgr.GetNpcTextLocale(textid);
                        if (nl)
                        {
                            if (nl->Text_1[0].size() > loc_idx && !nl->Text_1[0][loc_idx].empty())
                                title = nl->Text_1[0][loc_idx];
                        }
                    }
                }
            }
        }
        PlayerTalkClass->SendQuestGiverQuestList( qe, title, guid );
    }
}

bool Player::IsActiveQuest( uint32 quest_id ) const
{
    QuestStatusMap::const_iterator itr = mQuestStatus.find(quest_id);

    return itr != mQuestStatus.end() && itr->second.m_status != QUEST_STATUS_NONE;
}

Quest const * Player::GetNextQuest( uint64 guid, Quest const *pQuest )
{
    Object *pObject;
    QuestRelations* pObjectQR;
    QuestRelations* pObjectQIR;

    Creature *pCreature = ObjectAccessor::GetCreature(*this, guid);
    if( pCreature )
    {
        pObject = (Object*)pCreature;
        pObjectQR  = &objmgr.mCreatureQuestRelations;
        pObjectQIR = &objmgr.mCreatureQuestInvolvedRelations;
    }
    else
    {
        GameObject *pGameObject = ObjectAccessor::GetGameObject(*this, guid);
        if( pGameObject )
        {
            pObject = (Object*)pGameObject;
            pObjectQR  = &objmgr.mGOQuestRelations;
            pObjectQIR = &objmgr.mGOQuestInvolvedRelations;
        }
        else
            return NULL;
    }

    uint32 nextQuestID = pQuest->GetNextQuestInChain();
    for(QuestRelations::const_iterator itr = pObjectQR->lower_bound(pObject->GetEntry()); itr != pObjectQR->upper_bound(pObject->GetEntry()); ++itr)
    {
        if (itr->second == nextQuestID)
            return objmgr.GetQuestTemplate(nextQuestID);
    }

    return NULL;
}

bool Player::CanSeeStartQuest( Quest const *pQuest )
{
    if( SatisfyQuestRace( pQuest, false ) && SatisfyQuestSkillOrClass( pQuest, false ) &&
        SatisfyQuestExclusiveGroup( pQuest, false ) && SatisfyQuestReputation( pQuest, false ) &&
        SatisfyQuestPreviousQuest( pQuest, false ) && SatisfyQuestNextChain( pQuest, false ) &&
        SatisfyQuestPrevChain( pQuest, false ) && SatisfyQuestDay( pQuest, false ) )
    {
        return getLevel() + sWorld.getConfig(CONFIG_QUEST_HIGH_LEVEL_HIDE_DIFF) >= pQuest->GetMinLevel();
    }

    return false;
}

bool Player::CanTakeQuest( Quest const *pQuest, bool msg )
{
    return SatisfyQuestStatus( pQuest, msg ) && SatisfyQuestExclusiveGroup( pQuest, msg )
        && SatisfyQuestRace( pQuest, msg ) && SatisfyQuestLevel( pQuest, msg )
        && SatisfyQuestSkillOrClass( pQuest, msg ) && SatisfyQuestReputation( pQuest, msg )
        && SatisfyQuestPreviousQuest( pQuest, msg ) && SatisfyQuestTimed( pQuest, msg )
        && SatisfyQuestNextChain( pQuest, msg ) && SatisfyQuestPrevChain( pQuest, msg )
        && SatisfyQuestDay( pQuest, msg );
}

bool Player::CanAddQuest( Quest const *pQuest, bool msg )
{
    if( !SatisfyQuestLog( msg ) )
        return false;

    uint32 srcitem = pQuest->GetSrcItemId();
    if( srcitem > 0 )
    {
        uint32 count = pQuest->GetSrcItemCount();
        ItemPosCountVec dest;
        uint8 msg = CanStoreNewItem( NULL_BAG, NULL_SLOT, dest, srcitem, count );

        // player already have max number (in most case 1) source item, no additional item needed and quest can be added.
        if( msg == EQUIP_ERR_CANT_CARRY_MORE_OF_THIS )
            return true;
        else if( msg != EQUIP_ERR_OK )
        {
            SendEquipError( msg, NULL, NULL );
            return false;
        }
    }
    return true;
}

bool Player::CanCompleteQuest( uint32 quest_id )
{
    if( quest_id )
    {
        QuestStatusData& q_status = mQuestStatus[quest_id];
        if( q_status.m_status == QUEST_STATUS_COMPLETE )
            return false;                                   // not allow re-complete quest

        Quest const* qInfo = objmgr.GetQuestTemplate(quest_id);

        if(!qInfo)
            return false;

        // auto complete quest
        if (qInfo->IsAutoComplete() && CanTakeQuest(qInfo, false))
            return true;

        if ( q_status.m_status == QUEST_STATUS_INCOMPLETE )
        {

            if ( qInfo->HasFlag( QUEST_TRINITY_FLAGS_DELIVER ) )
            {
                for(int i = 0; i < QUEST_OBJECTIVES_COUNT; i++)
                {
                    if( qInfo->ReqItemCount[i]!= 0 && q_status.m_itemcount[i] < qInfo->ReqItemCount[i] )
                        return false;
                }
            }

            if ( qInfo->HasFlag(QUEST_TRINITY_FLAGS_KILL_OR_CAST | QUEST_TRINITY_FLAGS_SPEAKTO) )
            {
                for(int i = 0; i < QUEST_OBJECTIVES_COUNT; i++)
                {
                    if( qInfo->ReqCreatureOrGOId[i] == 0 )
                        continue;

                    if( qInfo->ReqCreatureOrGOCount[i] != 0 && q_status.m_creatureOrGOcount[i] < qInfo->ReqCreatureOrGOCount[i] )
                        return false;
                }
            }

            if ( qInfo->HasFlag( QUEST_TRINITY_FLAGS_EXPLORATION_OR_EVENT ) && !q_status.m_explored )
                return false;

            if ( qInfo->HasFlag( QUEST_TRINITY_FLAGS_TIMED ) && q_status.m_timer == 0 )
                return false;

            if ( qInfo->GetRewOrReqMoney() < 0 )
            {
                if ( GetMoney() < uint32(-qInfo->GetRewOrReqMoney()) )
                    return false;
            }

            uint32 repFacId = qInfo->GetRepObjectiveFaction();
            if ( repFacId && GetReputation(repFacId) < qInfo->GetRepObjectiveValue() )
                return false;

            return true;
        }
    }
    return false;
}

bool Player::CanCompleteRepeatableQuest( Quest const *pQuest )
{
    // Solve problem that player don't have the quest and try complete it.
    // if repeatable she must be able to complete event if player don't have it.
    // Seem that all repeatable quest are DELIVER Flag so, no need to add more.
    if( !CanTakeQuest(pQuest, false) )
        return false;

    if (pQuest->HasFlag( QUEST_TRINITY_FLAGS_DELIVER) )
        for(int i = 0; i < QUEST_OBJECTIVES_COUNT; i++)
            if( pQuest->ReqItemId[i] && pQuest->ReqItemCount[i] && !HasItemCount(pQuest->ReqItemId[i],pQuest->ReqItemCount[i]) )
                return false;

    if( !CanRewardQuest(pQuest, false) )
        return false;

    return true;
}

bool Player::CanRewardQuest( Quest const *pQuest, bool msg )
{
    // not auto complete quest and not completed quest (only cheating case, then ignore without message)
    if(!pQuest->IsAutoComplete() && GetQuestStatus(pQuest->GetQuestId()) != QUEST_STATUS_COMPLETE)
        return false;

    // daily quest can't be rewarded (25 daily quest already completed)
    if(!SatisfyQuestDay(pQuest,true))
        return false;

    // rewarded and not repeatable quest (only cheating case, then ignore without message)
    if(GetQuestRewardStatus(pQuest->GetQuestId()))
        return false;

    // prevent receive reward with quest items in bank
    if ( pQuest->HasFlag( QUEST_TRINITY_FLAGS_DELIVER ) )
    {
        for(int i = 0; i < QUEST_OBJECTIVES_COUNT; i++)
        {
            if( pQuest->ReqItemCount[i]!= 0 &&
                GetItemCount(pQuest->ReqItemId[i]) < pQuest->ReqItemCount[i] )
            {
                if(msg)
                    SendEquipError( EQUIP_ERR_ITEM_NOT_FOUND, NULL, NULL );
                return false;
            }
        }
    }

    // prevent receive reward with low money and GetRewOrReqMoney() < 0
    if(pQuest->GetRewOrReqMoney() < 0 && GetMoney() < uint32(-pQuest->GetRewOrReqMoney()) )
        return false;

    return true;
}

bool Player::CanRewardQuest( Quest const *pQuest, uint32 reward, bool msg )
{
    // prevent receive reward with quest items in bank or for not completed quest
    if(!CanRewardQuest(pQuest,msg))
        return false;

    if ( pQuest->GetRewChoiceItemsCount() > 0 )
    {
        if( pQuest->RewChoiceItemId[reward] )
        {
            ItemPosCountVec dest;
            uint8 res = CanStoreNewItem( NULL_BAG, NULL_SLOT, dest, pQuest->RewChoiceItemId[reward], pQuest->RewChoiceItemCount[reward] );
            if( res != EQUIP_ERR_OK )
            {
                SendEquipError( res, NULL, NULL );
                return false;
            }
        }
    }

    if ( pQuest->GetRewItemsCount() > 0 )
    {
        for (uint32 i = 0; i < pQuest->GetRewItemsCount(); ++i)
        {
            if( pQuest->RewItemId[i] )
            {
                ItemPosCountVec dest;
                uint8 res = CanStoreNewItem( NULL_BAG, NULL_SLOT, dest, pQuest->RewItemId[i], pQuest->RewItemCount[i] );
                if( res != EQUIP_ERR_OK )
                {
                    SendEquipError( res, NULL, NULL );
                    return false;
                }
            }
        }
    }

    return true;
}

void Player::AddQuest( Quest const *pQuest, Object *questGiver )
{
    uint16 log_slot = FindQuestSlot( 0 );
    assert(log_slot < MAX_QUEST_LOG_SIZE);

    uint32 quest_id = pQuest->GetQuestId();

    // if not exist then created with set uState==NEW and rewarded=false
    QuestStatusData& questStatusData = mQuestStatus[quest_id];
    if (questStatusData.uState != QUEST_NEW)
        questStatusData.uState = QUEST_CHANGED;

    // check for repeatable quests status reset
    questStatusData.m_status = QUEST_STATUS_INCOMPLETE;
    questStatusData.m_explored = false;

    if ( pQuest->HasFlag( QUEST_TRINITY_FLAGS_DELIVER ) )
    {
        for(int i = 0; i < QUEST_OBJECTIVES_COUNT; i++)
            questStatusData.m_itemcount[i] = 0;
    }

    if ( pQuest->HasFlag(QUEST_TRINITY_FLAGS_KILL_OR_CAST | QUEST_TRINITY_FLAGS_SPEAKTO) )
    {
        for(int i = 0; i < QUEST_OBJECTIVES_COUNT; i++)
            questStatusData.m_creatureOrGOcount[i] = 0;
    }

    GiveQuestSourceItem( pQuest );
    AdjustQuestReqItemCount( pQuest );

    if( pQuest->GetRepObjectiveFaction() )
        SetFactionVisibleForFactionId(pQuest->GetRepObjectiveFaction());

    uint32 qtime = 0;
    if( pQuest->HasFlag( QUEST_TRINITY_FLAGS_TIMED ) )
    {
        uint32 limittime = pQuest->GetLimitTime();

        // shared timed quest
        if(questGiver && questGiver->GetTypeId()==TYPEID_PLAYER)
            limittime = (questGiver->ToPlayer())->getQuestStatusMap()[quest_id].m_timer / 1000;

        AddTimedQuest( quest_id );
        questStatusData.m_timer = limittime * 1000;
        qtime = static_cast<uint32>(time(NULL)) + limittime;
    }
    else
        questStatusData.m_timer = 0;

    SetQuestSlot(log_slot, quest_id, qtime);

    //starting initial quest script
    if(questGiver && pQuest->GetQuestStartScript()!=0)
        sWorld.ScriptsStart(sQuestStartScripts, pQuest->GetQuestStartScript(), questGiver, this);

    UpdateForQuestsGO();
}

void Player::CompleteQuest( uint32 quest_id )
{
    if( quest_id )
    {
        SetQuestStatus( quest_id, QUEST_STATUS_COMPLETE );

        uint16 log_slot = FindQuestSlot( quest_id );
        if( log_slot < MAX_QUEST_LOG_SIZE)
            SetQuestSlotState(log_slot,QUEST_STATE_COMPLETE);

        if(Quest const* qInfo = objmgr.GetQuestTemplate(quest_id))
        {
            if( qInfo->HasFlag(QUEST_FLAGS_AUTO_REWARDED) )
                RewardQuest(qInfo,0,this,false);
            else
                SendQuestComplete( quest_id );
        }
    }
}

void Player::IncompleteQuest( uint32 quest_id )
{
    if( quest_id )
    {
        SetQuestStatus( quest_id, QUEST_STATUS_INCOMPLETE );

        uint16 log_slot = FindQuestSlot( quest_id );
        if( log_slot < MAX_QUEST_LOG_SIZE)
            RemoveQuestSlotState(log_slot,QUEST_STATE_COMPLETE);
    }
}

void Player::RewardQuest( Quest const *pQuest, uint32 reward, Object* questGiver, bool announce )
{
    uint32 quest_id = pQuest->GetQuestId();

    for (int i = 0; i < QUEST_OBJECTIVES_COUNT; i++ )
    {
        if ( pQuest->ReqItemId[i] )
            DestroyItemCount( pQuest->ReqItemId[i], pQuest->ReqItemCount[i], true);
    }

    //if( qInfo->HasSpecialFlag( QUEST_FLAGS_TIMED ) )
    //    SetTimedQuest( 0 );
    m_timedquests.erase(pQuest->GetQuestId());

    if ( pQuest->GetRewChoiceItemsCount() > 0 )
    {
        if( pQuest->RewChoiceItemId[reward] )
        {
            ItemPosCountVec dest;
            if( CanStoreNewItem( NULL_BAG, NULL_SLOT, dest, pQuest->RewChoiceItemId[reward], pQuest->RewChoiceItemCount[reward] ) == EQUIP_ERR_OK )
            {
                Item* item = StoreNewItem( dest, pQuest->RewChoiceItemId[reward], true);
                SendNewItem(item, pQuest->RewChoiceItemCount[reward], true, false);
            }
        }
    }

    if ( pQuest->GetRewItemsCount() > 0 )
    {
        for (uint32 i=0; i < pQuest->GetRewItemsCount(); ++i)
        {
            if( pQuest->RewItemId[i] )
            {
                ItemPosCountVec dest;
                if( CanStoreNewItem( NULL_BAG, NULL_SLOT, dest, pQuest->RewItemId[i], pQuest->RewItemCount[i] ) == EQUIP_ERR_OK )
                {
                    Item* item = StoreNewItem( dest, pQuest->RewItemId[i], true);
                    SendNewItem(item, pQuest->RewItemCount[i], true, false);
                }
            }
        }
    }

    RewardReputation( pQuest );

    if( pQuest->GetRewSpellCast() > 0 )
        CastSpell( this, pQuest->GetRewSpellCast(), true);
    else if( pQuest->GetRewSpell() > 0)
        CastSpell( this, pQuest->GetRewSpell(), true);

    uint16 log_slot = FindQuestSlot( quest_id );
    if( log_slot < MAX_QUEST_LOG_SIZE)
        SetQuestSlot(log_slot,0);

    QuestStatusData& q_status = mQuestStatus[quest_id];

    // Not give XP in case already completed once repeatable quest
    uint32 XP = 0;
    if (hasCustomXpRate())
        XP = q_status.m_rewarded ? 0 : uint32(pQuest->XPValue( this )*m_customXp);
    else
        XP = q_status.m_rewarded ? 0 : uint32(pQuest->XPValue( this )*sWorld.getRate(RATE_XP_QUEST));

    if ( getLevel() < sWorld.getConfig(CONFIG_MAX_PLAYER_LEVEL) )
        GiveXP( XP , NULL );
    else
        ModifyMoney( int32(pQuest->GetRewMoneyMaxLevel() * sWorld.getRate(RATE_DROP_MONEY)) );

    // Give player extra money if GetRewOrReqMoney > 0 and get ReqMoney if negative
    ModifyMoney( pQuest->GetRewOrReqMoney() );

    // honor reward
    if(pQuest->GetRewHonorableKills())
        RewardHonor(NULL, 0, Trinity::Honor::hk_honor_at_level(getLevel(), pQuest->GetRewHonorableKills()));

    // title reward
    if(pQuest->GetCharTitleId())
    {
        if(CharTitlesEntry const* titleEntry = sCharTitlesStore.LookupEntry(pQuest->GetCharTitleId()))
            SetTitle(titleEntry);
    }

    // Send reward mail
    if(pQuest->GetRewMailTemplateId())
    {
        MailMessageType mailType;
        uint32 senderGuidOrEntry;
        switch(questGiver->GetTypeId())
        {
            case TYPEID_UNIT:
                mailType = MAIL_CREATURE;
                senderGuidOrEntry = questGiver->GetEntry();
                break;
            case TYPEID_GAMEOBJECT:
                mailType = MAIL_GAMEOBJECT;
                senderGuidOrEntry = questGiver->GetEntry();
                break;
            case TYPEID_ITEM:
                mailType = MAIL_ITEM;
                senderGuidOrEntry = questGiver->GetEntry();
                break;
            case TYPEID_PLAYER:
                mailType = MAIL_NORMAL;
                senderGuidOrEntry = questGiver->GetGUIDLow();
                break;
            default:
                mailType = MAIL_NORMAL;
                senderGuidOrEntry = GetGUIDLow();
                break;
        }

        Loot questMailLoot;

        questMailLoot.FillLoot(pQuest->GetQuestId(), LootTemplates_QuestMail, this);

        // fill mail
        MailItemsInfo mi;                                   // item list preparing

        SQLTransaction trans = CharacterDatabase.BeginTransaction();
        for(size_t i = 0; mi.size() < MAX_MAIL_ITEMS && i < questMailLoot.items.size(); ++i)
        {
            if(LootItem* lootitem = questMailLoot.LootItemInSlot(i,this))
            {
                if(Item* item = Item::CreateItem(lootitem->itemid,lootitem->count,this))
                {
                    item->SaveToDB(trans);                       // save for prevent lost at next mail load, if send fail then item will deleted
                    mi.AddItem(item->GetGUIDLow(), item->GetEntry(), item);
                }
            }
        }

        for(size_t i = 0; mi.size() < MAX_MAIL_ITEMS && i < questMailLoot.quest_items.size(); ++i)
        {
            if(LootItem* lootitem = questMailLoot.LootItemInSlot(i+questMailLoot.items.size(),this))
            {
                if(Item* item = Item::CreateItem(lootitem->itemid,lootitem->count,this))
                {
                    item->SaveToDB(trans);                       // save for prevent lost at next mail load, if send fail then item will deleted
                    mi.AddItem(item->GetGUIDLow(), item->GetEntry(), item);
                }
            }
        }
        CharacterDatabase.CommitTransaction(trans);

        WorldSession::SendMailTo(this, mailType, MAIL_STATIONERY_NORMAL, senderGuidOrEntry, GetGUIDLow(), "", 0, &mi, 0, 0, MAIL_CHECK_MASK_NONE,pQuest->GetRewMailDelaySecs(),pQuest->GetRewMailTemplateId());
    }

    if(pQuest->IsDaily())
        SetDailyQuestStatus(quest_id);

    if ( !pQuest->IsRepeatable() )
        SetQuestStatus(quest_id, QUEST_STATUS_COMPLETE);
    else
        SetQuestStatus(quest_id, QUEST_STATUS_NONE);

    q_status.m_rewarded = true;

    if(announce)
        SendQuestReward( pQuest, XP, questGiver );

    if (q_status.uState != QUEST_NEW) q_status.uState = QUEST_CHANGED;
}

void Player::FailQuest( uint32 quest_id )
{
    if( quest_id )
    {
        IncompleteQuest( quest_id );

        uint16 log_slot = FindQuestSlot( quest_id );
        if( log_slot < MAX_QUEST_LOG_SIZE)
        {
            SetQuestSlotTimer(log_slot, 1 );
            SetQuestSlotState(log_slot,QUEST_STATE_FAIL);
        }
        SendQuestFailed( quest_id );
    }
}

void Player::FailTimedQuest( uint32 quest_id )
{
    if( quest_id )
    {
        QuestStatusData& q_status = mQuestStatus[quest_id];

        if (q_status.uState != QUEST_NEW) q_status.uState = QUEST_CHANGED;
        q_status.m_timer = 0;

        IncompleteQuest( quest_id );

        uint16 log_slot = FindQuestSlot( quest_id );
        if( log_slot < MAX_QUEST_LOG_SIZE)
        {
            SetQuestSlotTimer(log_slot, 1 );
            SetQuestSlotState(log_slot,QUEST_STATE_FAIL);
        }
        SendQuestTimerFailed( quest_id );
    }
}

bool Player::SatisfyQuestSkillOrClass( Quest const* qInfo, bool msg )
{
    int32 zoneOrSort   = qInfo->GetZoneOrSort();
    int32 skillOrClass = qInfo->GetSkillOrClass();

    // skip zone zoneOrSort and 0 case skillOrClass
    if( zoneOrSort >= 0 && skillOrClass == 0 )
        return true;

    int32 questSort = -zoneOrSort;
    uint8 reqSortClass = ClassByQuestSort(questSort);

    // check class sort cases in zoneOrSort
    if( reqSortClass != 0 && getClass() != reqSortClass)
    {
        if( msg )
            SendCanTakeQuestResponse( INVALIDREASON_DONT_HAVE_REQ );
        return false;
    }

    // check class
    if( skillOrClass < 0 )
    {
        uint8 reqClass = -int32(skillOrClass);
        if(getClass() != reqClass)
        {
            if( msg )
                SendCanTakeQuestResponse( INVALIDREASON_DONT_HAVE_REQ );
            return false;
        }
    }
    // check skill
    else if( skillOrClass > 0 )
    {
        uint32 reqSkill = skillOrClass;
        if( GetSkillValue( reqSkill ) < qInfo->GetRequiredSkillValue() )
        {
            if( msg )
                SendCanTakeQuestResponse( INVALIDREASON_DONT_HAVE_REQ );
            return false;
        }
    }

    return true;
}

bool Player::SatisfyQuestLevel( Quest const* qInfo, bool msg )
{
    if( getLevel() < qInfo->GetMinLevel() )
    {
        if( msg )
            SendCanTakeQuestResponse( INVALIDREASON_DONT_HAVE_REQ );
        return false;
    }
    return true;
}

bool Player::SatisfyQuestLog( bool msg )
{
    // exist free slot
    if( FindQuestSlot(0) < MAX_QUEST_LOG_SIZE )
        return true;

    if( msg )
    {
        WorldPacket data( SMSG_QUESTLOG_FULL, 0 );
        GetSession()->SendPacket( &data );
    }
    return false;
}

bool Player::SatisfyQuestPreviousQuest( Quest const* qInfo, bool msg )
{
    // No previous quest (might be first quest in a series)
    if( qInfo->prevQuests.empty())
        return true;

    for(Quest::PrevQuests::const_iterator iter = qInfo->prevQuests.begin(); iter != qInfo->prevQuests.end(); ++iter )
    {
        uint32 prevId = abs(*iter);

        QuestStatusMap::iterator i_prevstatus = mQuestStatus.find( prevId );
        Quest const* qPrevInfo = objmgr.GetQuestTemplate(prevId);

        if( qPrevInfo && i_prevstatus != mQuestStatus.end() )
        {
            // If any of the positive previous quests completed, return true
            if( *iter > 0 && i_prevstatus->second.m_rewarded )
            {
                // skip one-from-all exclusive group
                if(qPrevInfo->GetExclusiveGroup() >= 0)
                    return true;

                // each-from-all exclusive group ( < 0)
                // can be start if only all quests in prev quest exclusive group completed and rewarded
                ObjectMgr::ExclusiveQuestGroups::iterator iter = objmgr.mExclusiveQuestGroups.lower_bound(qPrevInfo->GetExclusiveGroup());
                ObjectMgr::ExclusiveQuestGroups::iterator end  = objmgr.mExclusiveQuestGroups.upper_bound(qPrevInfo->GetExclusiveGroup());

                assert(iter!=end);                          // always must be found if qPrevInfo->ExclusiveGroup != 0

                for(; iter != end; ++iter)
                {
                    uint32 exclude_Id = iter->second;

                    // skip checked quest id, only state of other quests in group is interesting
                    if(exclude_Id == prevId)
                        continue;

                    QuestStatusMap::iterator i_exstatus = mQuestStatus.find( exclude_Id );

                    // alternative quest from group also must be completed and rewarded(reported)
                    if( i_exstatus == mQuestStatus.end() || !i_exstatus->second.m_rewarded )
                    {
                        if( msg )
                            SendCanTakeQuestResponse( INVALIDREASON_DONT_HAVE_REQ );
                        return false;
                    }
                }
                return true;
            }
            // If any of the negative previous quests active, return true
            if( *iter < 0 && (i_prevstatus->second.m_status == QUEST_STATUS_INCOMPLETE
                || (i_prevstatus->second.m_status == QUEST_STATUS_COMPLETE && !GetQuestRewardStatus(prevId))))
            {
                // skip one-from-all exclusive group
                if(qPrevInfo->GetExclusiveGroup() >= 0)
                    return true;

                // each-from-all exclusive group ( < 0)
                // can be start if only all quests in prev quest exclusive group active
                ObjectMgr::ExclusiveQuestGroups::iterator iter = objmgr.mExclusiveQuestGroups.lower_bound(qPrevInfo->GetExclusiveGroup());
                ObjectMgr::ExclusiveQuestGroups::iterator end  = objmgr.mExclusiveQuestGroups.upper_bound(qPrevInfo->GetExclusiveGroup());

                assert(iter!=end);                          // always must be found if qPrevInfo->ExclusiveGroup != 0

                for(; iter != end; ++iter)
                {
                    uint32 exclude_Id = iter->second;

                    // skip checked quest id, only state of other quests in group is interesting
                    if(exclude_Id == prevId)
                        continue;

                    QuestStatusMap::iterator i_exstatus = mQuestStatus.find( exclude_Id );

                    // alternative quest from group also must be active
                    if( i_exstatus == mQuestStatus.end() ||
                        i_exstatus->second.m_status != QUEST_STATUS_INCOMPLETE &&
                        (i_prevstatus->second.m_status != QUEST_STATUS_COMPLETE || GetQuestRewardStatus(prevId)) )
                    {
                        if( msg )
                            SendCanTakeQuestResponse( INVALIDREASON_DONT_HAVE_REQ );
                        return false;
                    }
                }
                return true;
            }
        }
    }

    // Has only positive prev. quests in non-rewarded state
    // and negative prev. quests in non-active state
    if( msg )
        SendCanTakeQuestResponse( INVALIDREASON_DONT_HAVE_REQ );

    return false;
}

bool Player::SatisfyQuestRace( Quest const* qInfo, bool msg )
{
    uint32 reqraces = qInfo->GetRequiredRaces();
    if ( reqraces == 0 )
        return true;
    if( (reqraces & getRaceMask()) == 0 )
    {
        if( msg )
            SendCanTakeQuestResponse( INVALIDREASON_QUEST_FAILED_WRONG_RACE );
        return false;
    }
    return true;
}

bool Player::SatisfyQuestReputation( Quest const* qInfo, bool msg )
{
    uint32 fIdMin = qInfo->GetRequiredMinRepFaction();      //Min required rep
    if(fIdMin && GetReputation(fIdMin) < qInfo->GetRequiredMinRepValue())
    {
        if( msg )
            SendCanTakeQuestResponse( INVALIDREASON_DONT_HAVE_REQ );
        return false;
    }

    uint32 fIdMax = qInfo->GetRequiredMaxRepFaction();      //Max required rep
    if(fIdMax && GetReputation(fIdMax) >= qInfo->GetRequiredMaxRepValue())
    {
        if( msg )
            SendCanTakeQuestResponse( INVALIDREASON_DONT_HAVE_REQ );
        return false;
    }

    return true;
}

bool Player::SatisfyQuestStatus( Quest const* qInfo, bool msg )
{
    QuestStatusMap::iterator itr = mQuestStatus.find( qInfo->GetQuestId() );
    if  ( itr != mQuestStatus.end() && itr->second.m_status != QUEST_STATUS_NONE )
    {
        if( msg )
            SendCanTakeQuestResponse( INVALIDREASON_QUEST_ALREADY_ON );
        return false;
    }
    return true;
}

bool Player::SatisfyQuestTimed( Quest const* qInfo, bool msg )
{
    if ( (find(m_timedquests.begin(), m_timedquests.end(), qInfo->GetQuestId()) != m_timedquests.end()) && qInfo->HasFlag(QUEST_TRINITY_FLAGS_TIMED) )
    {
        if( msg )
            SendCanTakeQuestResponse( INVALIDREASON_QUEST_ONLY_ONE_TIMED );
        return false;
    }
    return true;
}

bool Player::SatisfyQuestExclusiveGroup( Quest const* qInfo, bool msg )
{
    // non positive exclusive group, if > 0 then can be start if any other quest in exclusive group already started/completed
    if(qInfo->GetExclusiveGroup() <= 0)
        return true;

    ObjectMgr::ExclusiveQuestGroups::iterator iter = objmgr.mExclusiveQuestGroups.lower_bound(qInfo->GetExclusiveGroup());
    ObjectMgr::ExclusiveQuestGroups::iterator end  = objmgr.mExclusiveQuestGroups.upper_bound(qInfo->GetExclusiveGroup());

    assert(iter!=end);                                      // always must be found if qInfo->ExclusiveGroup != 0

    for(; iter != end; ++iter)
    {
        uint32 exclude_Id = iter->second;

        // skip checked quest id, only state of other quests in group is interesting
        if(exclude_Id == qInfo->GetQuestId())
            continue;

        // not allow have daily quest if daily quest from exclusive group already recently completed
        Quest const* Nquest = objmgr.GetQuestTemplate(exclude_Id);
        if( !SatisfyQuestDay(Nquest, false) )
        {
            if( msg )
                SendCanTakeQuestResponse( INVALIDREASON_DONT_HAVE_REQ );
            return false;
        }

        QuestStatusMap::iterator i_exstatus = mQuestStatus.find( exclude_Id );

        // alternative quest already started or completed
        if( i_exstatus != mQuestStatus.end()
            && (i_exstatus->second.m_status == QUEST_STATUS_COMPLETE || i_exstatus->second.m_status == QUEST_STATUS_INCOMPLETE) )
        {
            if( msg )
                SendCanTakeQuestResponse( INVALIDREASON_DONT_HAVE_REQ );
            return false;
        }
    }
    return true;
}

bool Player::SatisfyQuestNextChain( Quest const* qInfo, bool msg )
{
    if(!qInfo->GetNextQuestInChain())
        return true;

    // next quest in chain already started or completed
    QuestStatusMap::iterator itr = mQuestStatus.find( qInfo->GetNextQuestInChain() );
    if( itr != mQuestStatus.end()
        && (itr->second.m_status == QUEST_STATUS_COMPLETE || itr->second.m_status == QUEST_STATUS_INCOMPLETE) )
    {
        if( msg )
            SendCanTakeQuestResponse( INVALIDREASON_DONT_HAVE_REQ );
        return false;
    }

    // check for all quests further up the chain
    // only necessary if there are quest chains with more than one quest that can be skipped
    //return SatisfyQuestNextChain( qInfo->GetNextQuestInChain(), msg );
    return true;
}

bool Player::SatisfyQuestPrevChain( Quest const* qInfo, bool msg )
{
    // No previous quest in chain
    if( qInfo->prevChainQuests.empty())
        return true;

    for(Quest::PrevChainQuests::const_iterator iter = qInfo->prevChainQuests.begin(); iter != qInfo->prevChainQuests.end(); ++iter )
    {
        uint32 prevId = *iter;

        QuestStatusMap::iterator i_prevstatus = mQuestStatus.find( prevId );

        if( i_prevstatus != mQuestStatus.end() )
        {
            // If any of the previous quests in chain active, return false
            if( i_prevstatus->second.m_status == QUEST_STATUS_INCOMPLETE
                || (i_prevstatus->second.m_status == QUEST_STATUS_COMPLETE && !GetQuestRewardStatus(prevId)))
            {
                if( msg )
                    SendCanTakeQuestResponse( INVALIDREASON_DONT_HAVE_REQ );
                return false;
            }
        }

        // check for all quests further down the chain
        // only necessary if there are quest chains with more than one quest that can be skipped
        //if( !SatisfyQuestPrevChain( prevId, msg ) )
        //    return false;
    }

    // No previous quest in chain active
    return true;
}

bool Player::SatisfyQuestDay( Quest const* qInfo, bool msg )
{
    if(!qInfo->IsDaily())
        return true;

    bool have_slot = false;
    for(uint32 quest_daily_idx = 0; quest_daily_idx < PLAYER_MAX_DAILY_QUESTS; ++quest_daily_idx)
    {
        uint32 id = GetUInt32Value(PLAYER_FIELD_DAILY_QUESTS_1+quest_daily_idx);
        if(qInfo->GetQuestId()==id)
            return false;

        if(!id)
            have_slot = true;
    }

    if(!have_slot)
    {
        if( msg )
            SendCanTakeQuestResponse( INVALIDREASON_DAILY_QUESTS_REMAINING );
        return false;
    }

    return true;
}

bool Player::GiveQuestSourceItem( Quest const *pQuest )
{
    uint32 srcitem = pQuest->GetSrcItemId();
    if( srcitem > 0 )
    {
        uint32 count = pQuest->GetSrcItemCount();
        if( count <= 0 )
            count = 1;

        ItemPosCountVec dest;
        uint8 msg = CanStoreNewItem( NULL_BAG, NULL_SLOT, dest, srcitem, count );
        if( msg == EQUIP_ERR_OK )
        {
            Item * item = StoreNewItem(dest, srcitem, true);
            SendNewItem(item, count, true, false);
            return true;
        }
        // player already have max amount required item, just report success
        else if( msg == EQUIP_ERR_CANT_CARRY_MORE_OF_THIS )
            return true;
        else
            SendEquipError( msg, NULL, NULL );
        return false;
    }

    return true;
}

bool Player::TakeQuestSourceItem( uint32 quest_id, bool msg )
{
    Quest const* qInfo = objmgr.GetQuestTemplate(quest_id);
    if( qInfo )
    {
        uint32 srcitem = qInfo->GetSrcItemId();
        if( srcitem > 0 )
        {
            uint32 count = qInfo->GetSrcItemCount();
            if( count <= 0 )
                count = 1;

            // exist one case when destroy source quest item not possible:
            // non un-equippable item (equipped non-empty bag, for example)
            uint8 res = CanUnequipItems(srcitem,count);
            if(res != EQUIP_ERR_OK)
            {
                if(msg)
                    SendEquipError( res, NULL, NULL );
                return false;
            }

            DestroyItemCount(srcitem, count, true, true, true);
        }
    }
    return true;
}

bool Player::GetQuestRewardStatus( uint32 quest_id ) const
{
    Quest const* qInfo = objmgr.GetQuestTemplate(quest_id);
    if( qInfo )
    {
        // for repeatable quests: rewarded field is set after first reward only to prevent getting XP more than once
        QuestStatusMap::const_iterator itr = mQuestStatus.find( quest_id );
        if( itr != mQuestStatus.end() && itr->second.m_status != QUEST_STATUS_NONE
            && !qInfo->IsRepeatable() )
            return itr->second.m_rewarded;

        return false;
    }
    return false;
}

QuestStatus Player::GetQuestStatus( uint32 quest_id ) const
{
    if( quest_id )
    {
        QuestStatusMap::const_iterator itr = mQuestStatus.find( quest_id );
        if( itr != mQuestStatus.end() )
            return itr->second.m_status;
    }
    return QUEST_STATUS_NONE;
}

bool Player::CanShareQuest(uint32 quest_id) const
{
    Quest const* qInfo = objmgr.GetQuestTemplate(quest_id);
    if( qInfo && qInfo->HasFlag(QUEST_FLAGS_SHARABLE) )
    {
        QuestStatusMap::const_iterator itr = mQuestStatus.find( quest_id );
        if( itr != mQuestStatus.end() )
            return itr->second.m_status == QUEST_STATUS_NONE || itr->second.m_status == QUEST_STATUS_INCOMPLETE;
    }
    return false;
}

void Player::SetQuestStatus( uint32 quest_id, QuestStatus status )
{
    Quest const* qInfo = objmgr.GetQuestTemplate(quest_id);
    if( qInfo )
    {
        if( status == QUEST_STATUS_NONE || status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_COMPLETE )
        {
            if( qInfo->HasFlag( QUEST_TRINITY_FLAGS_TIMED ) )
                m_timedquests.erase(qInfo->GetQuestId());
        }

        QuestStatusData& q_status = mQuestStatus[quest_id];

        q_status.m_status = status;
        if (q_status.uState != QUEST_NEW) q_status.uState = QUEST_CHANGED;
    }

    UpdateForQuestsGO();
}

// not used in TrinIty, but used in scripting code
uint32 Player::GetReqKillOrCastCurrentCount(uint32 quest_id, int32 entry)
{
    Quest const* qInfo = objmgr.GetQuestTemplate(quest_id);
    if( !qInfo )
        return 0;

    for (int j = 0; j < QUEST_OBJECTIVES_COUNT; j++)
        if ( qInfo->ReqCreatureOrGOId[j] == entry )
            return mQuestStatus[quest_id].m_creatureOrGOcount[j];

    return 0;
}

void Player::AdjustQuestReqItemCount( Quest const* pQuest )
{
    if ( pQuest->HasFlag( QUEST_TRINITY_FLAGS_DELIVER ) )
    {
        for(int i = 0; i < QUEST_OBJECTIVES_COUNT; i++)
        {
            uint32 reqitemcount = pQuest->ReqItemCount[i];
            if( reqitemcount != 0 )
            {
                uint32 quest_id = pQuest->GetQuestId();
                uint32 curitemcount = GetItemCount(pQuest->ReqItemId[i],true);

                QuestStatusData& q_status = mQuestStatus[quest_id];
                q_status.m_itemcount[i] = std::min(curitemcount, reqitemcount);
                if (q_status.uState != QUEST_NEW) q_status.uState = QUEST_CHANGED;
            }
        }
    }
}

uint16 Player::FindQuestSlot( uint32 quest_id ) const
{
    for ( uint16 i = 0; i < MAX_QUEST_LOG_SIZE; i++ )
        if ( GetQuestSlotQuestId(i) == quest_id )
            return i;

    return MAX_QUEST_LOG_SIZE;
}

void Player::AreaExploredOrEventHappens( uint32 questId )
{
    if( questId )
    {
        uint16 log_slot = FindQuestSlot( questId );
        if( log_slot < MAX_QUEST_LOG_SIZE)
        {
            QuestStatusData& q_status = mQuestStatus[questId];

            if(!q_status.m_explored)
            {
                q_status.m_explored = true;
                if (q_status.uState != QUEST_NEW)
                    q_status.uState = QUEST_CHANGED;
            }
        }
        if( CanCompleteQuest( questId ) )
            CompleteQuest( questId );
    }
}

//not used in Trinityd, function for external script library
void Player::GroupEventHappens( uint32 questId, WorldObject const* pEventObject )
{
    if( Group *pGroup = GetGroup() )
    {
        for(GroupReference *itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            Player *pGroupGuy = itr->getSource();

            // for any leave or dead (with not released body) group member at appropriate distance
            if( pGroupGuy && pGroupGuy->IsAtGroupRewardDistance(pEventObject) && !pGroupGuy->GetCorpse() )
                pGroupGuy->AreaExploredOrEventHappens(questId);
        }
    }
    else
        AreaExploredOrEventHappens(questId);
}

void Player::ItemAddedQuestCheck( uint32 entry, uint32 count )
{
    for( int i = 0; i < MAX_QUEST_LOG_SIZE; i++ )
    {
        uint32 questid = GetQuestSlotQuestId(i);
        if ( questid == 0 )
            continue;

        QuestStatusData& q_status = mQuestStatus[questid];

        if ( q_status.m_status != QUEST_STATUS_INCOMPLETE )
            continue;

        Quest const* qInfo = objmgr.GetQuestTemplate(questid);
        if( !qInfo || !qInfo->HasFlag( QUEST_TRINITY_FLAGS_DELIVER ) )
            continue;

        for (int j = 0; j < QUEST_OBJECTIVES_COUNT; j++)
        {
            uint32 reqitem = qInfo->ReqItemId[j];
            if ( reqitem == entry )
            {
                uint32 reqitemcount = qInfo->ReqItemCount[j];
                uint32 curitemcount = q_status.m_itemcount[j];
                if ( curitemcount < reqitemcount )
                {
                    uint32 additemcount = ( curitemcount + count <= reqitemcount ? count : reqitemcount - curitemcount);
                    q_status.m_itemcount[j] += additemcount;
                    if (q_status.uState != QUEST_NEW) q_status.uState = QUEST_CHANGED;

                    SendQuestUpdateAddItem( qInfo, j, additemcount );
                }
                if ( CanCompleteQuest( questid ) )
                    CompleteQuest( questid );
                return;
            }
        }
    }
    UpdateForQuestsGO();
}

void Player::ItemRemovedQuestCheck( uint32 entry, uint32 count )
{
    for( int i = 0; i < MAX_QUEST_LOG_SIZE; i++ )
    {
        uint32 questid = GetQuestSlotQuestId(i);
        if(!questid)
            continue;
        Quest const* qInfo = objmgr.GetQuestTemplate(questid);
        if ( !qInfo )
            continue;
        if( !qInfo->HasFlag( QUEST_TRINITY_FLAGS_DELIVER ) )
            continue;

        for (int j = 0; j < QUEST_OBJECTIVES_COUNT; j++)
        {
            uint32 reqitem = qInfo->ReqItemId[j];
            if ( reqitem == entry )
            {
                QuestStatusData& q_status = mQuestStatus[questid];

                uint32 reqitemcount = qInfo->ReqItemCount[j];
                uint32 curitemcount;
                if( q_status.m_status != QUEST_STATUS_COMPLETE )
                    curitemcount = q_status.m_itemcount[j];
                else
                    curitemcount = GetItemCount(entry,true);
                if ( curitemcount < reqitemcount + count )
                {
                    uint32 remitemcount = ( curitemcount <= reqitemcount ? count : count + reqitemcount - curitemcount);
                    q_status.m_itemcount[j] = curitemcount - remitemcount;
                    if (q_status.uState != QUEST_NEW) q_status.uState = QUEST_CHANGED;

                    IncompleteQuest( questid );
                }
                return;
            }
        }
    }
    UpdateForQuestsGO();
}

void Player::KilledMonster(uint32 entry, uint64 guid, uint32 questId)
{
    uint32 addkillcount = 1;
    for( int i = 0; i < MAX_QUEST_LOG_SIZE; i++ )
    {
        uint32 questid = GetQuestSlotQuestId(i);
        if(!questid)
            continue;
            
        if (questId && questid != questId)
            continue;

        Quest const* qInfo = objmgr.GetQuestTemplate(questid);
        if( !qInfo )
            continue;
        // just if !ingroup || !noraidgroup || raidgroup
        QuestStatusData& q_status = mQuestStatus[questid];
        if( q_status.m_status == QUEST_STATUS_INCOMPLETE && (!GetGroup() || !GetGroup()->isRaidGroup() || qInfo->GetType() == QUEST_TYPE_RAID))
        {
            if( qInfo->HasFlag( QUEST_TRINITY_FLAGS_KILL_OR_CAST) )
            {
                for (int j = 0; j < QUEST_OBJECTIVES_COUNT; j++)
                {
                    // skip GO activate objective or none
                    if(qInfo->ReqCreatureOrGOId[j] <=0)
                        continue;

                    // skip Cast at creature objective
                    if(qInfo->ReqSpell[j] !=0 )
                        continue;

                    uint32 reqkill = qInfo->ReqCreatureOrGOId[j];

                    if ( reqkill == entry )
                    {
                        uint32 reqkillcount = qInfo->ReqCreatureOrGOCount[j];
                        uint32 curkillcount = q_status.m_creatureOrGOcount[j];
                        if ( curkillcount < reqkillcount )
                        {
                            q_status.m_creatureOrGOcount[j] = curkillcount + addkillcount;
                            if (q_status.uState != QUEST_NEW) q_status.uState = QUEST_CHANGED;

                            SendQuestUpdateAddCreatureOrGo( qInfo, guid, j, curkillcount, addkillcount);
                        }
                        if ( CanCompleteQuest( questid ) )
                            CompleteQuest( questid );

                        // same objective target can be in many active quests, but not in 2 objectives for single quest (code optimization).
                        continue;
                    }
                }
            }
        }
    }
}

void Player::ActivatedGO(uint32 entry, uint64 guid)
{
    uint32 addkillcount = 1;
    for( int i = 0; i < MAX_QUEST_LOG_SIZE; i++ )
    {
        uint32 questid = GetQuestSlotQuestId(i);
        if(!questid)
            continue;

        Quest const* qInfo = objmgr.GetQuestTemplate(questid);
        if( !qInfo )
            continue;
        // just if !ingroup || !noraidgroup || raidgroup
        QuestStatusData& q_status = mQuestStatus[questid];
        if( q_status.m_status == QUEST_STATUS_INCOMPLETE && (!GetGroup() || !GetGroup()->isRaidGroup() || qInfo->GetType() == QUEST_TYPE_RAID))
        {
            if( qInfo->HasFlag( QUEST_TRINITY_FLAGS_KILL_OR_CAST) )
            {
                for (int j = 0; j < QUEST_OBJECTIVES_COUNT; j++)
                {
                    // skip GO activate objective or none
                    if(qInfo->ReqCreatureOrGOId[j] >= 0)
                        continue;

                    // skip Cast at creature objective
                    if(qInfo->ReqSpell[j] !=0 )
                        continue;

                    int32 reqkill = qInfo->ReqCreatureOrGOId[j];

                    if ( -reqkill == entry )
                    {
                        uint32 reqkillcount = qInfo->ReqCreatureOrGOCount[j];
                        uint32 curkillcount = q_status.m_creatureOrGOcount[j];
                        if ( curkillcount < reqkillcount )
                        {
                            q_status.m_creatureOrGOcount[j] = curkillcount + addkillcount;
                            if (q_status.uState != QUEST_NEW) q_status.uState = QUEST_CHANGED;

                            SendQuestUpdateAddCreatureOrGo( qInfo, guid, j, curkillcount, addkillcount);
                        }
                        if ( CanCompleteQuest( questid ) )
                            CompleteQuest( questid );

                        // same objective target can be in many active quests, but not in 2 objectives for single quest (code optimization).
                        continue;
                    }
                }
            }
        }
    }
}

void Player::CastedCreatureOrGO( uint32 entry, uint64 guid, uint32 spell_id )
{
    bool isCreature = IS_CREATURE_GUID(guid);
    if (!guid)
        isCreature = true;

    uint32 addCastCount = 1;
    for( int i = 0; i < MAX_QUEST_LOG_SIZE; i++ )
    {
        uint32 questid = GetQuestSlotQuestId(i);
        if(!questid)
            continue;

        Quest const* qInfo = objmgr.GetQuestTemplate(questid);
        if ( !qInfo  )
            continue;

        QuestStatusData& q_status = mQuestStatus[questid];

        if ( q_status.m_status == QUEST_STATUS_INCOMPLETE )
        {
            if( qInfo->HasFlag( QUEST_TRINITY_FLAGS_KILL_OR_CAST ) )
            {
                for (int j = 0; j < QUEST_OBJECTIVES_COUNT; j++)
                {
                    // skip kill creature objective (0) or wrong spell casts
                    if(qInfo->ReqSpell[j] != spell_id )
                        continue;

                    uint32 reqTarget = 0;

                    if(isCreature)
                    {
                        // creature activate objectives
                        if(qInfo->ReqCreatureOrGOId[j] > 0)
                            // checked at quest_template loading
                            reqTarget = qInfo->ReqCreatureOrGOId[j];
                    }
                    else
                    {
                        // GO activate objective
                        if(qInfo->ReqCreatureOrGOId[j] < 0)
                            // checked at quest_template loading
                            reqTarget = - qInfo->ReqCreatureOrGOId[j];
                    }

                    // other not this creature/GO related objectives
                    if( reqTarget != entry )
                        continue;

                    uint32 reqCastCount = qInfo->ReqCreatureOrGOCount[j];
                    uint32 curCastCount = q_status.m_creatureOrGOcount[j];
                    if ( curCastCount < reqCastCount )
                    {
                        q_status.m_creatureOrGOcount[j] = curCastCount + addCastCount;
                        if (q_status.uState != QUEST_NEW) q_status.uState = QUEST_CHANGED;

                        SendQuestUpdateAddCreatureOrGo( qInfo, guid, j, curCastCount, addCastCount);
                    }

                    if ( CanCompleteQuest( questid ) )
                        CompleteQuest( questid );

                    // same objective target can be in many active quests, but not in 2 objectives for single quest (code optimization).
                    break;
                }
            }
        }
    }
}

void Player::TalkedToCreature( uint32 entry, uint64 guid )
{
    uint32 addTalkCount = 1;
    for( int i = 0; i < MAX_QUEST_LOG_SIZE; i++ )
    {
        uint32 questid = GetQuestSlotQuestId(i);
        if(!questid)
            continue;

        Quest const* qInfo = objmgr.GetQuestTemplate(questid);
        if ( !qInfo )
            continue;

        QuestStatusData& q_status = mQuestStatus[questid];

        if ( q_status.m_status == QUEST_STATUS_INCOMPLETE )
        {
            if( qInfo->HasFlag( QUEST_TRINITY_FLAGS_KILL_OR_CAST | QUEST_TRINITY_FLAGS_SPEAKTO ) )
            {
                for (int j = 0; j < QUEST_OBJECTIVES_COUNT; j++)
                {
                                                            // skip spell casts and Gameobject objectives
                    if(qInfo->ReqSpell[j] > 0 || qInfo->ReqCreatureOrGOId[j] < 0)
                        continue;

                    uint32 reqTarget = 0;

                    if(qInfo->ReqCreatureOrGOId[j] > 0)     // creature activate objectives
                                                            // checked at quest_template loading
                        reqTarget = qInfo->ReqCreatureOrGOId[j];
                    else
                        continue;

                    if ( reqTarget == entry )
                    {
                        uint32 reqTalkCount = qInfo->ReqCreatureOrGOCount[j];
                        uint32 curTalkCount = q_status.m_creatureOrGOcount[j];
                        if ( curTalkCount < reqTalkCount )
                        {
                            q_status.m_creatureOrGOcount[j] = curTalkCount + addTalkCount;
                            if (q_status.uState != QUEST_NEW) q_status.uState = QUEST_CHANGED;

                            SendQuestUpdateAddCreatureOrGo( qInfo, guid, j, curTalkCount, addTalkCount);
                        }
                        if ( CanCompleteQuest( questid ) )
                            CompleteQuest( questid );

                        // same objective target can be in many active quests, but not in 2 objectives for single quest (code optimization).
                        continue;
                    }
                }
            }
        }
    }
}

void Player::MoneyChanged( uint32 count )
{
    for( int i = 0; i < MAX_QUEST_LOG_SIZE; i++ )
    {
        uint32 questid = GetQuestSlotQuestId(i);
        if (!questid)
            continue;

        Quest const* qInfo = objmgr.GetQuestTemplate(questid);
        if( qInfo && qInfo->GetRewOrReqMoney() < 0 )
        {
            QuestStatusData& q_status = mQuestStatus[questid];

            if( q_status.m_status == QUEST_STATUS_INCOMPLETE )
            {
                if(int32(count) >= -qInfo->GetRewOrReqMoney())
                {
                    if ( CanCompleteQuest( questid ) )
                        CompleteQuest( questid );
                }
            }
            else if( q_status.m_status == QUEST_STATUS_COMPLETE )
            {
                if(int32(count) < -qInfo->GetRewOrReqMoney())
                    IncompleteQuest( questid );
            }
        }
    }
}

bool Player::HasQuestForItem( uint32 itemid ) const
{
    // Workaround for quests 7810/7838
    if (itemid == 18706)
        return true;
    for( QuestStatusMap::const_iterator i = mQuestStatus.begin( ); i != mQuestStatus.end( ); ++i )
    {
        QuestStatusData const& q_status = i->second;

        if (q_status.m_status == QUEST_STATUS_INCOMPLETE)
        {
            Quest const* qinfo = objmgr.GetQuestTemplate(i->first);
            if(!qinfo)
                continue;

            // hide quest if player is in raid-group and quest is no raid quest
            if(GetGroup() && GetGroup()->isRaidGroup() && qinfo->GetType() != QUEST_TYPE_RAID)
                if(!InBattleGround()) //there are two ways.. we can make every bg-quest a raidquest, or add this code here.. i don't know if this can be exploited by other quests, but i think all other quests depend on a specific area.. but keep this in mind, if something strange happens later
                    continue;

            // There should be no mixed ReqItem/ReqSource drop
            // This part for ReqItem drop
            for (int j = 0; j < QUEST_OBJECTIVES_COUNT; j++)
            {
                if(itemid == qinfo->ReqItemId[j] && q_status.m_itemcount[j] < qinfo->ReqItemCount[j] )
                    return true;
            }
            // This part - for ReqSource
            for (int j = 0; j < QUEST_SOURCE_ITEM_IDS_COUNT; j++)
            {
                // examined item is a source item
                if (qinfo->ReqSourceId[j] == itemid && qinfo->ReqSourceRef[j] > 0 && qinfo->ReqSourceRef[j] <= QUEST_OBJECTIVES_COUNT)
                {
                    uint32 idx = qinfo->ReqSourceRef[j]-1;

                    // total count of created ReqItems and SourceItems is less than ReqItemCount
                    if(qinfo->ReqItemId[idx] != 0 &&
                        q_status.m_itemcount[idx] * qinfo->ReqSourceCount[j] + GetItemCount(itemid,true) < qinfo->ReqItemCount[idx] * qinfo->ReqSourceCount[j])
                        return true;

                    // total count of casted ReqCreatureOrGOs and SourceItems is less than ReqCreatureOrGOCount
                    if (qinfo->ReqCreatureOrGOId[idx] != 0)
                    {
                        if(q_status.m_creatureOrGOcount[idx] * qinfo->ReqSourceCount[j] + GetItemCount(itemid,true) < qinfo->ReqCreatureOrGOCount[idx] * qinfo->ReqSourceCount[j])
                            return true;
                    }
                    // spell with SPELL_EFFECT_QUEST_COMPLETE or SPELL_EFFECT_SEND_EVENT (with script) case
                    else if(qinfo->ReqSpell[idx] != 0)
                    {
                        // not casted and need more reagents/item for use.
                        if(!q_status.m_explored && GetItemCount(itemid,true) < qinfo->ReqSourceCount[j])
                            return true;
                    }
                }
            }
        }
    }
    return false;
}

void Player::SendQuestComplete( uint32 quest_id )
{
    if( quest_id )
    {
        WorldPacket data( SMSG_QUESTUPDATE_COMPLETE, 4 );
        data << uint32(quest_id);
        GetSession()->SendPacket( &data );
    }
}

void Player::SendQuestReward( Quest const *pQuest, uint32 XP, Object * questGiver )
{
    uint32 questid = pQuest->GetQuestId();
    gameeventmgr.HandleQuestComplete(questid);
    WorldPacket data( SMSG_QUESTGIVER_QUEST_COMPLETE, (4+4+4+4+4+4+pQuest->GetRewItemsCount()*8) );
    data << questid;
    data << uint32(0x03);

    if ( getLevel() < sWorld.getConfig(CONFIG_MAX_PLAYER_LEVEL) )
    {
        data << XP;
        data << uint32(pQuest->GetRewOrReqMoney());
    }
    else
    {
        data << uint32(0);
        data << uint32(pQuest->GetRewOrReqMoney() + int32(pQuest->GetRewMoneyMaxLevel() * sWorld.getRate(RATE_DROP_MONEY)));
    }
    data << uint32(0);                                      // new 2.3.0, HonorPoints?
    data << uint32( pQuest->GetRewItemsCount() );           // max is 5

    for (uint32 i = 0; i < pQuest->GetRewItemsCount(); ++i)
    {
        if ( pQuest->RewItemId[i] > 0 )
            data << pQuest->RewItemId[i] << pQuest->RewItemCount[i];
        else
            data << uint32(0) << uint32(0);
    }
    GetSession()->SendPacket( &data );

    if (pQuest->GetQuestCompleteScript() != 0)
        sWorld.ScriptsStart(sQuestEndScripts, pQuest->GetQuestCompleteScript(), questGiver, this);
}

void Player::SendQuestFailed( uint32 quest_id )
{
    if( quest_id )
    {
        WorldPacket data( SMSG_QUESTGIVER_QUEST_FAILED, 4 );
        data << quest_id;
        GetSession()->SendPacket( &data );
    }
}

void Player::SendQuestTimerFailed( uint32 quest_id )
{
    if( quest_id )
    {
        WorldPacket data( SMSG_QUESTUPDATE_FAILEDTIMER, 4 );
        data << quest_id;
        GetSession()->SendPacket( &data );
    }
}

void Player::SendCanTakeQuestResponse( uint32 msg )
{
    WorldPacket data( SMSG_QUESTGIVER_QUEST_INVALID, 4 );
    data << uint32(msg);
    GetSession()->SendPacket( &data );
}

void Player::SendQuestConfirmAccept(const Quest* pQuest, Player* pReceiver)
{
    if (pReceiver) {
        std::string strTitle = pQuest->GetTitle();

        int loc_idx = pReceiver->GetSession()->GetSessionDbLocaleIndex();

        if (loc_idx >= 0) {
            if (const QuestLocale* pLocale = objmgr.GetQuestLocale(pQuest->GetQuestId())) {
                if (pLocale->Title.size() > loc_idx && !pLocale->Title[loc_idx].empty())
                    strTitle = pLocale->Title[loc_idx];
            }
        }

        WorldPacket data(SMSG_QUEST_CONFIRM_ACCEPT, (4 + strTitle.size() + 8));
        data << uint32(pQuest->GetQuestId());
        data << strTitle;
        data << uint64(GetGUID());
        pReceiver->GetSession()->SendPacket(&data);
    }
}

void Player::SendPushToPartyResponse( Player *pPlayer, uint32 msg )
{
    if( pPlayer )
    {
        WorldPacket data( MSG_QUEST_PUSH_RESULT, (8+1) );
        data << uint64(pPlayer->GetGUID());
        data << uint8(msg);                                 // valid values: 0-8
        GetSession()->SendPacket( &data );
    }
}

void Player::SendQuestUpdateAddItem( Quest const* pQuest, uint32 item_idx, uint32 count )
{
    WorldPacket data( SMSG_QUESTUPDATE_ADD_ITEM, (4+4) );
    data << pQuest->ReqItemId[item_idx];
    data << count;
    GetSession()->SendPacket( &data );
}

void Player::SendQuestUpdateAddCreatureOrGo( Quest const* pQuest, uint64 guid, uint32 creatureOrGO_idx, uint32 old_count, uint32 add_count )
{
    assert(old_count + add_count < 256 && "mob/GO count store in 8 bits 2^8 = 256 (0..256)");

    int32 entry = pQuest->ReqCreatureOrGOId[ creatureOrGO_idx ];
    if (entry < 0)
        // client expected gameobject template id in form (id|0x80000000)
        entry = (-entry) | 0x80000000;

    WorldPacket data( SMSG_QUESTUPDATE_ADD_KILL, (4*4+8) );
    data << uint32(pQuest->GetQuestId());
    data << uint32(entry);
    data << uint32(old_count + add_count);
    data << uint32(pQuest->ReqCreatureOrGOCount[ creatureOrGO_idx ]);
    data << uint64(guid);
    GetSession()->SendPacket(&data);

    uint16 log_slot = FindQuestSlot( pQuest->GetQuestId() );
    if( log_slot < MAX_QUEST_LOG_SIZE)
        SetQuestSlotCounter(log_slot,creatureOrGO_idx,GetQuestSlotCounter(log_slot,creatureOrGO_idx)+add_count);
}

/*********************************************************/
/***                   LOAD SYSTEM                     ***/
/*********************************************************/

bool Player::MinimalLoadFromDB( QueryResult *result, uint32 guid )
{
    bool delete_result = true;
    if(!result)
    {
        //                                        0     1     2     3           4           5           6    7          8          9
        result = CharacterDatabase.PQuery("SELECT guid, data, name, position_x, position_y, position_z, map, totaltime, leveltime, at_login FROM characters WHERE guid = '%u'",guid);
        if(!result) return false;
    }
    else delete_result = false;

    Field *fields = result->Fetch();
    
    // Override some data fields
    uint32 bytes0 = 0;
    bytes0 |= fields[14].GetUInt8();                         // race
    bytes0 |= fields[15].GetUInt8() << 8;                    // class
    bytes0 |= fields[16].GetUInt8() << 16;                   // gender
    SetUInt32Value(UNIT_FIELD_BYTES_0, bytes0);
    SetUInt32Value(PLAYER_BYTES, fields[17].GetUInt32());   // PlayerBytes
    SetUInt32Value(PLAYER_BYTES_2, fields[18].GetUInt32()); // PlayerBytes2
    SetUInt32Value(PLAYER_FLAGS, fields[19].GetUInt32());   // PlayerFlags
    Tokens tokens = StrSplit(fields[23].GetCppString(), " ");
    Tokens::iterator iter = tokens.begin();
    for (uint32 i = 0; i < 304; ++i) {
        if (i%16 == 2 || i%16 == 3) {
            SetUInt32Value(PLAYER_VISIBLE_ITEM_1_CREATOR + i, atol((*iter).c_str()));
            iter++;
        }
        else
            SetUInt32Value(PLAYER_VISIBLE_ITEM_1_CREATOR + i, 0);
    }

    // overwrite possible wrong/corrupted guid
    SetUInt64Value(OBJECT_FIELD_GUID, MAKE_NEW_GUID(guid, 0, HIGHGUID_PLAYER));

    m_name = fields[2].GetCppString();

    Relocate(fields[3].GetFloat(),fields[4].GetFloat(),fields[5].GetFloat());
    SetMapId(fields[6].GetUInt32());
    // the instance id is not needed at character enum

    m_Played_time[0] = fields[7].GetUInt32();
    m_Played_time[1] = fields[8].GetUInt32();

    m_atLoginFlags = fields[9].GetUInt32();

    // I don't see these used anywhere ..
    /*_LoadGroup();

    _LoadBoundInstances();*/

    if (delete_result) delete result;

    for (int i = 0; i < PLAYER_SLOTS_COUNT; i++)
        m_items[i] = NULL;

    if( HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST) )
        m_deathState = DEAD;

    return true;
}

void Player::_LoadDeclinedNames(QueryResult* result)
{
    if(!result)
        return;

    if(m_declinedname)
        delete m_declinedname;

    m_declinedname = new DeclinedName;
    Field *fields = result->Fetch();
    for(int i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
        m_declinedname->name[i] = fields[i].GetCppString();

    delete result;
}

void Player::_LoadArenaTeamInfo(QueryResult *result)
{
    // arenateamid, played_week, played_season, personal_rating
    memset((void*)&m_uint32Values[PLAYER_FIELD_ARENA_TEAM_INFO_1_1], 0, sizeof(uint32)*18);
    if (!result)
        return;

    do
    {
        Field *fields = result->Fetch();

        uint32 arenateamid     = fields[0].GetUInt32();
        uint32 played_week     = fields[1].GetUInt32();
        uint32 played_season   = fields[2].GetUInt32();
        uint32 personal_rating = fields[3].GetUInt32();

        ArenaTeam* aTeam = objmgr.GetArenaTeamById(arenateamid);
        if(!aTeam)
        {
            sLog.outError("FATAL: couldn't load arenateam %u", arenateamid);
            continue;
        }
        uint8  arenaSlot = aTeam->GetSlot();

        m_uint32Values[PLAYER_FIELD_ARENA_TEAM_INFO_1_1 + arenaSlot * 6]     = arenateamid;      // TeamID
        m_uint32Values[PLAYER_FIELD_ARENA_TEAM_INFO_1_1 + arenaSlot * 6 + 1] = ((aTeam->GetCaptain() == GetGUID()) ? (uint32)0 : (uint32)1); // Captain 0, member 1
        m_uint32Values[PLAYER_FIELD_ARENA_TEAM_INFO_1_1 + arenaSlot * 6 + 2] = played_week;      // Played Week
        m_uint32Values[PLAYER_FIELD_ARENA_TEAM_INFO_1_1 + arenaSlot * 6 + 3] = played_season;    // Played Season
        m_uint32Values[PLAYER_FIELD_ARENA_TEAM_INFO_1_1 + arenaSlot * 6 + 4] = 0;                // Unk
        m_uint32Values[PLAYER_FIELD_ARENA_TEAM_INFO_1_1 + arenaSlot * 6 + 5] = personal_rating;  // Personal Rating

    }while (result->NextRow());
    delete result;
}

bool Player::LoadPositionFromDB(uint32& mapid, float& x,float& y,float& z,float& o, bool& in_flight, uint64 guid)
{
    QueryResult *result = CharacterDatabase.PQuery("SELECT position_x,position_y,position_z,orientation,map,taxi_path FROM characters WHERE guid = '%u'",GUID_LOPART(guid));
    if(!result)
        return false;

    Field *fields = result->Fetch();

    x = fields[0].GetFloat();
    y = fields[1].GetFloat();
    z = fields[2].GetFloat();
    o = fields[3].GetFloat();
    mapid = fields[4].GetUInt32();
    in_flight = !fields[5].GetCppString().empty();

    delete result;
    return true;
}

bool Player::LoadValuesArrayFromDB(Tokens& data, uint64 guid)
{
    QueryResult *result = CharacterDatabase.PQuery("SELECT data FROM characters WHERE guid='%u'",GUID_LOPART(guid));
    if( !result )
        return false;

    Field *fields = result->Fetch();

    data = StrSplit(fields[0].GetCppString(), " ");

    delete result;

    return true;
}

uint32 Player::GetUInt32ValueFromArray(Tokens const& data, uint16 index)
{
    if(index >= data.size())
        return 0;

    return (uint32)atoi(data[index].c_str());
}

float Player::GetFloatValueFromArray(Tokens const& data, uint16 index)
{
    float result;
    uint32 temp = Player::GetUInt32ValueFromArray(data,index);
    memcpy(&result, &temp, sizeof(result));

    return result;
}

uint32 Player::GetUInt32ValueFromDB(uint16 index, uint64 guid)
{
    Tokens data;
    if(!LoadValuesArrayFromDB(data,guid))
        return 0;

    return GetUInt32ValueFromArray(data,index);
}

float Player::GetFloatValueFromDB(uint16 index, uint64 guid)
{
    float result;
    uint32 temp = Player::GetUInt32ValueFromDB(index, guid);
    memcpy(&result, &temp, sizeof(result));

    return result;
}

bool Player::LoadFromDB( uint32 guid, SQLQueryHolder *holder )
{
    QueryResult *result = holder->GetResult(PLAYER_LOGIN_QUERY_LOADFROM);

    Object::_Create( guid, 0, HIGHGUID_PLAYER );

    if(!result)
    {
        sLog.outError("ERROR: Player (GUID: %u) not found in table `characters`, can't load. ",guid);
        return false;
    }

    Field *fields = result->Fetch();

    uint32 dbAccountId = fields[LOAD_DATA_ACCOUNT].GetUInt32();

    // check if the character's account in the db and the logged in account match.
    // player should be able to load/delete character only with correct account!
    if( dbAccountId != GetSession()->GetAccountId() )
    {
        sLog.outError("ERROR: Player (GUID: %u) loading from wrong account (is: %u, should be: %u)",guid,GetSession()->GetAccountId(),dbAccountId);
        delete result;
        return false;
    }

    m_name = fields[LOAD_DATA_NAME].GetCppString();

    // check name limitations
    if(!ObjectMgr::IsValidName(m_name) || GetSession()->GetSecurity() == SEC_PLAYER && objmgr.IsReservedName(m_name))
    {
        delete result;
        CharacterDatabase.PExecute("UPDATE characters SET at_login = at_login | '%u' WHERE guid ='%u'", uint32(AT_LOGIN_RENAME),guid);
        return false;
    }

    /*if(!LoadValues( fields[LOAD_DATA_DATA].GetString()))
    {
        sLog.outError("ERROR: Player #%d have broken data in `data` field. Can't be loaded.",GUID_LOPART(guid));
        delete result;
        return false;
    }*/

    // overwrite possible wrong/corrupted guid
    SetUInt64Value(OBJECT_FIELD_GUID, MAKE_NEW_GUID(guid, 0, HIGHGUID_PLAYER));

    // cleanup inventory related item value fields (its will be filled correctly in _LoadInventory)
    for(uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
    {
        SetUInt64Value( (uint16)(PLAYER_FIELD_INV_SLOT_HEAD + (slot * 2) ), 0 );
        SetVisibleItemSlot(slot,NULL);

        if (m_items[slot])
        {
            delete m_items[slot];
            m_items[slot] = NULL;
        }
    }

    m_race = fields[LOAD_DATA_RACE].GetUInt8();
    m_class = fields[LOAD_DATA_CLASS].GetUInt8();
    m_gender = fields[LOAD_DATA_GENDER].GetUInt8();
    //Need to call it to initialize m_team (m_team can be calculated from m_race)
    //Other way is to saves m_team into characters table.
    setFactionForRace(m_race);
    SetCharm(0);
    
    // Override some data fields
    uint32 bytes0 = 0;
    bytes0 |= m_race;                               // race
    bytes0 |= m_class << 8;                         // class
    bytes0 |= m_gender << 16;                       // gender
    SetUInt32Value(UNIT_FIELD_LEVEL, fields[LOAD_DATA_LEVEL].GetUInt8());
    SetUInt32Value(PLAYER_XP, fields[LOAD_DATA_XP].GetUInt32());
    SetUInt32Value(PLAYER_FIELD_COINAGE, fields[LOAD_DATA_MONEY].GetUInt32());
    SetUInt32Value(UNIT_FIELD_BYTES_0, bytes0);
    SetByteValue(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_SANCTUARY | UNIT_BYTE2_FLAG_UNK5 );
    //SetByteValue(PLAYER_BYTES_3, 0, m_gender);
    SetUInt32Value(PLAYER_BYTES, fields[LOAD_DATA_PLAYERBYTES].GetUInt32());   // PlayerBytes
    SetUInt32Value(PLAYER_BYTES_2, fields[LOAD_DATA_PLAYERBYTES2].GetUInt32()); // PlayerBytes2
    SetUInt32Value(PLAYER_FLAGS, fields[LOAD_DATA_PLAYERFLAGS].GetUInt32());   // PlayerFlags
    SetUInt32Value(PLAYER_BYTES_3, (fields[LOAD_DATA_DRUNK].GetUInt16() & 0xFFFE) | fields[LOAD_DATA_GENDER].GetUInt8());
    SetInt32Value(PLAYER_FIELD_WATCHED_FACTION_INDEX, fields[LOAD_DATA_WATCHED_FACTION].GetUInt32());
    SetUInt32Value(PLAYER_AMMO_ID, fields[LOAD_DATA_AMMOID].GetUInt32());
    SetByteValue(PLAYER_FIELD_BYTES, 2, fields[LOAD_DATA_ACTIONBARS].GetUInt8());
    _LoadIntoDataField(fields[LOAD_DATA_EXPLOREDZONES].GetString(), PLAYER_EXPLORED_ZONES_1, 128);
    _LoadIntoDataField(fields[LOAD_DATA_KNOWNTITLES].GetString(), PLAYER_FIELD_KNOWN_TITLES, 2);
    
    SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS, DEFAULT_WORLD_OBJECT_SIZE);
    SetFloatValue(UNIT_FIELD_COMBATREACH, 1.5f);
    //SetFloatValue(UNIT_FIELD_HOVERHEIGHT, 1.0f);
    
    // update money limits
    if(GetMoney() > MAX_MONEY_AMOUNT)
        SetMoney(MAX_MONEY_AMOUNT);
    
    // Override NativeDisplayId in case of race/faction change
    PlayerInfo const* info = objmgr.GetPlayerInfo(m_race, m_class);
    if (!info) {
        sLog.outError("Player has incorrect race/class pair. Can't be loaded.");
        return false;
    }
    
    switch (m_gender) {
    case GENDER_FEMALE:
        SetDisplayId(info->displayId_f);
        SetNativeDisplayId(info->displayId_f);
        break;
    case GENDER_MALE:
        SetDisplayId(info->displayId_m);
        SetNativeDisplayId(info->displayId_m);
        break;
    default:
        sLog.outError("Invalid gender %u for player", m_gender);
        return false;
    }

    // load home bind and check in same time class/race pair, it used later for restore broken positions
    if (!_LoadHomeBind(holder->GetResult(PLAYER_LOGIN_QUERY_LOADHOMEBIND)))
        return false;

    InitPrimaryProffesions();                               // to max set before any spell loaded

    // init saved position, and fix it later if problematic
    uint32 transGUID = fields[LOAD_DATA_TRANSGUID].GetUInt32();
    if(sWorld.getConfig(CONFIG_ARENASERVER_ENABLED))
    {
        float x,y,z,o;
        uint32 tMapId;
        GetArenaZoneCoord(ShouldGoToSecondaryArenaZone(),tMapId,x,y,z,o);
        Relocate(x,y,z,o);
        SetFallInformation(0, z);
        SetMapId(tMapId);
    } else {
        Relocate(fields[LOAD_DATA_POSX].GetFloat(),fields[LOAD_DATA_POSY].GetFloat(),fields[LOAD_DATA_POSZ].GetFloat(),fields[LOAD_DATA_ORIENTATION].GetFloat());
        SetFallInformation(0, fields[LOAD_DATA_POSZ].GetFloat());
        SetMapId(fields[LOAD_DATA_MAP].GetUInt32()); 
    }

    SetDifficulty(fields[LOAD_DATA_DUNGEON_DIFF].GetUInt32());                  // may be changed in _LoadGroup
    

    // Experience Blocking
    m_isXpBlocked = fields[LOAD_DATA_XP_BLOCKED].GetUInt8();
    
    m_lastGenderChange = fields[LOAD_DATA_LAST_GENDER_CHANGE].GetUInt64();
    
    m_customXp = fields[LOAD_DATA_CUSTOM_XP].GetFloat();
    // Check value
    if (m_customXp < 1.0f || m_customXp > sWorld.getRate(RATE_XP_KILL))
        m_customXp = 0;

    // instance id
    SetInstanceId(fields[LOAD_DATA_INSTANCE_ID].GetUInt32());
    
    _LoadGroup(holder->GetResult(PLAYER_LOGIN_QUERY_LOADGROUP));

    _LoadArenaTeamInfo(holder->GetResult(PLAYER_LOGIN_QUERY_LOADARENAINFO));

    uint32 arena_currency = fields[LOAD_DATA_ARENAPOINTS].GetUInt32();
    if (arena_currency > sWorld.getConfig(CONFIG_MAX_ARENA_POINTS))
        arena_currency = sWorld.getConfig(CONFIG_MAX_ARENA_POINTS);

    SetUInt32Value(PLAYER_FIELD_ARENA_CURRENCY, arena_currency);

    // check arena teams integrity
    for(uint32 arena_slot = 0; arena_slot < MAX_ARENA_SLOT; ++arena_slot)
    {
        uint32 arena_team_id = GetArenaTeamId(arena_slot);
        if(!arena_team_id)
            continue;

        if(ArenaTeam * at = objmgr.GetArenaTeamById(arena_team_id))
            if(at->HaveMember(GetGUID()))
                continue;

        // arena team not exist or not member, cleanup fields
        for(int j =0; j < 6; ++j)
            SetUInt32Value(PLAYER_FIELD_ARENA_TEAM_INFO_1_1 + arena_slot * 6 + j, 0);
    }
    
    SetUInt32Value(PLAYER_FIELD_HONOR_CURRENCY, fields[LOAD_DATA_TOTALHONORPOINTS].GetUInt32());
    SetUInt32Value(PLAYER_FIELD_TODAY_CONTRIBUTION, fields[LOAD_DATA_TODAYHONORPOINTS].GetUInt32());
    SetUInt32Value(PLAYER_FIELD_YESTERDAY_CONTRIBUTION, fields[LOAD_DATA_YESTERDAYHONORPOINTS].GetUInt32());
    SetUInt32Value(PLAYER_FIELD_LIFETIME_HONORABLE_KILLS, fields[LOAD_DATA_TOTALKILLS].GetUInt32());
    SetUInt16Value(PLAYER_FIELD_KILLS, 0, fields[LOAD_DATA_TODAYKILLS].GetUInt16());
    SetUInt16Value(PLAYER_FIELD_KILLS, 1, fields[LOAD_DATA_YESTERDAYKILLS].GetUInt16());

    _LoadBoundInstances(holder->GetResult(PLAYER_LOGIN_QUERY_LOADBOUNDINSTANCES));

    if(!IsPositionValid())
    {
        sLog.outError("Player (guidlow %d) have invalid coordinates (X: %f Y: %f Z: %f O: %f). Teleport to default race/class locations.",guid,GetPositionX(),GetPositionY(),GetPositionZ(),GetOrientation());
        RelocateToHomebind();

        transGUID = 0;

        m_movementInfo.t_x = 0.0f;
        m_movementInfo.t_y = 0.0f;
        m_movementInfo.t_z = 0.0f;
        m_movementInfo.t_o = 0.0f;
    }

    ////                                                     0     1       2      3    4    5    6
    //QueryResult *result = CharacterDatabase.PQuery("SELECT bgid, bgteam, bgmap, bgx, bgy, bgz, bgo FROM character_bgcoord WHERE guid = '%u'", GUID_LOPART(m_guid));
    QueryResult *resultbg = holder->GetResult(PLAYER_LOGIN_QUERY_LOADBGCOORD);

    if(resultbg)
    {
        Field *fieldsbg = resultbg->Fetch();

        uint32 bgid = fieldsbg[0].GetUInt32();
        uint32 bgteam = fieldsbg[1].GetUInt32();

        if(bgid) //saved in BattleGround
        {
            SetBattleGroundEntryPoint(fieldsbg[2].GetUInt32(),fieldsbg[3].GetFloat(),fieldsbg[4].GetFloat(),fieldsbg[5].GetFloat(),fieldsbg[6].GetFloat());

            BattleGround *currentBg = sBattleGroundMgr.GetBattleGround(bgid);

            if(currentBg && currentBg->IsPlayerInBattleGround(GetGUID()))
            {
                uint32 bgQueueTypeId = sBattleGroundMgr.BGQueueTypeId(currentBg->GetTypeID(), currentBg->GetArenaType());
                uint32 queueSlot = AddBattleGroundQueueId(bgQueueTypeId);

                SetBattleGroundId(currentBg->GetInstanceID());
                SetBGTeam(bgteam);

                SetInviteForBattleGroundQueueType(bgQueueTypeId,currentBg->GetInstanceID());
            }
            else
            {
                SetMapId(GetBattleGroundEntryPointMap());
                Relocate(GetBattleGroundEntryPointX(),GetBattleGroundEntryPointY(),GetBattleGroundEntryPointZ(),GetBattleGroundEntryPointO());
                //RemoveArenaAuras(true);
            }
        }
    }

    if (transGUID != 0)
    {
        m_movementInfo.t_x = fields[LOAD_DATA_TRANSX].GetFloat();
        m_movementInfo.t_y = fields[LOAD_DATA_TRANSY].GetFloat();
        m_movementInfo.t_z = fields[LOAD_DATA_TRANSZ].GetFloat();
        m_movementInfo.t_o = fields[LOAD_DATA_TRANSO].GetFloat();

        if( !Trinity::IsValidMapCoord(
            GetPositionX()+m_movementInfo.t_x,GetPositionY()+m_movementInfo.t_y,
            GetPositionZ()+m_movementInfo.t_z,GetOrientation()+m_movementInfo.t_o) ||
            // transport size limited
            m_movementInfo.t_x > 50 || m_movementInfo.t_y > 50 || m_movementInfo.t_z > 50 )
        {
            sLog.outError("Player (guidlow %d) have invalid transport coordinates (X: %f Y: %f Z: %f O: %f). Teleport to default race/class locations.",
                guid,GetPositionX()+m_movementInfo.t_x,GetPositionY()+m_movementInfo.t_y,
                GetPositionZ()+m_movementInfo.t_z,GetOrientation()+m_movementInfo.t_o);

            RelocateToHomebind();

            m_movementInfo.t_x = 0.0f;
            m_movementInfo.t_y = 0.0f;
            m_movementInfo.t_z = 0.0f;
            m_movementInfo.t_o = 0.0f;

            transGUID = 0;
        }
    }

    if (transGUID != 0)
    {
        for (MapManager::TransportSet::iterator iter = MapManager::Instance().m_Transports.begin(); iter != MapManager::Instance().m_Transports.end(); ++iter)
        {
            if( (*iter)->GetGUIDLow() == transGUID)
            {
                m_transport = *iter;
                m_transport->AddPassenger(this);
                SetMapId(m_transport->GetMapId());
                break;
            }
        }

        if(!m_transport)
        {
            sLog.outError("Player (guidlow %d) have invalid transport guid (%u). Teleport to default race/class locations.",
                guid,transGUID);

            RelocateToHomebind();

            m_movementInfo.t_x = 0.0f;
            m_movementInfo.t_y = 0.0f;
            m_movementInfo.t_z = 0.0f;
            m_movementInfo.t_o = 0.0f;

            transGUID = 0;
        }
    }

    // In some old saves players' instance id are not correctly ordered
    // This fixes the crash. But it is not needed for a new db
    if (InstanceSave *pSave = GetInstanceSave(GetMapId()))
        if (pSave->GetInstanceId() != GetInstanceId())
            SetInstanceId(pSave->GetInstanceId());

    // NOW player must have valid map
    // load the player's map here if it's not already loaded
    Map *map = GetMap();

    if (!map)
    {
        AreaTrigger const* at = objmgr.GetGoBackTrigger(GetMapId());
        if (at)
        {
            SetMapId(at->target_mapId);
            Relocate(at->target_X, at->target_Y, at->target_Z, GetOrientation());
            sLog.outError("Player (guidlow %d) is teleported to gobacktrigger (Map: %u X: %f Y: %f Z: %f O: %f).",guid,GetMapId(),GetPositionX(),GetPositionY(),GetPositionZ(),GetOrientation());
        }
        else
        {
            RelocateToHomebind();
            sLog.outError("Player (guidlow %d) is teleported to home (Map: %u X: %f Y: %f Z: %f O: %f).",guid,GetMapId(),GetPositionX(),GetPositionY(),GetPositionZ(),GetOrientation());
        }

        map = GetMap();
        if (!map)
        {
            sLog.outError("ERROR: Player (guidlow %d) have invalid coordinates (X: %f Y: %f Z: %f O: %f). Teleport to default race/class locations.",guid,GetPositionX(),GetPositionY(),GetPositionZ(),GetOrientation());
            return false;
        }

    }

    // since the player may not be bound to the map yet, make sure subsequent
    // getmap calls won't create new maps
    SetInstanceId(map->GetInstanceId());

    // if the player is in an instance and it has been reset in the meantime teleport him to the entrance
    if (GetInstanceId() && !sInstanceSaveManager.GetInstanceSave(GetInstanceId()))
    {
        AreaTrigger const* at = objmgr.GetMapEntranceTrigger(GetMapId());
        if (at)
            Relocate(at->target_X, at->target_Y, at->target_Z, at->target_Orientation);
        else if (!map->IsBattleGroundOrArena())
            sLog.outError("Player %s(GUID: %u) logged in to a reset instance (map: %u) and there is no area-trigger leading to this map. Thus he can't be ported back to the entrance. This _might_ be an exploit attempt.", 
                    GetName(), GetGUIDLow(), GetMapId());
    }

    SaveRecallPosition();

    time_t now = time(NULL);
    time_t logoutTime = time_t(fields[LOAD_DATA_LOGOUT_TIME].GetUInt64());

    // since last logout (in seconds)
    uint64 time_diff = uint64(now - logoutTime);

    // set value, including drunk invisibility detection
    // calculate sobering. after 15 minutes logged out, the player will be sober again
    float soberFactor;
    if(time_diff > 15*MINUTE)
        soberFactor = 0;
    else
        soberFactor = 1-time_diff/(15.0f*MINUTE);
    uint16 newDrunkenValue = uint16(soberFactor*(GetUInt32Value(PLAYER_BYTES_3) & 0xFFFE));
    SetDrunkValue(newDrunkenValue);

    m_cinematic = fields[LOAD_DATA_CINEMATIC].GetUInt32();
    m_Played_time[0]= fields[LOAD_DATA_TOTALTIME].GetUInt32();
    m_Played_time[1]= fields[LOAD_DATA_LEVELTIME].GetUInt32();

    m_resetTalentsCost = fields[LOAD_DATA_RESETTALENTS_COST].GetUInt32();
    m_resetTalentsTime = time_t(fields[LOAD_DATA_RESETTALENTS_TIME].GetUInt64());

    // reserve some flags
    uint32 old_safe_flags = GetUInt32Value(PLAYER_FLAGS) & ( PLAYER_FLAGS_HIDE_CLOAK | PLAYER_FLAGS_HIDE_HELM );

    if( HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_GM) )
        SetUInt32Value(PLAYER_FLAGS, 0 | old_safe_flags);

    m_taxi.LoadTaxiMask( fields[LOAD_DATA_TAXIMASK].GetString() );          // must be before InitTaxiNodesForLevel

    uint32 extraflags = fields[LOAD_DATA_EXTRA_FLAGS].GetUInt32();

    m_stableSlots = fields[LOAD_DATA_STABLE_SLOTS].GetUInt32();
    if(m_stableSlots > 2)
    {
        sLog.outError("Player can have not more 2 stable slots, but have in DB %u",uint32(m_stableSlots));
        m_stableSlots = 2;
    }

    m_atLoginFlags = fields[LOAD_DATA_AT_LOGIN].GetUInt32();

    // Honor system
    // Update Honor kills data
    m_lastHonorUpdateTime = logoutTime;
    UpdateHonorFields();

    m_deathExpireTime = (time_t)fields[LOAD_DATA_DEATH_EXPIRE_TIME].GetUInt64();
    if(m_deathExpireTime > now+MAX_DEATH_COUNT*DEATH_EXPIRE_STEP)
        m_deathExpireTime = now+MAX_DEATH_COUNT*DEATH_EXPIRE_STEP-1;

    std::string taxi_nodes = fields[LOAD_DATA_TAXI_PATH].GetCppString();

    // clear channel spell data (if saved at channel spell casting)
    SetUInt64Value(UNIT_FIELD_CHANNEL_OBJECT, 0);
    SetUInt32Value(UNIT_CHANNEL_SPELL,0);

    // clear charm/summon related fields
    SetCharm(NULL);
    SetPet(NULL);
    SetCharmerGUID(NULL);
    SetOwnerGUID(NULL);
    SetCreatorGUID(NULL);

    // reset some aura modifiers before aura apply
    SetFarSight(NULL);
    SetUInt32Value(PLAYER_TRACK_CREATURES, 0 );
    SetUInt32Value(PLAYER_TRACK_RESOURCES, 0 );

    _LoadSkills(holder->GetResult(PLAYER_LOGIN_QUERY_LOADSKILLS));

    // make sure the unit is considered out of combat for proper loading
    ClearInCombat();

    // make sure the unit is considered not in duel for proper loading
    SetUInt64Value(PLAYER_DUEL_ARBITER, 0);
    SetUInt32Value(PLAYER_DUEL_TEAM, 0);

    // reset stats before loading any modifiers
    InitStatsForLevel();
    InitTaxiNodesForLevel();

    // After InitStatsForLevel(), or PLAYER_NEXT_LEVEL_XP is 0 and rest bonus too
    m_rest_bonus = fields[LOAD_DATA_REST_BONUS].GetFloat();
    //speed collect rest bonus in offline, in logout, far from tavern, city (section/in hour)
    float bubble0 = 0.031;
    //speed collect rest bonus in offline, in logout, in tavern, city (section/in hour)
    float bubble1 = 0.125;
    
    if((int32)fields[LOAD_DATA_LOGOUT_TIME].GetUInt32() > 0)
    {
        float bubble = fields[LOAD_DATA_IS_LOGOUT_RESTING].GetUInt32() > 0
            ? bubble1*sWorld.getRate(RATE_REST_OFFLINE_IN_TAVERN_OR_CITY)
            : bubble0*sWorld.getRate(RATE_REST_OFFLINE_IN_WILDERNESS);

        SetRestBonus(GetRestBonus()+ time_diff*((float)GetUInt32Value(PLAYER_NEXT_LEVEL_XP)/72000)*bubble);
    }
    // apply original stats mods before spell loading or item equipment that call before equip _RemoveStatsMods()

    //mails are loaded only when needed ;-) - when player in game click on mailbox.
    //_LoadMail();

    _LoadAuras(holder->GetResult(PLAYER_LOGIN_QUERY_LOADAURAS), time_diff);

    // add ghost flag (must be after aura load: PLAYER_FLAGS_GHOST set in aura)
    if( HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST) )
        m_deathState = DEAD;

    _LoadSpells(holder->GetResult(PLAYER_LOGIN_QUERY_LOADSPELLS));

    // after spell load
    InitTalentForLevel();
    learnSkillRewardedSpells();

    // after spell load, learn rewarded spell if need also
    _LoadQuestStatus(holder->GetResult(PLAYER_LOGIN_QUERY_LOADQUESTSTATUS));
    _LoadDailyQuestStatus(holder->GetResult(PLAYER_LOGIN_QUERY_LOADDAILYQUESTSTATUS));

    _LoadTutorials(holder->GetResult(PLAYER_LOGIN_QUERY_LOADTUTORIALS));

    // must be before inventory (some items required reputation check)
    _LoadReputation(holder->GetResult(PLAYER_LOGIN_QUERY_LOADREPUTATION));

    _LoadInventory(holder->GetResult(PLAYER_LOGIN_QUERY_LOADINVENTORY), time_diff);
    
    // update items with duration and realtime
    UpdateItemDuration(time_diff, true);

    _LoadActions(holder->GetResult(PLAYER_LOGIN_QUERY_LOADACTIONS));

    // unread mails and next delivery time, actual mails not loaded
    _LoadMailInit(holder->GetResult(PLAYER_LOGIN_QUERY_LOADMAILCOUNT), holder->GetResult(PLAYER_LOGIN_QUERY_LOADMAILDATE));

    m_social = sSocialMgr.LoadFromDB(holder->GetResult(PLAYER_LOGIN_QUERY_LOADSOCIALLIST), GetGUIDLow());

    // check PLAYER_CHOSEN_TITLE compatibility with PLAYER_FIELD_KNOWN_TITLES
    // note: PLAYER_FIELD_KNOWN_TITLES updated at quest status loaded
    uint32 curTitle = fields[LOAD_DATA_CHOSEN_TITLE].GetUInt32();
    
    if (curTitle && !HasTitle(curTitle))
        curTitle = 0;
    
    SetUInt32Value(PLAYER_CHOSEN_TITLE, curTitle);

    // Not finish taxi flight path
    if(!m_taxi.LoadTaxiDestinationsFromString(taxi_nodes))
    {
        // problems with taxi path loading
        TaxiNodesEntry const* nodeEntry = NULL;
        if(uint32 node_id = m_taxi.GetTaxiSource())
            nodeEntry = sTaxiNodesStore.LookupEntry(node_id);

        if(!nodeEntry)                                      // don't know taxi start node, to homebind
        {
            sLog.outError("Character %u have wrong data in taxi destination list, teleport to homebind.",GetGUIDLow());
            RelocateToHomebind();
            SaveRecallPosition();                           // save as recall also to prevent recall and fall from sky
        }
        else                                                // have start node, to it
        {
            sLog.outError("Character %u have too short taxi destination list, teleport to original node.",GetGUIDLow());
            SetMapId(nodeEntry->map_id);
            Relocate(nodeEntry->x, nodeEntry->y, nodeEntry->z,0.0f);
            SaveRecallPosition();                           // save as recall also to prevent recall and fall from sky
        }
        CleanupAfterTaxiFlight();
    }
    else if(uint32 node_id = m_taxi.GetTaxiSource())
    {
        // save source node as recall coord to prevent recall and fall from sky
        TaxiNodesEntry const* nodeEntry = sTaxiNodesStore.LookupEntry(node_id);
        assert(nodeEntry);                                  // checked in m_taxi.LoadTaxiDestinationsFromString
        m_recallMap = nodeEntry->map_id;
        m_recallX = nodeEntry->x;
        m_recallY = nodeEntry->y;
        m_recallZ = nodeEntry->z;

        // flight will started later
    }

    _LoadSpellCooldowns(holder->GetResult(PLAYER_LOGIN_QUERY_LOADSPELLCOOLDOWNS));

    // Spell code allow apply any auras to dead character in load time in aura/spell/item loading
    // Do now before stats re-calculation cleanup for ghost state unexpected auras
    if(!isAlive())
        RemoveAllAurasOnDeath();

    //apply all stat bonuses from items and auras
    SetCanModifyStats(true);
    UpdateAllStats();

    // restore remembered power/health values (but not more max values)
    uint32 savedHealth = fields[LOAD_DATA_HEALTH].GetUInt32();
    SetHealth(savedHealth > GetMaxHealth() ? GetMaxHealth() : savedHealth);
    for(uint32 i = 0; i < MAX_POWERS; ++i) {
        uint32 savedPower = fields[LOAD_DATA_POWER1+i].GetUInt32();
        SetPower(Powers(i),savedPower > GetMaxPower(Powers(i)) ? GetMaxPower(Powers(i)) : savedPower);
    }

    // GM state
    if(GetSession()->GetSecurity() > SEC_PLAYER)
    {
        switch(sWorld.getConfig(CONFIG_GM_LOGIN_STATE))
        {
            default:
            case 0:                      break;             // disable
            case 1: SetGameMaster(true); break;             // enable
            case 2:                                         // save state
                if(extraflags & PLAYER_EXTRA_GM_ON)
                    SetGameMaster(true);
                break;
        }

        switch(sWorld.getConfig(CONFIG_GM_VISIBLE_STATE))
        {
            default:
            case 0: SetGMVisible(false); break;             // invisible
            case 1:                      break;             // visible
            case 2:                                         // save state
                if(extraflags & PLAYER_EXTRA_GM_INVISIBLE)
                    SetGMVisible(false);
                break;
        }

        switch(sWorld.getConfig(CONFIG_GM_CHAT))
        {
            default:
            case 0:                  break;                 // disable
            case 1: SetGMChat(true); break;                 // enable
            case 2:                                         // save state
                if(extraflags & PLAYER_EXTRA_GM_CHAT)
                    SetGMChat(true);
                break;
        }

        switch(sWorld.getConfig(CONFIG_GM_WISPERING_TO))
        {
            default:
            case 0:                          break;         // disable
            case 1: SetAcceptWhispers(true); break;         // enable
            case 2:                                         // save state
                if(extraflags & PLAYER_EXTRA_ACCEPT_WHISPERS)
                    SetAcceptWhispers(true);
                break;
        }
    }

    _LoadDeclinedNames(holder->GetResult(PLAYER_LOGIN_QUERY_LOADDECLINEDNAMES));

    delete result;
    return true;
}

bool Player::isAllowedToLoot(Creature* creature)
{
    if(creature->isDead() && !creature->IsDamageEnoughForLootingAndReward())
       return false;

    if(Player* recipient = creature->GetLootRecipient())
    {
        if (recipient == this)
            return true;
        if( Group* otherGroup = recipient->GetGroup())
        {
            Group* thisGroup = GetGroup();
            if(!thisGroup)
                return false;
            return thisGroup == otherGroup;
        }
        return false;
    }
    else
        // prevent other players from looting if the recipient got disconnected
        return !creature->hasLootRecipient();
}

void Player::_LoadActions(QueryResult *result)
{
    m_actionButtons.clear();

    //QueryResult *result = CharacterDatabase.PQuery("SELECT button,action,type,misc FROM character_action WHERE guid = '%u' ORDER BY button",GetGUIDLow());

    if(result)
    {
        do
        {
            Field *fields = result->Fetch();

            uint8 button = fields[0].GetUInt8();

            addActionButton(button, fields[1].GetUInt16(), fields[2].GetUInt8(), fields[3].GetUInt8());

            m_actionButtons[button].uState = ACTIONBUTTON_UNCHANGED;
        }
        while( result->NextRow() );

        delete result;
    }
}

void Player::_LoadIntoDataField(const char* data, uint32 startOffset, uint32 count)
{
    if (!data)
        return;
        
    Tokens tokens = StrSplit(data, " ");
    
    if (tokens.size() != count)
        return;
        
    Tokens::iterator iter;
    uint32 index;
    for (iter = tokens.begin(), index = 0; index < count; ++iter, ++index)
    {
        m_uint32Values[startOffset + index] = atol((*iter).c_str());
    }
}

void Player::_LoadAuras(QueryResult *result, uint32 timediff)
{
    m_Auras.clear();
    for (int i = 0; i < TOTAL_AURAS; i++)
        m_modAuras[i].clear();

    // all aura related fields
    for(int i = UNIT_FIELD_AURA; i <= UNIT_FIELD_AURASTATE; ++i)
        SetUInt32Value(i, 0);

    //QueryResult *result = CharacterDatabase.PQuery("SELECT caster_guid,spell,effect_index,stackcount,amount,maxduration,remaintime,remaincharges FROM character_aura WHERE guid = '%u'",GetGUIDLow());

    if(result)
    {
        do
        {
            Field *fields = result->Fetch();
            uint64 caster_guid = fields[0].GetUInt64();
            uint32 spellid = fields[1].GetUInt32();
            uint32 effindex = fields[2].GetUInt32();
            uint32 stackcount = fields[3].GetUInt32();
            int32 damage     = (int32)fields[4].GetUInt32();
            int32 maxduration = (int32)fields[5].GetUInt32();
            int32 remaintime = (int32)fields[6].GetUInt32();
            int32 remaincharges = (int32)fields[7].GetUInt32();

            if(spellid == SPELL_ARENA_PREPARATION || spellid == SPELL_PREPARATION)
            {
               if(BattleGround const *bg = GetBattleGround())
                   if(bg->GetStatus() == STATUS_IN_PROGRESS)
                       continue;
            }

            SpellEntry const* spellproto = spellmgr.LookupSpell(spellid);
            if(!spellproto)
            {
                sLog.outError("Unknown aura (spellid %u, effindex %u), ignore.",spellid,effindex);
                continue;
            }

            if(effindex >= 3)
            {
                sLog.outError("Invalid effect index (spellid %u, effindex %u), ignore.",spellid,effindex);
                continue;
            }

            // negative effects should continue counting down after logout
            if (remaintime != -1 && !IsPositiveEffect(spellid, effindex))
            {
                if(remaintime  <= int32(timediff))
                    continue;

                remaintime -= timediff;
            }

            // prevent wrong values of remaincharges
            if(spellproto->procCharges)
            {
                if(remaincharges <= 0 || remaincharges > spellproto->procCharges)
                    remaincharges = spellproto->procCharges;
            }
            else
                remaincharges = -1;

            //do not load single target auras (unless they were cast by the player)
            if (caster_guid != GetGUID() && IsSingleTargetSpell(spellproto))
                continue;
            
            // Do not load SPELL_AURA_IGNORED auras
	    bool abort = false;
            for (uint8 i = 0; i < 3; i++) {
                if (spellproto->Effect[i] == SPELL_EFFECT_APPLY_AURA && spellproto->EffectApplyAuraName[i] == 221)
                    abort = true;
            }

	    if (abort)
		continue;

            for(uint32 i=0; i<stackcount; i++)
            {
                Aura* aura = CreateAura(spellproto, effindex, NULL, this, NULL);
                if(!damage)
                    damage = aura->GetModifier()->m_amount;
                aura->SetLoadedState(caster_guid,damage,maxduration,remaintime,remaincharges);
                AddAura(aura);
                sLog.outDetail("Added aura spellid %u, effect %u", spellproto->Id, effindex);
            }
        }
        while( result->NextRow() );

        delete result;
    }

    if(m_class == CLASS_WARRIOR)
        CastSpell(this,SPELL_ID_PASSIVE_BATTLE_STANCE,true);
}

void Player::LoadCorpse()
{
    if( isAlive() )
    {
        ObjectAccessor::Instance().ConvertCorpseForPlayer(GetGUID());
    }
    else
    {
        if(Corpse *corpse = GetCorpse())
        {
            ApplyModFlag(PLAYER_FIELD_BYTES, PLAYER_FIELD_BYTE_RELEASE_TIMER, corpse && !sMapStore.LookupEntry(corpse->GetMapId())->Instanceable() );
        }
        else
        {
            //Prevent Dead Player login without corpse
            ResurrectPlayer(0.5f);
        }
    }
}

void Player::_LoadInventory(QueryResult *result, uint32 timediff)
{
    //QueryResult *result = CharacterDatabase.PQuery("SELECT data,bag,slot,item,item_template FROM character_inventory JOIN item_instance ON character_inventory.item = item_instance.guid WHERE character_inventory.guid = '%u' ORDER BY bag,slot", GetGUIDLow());
    std::map<uint64, Bag*> bagMap;                          // fast guid lookup for bags
    //NOTE: the "order by `bag`" is important because it makes sure
    //the bagMap is filled before items in the bags are loaded
    //NOTE2: the "order by `slot`" is needed because mainhand weapons are (wrongly?)
    //expected to be equipped before offhand items (TODO: fixme)

    uint32 zone = GetZoneId();

    if (result)
    {
        std::list<Item*> problematicItems;

        // prevent items from being added to the queue when stored
        m_itemUpdateQueueBlocked = true;
        do
        {
            Field *fields = result->Fetch();
            uint32 bag_guid  = fields[1].GetUInt32();
            uint8  slot      = fields[2].GetUInt8();
            uint32 item_guid = fields[3].GetUInt32();
            uint32 item_id   = fields[4].GetUInt32();

            ItemPrototype const * proto = objmgr.GetItemPrototype(item_id);

            if(!proto)
            {
                /*SQLTransaction trans = CharacterDatabase.BeginTransaction();
                trans->PAppend("DELETE FROM character_inventory WHERE item = '%u'", item_guid);
                trans->PAppend("DELETE FROM item_instance WHERE guid = '%u'", item_guid);
                CharacterDatabase.CommitTransaction(trans);*/
                sLog.outError( "Player::_LoadInventory: Player %s has an unknown item (id: #%u) in inventory, not loaded.", GetName(),item_id );
                continue;
            }

            Item *item = NewItemOrBag(proto);

            if(!item->LoadFromDB(item_guid, GetGUID(), result))
            {
                sLog.outError( "Player::_LoadInventory: Player %s has broken item (id: #%u) in inventory, not loaded.", GetName(),item_id );
                //CharacterDatabase.PExecute("DELETE FROM character_inventory WHERE item = '%u'", item_guid);
                //item->FSetState(ITEM_REMOVED);
                //item->SaveToDB();                           // it also deletes item object !
                continue;
            }

            // not allow have in alive state item limited to another map/zone
            if(isAlive() && item->IsLimitedToAnotherMapOrZone(GetMapId(),zone) )
            {
                SQLTransaction trans = CharacterDatabase.BeginTransaction();
                trans->PAppend("DELETE FROM character_inventory WHERE item = '%u'", item_guid);
                item->FSetState(ITEM_REMOVED);
                item->SaveToDB(trans);                           // it also deletes item object !
                CharacterDatabase.CommitTransaction(trans);
                continue;
            }

            // "Conjured items disappear if you are logged out for more than 15 minutes"
            if ((timediff > 15*60) && (item->HasFlag(ITEM_FIELD_FLAGS, ITEM_FLAGS_CONJURED)))
            {
                SQLTransaction trans = CharacterDatabase.BeginTransaction();
                trans->PAppend("DELETE FROM character_inventory WHERE item = '%u'", item_guid);
                item->FSetState(ITEM_REMOVED);
                item->SaveToDB(trans);                           // it also deletes item object !
                CharacterDatabase.CommitTransaction(trans);
                continue;
            }
            
            /*if (item->GetUInt32Value(ITEM_FIELD_DURATION) && item->GetProto()->Duration != 0)
                AddItemDurations(item);*/

            bool success = true;

            if (!bag_guid)
            {
                // the item is not in a bag
                item->SetContainer( NULL );
                item->SetSlot(slot);

                if( IsInventoryPos( INVENTORY_SLOT_BAG_0, slot ) )
                {
                    ItemPosCountVec dest;
                    if( CanStoreItem( INVENTORY_SLOT_BAG_0, slot, dest, item, false ) == EQUIP_ERR_OK )
                        item = StoreItem(dest, item, true);
                    else
                        success = false;
                }
                else if( IsEquipmentPos( INVENTORY_SLOT_BAG_0, slot ) )
                {
                    uint16 dest;
                    if( CanEquipItem( slot, dest, item, false, false ) == EQUIP_ERR_OK )
                        QuickEquipItem(dest, item);
                    else
                        success = false;
                }
                else if( IsBankPos( INVENTORY_SLOT_BAG_0, slot ) )
                {
                    ItemPosCountVec dest;
                    if( CanBankItem( INVENTORY_SLOT_BAG_0, slot, dest, item, false, false ) == EQUIP_ERR_OK )
                        item = BankItem(dest, item, true);
                    else
                        success = false;
                }

                if(success)
                {
                    // store bags that may contain items in them
                    if(item->IsBag() && IsBagPos(item->GetPos()))
                        bagMap[item_guid] = (Bag*)item;
                }
            }
            else
            {
                item->SetSlot(NULL_SLOT);
                // the item is in a bag, find the bag
                std::map<uint64, Bag*>::iterator itr = bagMap.find(bag_guid);
                if(itr != bagMap.end())
                    itr->second->StoreItem(slot, item, true );
                else
                    success = false;
            }

            // item's state may have changed after stored
            if (success)
                item->SetState(ITEM_UNCHANGED, this);
            else
            {
                sLog.outError("Player::_LoadInventory: Player %s has item (GUID: %u Entry: %u) can't be loaded to inventory (Bag GUID: %u Slot: %u) by some reason, will send by mail.", GetName(),item_guid, item_id, bag_guid, slot);
                CharacterDatabase.PExecute("DELETE FROM character_inventory WHERE item = '%u'", item_guid);
                problematicItems.push_back(item);
            }
        } while (result->NextRow());

        delete result;
        m_itemUpdateQueueBlocked = false;

        // send by mail problematic items
        while(!problematicItems.empty())
        {
            // fill mail
            MailItemsInfo mi;                               // item list preparing

            for(int i = 0; !problematicItems.empty() && i < MAX_MAIL_ITEMS; ++i)
            {
                Item* item = problematicItems.front();
                problematicItems.pop_front();

                mi.AddItem(item->GetGUIDLow(), item->GetEntry(), item);
            }

            std::string subject = GetSession()->GetTrinityString(LANG_NOT_EQUIPPED_ITEM);

            WorldSession::SendMailTo(this, MAIL_NORMAL, MAIL_STATIONERY_GM, GetGUIDLow(), GetGUIDLow(), subject, 0, &mi, 0, 0, MAIL_CHECK_MASK_NONE);
        }
    }
    //if(isAlive())
    _ApplyAllItemMods();
}

// load mailed item which should receive current player
void Player::_LoadMailedItems(Mail *mail)
{
    QueryResult* result = CharacterDatabase.PQuery("SELECT item_guid, item_template FROM mail_items WHERE mail_id='%u'", mail->messageID);
    if(!result)
        return;

    do
    {
        Field *fields = result->Fetch();
        uint32 item_guid_low = fields[0].GetUInt32();
        uint32 item_template = fields[1].GetUInt32();

        mail->AddItem(item_guid_low, item_template);

        ItemPrototype const *proto = objmgr.GetItemPrototype(item_template);

        if(!proto)
        {
            sLog.outError( "Player %u have unknown item_template (ProtoType) in mailed items(GUID: %u template: %u) in mail (%u), deleted.", GetGUIDLow(), item_guid_low, item_template,mail->messageID);
            SQLTransaction trans = CharacterDatabase.BeginTransaction();
            trans->PAppend("DELETE FROM mail_items WHERE item_guid = '%u'", item_guid_low);
            trans->PAppend("DELETE FROM item_instance WHERE guid = '%u'", item_guid_low);
            CharacterDatabase.CommitTransaction(trans);
            continue;
        }

        Item *item = NewItemOrBag(proto);

        if(!item->LoadFromDB(item_guid_low, 0))
        {
            sLog.outError( "Player::_LoadMailedItems - Item in mail (%u) doesn't exist !!!! - item guid: %u, deleted from mail", mail->messageID, item_guid_low);
            SQLTransaction trans = CharacterDatabase.BeginTransaction();
            trans->PAppend("DELETE FROM mail_items WHERE item_guid = '%u'", item_guid_low);
            item->FSetState(ITEM_REMOVED);
            item->SaveToDB(trans);                               // it also deletes item object !
            CharacterDatabase.CommitTransaction(trans);
            continue;
        }

        AddMItem(item);
    } while (result->NextRow());

    delete result;
}

void Player::_LoadMailInit(QueryResult *resultUnread, QueryResult *resultDelivery)
{
    //set a count of unread mails
    //QueryResult *resultMails = CharacterDatabase.PQuery("SELECT COUNT(id) FROM mail WHERE receiver = '%u' AND (checked & 1)=0 AND deliver_time <= '" I64FMTD "'", GUID_LOPART(playerGuid),(uint64)cTime);
    if (resultUnread)
    {
        Field *fieldMail = resultUnread->Fetch();
        unReadMails = fieldMail[0].GetUInt8();
        delete resultUnread;
    }

    // store nearest delivery time (it > 0 and if it < current then at next player update SendNewMaill will be called)
    //resultMails = CharacterDatabase.PQuery("SELECT MIN(deliver_time) FROM mail WHERE receiver = '%u' AND (checked & 1)=0", GUID_LOPART(playerGuid));
    if (resultDelivery)
    {
        Field *fieldMail = resultDelivery->Fetch();
        m_nextMailDelivereTime = (time_t)fieldMail[0].GetUInt64();
        delete resultDelivery;
    }
}

void Player::_LoadMail()
{
    m_mail.clear();
    //mails are in right order                             0  1           2      3        4       5          6         7           8            9     10  11      12         13
    QueryResult *result = CharacterDatabase.PQuery("SELECT id,messageType,sender,receiver,subject,itemTextId,has_items,expire_time,deliver_time,money,cod,checked,stationery,mailTemplateId FROM mail WHERE receiver = '%u' ORDER BY id DESC",GetGUIDLow());
    if(result)
    {
        do
        {
            Field *fields = result->Fetch();
            Mail *m = new Mail;
            m->messageID = fields[0].GetUInt32();
            m->messageType = fields[1].GetUInt8();
            m->sender = fields[2].GetUInt32();
            m->receiver = fields[3].GetUInt32();
            m->subject = fields[4].GetCppString();
            m->itemTextId = fields[5].GetUInt32();
            bool has_items = fields[6].GetBool();
            m->expire_time = (time_t)fields[7].GetUInt64();
            m->deliver_time = (time_t)fields[8].GetUInt64();
            m->money = fields[9].GetUInt32();
            m->COD = fields[10].GetUInt32();
            m->checked = fields[11].GetUInt32();
            m->stationery = fields[12].GetUInt8();
            m->mailTemplateId = fields[13].GetInt16();

            if(m->mailTemplateId && !sMailTemplateStore.LookupEntry(m->mailTemplateId))
            {
                sLog.outError( "Player::_LoadMail - Mail (%u) have not existed MailTemplateId (%u), remove at load", m->messageID, m->mailTemplateId);
                m->mailTemplateId = 0;
            }

            m->state = MAIL_STATE_UNCHANGED;

            if (has_items)
                _LoadMailedItems(m);

            m_mail.push_back(m);
        } while( result->NextRow() );
        delete result;
    }
    m_mailsLoaded = true;
}

void Player::LoadPet()
{
    //fixme: the pet should still be loaded if the player is not in world
    // just not added to the map
    if(IsInWorld())
    {
        Pet *pet = new Pet;
        if(!pet->LoadPetFromDB(this,0,0,true))
            delete pet;
    }
}

void Player::_LoadQuestStatus(QueryResult *result)
{
    mQuestStatus.clear();

    uint32 slot = 0;

    ////                                                     0      1       2         3         4      5          6          7          8          9           10          11          12
    //QueryResult *result = CharacterDatabase.PQuery("SELECT quest, status, rewarded, explored, timer, mobcount1, mobcount2, mobcount3, mobcount4, itemcount1, itemcount2, itemcount3, itemcount4 FROM character_queststatus WHERE guid = '%u'", GetGUIDLow());

    if(result)
    {
        do
        {
            Field *fields = result->Fetch();

            uint32 quest_id = fields[0].GetUInt32();
                                                            // used to be new, no delete?
            Quest const* pQuest = objmgr.GetQuestTemplate(quest_id);
            if( pQuest )
            {
                // find or create
                QuestStatusData& questStatusData = mQuestStatus[quest_id];

                uint32 qstatus = fields[1].GetUInt32();
                if(qstatus < MAX_QUEST_STATUS)
                    questStatusData.m_status = QuestStatus(qstatus);
                else
                {
                    questStatusData.m_status = QUEST_STATUS_NONE;
                    sLog.outError("Player %s have invalid quest %d status (%d), replaced by QUEST_STATUS_NONE(0).",GetName(),quest_id,qstatus);
                }

                questStatusData.m_rewarded = ( fields[2].GetUInt8() > 0 );
                questStatusData.m_explored = ( fields[3].GetUInt8() > 0 );

                time_t quest_time = time_t(fields[4].GetUInt64());

                if( pQuest->HasFlag( QUEST_TRINITY_FLAGS_TIMED ) && !GetQuestRewardStatus(quest_id) &&  questStatusData.m_status != QUEST_STATUS_NONE )
                {
                    AddTimedQuest( quest_id );

                    if (quest_time <= sWorld.GetGameTime())
                        questStatusData.m_timer = 1;
                    else
                        questStatusData.m_timer = (quest_time - sWorld.GetGameTime()) * 1000;
                }
                else
                    quest_time = 0;

                questStatusData.m_creatureOrGOcount[0] = fields[5].GetUInt32();
                questStatusData.m_creatureOrGOcount[1] = fields[6].GetUInt32();
                questStatusData.m_creatureOrGOcount[2] = fields[7].GetUInt32();
                questStatusData.m_creatureOrGOcount[3] = fields[8].GetUInt32();
                questStatusData.m_itemcount[0] = fields[9].GetUInt32();
                questStatusData.m_itemcount[1] = fields[10].GetUInt32();
                questStatusData.m_itemcount[2] = fields[11].GetUInt32();
                questStatusData.m_itemcount[3] = fields[12].GetUInt32();

                questStatusData.uState = QUEST_UNCHANGED;

                // add to quest log
                if( slot < MAX_QUEST_LOG_SIZE &&
                    ( questStatusData.m_status==QUEST_STATUS_INCOMPLETE ||
                    questStatusData.m_status==QUEST_STATUS_COMPLETE &&
                    (!questStatusData.m_rewarded || pQuest->IsDaily()) ) )
                {
                    SetQuestSlot(slot,quest_id,quest_time);

                    if(questStatusData.m_status == QUEST_STATUS_COMPLETE)
                        SetQuestSlotState(slot,QUEST_STATE_COMPLETE);

                    for(uint8 idx = 0; idx < QUEST_OBJECTIVES_COUNT; ++idx)
                        if(questStatusData.m_creatureOrGOcount[idx])
                            SetQuestSlotCounter(slot,idx,questStatusData.m_creatureOrGOcount[idx]);

                    ++slot;
                }

                if(questStatusData.m_rewarded)
                {
                    // learn rewarded spell if unknown
                    learnQuestRewardedSpells(pQuest);

                    // set rewarded title if any
                    if(pQuest->GetCharTitleId())
                    {
                        if(CharTitlesEntry const* titleEntry = sCharTitlesStore.LookupEntry(pQuest->GetCharTitleId()))
                            SetTitle(titleEntry);
                    }
                }
            }
        }
        while( result->NextRow() );

        delete result;
    }

    // clear quest log tail
    for ( uint16 i = slot; i < MAX_QUEST_LOG_SIZE; ++i )
        SetQuestSlot(i,0);
}

void Player::_LoadDailyQuestStatus(QueryResult *result)
{
    for(uint32 quest_daily_idx = 0; quest_daily_idx < PLAYER_MAX_DAILY_QUESTS; ++quest_daily_idx)
        SetUInt32Value(PLAYER_FIELD_DAILY_QUESTS_1+quest_daily_idx,0);

    //QueryResult *result = CharacterDatabase.PQuery("SELECT quest,time FROM character_queststatus_daily WHERE guid = '%u'", GetGUIDLow());

    if(result)
    {
        uint32 quest_daily_idx = 0;

        do
        {
            if(quest_daily_idx >= PLAYER_MAX_DAILY_QUESTS)  // max amount with exist data in query
            {
                sLog.outError("Player (GUID: %u) have more 25 daily quest records in `charcter_queststatus_daily`",GetGUIDLow());
                break;
            }

            Field *fields = result->Fetch();

            uint32 quest_id = fields[0].GetUInt32();

            // save _any_ from daily quest times (it must be after last reset anyway)
            m_lastDailyQuestTime = (time_t)fields[1].GetUInt64();

            Quest const* pQuest = objmgr.GetQuestTemplate(quest_id);
            if( !pQuest )
                continue;

            SetUInt32Value(PLAYER_FIELD_DAILY_QUESTS_1+quest_daily_idx,quest_id);
            ++quest_daily_idx;
        }
        while( result->NextRow() );

        delete result;
    }

    m_DailyQuestChanged = false;
}

void Player::_LoadReputation(QueryResult *result)
{
    m_factions.clear();

    // Set initial reputations (so everything is nifty before DB data load)
    SetInitialFactions();

    //QueryResult *result = CharacterDatabase.PQuery("SELECT faction,standing,flags FROM character_reputation WHERE guid = '%u'",GetGUIDLow());

    if(result)
    {
        do
        {
            Field *fields = result->Fetch();

            FactionEntry const *factionEntry = sFactionStore.LookupEntry(fields[0].GetUInt32());
            if( factionEntry && (factionEntry->reputationListID >= 0))
            {
                FactionState* faction = &m_factions[factionEntry->reputationListID];

                // update standing to current
                faction->Standing = int32(fields[1].GetUInt32());

                uint32 dbFactionFlags = fields[2].GetUInt32();

                if( dbFactionFlags & FACTION_FLAG_VISIBLE )
                    SetFactionVisible(faction);             // have internal checks for forced invisibility

                if( dbFactionFlags & FACTION_FLAG_INACTIVE)
                    SetFactionInactive(faction,true);       // have internal checks for visibility requirement

                if( dbFactionFlags & FACTION_FLAG_AT_WAR )  // DB at war
                    SetFactionAtWar(faction,true);          // have internal checks for FACTION_FLAG_PEACE_FORCED
                else                                        // DB not at war
                {
                    // allow remove if visible (and then not FACTION_FLAG_INVISIBLE_FORCED or FACTION_FLAG_HIDDEN)
                    if( faction->Flags & FACTION_FLAG_VISIBLE )
                        SetFactionAtWar(faction,false);     // have internal checks for FACTION_FLAG_PEACE_FORCED
                }

                // set atWar for hostile
                if(GetReputationRank(factionEntry) <= REP_HOSTILE)
                    SetFactionAtWar(faction,true);

                // reset changed flag if values similar to saved in DB
                if(faction->Flags==dbFactionFlags)
                    faction->Changed = false;
            }
        }
        while( result->NextRow() );

        delete result;
    }
}

void Player::_LoadSpells(QueryResult *result)
{
    for (PlayerSpellMap::iterator itr = m_spells.begin(); itr != m_spells.end(); ++itr)
        delete itr->second;
    m_spells.clear();

    //QueryResult *result = CharacterDatabase.PQuery("SELECT spell,slot,active FROM character_spell WHERE guid = '%u'",GetGUIDLow());

    if(result)
    {
        do
        {
            Field *fields = result->Fetch();

            addSpell(fields[0].GetUInt16(), fields[2].GetBool(), false, true, fields[1].GetUInt16(), fields[3].GetBool());
        }
        while( result->NextRow() );

        delete result;
    }
}

void Player::_LoadTutorials(QueryResult *result)
{
    //QueryResult *result = CharacterDatabase.PQuery("SELECT tut0,tut1,tut2,tut3,tut4,tut5,tut6,tut7 FROM character_tutorial WHERE account = '%u' AND realmid = '%u'", GetAccountId(), realmid);

    if(result)
    {
        do
        {
            Field *fields = result->Fetch();

            for (int iI=0; iI<8; iI++)
                m_Tutorials[iI] = fields[iI].GetUInt32();
        }
        while( result->NextRow() );

        delete result;
    }

    m_TutorialsChanged = false;
}

void Player::_LoadGroup(QueryResult *result)
{
    //QueryResult *result = CharacterDatabase.PQuery("SELECT leaderGuid FROM group_member WHERE memberGuid='%u'", GetGUIDLow());
    if(result)
    {
        uint64 leaderGuid = MAKE_NEW_GUID((*result)[0].GetUInt32(), 0, HIGHGUID_PLAYER);
        delete result;
        Group* group = objmgr.GetGroupByLeader(leaderGuid);
        if(group)
        {
            uint8 subgroup = group->GetMemberGroup(GetGUID());
            SetGroup(group, subgroup);
            if(getLevel() >= LEVELREQUIREMENT_HEROIC)
            {
                // the group leader may change the instance difficulty while the player is offline
                SetDifficulty(group->GetDifficulty());
            }
        }
    }
}

void Player::_LoadBoundInstances(QueryResult *result)
{
    for(uint8 i = 0; i < TOTAL_DIFFICULTIES; i++)
        m_boundInstances[i].clear();

    Group *group = GetGroup();

    //QueryResult *result = CharacterDatabase.PQuery("SELECT id, permanent, map, difficulty, resettime FROM character_instance LEFT JOIN instance ON instance = id WHERE guid = '%u'", GUID_LOPART(m_guid));
    if(result)
    {
        do
        {
            Field *fields = result->Fetch();
            bool perm = fields[1].GetBool();
            uint32 mapId = fields[2].GetUInt32();
            uint32 instanceId = fields[0].GetUInt32();
            uint8 difficulty = fields[3].GetUInt8();
            time_t resetTime = (time_t)fields[4].GetUInt64();
            // the resettime for normal instances is only saved when the InstanceSave is unloaded
            // so the value read from the DB may be wrong here but only if the InstanceSave is loaded
            // and in that case it is not used

            if(!perm && group)
            {
                sLog.outError("_LoadBoundInstances: player %s(%d) is in group %d but has a non-permanent character bind to map %d,%d,%d", GetName(), GetGUIDLow(), GUID_LOPART(group->GetLeaderGUID()), mapId, instanceId, difficulty);
                CharacterDatabase.PExecute("DELETE FROM character_instance WHERE guid = '%d' AND instance = '%d'", GetGUIDLow(), instanceId);
                continue;
            }

            // since non permanent binds are always solo bind, they can always be reset
            InstanceSave *save = sInstanceSaveManager.AddInstanceSave(mapId, instanceId, difficulty, resetTime, !perm, true);
            if(save) BindToInstance(save, perm, true);
        } while(result->NextRow());
        delete result;
    }
}

InstancePlayerBind* Player::GetBoundInstance(uint32 mapid, uint8 difficulty)
{
    // some instances only have one difficulty
    const MapEntry* entry = sMapStore.LookupEntry(mapid);
    if(!entry || !entry->SupportsHeroicMode()) difficulty = DIFFICULTY_NORMAL;

    BoundInstancesMap::iterator itr = m_boundInstances[difficulty].find(mapid);
    if(itr != m_boundInstances[difficulty].end())
        return &itr->second;
    else
        return NULL;
}

InstanceSave * Player::GetInstanceSave(uint32 mapid)
{
    InstancePlayerBind *pBind = GetBoundInstance(mapid, GetDifficulty());
    InstanceSave *pSave = pBind ? pBind->save : NULL;
    if (!pBind || !pBind->perm)
    {
        if (Group *group = GetGroup())
            if (InstanceGroupBind *groupBind = group->GetBoundInstance(mapid, GetDifficulty()))
                pSave = groupBind->save;
    }
    return pSave;
}

void Player::UnbindInstance(uint32 mapid, uint8 difficulty, bool unload)
{
    BoundInstancesMap::iterator itr = m_boundInstances[difficulty].find(mapid);
    UnbindInstance(itr, difficulty, unload);
}

void Player::UnbindInstance(BoundInstancesMap::iterator &itr, uint8 difficulty, bool unload)
{
    if(itr != m_boundInstances[difficulty].end())
    {
        if(!unload) CharacterDatabase.PExecute("DELETE FROM character_instance WHERE guid = '%u' AND instance = '%u'", GetGUIDLow(), itr->second.save->GetInstanceId());
        itr->second.save->RemovePlayer(this);               // save can become invalid
        m_boundInstances[difficulty].erase(itr++);
    }
}

InstancePlayerBind* Player::BindToInstance(InstanceSave *save, bool permanent, bool load)
{
    if(save)
    {
        InstancePlayerBind& bind = m_boundInstances[save->GetDifficulty()][save->GetMapId()];
        if(bind.save)
        {
            // update the save when the group kills a boss
            if(permanent != bind.perm || save != bind.save)
                if(!load) CharacterDatabase.PExecute("UPDATE character_instance SET instance = '%u', permanent = '%u' WHERE guid = '%u' AND instance = '%u'", save->GetInstanceId(), permanent, GetGUIDLow(), bind.save->GetInstanceId());
        }
        else
            if(!load) CharacterDatabase.PExecute("INSERT INTO character_instance (guid, instance, permanent) VALUES ('%u', '%u', '%u')", GetGUIDLow(), save->GetInstanceId(), permanent);

        if(bind.save != save)
        {
            if(bind.save) bind.save->RemovePlayer(this);
            save->AddPlayer(this);
        }

        if(permanent) save->SetCanReset(false);

        bind.save = save;
        bind.perm = permanent;
        return &bind;
    }
    else
        return NULL;
}

void Player::SendRaidInfo()
{
    WorldPacket data(SMSG_RAID_INSTANCE_INFO, 4);

    uint32 counter = 0, i;
    for(i = 0; i < TOTAL_DIFFICULTIES; i++)
        for (BoundInstancesMap::iterator itr = m_boundInstances[i].begin(); itr != m_boundInstances[i].end(); ++itr)
            if(itr->second.perm) counter++;

    data << counter;
    for(i = 0; i < TOTAL_DIFFICULTIES; i++)
    {
        for (BoundInstancesMap::iterator itr = m_boundInstances[i].begin(); itr != m_boundInstances[i].end(); ++itr)
        {
            if(itr->second.perm)
            {
                InstanceSave *save = itr->second.save;
                data << (save->GetMapId());
                data << (uint32)(save->GetResetTime() - time(NULL));
                data << save->GetInstanceId();
                data << uint32(counter);
                counter--;
            }
        }
    }
    GetSession()->SendPacket(&data);
}

/*
- called on every successful teleportation to a map
*/
void Player::SendSavedInstances()
{
    bool hasBeenSaved = false;
    WorldPacket data;

    for(uint8 i = 0; i < TOTAL_DIFFICULTIES; i++)
    {
        for (BoundInstancesMap::iterator itr = m_boundInstances[i].begin(); itr != m_boundInstances[i].end(); ++itr)
        {
            if(itr->second.perm)                                // only permanent binds are sent
            {
                hasBeenSaved = true;
                break;
            }
        }
    }

    //Send opcode 811. true or false means, whether you have current raid/heroic instances
    data.Initialize(SMSG_UPDATE_INSTANCE_OWNERSHIP);
    data << uint32(hasBeenSaved);
    GetSession()->SendPacket(&data);

    if(!hasBeenSaved)
        return;

    for(uint8 i = 0; i < TOTAL_DIFFICULTIES; i++)
    {
        for (BoundInstancesMap::iterator itr = m_boundInstances[i].begin(); itr != m_boundInstances[i].end(); ++itr)
        {
            if(itr->second.perm)
            {
                data.Initialize(SMSG_UPDATE_LAST_INSTANCE);
                data << uint32(itr->second.save->GetMapId());
                GetSession()->SendPacket(&data);
            }
        }
    }
}

/// convert the player's binds to the group
void Player::ConvertInstancesToGroup(Player *player, Group *group, uint64 player_guid)
{
    PROFILE;
    
    bool has_binds = false;
    bool has_solo = false;

    if(player) { player_guid = player->GetGUID(); if(!group) group = player->GetGroup(); }
    assert(player_guid);

    // copy all binds to the group, when changing leader it's assumed the character
    // will not have any solo binds

    if(player)
    {
        for(uint8 i = 0; i < TOTAL_DIFFICULTIES; i++)
        {
            for (BoundInstancesMap::iterator itr = player->m_boundInstances[i].begin(); itr != player->m_boundInstances[i].end();)
            {
                has_binds = true;
                if(group) group->BindToInstance(itr->second.save, itr->second.perm, true);
                // permanent binds are not removed
                if(!itr->second.perm)
                {
                    player->UnbindInstance(itr, i, true);   // increments itr
                    has_solo = true;
                }
                else
                    ++itr;
            }
        }
    }

    // if the player's not online we don't know what binds it has
    if(!player || !group || has_binds) CharacterDatabase.PExecute("INSERT INTO group_instance SELECT guid, instance, permanent FROM character_instance WHERE guid = '%u'", GUID_LOPART(player_guid));
    // the following should not get executed when changing leaders
    if(!player || has_solo) CharacterDatabase.PExecute("DELETE FROM character_instance WHERE guid = '%d' AND permanent = 0", GUID_LOPART(player_guid));
}

bool Player::Satisfy(AccessRequirement const *ar, uint32 target_map, bool report)
{
    if(!isGameMaster() && ar)
    {
        uint32 LevelMin = 0;
        if(getLevel() < ar->levelMin && !sWorld.getConfig(CONFIG_INSTANCE_IGNORE_LEVEL))
            LevelMin = ar->levelMin;

        uint32 LevelMax = 0;
        if(ar->levelMax >= ar->levelMin && getLevel() > ar->levelMax && !sWorld.getConfig(CONFIG_INSTANCE_IGNORE_LEVEL))
            LevelMax = ar->levelMax;

        uint32 missingItem = 0;
        if(ar->item)
        {
            if(!HasItemCount(ar->item, 1) &&
                (!ar->item2 || !HasItemCount(ar->item2, 1)))
                missingItem = ar->item;
        }
        else if(ar->item2 && !HasItemCount(ar->item2, 1))
            missingItem = ar->item2;

        uint32 missingKey = 0;
        uint32 missingHeroicQuest = 0;
        if(GetDifficulty() == DIFFICULTY_HEROIC)
        {
            if(ar->heroicKey)
            {
                if(!HasItemCount(ar->heroicKey, 1) &&
                    (!ar->heroicKey2 || !HasItemCount(ar->heroicKey2, 1)))
                    missingKey = ar->heroicKey;
            }
            else if(ar->heroicKey2 && !HasItemCount(ar->heroicKey2, 1))
                missingKey = ar->heroicKey2;

            if(ar->heroicQuest && !GetQuestRewardStatus(ar->heroicQuest))
                missingHeroicQuest = ar->heroicQuest;
        }

        uint32 missingQuest = 0;
        if(ar->quest && !GetQuestRewardStatus(ar->quest))
            missingQuest = ar->quest;

        if(LevelMin || LevelMax || missingItem || missingKey || missingQuest || missingHeroicQuest)
        {
            if(report)
            {
                if(missingItem)
                    GetSession()->SendAreaTriggerMessage(GetSession()->GetTrinityString(LANG_LEVEL_MINREQUIRED_AND_ITEM), ar->levelMin, objmgr.GetItemPrototype(missingItem)->Name1);
                else if(missingKey)
                    SendTransferAborted(target_map, TRANSFER_ABORT_DIFFICULTY2);
                else if(missingHeroicQuest)
                    GetSession()->SendAreaTriggerMessage(ar->heroicQuestFailedText.c_str());
                else if(missingQuest)
                    GetSession()->SendAreaTriggerMessage(ar->questFailedText.c_str());
                else if(LevelMin)
                    GetSession()->SendAreaTriggerMessage(GetSession()->GetTrinityString(LANG_LEVEL_MINREQUIRED), LevelMin);
            }
            return false;
        }
    }
    return true;
}

bool Player::_LoadHomeBind(QueryResult *result)
{
    PlayerInfo const *info = objmgr.GetPlayerInfo(getRace(), getClass());
    if (!info)
    {
        sLog.outError("Player have incorrect race/class pair. Can't be loaded.");
        return false;
    }

    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    bool ok = false;
    //QueryResult *result = CharacterDatabase.PQuery("SELECT map,zone,position_x,position_y,position_z FROM character_homebind WHERE guid = '%u'", GUID_LOPART(playerGuid));
    if (result)
    {
        Field *fields = result->Fetch();
        m_homebindMapId = fields[0].GetUInt32();
        m_homebindAreaId = fields[1].GetUInt16();
        m_homebindX = fields[2].GetFloat();
        m_homebindY = fields[3].GetFloat();
        m_homebindZ = fields[4].GetFloat();
        delete result;

        // accept saved data only for valid position (and non instanceable)
        if( MapManager::IsValidMapCoord(m_homebindMapId,m_homebindX,m_homebindY,m_homebindZ) &&
            !sMapStore.LookupEntry(m_homebindMapId)->Instanceable() )
        {
            ok = true;
        }
        else
            trans->PAppend("DELETE FROM character_homebind WHERE guid = '%u'", GetGUIDLow());
    }

    if(!ok)
    {
        m_homebindMapId = info->mapId;
        m_homebindAreaId = info->areaId;
        m_homebindX = info->positionX;
        m_homebindY = info->positionY;
        m_homebindZ = info->positionZ;

        trans->PAppend("INSERT INTO character_homebind (guid,map,zone,position_x,position_y,position_z) VALUES ('%u', '%u', '%u', '%f', '%f', '%f')", GetGUIDLow(), m_homebindMapId, (uint32)m_homebindAreaId, m_homebindX, m_homebindY, m_homebindZ);
    }
    CharacterDatabase.CommitTransaction(trans);

    DEBUG_LOG("Setting player home position: mapid is: %u, zoneid is %u, X is %f, Y is %f, Z is %f",
        m_homebindMapId, m_homebindAreaId, m_homebindX, m_homebindY, m_homebindZ);

    return true;
}

/*********************************************************/
/***                   SAVE SYSTEM                     ***/
/*********************************************************/

//TODO Transaction
void Player::SaveToDB()
{
    // delay auto save at any saves (manual, in code, or autosave)
    m_nextSave = sWorld.getConfig(CONFIG_INTERVAL_SAVE);

    // first save/honor gain after midnight will also update the player's honor fields
    UpdateHonorFields();

    uint32 mapid = IsBeingTeleported() ? GetTeleportDest().m_mapId : GetMapId();
    const MapEntry * me = sMapStore.LookupEntry(mapid);
    // players aren't saved on arena maps
    if(!me || me->IsBattleArena())
        return;

    int is_save_resting = HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_RESTING) ? 1 : 0;
                                                            //save, far from tavern/city
                                                            //save, but in tavern/city

    // save state (after auras removing), if aura remove some flags then it must set it back by self)
    uint32 tmp_bytes = GetUInt32Value(UNIT_FIELD_BYTES_1);
    uint32 tmp_bytes2 = GetUInt32Value(UNIT_FIELD_BYTES_2);
    uint32 tmp_flags = GetUInt32Value(UNIT_FIELD_FLAGS);
    uint32 tmp_pflags = GetUInt32Value(PLAYER_FLAGS);
    uint32 tmp_displayid = GetDisplayId();

    // Set player sit state to standing on save, also stealth and shifted form
    SetByteValue(UNIT_FIELD_BYTES_1, 0, 0);                 // stand state
    SetByteValue(UNIT_FIELD_BYTES_2, 3, 0);                 // shapeshift
    SetByteValue(UNIT_FIELD_BYTES_1, 3, 0);                 // stand flags?
    RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_ROTATE);
    SetDisplayId(GetNativeDisplayId());

    bool inworld = IsInWorld();

    std::string sql_name = m_name;
    CharacterDatabase.escape_string(sql_name);

    uint32 pflags = GetUInt32Value(PLAYER_FLAGS);
    pflags &= ~PLAYER_FLAGS_COMMENTATOR;
    pflags &= ~PLAYER_FLAGS_COMMENTATOR_UBER;

    std::ostringstream ss;
    ss << "REPLACE INTO characters (guid,account,name,race,class,gender, level, xp, money, playerBytes, playerBytes2, playerFlags,"
        "map, instance_id, dungeon_difficulty, position_x, position_y, position_z, orientation, data, "
        "taximask, online, cinematic, "
        "totaltime, leveltime, rest_bonus, logout_time, is_logout_resting, resettalents_cost, resettalents_time, "
        "trans_x, trans_y, trans_z, trans_o, transguid, extra_flags, stable_slots, at_login, zone, "
        "death_expire_time, taxi_path, arena_pending_points, arenapoints, totalHonorPoints, todayHonorPoints, yesterdayHonorPoints, "
        "totalKills, todayKills, yesterdayKills, chosenTitle, watchedFaction, drunk, health, power1, power2, power3, power4, power5, latency, "
        "exploredZones, equipmentCache, ammoId, knownTitles, actionBars, xp_blocked, lastGenderChange) VALUES ("
        << GetGUIDLow() << ", "
        << GetSession()->GetAccountId() << ", '"
        << sql_name << "', "
        << uint32(m_race) << ", "
        << uint32(m_class) << ", "
        << uint32(m_gender) << ", "
        << getLevel() << ", "
        << GetUInt32Value(PLAYER_XP) << ", "
        << GetMoney() << ", "
        << GetUInt32Value(PLAYER_BYTES) << ", "
        << GetUInt32Value(PLAYER_BYTES_2) << ", "
        << pflags << ", ";

    if(!IsBeingTeleported())
    {
        ss << GetMapId() << ", "
        << (uint32)GetInstanceId() << ", "
        << (uint32)GetDifficulty() << ", "
        << finiteAlways(GetPositionX()) << ", "
        << finiteAlways(GetPositionY()) << ", "
        << finiteAlways(GetPositionZ()) << ", "
        << finiteAlways(GetOrientation()) << ", '";
    }
    else
    {
        ss << GetTeleportDest().m_mapId << ", "
        << (uint32)0 << ", "
        << (uint32)GetDifficulty() << ", "
        << finiteAlways(GetTeleportDest().m_positionX) << ", "
        << finiteAlways(GetTeleportDest().m_positionY) << ", "
        << finiteAlways(GetTeleportDest().m_positionZ) << ", "
        << finiteAlways(GetTeleportDest().m_orientation) << ", '";
    }

    uint16 i;
    for( i = 0; i < m_valuesCount; i++ )
    {
        ss << GetUInt32Value(i) << " ";
    }

    ss << "', '";

    for( i = 0; i < 8; i++ )
        ss << m_taxi.GetTaximask(i) << " ";

    ss << "', ";
    ss << (inworld ? 1 : 0);

    ss << ", ";
    ss << m_cinematic;

    ss << ", ";
    ss << m_Played_time[0];
    ss << ", ";
    ss << m_Played_time[1];

    ss << ", ";
    ss << finiteAlways(m_rest_bonus);
    ss << ", ";
    ss << (uint64)time(NULL);
    ss << ", ";
    ss << is_save_resting;
    ss << ", ";
    ss << m_resetTalentsCost;
    ss << ", ";
    ss << (uint64)m_resetTalentsTime;

    ss << ", ";
    ss << finiteAlways(m_movementInfo.t_x);
    ss << ", ";
    ss << finiteAlways(m_movementInfo.t_y);
    ss << ", ";
    ss << finiteAlways(m_movementInfo.t_z);
    ss << ", ";
    ss << finiteAlways(m_movementInfo.t_o);
    ss << ", ";
    if (m_transport)
        ss << m_transport->GetGUIDLow();
    else
        ss << "0";

    ss << ", ";
    ss << m_ExtraFlags;

    ss << ", ";
    ss << uint32(m_stableSlots);                            // to prevent save uint8 as char

    ss << ", ";
    ss << uint32(m_atLoginFlags);

    ss << ", ";
    ss << GetZoneId();

    ss << ", ";
    ss << (uint64)m_deathExpireTime;

    ss << ", '";
    ss << m_taxi.SaveTaxiDestinationsToString();

    ss << "', '0', ";
    ss << GetArenaPoints();
    ss << ", ";
    ss << GetHonorPoints() << ", ";
    ss << GetUInt32Value(PLAYER_FIELD_TODAY_CONTRIBUTION) << ", ";
    ss << GetUInt32Value(PLAYER_FIELD_YESTERDAY_CONTRIBUTION) << ", ";
    ss << GetUInt32Value(PLAYER_FIELD_LIFETIME_HONORABLE_KILLS) << ", ";
    ss << uint32(GetUInt16Value(PLAYER_FIELD_KILLS, 0)) << ", ";
    ss << uint32(GetUInt16Value(PLAYER_FIELD_KILLS, 1)) << ", ";
    ss << GetUInt32Value(PLAYER_CHOSEN_TITLE) << ", ";
    ss << GetUInt32Value(PLAYER_FIELD_WATCHED_FACTION_INDEX) << ", ";
    ss << (uint16)(GetUInt32Value(PLAYER_BYTES_3) & 0xFFFE) << ", ";
    ss << GetHealth();
    for (uint32 i = 0; i < MAX_POWERS; ++i)
        ss << ", " << GetPower(Powers(i));
    ss << ", '";
    ss << GetSession()->GetLatency();
    ss << "', '";
    // EXPLORED_ZONES
    for (uint32 i = 0; i < 128; ++i)
        ss << GetUInt32Value(PLAYER_EXPLORED_ZONES_1 + i) << " ";
    ss << "', '";
    for (uint32 i = 0; i < 304; ++i) {
        if (i%16 == 2 || i%16 == 3)
            ss << GetUInt32Value(PLAYER_VISIBLE_ITEM_1_CREATOR + i) << " ";
    }
    ss << "', ";
    ss << GetUInt32Value(PLAYER_AMMO_ID) << ", '";
    // Known titles
    for (uint32 i = 0; i < 2; ++i)
        ss << GetUInt32Value(PLAYER_FIELD_KNOWN_TITLES + i) << " ";
    ss << "', '";
    ss << uint32(GetByteValue(PLAYER_FIELD_BYTES, 2));
    ss << "', '";
    ss << m_isXpBlocked;
    ss << "', '";
    ss << m_lastGenderChange;
    ss << "' )";

    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    trans->Append( ss.str().c_str() );

    if(m_mailsUpdated) {                                     //save mails only when needed
        _SaveMail(trans);
    }

    // TODO add Transaction
    _SaveBattleGroundCoord(trans);
    _SaveInventory(trans);
    _SaveQuestStatus(trans);
    _SaveDailyQuestStatus(trans);
    _SaveTutorials(trans);
    _SaveSpells(trans);
    _SaveSpellCooldowns(trans);
    _SaveActions(trans);
    _SaveAuras(trans);
    _SaveSkills(trans);
    _SaveReputation(trans);

    CharacterDatabase.CommitTransaction(trans);

    // restore state (before aura apply, if aura remove flag then aura must set it ack by self)
    SetDisplayId(tmp_displayid);
    SetUInt32Value(UNIT_FIELD_BYTES_1, tmp_bytes);
    SetUInt32Value(UNIT_FIELD_BYTES_2, tmp_bytes2);
    SetUInt32Value(UNIT_FIELD_FLAGS, tmp_flags);
    SetUInt32Value(PLAYER_FLAGS, tmp_pflags);

    // save pet (hunter pet level and experience and all type pets health/mana).
    if(Pet* pet = GetPet())
        pet->SavePetToDB(PET_SAVE_AS_CURRENT);
}

// fast save function for item/money cheating preventing - save only inventory and money state
void Player::SaveInventoryAndGoldToDB(SQLTransaction trans)
{
    _SaveInventory(trans);
    SaveGoldToDB(trans);
}

void Player::SaveGoldToDB(SQLTransaction trans)
{
    trans->PAppend("UPDATE characters SET money = '%u' WHERE guid = '%u'", GetMoney(), GetGUIDLow());
}

void Player::_SaveActions(SQLTransaction trans)
{
    for(ActionButtonList::iterator itr = m_actionButtons.begin(); itr != m_actionButtons.end(); )
    {
        switch (itr->second.uState)
        {
            case ACTIONBUTTON_NEW:
                trans->PAppend("INSERT INTO character_action (guid,button,action,type,misc) VALUES ('%u', '%u', '%u', '%u', '%u')",
                    GetGUIDLow(), (uint32)itr->first, (uint32)itr->second.action, (uint32)itr->second.type, (uint32)itr->second.misc );
                itr->second.uState = ACTIONBUTTON_UNCHANGED;
                ++itr;
                break;
            case ACTIONBUTTON_CHANGED:
                trans->PAppend("UPDATE character_action  SET action = '%u', type = '%u', misc= '%u' WHERE guid= '%u' AND button= '%u' ",
                    (uint32)itr->second.action, (uint32)itr->second.type, (uint32)itr->second.misc, GetGUIDLow(), (uint32)itr->first );
                itr->second.uState = ACTIONBUTTON_UNCHANGED;
                ++itr;
                break;
            case ACTIONBUTTON_DELETED:
                trans->PAppend("DELETE FROM character_action WHERE guid = '%u' and button = '%u'", GetGUIDLow(), (uint32)itr->first );
                m_actionButtons.erase(itr++);
                break;
            default:
                ++itr;
                break;
        };
    }
}

void Player::_SaveAuras(SQLTransaction trans)
{
    trans->PAppend("DELETE FROM character_aura WHERE guid = '%u'",GetGUIDLow());

    AuraMap const& auras = GetAuras();

    if (auras.empty())
        return;

    spellEffectPair lastEffectPair = auras.begin()->first;
    uint32 stackCounter = 1;

    for(AuraMap::const_iterator itr = auras.begin(); ; ++itr)
    {
        if(itr == auras.end() || lastEffectPair != itr->first)
        {
            AuraMap::const_iterator itr2 = itr;
            // save previous spellEffectPair to db
            itr2--;
            SpellEntry const *spellInfo = itr2->second->GetSpellProto();

            //skip all auras from spells that are passive or need a shapeshift
            if (!(itr2->second->IsPassive() || itr2->second->IsRemovedOnShapeLost()))
            {
                //do not save single target auras (unless they were cast by the player)
                if (!(itr2->second->GetCasterGUID() != GetGUID() && IsSingleTargetSpell(spellInfo)))
                {
                    uint8 i;
                    // or apply at cast SPELL_AURA_MOD_SHAPESHIFT or SPELL_AURA_MOD_STEALTH auras
                    for (i = 0; i < 3; i++)
                        if (spellInfo->EffectApplyAuraName[i] == SPELL_AURA_MOD_SHAPESHIFT ||
                        spellInfo->EffectApplyAuraName[i] == SPELL_AURA_MOD_STEALTH)
                            break;

                    if (i == 3)
                    {
                        trans->PAppend("INSERT INTO character_aura (guid,caster_guid,spell,effect_index,stackcount,amount,maxduration,remaintime,remaincharges) "
                            "VALUES ('%u', '" I64FMTD "' ,'%u', '%u', '%u', '%d', '%d', '%d', '%d')",
                            GetGUIDLow(), itr2->second->GetCasterGUID(), (uint32)itr2->second->GetId(), (uint32)itr2->second->GetEffIndex(), (uint32)itr2->second->GetStackAmount(), itr2->second->GetModifier()->m_amount,int(itr2->second->GetAuraMaxDuration()),int(itr2->second->GetAuraDuration()),int(itr2->second->m_procCharges));
                    }
                }
            }

            if(itr == auras.end())
                break;
        }

        //TODO: if need delete this
        if (lastEffectPair == itr->first)
            stackCounter++;
        else
        {
            lastEffectPair = itr->first;
            stackCounter = 1;
        }
    }
}

void Player::_SaveBattleGroundCoord(SQLTransaction trans)
{
    trans->PAppend("DELETE FROM character_bgcoord WHERE guid = '%u'", GetGUIDLow());

    // don't save if not needed
    if(!InBattleGround())
        return;

    std::ostringstream ss;
    ss << "INSERT INTO character_bgcoord (guid, bgid, bgteam, bgmap, bgx,"
        "bgy, bgz, bgo) VALUES ("
        << GetGUIDLow() << ", ";
    ss << GetBattleGroundId();
    ss << ", ";
    ss << GetBGTeam();
    ss << ", ";
    ss << GetBattleGroundEntryPointMap() << ", "
        << finiteAlways(GetBattleGroundEntryPointX()) << ", "
        << finiteAlways(GetBattleGroundEntryPointY()) << ", "
        << finiteAlways(GetBattleGroundEntryPointZ()) << ", "
        << finiteAlways(GetBattleGroundEntryPointO());
    ss << ")";

    trans->Append( ss.str().c_str() );
}

void Player::_SaveInventory(SQLTransaction trans)
{
    // force items in buyback slots to new state
    // and remove those that aren't already
    for (uint8 i = BUYBACK_SLOT_START; i < BUYBACK_SLOT_END; i++)
    {
        Item *item = m_items[i];
        if (!item || item->GetState() == ITEM_NEW) continue;
        trans->PAppend("DELETE FROM character_inventory WHERE item = '%u'", item->GetGUIDLow());
        trans->PAppend("DELETE FROM item_instance WHERE guid = '%u'", item->GetGUIDLow());
        m_items[i]->FSetState(ITEM_NEW);
    }

    // update enchantment durations
    for(EnchantDurationList::iterator itr = m_enchantDuration.begin();itr != m_enchantDuration.end();++itr)
    {
        itr->item->SetEnchantmentDuration(itr->slot,itr->leftduration);
    }

    // if no changes
    if (m_itemUpdateQueue.empty()) return;

    // do not save if the update queue is corrupt
    bool error = false;
    bool dup = false;
    for(size_t i = 0; i < m_itemUpdateQueue.size(); i++)
    {
        Item *item = m_itemUpdateQueue[i];
        if(!item || item->GetState() == ITEM_REMOVED) continue;
        Item *test = GetItemByPos( item->GetBagSlot(), item->GetSlot());

        if (test == NULL)
        {
            sLog.outError("POSSIBLE ITEM DUPLICATION ATTEMPT: Player(GUID: %u Name: %s)::_SaveInventory - the bag(%d) and slot(%d) values for the item with guid %d are incorrect, the player doesn't have an item at that position!", GetGUIDLow(), GetName(), item->GetBagSlot(), item->GetSlot(), item->GetGUIDLow());
            error = true;
            //dup = true;
        }
        else if (test != item)
        {
            sLog.outError("Player(GUID: %u Name: %s)::_SaveInventory - the bag(%d) and slot(%d) values for the item with guid %d are incorrect, the item with guid %d is there instead!", GetGUIDLow(), GetName(), item->GetBagSlot(), item->GetSlot(), item->GetGUIDLow(), test->GetGUIDLow());
            error = true;
        }
    }

    if (error)
    {
        sLog.outError("Player::_SaveInventory - one or more errors occurred save aborted!");
        ChatHandler(this).SendSysMessage(LANG_ITEM_SAVE_FAILED);
        m_itemUpdateQueue.clear();
        if (dup) {
            std::string banuname; 
            QueryResult* result = LoginDatabase.PQuery("SELECT username FROM account WHERE id = '%u'", m_session->GetAccountId());
            if (result) {
                Field* fields = result->Fetch();
                banuname = fields[0].GetCppString();
                sWorld.BanAccount(BAN_ACCOUNT, banuname, "0", "auto ban on dupe", "Big Brother");
                delete result;
            }
        }

        return;
    }

    for(size_t i = 0; i < m_itemUpdateQueue.size(); i++)
    {
        Item *item = m_itemUpdateQueue[i];
        if(!item) continue;

        Bag *container = item->GetContainer();
        uint32 bag_guid = container ? container->GetGUIDLow() : 0;

        switch(item->GetState())
        {
            case ITEM_NEW:
                trans->PAppend("INSERT INTO character_inventory (guid,bag,slot,item,item_template) VALUES ('%u', '%u', '%u', '%u', '%u')", GetGUIDLow(), bag_guid, item->GetSlot(), item->GetGUIDLow(), item->GetEntry());
                break;
            case ITEM_CHANGED:
                trans->PAppend("UPDATE character_inventory SET guid='%u', bag='%u', slot='%u', item_template='%u' WHERE item='%u'", GetGUIDLow(), bag_guid, item->GetSlot(), item->GetEntry(), item->GetGUIDLow());
                break;
            case ITEM_REMOVED:
                trans->PAppend("DELETE FROM character_inventory WHERE item = '%u'", item->GetGUIDLow());
                break;
            case ITEM_UNCHANGED:
                break;
        }

        item->SaveToDB(trans);                                   // item have unchanged inventory record and can be save standalone
    }
    m_itemUpdateQueue.clear();
}

void Player::_SaveMail(SQLTransaction trans)
{
    if (!m_mailsLoaded)
        return;

    for (PlayerMails::iterator itr = m_mail.begin(); itr != m_mail.end(); ++itr)
    {
        Mail *m = (*itr);
        if (m->state == MAIL_STATE_CHANGED)
        {
            trans->PAppend("UPDATE mail SET itemTextId = '%u',has_items = '%u',expire_time = '" I64FMTD "', deliver_time = '" I64FMTD "',money = '%u',cod = '%u',checked = '%u' WHERE id = '%u'",
                m->itemTextId, m->HasItems() ? 1 : 0, (uint64)m->expire_time, (uint64)m->deliver_time, m->money, m->COD, m->checked, m->messageID);
            if(m->removedItems.size())
            {
                for(std::vector<uint32>::iterator itr2 = m->removedItems.begin(); itr2 != m->removedItems.end(); ++itr2)
                    trans->PAppend("DELETE FROM mail_items WHERE item_guid = '%u'", *itr2);
                m->removedItems.clear();
            }
            m->state = MAIL_STATE_UNCHANGED;
        }
        else if (m->state == MAIL_STATE_DELETED)
        {
            if (m->HasItems())
                for(std::vector<MailItemInfo>::iterator itr2 = m->items.begin(); itr2 != m->items.end(); ++itr2)
                    trans->PAppend("DELETE FROM item_instance WHERE guid = '%u'", itr2->item_guid);
            if (m->itemTextId)
                trans->PAppend("DELETE FROM item_text WHERE id = '%u'", m->itemTextId);
            trans->PAppend("DELETE FROM mail WHERE id = '%u'", m->messageID);
            trans->PAppend("DELETE FROM mail_items WHERE mail_id = '%u'", m->messageID);
        }
    }

    //deallocate deleted mails...
    for (PlayerMails::iterator itr = m_mail.begin(); itr != m_mail.end(); )
    {
        if ((*itr)->state == MAIL_STATE_DELETED)
        {
            Mail* m = *itr;
            m_mail.erase(itr);
            delete m;
            itr = m_mail.begin();
        }
        else
            ++itr;
    }

    m_mailsUpdated = false;
}

void Player::_SaveQuestStatus(SQLTransaction trans)
{
    for( QuestStatusMap::iterator i = mQuestStatus.begin( ); i != mQuestStatus.end( ); ++i )
    {
        switch (i->second.uState)
        {
            case QUEST_NEW :
                trans->PAppend("INSERT INTO character_queststatus (guid,quest,status,rewarded,explored,timer,mobcount1,mobcount2,mobcount3,mobcount4,itemcount1,itemcount2,itemcount3,itemcount4) "
                    "VALUES ('%u', '%u', '%u', '%u', '%u', '" I64FMTD "', '%u', '%u', '%u', '%u', '%u', '%u', '%u', '%u')",
                    GetGUIDLow(), i->first, i->second.m_status, i->second.m_rewarded, i->second.m_explored, uint64(i->second.m_timer / 1000 + sWorld.GetGameTime()), i->second.m_creatureOrGOcount[0], i->second.m_creatureOrGOcount[1], i->second.m_creatureOrGOcount[2], i->second.m_creatureOrGOcount[3], i->second.m_itemcount[0], i->second.m_itemcount[1], i->second.m_itemcount[2], i->second.m_itemcount[3]);
                break;
            case QUEST_CHANGED :
                trans->PAppend("UPDATE character_queststatus SET status = '%u',rewarded = '%u',explored = '%u',timer = '" I64FMTD "',mobcount1 = '%u',mobcount2 = '%u',mobcount3 = '%u',mobcount4 = '%u',itemcount1 = '%u',itemcount2 = '%u',itemcount3 = '%u',itemcount4 = '%u'  WHERE guid = '%u' AND quest = '%u' ",
                    i->second.m_status, i->second.m_rewarded, i->second.m_explored, uint64(i->second.m_timer / 1000 + sWorld.GetGameTime()), i->second.m_creatureOrGOcount[0], i->second.m_creatureOrGOcount[1], i->second.m_creatureOrGOcount[2], i->second.m_creatureOrGOcount[3], i->second.m_itemcount[0], i->second.m_itemcount[1], i->second.m_itemcount[2], i->second.m_itemcount[3], GetGUIDLow(), i->first );
                break;
            case QUEST_UNCHANGED:
                break;
        };
        i->second.uState = QUEST_UNCHANGED;
    }
}

void Player::_SaveDailyQuestStatus(SQLTransaction trans)
{
    if(!m_DailyQuestChanged)
        return;

    m_DailyQuestChanged = false;

    // save last daily quest time for all quests: we need only mostly reset time for reset check anyway
    trans->PAppend("DELETE FROM character_queststatus_daily WHERE guid = '%u'",GetGUIDLow());
    for(uint32 quest_daily_idx = 0; quest_daily_idx < PLAYER_MAX_DAILY_QUESTS; ++quest_daily_idx)
        if(GetUInt32Value(PLAYER_FIELD_DAILY_QUESTS_1+quest_daily_idx))
            trans->PAppend("INSERT INTO character_queststatus_daily (guid,quest,time) VALUES ('%u', '%u','" I64FMTD "')",
                GetGUIDLow(), GetUInt32Value(PLAYER_FIELD_DAILY_QUESTS_1+quest_daily_idx),uint64(m_lastDailyQuestTime));
}

void Player::_SaveSkills(SQLTransaction trans)
{
    for( SkillStatusMap::iterator itr = mSkillStatus.begin(); itr != mSkillStatus.end(); )
    {
        if(itr->second.uState == SKILL_UNCHANGED)
        {
            ++itr;
            continue;
        }

        if(itr->second.uState == SKILL_DELETED)
        {
            trans->PAppend("DELETE FROM character_skills WHERE guid = '%u' AND skill = '%u' ", GetGUIDLow(), itr->first );
            mSkillStatus.erase(itr++);
            continue;
        }

        uint32 valueData = GetUInt32Value(PLAYER_SKILL_VALUE_INDEX(itr->second.pos));
        uint16 value = SKILL_VALUE(valueData);
        uint16 max = SKILL_MAX(valueData);

        switch (itr->second.uState)
        {
            case SKILL_NEW:
                trans->PAppend("INSERT INTO character_skills (guid, skill, value, max) VALUES ('%u', '%u', '%u', '%u')",
                    GetGUIDLow(), itr->first, value, max);
                break;
            case SKILL_CHANGED:
                trans->PAppend("UPDATE character_skills SET value = '%u',max = '%u'WHERE guid = '%u' AND skill = '%u' ",
                    value, max, GetGUIDLow(), itr->first );
                break;
        };
        itr->second.uState = SKILL_UNCHANGED;

        ++itr;
    }
}

void Player::_SaveReputation(SQLTransaction trans)
{
    for(FactionStateList::iterator itr = m_factions.begin(); itr != m_factions.end(); ++itr)
    {
        if (itr->second.Changed)
        {
            trans->PAppend("DELETE FROM character_reputation WHERE guid = '%u' AND faction='%u'", GetGUIDLow(), itr->second.ID);
            if (!itr->second.Deleted)
                trans->PAppend("INSERT INTO character_reputation (guid,faction,standing,flags) VALUES ('%u', '%u', '%i', '%u')", GetGUIDLow(), itr->second.ID, itr->second.Standing, itr->second.Flags);
            itr->second.Changed = false;
        }
    }
}

void Player::_SaveSpells(SQLTransaction trans)
{
    for (PlayerSpellMap::const_iterator itr = m_spells.begin(), next = m_spells.begin(); itr != m_spells.end(); itr = next)
    {
        ++next;
        if (itr->second->state == PLAYERSPELL_REMOVED || itr->second->state == PLAYERSPELL_CHANGED)
            trans->PAppend("DELETE FROM character_spell WHERE guid = '%u' and spell = '%u'", GetGUIDLow(), itr->first);
        if (itr->second->state == PLAYERSPELL_NEW || itr->second->state == PLAYERSPELL_CHANGED)
            trans->PAppend("INSERT INTO character_spell (guid,spell,slot,active,disabled) VALUES ('%u', '%u', '%u','%u','%u')", GetGUIDLow(), itr->first, itr->second->slotId,itr->second->active ? 1 : 0,itr->second->disabled ? 1 : 0);

        if (itr->second->state == PLAYERSPELL_REMOVED)
            _removeSpell(itr->first);
        else
            itr->second->state = PLAYERSPELL_UNCHANGED;
    }
}

void Player::_SaveTutorials(SQLTransaction trans)
{
    if(!m_TutorialsChanged)
        return;

    uint32 Rows=0;
    // it's better than rebuilding indexes multiple times
    QueryResult *result = CharacterDatabase.PQuery("SELECT count(*) AS r FROM character_tutorial WHERE account = '%u' AND realmid = '%u'", GetSession()->GetAccountId(), realmID );
    if(result)
    {
        Rows = result->Fetch()[0].GetUInt32();
        delete result;
    }

    if (Rows)
    {
        trans->PAppend("UPDATE character_tutorial SET tut0='%u', tut1='%u', tut2='%u', tut3='%u', tut4='%u', tut5='%u', tut6='%u', tut7='%u' WHERE account = '%u' AND realmid = '%u'",
            m_Tutorials[0], m_Tutorials[1], m_Tutorials[2], m_Tutorials[3], m_Tutorials[4], m_Tutorials[5], m_Tutorials[6], m_Tutorials[7], GetSession()->GetAccountId(), realmID );
    }
    else
    {
        trans->PAppend("INSERT INTO character_tutorial (account,realmid,tut0,tut1,tut2,tut3,tut4,tut5,tut6,tut7) VALUES ('%u', '%u', '%u', '%u', '%u', '%u', '%u', '%u', '%u', '%u')", GetSession()->GetAccountId(), realmID, m_Tutorials[0], m_Tutorials[1], m_Tutorials[2], m_Tutorials[3], m_Tutorials[4], m_Tutorials[5], m_Tutorials[6], m_Tutorials[7]);
    };

    m_TutorialsChanged = false;
}

/*********************************************************/
/***               FLOOD FILTER SYSTEM                 ***/
/*********************************************************/

void Player::UpdateSpeakTime()
{
    // ignore chat spam protection for GMs in any mode
    if(GetSession()->GetSecurity() > SEC_PLAYER)
        return;

    time_t current = time (NULL);
    if(m_speakTime > current)
    {
        uint32 max_count = sWorld.getConfig(CONFIG_CHATFLOOD_MESSAGE_COUNT);
        if(!max_count)
            return;

        ++m_speakCount;
        if(m_speakCount >= max_count)
        {
            // prevent overwrite mute time, if message send just before mutes set, for example.
            time_t new_mute = current + sWorld.getConfig(CONFIG_CHATFLOOD_MUTE_TIME);
            if(GetSession()->m_muteTime < new_mute)
                GetSession()->m_muteTime = new_mute;

            m_speakCount = 0;
        }
    }
    else
        m_speakCount = 0;

    m_speakTime = current + sWorld.getConfig(CONFIG_CHATFLOOD_MESSAGE_DELAY);
}

bool Player::CanSpeak() const
{
    return  GetSession()->m_muteTime <= time (NULL);
}

/*********************************************************/
/***              LOW LEVEL FUNCTIONS:Notifiers        ***/
/*********************************************************/

void Player::SendAttackSwingNotInRange()
{
    WorldPacket data(SMSG_ATTACKSWING_NOTINRANGE, 0);
    GetSession()->SendPacket( &data );
}

// SQLTransaction here?
void Player::SavePositionInDB(uint32 mapid, float x,float y,float z,float o,uint32 zone,uint64 guid)
{
    std::ostringstream ss;
    ss << "UPDATE characters SET position_x='"<<x<<"',position_y='"<<y
        << "',position_z='"<<z<<"',orientation='"<<o<<"',map='"<<mapid
        << "',zone='"<<zone<<"',trans_x='0',trans_y='0',trans_z='0',"
        << "transguid='0',taxi_path='' WHERE guid='"<< GUID_LOPART(guid) <<"'";

    CharacterDatabase.Execute(ss.str().c_str());
}

void Player::SaveDataFieldToDB()
{
    std::ostringstream ss;
    ss<<"UPDATE characters SET data='";

    for(uint16 i = 0; i < m_valuesCount; i++ )
    {
        ss << GetUInt32Value(i) << " ";
    }
    ss<<"' WHERE guid='"<< GUID_LOPART(GetGUIDLow()) <<"'";

    CharacterDatabase.Execute(ss.str().c_str());
}

bool Player::SaveValuesArrayInDB(Tokens const& tokens, uint64 guid)
{
    std::ostringstream ss2;
    ss2<<"UPDATE characters SET data='";
    int i=0;
    for (Tokens::const_iterator iter = tokens.begin(); iter != tokens.end(); ++iter, ++i)
    {
        ss2<<tokens[i]<<" ";
    }
    ss2<<"' WHERE guid='"<< GUID_LOPART(guid) <<"'";

    CharacterDatabase.Execute(ss2.str().c_str());
    
    return true;
}

void Player::SetUInt32ValueInArray(Tokens& tokens,uint16 index, uint32 value)
{
    char buf[11];
    snprintf(buf,11,"%u",value);

    if(index >= tokens.size())
        return;

    tokens[index] = buf;
}

void Player::SetUInt32ValueInDB(uint16 index, uint32 value, uint64 guid)
{
    Tokens tokens;
    if(!LoadValuesArrayFromDB(tokens,guid))
        return;

    if(index >= tokens.size())
        return;

    char buf[11];
    snprintf(buf,11,"%u",value);
    tokens[index] = buf;

    SaveValuesArrayInDB(tokens,guid);
}

void Player::SetFloatValueInDB(uint16 index, float value, uint64 guid)
{
    uint32 temp;
    memcpy(&temp, &value, sizeof(value));
    Player::SetUInt32ValueInDB(index, temp, guid);
}

void Player::SendAttackSwingNotStanding()
{
    WorldPacket data(SMSG_ATTACKSWING_NOTSTANDING, 0);
    GetSession()->SendPacket( &data );
}

void Player::SendAttackSwingDeadTarget()
{
    WorldPacket data(SMSG_ATTACKSWING_DEADTARGET, 0);
    GetSession()->SendPacket( &data );
}

void Player::SendAttackSwingCantAttack()
{
    WorldPacket data(SMSG_ATTACKSWING_CANT_ATTACK, 0);
    GetSession()->SendPacket( &data );
}

void Player::SendAttackSwingCancelAttack()
{
    WorldPacket data(SMSG_CANCEL_COMBAT, 0);
    GetSession()->SendPacket( &data );
}

void Player::SendAttackSwingBadFacingAttack()
{
    WorldPacket data(SMSG_ATTACKSWING_BADFACING, 0);
    GetSession()->SendPacket( &data );
}

void Player::SendAutoRepeatCancel()
{
    WorldPacket data(SMSG_CANCEL_AUTO_REPEAT, 0);
    GetSession()->SendPacket( &data );
}

void Player::PlaySound(uint32 Sound, bool OnlySelf)
{
    WorldPacket data(SMSG_PLAY_SOUND, 4);
    data << Sound;
    if (OnlySelf)
        GetSession()->SendPacket( &data );
    else
        SendMessageToSet( &data, true );
}

void Player::SendExplorationExperience(uint32 Area, uint32 Experience)
{
    WorldPacket data( SMSG_EXPLORATION_EXPERIENCE, 8 );
    data << Area;
    data << Experience;
    GetSession()->SendPacket(&data);
}

void Player::SendDungeonDifficulty(bool IsInGroup)
{
    uint8 val = 0x00000001;
    WorldPacket data(MSG_SET_DUNGEON_DIFFICULTY, 12);
    data << (uint32)GetDifficulty();
    data << uint32(val);
    data << uint32(IsInGroup);
    GetSession()->SendPacket(&data);
}

void Player::SendResetFailedNotify(uint32 mapid)
{
    WorldPacket data(SMSG_RESET_FAILED_NOTIFY, 4);
    data << uint32(mapid);
    GetSession()->SendPacket(&data);
}

/// Reset all solo instances and optionally send a message on success for each
void Player::ResetInstances(uint8 method)
{
    // method can be INSTANCE_RESET_ALL, INSTANCE_RESET_CHANGE_DIFFICULTY, INSTANCE_RESET_GROUP_JOIN

    // we assume that when the difficulty changes, all instances that can be reset will be
    uint8 dif = GetDifficulty();

    for (BoundInstancesMap::iterator itr = m_boundInstances[dif].begin(); itr != m_boundInstances[dif].end();)
    {
        InstanceSave *p = itr->second.save;
        const MapEntry *entry = sMapStore.LookupEntry(itr->first);
        if(!entry || !p->CanReset() || entry->MapID == 580)
        {
            ++itr;
            continue;
        }

        if(method == INSTANCE_RESET_ALL)
        {
            // the "reset all instances" method can only reset normal maps
            if(dif == DIFFICULTY_HEROIC || entry->map_type == MAP_RAID)
            {
                ++itr;
                continue;
            }
        }
        
        if (method == INSTANCE_RESET_CHANGE_DIFFICULTY)
        {
			if (entry->map_type == MAP_RAID) {
				++itr;
				continue;
			}
		}

        // if the map is loaded, reset it
        Map *map = MapManager::Instance().FindMap(p->GetMapId(), p->GetInstanceId());
        if(map && map->IsDungeon())
            if(!((InstanceMap*)map)->Reset(method))
            {
                ++itr;
                continue;
            }

        // since this is a solo instance there should not be any players inside
        if(method == INSTANCE_RESET_ALL || method == INSTANCE_RESET_CHANGE_DIFFICULTY)
            SendResetInstanceSuccess(p->GetMapId());

        p->DeleteFromDB();
        m_boundInstances[dif].erase(itr++);

        // the following should remove the instance save from the manager and delete it as well
        p->RemovePlayer(this);
    }
}

void Player::SendResetInstanceSuccess(uint32 MapId)
{
    WorldPacket data(SMSG_INSTANCE_RESET, 4);
    data << MapId;
    GetSession()->SendPacket(&data);
}

void Player::SendResetInstanceFailed(uint32 reason, uint32 MapId)
{
    // TODO: find what other fail reasons there are besides players in the instance
    WorldPacket data(SMSG_INSTANCE_RESET_FAILED, 4);
    data << reason;
    data << MapId;
    GetSession()->SendPacket(&data);
}

/*********************************************************/
/***              Update timers                        ***/
/*********************************************************/

///checks the 15 afk reports per 5 minutes limit
void Player::UpdateAfkReport(time_t currTime)
{
    if(m_bgAfkReportedTimer <= currTime)
    {
        m_bgAfkReportedCount = 0;
        m_bgAfkReportedTimer = currTime+5*MINUTE;
    }
}

void Player::UpdateContestedPvP(uint32 diff)
{
    if(!m_contestedPvPTimer||isInCombat())
        return;
    if(m_contestedPvPTimer <= diff)
    {
        ResetContestedPvP();
    }
    else
        m_contestedPvPTimer -= diff;
}

void Player::UpdatePvPFlag(time_t currTime)
{
    if(!IsPvP())
        return;
    if(!isInPvPZone() && (pvpInfo.endTimer == 0 || currTime < (pvpInfo.endTimer + 300)))
        return;

    UpdatePvP(false);
}

void Player::UpdateDuelFlag(time_t currTime)
{
    if(!duel || duel->startTimer == 0 ||currTime < duel->startTimer + 3)
        return;

    SetUInt32Value(PLAYER_DUEL_TEAM, 1);
    duel->opponent->SetUInt32Value(PLAYER_DUEL_TEAM, 2);

    duel->startTimer = 0;
    duel->startTime  = currTime;
    duel->opponent->duel->startTimer = 0;
    duel->opponent->duel->startTime  = currTime;
}

void Player::RemovePet(Pet* pet, PetSaveMode mode, bool returnreagent)
{
    if(!pet)
        pet = GetPet();

    if(returnreagent && (pet || m_temporaryUnsummonedPetNumber))
    {
        //returning of reagents only for players, so best done here
        uint32 spellId = pet ? pet->GetUInt32Value(UNIT_CREATED_BY_SPELL) : m_oldpetspell;
        SpellEntry const *spellInfo = spellmgr.LookupSpell(spellId);

        if(spellInfo)
        {
            for(uint32 i = 0; i < 7; ++i)
            {
                if(spellInfo->Reagent[i] > 0)
                {
                    ItemPosCountVec dest;                   //for succubus, voidwalker, felhunter and felguard credit soulshard when despawn reason other than death (out of range, logout)
                    uint8 msg = CanStoreNewItem( NULL_BAG, NULL_SLOT, dest, spellInfo->Reagent[i], spellInfo->ReagentCount[i] );
                    if( msg == EQUIP_ERR_OK )
                    {
                        Item* item = StoreNewItem( dest, spellInfo->Reagent[i], true);
                        if(IsInWorld())
                            SendNewItem(item,spellInfo->ReagentCount[i],true,false);
                    }
                }
            }
        }
        m_temporaryUnsummonedPetNumber = 0;
    }

    if(!pet || !pet->IsInWorld() || pet->GetOwnerGUID()!=GetGUID())
        return;

    // only if current pet in slot
    switch(pet->getPetType())
    {
        case MINI_PET:
            m_miniPet = 0;
            break;
        case GUARDIAN_PET:
            m_guardianPets.erase(pet->GetGUID());
            break;
        case POSSESSED_PET:
            m_guardianPets.erase(pet->GetGUID());
            pet->RemoveCharmedOrPossessedBy(NULL);
            break;
        default:
            if(GetPetGUID() == pet->GetGUID())
                SetPet(NULL);
            break;
    }

    pet->CombatStop();

    if(returnreagent)
    {
        switch(pet->GetEntry())
        {
            //warlock pets except imp are removed(?) when logging out
            case 1860:
            case 1863:
            case 417:
            case 17252:
                mode = PET_SAVE_NOT_IN_SLOT;
                break;
        }
    }

    pet->SavePetToDB(mode);

    pet->CleanupsBeforeDelete();
    pet->AddObjectToRemoveList();
    pet->m_removed = true;

    if(pet->isControlled())
    {
        WorldPacket data(SMSG_PET_SPELLS, 8);
        data << uint64(0);
        GetSession()->SendPacket(&data);

        if(GetGroup())
            SetGroupUpdateFlag(GROUP_UPDATE_PET);
    }
}

void Player::RemoveMiniPet()
{
    if(Pet* pet = GetMiniPet())
    {
        pet->Remove(PET_SAVE_AS_DELETED);
        m_miniPet = 0;
    }
}

Pet* Player::GetMiniPet()
{
    if(!m_miniPet)
        return NULL;
    return ObjectAccessor::GetPet(m_miniPet);
}

void Player::RemoveGuardians()
{
    while(!m_guardianPets.empty())
    {
        uint64 guid = *m_guardianPets.begin();
        if(Pet* pet = ObjectAccessor::GetPet(guid))
            pet->Remove(PET_SAVE_AS_DELETED);

        m_guardianPets.erase(guid);
    }
}

void Player::RemoveGuardiansWithEntry(uint32 entry)
{
    while(!m_guardianPets.empty())
    {
        uint64 guid = *m_guardianPets.begin();
        if (Pet* pet = ObjectAccessor::GetPet(guid)) {
            if (pet->GetEntry() == entry)
                pet->Remove(PET_SAVE_AS_DELETED);
        }

        m_guardianPets.erase(guid);
    }
}

bool Player::HasGuardianWithEntry(uint32 entry)
{
    // pet guid middle part is entry (and creature also)
    // and in guardian list must be guardians with same entry _always_
    for(GuardianPetList::const_iterator itr = m_guardianPets.begin(); itr != m_guardianPets.end(); ++itr)
        if(GUID_ENPART(*itr)==entry)
            return true;

    return false;
}

void Player::Uncharm()
{
    Unit* charm = GetCharm();
    if(!charm)
        return;

    if(charm->GetTypeId() == TYPEID_UNIT && (charm->ToCreature())->isPet()
        && ((Pet*)charm)->getPetType() == POSSESSED_PET)
    {
        ((Pet*)charm)->Remove(PET_SAVE_AS_DELETED);
    }
    else
    {
        charm->RemoveSpellsCausingAura(SPELL_AURA_MOD_CHARM);
        charm->RemoveSpellsCausingAura(SPELL_AURA_MOD_POSSESS_PET);
        charm->RemoveSpellsCausingAura(SPELL_AURA_MOD_POSSESS);
    }

    if(GetCharmGUID())
    {
        sLog.outError("CRASH ALARM! Player %s is not able to uncharm unit (Entry: %u, Type: %u)", GetName(), charm->GetEntry(), charm->GetTypeId());
    }
}

void Player::BuildPlayerChat(WorldPacket *data, uint8 msgtype, const std::string& text, uint32 language) const
{
    *data << (uint8)msgtype;
    *data << (uint32)language;
    *data << (uint64)GetGUID();
    *data << (uint32)language;                               //language 2.1.0 ?
    *data << (uint64)GetGUID();
    *data << (uint32)(text.length()+1);
    *data << text;
    *data << (uint8)chatTag();
}

void Player::Say(const std::string& text, const uint32 language)
{
    WorldPacket data(SMSG_MESSAGECHAT, 200);
    BuildPlayerChat(&data, CHAT_MSG_SAY, text, language);
    SendMessageToSetInRange(&data,sWorld.getConfig(CONFIG_LISTEN_RANGE_SAY),true);
}

void Player::Yell(const std::string& text, const uint32 language)
{
    WorldPacket data(SMSG_MESSAGECHAT, 200);
    BuildPlayerChat(&data, CHAT_MSG_YELL, text, language);
    SendMessageToSetInRange(&data,sWorld.getConfig(CONFIG_LISTEN_RANGE_YELL),true);
}

void Player::TextEmote(const std::string& text)
{
    WorldPacket data(SMSG_MESSAGECHAT, 200);
    BuildPlayerChat(&data, CHAT_MSG_EMOTE, text, LANG_UNIVERSAL);
    SendMessageToSetInRange(&data,sWorld.getConfig(CONFIG_LISTEN_RANGE_TEXTEMOTE),true, !sWorld.getConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_CHAT), true );
}

void Player::Whisper(const std::string& text, uint32 language,uint64 receiver)
{
    if (language != LANG_ADDON)                             // if not addon data
        language = LANG_UNIVERSAL;                          // whispers should always be readable

    Player *rPlayer = objmgr.GetPlayer(receiver);

    // when player you are whispering to is dnd, he cannot receive your message, unless you are in gm mode
    if(!rPlayer->isDND() || isGameMaster())
    {
        WorldPacket data(SMSG_MESSAGECHAT, 200);
        BuildPlayerChat(&data, CHAT_MSG_WHISPER, text, language);
        rPlayer->GetSession()->SendPacket(&data);

        // Do not send confirmation for addon messages
        if (language != LANG_ADDON) {
            data.Initialize(SMSG_MESSAGECHAT, 200);
            rPlayer->BuildPlayerChat(&data, CHAT_MSG_REPLY, text, language);
            GetSession()->SendPacket(&data);
        }
    }
    else
    {
        // announce to player that player he is whispering to is dnd and cannot receive his message
        ChatHandler(this).PSendSysMessage(LANG_PLAYER_DND, rPlayer->GetName(), rPlayer->dndMsg.c_str());
    }

    if(!isAcceptWhispers() && !isGameMaster() && !rPlayer->isGameMaster())
    {
        SetAcceptWhispers(true);
        ChatHandler(this).SendSysMessage(LANG_COMMAND_WHISPERON);
    }

    // announce to player that player he is whispering to is afk
    if(rPlayer->isAFK() && language != LANG_ADDON)
        ChatHandler(this).PSendSysMessage(LANG_PLAYER_AFK, rPlayer->GetName(), rPlayer->afkMsg.c_str());

    // if player whisper someone, auto turn of dnd to be able to receive an answer
    if(isDND() && !rPlayer->isGameMaster())
        ToggleDND();
}

void Player::PetSpellInitialize()
{
    Pet* pet = GetPet();

    if(pet)
    {
        uint8 addlist = 0;

        CreatureInfo const *cinfo = pet->GetCreatureInfo();

        if(pet->isControlled() && (pet->getPetType() == HUNTER_PET || cinfo && cinfo->type == CREATURE_TYPE_DEMON && getClass() == CLASS_WARLOCK))
        {
            for(PetSpellMap::iterator itr = pet->m_spells.begin();itr != pet->m_spells.end();++itr)
            {
                if(itr->second->state == PETSPELL_REMOVED)
                    continue;
                ++addlist;
            }
        }

        // first line + actionbar + spellcount + spells + last adds
        WorldPacket data(SMSG_PET_SPELLS, 16+40+1+4*addlist+25);

        CharmInfo *charmInfo = pet->GetCharmInfo();

                                                            //16
        data << (uint64)pet->GetGUID() << uint32(0x00000000) << uint8(pet->GetReactState()) << uint8(charmInfo->GetCommandState()) << uint16(0);

        for(uint32 i = 0; i < 10; i++)                      //40
        {
            data << uint16(charmInfo->GetActionBarEntry(i)->SpellOrAction) << uint16(charmInfo->GetActionBarEntry(i)->Type);
        }

        data << uint8(addlist);                             //1

        if(addlist && pet->isControlled())
        {
            for (PetSpellMap::iterator itr = pet->m_spells.begin(); itr != pet->m_spells.end(); ++itr)
            {
                if(itr->second->state == PETSPELL_REMOVED)
                    continue;

                data << uint16(itr->first);
                data << uint16(itr->second->active);        // pet spell active state isn't boolean
            }
        }

        //data << uint8(0x01) << uint32(0x6010) << uint32(0x01) << uint32(0x05) << uint16(0x00);    //15
        uint8 count = 3;                                    //1+8+8+8=25

        // if count = 0, then end of packet...
        data << count;
        // uint32 value is spell id...
        // uint64 value is constant 0, unknown...
        data << uint32(0x6010) << uint64(0);                // if count = 1, 2 or 3
        //data << uint32(0x5fd1) << uint64(0);  // if count = 2
        data << uint32(0x8e8c) << uint64(0);                // if count = 3
        data << uint32(0x8e8b) << uint64(0);                // if count = 3

        GetSession()->SendPacket(&data);
    }
}

void Player::PossessSpellInitialize()
{
    Unit* charm = GetCharm();

    if(!charm)
        return;

    CharmInfo *charmInfo = charm->GetCharmInfo();

    if(!charmInfo)
    {
        sLog.outError("Player::PossessSpellInitialize(): charm ("I64FMTD") has no charminfo!", charm->GetGUID());
        return;
    }

    uint8 addlist = 0;
    WorldPacket data(SMSG_PET_SPELLS, 16+40+1+4*addlist+25);// first line + actionbar + spellcount + spells + last adds

                                                            //16
    data << (uint64)charm->GetGUID() << uint32(0x00000000) << uint8(0) << uint8(0) << uint16(0);

    for(uint32 i = 0; i < 10; i++)                          //40
    {
        data << uint16(charmInfo->GetActionBarEntry(i)->SpellOrAction) << uint16(charmInfo->GetActionBarEntry(i)->Type);
    }

    data << uint8(addlist);                                 //1

    uint8 count = 3;
    data << count;
    data << uint32(0x6010) << uint64(0);                    // if count = 1, 2 or 3
    data << uint32(0x8e8c) << uint64(0);                    // if count = 3
    data << uint32(0x8e8b) << uint64(0);                    // if count = 3

    GetSession()->SendPacket(&data);
}

void Player::CharmSpellInitialize()
{
    Unit* charm = GetCharm();

    if(!charm)
        return;

    CharmInfo *charmInfo = charm->GetCharmInfo();
    if(!charmInfo)
    {
        sLog.outError("Player::CharmSpellInitialize(): the player's charm ("I64FMTD") has no charminfo!", charm->GetGUID());
        return;
    }

    uint8 addlist = 0;

    if(charm->GetTypeId() != TYPEID_PLAYER)
    {
        CreatureInfo const *cinfo = (charm->ToCreature())->GetCreatureInfo();

        if(cinfo && cinfo->type == CREATURE_TYPE_DEMON && getClass() == CLASS_WARLOCK)
        {
            for(uint32 i = 0; i < CREATURE_MAX_SPELLS; ++i)
            {
                if(charmInfo->GetCharmSpell(i)->spellId)
                    ++addlist;
            }
        }
    }

    WorldPacket data(SMSG_PET_SPELLS, 16+40+1+4*addlist+25);// first line + actionbar + spellcount + spells + last adds

    data << (uint64)charm->GetGUID() << uint32(0x00000000);

    if(charm->GetTypeId() != TYPEID_PLAYER)
        data << uint8((charm->ToCreature())->GetReactState()) << uint8(charmInfo->GetCommandState());
    else
        data << uint8(0) << uint8(0);

    data << uint16(0);

    for(uint32 i = 0; i < 10; i++)                          //40
    {
        data << uint16(charmInfo->GetActionBarEntry(i)->SpellOrAction) << uint16(charmInfo->GetActionBarEntry(i)->Type);
    }

    data << uint8(addlist);                                 //1

    if(addlist)
    {
        for(uint32 i = 0; i < CREATURE_MAX_SPELLS; ++i)
        {
            CharmSpellEntry *cspell = charmInfo->GetCharmSpell(i);
            if(cspell->spellId)
            {
                data << uint16(cspell->spellId);
                data << uint16(cspell->active);
            }
        }
    }

    uint8 count = 3;
    data << count;
    data << uint32(0x6010) << uint64(0);                    // if count = 1, 2 or 3
    data << uint32(0x8e8c) << uint64(0);                    // if count = 3
    data << uint32(0x8e8b) << uint64(0);                    // if count = 3

    GetSession()->SendPacket(&data);
}

int32 Player::GetTotalFlatMods(uint32 spellId, SpellModOp op)
{
    SpellEntry const *spellInfo = spellmgr.LookupSpell(spellId);
    if (!spellInfo) return 0;
    int32 total = 0;
    for (SpellModList::iterator itr = m_spellMods[op].begin(); itr != m_spellMods[op].end(); ++itr)
    {
        SpellModifier *mod = *itr;

        if(!IsAffectedBySpellmod(spellInfo,mod))
            continue;

        if (mod->type == SPELLMOD_FLAT)
            total += mod->value;
    }
    return total;
}

int32 Player::GetTotalPctMods(uint32 spellId, SpellModOp op)
{
    SpellEntry const *spellInfo = spellmgr.LookupSpell(spellId);
    if (!spellInfo) return 0;
    int32 total = 0;
    for (SpellModList::iterator itr = m_spellMods[op].begin(); itr != m_spellMods[op].end(); ++itr)
    {
        SpellModifier *mod = *itr;

        if(!IsAffectedBySpellmod(spellInfo,mod))
            continue;

        if (mod->type == SPELLMOD_PCT)
            total += mod->value;
    }
    return total;
}

bool Player::IsAffectedBySpellmod(SpellEntry const *spellInfo, SpellModifier *mod, Spell const* spell)
{
    if (!mod || !spellInfo)
        return false;
        
    //if (spellInfo && spell)
        //sLog.outString("IsAffectedBySpellmod1: spell %u against spell %u: %u %u %u", spellInfo->Id, spell->m_spellInfo->Id, mod->op, mod->type, mod->value);

    if(mod->charges == -1 && mod->lastAffected )            // marked as expired but locked until spell casting finish
    {
        // prevent apply to any spell except spell that trigger expire
        if(spell)
        {
            if(mod->lastAffected != spell)
                return false;
        }
        else if(mod->lastAffected != FindCurrentSpellBySpellId(spellInfo->Id))
            return false;
    }
    
    //if (spellInfo && spell)
        //sLog.outString("IsAffectedBySpellmod2: spell %u against spell %u: %u %u %u", spellInfo->Id, spell->m_spellInfo->Id, mod->op, mod->type, mod->value);

    return spellmgr.IsAffectedBySpell(spellInfo,mod->spellId,mod->effectId,mod->mask);
}

void Player::AddSpellMod(SpellModifier* mod, bool apply)
{
    uint16 Opcode= (mod->type == SPELLMOD_FLAT) ? SMSG_SET_FLAT_SPELL_MODIFIER : SMSG_SET_PCT_SPELL_MODIFIER;

    for(int eff=0;eff<64;++eff)
    {
        uint64 _mask = uint64(1) << eff;
        if ( mod->mask & _mask)
        {
            int32 val = 0;
            for (SpellModList::iterator itr = m_spellMods[mod->op].begin(); itr != m_spellMods[mod->op].end(); ++itr)
            {
                if ((*itr)->type == mod->type && (*itr)->mask & _mask)
                    val += (*itr)->value;
            }
            val += apply ? mod->value : -(mod->value);
            WorldPacket data(Opcode, (1+1+4));
            data << uint8(eff);
            data << uint8(mod->op);
            data << int32(val);
            SendDirectMessage(&data);
        }
    }

    if (apply)
        m_spellMods[mod->op].push_back(mod);
    else
    {
        if (mod->charges == -1)
            --m_SpellModRemoveCount;
        m_spellMods[mod->op].remove(mod);
        delete mod;
    }
}

// Restore spellmods in case of failed cast
void Player::RestoreSpellMods(Spell const* spell)
{
    if(!spell || (m_SpellModRemoveCount == 0))
        return;

    for(int i=0;i<MAX_SPELLMOD;++i)
    {
        for (SpellModList::iterator itr = m_spellMods[i].begin(); itr != m_spellMods[i].end();++itr)
        {
            SpellModifier *mod = *itr;

            if (mod && mod->charges == -1 && mod->lastAffected == spell)
            {
                mod->lastAffected = NULL;
                mod->charges = 1;
                m_SpellModRemoveCount--;
            }
        }
    }
}

void Player::RemoveSpellMods(Spell const* spell)
{
    if(!spell || (m_SpellModRemoveCount == 0))
        return;

    for(int i=0;i<MAX_SPELLMOD;++i)
    {
        for (SpellModList::iterator itr = m_spellMods[i].begin(); itr != m_spellMods[i].end();)
        {
            SpellModifier *mod = *itr;
            ++itr;

            if (mod && mod->charges == -1 && (mod->lastAffected == spell || mod->lastAffected==NULL))
            {
                RemoveAurasDueToSpell(mod->spellId);
                if (m_spellMods[i].empty())
                    break;
                else
                    itr = m_spellMods[i].begin();
            }
        }
    }
}

// send Proficiency
void Player::SendProficiency(uint8 pr1, uint32 pr2)
{
    WorldPacket data(SMSG_SET_PROFICIENCY, 8);
    data << pr1 << pr2;
    GetSession()->SendPacket (&data);
}

void Player::RemovePetitionsAndSigns(uint64 guid, uint32 type, SQLTransaction trans)
{
    QueryResult *result = NULL;
    if(type==10)
        result = CharacterDatabase.PQuery("SELECT ownerguid,petitionguid FROM petition_sign WHERE playerguid = '%u'", GUID_LOPART(guid));
    else
        result = CharacterDatabase.PQuery("SELECT ownerguid,petitionguid FROM petition_sign WHERE playerguid = '%u' AND type = '%u'", GUID_LOPART(guid), type);
    if(result)
    {
        do                                                  // this part effectively does nothing, since the deletion / modification only takes place _after_ the PetitionQuery. Though I don't know if the result remains intact if I execute the delete query beforehand.
        {                                                   // and SendPetitionQueryOpcode reads data from the DB
            Field *fields = result->Fetch();
            uint64 ownerguid   = MAKE_NEW_GUID(fields[0].GetUInt32(), 0, HIGHGUID_PLAYER);
            uint64 petitionguid = MAKE_NEW_GUID(fields[1].GetUInt32(), 0, HIGHGUID_ITEM);

            // send update if charter owner in game
            Player* owner = objmgr.GetPlayer(ownerguid);
            if(owner)
                owner->GetSession()->SendPetitionQueryOpcode(petitionguid);

        } while ( result->NextRow() );

        delete result;

        if(type==10)
            trans->PAppend("DELETE FROM petition_sign WHERE playerguid = '%u'", GUID_LOPART(guid));
        else
            trans->PAppend("DELETE FROM petition_sign WHERE playerguid = '%u' AND type = '%u'", GUID_LOPART(guid), type);
    }

    if(type == 10)
    {
        trans->PAppend("DELETE FROM petition WHERE ownerguid = '%u'", GUID_LOPART(guid));
        trans->PAppend("DELETE FROM petition_sign WHERE ownerguid = '%u'", GUID_LOPART(guid));
    }
    else
    {
        trans->PAppend("DELETE FROM petition WHERE ownerguid = '%u' AND type = '%u'", GUID_LOPART(guid), type);
        trans->PAppend("DELETE FROM petition_sign WHERE ownerguid = '%u' AND type = '%u'", GUID_LOPART(guid), type);
    }
}

void Player::SetRestBonus (float rest_bonus_new)
{
    // Prevent resting on max level
    if(getLevel() >= sWorld.getConfig(CONFIG_MAX_PLAYER_LEVEL))
        rest_bonus_new = 0;

    if(rest_bonus_new < 0)
        rest_bonus_new = 0;

    float rest_bonus_max = (float)GetUInt32Value(PLAYER_NEXT_LEVEL_XP)*1.5/2;
    if(rest_bonus_new > rest_bonus_max)
        m_rest_bonus = rest_bonus_max;
    else
        m_rest_bonus = rest_bonus_new;

    // update data for client
    if(m_rest_bonus>10)
        SetByteValue(PLAYER_BYTES_2, 3, 0x01);              // Set Reststate = Rested
    else if(m_rest_bonus<=1)
        SetByteValue(PLAYER_BYTES_2, 3, 0x02);              // Set Reststate = Normal

    //RestTickUpdate
    SetUInt32Value(PLAYER_REST_STATE_EXPERIENCE, uint32(m_rest_bonus));
}

void Player::HandleStealthedUnitsDetection()
{
    m_GiantLock.acquire();
    std::list<Unit*> stealthedUnits;
    Trinity::AnyStealthedCheck u_check;
    Trinity::UnitListSearcher<Trinity::AnyStealthedCheck > searcher(stealthedUnits, u_check);
    VisitNearbyObject(GetMap()->GetVisibilityDistance(), searcher);

    for (std::list<Unit*>::iterator i = stealthedUnits.begin(); i != stealthedUnits.end(); ++i)
    {
        if (!HaveAtClient(*i) && canSeeOrDetect(*i, true))
        {
            (*i)->SendUpdateToPlayer(this);
            m_clientGUIDs.insert((*i)->GetGUID());

            #ifdef TRINITY_DEBUG
            if((sLog.getLogFilter() & LOG_FILTER_VISIBILITY_CHANGES)==0)
                sLog.outDebug("Object %u (Type: %u) is detected in stealth by player %u. Distance = %f",(*i)->GetGUIDLow(),(*i)->GetTypeId(),GetGUIDLow(),GetDistance(*i));
            #endif

            SendInitialVisiblePackets(*i);
        }
    }
    m_GiantLock.release();
}

bool Player::ActivateTaxiPathTo(std::vector<uint32> const& nodes, uint32 mount_id, Creature* npc)
{
    if(nodes.size() < 2)
        return false;

    // not let cheating with start flight mounted
    if(IsMounted())
    {
        WorldPacket data(SMSG_ACTIVATETAXIREPLY, 4);
        data << uint32(ERR_TAXIPLAYERALREADYMOUNTED);
        GetSession()->SendPacket(&data);
        return false;
    }
    
    if (GetSession()->isLogingOut() || isInCombat() || hasUnitState(UNIT_STAT_STUNNED) || hasUnitState(UNIT_STAT_ROOT))
    {
        WorldPacket data(SMSG_ACTIVATETAXIREPLY, 4);
        data << uint32(ERR_TAXIPLAYERBUSY);
        GetSession()->SendPacket(&data);
        return false;
    }

    if( m_ShapeShiftFormSpellId && m_form != FORM_BATTLESTANCE && m_form != FORM_BERSERKERSTANCE && m_form != FORM_DEFENSIVESTANCE && m_form != FORM_SHADOW )
    {
        WorldPacket data(SMSG_ACTIVATETAXIREPLY, 4);
        data << uint32(ERR_TAXIPLAYERSHAPESHIFTED);
        GetSession()->SendPacket(&data);
        return false;
    }

    // not let cheating with start flight in time of logout process || if casting not finished || while in combat || if not use Spell's with EffectSendTaxi
    if(GetSession()->isLogingOut() ||
        (!m_currentSpells[CURRENT_GENERIC_SPELL] ||
        m_currentSpells[CURRENT_GENERIC_SPELL]->m_spellInfo->Effect[0] != SPELL_EFFECT_SEND_TAXI)&&
        IsNonMeleeSpellCasted(false) ||
        isInCombat())
    {
        WorldPacket data(SMSG_ACTIVATETAXIREPLY, 4);
        data << uint32(ERR_TAXIPLAYERBUSY);
        GetSession()->SendPacket(&data);
        return false;
    }

    if(HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE))
        return false;

    uint32 sourcenode = nodes[0];

    // starting node too far away (cheat?)
    TaxiNodesEntry const* node = sTaxiNodesStore.LookupEntry(sourcenode);
    if( (!node || node->map_id != GetMapId() ||
        (node->x - GetPositionX())*(node->x - GetPositionX())+
        (node->y - GetPositionY())*(node->y - GetPositionY())+
        (node->z - GetPositionZ())*(node->z - GetPositionZ()) >
        (2*INTERACTION_DISTANCE)*(2*INTERACTION_DISTANCE)*(2*INTERACTION_DISTANCE)) && node->ID != 173) // Exception for quest 11108
    {
        WorldPacket data(SMSG_ACTIVATETAXIREPLY, 4);
        data << uint32(ERR_TAXIUNSPECIFIEDSERVERERROR);
        GetSession()->SendPacket(&data);
        return false;
    }

    // Prepare to flight start now

    // stop combat at start taxi flight if any
    CombatStop();

    // stop trade (client cancel trade at taxi map open but cheating tools can be used for reopen it)
    TradeCancel(true);

    // clean not finished taxi path if any
    CleanupAfterTaxiFlight();

    // 0 element current node
    m_taxi.AddTaxiDestination(sourcenode);

    // fill destinations path tail
    uint32 sourcepath = 0;
    uint32 totalcost = 0;

    uint32 prevnode = sourcenode;
    uint32 lastnode = 0;

    for(uint32 i = 1; i < nodes.size(); ++i)
    {
        uint32 path, cost;

        lastnode = nodes[i];
        objmgr.GetTaxiPath(prevnode, lastnode, path, cost);

        if(!path)
        {
            CleanupAfterTaxiFlight();
            return false;
        }

        totalcost += cost;

        if(prevnode == sourcenode)
            sourcepath = path;

        m_taxi.AddTaxiDestination(lastnode);

        prevnode = lastnode;
    }

    if(!mount_id)                                           // if not provide then attempt use default.
        mount_id = objmgr.GetTaxiMount(sourcenode, GetTeam());

    if (mount_id == 0 || sourcepath == 0)
    {
        WorldPacket data(SMSG_ACTIVATETAXIREPLY, 4);
        data << uint32(ERR_TAXIUNSPECIFIEDSERVERERROR);
        GetSession()->SendPacket(&data);
        CleanupAfterTaxiFlight();
        return false;
    }

    uint32 money = GetMoney();

    if(npc)
    {
        totalcost = (uint32)ceil(totalcost*GetReputationPriceDiscount(npc));
    }

    if(money < totalcost)
    {
        WorldPacket data(SMSG_ACTIVATETAXIREPLY, 4);
        data << uint32(ERR_TAXINOTENOUGHMONEY);
        GetSession()->SendPacket(&data);
        CleanupAfterTaxiFlight();
        return false;
    }

    //Checks and preparations done, DO FLIGHT
    ModifyMoney(-(int32)totalcost);

    // prevent stealth flight
    //RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_TALK);

    WorldPacket data(SMSG_ACTIVATETAXIREPLY, 4);
    data << uint32(ERR_TAXIOK);
    GetSession()->SendPacket(&data);

    GetSession()->SendDoFlight(mount_id, sourcepath);

    return true;
}

void Player::CleanupAfterTaxiFlight()
{
    m_taxi.ClearTaxiDestinations();        // not destinations, clear source node
    Unmount();
    RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE | UNIT_FLAG_TAXI_FLIGHT);
    getHostilRefManager().setOnlineOfflineState(true);
}

void Player::ProhibitSpellSchool(SpellSchoolMask idSchoolMask, uint32 unTimeMs )
{
                                                            // last check 2.0.10
    WorldPacket data(SMSG_SPELL_COOLDOWN, 8+1+m_spells.size()*8);
    data << GetGUID();
    data << uint8(0x0);                                     // flags (0x1, 0x2)
    time_t curTime = time(NULL);
    for(PlayerSpellMap::const_iterator itr = m_spells.begin(); itr != m_spells.end(); ++itr)
    {
        if (itr->second->state == PLAYERSPELL_REMOVED)
            continue;
        uint32 unSpellId = itr->first;
        SpellEntry const *spellInfo = spellmgr.LookupSpell(unSpellId);
        if (!spellInfo)
        {
            ASSERT(spellInfo);
            continue;
        }

        // Not send cooldown for this spells
        if (spellInfo->Attributes & SPELL_ATTR_DISABLED_WHILE_ACTIVE)
            continue;

        if(spellInfo->PreventionType != SPELL_PREVENTION_TYPE_SILENCE)
            continue;

        if((idSchoolMask & GetSpellSchoolMask(spellInfo)) && GetSpellCooldownDelay(unSpellId) < unTimeMs )
        {
            data << unSpellId;
            data << unTimeMs;                               // in m.secs
            AddSpellCooldown(unSpellId, 0, curTime + unTimeMs/1000);
        }
    }
    GetSession()->SendPacket(&data);
}

void Player::InitDataForForm(bool reapplyMods)
{
    SpellShapeshiftEntry const* ssEntry = sSpellShapeshiftStore.LookupEntry(m_form);
    if(ssEntry && ssEntry->attackSpeed)
    {
        SetAttackTime(BASE_ATTACK,ssEntry->attackSpeed);
        SetAttackTime(OFF_ATTACK,ssEntry->attackSpeed);
        SetAttackTime(RANGED_ATTACK, BASE_ATTACK_TIME);
    }
    else
        SetRegularAttackTime();

    switch(m_form)
    {
        case FORM_CAT:
        {
            if(getPowerType()!=POWER_ENERGY)
                setPowerType(POWER_ENERGY);
            break;
        }
        case FORM_BEAR:
        case FORM_DIREBEAR:
        {
            if(getPowerType()!=POWER_RAGE)
                setPowerType(POWER_RAGE);
            break;
        }
        default:                                            // 0, for example
        {
            ChrClassesEntry const* cEntry = sChrClassesStore.LookupEntry(getClass());
            if(cEntry && cEntry->powerType < MAX_POWERS && uint32(getPowerType()) != cEntry->powerType)
                setPowerType(Powers(cEntry->powerType));
            break;
        }
    }

    // update auras at form change, ignore this at mods reapply (.reset stats/etc) when form not change.
    if (!reapplyMods)
        UpdateEquipSpellsAtFormChange();

    UpdateAttackPowerAndDamage();
    UpdateAttackPowerAndDamage(true);
}

// Return true is the bought item has a max count to force refresh of window by caller
bool Player::BuyItemFromVendor(uint64 vendorguid, uint32 item, uint8 count, uint64 bagguid, uint8 slot)
{
    // cheating attempt
    if(count < 1) count = 1;
    
    // cheating attempt
    if (slot > MAX_BAG_SIZE && slot != NULL_SLOT)
        return false;

    if(!isAlive())
        return false;

    ItemPrototype const *pProto = objmgr.GetItemPrototype( item );
    if( !pProto )
    {
        SendBuyError( BUY_ERR_CANT_FIND_ITEM, NULL, item, 0);
        return false;
    }

    Creature *pCreature = ObjectAccessor::GetNPCIfCanInteractWith(*this, vendorguid,UNIT_NPC_FLAG_VENDOR);
    if (!pCreature)
    {
        SendBuyError( BUY_ERR_DISTANCE_TOO_FAR, NULL, item, 0);
        return false;
    }

    VendorItemData const* vItems = pCreature->GetVendorItems();
    if(!vItems || vItems->Empty())
    {
        SendBuyError( BUY_ERR_CANT_FIND_ITEM, pCreature, item, 0);
        return false;
    }

    size_t vendor_slot = vItems->FindItemSlot(item);
    if(vendor_slot >= vItems->GetItemCount())
    {
        SendBuyError( BUY_ERR_CANT_FIND_ITEM, pCreature, item, 0);
        return false;
    }

    VendorItem const* crItem = vItems->m_items[vendor_slot];

    // check current item amount if it limited
    if( crItem->maxcount != 0 )
    {
        if(pCreature->GetVendorItemCurrentCount(crItem) < pProto->BuyCount * count )
        {
            SendBuyError( BUY_ERR_ITEM_ALREADY_SOLD, pCreature, item, 0);
            return false;
        }
    }

    if( uint32(GetReputationRank(pProto->RequiredReputationFaction)) < pProto->RequiredReputationRank)
    {
        SendBuyError( BUY_ERR_REPUTATION_REQUIRE, pCreature, item, 0);
        return false;
    }

    if(crItem->ExtendedCost)
    {
        ItemExtendedCostEntry const* iece = sItemExtendedCostStore.LookupEntry(crItem->ExtendedCost);
        if(!iece)
        {
            sLog.outError("Item %u have wrong ExtendedCost field value %u", pProto->ItemId, crItem->ExtendedCost);
            return false;
        }

        // honor points price
        if(GetHonorPoints() < (iece->reqhonorpoints * count))
        {
            SendEquipError(EQUIP_ERR_NOT_ENOUGH_HONOR_POINTS, NULL, NULL);
            return false;
        }

        // arena points price
        if(GetArenaPoints() < (iece->reqarenapoints * count))
        {
            SendEquipError(EQUIP_ERR_NOT_ENOUGH_ARENA_POINTS, NULL, NULL);
            return false;
        }

        // item base price
        for (uint8 i = 0; i < 5; ++i)
        {
            if(iece->reqitem[i] && !HasItemCount(iece->reqitem[i], (iece->reqitemcount[i] * count)))
            {
                SendEquipError(EQUIP_ERR_VENDOR_MISSING_TURNINS, NULL, NULL);
                return false;
            }
        }

        // check for personal arena rating requirement
        if( GetMaxPersonalArenaRatingRequirement() < iece->reqpersonalarenarating )
        {
            // probably not the proper equip err
            SendEquipError(EQUIP_ERR_CANT_EQUIP_RANK,NULL,NULL);
            return false;
        }
    }

    uint32 price  = pProto->BuyPrice * count;

    // reputation discount
    price = uint32(floor(price * GetReputationPriceDiscount(pCreature)));

    if( GetMoney() < price )
    {
        SendBuyError( BUY_ERR_NOT_ENOUGHT_MONEY, pCreature, item, 0);
        return false;
    }

    uint8 bag = 0;                                          // init for case invalid bagGUID

    if (bagguid != NULL_BAG && slot != NULL_SLOT)
    {
        Bag *pBag;
        if( bagguid == GetGUID() )
        {
            bag = INVENTORY_SLOT_BAG_0;
        }
        else
        {
            for (int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END;i++)
            {
                pBag = (Bag*)GetItemByPos(INVENTORY_SLOT_BAG_0,i);
                if( pBag )
                {
                    if( bagguid == pBag->GetGUID() )
                    {
                        bag = i;
                        break;
                    }
                }
            }
        }
    }

    if( IsInventoryPos( bag, slot ) || (bagguid == NULL_BAG && slot == NULL_SLOT) )
    {
        ItemPosCountVec dest;
        uint8 msg = CanStoreNewItem( bag, slot, dest, item, pProto->BuyCount * count );
        if( msg != EQUIP_ERR_OK )
        {
            SendEquipError( msg, NULL, NULL );
            return false;
        }

        ModifyMoney( -(int32)price );
        if(crItem->ExtendedCost)                            // case for new honor system
        {
            ItemExtendedCostEntry const* iece = sItemExtendedCostStore.LookupEntry(crItem->ExtendedCost);
            if(iece->reqhonorpoints)
                ModifyHonorPoints( - int32(iece->reqhonorpoints * count));
            if(iece->reqarenapoints)
                ModifyArenaPoints( - int32(iece->reqarenapoints * count));
            for (uint8 i = 0; i < 5; ++i)
            {
                if(iece->reqitem[i])
                    DestroyItemCount(iece->reqitem[i], (iece->reqitemcount[i] * count), true);
            }
        }

        if(Item *it = StoreNewItem( dest, item, true ))
        {
            uint32 new_count = pCreature->UpdateVendorItemCurrentCount(crItem,pProto->BuyCount * count);

            WorldPacket data(SMSG_BUY_ITEM, (8+4+4+4));
            data << pCreature->GetGUID();
            data << (uint32)(vendor_slot+1);                // numbered from 1 at client
            data << (uint32)(crItem->maxcount > 0 ? new_count : 0xFFFFFFFF);
            data << (uint32)count;
            GetSession()->SendPacket(&data);

            SendNewItem(it, pProto->BuyCount*count, true, false, false);
        }
    }
    else if( IsEquipmentPos( bag, slot ) )
    {
        if(pProto->BuyCount * count != 1)
        {
            SendEquipError( EQUIP_ERR_ITEM_CANT_BE_EQUIPPED, NULL, NULL );
            return false;
        }

        uint16 dest;
        uint8 msg = CanEquipNewItem( slot, dest, item, false );
        if( msg != EQUIP_ERR_OK )
        {
            SendEquipError( msg, NULL, NULL );
            return false;
        }

        ModifyMoney( -(int32)price );
        if(crItem->ExtendedCost)                            // case for new honor system
        {
            ItemExtendedCostEntry const* iece = sItemExtendedCostStore.LookupEntry(crItem->ExtendedCost);
            if(iece->reqhonorpoints)
                ModifyHonorPoints( - int32(iece->reqhonorpoints));
            if(iece->reqarenapoints)
                ModifyArenaPoints( - int32(iece->reqarenapoints));
            for (uint8 i = 0; i < 5; ++i)
            {
                if(iece->reqitem[i])
                    DestroyItemCount(iece->reqitem[i], iece->reqitemcount[i], true);
            }
        }

        if(Item *it = EquipNewItem( dest, item, true ))
        {
            uint32 new_count = pCreature->UpdateVendorItemCurrentCount(crItem,pProto->BuyCount * count);

            WorldPacket data(SMSG_BUY_ITEM, (8+4+4+4));
            data << pCreature->GetGUID();
            data << (uint32)(vendor_slot+1);                // numbered from 1 at client
            data << (uint32)(crItem->maxcount > 0 ? new_count : 0xFFFFFFFF);
            data << (uint32)count;
            GetSession()->SendPacket(&data);

            SendNewItem(it, pProto->BuyCount*count, true, false, false);

            AutoUnequipOffhandIfNeed();
        }
    }
    else
    {
        SendEquipError( EQUIP_ERR_ITEM_DOESNT_GO_TO_SLOT, NULL, NULL );
        return false;
    }

    return crItem->maxcount!=0;
}

uint32 Player::GetMaxPersonalArenaRatingRequirement()
{
    // returns the maximal personal arena rating that can be used to purchase items requiring this condition
    // the personal rating of the arena team must match the required limit as well
    // so return max[in arenateams](min(personalrating[teamtype], teamrating[teamtype]))
    uint32 max_personal_rating = 0;
    for(int i = 0; i < MAX_ARENA_SLOT; ++i)
    {
        if(ArenaTeam * at = objmgr.GetArenaTeamById(GetArenaTeamId(i)))
        {
            uint32 p_rating = GetUInt32Value(PLAYER_FIELD_ARENA_TEAM_INFO_1_1 + (i * 6) + 5);
            uint32 t_rating = at->GetRating();
            p_rating = p_rating<t_rating? p_rating : t_rating;
            if(max_personal_rating < p_rating)
                max_personal_rating = p_rating;
        }
    }
    return max_personal_rating;
}

void Player::UpdateHomebindTime(uint32 time)
{
    // GMs never get homebind timer online
    if (m_InstanceValid || isGameMaster())
    {
        if(m_HomebindTimer)                                 // instance valid, but timer not reset
        {
            // hide reminder
            WorldPacket data(SMSG_RAID_GROUP_ONLY, 4+4);
            data << uint32(0);
            data << uint32(0);
            GetSession()->SendPacket(&data);
        }
        // instance is valid, reset homebind timer
        m_HomebindTimer = 0;
    }
    else if (m_HomebindTimer > 0)
    {
        if (time >= m_HomebindTimer)
        {
            // teleport to homebind location
            TeleportTo(m_homebindMapId, m_homebindX, m_homebindY, m_homebindZ, GetOrientation());
        }
        else
            m_HomebindTimer -= time;
    }
    else
    {
        // instance is invalid, start homebind timer
        m_HomebindTimer = 60000;
        // send message to player
        WorldPacket data(SMSG_RAID_GROUP_ONLY, 4+4);
        data << m_HomebindTimer;
        data << uint32(1);
        GetSession()->SendPacket(&data);
    }
}

void Player::UpdatePvP(bool state, bool ovrride)
{
    if(!state || ovrride)
    {
        SetPvP(state);
        if(Pet* pet = GetPet())
            pet->SetPvP(state);
        if(Unit* charmed = GetCharm())
            charmed->SetPvP(state);

        pvpInfo.endTimer = 0;
    }
    else
    {
        if(pvpInfo.endTimer != 0)
            pvpInfo.endTimer = time(NULL);
        else
        {
            SetPvP(state);

            if(Pet* pet = GetPet())
                pet->SetPvP(state);
            if(Unit* charmed = GetCharm())
                charmed->SetPvP(state);
        }
    }
}

void Player::AddSpellCooldown(uint32 spellid, uint32 itemid, time_t end_time)
{
    SpellCooldown sc;
    sc.end = end_time;
    sc.itemid = itemid;
    m_spellCooldowns[spellid] = sc;
}

void Player::SendCooldownEvent(SpellEntry const *spellInfo)
{
    if ( !(spellInfo->Attributes & SPELL_ATTR_DISABLED_WHILE_ACTIVE) )
        return;

    // Get spell cooldown
    int32 cooldown = GetSpellRecoveryTime(spellInfo);
    // Apply spellmods
    ApplySpellMod(spellInfo->Id, SPELLMOD_COOLDOWN, cooldown);
    if (cooldown < 0)
        cooldown = 0;
    // Add cooldown
    AddSpellCooldown(spellInfo->Id, 0, time(NULL) +  cooldown / 1000);
    // Send activate
    WorldPacket data(SMSG_COOLDOWN_EVENT, (4+8));
    data << spellInfo->Id;
    data << GetGUID();
    SendDirectMessage(&data);
}
                                                           //slot to be excluded while counting
bool Player::EnchantmentFitsRequirements(uint32 enchantmentcondition, int8 slot)
{
    if(!enchantmentcondition)
        return true;

    SpellItemEnchantmentConditionEntry const *Condition = sSpellItemEnchantmentConditionStore.LookupEntry(enchantmentcondition);

    if(!Condition)
        return true;

    uint8 curcount[4] = {0, 0, 0, 0};

    //counting current equipped gem colors
    for(uint8 i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
    {
        if(i == slot)
            continue;
        Item *pItem2 = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
        if(pItem2 && pItem2->GetProto()->Socket[0].Color)
        {
            for(uint32 enchant_slot = SOCK_ENCHANTMENT_SLOT; enchant_slot < SOCK_ENCHANTMENT_SLOT+3; ++enchant_slot)
            {
                uint32 enchant_id = pItem2->GetEnchantmentId(EnchantmentSlot(enchant_slot));
                if(!enchant_id)
                    continue;

                SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
                if(!enchantEntry)
                    continue;

                uint32 gemid = enchantEntry->GemID;
                if(!gemid)
                    continue;

                ItemPrototype const* gemProto = sItemStorage.LookupEntry<ItemPrototype>(gemid);
                if(!gemProto)
                    continue;

                GemPropertiesEntry const* gemProperty = sGemPropertiesStore.LookupEntry(gemProto->GemProperties);
                if(!gemProperty)
                    continue;

                uint8 GemColor = gemProperty->color;

                for(uint8 b = 0, tmpcolormask = 1; b < 4; b++, tmpcolormask <<= 1)
                {
                    if(tmpcolormask & GemColor)
                        ++curcount[b];
                }
            }
        }
    }

    bool activate = true;

    for(int i = 0; i < 5; i++)
    {
        if(!Condition->Color[i])
            continue;

        uint32 _cur_gem = curcount[Condition->Color[i] - 1];

        // if have <CompareColor> use them as count, else use <value> from Condition
        uint32 _cmp_gem = Condition->CompareColor[i] ? curcount[Condition->CompareColor[i] - 1]: Condition->Value[i];

        switch(Condition->Comparator[i])
        {
            case 2:                                         // requires less <color> than (<value> || <comparecolor>) gems
                activate &= (_cur_gem < _cmp_gem) ? true : false;
                break;
            case 3:                                         // requires more <color> than (<value> || <comparecolor>) gems
                activate &= (_cur_gem > _cmp_gem) ? true : false;
                break;
            case 5:                                         // requires at least <color> than (<value> || <comparecolor>) gems
                activate &= (_cur_gem >= _cmp_gem) ? true : false;
                break;
        }
    }

    return activate;
}

void Player::CorrectMetaGemEnchants(uint8 exceptslot, bool apply)
{
                                                            //cycle all equipped items
    for(uint32 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
    {
        //enchants for the slot being socketed are handled by Player::ApplyItemMods
        if(slot == exceptslot)
            continue;

        Item* pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, slot );

        if(!pItem || !pItem->GetProto()->Socket[0].Color)
            continue;

        for(uint32 enchant_slot = SOCK_ENCHANTMENT_SLOT; enchant_slot < SOCK_ENCHANTMENT_SLOT+3; ++enchant_slot)
        {
            uint32 enchant_id = pItem->GetEnchantmentId(EnchantmentSlot(enchant_slot));
            if(!enchant_id)
                continue;

            SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
            if(!enchantEntry)
                continue;

            uint32 condition = enchantEntry->EnchantmentCondition;
            if(condition)
            {
                                                            //was enchant active with/without item?
                bool wasactive = EnchantmentFitsRequirements(condition, apply ? exceptslot : -1);
                                                            //should it now be?
                if(wasactive ^ EnchantmentFitsRequirements(condition, apply ? -1 : exceptslot))
                {
                    // ignore item gem conditions
                                                            //if state changed, (dis)apply enchant
                    ApplyEnchantment(pItem,EnchantmentSlot(enchant_slot),!wasactive,true,true);
                }
            }
        }
    }
}

                                                            //if false -> then toggled off if was on| if true -> toggled on if was off AND meets requirements
void Player::ToggleMetaGemsActive(uint8 exceptslot, bool apply)
{
    //cycle all equipped items
    for(int slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
    {
        //enchants for the slot being socketed are handled by WorldSession::HandleSocketOpcode(WorldPacket& recv_data)
        if(slot == exceptslot)
            continue;

        Item *pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, slot );

        if(!pItem || !pItem->GetProto()->Socket[0].Color)   //if item has no sockets or no item is equipped go to next item
            continue;

        //cycle all (gem)enchants
        for(uint32 enchant_slot = SOCK_ENCHANTMENT_SLOT; enchant_slot < SOCK_ENCHANTMENT_SLOT+3; ++enchant_slot)
        {
            uint32 enchant_id = pItem->GetEnchantmentId(EnchantmentSlot(enchant_slot));
            if(!enchant_id)                                 //if no enchant go to next enchant(slot)
                continue;

            SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
            if(!enchantEntry)
                continue;

            //only metagems to be (de)activated, so only enchants with condition
            uint32 condition = enchantEntry->EnchantmentCondition;
            if(condition)
                ApplyEnchantment(pItem,EnchantmentSlot(enchant_slot), apply);
        }
    }
}

void Player::LeaveBattleground(bool teleportToEntryPoint)
{
	if(BattleGround *bg = GetBattleGround())
    {
		if (bg->isSpectator(GetGUID()))
			return;

        bool need_debuf = bg->isBattleGround() && !isGameMaster() && ((bg->GetStatus() == STATUS_IN_PROGRESS) || (bg->GetStatus() == STATUS_WAIT_JOIN)) && sWorld.getConfig(CONFIG_BATTLEGROUND_CAST_DESERTER);

        if(bg->isArena() && bg->isRated() && bg->GetStatus() == STATUS_WAIT_JOIN) //if game has not end then make sure that personal raiting is decreased
		{
			//decrease private raiting here
			Team Loser = (Team)bg->GetPlayerTeam(GetGUID());
			Team Winner = Loser == ALLIANCE ? HORDE : ALLIANCE;
			ArenaTeam* WinnerTeam = objmgr.GetArenaTeamById(bg->GetArenaTeamIdForTeam(Winner));
			ArenaTeam* LoserTeam = objmgr.GetArenaTeamById(bg->GetArenaTeamIdForTeam(Loser));
            if (WinnerTeam && LoserTeam)
    			LoserTeam->MemberLost(this,WinnerTeam->GetStats().rating);
		}
        if (bg->GetTypeID() == BATTLEGROUND_WS) {
            RemoveAurasDueToSpell(46392);
            RemoveAurasDueToSpell(46393);
        }
		bg->RemovePlayerAtLeave(GetGUID(), teleportToEntryPoint, true);

        // call after remove to be sure that player resurrected for correct cast
        if(need_debuf && !sWorld.IsShuttingDown())
            CastSpell(this, 26013, true);                   // Deserter
    }
}

bool Player::CanJoinToBattleground() const
{
    // check Deserter debuff
    if(GetDummyAura(26013))
        return false;

    return true;
}

bool Player::CanReportAfkDueToLimit()
{
    // a player can complain about 15 people per 5 minutes
    if(m_bgAfkReportedCount >= 15)
        return false;
    ++m_bgAfkReportedCount;
    return true;
}

///This player has been blamed to be inactive in a battleground
void Player::ReportedAfkBy(Player* reporter)
{
    BattleGround *bg = GetBattleGround();
    if(!bg || bg != reporter->GetBattleGround() || GetTeam() != reporter->GetTeam())
        return;

    // check if player has 'Idle' or 'Inactive' debuff
    if(m_bgAfkReporter.find(reporter->GetGUIDLow())==m_bgAfkReporter.end() && !HasAura(SPELL_AURA_PLAYER_IDLE,0) && !HasAura(SPELL_AURA_PLAYER_INACTIVE,0) && reporter->CanReportAfkDueToLimit())
    {
        m_bgAfkReporter.insert(reporter->GetGUIDLow());
        // 3 players have to complain to apply debuff
        if(m_bgAfkReporter.size() >= 3)
        {
            // cast 'Idle' spell
            CastSpell(this, SPELL_AURA_PLAYER_IDLE, true);
            m_bgAfkReporter.clear();
        }
    }
}

bool Player::canSeeOrDetect(Unit const* u, bool detect, bool inVisibleList, bool is3dDistance) const
{
    // Always can see self
    if (u == this)
        return true;

    // Check spectator case
    if (GetBattleGround())
    {
    	if (GetBattleGround()->isSpectator(GetGUID()))
    	{
    		if (GetBattleGround()->isSpectator(u->GetGUID()))
    		    return false;
    	}
    	else
    	{
    		if (!isGameMaster() && GetBattleGround()->isSpectator(u->GetGUID()))
    		    return false;
    	}
    }

    // Arena visibility before arena start
    if (InArena() && GetBattleGround() && GetBattleGround()->GetStatus() == STATUS_WAIT_JOIN) {
        if (const Player* target = u->GetCharmerOrOwnerPlayerOrPlayerItself()) {
            if (target->isGameMaster())
                return false;
            else
                return GetBGTeam() == target->GetBGTeam();
        }
    }

    // player visible for other player if not logout and at same transport
    // including case when player is out of world
    bool at_same_transport =
        GetTransport() && u->GetTypeId() == TYPEID_PLAYER
        && !GetSession()->PlayerLogout() && !(u->ToPlayer())->GetSession()->PlayerLogout()
        && !GetSession()->PlayerLoading() && !(u->ToPlayer())->GetSession()->PlayerLoading()
        && GetTransport() == (u->ToPlayer())->GetTransport();

    // not in world
    if(!at_same_transport && (!IsInWorld() || !u->IsInWorld()))
        return false;

    // forbidden to seen (at GM respawn command)
    if(u->GetVisibility() == VISIBILITY_RESPAWN)
        return false;

    // always seen by owner
    if(GetGUID() == u->GetCharmerOrOwnerGUID())
        return true;

    Map& _map = *u->GetMap();
    // Grid dead/alive checks
    // non visible at grid for any stealth state
    if(!u->IsVisibleInGridForPlayer(this))
        return false;

    // If the player is currently channeling vision, update visibility from the target unit's location
    const WorldObject* target = GetFarsightTarget();
    if (!target || !HasFarsightVision()) // Vision needs to be on the farsight target
        target = this;

    // different visible distance checks
    if(isInFlight())                                     // what see player in flight
    {
        if (!target->IsWithinDistInMap(u,_map.GetVisibilityDistance()+(inVisibleList ? World::GetVisibleObjectGreyDistance() : 0.0f), is3dDistance))
            return false;
    }
    else if(!u->isAlive())                                     // distance for show body
    {
        if (!target->IsWithinDistInMap(u,_map.GetVisibilityDistance()+(inVisibleList ? World::GetVisibleObjectGreyDistance() : 0.0f), is3dDistance))
            return false;
    }
    else if(u->GetTypeId()==TYPEID_PLAYER)                     // distance for show player
    {
        // Players far than max visible distance for player or not in our map are not visible too
        if (!at_same_transport && !target->IsWithinDistInMap(u,_map.GetVisibilityDistance()+(inVisibleList ? World::GetVisibleUnitGreyDistance() : 0.0f), is3dDistance))
            return false;
    }
    else if(u->GetCharmerOrOwnerGUID())                        // distance for show pet/charmed
    {
        // Pet/charmed far than max visible distance for player or not in our map are not visible too
        if (!target->IsWithinDistInMap(u,_map.GetVisibilityDistance()+(inVisibleList ? World::GetVisibleUnitGreyDistance() : 0.0f), is3dDistance))
            return false;
    }
    else                                                    // distance for show creature
    {
        // Units far than max visible distance for creature or not in our map are not visible too
        if (!target->IsWithinDistInMap(u
            , u->isActiveObject() ? (MAX_VISIBILITY_DISTANCE - (inVisibleList ? 0.0f : World::GetVisibleUnitGreyDistance()))
            : (_map.GetVisibilityDistance() + (inVisibleList ? World::GetVisibleUnitGreyDistance() : 0.0f))
            , is3dDistance))
            return false;
    }

    if(u->GetVisibility() == VISIBILITY_OFF)
    {
        // GMs see all unit. Moderators can see all units except higher gm's.
        if(isGameMaster())
        {
            if(u->GetTypeId() == TYPEID_PLAYER
              && GetSession()->GetSecurity() == SEC_MODERATOR
              && u->ToPlayer()->GetSession()->GetSecurity() > SEC_MODERATOR)
                return false;
            else
                return true;
        }
        else if (sWorld.getConfig(CONFIG_ARENA_SPECTATOR_GHOST) && isSpectator())
        {
        	if(u->GetTypeId() == TYPEID_PLAYER
        	  && GetSession()->GetSecurity() < u->ToPlayer()->GetSession()->GetSecurity())
        		return false;
        	else
        		return true;
        }
        return false;
    }

    Player *p = const_cast<Player*>(u->ToPlayer());

    // GM's can see everyone with invisibilitymask. Moderator can only see those with less or equal security level.
    if(m_invisibilityMask || u->m_invisibilityMask)
    {
        if(isGameMaster())
        {
            if(u->GetTypeId() == TYPEID_PLAYER
              && GetSession()->GetSecurity() == SEC_MODERATOR
              && u->ToPlayer()->GetSession()->GetSecurity() > SEC_MODERATOR)
                return false;
            else
                return true;
        }
        else if (sWorld.getConfig(CONFIG_ARENA_SPECTATOR_GHOST) && isSpectator())
        {
        	if(u->GetTypeId() == TYPEID_PLAYER
        	  && GetSession()->GetSecurity() < u->ToPlayer()->GetSession()->GetSecurity())
        	    return false;
        	else
        	    return true;
        }

        // player see other player with stealth/invisibility only if he in same group or raid or same team (raid/team case dependent from conf setting)
        if(!canDetectInvisibilityOf(u))
            if(!(u->GetTypeId()==TYPEID_PLAYER && !IsHostileTo(u) && IsGroupVisibleFor(p)))
                return false;
    }

    //duel zone case
    if(duel && duel->startTime && isInPvPZone())
    {
        if(u->ToPlayer() && duel->opponent != u->ToPlayer()) //isn't opponent
            return false;
        if(u->ToCreature() && u->ToCreature()->GetCharmerOrOwnerOrOwnGUID() != duel->opponent->GetGUID()) //isn't charmed by opponent
            return false;
    }

    // GM invisibility checks early, invisibility if any detectable, so if not stealth then visible
    if(u->GetVisibility() == VISIBILITY_GROUP_STEALTH)
    {
    	if (!isGameMaster())
    	{
    		if (!isSpectator() || !sWorld.getConfig(CONFIG_ARENA_SPECTATOR_GHOST))
    		{
                // if player is dead then he can't detect anyone in any cases
                //do not know what is the use of this detect
                // stealth and detected and visible for some seconds
                if(!isAlive())
                    detect = false;
                if(m_DetectInvTimer < 300 || !HaveAtClient(u))
                    if(!(u->GetTypeId()==TYPEID_PLAYER && !IsHostileTo(u) && IsGroupVisibleFor(p)))
                        if(!detect || !canDetectStealthOf(u, GetDistance(u)))
                            return false;
    		}
    	}
    }

    // If use this server will be too laggy
    // Now check is target visible with LoS
    //return u->IsWithinLOS(GetPositionX(),GetPositionY(),GetPositionZ());
    return true;
}

bool Player::IsVisibleInGridForPlayer( Player const * pl ) const
{
    if(pl->isGameMaster())
    {
        // gamemaster in GM mode see all, including ghosts
        if(pl->GetSession()->GetSecurity() >= SEC_GAMEMASTER)
            return true;
        // (else) moderators cant see higher gm's
        if(GetSession()->GetSecurity() <= pl->GetSession()->GetSecurity())
            return true;
    }
    else if (sWorld.getConfig(CONFIG_ARENA_SPECTATOR_GHOST) && pl->isSpectator())
    {
    	if(GetSession()->GetSecurity() <= pl->GetSession()->GetSecurity())
    		return true;
    }

    // It seems in battleground everyone sees everyone, except the enemy-faction ghosts
    if (InBattleGround())
    {
        if (!(isAlive() || m_deathTimer > 0) && !IsFriendlyTo(pl) )
            return false;
        return true;
    }

    // Live player see live player or dead player with not realized corpse
    if(pl->isAlive() || pl->m_deathTimer > 0)
    {
        return isAlive() || m_deathTimer > 0;
    }

    // Ghost see other friendly ghosts, that's for sure
    if(!(isAlive() || m_deathTimer > 0) && IsFriendlyTo(pl))
        return true;

    // Dead player see live players near own corpse
    if(isAlive())
    {
        Corpse *corpse = pl->GetCorpse();
        if(corpse)
        {
            // 20 - aggro distance for same level, 25 - max additional distance if player level less that creature level
            if(corpse->IsWithinDistInMap(this,(20+25)*sWorld.getRate(RATE_CREATURE_AGGRO)))
                return true;
        }
    }

    // and not see any other
    return false;
}

bool Player::IsVisibleGloballyFor( Player* u ) const
{
    if(!u)
        return false;

    // Always can see self
    if (u==this)
        return true;

    // Visible units, always are visible for all players
    if (GetVisibility() == VISIBILITY_ON)
        return true;

    //GMs can always see everyone
     if (u->GetSession()->GetSecurity() >= SEC_GAMEMASTER)
        return true;

     //moderators can see everyone except higher GMs
     if (GetSession()->GetSecurity() == SEC_MODERATOR && u->GetSession()->GetSecurity() >= SEC_MODERATOR)
        return true;

    // non faction visibility non-breakable for non-GMs
    if (GetVisibility() == VISIBILITY_OFF)
        return false;

    // non-gm stealth/invisibility not hide from global player lists
    return true;
}

void Player::UpdateVisibilityOf(WorldObject* target)
{
    m_GiantLock.acquire();
    if(HaveAtClient(target))
    {
        if(!target->isVisibleForInState(this,true))
        {
            target->DestroyForPlayer(this);
            m_clientGUIDs.erase(target->GetGUID());

            #ifdef TRINITY_DEBUG
            if((sLog.getLogFilter() & LOG_FILTER_VISIBILITY_CHANGES)==0)
                sLog.outDebug("Object %u (Type: %u) out of range for player %u. Distance = %f",target->GetGUIDLow(),target->GetTypeId(),GetGUIDLow(),GetDistance(target));
            #endif
        }
    }
    else
    {
        if(target->isVisibleForInState(this,false))
        {
            target->SendUpdateToPlayer(this);
            if(target->GetTypeId()!=TYPEID_GAMEOBJECT||!((GameObject*)target)->IsTransport())
                m_clientGUIDs.insert(target->GetGUID());

            #ifdef TRINITY_DEBUG
            if((sLog.getLogFilter() & LOG_FILTER_VISIBILITY_CHANGES)==0)
                sLog.outDebug("Object %u (Type: %u) is visible now for player %u. Distance = %f",target->GetGUIDLow(),target->GetTypeId(),GetGUIDLow(),GetDistance(target));
            #endif

            // target aura duration for caster show only if target exist at caster client
            // send data at target visibility change (adding to client)
            if(target->isType(TYPEMASK_UNIT))
                SendInitialVisiblePackets((Unit*)target);
        }
    }
    m_GiantLock.release();
}

void Player::SendInitialVisiblePackets(Unit* target)
{
    SendAuraDurationsForTarget(target);

    if (BattleGround *bg = GetBattleGround())
    {
    	if (bg->isSpectator(GetGUID()))
    	{
            for(uint8 i = 0; i < MAX_AURAS; ++i)
            {
                if(uint32 auraId = target->GetUInt32Value(UNIT_FIELD_AURA + i))
                {
        	        if (Player *stream = target->ToPlayer())
        	        {
        	        	if (bg->isSpectator(stream->GetGUID()))
        	        		continue;

        	    	    AuraMap& Auras = target->GetAuras();

        	    	    for(AuraMap::iterator iter = Auras.begin(); iter != Auras.end(); ++iter)
        	    	    {
        	    	        if (iter->second->GetId() == auraId)
		                    {
        	                    Aura* aura = iter->second;

        	                    SpectatorAddonMsg msg;
        	                    uint64 casterID = 0;
        	                    if (aura->GetCaster())
        	                        casterID = (aura->GetCaster()->ToPlayer()) ? aura->GetCaster()->GetGUID() : 0;
        	                    msg.SetPlayer(stream->GetName());
        	                    msg.CreateAura(casterID, aura->GetSpellProto()->Id,
        	                                   aura->IsPositive(), aura->GetSpellProto()->Dispel,
        	                                   aura->GetAuraDuration(), aura->GetAuraMaxDuration(),
        	                                   aura->GetStackAmount(), false);
        	                    msg.SendPacket(GetGUID());
        	                }
        	            }
        	        }
                }
            }
        }
    }

    if(target->isAlive())
    {
        if(target->GetMotionMaster()->GetCurrentMovementGeneratorType() != IDLE_MOTION_TYPE)
            target->SendMonsterMoveWithSpeedToCurrentDestination(this);
        if(target->hasUnitState(UNIT_STAT_MELEE_ATTACKING) && target->getVictim())
            target->SendAttackStart(target->getVictim());
    }
}

template<class T>
inline void UpdateVisibilityOf_helper(std::set<uint64>& s64, T* target)
{
    s64.insert(target->GetGUID());
}

template<>
inline void UpdateVisibilityOf_helper(std::set<uint64>& s64, GameObject* target)
{
    if(!target->IsTransport())
        s64.insert(target->GetGUID());
}

template<class T>
void Player::UpdateVisibilityOf(T* target, UpdateData& data, std::set<WorldObject*>& visibleNow)
{
    if(!target)
    return;
    m_GiantLock.acquire();
    if(HaveAtClient(target))
    {
        if(!target->isVisibleForInState(this,true))
        {
            target->BuildOutOfRangeUpdateBlock(&data);
            m_clientGUIDs.erase(target->GetGUID());

            #ifdef TRINITY_DEBUG
            if((sLog.getLogFilter() & LOG_FILTER_VISIBILITY_CHANGES)==0)
                sLog.outDebug("Object %u (Type: %u, Entry: %u) is out of range for player %u. Distance = %f",target->GetGUIDLow(),target->GetTypeId(),target->GetEntry(),GetGUIDLow(),GetDistance(target));
            #endif
        }
    }
    else if(visibleNow.size() < 30)
    {
        if(target->isVisibleForInState(this,false))
        {
            visibleNow.insert(target);
            target->BuildCreateUpdateBlockForPlayer(&data, this);
            UpdateVisibilityOf_helper(m_clientGUIDs,target);

            #ifdef TRINITY_DEBUG
            if((sLog.getLogFilter() & LOG_FILTER_VISIBILITY_CHANGES)==0)
                sLog.outDebug("Object %u (Type: %u, Entry: %u) is visible now for player %u. Distance = %f",target->GetGUIDLow(),target->GetTypeId(),target->GetEntry(),GetGUIDLow(),GetDistance(target));
            #endif
        }
    }
    m_GiantLock.release();
}

/*template<>
void Player::UpdateVisibilityOf<Creature>(Creature* target, UpdateData& data, UpdateDataMapType& data_updates, std::set<WorldObject*>& visibleNow)
{
    if(HaveAtClient(target))
    {
        if(!target->isVisibleForInState(this,true))
        {
            target->DestroyForPlayer(this);
            target->BuildOutOfRangeUpdateBlock(&data);
            m_clientGUIDs.erase(target->GetGUID());

            #ifdef TRINITY_DEBUG
            if((sLog.getLogFilter() & LOG_FILTER_VISIBILITY_CHANGES)==0)
                sLog.outDebug("Object %u (Type: %u, Entry: %u) is out of range for player %u. Distance = %f",target->GetGUIDLow(),target->GetTypeId(),target->GetEntry(),GetGUIDLow(),GetDistance(target));
            #endif
        }
    }
    else
    {
        if(target->isVisibleForInState(this,false))
        {
            visibleNow.insert(target);
            target->BuildUpdate(data_updates);
            target->SendUpdateToPlayer(this);
            target->SendMonsterMoveWithSpeedToCurrentDestination(this);
            UpdateVisibilityOf_helper(m_clientGUIDs,target);

            #ifdef TRINITY_DEBUG
            if((sLog.getLogFilter() & LOG_FILTER_VISIBILITY_CHANGES)==0)
                sLog.outDebug("Object %u (Type: %u, Entry: %u) is visible now for player %u. Distance = %f",target->GetGUIDLow(),target->GetTypeId(),target->GetEntry(),GetGUIDLow(),GetDistance(target));
            #endif
        }
    }
}*/

template void Player::UpdateVisibilityOf(Player*        target, UpdateData& data, std::set<WorldObject*>& visibleNow);
template void Player::UpdateVisibilityOf(Creature*      target, UpdateData& data, std::set<WorldObject*>& visibleNow);
template void Player::UpdateVisibilityOf(Corpse*        target, UpdateData& data, std::set<WorldObject*>& visibleNow);
template void Player::UpdateVisibilityOf(GameObject*    target, UpdateData& data, std::set<WorldObject*>& visibleNow);
template void Player::UpdateVisibilityOf(DynamicObject* target, UpdateData& data, std::set<WorldObject*>& visibleNow);

void Player::InitPrimaryProffesions()
{
    SetFreePrimaryProffesions(sWorld.getConfig(CONFIG_MAX_PRIMARY_TRADE_SKILL));
}

Unit* Player::GetSelectedUnit() const
{
    if (m_curSelection)
        return ObjectAccessor::GetUnit(*this, m_curSelection);
    return NULL;
}

Player* Player::GetSelectedPlayer() const
{
    if (m_curSelection)
        return ObjectAccessor::GetPlayer(*this, m_curSelection);
    return NULL;
}

void Player::SendComboPoints()
{
    Unit *combotarget = ObjectAccessor::GetUnit(*this, m_comboTarget);
    if (combotarget)
    {
        WorldPacket data(SMSG_UPDATE_COMBO_POINTS, combotarget->GetPackGUID().size()+1);
        data.append(combotarget->GetPackGUID());
        data << uint8(m_comboPoints);
        GetSession()->SendPacket(&data);
    }
}

void Player::AddComboPoints(Unit* target, int8 count, bool forceCurrent /* = false */) // forceCurrent: forces combo add on current combo target (fixes rogue's Setup)
{
    if (!count)
        return;

    // without combo points lost (duration checked in aura)
    RemoveSpellsCausingAura(SPELL_AURA_RETAIN_COMBO_POINTS);

    if (target->GetGUID() == m_comboTarget)
        m_comboPoints += count;
    else if (!forceCurrent || !m_comboTarget) { // Accept this only if not force current or no current combo target
        if (m_comboTarget) {
            if (Unit* target = ObjectAccessor::GetUnit(*this, m_comboTarget))
                target->RemoveComboPointHolder(GetGUIDLow());
        }

        m_comboTarget = target->GetGUID();
        m_comboPoints = count;

        target->AddComboPointHolder(GetGUIDLow());
    }

    if (m_comboPoints > 5) m_comboPoints = 5;
    if (m_comboPoints < 0) m_comboPoints = 0;

    SendComboPoints();
}

void Player::ClearComboPoints(uint32 spellId)
{
    if(!m_comboTarget)
        return;

    // without combopoints lost (duration checked in aura)
    RemoveSpellsCausingAura(SPELL_AURA_RETAIN_COMBO_POINTS);

    m_comboPoints = 0;

    SendComboPoints();

    if(Unit* target = ObjectAccessor::GetUnit(*this,m_comboTarget))
        target->RemoveComboPointHolder(GetGUIDLow());

    m_comboTarget = 0;
    
    //handle Ruthlessness - shouldn't proc on Deadly Throw
    if (spellId != 26679 && spellId != 48673) {
        if(HasSpell(14156) /*rank 1, 20%*/ || HasSpell(14160) /*Rank 2, 40%*/ || HasSpell(14161) /*Rank 3, 60% */)
        {
            uint32 procChance = urand(1,100);
            if ( (HasSpell(14161) && procChance <= 60) || (HasSpell(14160) && procChance <= 40) || (HasSpell(14156) && procChance <= 20) )
            {
                if (this->getVictim())
                    AddComboPoints(this->getVictim(), 1);
            }
        }
    }
}

void Player::SetGroup(Group *group, int8 subgroup)
{
    if(group == NULL) m_group.unlink();
    else
    {
        // never use SetGroup without a subgroup unless you specify NULL for group
        assert(subgroup >= 0);
        m_group.link(group, this);
        m_group.setSubGroup((uint8)subgroup);
    }
}

void Player::SendInitialPacketsBeforeAddToMap()
{
    WorldPacket data(SMSG_SET_REST_START, 4);
    data << uint32(0);                                      // unknown, may be rest state time or experience
    GetSession()->SendPacket(&data);

    // Homebind
    data.Initialize(SMSG_BINDPOINTUPDATE, 5*4);
    data << m_homebindX << m_homebindY << m_homebindZ;
    data << (uint32) m_homebindMapId;
    data << (uint32) m_homebindAreaId;
    GetSession()->SendPacket(&data);

    // SMSG_SET_PROFICIENCY
    // SMSG_UPDATE_AURA_DURATION

    // tutorial stuff
    data.Initialize(SMSG_TUTORIAL_FLAGS, 8*4);
    for (int i = 0; i < 8; ++i)
        data << uint32( GetTutorialInt(i) );
    GetSession()->SendPacket(&data);

    SendInitialSpells();

    data.Initialize(SMSG_SEND_UNLEARN_SPELLS, 4);
    data << uint32(0);                                      // count, for(count) uint32;
    GetSession()->SendPacket(&data);

    SendInitialActionButtons();
    SendInitialReputations();
    UpdateZone(GetZoneId());
    SendInitWorldStates();

    // SMSG_SET_AURA_SINGLE

    data.Initialize(SMSG_LOGIN_SETTIMESPEED, 8);
    data << uint32(secsToTimeBitFields(sWorld.GetGameTime()));
    data << (float)0.01666667f;                             // game speed
    GetSession()->SendPacket( &data );

    data.Initialize(SMSG_UPDATE_WORLD_STATE, 4+4);
    data << uint32(3191);
    data << uint32(sWorld.getConfig(CONFIG_ARENA_SEASON));
    GetSession()->SendPacket(&data);

    // set fly flag if in fly form or taxi flight to prevent visually drop at ground in showup moment
    if(HasAuraType(SPELL_AURA_MOD_INCREASE_FLIGHT_SPEED) || isInFlight())
        AddUnitMovementFlag(MOVEMENTFLAG_FLYING2);
}

void Player::SendInitialPacketsAfterAddToMap()
{
    CastSpell(this, 836, true);                             // LOGINEFFECT

    // set some aura effects that send packet to player client after add player to map
    // SendMessageToSet not send it to player not it map, only for aura that not changed anything at re-apply
    // same auras state lost at far teleport, send it one more time in this case also
    static const AuraType auratypes[] =
    {
        SPELL_AURA_MOD_FEAR,     SPELL_AURA_TRANSFORM,                 SPELL_AURA_WATER_WALK,
        SPELL_AURA_FEATHER_FALL, SPELL_AURA_HOVER,                     SPELL_AURA_SAFE_FALL,
        SPELL_AURA_FLY,          SPELL_AURA_MOD_INCREASE_FLIGHT_SPEED, SPELL_AURA_NONE
    };
    for(AuraType const* itr = &auratypes[0]; itr && itr[0] != SPELL_AURA_NONE; ++itr)
    {
        Unit::AuraList const& auraList = GetAurasByType(*itr);
        if(!auraList.empty())
            auraList.front()->ApplyModifier(true,true);
    }

    if(HasAuraType(SPELL_AURA_MOD_STUN))
        SetMovement(MOVE_ROOT);

    // manual send package (have code in ApplyModifier(true,true); that don't must be re-applied.
    if(HasAuraType(SPELL_AURA_MOD_ROOT))
    {
        WorldPacket data(SMSG_FORCE_MOVE_ROOT, 10);
        data.append(GetPackGUID());
        data << (uint32)2;
        SendMessageToSet(&data,true);
    }

    // setup BG group membership if need
    if(BattleGround* currentBg = GetBattleGround())
    {
        // call for invited (join) or listed (relogin) and avoid other cases (GM teleport)
        if (IsInvitedForBattleGroundInstance(GetBattleGroundId()) ||
            currentBg->IsPlayerInBattleGround(GetGUID()))
        {
            currentBg->PlayerRelogin(GetGUID());
            if(currentBg->GetMapId() == GetMapId())             // we teleported/login to/in bg
            {
                SQLTransaction trans = CharacterDatabase.BeginTransaction();
                uint32 team = currentBg->GetPlayerTeam(GetGUID());
                if(!team)
                    team = GetTeam();
                Group* group = currentBg->GetBgRaid(team);
                if(!group)                                      // first player joined
                {
                    group = new Group;
                    currentBg->SetBgRaid(team, group);
                    group->Create(GetGUIDLow(), GetName(), trans);
                }
                else                                            // raid already exist
                {
                    if(group->IsMember(GetGUID()))
                    {
                        uint8 subgroup = group->GetMemberGroup(GetGUID());
                        SetBattleGroundRaid(group, subgroup);
                    }
                    else
                        currentBg->GetBgRaid(team)->AddMember(GetGUID(), GetName(), trans);
                }
                CharacterDatabase.CommitTransaction(trans);
            }
        }
    }

    SendEnchantmentDurations();                             // must be after add to map
    SendItemDurations();                                    // must be after add to map
}

void Player::SendUpdateToOutOfRangeGroupMembers()
{
    if (m_groupUpdateMask == GROUP_UPDATE_FLAG_NONE)
        return;
    if(Group* group = GetGroup())
        group->UpdatePlayerOutOfRange(this);

    m_groupUpdateMask = GROUP_UPDATE_FLAG_NONE;
    m_auraUpdateMask = 0;
    if(Pet *pet = GetPet())
        pet->ResetAuraUpdateMask();
}

void Player::SendTransferAborted(uint32 mapid, uint16 reason)
{
    WorldPacket data(SMSG_TRANSFER_ABORTED, 4+2);
    data << uint32(mapid);
    data << uint16(reason);                                 // transfer abort reason
    GetSession()->SendPacket(&data);
}

void Player::SendInstanceResetWarning(uint32 mapid, uint32 time)
{
    // type of warning, based on the time remaining until reset
    uint32 type;
    if(time > 3600)
        type = RAID_INSTANCE_WELCOME;
    else if(time > 900 && time <= 3600)
        type = RAID_INSTANCE_WARNING_HOURS;
    else if(time > 300 && time <= 900)
        type = RAID_INSTANCE_WARNING_MIN;
    else
        type = RAID_INSTANCE_WARNING_MIN_SOON;
    WorldPacket data(SMSG_RAID_INSTANCE_MESSAGE, 4+4+4);
    data << uint32(type);
    data << uint32(mapid);
    data << uint32(time);
    GetSession()->SendPacket(&data);
}

void Player::ApplyEquipCooldown( Item * pItem )
{
    for(int i = 0; i <5; ++i)
    {
        _Spell const& spellData = pItem->GetProto()->Spells[i];

        // no spell
        if( !spellData.SpellId )
            continue;

        // wrong triggering type (note: ITEM_SPELLTRIGGER_ON_NO_DELAY_USE not have cooldown)
        if( spellData.SpellTrigger != ITEM_SPELLTRIGGER_ON_USE )
            continue;

        AddSpellCooldown(spellData.SpellId, pItem->GetEntry(), time(NULL) + 30);

        WorldPacket data(SMSG_ITEM_COOLDOWN, 12);
        data << pItem->GetGUID();
        data << uint32(spellData.SpellId);
        GetSession()->SendPacket(&data);
    }
}

void Player::resetSpells()
{
    // not need after this call
    if(HasAtLoginFlag(AT_LOGIN_RESET_SPELLS))
    {
        m_atLoginFlags = m_atLoginFlags & ~AT_LOGIN_RESET_SPELLS;
        CharacterDatabase.PExecute("UPDATE characters SET at_login = at_login & ~ %u WHERE guid ='%u'", uint32(AT_LOGIN_RESET_SPELLS), GetGUIDLow());
    }

    // make full copy of map (spells removed and marked as deleted at another spell remove
    // and we can't use original map for safe iterative with visit each spell at loop end
    PlayerSpellMap smap = GetSpellMap();

    for(PlayerSpellMap::const_iterator iter = smap.begin();iter != smap.end(); ++iter)
        removeSpell(iter->first);                           // only iter->first can be accessed, object by iter->second can be deleted already

    learnDefaultSpells();
    learnQuestRewardedSpells();
}

void Player::learnDefaultSpells(bool loading)
{
    // learn default race/class spells
    PlayerInfo const *info = objmgr.GetPlayerInfo(getRace(),getClass());
    std::list<CreateSpellPair>::const_iterator spell_itr;
    for (spell_itr = info->spell.begin(); spell_itr!=info->spell.end(); ++spell_itr)
    {
        uint16 tspell = spell_itr->first;
        if (tspell)
        {
            if(loading || !spell_itr->second)               // not care about passive spells or loading case
                addSpell(tspell,spell_itr->second);
            else                                            // but send in normal spell in game learn case
                learnSpell(tspell);
        }
    }
}

void Player::learnQuestRewardedSpells(Quest const* quest)
{
    uint32 spell_id = quest->GetRewSpellCast();

    // skip quests without rewarded spell
    if( !spell_id )
        return;

    SpellEntry const *spellInfo = spellmgr.LookupSpell(spell_id);
    if(!spellInfo)
        return;

    // check learned spells state
    bool found = false;
    for(int i=0; i < 3; ++i)
    {
        if(spellInfo->Effect[i] == SPELL_EFFECT_LEARN_SPELL && !HasSpell(spellInfo->EffectTriggerSpell[i]))
        {
            found = true;
            break;
        }
    }

    // skip quests with not teaching spell or already known spell
    if(!found)
        return;

    // prevent learn non first rank unknown profession and second specialization for same profession)
    uint32 learned_0 = spellInfo->EffectTriggerSpell[0];
    if( spellmgr.GetSpellRank(learned_0) > 1 && !HasSpell(learned_0) )
    {
        // not have first rank learned (unlearned prof?)
        uint32 first_spell = spellmgr.GetFirstSpellInChain(learned_0);
        if( !HasSpell(first_spell) )
            return;

        SpellEntry const *learnedInfo = spellmgr.LookupSpell(learned_0);
        if(!learnedInfo)
            return;

        // specialization
        if(learnedInfo->Effect[0]==SPELL_EFFECT_TRADE_SKILL && learnedInfo->Effect[1]==0)
        {
            // search other specialization for same prof
            for(PlayerSpellMap::const_iterator itr = m_spells.begin(); itr != m_spells.end(); ++itr)
            {
                if(itr->second->state == PLAYERSPELL_REMOVED || itr->first==learned_0)
                    continue;

                SpellEntry const *itrInfo = spellmgr.LookupSpell(itr->first);
                if(!itrInfo)
                    return;

                // compare only specializations
                if(itrInfo->Effect[0]!=SPELL_EFFECT_TRADE_SKILL || itrInfo->Effect[1]!=0)
                    continue;

                // compare same chain spells
                if(spellmgr.GetFirstSpellInChain(itr->first) != first_spell)
                    continue;

                // now we have 2 specialization, learn possible only if found is lesser specialization rank
                if(!spellmgr.IsHighRankOfSpell(learned_0,itr->first))
                    return;
            }
        }
    }

    CastSpell( this, spell_id, true);
}

void Player::learnQuestRewardedSpells()
{
    // learn spells received from quest completing
    for(QuestStatusMap::const_iterator itr = mQuestStatus.begin(); itr != mQuestStatus.end(); ++itr)
    {
        // skip no rewarded quests
        if(!itr->second.m_rewarded)
            continue;

        Quest const* quest = objmgr.GetQuestTemplate(itr->first);
        if( !quest )
            continue;

        learnQuestRewardedSpells(quest);
    }
}

void Player::learnSkillRewardedSpells(uint32 skill_id )
{
    uint32 raceMask  = getRaceMask();
    uint32 classMask = getClassMask();
    for (uint32 j=0; j<sSkillLineAbilityStore.GetNumRows(); ++j)
    {
        SkillLineAbilityEntry const *pAbility = sSkillLineAbilityStore.LookupEntry(j);
        if (!pAbility || pAbility->skillId!=skill_id || pAbility->learnOnGetSkill != ABILITY_LEARNED_ON_GET_PROFESSION_SKILL)
            continue;
        // Check race if set
        if (pAbility->racemask && !(pAbility->racemask & raceMask))
            continue;
        // Check class if set
        if (pAbility->classmask && !(pAbility->classmask & classMask))
            continue;

        if (spellmgr.LookupSpell(pAbility->spellId))
        {
            // Ok need learn spell
            learnSpell(pAbility->spellId);
        }
    }
}

void Player::learnSkillRewardedSpells()
{
    for (uint16 i=0; i < PLAYER_MAX_SKILLS; i++)
    {
        if(!GetUInt32Value(PLAYER_SKILL_INDEX(i)))
            continue;

        uint32 pskill = GetUInt32Value(PLAYER_SKILL_INDEX(i)) & 0x0000FFFF;

        learnSkillRewardedSpells(pskill);
    }
}

void Player::SendAuraDurationsForTarget(Unit* target)
{
    for(Unit::AuraMap::const_iterator itr = target->GetAuras().begin(); itr != target->GetAuras().end(); ++itr)
    {
        Aura* aura = itr->second;
        if(aura->GetAuraSlot() >= MAX_AURAS || aura->IsPassive() || aura->GetCasterGUID()!=GetGUID())
            continue;

        aura->SendAuraDurationForCaster(this);
    }
}

void Player::SetDailyQuestStatus( uint32 quest_id )
{
    for(uint32 quest_daily_idx = 0; quest_daily_idx < PLAYER_MAX_DAILY_QUESTS; ++quest_daily_idx)
    {
        if(!GetUInt32Value(PLAYER_FIELD_DAILY_QUESTS_1+quest_daily_idx))
        {
            SetUInt32Value(PLAYER_FIELD_DAILY_QUESTS_1+quest_daily_idx,quest_id);
            m_lastDailyQuestTime = time(NULL);              // last daily quest time
            m_DailyQuestChanged = true;
            break;
        }
    }
}

void Player::ResetDailyQuestStatus()
{
    for(uint32 quest_daily_idx = 0; quest_daily_idx < PLAYER_MAX_DAILY_QUESTS; ++quest_daily_idx)
        SetUInt32Value(PLAYER_FIELD_DAILY_QUESTS_1+quest_daily_idx,0);

    // DB data deleted in caller
    m_DailyQuestChanged = false;
    m_lastDailyQuestTime = 0;
}

BattleGround* Player::GetBattleGround() const
{
    if(GetBattleGroundId()==0)
        return NULL;

    return sBattleGroundMgr.GetBattleGround(GetBattleGroundId());
}

bool Player::InArena() const
{
    BattleGround *bg = GetBattleGround();
    if(!bg || !bg->isArena())
        return false;

    return true;
}

bool Player::GetBGAccessByLevel(uint32 bgTypeId) const
{
    // get a template bg instead of running one
    BattleGround *bg = sBattleGroundMgr.GetBattleGroundTemplate(bgTypeId);
    if(!bg)
        return false;

    if(getLevel() < bg->GetMinLevel() || getLevel() > bg->GetMaxLevel())
        return false;

    return true;
}

uint32 Player::GetMinLevelForBattleGroundQueueId(uint32 queue_id)
{
    if(queue_id < 1)
        return 0;

    if(queue_id >=6)
        queue_id = 6;

    return 10*(queue_id+1);
}

uint32 Player::GetMaxLevelForBattleGroundQueueId(uint32 queue_id)
{
    if(queue_id >=6)
        return 255;                                         // hardcoded max level

    return 10*(queue_id+2)-1;
}

//TODO make this more generic - current implementation is wrong
uint32 Player::GetBattleGroundQueueIdFromLevel() const
{
    uint32 level = getLevel();
    if(level <= 19)
        return 0;
    else if (level > 69)
        return 6;
    else
        return level/10 - 1;                                // 20..29 -> 1, 30-39 -> 2, ...
    /*
    assert(bgTypeId < MAX_BATTLEGROUND_TYPES);
    BattleGround *bg = sBattleGroundMgr.GetBattleGroundTemplate(bgTypeId);
    assert(bg);
    return (getLevel() - bg->GetMinLevel()) / 10;*/
}

float Player::GetReputationPriceDiscount( Creature const* pCreature ) const
{
    FactionTemplateEntry const* vendor_faction = pCreature->getFactionTemplateEntry();
    if(!vendor_faction)
        return 1.0f;

    ReputationRank rank = GetReputationRank(vendor_faction->faction);
    if(rank <= REP_NEUTRAL)
        return 1.0f;

    return 1.0f - 0.05f* (rank - REP_NEUTRAL);
}

/* Warning : This is wrong for some spells such as draenei racials or paladin mount skills/spells */
bool Player::IsSpellFitByClassAndRace( uint32 spell_id ) const
{
    uint32 racemask  = getRaceMask();
    uint32 classmask = getClassMask();

    SkillLineAbilityMap::const_iterator lower = spellmgr.GetBeginSkillLineAbilityMap(spell_id);
    SkillLineAbilityMap::const_iterator upper = spellmgr.GetEndSkillLineAbilityMap(spell_id);

    for(SkillLineAbilityMap::const_iterator _spell_idx = lower; _spell_idx != upper; ++_spell_idx)
    {
        // skip wrong race skills
        if( _spell_idx->second->racemask && (_spell_idx->second->racemask & racemask) == 0)
            return false;

        // skip wrong class skills
        if( _spell_idx->second->classmask && (_spell_idx->second->classmask & classmask) == 0)
            return false;
    }
    return true;
}

bool Player::HasQuestForGO(int32 GOId)
{
    for( QuestStatusMap::iterator i = mQuestStatus.begin( ); i != mQuestStatus.end( ); ++i )
    {
        QuestStatusData qs=i->second;
        if (qs.m_status == QUEST_STATUS_INCOMPLETE)
        {
            Quest const* qinfo = objmgr.GetQuestTemplate(i->first);
            if(!qinfo)
                continue;

            if(GetGroup() && GetGroup()->isRaidGroup() && qinfo->GetType() != QUEST_TYPE_RAID)
                continue;

            for (int j = 0; j < QUEST_OBJECTIVES_COUNT; j++)
            {
                if (qinfo->ReqCreatureOrGOId[j]>=0)         //skip non GO case
                    continue;

                if((-1)*GOId == qinfo->ReqCreatureOrGOId[j] && qs.m_creatureOrGOcount[j] < qinfo->ReqCreatureOrGOCount[j])
                    return true;
            }
        }
    }
    return false;
}

void Player::UpdateForQuestsGO()
{
    if(m_clientGUIDs.empty())
        return;

    m_GiantLock.acquire();
    UpdateData udata;
    WorldPacket packet;
    for(ClientGUIDs::iterator itr=m_clientGUIDs.begin(); itr!=m_clientGUIDs.end(); ++itr)
    {
        if(IS_GAMEOBJECT_GUID(*itr))
        {
            GameObject *obj = HashMapHolder<GameObject>::Find(*itr);
            if(obj)
                obj->BuildValuesUpdateBlockForPlayer(&udata,this);
        }
    }
    udata.BuildPacket(&packet);
    GetSession()->SendPacket(&packet);
    m_GiantLock.release();
}

void Player::SummonIfPossible(bool agree)
{
    // expire and auto declined
    if(m_summon_expire < time(NULL))
        return;

    if(!agree)
    {
        m_summon_expire = 0;
        return;
    }

    // stop taxi flight at summon
    if(isInFlight())
    {
        GetMotionMaster()->MovementExpired();
        CleanupAfterTaxiFlight();
    }

    // drop flag at summon
    if(BattleGround *bg = GetBattleGround())
        bg->EventPlayerDroppedFlag(this);

    m_summon_expire = 0;

    TeleportTo(m_summon_mapid, m_summon_x, m_summon_y, m_summon_z,GetOrientation());
    m_invite_summon = false;
}

void Player::RemoveItemDurations( Item *item )
{
    for(ItemDurationList::iterator itr = m_itemDuration.begin();itr != m_itemDuration.end(); ++itr)
    {
        if(*itr==item)
        {
            m_itemDuration.erase(itr);
            break;
        }
    }
}

void Player::AddItemDurations( Item *item )
{
    if(item->GetUInt32Value(ITEM_FIELD_DURATION))
    {
        m_itemDuration.push_back(item);
        item->SendTimeUpdate(this);
    }
}

void Player::AutoUnequipOffhandIfNeed()
{
    Item *offItem = GetItemByPos( INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND );
    if(!offItem)
        return;

    Item *mainItem = GetItemByPos( INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND );

    if(!mainItem || mainItem->GetProto()->InventoryType != INVTYPE_2HWEAPON)
        return;

    ItemPosCountVec off_dest;
    uint8 off_msg = CanStoreItem( NULL_BAG, NULL_SLOT, off_dest, offItem, false );
    if( off_msg == EQUIP_ERR_OK )
    {
        RemoveItem(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND, true);
        StoreItem( off_dest, offItem, true );
    }
    else
    {
        sLog.outError("Player::EquipItem: Can's store offhand item at 2hand item equip for player (GUID: %u).",GetGUIDLow());
    }
}

OutdoorPvP * Player::GetOutdoorPvP() const
{
    return sOutdoorPvPMgr.GetOutdoorPvPToZoneId(GetZoneId());
}

bool Player::HasItemFitToSpellReqirements(SpellEntry const* spellInfo, Item const* ignoreItem)
{
    if(spellInfo->EquippedItemClass < 0)
        return true;

    // scan other equipped items for same requirements (mostly 2 daggers/etc)
    // for optimize check 2 used cases only
    switch(spellInfo->EquippedItemClass)
    {
        case ITEM_CLASS_WEAPON:
        {
            for(int i= EQUIPMENT_SLOT_MAINHAND; i < EQUIPMENT_SLOT_TABARD; ++i)
                if(Item *item = GetItemByPos( INVENTORY_SLOT_BAG_0, i ))
                    if(item!=ignoreItem && item->IsFitToSpellRequirements(spellInfo))
                        return true;
            break;
        }
        case ITEM_CLASS_ARMOR:
        {
            // tabard not have dependent spells
            for(int i= EQUIPMENT_SLOT_START; i< EQUIPMENT_SLOT_MAINHAND; ++i)
                if(Item *item = GetItemByPos( INVENTORY_SLOT_BAG_0, i ))
                    if(item!=ignoreItem && item->IsFitToSpellRequirements(spellInfo))
                        return true;

            // shields can be equipped to offhand slot
            if(Item *item = GetItemByPos( INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND))
                if(item!=ignoreItem && item->IsFitToSpellRequirements(spellInfo))
                    return true;

            // ranged slot can have some armor subclasses
            if(Item *item = GetItemByPos( INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_RANGED))
                if(item!=ignoreItem && item->IsFitToSpellRequirements(spellInfo))
                    return true;

            break;
        }
        default:
            sLog.outError("HasItemFitToSpellReqirements: Not handled spell requirement for item class %u",spellInfo->EquippedItemClass);
            break;
    }

    return false;
}

void Player::RemoveItemDependentAurasAndCasts( Item * pItem )
{
    AuraMap& auras = GetAuras();
    for(AuraMap::iterator itr = auras.begin(); itr != auras.end(); )
    {
        Aura* aura = itr->second;

        // skip passive (passive item dependent spells work in another way) and not self applied auras
        SpellEntry const* spellInfo = aura->GetSpellProto();
        if(aura->IsPassive() ||  aura->GetCasterGUID()!=GetGUID())
        {
            ++itr;
            continue;
        }

        // skip if not item dependent or have alternative item
        if(HasItemFitToSpellReqirements(spellInfo,pItem))
        {
            ++itr;
            continue;
        }

        // no alt item, remove aura, restart check
        RemoveAurasDueToSpell(aura->GetId());
        itr = auras.begin();
    }

    // currently casted spells can be dependent from item
    for (uint32 i = 0; i < CURRENT_MAX_SPELL; i++)
    {
        if( m_currentSpells[i] && m_currentSpells[i]->getState()!=SPELL_STATE_DELAYED &&
            !HasItemFitToSpellReqirements(m_currentSpells[i]->m_spellInfo,pItem) )
            InterruptSpell(i);
    }
}

uint32 Player::GetResurrectionSpellId()
{
    // search priceless resurrection possibilities
    uint32 prio = 0;
    uint32 spell_id = 0;
    AuraList const& dummyAuras = GetAurasByType(SPELL_AURA_DUMMY);
    for(AuraList::const_iterator itr = dummyAuras.begin(); itr != dummyAuras.end(); ++itr)
    {
        // Soulstone Resurrection                           // prio: 3 (max, non death persistent)
        if( prio < 2 && (*itr)->GetSpellProto()->SpellVisual == 99 && (*itr)->GetSpellProto()->SpellIconID == 92 )
        {
            switch((*itr)->GetId())
            {
                case 20707: spell_id =  3026; break;        // rank 1
                case 20762: spell_id = 20758; break;        // rank 2
                case 20763: spell_id = 20759; break;        // rank 3
                case 20764: spell_id = 20760; break;        // rank 4
                case 20765: spell_id = 20761; break;        // rank 5
                case 27239: spell_id = 27240; break;        // rank 6
                default:
                    sLog.outError("Unhandled spell %%u: S.Resurrection",(*itr)->GetId());
                    continue;
            }

            prio = 3;
        }
        // Twisting Nether                                  // prio: 2 (max)
        else if((*itr)->GetId()==23701 && roll_chance_i(10))
        {
            prio = 2;
            spell_id = 23700;
        }
    }

    // Reincarnation (passive spell)                        // prio: 1
    if(prio < 1 && HasSpell(20608) && !HasSpellCooldown(21169) && HasItemCount(17030,1))
        spell_id = 21169;

    return spell_id;
}

bool Player::RewardPlayerAndGroupAtKill(Unit* pVictim)
{
    bool PvP = pVictim->isCharmedOwnedByPlayerOrPlayer();

    // prepare data for near group iteration (PvP and !PvP cases)
    uint32 xp = 0;
    bool honored_kill = false;

    if(Group *pGroup = GetGroup())
    {
        uint32 count = 0;
        uint32 sum_level = 0;
        Player* member_with_max_level = NULL;
        Player* not_gray_member_with_max_level = NULL;

        pGroup->GetDataForXPAtKill(pVictim,count,sum_level,member_with_max_level,not_gray_member_with_max_level);

        if(member_with_max_level)
        {
            // PvP kills doesn't yield experience
            // also no XP gained if there is no member below gray level
            xp = (PvP || !not_gray_member_with_max_level) ? 0 : Trinity::XP::Gain(not_gray_member_with_max_level, pVictim);

            /// skip in check PvP case (for speed, not used)
            bool is_raid = PvP ? false : sMapStore.LookupEntry(GetMapId())->IsRaid() && pGroup->isRaidGroup();
            bool is_dungeon = PvP ? false : sMapStore.LookupEntry(GetMapId())->IsDungeon();
            float group_rate = Trinity::XP::xp_in_group_rate(count,is_raid);

            for(GroupReference *itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
            {
                Player* pGroupGuy = itr->getSource();
                if(!pGroupGuy)
                    continue;

                if(!pGroupGuy->IsAtGroupRewardDistance(pVictim))
                    continue;                               // member (alive or dead) or his corpse at req. distance

                // honor can be in PvP and !PvP (racial leader) cases (for alive)
                if(pGroupGuy->isAlive() && pGroupGuy->RewardHonor(pVictim,count, -1, true) && pGroupGuy==this)
                    honored_kill = true;

                // xp and reputation only in !PvP case
                if(!PvP)
                {
                    float rate = group_rate * float(pGroupGuy->getLevel()) / sum_level;

                    // if is in dungeon then all receive full reputation at kill
                    // rewarded any alive/dead/near_corpse group member
                    pGroupGuy->RewardReputation(pVictim,is_dungeon ? 1.0f : rate);

                    // XP updated only for alive group member
                    if(pGroupGuy->isAlive() && not_gray_member_with_max_level &&
                       pGroupGuy->getLevel() <= not_gray_member_with_max_level->getLevel())
                    {
                        uint32 itr_xp = (member_with_max_level == not_gray_member_with_max_level) ? uint32(xp*rate) : uint32((xp*rate/2)+1);

                        pGroupGuy->GiveXP(itr_xp, pVictim);
                        if(Pet* pet = pGroupGuy->GetPet())
                            pet->GivePetXP(itr_xp/2);
                    }

                    // quest objectives updated only for alive group member or dead but with not released body
                    if(pGroupGuy->isAlive()|| !pGroupGuy->GetCorpse())
                    {
                        // normal creature (not pet/etc) can be only in !PvP case
                        if(pVictim->GetTypeId()==TYPEID_UNIT)
                            pGroupGuy->KilledMonster(pVictim->GetEntry(), pVictim->GetGUID());
                    }
                }
            }
        }
    }
    else                                                    // if (!pGroup)
    {
        xp = PvP ? 0 : Trinity::XP::Gain(this, pVictim);

        // honor can be in PvP and !PvP (racial leader) cases
        if(RewardHonor(pVictim,1, -1, true))
            honored_kill = true;

        // xp and reputation only in !PvP case
        if(!PvP)
        {
            RewardReputation(pVictim,1);
            GiveXP(xp, pVictim);

            if(Pet* pet = GetPet())
                pet->GivePetXP(xp);

            // normal creature (not pet/etc) can be only in !PvP case
            if(pVictim->GetTypeId()==TYPEID_UNIT)
                KilledMonster(pVictim->GetEntry(),pVictim->GetGUID());
        }
    }
    return xp || honored_kill;
}

void Player::RewardPlayerAndGroupAtEvent(uint32 creature_id, WorldObject* pRewardSource)
{
    if (!pRewardSource)
        return;
    uint64 creature_guid = (pRewardSource->GetTypeId() == TYPEID_UNIT) ? pRewardSource->GetGUID() : uint64(0);

    // prepare data for near group iteration
    if (Group *pGroup = GetGroup())
    {
        for (GroupReference *itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            Player* pGroupGuy = itr->getSource();
            if (!pGroupGuy)
                continue;

            if (!pGroupGuy->IsAtGroupRewardDistance(pRewardSource))
                continue;                               // member (alive or dead) or his corpse at req. distance

            // quest objectives updated only for alive group member or dead but with not released body
            if (pGroupGuy->isAlive()|| !pGroupGuy->GetCorpse())
                pGroupGuy->KilledMonster(creature_id, creature_guid);
        }
    }
    else                                                    // if (!pGroup)
        KilledMonster(creature_id, creature_guid);
}

bool Player::IsAtGroupRewardDistance(WorldObject const* pRewardSource) const
{
    const WorldObject* player = GetCorpse();
    if(!player || isAlive())
        player = this;

    if(player->GetMapId() != pRewardSource->GetMapId() || player->GetInstanceId() != pRewardSource->GetInstanceId())
        return false;

    return pRewardSource->GetDistance(player) <= sWorld.getConfig(CONFIG_GROUP_XP_DISTANCE);
}

uint32 Player::GetBaseWeaponSkillValue (WeaponAttackType attType) const
{
    Item* item = GetWeaponForAttack(attType,true);

    // unarmed only with base attack
    if(attType != BASE_ATTACK && !item)
        return 0;

    // weapon skill or (unarmed for base attack)
    uint32  skill = item ? item->GetSkill() : SKILL_UNARMED;
    return GetBaseSkillValue(skill);
}

void Player::ResurectUsingRequestData()
{
    ResurrectPlayer(0.0f,false);

    if(GetMaxHealth() > m_resurrectHealth)
        SetHealth( m_resurrectHealth );
    else
        SetHealth( GetMaxHealth() );

    if(GetMaxPower(POWER_MANA) > m_resurrectMana)
        SetPower(POWER_MANA, m_resurrectMana );
    else
        SetPower(POWER_MANA, GetMaxPower(POWER_MANA) );

    SetPower(POWER_RAGE, 0 );

    SetPower(POWER_ENERGY, GetMaxPower(POWER_ENERGY) );

    SpawnCorpseBones();

    TeleportTo(m_resurrectMap, m_resurrectX, m_resurrectY, m_resurrectZ, GetOrientation());
}

void Player::SetClientControl(Unit* target, uint8 allowMove)
{
    WorldPacket data(SMSG_CLIENT_CONTROL_UPDATE, target->GetPackGUID().size()+1);
    data.append(target->GetPackGUID());
    data << uint8(allowMove);
    GetSession()->SendPacket(&data);
}

void Player::UpdateZoneDependentAuras( uint32 newZone )
{
    // remove new continent flight forms
    if( !isGameMaster() &&
    GetVirtualMapForMapAndZone(GetMapId(),newZone) != 530)
    {
        RemoveSpellsCausingAura(SPELL_AURA_MOD_INCREASE_FLIGHT_SPEED);
        RemoveSpellsCausingAura(SPELL_AURA_FLY);
    }

    // Some spells applied at enter into zone (with subzones)
    // Human Illusion
    // NOTE: these are removed by RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_CHANGE_MAP);
    if ( newZone == 2367 )                                  // Old Hillsbrad Foothills
    {
        uint32 spellid = 0;
        // all horde races
        if( GetTeam() == HORDE )
            spellid = getGender() == GENDER_FEMALE ? 35481 : 35480;
        // and some alliance races
        else if( getRace() == RACE_NIGHTELF || getRace() == RACE_DRAENEI )
            spellid = getGender() == GENDER_FEMALE ? 35483 : 35482;

        if(spellid && !HasAura(spellid,0) )
            CastSpell(this,spellid,true);
    }
}

void Player::UpdateAreaDependentAuras( uint32 newArea )
{
    // remove auras from spells with area limitations
    if (isAlive()) {
        for(AuraMap::iterator iter = m_Auras.begin(); iter != m_Auras.end();)
        {
            // use m_zoneUpdateId for speed: UpdateArea called from UpdateZone or instead UpdateZone in both cases m_zoneUpdateId up-to-date
            if(!IsSpellAllowedInLocation(iter->second->GetSpellProto(),GetMapId(),m_zoneUpdateId,newArea))
                RemoveAura(iter);
            else
                ++iter;
        }
    }

    // unmount if enter in this subzone
    if( newArea == 35)
        RemoveSpellsCausingAura(SPELL_AURA_MOUNTED);
    // Dragonmaw Illusion
    else if( newArea == 3759 || newArea == 3966 || newArea == 3939 || newArea == 3965 )
    {
        //if( GetDummyAura(40214) )
        if (GetReputationRank(1015) >= REP_NEUTRAL)
        {
            if( !HasAura(42016,0) ) {
                CastSpell(this,42016,true);
                CastSpell(this,40216,true);
            }
        }
    }
}

uint32 Player::GetCorpseReclaimDelay(bool pvp) const
{
    if(pvp)
    {
        if(!sWorld.getConfig(CONFIG_DEATH_CORPSE_RECLAIM_DELAY_PVP))
            return copseReclaimDelay[0];
    }
    else if(!sWorld.getConfig(CONFIG_DEATH_CORPSE_RECLAIM_DELAY_PVE) )
        return 0;

    time_t now = time(NULL);
    // 0..2 full period
    // should be ceil(x)-1 but not floor(x)
    uint32 count = (now < m_deathExpireTime - 1) ? (m_deathExpireTime - 1 - now)/DEATH_EXPIRE_STEP : 0;
    return copseReclaimDelay[count];
}

void Player::UpdateCorpseReclaimDelay()
{
    bool pvp = m_ExtraFlags & PLAYER_EXTRA_PVP_DEATH;

    if( pvp && !sWorld.getConfig(CONFIG_DEATH_CORPSE_RECLAIM_DELAY_PVP) ||
        !pvp && !sWorld.getConfig(CONFIG_DEATH_CORPSE_RECLAIM_DELAY_PVE) )
        return;

    time_t now = time(NULL);
    if(now < m_deathExpireTime)
    {
        // full and partly periods 1..3
        uint32 count = (m_deathExpireTime - now)/DEATH_EXPIRE_STEP +1;
        if(count < MAX_DEATH_COUNT)
            m_deathExpireTime = now+(count+1)*DEATH_EXPIRE_STEP;
        else
            m_deathExpireTime = now+MAX_DEATH_COUNT*DEATH_EXPIRE_STEP;
    }
    else
        m_deathExpireTime = now+DEATH_EXPIRE_STEP;
}

void Player::SendCorpseReclaimDelay(bool load)
{
    Corpse* corpse = GetCorpse();
    if(load && !corpse)
        return;

    bool pvp;
    if(corpse)
        pvp = (corpse->GetType() == CORPSE_RESURRECTABLE_PVP);
    else
        pvp = (m_ExtraFlags & PLAYER_EXTRA_PVP_DEATH);

    uint32 delay;
    if(load)
    {
        if(corpse->GetGhostTime() > m_deathExpireTime)
            return;

        uint32 count;
        if( pvp && sWorld.getConfig(CONFIG_DEATH_CORPSE_RECLAIM_DELAY_PVP) ||
           !pvp && sWorld.getConfig(CONFIG_DEATH_CORPSE_RECLAIM_DELAY_PVE) )
        {
            count = (m_deathExpireTime-corpse->GetGhostTime())/DEATH_EXPIRE_STEP;
            if(count>=MAX_DEATH_COUNT)
                count = MAX_DEATH_COUNT-1;
        }
        else
            count=0;

        //time_t expected_time = corpse->GetGhostTime()+copseReclaimDelay[count];
		time_t expected_time = m_deathTime+copseReclaimDelay[count];

        time_t now = time(NULL);
        if(now >= expected_time)
            return;

        delay = expected_time-now;
    }
    else
        delay = GetCorpseReclaimDelay(pvp);

    if(!delay) return;

    //! corpse reclaim delay 30 * 1000ms or longer at often deaths
    WorldPacket data(SMSG_CORPSE_RECLAIM_DELAY, 4);
    data << uint32(delay*1000);
    GetSession()->SendPacket( &data );
}

Player* Player::GetNextRandomRaidMember(float radius)
{
    Group *pGroup = GetGroup();
    if(!pGroup)
        return NULL;

    std::vector<Player*> nearMembers;
    nearMembers.reserve(pGroup->GetMembersCount());

    for(GroupReference *itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
    {
        Player* Target = itr->getSource();

        // IsHostileTo check duel and controlled by enemy
        if( Target && Target != this && IsWithinDistInMap(Target, radius) &&
            !Target->HasInvisibilityAura() && !IsHostileTo(Target) )
            nearMembers.push_back(Target);
    }

    if (nearMembers.empty())
        return NULL;

    uint32 randTarget = GetMap()->urand(0,nearMembers.size()-1);
    return nearMembers[randTarget];
}

PartyResult Player::CanUninviteFromGroup() const
{
    const Group* grp = GetGroup();
    if(!grp)
        return PARTY_RESULT_YOU_NOT_IN_GROUP;

    if(!grp->IsLeader(GetGUID()) && !grp->IsAssistant(GetGUID()))
        return PARTY_RESULT_YOU_NOT_LEADER;

    if(InBattleGround())
        return PARTY_RESULT_INVITE_RESTRICTED;

    return PARTY_RESULT_OK;
}

void Player::SetBattleGroundRaid(Group* group, int8 subgroup)
{
    // we must move references from m_group to m_originalGroup
    SetOriginalGroup(GetGroup(), GetSubGroup());

    m_group.unlink();
    m_group.link(group, this);
    m_group.setSubGroup((uint8)subgroup);
}

void Player::RemoveFromBattleGroundRaid()
{
    // remove existing reference
    m_group.unlink();
    if (Group* group = GetOriginalGroup())
    {
        m_group.link(group, this);
        m_group.setSubGroup(GetOriginalSubGroup());
    }
    SetOriginalGroup(NULL);
}

void Player::SetOriginalGroup(Group *group, int8 subgroup)
{
    if (group == NULL)
        m_originalGroup.unlink();
    else
    {
        // never use SetOriginalGroup without a subgroup unless you specify NULL for group
        assert(subgroup >= 0);
        m_originalGroup.link(group, this);
        m_originalGroup.setSubGroup((uint8)subgroup);
    }
}

void Player::UpdateUnderwaterState(Map* m, float x, float y, float z)
{
    LiquidData liquid_status;
    ZLiquidStatus res = m->getLiquidStatus(x, y, z, MAP_ALL_LIQUIDS, &liquid_status);
    if (!res)
    {
        m_MirrorTimerFlags &= ~(UNDERWATER_INWATER|UNDERWATER_INLAVA|UNDERWATER_INSLIME|UNDERWATER_INDARKWATER);
        // Small hack for enable breath in WMO + enable lava in Molten Core, Blackrock Mountain, BlackRock Dephts and Ironforge
        if (IsInWater())
        {
            m_MirrorTimerFlags|=UNDERWATER_INWATER;
            if(GetMapId() == 409 || GetZoneId() == 25 || GetMapId() == 230 || GetZoneId() == 1537)
                m_MirrorTimerFlags |= UNDERWATER_INLAVA;
        }
        return;
    }

    // All liquids type - check under water position
    if (liquid_status.type&(MAP_LIQUID_TYPE_WATER|MAP_LIQUID_TYPE_OCEAN|MAP_LIQUID_TYPE_MAGMA|MAP_LIQUID_TYPE_SLIME))
    {
        if (res & LIQUID_MAP_UNDER_WATER)
            m_MirrorTimerFlags |= UNDERWATER_INWATER;
        else
            m_MirrorTimerFlags &= ~UNDERWATER_INWATER;
    }

    // Allow travel in dark water on taxi or transport
    if ((liquid_status.type & MAP_LIQUID_TYPE_DARK_WATER) && !isInFlight() && !GetTransport())
        m_MirrorTimerFlags |= UNDERWATER_INDARKWATER;
    else
        m_MirrorTimerFlags &= ~UNDERWATER_INDARKWATER;

    // in lava check, anywhere in lava level
    if (liquid_status.type&MAP_LIQUID_TYPE_MAGMA)
    {
        if (res & (LIQUID_MAP_UNDER_WATER|LIQUID_MAP_IN_WATER|LIQUID_MAP_WATER_WALK))
            m_MirrorTimerFlags |= UNDERWATER_INLAVA;
        else
            m_MirrorTimerFlags &= ~UNDERWATER_INLAVA;
    }
    // in slime check, anywhere in slime level
    if (liquid_status.type&MAP_LIQUID_TYPE_SLIME)
    {
        if (res & (LIQUID_MAP_UNDER_WATER|LIQUID_MAP_IN_WATER|LIQUID_MAP_WATER_WALK))
            m_MirrorTimerFlags |= UNDERWATER_INSLIME;
        else
            m_MirrorTimerFlags &= ~UNDERWATER_INSLIME;
    }
}

void Player::SetCanParry( bool value )
{
    if(m_canParry==value)
        return;

    m_canParry = value;
    UpdateParryPercentage();
}

void Player::SetCanBlock( bool value )
{
    if(m_canBlock==value)
        return;

    m_canBlock = value;
    UpdateBlockPercentage();
}

bool ItemPosCount::isContainedIn(ItemPosCountVec const& vec) const
{
    for(ItemPosCountVec::const_iterator itr = vec.begin(); itr != vec.end();++itr)
        if(itr->pos == pos)
            return true;
    return false;
}

//***********************************
//-------------TRINITY---------------
//***********************************

void Player::HandleFallDamage(MovementInfo& movementInfo)
{
    PROFILE;
    
    if(movementInfo.fallTime < 1500)
        return;

    // calculate total z distance of the fall
    float z_diff = m_lastFallZ - movementInfo.z;

    //Players with low fall distance, Feather Fall or physical immunity (charges used) are ignored
    // 14.57 can be calculated by resolving damageperc formular below to 0
    if (z_diff >= 14.57f && !isDead() && !isGameMaster() && !isSpectator() &&
        !HasAuraType(SPELL_AURA_HOVER) && !HasAuraType(SPELL_AURA_FEATHER_FALL) &&
        !HasAuraType(SPELL_AURA_FLY) && !IsImmunedToDamage(SPELL_SCHOOL_MASK_NORMAL,true) &&
        !m_session->GetPlayer()->GetKnockedBack())
    {
        //Safe fall, fall height reduction
        int32 safe_fall = GetTotalAuraModifier(SPELL_AURA_SAFE_FALL);

        float damageperc = 0.018f*(z_diff-safe_fall)-0.2426f;

        if(damageperc >0 )
        {
            uint32 damage = (uint32)(damageperc * GetMaxHealth()*sWorld.getRate(RATE_DAMAGE_FALL));

            float height = movementInfo.z;
            UpdateGroundPositionZ(movementInfo.x,movementInfo.y,height);

            if (damage > 0)
            {
                //Prevent fall damage from being more than the player maximum health
                if (damage > GetMaxHealth())
                    damage = GetMaxHealth();

                // Gust of Wind
                if (GetDummyAura(43621))
                    damage = GetMaxHealth()/2;

                EnvironmentalDamage(DAMAGE_FALL, damage);
            }

            //Z given by moveinfo, LastZ, FallTime, WaterZ, MapZ, Damage, Safefall reduction
            DEBUG_LOG("FALLDAMAGE z=%f sz=%f pZ=%f FallTime=%d mZ=%f damage=%d SF=%d" , movementInfo.z, height, GetPositionZ(), movementInfo.fallTime, height, damage, safe_fall);
        }
    }
    m_session->GetPlayer()->SetKnockedBack(false);
}

void Player::HandleFallUnderMap()
{
    if(InBattleGround()
        && GetBattleGround()
        && GetBattleGround()->HandlePlayerUnderMap(this))
    {
        // do nothing, the handle already did if returned true
    }
    else
    {
        // NOTE: this is actually called many times while falling
        // even after the player has been teleported away
        // TODO: discard movement packets after the player is rooted
        if(isAlive())
        {
            EnvironmentalDamage(DAMAGE_FALL_TO_VOID, GetMaxHealth());
            // change the death state to CORPSE to prevent the death timer from
            // starting in the next player update
            KillPlayer();
            BuildPlayerRepop();
        }

        // cancel the death timer here if started
        RepopAtGraveyard();
    }
}

void Player::SetViewport(uint64 guid, bool moveable)
{
    WorldPacket data(SMSG_CLIENT_CONTROL_UPDATE, 8+1);
    data.appendPackGUID(guid); // Packed guid of object to set client's view to
    data << (moveable ? uint8(0x01) : uint8(0x00)); // 0 - can't move; 1 - can move
    m_session->SendPacket(&data);
    sLog.outDetail("Viewport for "I64FMT" (%s) changed to "I64FMT, GetGUID(), GetName(), guid);
}

WorldObject* Player::GetFarsightTarget() const
{
    // Players can have in farsight field another player's guid, a creature's guid, or a dynamic object's guid
    if (uint64 guid = GetUInt64Value(PLAYER_FARSIGHT))
        return (WorldObject*)ObjectAccessor::GetObjectByTypeMask(*this, guid, TYPEMASK_PLAYER | TYPEMASK_UNIT | TYPEMASK_DYNAMICOBJECT);
    return NULL;
}

void Player::StopCastingBindSight()
{
    if(WorldObject* target = GetFarsightTarget())
    {
        if(target->isType(TYPEMASK_UNIT))
        {
            ((Unit*)target)->RemoveAuraTypeByCaster(SPELL_AURA_BIND_SIGHT, GetGUID());
            ((Unit*)target)->RemoveAuraTypeByCaster(SPELL_AURA_MOD_POSSESS, GetGUID());
            ((Unit*)target)->RemoveAuraTypeByCaster(SPELL_AURA_MOD_POSSESS_PET, GetGUID());
        }
    }
}

void Player::ClearFarsight()
{
	if (isSpectator() && !spectateFrom)
	    return;

	if (isSpectator())
	    spectateFrom = NULL;

    if (GetUInt64Value(PLAYER_FARSIGHT))
    {
        SetUInt64Value(PLAYER_FARSIGHT, 0);

        WorldPacket data(SMSG_CLEAR_FAR_SIGHT_IMMEDIATE, 0);
        GetSession()->SendPacket(&data);
    }
}

void Player::SetFarsightTarget(WorldObject* obj)
{
    if (!obj || !obj->isType(TYPEMASK_PLAYER | TYPEMASK_UNIT | TYPEMASK_DYNAMICOBJECT))
        return;

    if (obj->ToPlayer() == this)
        return;

    if (isSpectator() && spectateFrom)
    	RemovePlayerFromVision(spectateFrom);

    // Remove the current target if there is one
    StopCastingBindSight();

    SetUInt64Value(PLAYER_FARSIGHT, obj->GetGUID());

    if (isSpectator())
        spectateFrom = (Player*)obj;
}

bool Player::isAllowUseBattleGroundObject()
{
    return ( //InBattleGround() &&                            // in battleground - not need, check in other cases
             !IsMounted() &&                                  // not mounted
             !isTotalImmunity() &&                              // not totally immuned
             !HasStealthAura() &&                             // not stealthed
             !HasInvisibilityAura() &&                        // not invisible
             !HasAura(SPELL_RECENTLY_DROPPED_FLAG, 0) &&      // can't pickup
             isAlive()                                        // live player
           );
}

bool Player::HasTitle(uint32 bitIndex)
{
    if (bitIndex > 128)
        return false;

    uint32 fieldIndexOffset = bitIndex/32;
    uint32 flag = 1 << (bitIndex%32);
    return HasFlag(PLAYER_FIELD_KNOWN_TITLES+fieldIndexOffset, flag);
}

void Player::SetTitle(CharTitlesEntry const* title)
{
    uint32 fieldIndexOffset = title->bit_index/32;
    uint32 flag = 1 << (title->bit_index%32);
    SetFlag(PLAYER_FIELD_KNOWN_TITLES+fieldIndexOffset, flag);
    
    /*WorldPacket data(SMSG_TITLE_EARNED, 4+4);
    data << uint32(title->bit_index);
    data << uint32(1);      // 1 - earned
    GetSession()->SendPacket(&data);*/
}

void Player::RemoveTitle(CharTitlesEntry const* title)
{
    uint32 fieldIndexOffset = title->bit_index/32;
    uint32 flag = 1 << (title->bit_index%32);
    RemoveFlag(PLAYER_FIELD_KNOWN_TITLES+fieldIndexOffset, flag);
    
    /*WorldPacket data(SMSG_TITLE_EARNED, 4+4);
    data << uint32(title->bit_index);
    data << uint32(0);      // 0 - lost
    GetSession()->SendPacket(&data);*/
}

bool Player::HasLevelInRangeForTeleport()
{
    uint8 minLevel = sConfig.GetIntDefault("Teleporter.MinLevel", 1);
    uint8 maxLevel = sConfig.GetIntDefault("Teleporter.MaxLevel", 255);
    
    return (getLevel() >= minLevel && getLevel() <= maxLevel);
}

/*-----------------------TRINITY--------------------------*/
bool Player::isTotalImmunity()
{
    AuraList const& immune = GetAurasByType(SPELL_AURA_SCHOOL_IMMUNITY);

    for(AuraList::const_iterator itr = immune.begin(); itr != immune.end(); ++itr)
    {
        if (((*itr)->GetModifier()->m_miscvalue & SPELL_SCHOOL_MASK_ALL) !=0)   // total immunity
        {
            return true;
        }
        if (((*itr)->GetModifier()->m_miscvalue & SPELL_SCHOOL_MASK_NORMAL) !=0)   // physical damage immunity
        {
            for(AuraList::const_iterator i = immune.begin(); i != immune.end(); ++i)
            {
                if (((*i)->GetModifier()->m_miscvalue & SPELL_SCHOOL_MASK_MAGIC) !=0)   // magic immunity
                {
                    return true;
                }
            }
        }
    }
    return false;
}

void Player::UpdateCharmedAI()
{
    //This should only called in Player::Update
    Creature *charmer = GetCharmer()->ToCreature();

    //kill self if charm aura has infinite duration
    if(charmer->IsInEvadeMode())
    {
        AuraList const& auras = GetAurasByType(SPELL_AURA_MOD_CHARM);
        for(AuraList::const_iterator iter = auras.begin(); iter != auras.end(); ++iter)
            if((*iter)->GetCasterGUID() == charmer->GetGUID() && (*iter)->IsPermanent())
            {
                charmer->DealDamage(this, GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
                return;
            }
    }

    if(!charmer->isInCombat())
        GetMotionMaster()->MoveFollow(charmer, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);

    Unit *target = getVictim();
    if(!target || !charmer->canAttack(target))
    {
        target = charmer->SelectNearestTarget();
        if(!target)
            return;

        GetMotionMaster()->MoveChase(target);
        Attack(target, true);
    }
}

void Player::AddGlobalCooldown(SpellEntry const *spellInfo, Spell const *spell, bool allowTinyCd)
{
    if(!spellInfo || !spellInfo->StartRecoveryTime)
        return;

    uint32 cdTime = spellInfo->StartRecoveryTime;

    if( !(spellInfo->Attributes & (SPELL_ATTR_ABILITY|SPELL_ATTR_TRADESPELL )) )
        cdTime *= GetFloatValue(UNIT_MOD_CAST_SPEED);
    else if (spell->IsRangedSpell() && !spell->IsAutoRepeat())
        cdTime *= m_modAttackSpeedPct[RANGED_ATTACK];

    m_globalCooldowns[spellInfo->StartRecoveryCategory] = (((cdTime<1000 || cdTime>2000) && !allowTinyCd) ? 1000 : cdTime);
}

bool Player::HasGlobalCooldown(SpellEntry const *spellInfo) const
{
    if(!spellInfo)
        return false;

    std::map<uint32, uint32>::const_iterator itr = m_globalCooldowns.find(spellInfo->StartRecoveryCategory);
    return itr != m_globalCooldowns.end() && (itr->second > sWorld.GetUpdateTime());
}

void Player::RemoveGlobalCooldown(SpellEntry const *spellInfo)
{
    if(!spellInfo)
        return;

    m_globalCooldowns[spellInfo->StartRecoveryCategory] = 0;
}

void Player::SetHomebindToLocation(WorldLocation const& loc, uint32 area_id)
{
    m_homebindMapId = loc.m_mapId;
    m_homebindAreaId = area_id;
    m_homebindX = loc.m_positionX;
    m_homebindY = loc.m_positionY;
    m_homebindZ = loc.m_positionZ;
    
    // update sql homebind
    CharacterDatabase.PExecute("UPDATE character_homebind SET map = '%u', zone = '%u', position_x = '%f', position_y = '%f', position_z = '%f' WHERE guid = '%u'",
        m_homebindMapId, m_homebindAreaId, m_homebindX, m_homebindY, m_homebindZ, GetGUIDLow());
}

void Player::_LoadSkills(QueryResult *result)
{
    //                                                           0      1      2
    // SetPQuery(PLAYER_LOGIN_QUERY_LOADSKILLS,          "SELECT skill, value, max FROM character_skills WHERE guid = '%u'", GUID_LOPART(m_guid));

    uint32 count = 0;
    if (result)
    {
        do
        {
            Field *fields = result->Fetch();

            uint16 skill    = fields[0].GetUInt16();
            uint16 value    = fields[1].GetUInt16();
            uint16 max      = fields[2].GetUInt16();

            SkillLineEntry const *pSkill = sSkillLineStore.LookupEntry(skill);
            if(!pSkill)
            {
                sLog.outError("Character %u has skill %u that does not exist.", GetGUIDLow(), skill);
                continue;
            }

            // set fixed skill ranges
            switch(GetSkillRangeType(pSkill,false))
            {
                case SKILL_RANGE_LANGUAGE:                      // 300..300
                    value = max = 300;
                    break;
                case SKILL_RANGE_MONO:                          // 1..1, grey monolite bar
                    value = max = 1;
                    break;
                default:
                    break;
            }
            if(value == 0)
            {
                sLog.outError("Character %u has skill %u with value 0. Will be deleted.", GetGUIDLow(), skill);
                CharacterDatabase.PExecute("DELETE FROM character_skills WHERE guid = '%u' AND skill = '%u' ", GetGUIDLow(), skill );
                continue;
            }

            // enable unlearn button for primary professions only
            if (pSkill->categoryId == SKILL_CATEGORY_PROFESSION)
                SetUInt32Value(PLAYER_SKILL_INDEX(count), MAKE_PAIR32(skill,1));
            else
                SetUInt32Value(PLAYER_SKILL_INDEX(count), MAKE_PAIR32(skill,0));

            SetUInt32Value(PLAYER_SKILL_VALUE_INDEX(count),MAKE_SKILL_VALUE(value, max));
            SetUInt32Value(PLAYER_SKILL_BONUS_INDEX(count),0);

            mSkillStatus.insert(SkillStatusMap::value_type(skill, SkillStatusData(count, SKILL_UNCHANGED)));

            learnSkillRewardedSpells(skill);

            ++count;

            if(count >= PLAYER_MAX_SKILLS)                      // client limit
            {
                sLog.outError("Character %u has more than %u skills.", PLAYER_MAX_SKILLS);
                break;
            }
        } while (result->NextRow());
        delete result;
    }

    for (; count < PLAYER_MAX_SKILLS; ++count)
    {
        SetUInt32Value(PLAYER_SKILL_INDEX(count), 0);
        SetUInt32Value(PLAYER_SKILL_VALUE_INDEX(count),0);
        SetUInt32Value(PLAYER_SKILL_BONUS_INDEX(count),0);
    }
}

uint32 Player::GetTotalAccountPlayedTime()
{
    uint32 accountId = m_session->GetAccountId();
    
    QueryResult* result = CharacterDatabase.PQuery("SELECT SUM(totaltime) FROM characters WHERE account = %u", accountId);
    
    if (!result)
        return 0;
        
    Field* fields = result->Fetch();
    
    return fields[0].GetUInt32();
}

bool Player::IsMainFactionForRace(uint32 race, uint32 factionId)
{
    switch (race) {
    case RACE_HUMAN:
        return factionId ==  72;
    case RACE_ORC:
        return factionId == 76;
    case RACE_DWARF:
        return factionId == 47;
    case RACE_NIGHTELF:
        return factionId == 69;
    case RACE_UNDEAD_PLAYER:
        return factionId == 68;
    case RACE_TAUREN:
        return factionId == 81;
    case RACE_GNOME:
        return factionId == 54;
    case RACE_TROLL:
        return factionId == 530;
    case RACE_BLOODELF:
        return factionId == 911;
    case RACE_DRAENEI:
        return factionId == 930;
    default:
        break;
    }
    
    return false;
}

uint32 Player::GetMainFactionForRace(uint32 race)
{
    switch (race) {
    case RACE_HUMAN:
        return 72;
    case RACE_ORC:
        return 76;
    case RACE_DWARF:
        return 47;
    case RACE_NIGHTELF:
        return 69;
    case RACE_UNDEAD_PLAYER:
        return 68;
    case RACE_TAUREN:
        return 81;
    case RACE_GNOME:
        return 54;
    case RACE_TROLL:
        return 530;
    case RACE_BLOODELF:
        return 911;
    case RACE_DRAENEI:
        return 930;
    default:
        break;
    }
    
    return 0;
}

uint32 Player::GetNewFactionForRaceChange(uint32 oldRace, uint32 newRace, uint32 factionId)
{
    // Main faction case
    if (IsMainFactionForRace(oldRace, factionId))
        return GetMainFactionForRace(newRace);

    // Default case
    switch (factionId) {
    case 47:    // Ironforge
        return 530;
    case 54:    // Gnomeregan
    case 68:    // Undercity
    case 69:    // Darnassus
    case 72:    // Stormwind
    case 76:    // Orgrimmar
    case 81:    // Thunder Bluff
    case 530:   // Darkspear Trolls 
    case 911:   // Silvermoon
    case 930:   // Exodar
    default:
        break;
    }
    
    return factionId;
}

void Player::UnsummonPetTemporaryIfAny()
{
    Pet* pet = GetPet();
    if (!pet)
        return;

    if (!m_temporaryUnsummonedPetNumber && pet->isControlled() && !pet->isTemporarySummoned())
    {
        m_temporaryUnsummonedPetNumber = pet->GetCharmInfo()->GetPetNumber();
        m_oldpetspell = pet->GetUInt32Value(UNIT_CREATED_BY_SPELL);
    }

    RemovePet(pet, PET_SAVE_AS_CURRENT);
}

void Player::setCommentator(bool on)
{
	if (on)
	{
		SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_COMMENTATOR | PLAYER_FLAGS_COMMENTATOR_UBER);

		WorldPacket data(SMSG_COMMENTATOR_STATE_CHANGED, 8 + 1);
		data << uint64(GetGUID());
		data << uint8(1);
		SendMessageToSet(&data, true);
	}
	else
	{
		RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_COMMENTATOR | PLAYER_FLAGS_COMMENTATOR_UBER);

		WorldPacket data(SMSG_COMMENTATOR_STATE_CHANGED, 8 + 1);
		data << uint64(GetGUID());
		data << uint8(0);
		SendMessageToSet(&data, true);
	}
}

void Player::SetSpectate(bool on)
{
    if (on)
    {
        SetSpeed(MOVE_RUN, 2.5);
        spectatorFlag = true;

        setFaction(35);

        RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_FFA_PVP);

        CombatStopWithPets();
        if (Pet* pet = GetPet())
        {
            RemovePet(pet, PET_SAVE_AS_CURRENT);
        }
        SetTemporaryUnsummonedPetNumber(0);
        UnsummonPetTemporaryIfAny();
        RemoveMiniPet();

        ResetContestedPvP();

        getHostilRefManager().setOnlineOfflineState(false);

        SetDisplayId(10045);

        SetVisibility(VISIBILITY_OFF);
    }
    else
    {
        setFactionForRace(getRace());

        if (spectateFrom)
        	spectateFrom->RemovePlayerFromVision(this);

        // restore FFA PvP Server state
        if(sWorld.IsFFAPvPRealm())
            SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_FFA_PVP);

        // restore FFA PvP area state, remove not allowed for GM mounts
        UpdateArea(m_areaUpdateId);

        getHostilRefManager().setOnlineOfflineState(true);
        spectateCanceled = false;
        spectatorFlag = false;
        SetDisplayId(GetNativeDisplayId());
        UpdateSpeed(MOVE_RUN, true);

        SetVisibility(VISIBILITY_ON);
    }

    //ObjectAccessor::UpdateVisibilityForPlayer(this);
    SetToNotify();
}

bool Player::HaveSpectators()
{
    if (BattleGround *bg = GetBattleGround())
    {
        if (bg->isSpectator(GetGUID()))
        	return false;

    	if (bg->isArena())
        {
            if (bg->GetStatus() != STATUS_IN_PROGRESS)
                return false;

            return bg->HaveSpectators();
        }
    }

    return false;
}

void Player::SendDataForSpectator()
{
	BattleGround *bGround = GetBattleGround();
	if (!bGround)
		return;

	if (!bGround->isSpectator(GetGUID()))
	    return;

	if (bGround->GetStatus() != STATUS_IN_PROGRESS)
	    return;

	for (BattleGround::BattleGroundPlayerMap::const_iterator itr = bGround->GetPlayers().begin(); itr != bGround->GetPlayers().end(); ++itr)
	    if (Player* tmpPlayer = ObjectAccessor::FindPlayer(itr->first))
	    {
	        if (bGround->isSpectator(tmpPlayer->GetGUID()))
	            continue;

	        uint32 tmpID = bGround->GetPlayerTeam(tmpPlayer->GetGUID());

	        // generate addon massage
	        std::string pName = tmpPlayer->GetName();
	        std::string tName = "";

	        if (Player *target = tmpPlayer->GetSelectedPlayer())
	        {
	        	if (!bGround->isSpectator(target->GetGUID()))
	                tName = target->GetName();
	        }

	        SpectatorAddonMsg msg;
	        msg.SetPlayer(pName);
	        if (tName != "")
	            msg.SetTarget(tName);
	        msg.SetStatus(tmpPlayer->isAlive());
	        msg.SetClass(tmpPlayer->getClass());
	        msg.SetCurrentHP(tmpPlayer->GetHealth());
	        msg.SetMaxHP(tmpPlayer->GetMaxHealth());
	        Powers powerType = tmpPlayer->getPowerType();
	        msg.SetMaxPower(tmpPlayer->GetMaxPower(powerType));
	        msg.SetCurrentPower(tmpPlayer->GetPower(powerType));
	        msg.SetPowerType(powerType);
	        msg.SetTeam(tmpID);
	        msg.SendPacket(GetGUID());
	    }
}

void Player::SendSpectatorAddonMsgToBG(SpectatorAddonMsg msg)
{
    if (!HaveSpectators())
        return;

    if (BattleGround *bg = GetBattleGround())
    	bg->SendSpectateAddonsMsg(msg);
}

void Player::UpdateKnownTitles()
{
    uint32 bit_index;
    uint32 honor_kills = GetUInt32Value(PLAYER_FIELD_LIFETIME_HONORABLE_KILLS);
    uint32 old_title = GetUInt32Value(PLAYER_CHOSEN_TITLE);

    if (honor_kills < 0)
        return;

    for (int i = HKRANK01; i != HKRANKMAX; ++i)
    {
    	uint32 checkTitle = i;
        checkTitle += ((GetTeam() == ALLIANCE) ? (HKRANKMAX-1) : 0);
    	if(CharTitlesEntry const* tEntry = sCharTitlesStore.LookupEntry(checkTitle))
    	{
    		bit_index = tEntry->bit_index;
    		if (HasTitle(bit_index))
    			RemoveTitle(tEntry);
    	}

        if (honor_kills >= sWorld.pvp_ranks[i])
        {
        	uint32 new_title = i;
            new_title += ((GetTeam() == ALLIANCE) ? 0 : (HKRANKMAX-1));

            if(CharTitlesEntry const* tEntry = sCharTitlesStore.LookupEntry(new_title))
            {
            	bit_index = tEntry->bit_index;
            	if (!HasTitle(bit_index))
            	{
            		SetFlag64(PLAYER_FIELD_KNOWN_TITLES,uint64(1) << bit_index);

            		WorldPacket data(SMSG_TITLE_EARNED, 4 + 4);
            		data << uint32(bit_index);
            		data << uint32(1);
            		GetSession()->SendPacket(&data);
            	}
            }
        }
    }
}

void Player::addSpamReport(uint64 reporterGUID, std::string message)
{
    // Add the new report
    time_t now = time(NULL);
    SpamReport spr;
    spr.time = now;
    spr.reporterGUID = reporterGUID;
    spr.message = message;
    
    _spamReports[GUID_LOPART(reporterGUID)] = spr;
    
    // Trash expired reports according to world config
    uint32 period = sWorld.getConfig(CONFIG_SPAM_REPORT_PERIOD);
    for (SpamReports::iterator itr = _spamReports.begin(); itr != _spamReports.end(); itr++) {
        if (itr->second.time < (now - period)) {
            _spamReports.erase(itr);
            itr = _spamReports.begin();
        }
    }
    
    // If we reported that spammer a little while ago, there's no need to do it again
    if (_lastSpamAlert > (now - sWorld.getConfig(CONFIG_SPAM_REPORT_COOLDOWN)))
        return;
    
    // Oooh, you little spammer!
    if (_spamReports.size() >= sWorld.getConfig(CONFIG_SPAM_REPORT_THRESHOLD)) {
        sIRCMgr.onReportSpam(GetName(), GetGUIDLow());
        _lastSpamAlert = now;
    }
}

void Player::RemoveAllCurrentPetAuras()
{
    if(Pet* pet = GetPet())
        pet->RemoveAllAuras();
     else 
        CharacterDatabase.PQuery("DELETE FROM pet_aura WHERE guid IN ( SELECT id FROM character_pet WHERE owner = %u AND slot = %u )", GetGUIDLow(), PET_SAVE_NOT_IN_SLOT);
}

void Player::GetArenaZoneCoord(bool secondary, uint32& map, float& x, float& y, float& z, float& o)
{
    bool set = false;
    QueryResult* query = WorldDatabase.PQuery("SELECT position_x, position_y, position_z, orientation, map FROM game_tele WHERE name = \"%s\"", secondary ? "pvpzone2" : "pvpzone");
    if (query) {
        Field* fields = query->Fetch();
        if(fields)
        {
            x = fields[0].GetFloat();
            y = fields[1].GetFloat();
            z = fields[2].GetFloat();
            o = fields[3].GetFloat();
            map = fields[4].GetUInt32();
            set = true;
        }
    }

    if(!set) //default values
    {
        sLog.outError("GetArenaZoneCoord(...) : DB values not found, using default values");
        if(!secondary) { //hyjal area
           map = 1;x = 4717.020020;y = -1973.829956;z = 1087.079956;o = 0.068669;
        } else { //ZG Area
           map = 0;x = -12248.573242;y = -1679.274902;z = 130.267273;o = 3.024384;
        }
    }
}

void Player::TeleportToArenaZone(bool secondary)
{    
    float x, y, z, o;
    uint32 map;
    GetArenaZoneCoord(secondary, map, x, y, z, o);
    TeleportTo(map, x, y, z, o, TELE_TO_GM_MODE); 
}

/* true if the player threshold is reached and there is more player in the main zone than the secondary */
bool Player::ShouldGoToSecondaryArenaZone()
{
    bool teleportToSecondaryZone = false;
    uint32 onlinePlayers = sWorld.GetActiveSessionCount();
    uint32 repartitionTheshold = sWorld.getConfig(CONFIG_ARENASERVER_PLAYER_REPARTITION_THRESHOLD);

    if(repartitionTheshold && onlinePlayers > repartitionTheshold)
    {
        float x,y,z,o; 
        uint32 mainMapId, secMapId;
        GetArenaZoneCoord(false, mainMapId, x, y, z, o);
        GetArenaZoneCoord(true, secMapId, x, y, z, o);
        Map* mainMap = MapManager::Instance().FindMap(mainMapId);
        Map* secMap = MapManager::Instance().FindMap(secMapId);
        if(mainMap && secMap && mainMap->GetPlayers().getSize() > secMap->GetPlayers().getSize())
                return true;
    }

    return false;
}
