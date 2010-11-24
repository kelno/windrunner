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

#ifndef TRINITYCORE_CREATURE_H
#define TRINITYCORE_CREATURE_H

#include "Common.h"
#include "Unit.h"
#include "UpdateMask.h"
#include "ItemPrototype.h"
#include "LootMgr.h"
#include "Database/DatabaseEnv.h"
#include "Cell.h"
#include "CreatureGroups.h"

#include <list>

struct SpellEntry;

class CreatureAI;
class Quest;
class Player;
class WorldSession;
class CreatureGroup;

enum Gossip_Option
{
    GOSSIP_OPTION_NONE              = 0,                    //UNIT_NPC_FLAG_NONE              = 0,
    GOSSIP_OPTION_GOSSIP            = 1,                    //UNIT_NPC_FLAG_GOSSIP            = 1,
    GOSSIP_OPTION_QUESTGIVER        = 2,                    //UNIT_NPC_FLAG_QUESTGIVER        = 2,
    GOSSIP_OPTION_VENDOR            = 3,                    //UNIT_NPC_FLAG_VENDOR            = 4,
    GOSSIP_OPTION_TAXIVENDOR        = 4,                    //UNIT_NPC_FLAG_TAXIVENDOR        = 8,
    GOSSIP_OPTION_TRAINER           = 5,                    //UNIT_NPC_FLAG_TRAINER           = 16,
    GOSSIP_OPTION_SPIRITHEALER      = 6,                    //UNIT_NPC_FLAG_SPIRITHEALER      = 32,
    GOSSIP_OPTION_SPIRITGUIDE       = 7,                    //UNIT_NPC_FLAG_SPIRITGUIDE       = 64,
    GOSSIP_OPTION_INNKEEPER         = 8,                    //UNIT_NPC_FLAG_INNKEEPER         = 128,
    GOSSIP_OPTION_BANKER            = 9,                    //UNIT_NPC_FLAG_BANKER            = 256,
    GOSSIP_OPTION_PETITIONER        = 10,                   //UNIT_NPC_FLAG_PETITIONER        = 512,
    GOSSIP_OPTION_TABARDDESIGNER    = 11,                   //UNIT_NPC_FLAG_TABARDDESIGNER    = 1024,
    GOSSIP_OPTION_BATTLEFIELD       = 12,                   //UNIT_NPC_FLAG_BATTLEFIELDPERSON = 2048,
    GOSSIP_OPTION_AUCTIONEER        = 13,                   //UNIT_NPC_FLAG_AUCTIONEER        = 4096,
    GOSSIP_OPTION_STABLEPET         = 14,                   //UNIT_NPC_FLAG_STABLE            = 8192,
    GOSSIP_OPTION_ARMORER           = 15,                   //UNIT_NPC_FLAG_ARMORER           = 16384,
    GOSSIP_OPTION_UNLEARNTALENTS    = 16,                   //UNIT_NPC_FLAG_TRAINER (bonus option for GOSSIP_OPTION_TRAINER)
    GOSSIP_OPTION_UNLEARNPETSKILLS  = 17,                   //UNIT_NPC_FLAG_TRAINER (bonus option for GOSSIP_OPTION_TRAINER)
    GOSSIP_OPTION_OUTDOORPVP        = 18                    //added by code (option for outdoor pvp creatures)
};

enum Gossip_Guard
{
    GOSSIP_GUARD_BANK               = 32,
    GOSSIP_GUARD_RIDE               = 33,
    GOSSIP_GUARD_GUILD              = 34,
    GOSSIP_GUARD_INN                = 35,
    GOSSIP_GUARD_MAIL               = 36,
    GOSSIP_GUARD_AUCTION            = 37,
    GOSSIP_GUARD_WEAPON             = 38,
    GOSSIP_GUARD_STABLE             = 39,
    GOSSIP_GUARD_BATTLE             = 40,
    GOSSIP_GUARD_SPELLTRAINER       = 41,
    GOSSIP_GUARD_SKILLTRAINER       = 42
};

enum Gossip_Guard_Spell
{
    GOSSIP_GUARD_SPELL_WARRIOR      = 64,
    GOSSIP_GUARD_SPELL_PALADIN      = 65,
    GOSSIP_GUARD_SPELL_HUNTER       = 66,
    GOSSIP_GUARD_SPELL_ROGUE        = 67,
    GOSSIP_GUARD_SPELL_PRIEST       = 68,
    GOSSIP_GUARD_SPELL_UNKNOWN1     = 69,
    GOSSIP_GUARD_SPELL_SHAMAN       = 70,
    GOSSIP_GUARD_SPELL_MAGE         = 71,
    GOSSIP_GUARD_SPELL_WARLOCK      = 72,
    GOSSIP_GUARD_SPELL_UNKNOWN2     = 73,
    GOSSIP_GUARD_SPELL_DRUID        = 74
};

enum Gossip_Guard_Skill
{
    GOSSIP_GUARD_SKILL_ALCHEMY      = 80,
    GOSSIP_GUARD_SKILL_BLACKSMITH   = 81,
    GOSSIP_GUARD_SKILL_COOKING      = 82,
    GOSSIP_GUARD_SKILL_ENCHANT      = 83,
    GOSSIP_GUARD_SKILL_FIRSTAID     = 84,
    GOSSIP_GUARD_SKILL_FISHING      = 85,
    GOSSIP_GUARD_SKILL_HERBALISM    = 86,
    GOSSIP_GUARD_SKILL_LEATHER      = 87,
    GOSSIP_GUARD_SKILL_MINING       = 88,
    GOSSIP_GUARD_SKILL_SKINNING     = 89,
    GOSSIP_GUARD_SKILL_TAILORING    = 90,
    GOSSIP_GUARD_SKILL_ENGINERING   = 91
};

enum GossipOptionIcon
{
    GOSSIP_ICON_CHAT                = 0,                    //white chat bubble
    GOSSIP_ICON_VENDOR              = 1,                    //brown bag
    GOSSIP_ICON_TAXI                = 2,                    //flight
    GOSSIP_ICON_TRAINER             = 3,                    //book
    GOSSIP_ICON_INTERACT_1          = 4,                    //interaction wheel
    GOSSIP_ICON_INTERACT_2          = 5,                    //interaction wheel
    GOSSIP_ICON_MONEY_BAG           = 6,                    //brown bag with yellow dot
    GOSSIP_ICON_TALK                = 7,                    //white chat bubble with black dots
    GOSSIP_ICON_TABARD              = 8,                    //tabard
    GOSSIP_ICON_BATTLE              = 9,                    //two swords
    GOSSIP_ICON_DOT                 = 10                    //yellow dot
};

struct GossipOption
{
    uint32 Id;
    uint32 GossipId;
    uint32 NpcFlag;
    uint32 Icon;
    uint32 Action;
    uint32 BoxMoney;
    bool Coded;
    std::string OptionText;
    std::string BoxText;
};

enum CreatureFlagsExtra
{
    CREATURE_FLAG_EXTRA_INSTANCE_BIND   = 0x00000001,       // creature kill bind instance with killer and killer's group
    CREATURE_FLAG_EXTRA_CIVILIAN        = 0x00000002,       // not aggro (ignore faction/reputation hostility)
    CREATURE_FLAG_EXTRA_NO_PARRY        = 0x00000004,       // creature can't parry
    CREATURE_FLAG_EXTRA_NO_PARRY_HASTEN = 0x00000008,       // creature can't counter-attack at parry
    CREATURE_FLAG_EXTRA_NO_BLOCK        = 0x00000010,       // creature can't block
    CREATURE_FLAG_EXTRA_NO_CRUSH        = 0x00000020,       // creature can't do crush attacks
    CREATURE_FLAG_EXTRA_NO_XP_AT_KILL   = 0x00000040,       // creature kill not provide XP
    CREATURE_FLAG_EXTRA_TRIGGER         = 0x00000080,       // trigger creature
    CREATURE_FLAG_EXTRA_WORLDEVENT      = 0x00004000,       // custom flag for world event creatures (left room for merging)
    //CREATURE_FLAG_EXTRA_CHARM_AI        = 0x00008000,       // use ai when charmed
    CREATURE_FLAG_EXTRA_NO_TAUNT        = 0x00010000,       // cannot be taunted
    CREATURE_FLAG_EXTRA_NO_CRIT         = 0x00020000,       // creature can't do critical strikes
};

// GCC have alternative #pragma pack(N) syntax and old gcc version not support pack(push,N), also any gcc version not support it at some platform
#if defined( __GNUC__ )
#pragma pack(1)
#else
#pragma pack(push,1)
#endif

// from `creature_template` table
struct CreatureInfo
{
    uint32  Entry;
    uint32  HeroicEntry;
    uint32  Modelid_A1;
    uint32  Modelid_A2;
    uint32  Modelid_H1;
    uint32  Modelid_H2;
    char*   Name;
    char*   SubName;
    char*   IconName;
    uint32  minlevel;
    uint32  maxlevel;
    uint32  minhealth;
    uint32  maxhealth;
    uint32  minmana;
    uint32  maxmana;
    uint32  armor;
    uint32  faction_A;
    uint32  faction_H;
    uint32  npcflag;
    float   speed;
    float   scale;
    uint32  rank;
    float   mindmg;
    float   maxdmg;
    uint32  dmgschool;
    uint32  attackpower;
    uint32  baseattacktime;
    uint32  rangeattacktime;
    uint32  unit_flags;                                     // enum UnitFlags mask values
    uint32  dynamicflags;
    uint32  family;                                         // enum CreatureFamily values for type==CREATURE_TYPE_BEAST, or 0 in another cases
    uint32  trainer_type;
    uint32  trainer_spell;
    uint32  classNum;
    uint32  race;
    float   minrangedmg;
    float   maxrangedmg;
    uint32  rangedattackpower;
    uint32  type;                                           // enum CreatureType values
    uint32  type_flags;                                     // enum CreatureTypeFlags mask values
    uint32  lootid;
    uint32  pickpocketLootId;
    uint32  SkinLootId;
    int32   resistance1;
    int32   resistance2;
    int32   resistance3;
    int32   resistance4;
    int32   resistance5;
    int32   resistance6;
    uint32  spell1;
    uint32  spell2;
    uint32  spell3;
    uint32  spell4;
    uint32  spell5;
    uint32  spell6;
    uint32  spell7;
    uint32  spell8;
    uint32  PetSpellDataId;
    uint32  mingold;
    uint32  maxgold;
    char const* AIName;
    uint32  MovementType;
    uint32  InhabitType;
    bool    RacialLeader;
    bool    RegenHealth;
    uint32  equipmentId;
    uint32  MechanicImmuneMask;
    uint32  flags_extra;
    uint32  ScriptID;
    uint32  QuestPoolId;
    uint32 GetRandomValidModelId() const;
    uint32 GetFirstValidModelId() const;

    // helpers
    SkillType GetRequiredLootSkill() const
    {
        if(type_flags & CREATURE_TYPEFLAGS_HERBLOOT)
            return SKILL_HERBALISM;
        else if(type_flags & CREATURE_TYPEFLAGS_MININGLOOT)
            return SKILL_MINING;
        else
            return SKILL_SKINNING;                          // normal case
    }

    bool isTameable() const
    {
        return type == CREATURE_TYPE_BEAST && family != 0 && (type_flags & CREATURE_TYPEFLAGS_TAMEABLE);
    }
};

struct CreatureLocale
{
    std::vector<std::string> Name;
    std::vector<std::string> SubName;
};

struct NpcOptionLocale
{
    std::vector<std::string> OptionText;
    std::vector<std::string> BoxText;
};

struct EquipmentInfo
{
    uint32  entry;
    uint32  equipmodel[3];
    uint32  equipinfo[3];
    uint32  equipslot[3];
};

// from `creature` table
struct CreatureData
{
    uint32 id;                                              // entry in creature_template
    uint16 mapid;
    uint32 displayid;
    int32 equipmentId;
    float posX;
    float posY;
    float posZ;
    float orientation;
    uint32 spawntimesecs;
    float spawndist;
    uint32 currentwaypoint;
    uint32 curhealth;
    uint32 curmana;
    bool  is_dead;
    uint8 movementType;
    uint8 spawnMask;
    uint32 poolId;
};

struct CreatureDataAddonAura
{
    uint16 spell_id;
    uint8 effect_idx;
};

// from `creature_addon` table
struct CreatureDataAddon
{
    uint32 guidOrEntry;
    uint32 path_id;
    uint32 mount;
    uint32 bytes0;
    uint32 bytes1;
    uint32 bytes2;
    uint32 emote;
    uint32 move_flags;
    CreatureDataAddonAura const* auras;                     // loaded as char* "spell1 eff1 spell2 eff2 ... "
};

struct CreatureModelInfo
{
    uint32 modelid;
    float bounding_radius;
    float combat_reach;
    uint8 gender;
    uint32 modelid_other_gender;
};

enum InhabitTypeValues
{
    INHABIT_GROUND = 1,
    INHABIT_WATER  = 2,
    INHABIT_AIR    = 4,
    INHABIT_ANYWHERE = INHABIT_GROUND | INHABIT_WATER | INHABIT_AIR
};

// GCC have alternative #pragma pack() syntax and old gcc version not support pack(pop), also any gcc version not support it at some platform
#if defined( __GNUC__ )
#pragma pack()
#else
#pragma pack(pop)
#endif

// Vendors
struct VendorItem
{
    VendorItem(uint32 _item, uint32 _maxcount, uint32 _incrtime, uint32 _ExtendedCost)
        : item(_item), maxcount(_maxcount), incrtime(_incrtime), ExtendedCost(_ExtendedCost) {}

    uint32 item;
    uint32 maxcount;                                        // 0 for infinity item amount
    uint32 incrtime;                                        // time for restore items amount if maxcount != 0
    uint32 ExtendedCost;
};
typedef std::vector<VendorItem*> VendorItemList;

struct VendorItemData
{
    VendorItemList m_items;

    VendorItem* GetItem(uint32 slot) const
    {
        if(slot>=m_items.size()) return NULL;
        return m_items[slot];
    }
    bool Empty() const { return m_items.empty(); }
    uint8 GetItemCount() const { return m_items.size(); }
    void AddItem( uint32 item, uint32 maxcount, uint32 ptime, uint32 ExtendedCost)
    {
        m_items.push_back(new VendorItem(item, maxcount, ptime, ExtendedCost));
    }
    bool RemoveItem( uint32 item_id );
    VendorItem const* FindItem(uint32 item_id) const;
    size_t FindItemSlot(uint32 item_id) const;

    void Clear()
    {
        for (VendorItemList::iterator itr = m_items.begin(); itr != m_items.end(); ++itr)
            delete (*itr);
    }
};

struct VendorItemCount
{
    explicit VendorItemCount(uint32 _item, uint32 _count)
        : itemId(_item), count(_count), lastIncrementTime(time(NULL)) {}

    uint32 itemId;
    uint32 count;
    time_t lastIncrementTime;
};

typedef std::list<VendorItemCount> VendorItemCounts;

struct TrainerSpell
{
    uint32 spell;
    uint32 spellcost;
    uint32 reqskill;
    uint32 reqskillvalue;
    uint32 reqlevel;
};

typedef std::vector<TrainerSpell*> TrainerSpellList;

struct TrainerSpellData
{
    TrainerSpellData() : trainerType(0) {}

    TrainerSpellList spellList;
    uint32 trainerType;                                     // trainer type based at trainer spells, can be different from creature_template value.
                                                            // req. for correct show non-prof. trainers like weaponmaster, allowed values 0 and 2.

    void Clear();
    TrainerSpell const* Find(uint32 spell_id) const;
};

typedef std::list<GossipOption> GossipOptionList;

typedef std::map<uint32,time_t> CreatureSpellCooldowns;

// max different by z coordinate for creature aggro reaction
#define CREATURE_Z_ATTACK_RANGE 3

#define MAX_VENDOR_ITEMS 255                                // Limitation in item count field size in SMSG_LIST_INVENTORY

class Creature : public Unit
{
    public:

        explicit Creature();
        virtual ~Creature();

        void AddToWorld();
        void RemoveFromWorld();
        
        void DisappearAndDie();

        bool Create (uint32 guidlow, Map *map, uint32 Entry, uint32 team, const CreatureData *data = NULL);
        bool LoadCreaturesAddon(bool reload = false);
        void SelectLevel(const CreatureInfo *cinfo);
        void LoadEquipment(uint32 equip_entry, bool force=false);

        uint32 GetDBTableGUIDLow() const { return m_DBTableGuid; }
        char const* GetSubName() const { return GetCreatureInfo()->SubName; }

        void Update( uint32 time );                         // overwrited Unit::Update
        void GetRespawnCoord(float &x, float &y, float &z, float* ori = NULL, float* dist =NULL) const;
        uint32 GetEquipmentId() const { return m_equipmentId; }

        bool isPet() const { return m_isPet; }
        void SetCorpseDelay(uint32 delay) { m_corpseDelay = delay; }
        bool isTotem() const { return m_isTotem; }
        bool isRacialLeader() const { return GetCreatureInfo()->RacialLeader; }
        bool isCivilian() const { return GetCreatureInfo()->flags_extra & CREATURE_FLAG_EXTRA_CIVILIAN; }
        bool isTrigger() const { return GetCreatureInfo()->flags_extra & CREATURE_FLAG_EXTRA_TRIGGER; }
        bool canWalk() const { return GetCreatureInfo()->InhabitType & INHABIT_GROUND; }
        bool canSwim() const { return GetCreatureInfo()->InhabitType & INHABIT_WATER; }
        bool canFly()  const { return GetCreatureInfo()->InhabitType & INHABIT_AIR; }
        void SetReactState(ReactStates st) { m_reactState = st; }
        ReactStates GetReactState() { return m_reactState; }
        bool HasReactState(ReactStates state) const { return (m_reactState == state); }
        ///// TODO RENAME THIS!!!!!
        bool isCanTrainingOf(Player* player, bool msg) const;
        bool isCanInteractWithBattleMaster(Player* player, bool msg) const;
        bool isCanTrainingAndResetTalentsOf(Player* pPlayer) const;
        bool IsOutOfThreatArea(Unit* pVictim) const;
        bool IsImmunedToSpell(SpellEntry const* spellInfo, bool useCharges = false);
                                                            // redefine Unit::IsImmunedToSpell
        bool IsImmunedToSpellEffect(uint32 effect, uint32 mechanic) const;
                                                            // redefine Unit::IsImmunedToSpellEffect
        bool isElite() const
        {
            if(isPet())
                return false;

            uint32 rank = GetCreatureInfo()->rank;
            return rank != CREATURE_ELITE_NORMAL && rank != CREATURE_ELITE_RARE;
        }

        bool isWorldBoss() const
        {
            if(isPet())
                return false;

            return GetCreatureInfo()->rank == CREATURE_ELITE_WORLDBOSS;
        }

        uint32 getLevelForTarget(Unit const* target) const; // overwrite Unit::getLevelForTarget for boss level support

        bool IsInEvadeMode() const;

        bool AIM_Initialize(CreatureAI* ai = NULL);

        void AI_SendMoveToPacket(float x, float y, float z, uint32 time, uint32 MovementFlags, uint8 type);
        CreatureAI* AI() { return (CreatureAI*)i_AI; }

        uint32 GetShieldBlockValue() const                  //dunno mob block value
        {
            return (getLevel()/2 + uint32(GetStat(STAT_STRENGTH)/20));
        }

        SpellSchoolMask GetMeleeDamageSchoolMask() const { return m_meleeDamageSchoolMask; }
        void SetMeleeDamageSchool(SpellSchools school) { m_meleeDamageSchoolMask = SpellSchoolMask(1 << school); }

        void _AddCreatureSpellCooldown(uint32 spell_id, time_t end_time);
        void _AddCreatureCategoryCooldown(uint32 category, time_t apply_time);
        void AddCreatureSpellCooldown(uint32 spellid);
        bool HasSpellCooldown(uint32 spell_id) const;
        bool HasCategoryCooldown(uint32 spell_id) const;

        bool HasSpell(uint32 spellID) const;

        bool UpdateEntry(uint32 entry, uint32 team=ALLIANCE, const CreatureData* data=NULL);
        bool UpdateStats(Stats stat);
        bool UpdateAllStats();
        void UpdateResistances(uint32 school);
        void UpdateArmor();
        void UpdateMaxHealth();
        void UpdateMaxPower(Powers power);
        void UpdateAttackPowerAndDamage(bool ranged = false);
        void UpdateDamagePhysical(WeaponAttackType attType);
        uint32 GetCurrentEquipmentId() { return m_equipmentId; }
        float GetSpellDamageMod(int32 Rank);

        VendorItemData const* GetVendorItems() const;
        uint32 GetVendorItemCurrentCount(VendorItem const* vItem);
        uint32 UpdateVendorItemCurrentCount(VendorItem const* vItem, uint32 used_count);

        TrainerSpellData const* GetTrainerSpells() const;

        CreatureInfo const *GetCreatureInfo() const { return m_creatureInfo; }
        CreatureDataAddon const* GetCreatureAddon() const;

        std::string GetScriptName();
        uint32 GetScriptId();

        void prepareGossipMenu( Player *pPlayer, uint32 gossipid = 0 );
        void sendPreparedGossip( Player* player );
        void OnGossipSelect(Player* player, uint32 option);
        void OnPoiSelect(Player* player, GossipOption const *gossip);

        uint32 GetGossipTextId(uint32 action, uint32 zoneid);
        uint32 GetNpcTextId();
        void LoadGossipOptions();
        void ResetGossipOptions();
        GossipOption const* GetGossipOption( uint32 id ) const;
        void addGossipOption(GossipOption const& gso) { m_goptions.push_back(gso); }

        void setEmoteState(uint8 emote) { m_emoteState = emote; };
        void Say(const char* text, uint32 language, uint64 TargetGuid) { MonsterSay(text,language,TargetGuid); }
        void Yell(const char* text, uint32 language, uint64 TargetGuid) { MonsterYell(text,language,TargetGuid); }
        void TextEmote(const char* text, uint64 TargetGuid, bool IsBossEmote = false) { MonsterTextEmote(text,TargetGuid,IsBossEmote); }
        void Whisper(const char* text, uint64 receiver, bool IsBossWhisper = false) { MonsterWhisper(text,receiver,IsBossWhisper); }
        void Say(int32 textId, uint32 language, uint64 TargetGuid) { MonsterSay(textId,language,TargetGuid); }
        void Yell(int32 textId, uint32 language, uint64 TargetGuid) { MonsterYell(textId,language,TargetGuid); }
        void TextEmote(int32 textId, uint64 TargetGuid, bool IsBossEmote = false) { MonsterTextEmote(textId,TargetGuid,IsBossEmote); }
        void Whisper(int32 textId, uint64 receiver, bool IsBossWhisper = false) { MonsterWhisper(textId,receiver,IsBossWhisper); }

        // overwrite WorldObject function for proper name localization
        const char* GetNameForLocaleIdx(int32 locale_idx) const;

        void setDeathState(DeathState s);                   // overwrite virtual Unit::setDeathState
        bool FallGround();

        bool LoadFromDB(uint32 guid, Map *map);
        void SaveToDB();
                                                            // overwrited in Pet
        virtual void SaveToDB(uint32 mapid, uint8 spawnMask);
        virtual void DeleteFromDB();                        // overwrited in Pet

        Loot loot;
        bool lootForPickPocketed;
        bool lootForBody;
        Player *GetLootRecipient() const;
        bool hasLootRecipient() const { return m_lootRecipient!=0; }

        void SetLootRecipient (Unit* unit);
        void AllLootRemovedFromCorpse();

        SpellEntry const *reachWithSpellAttack(Unit *pVictim);
        SpellEntry const *reachWithSpellCure(Unit *pVictim);

        uint32 m_spells[CREATURE_MAX_SPELLS];
        CreatureSpellCooldowns m_CreatureSpellCooldowns;
        CreatureSpellCooldowns m_CreatureCategoryCooldowns;
        uint32 m_GlobalCooldown;

        bool canSeeOrDetect(Unit const* u, bool detect, bool inVisibleList = false, bool is3dDistance = true) const;
        bool IsWithinSightDist(Unit const* u) const;
        bool canStartAttack(Unit const* u) const;
        float GetAttackDistance(Unit const* pl) const;

        Unit* SelectNearestTarget(float dist = 0) const;
        void CallAssistance();
        void SetNoCallAssistance(bool val) { m_AlreadyCallAssistance = val; }
        bool CanAssistTo(const Unit* u, const Unit* enemy) const;
        void DoFleeToGetAssistance(float radius = 50);

        MovementGeneratorType GetDefaultMovementType() const { return m_defaultMovementType; }
        void SetDefaultMovementType(MovementGeneratorType mgt) { m_defaultMovementType = mgt; }

        // for use only in LoadHelper, Map::Add Map::CreatureCellRelocation
        Cell const& GetCurrentCell() const { return m_currentCell; }
        void SetCurrentCell(Cell const& cell) { m_currentCell = cell; }

        bool IsVisibleInGridForPlayer(Player const* pl) const;

        void RemoveCorpse(bool setSpawnTime = true);
        
        void ForcedDespawn();

        time_t const& GetRespawnTime() const { return m_respawnTime; }
        time_t GetRespawnTimeEx() const;
        void SetRespawnTime(uint32 respawn) { m_respawnTime = respawn ? time(NULL) + respawn : 0; }
        void Respawn();
        void SaveRespawnTime();

        uint32 GetRespawnDelay() const { return m_respawnDelay; }
        void SetRespawnDelay(uint32 delay) { m_respawnDelay = delay; }

        float GetRespawnRadius() const { return m_respawnradius; }
        void SetRespawnRadius(float dist) { m_respawnradius = dist; }

        // Linked Creature Respawning System
        time_t GetLinkedCreatureRespawnTime() const;
        const CreatureData* GetLinkedRespawnCreatureData() const;

        void SendZoneUnderAttackMessage(Player* attacker);

        bool hasQuest(uint32 quest_id) const;
        bool hasInvolvedQuest(uint32 quest_id)  const;

        GridReference<Creature> &GetGridRef() { return m_gridRef; }
        bool isRegeneratingHealth() { return m_regenHealth; }
        virtual uint8 GetPetAutoSpellSize() const { return CREATURE_MAX_SPELLS; }
        virtual uint32 GetPetAutoSpellOnPos(uint8 pos) const
        {
            if (pos >= CREATURE_MAX_SPELLS || m_charmInfo->GetCharmSpell(pos)->active != ACT_ENABLED)
                return 0;
            else
                return m_charmInfo->GetCharmSpell(pos)->spellId;
        }

        void SetHomePosition(float x, float y, float z, float ori) { mHome_X = x; mHome_Y = y; mHome_Z = z; mHome_O = ori;}
        void GetHomePosition(float &x, float &y, float &z, float &ori) { x = mHome_X; y = mHome_Y; z = mHome_Z; ori = mHome_O; }

        uint32 GetGlobalCooldown() const { return m_GlobalCooldown; }

        uint32 GetWaypointPath(){return m_path_id;}
        void LoadPath(uint32 pathid) { m_path_id = pathid; }

        uint32 GetCurrentWaypointID(){return m_waypointID;}
        void UpdateWaypointID(uint32 wpID){m_waypointID = wpID;}

        void SearchFormation();
        CreatureGroup *GetFormation(){return m_formation;}
        void SetFormation(CreatureGroup *formation) {m_formation = formation;}

        Unit *SelectVictim();

        void SetDisableReputationGain(bool disable) { DisableReputationGain = disable; }
        bool IsReputationGainDisabled() { return DisableReputationGain; }
        bool IsDamageEnoughForLootingAndReward() { return m_PlayerDamageReq == 0; }
        void LowerPlayerDamageReq(uint32 unDamage)
        {
            if(m_PlayerDamageReq)
                m_PlayerDamageReq > unDamage ? m_PlayerDamageReq -= unDamage : m_PlayerDamageReq = 0;
        }
        void ResetPlayerDamageReq() { m_PlayerDamageReq = GetHealth() / 2; }
        uint32 m_PlayerDamageReq;
        
        uint32 GetQuestPoolId() { return m_questPoolId; }
        void SetQuestPoolId(uint32 id) { m_questPoolId = id; }
        
        void AllowToLoot(uint64 guid) { m_allowedToLoot.push_back(guid); }
        bool IsAllowedToLoot(uint64 guid);
        void ResetAllowedToLootList() { m_allowedToLoot.clear(); }

    protected:
        bool CreateFromProto(uint32 guidlow,uint32 Entry,uint32 team, const CreatureData *data = NULL);
        bool InitEntry(uint32 entry, uint32 team=ALLIANCE, const CreatureData* data=NULL);

        // vendor items
        VendorItemCounts m_vendorItemCounts;

        void _RealtimeSetCreatureInfo();

        static float _GetHealthMod(int32 Rank);
        static float _GetDamageMod(int32 Rank);

        uint32 m_lootMoney;
        uint64 m_lootRecipient;

        /// Timers
        uint32 m_corpseRemoveTime;                          // (msecs)timer for death or corpse disappearance
        time_t m_respawnTime;                               // (secs) time of next respawn
        uint32 m_respawnDelay;                              // (secs) delay between corpse disappearance and respawning
        uint32 m_corpseDelay;                               // (secs) delay between death and corpse disappearance
        float m_respawnradius;

        bool m_gossipOptionLoaded;
        GossipOptionList m_goptions;

        uint8 m_emoteState;
        bool m_isPet;                                       // set only in Pet::Pet
        bool m_isTotem;                                     // set only in Totem::Totem
        ReactStates m_reactState;                           // for AI, not charmInfo
        void RegenerateMana();
        void RegenerateHealth();
        uint32 m_regenTimer;
        uint32 m_areaCombatTimer;
        void AreaCombat();
        MovementGeneratorType m_defaultMovementType;
        Cell m_currentCell;                                 // store current cell where creature listed
        uint32 m_DBTableGuid;                               ///< For new or temporary creatures is 0 for saved it is lowguid
        uint32 m_equipmentId;

        bool m_AlreadyCallAssistance;
        bool m_regenHealth;
        bool m_AI_locked;
        bool m_isDeadByDefault;

        SpellSchoolMask m_meleeDamageSchoolMask;
        uint32 m_originalEntry;

        float mHome_X;
        float mHome_Y;
        float mHome_Z;
        float mHome_O;

        bool DisableReputationGain;
        
        uint32 m_questPoolId;
        
        uint32 m_creaturePoolId;
        
        std::vector<uint64> m_allowedToLoot;
        
        uint64 m_timeSinceSpawn;                            // (msecs) elapsed time since (re)spawn
        bool m_changedReactStateAfterFiveSecs;

    private:
        //WaypointMovementGenerator vars
        uint32 m_waypointID;
        uint32 m_path_id;

        //Formation var
        CreatureGroup *m_formation;
        bool TriggerJustRespawned;

        GridReference<Creature> m_gridRef;
        CreatureInfo const* m_creatureInfo;                 // in heroic mode can different from ObjMgr::GetCreatureTemplate(GetEntry())
};

class AssistDelayEvent : public BasicEvent
{
    public:
        AssistDelayEvent(const uint64& victim, Unit& owner) : BasicEvent(), m_victim(victim), m_owner(owner) { }

        bool Execute(uint64 e_time, uint32 p_time);
        void AddAssistant(const uint64& guid) { m_assistants.push_back(guid); }
    private:
        AssistDelayEvent();

        uint64            m_victim;
        std::list<uint64> m_assistants;
        Unit&             m_owner;
};

#endif

