/* Copyright (C) 2006 - 2008 TrinityScript <https://scriptdev2.svn.sourceforge.net/>
 * This program is free software licensed under GPL version 2
 * Please see the included DOCS/LICENSE.TXT for more information */

#include "precompiled.h"
#include "Config/Config.h"
#include "Database/DatabaseEnv.h"
#include "Database/DBCStores.h"
#include "ObjectMgr.h"
#include "EventAI.h"
#include "Policies/SingletonImp.h"
#include "Spell.h"

class CreatureScript;

#define _FULLVERSION "TrinityScript"

INSTANTIATE_SINGLETON_1(ScriptMgr);

#ifndef _TRINITY_SCRIPT_CONFIG
# define _TRINITY_SCRIPT_CONFIG  "trinitycore.conf"
#endif _TRINITY_SCRIPT_CONFIG

//*** Global data ***
int num_sc_scripts;
Script *m_scripts[MAX_SCRIPTS];

DatabaseType TScriptDB;
Config TScriptConfig;

// String text additional data, used in TextMap
struct StringTextData
{
    uint32 SoundId;
    uint8  Type;
    uint32 Language;
    uint32 Emote;
};

#define TEXT_SOURCE_RANGE   -1000000                        //the amount of entries each text source has available

// Text Maps
UNORDERED_MAP<int32, StringTextData> TextMap;

//*** End Global data ***

//*** EventAI data ***
//Event AI structure. Used exclusivly by mob_event_ai.cpp (60 bytes each)
UNORDERED_MAP<uint32, std::vector<EventAI_Event> > EventAI_Event_Map;

//Event AI summon structure. Used exclusivly by mob_event_ai.cpp.
UNORDERED_MAP<uint32, EventAI_Summon> EventAI_Summon_Map;

//Event AI error prevention structure. Used at runtime to prevent error log spam of same creature id.
//UNORDERED_MAP<uint32, EventAI_CreatureError> EventAI_CreatureErrorPreventionList;

uint32 EAI_ErrorLevel;
//*** End EventAI data ***

void FillSpellSummary();
void LoadOverridenSQLData();
void LoadOverridenDBCData();

// -- Scripts to be added --

// -- Areatrigger --
extern void AddSC_areatrigger_scripts();

// -- Outdoors dragons --
extern void AddSC_boss_dragonsofnightmare();

// -- Creature --
extern void AddSC_mob_event();
extern void AddSC_generic_creature();

// -- Custom --
extern void AddSC_npc_rez();
extern void AddSC_training_dummy();
extern void AddSC_zone_silence();
extern void AddSC_custom_example();
extern void AddSC_custom_gossip_codebox();
extern void AddSC_test();
extern void AddSC_onevents();
extern void AddSC_npc_lottery();
extern void AddSC_theinform();
extern void AddSC_mylittlebombling();
extern void AddSC_firework_controller();
extern void AddSC_npc_interpreter();
extern void AddSC_custom_gnominizer();

extern void AddSC_catapultmaster();
extern void AddSC_npc_teleporter();
extern void AddSC_npc_teleporter_pvpzone();

// -- GO --
extern void AddSC_go_scripts();

// -- Guard --
extern void AddSC_guards();

// -- Honor --

// -- Item --
extern void AddSC_item_scripts();
extern void AddSC_item_test();

// -- NPC --
extern void AddSC_npc_professions();
extern void AddSC_npcs_special();
extern void AddSC_npc_xp_blocker();

// -- Servers --
extern void AddSC_SmartSCripts();

// -- Misc --
extern void AddSC_arena_spectator_script();
extern void AddSC_hallows_end();
extern void AddSC_arenabeastmaster();

//--------------------
//------ ZONE --------

//Alterac Mountains
extern void AddSC_alterac_mountains();

//Alterac Valley
extern void AddSC_AV_Marshals();
extern void AddSC_AV_Warmasters();
extern void AddSC_boss_balinda();
extern void AddSC_boss_drekthar();
extern void AddSC_boss_galvangar();
extern void AddSC_boss_vanndar();
extern void AddSC_alterac_bowman();

//Arathi Highlands
extern void AddSC_arathi_highlands();

//Ashenvale Forest
extern void AddSC_ashenvale();

//Aunchindoun
//--Auchenai Crypts
extern void AddSC_boss_exarch_maladaar();
extern void AddSC_boss_shirrak_the_dead_watcher();

//--Mana Tombs
extern void AddSC_boss_nexusprince_shaffar();
extern void AddSC_boss_pandemonius();

//--Sekketh Halls
extern void AddSC_boss_darkweaver_syth();
extern void AddSC_boss_talon_king_ikiss();
extern void AddSC_boss_anzu();
extern void AddSC_instance_sethekk_halls();
extern void AddSC_sethekk_halls();

//--Shadow Labyrinth
extern void AddSC_boss_ambassador_hellmaw();
extern void AddSC_boss_blackheart_the_inciter();
extern void AddSC_boss_grandmaster_vorpil();
extern void AddSC_boss_murmur();
extern void AddSC_instance_shadow_labyrinth();

//Azshara
extern void AddSC_boss_azuregos();
extern void AddSC_azshara();

//Azuremyst Isle
extern void AddSC_azuremyst_isle();

//Badlands
//Barrens
extern void AddSC_the_barrens();

//Black Temple
extern void AddSC_black_temple();
extern void AddSC_boss_illidan();
extern void AddSC_boss_shade_of_akama();
extern void AddSC_boss_supremus();
extern void AddSC_boss_gurtogg_bloodboil();
extern void AddSC_boss_mother_shahraz();
extern void AddSC_boss_reliquary_of_souls();
extern void AddSC_boss_teron_gorefiend();
extern void AddSC_boss_najentus();
extern void AddSC_boss_illidari_council();
extern void AddSC_instance_black_temple();

//Blackfathom Depths
extern void AddSC_blackfathom_deeps();
extern void AddSC_boss_gelihast();
extern void AddSC_boss_kelris();
extern void AddSC_boss_aku_mai();
extern void AddSC_instance_blackfathom_deeps();

//Blackrock Depths
extern void AddSC_blackrock_depths();
extern void AddSC_boss_ambassador_flamelash();
extern void AddSC_boss_anubshiah();
extern void AddSC_boss_draganthaurissan();
extern void AddSC_boss_general_angerforge();
extern void AddSC_boss_gorosh_the_dervish();
extern void AddSC_boss_grizzle();
extern void AddSC_boss_high_interrogator_gerstahn();
extern void AddSC_boss_magmus();
extern void AddSC_boss_moira_bronzebeard();
extern void AddSC_boss_tomb_of_seven();
extern void AddSC_instance_blackrock_depths();

//Blackrock Spire
extern void AddSC_boss_drakkisath();
extern void AddSC_boss_halycon();
extern void AddSC_boss_highlordomokk();
extern void AddSC_boss_mothersmolderweb();
extern void AddSC_boss_overlordwyrmthalak();
extern void AddSC_boss_shadowvosh();
extern void AddSC_boss_thebeast();
extern void AddSC_boss_warmastervoone();
extern void AddSC_boss_quatermasterzigris();
extern void AddSC_boss_pyroguard_emberseer();
extern void AddSC_boss_gyth();
extern void AddSC_boss_rend_blackhand();

//Blackwing lair
extern void AddSC_boss_razorgore();
extern void AddSC_boss_vael();
extern void AddSC_boss_broodlord();
extern void AddSC_boss_firemaw();
extern void AddSC_boss_ebonroc();
extern void AddSC_boss_flamegor();
extern void AddSC_boss_chromaggus();
extern void AddSC_boss_nefarian();
extern void AddSC_boss_victor_nefarius();

//Blade's Edge Mountains
extern void AddSC_blades_edge_mountains();
extern void AddSC_shartuul_event();

//Blasted lands
extern void AddSC_boss_kruul();
extern void AddSC_blasted_lands();

//Bloodmyst Isle
extern void AddSC_bloodmyst_isle();

//Burning steppes
extern void AddSC_burning_steppes();

//Caverns of Time
//--Battle for Mt. Hyjal
extern void AddSC_hyjal();
extern void AddSC_boss_archimonde();
extern void AddSC_boss_archimonde_new();
extern void AddSC_instance_mount_hyjal();
extern void AddSC_hyjal_trash();
extern void AddSC_boss_rage_winterchill();
extern void AddSC_boss_anetheron();
extern void AddSC_boss_kazrogal();
extern void AddSC_boss_azgalor();

//--Old Hillsbrad
extern void AddSC_boss_captain_skarloc();
extern void AddSC_boss_epoch_hunter();
extern void AddSC_boss_lieutenant_drake();
extern void AddSC_instance_old_hillsbrad();
extern void AddSC_old_hillsbrad();

//--The Dark Portal
extern void AddSC_boss_aeonus();
extern void AddSC_boss_chrono_lord_deja();
extern void AddSC_boss_temporus();
extern void AddSC_dark_portal();
extern void AddSC_instance_dark_portal();

//Coilfang Resevoir
//--Serpent Shrine Cavern
extern void AddSC_boss_fathomlord_karathress();
extern void AddSC_boss_hydross_the_unstable();
extern void AddSC_boss_lady_vashj();
extern void AddSC_boss_leotheras_the_blind();
extern void AddSC_boss_morogrim_tidewalker();
extern void AddSC_instance_serpentshrine_cavern();
extern void AddSC_boss_the_lurker_below();

//--Slave Pens
extern void AddSC_boss_ahune();
extern void AddSC_instance_slave_pens();

//--Steam Vault
extern void AddSC_boss_hydromancer_thespia();
extern void AddSC_boss_mekgineer_steamrigger();
extern void AddSC_boss_warlord_kalithresh();
extern void AddSC_instance_steam_vault();

//--Underbog
extern void AddSC_boss_hungarfen();
extern void AddSC_boss_the_black_stalker();

//Darkshore
extern void AddSC_darkshore();
//Darnassus
//Deadmines
extern void AddSC_instance_deadmines();

//Deadwind pass
//Desolace
extern void AddSC_desolace();

//Dire Maul
extern void AddSC_instance_dire_maul();
extern void AddSC_dire_maul();
extern void AddSC_boss_pusillin();

//Dun Morogh
extern void AddSC_dun_morogh();

//Durotar
extern void AddSC_durotar();

//Duskwood
//Dustwallow marsh
extern void AddSC_dustwallow_marsh();

//Eversong Woods
extern void AddSC_eversong_woods();

//Exodar
//Eastern Plaguelands
extern void AddSC_eastern_plaguelands();

//Elwynn Forest
extern void AddSC_elwynn_forest();

//Felwood
extern void AddSC_felwood();

//Feralas
extern void AddSC_feralas();

//Ghostlands
extern void AddSC_ghostlands();

//Gnomeregan
//Gruul's Lair
extern void AddSC_boss_gruul();
extern void AddSC_boss_high_king_maulgar();
extern void AddSC_instance_gruuls_lair();

//Hellfire Citadel
//--Blood Furnace
extern void AddSC_boss_broggok();
extern void AddSC_boss_kelidan_the_breaker();
extern void AddSC_boss_the_maker();
extern void AddSC_instance_blood_furnace();

//--Magtheridon's Lair
extern void AddSC_boss_magtheridon();
extern void AddSC_instance_magtheridons_lair();

//--Shattered Halls
extern void AddSC_boss_grand_warlock_nethekurse();
extern void AddSC_boss_warbringer_omrogg();
extern void AddSC_boss_warchief_kargath_bladefist();
extern void AddSC_instance_shattered_halls();

//--Ramparts
extern void AddSC_boss_watchkeeper_gargolmar();
extern void AddSC_boss_omor_the_unscarred();
extern void AddSC_boss_vazruden_the_herald();

//Hellfire Peninsula
extern void AddSC_boss_doomlordkazzak();
extern void AddSC_hellfire_peninsula();

//Hillsbrad Foothills
//Hinterlands
extern void AddSC_hinterlands();

//Ironforge
extern void AddSC_ironforge();

//Isle of Quel'Danas
extern void AddSC_isle_of_queldanas();

//Karazhan
extern void AddSC_boss_attumen();
extern void AddSC_boss_curator();
extern void AddSC_boss_maiden_of_virtue();
extern void AddSC_boss_shade_of_aran();
extern void AddSC_boss_malchezaar();
extern void AddSC_boss_terestian_illhoof();
extern void AddSC_boss_moroes();
extern void AddSC_bosses_opera();
extern void AddSC_boss_netherspite();
extern void AddSC_instance_karazhan();
extern void AddSC_karazhan();
extern void AddSC_boss_nightbane();
extern void AddSC_chess_event();

//Loch Modan
extern void AddSC_loch_modan();

//Lower Blackrock Spire
extern void AddSC_instance_blackrock_spire();

// Magister's Terrace
extern void AddSC_boss_felblood_kaelthas();
extern void AddSC_boss_selin_fireheart();
extern void AddSC_boss_vexallus();
extern void AddSC_boss_priestess_delrissa();
extern void AddSC_instance_magisters_terrace();

//Maraudon
extern void AddSC_boss_celebras_the_cursed();
extern void AddSC_boss_landslide();
extern void AddSC_boss_noxxion();
extern void AddSC_boss_ptheradras();

//Molten core
extern void AddSC_boss_lucifron();
extern void AddSC_boss_magmadar();
extern void AddSC_boss_gehennas();
extern void AddSC_boss_garr();
extern void AddSC_boss_baron_geddon();
extern void AddSC_boss_shazzrah();
extern void AddSC_boss_golemagg();
extern void AddSC_boss_sulfuron();
extern void AddSC_boss_majordomo();
extern void AddSC_boss_ragnaros();
extern void AddSC_instance_molten_core();
extern void AddSC_molten_core();

//Moonglade
extern void AddSC_moonglade();

//Mulgore
extern void AddSC_mulgore();

//Nagrand
extern void AddSC_nagrand();

//Naxxramas
extern void AddSC_boss_anubrekhan();
extern void AddSC_boss_maexxna();
extern void AddSC_boss_patchwerk();
extern void AddSC_boss_razuvious();
extern void AddSC_boss_highlord_mograine();
extern void AddSC_boss_kelthuzad();
extern void AddSC_boss_faerlina();
extern void AddSC_boss_loatheb();
extern void AddSC_boss_noth();
extern void AddSC_boss_gluth();
extern void AddSC_boss_sapphiron();
extern void AddSC_boss_four_horsemen();

//Netherstorm
extern void AddSC_netherstorm();

//Onyxia's Lair
extern void AddSC_boss_onyxia();

//Orgrimmar
extern void AddSC_orgrimmar();

//Ragefire Chasm
//Razorfen Downs
extern void AddSC_boss_tuten_kash();
extern void AddSC_boss_plaguemaw_the_rotting();
extern void AddSC_boss_ragglesnout();
extern void AddSC_boss_glutton();
extern void AddSC_boss_mordresh_fire_eye();
extern void AddSC_boss_amnennar_the_coldbringer();
extern void AddSC_razorfen_downs();
extern void AddSC_instance_razorfen_downs();

//Razorfen Kraul
extern void AddSC_razorfen_kraul();
extern void AddSC_instance_razorfen_kraul();

//Redridge Mountains
extern void AddSC_redridge_mountains();

//Ruins of Ahn'Qiraj
extern void AddSC_boss_kurinnaxx();
extern void AddSC_boss_rajaxx();
extern void AddSC_boss_moam();
extern void AddSC_boss_buru();
extern void AddSC_boss_ayamiss();
extern void AddSC_boss_ossirian();
extern void AddSC_instance_ruins_of_ahnqiraj();
//Scarlet Monastery
extern void AddSC_boss_arcanist_doan();
extern void AddSC_boss_azshir_the_sleepless();
extern void AddSC_boss_bloodmage_thalnos();
extern void AddSC_boss_headless_horseman();
extern void AddSC_boss_herod();
extern void AddSC_boss_high_inquisitor_fairbanks();
extern void AddSC_boss_houndmaster_loksey();
extern void AddSC_boss_interrogator_vishas();
extern void AddSC_boss_scorn();
extern void AddSC_instance_scarlet_monastery();
extern void AddSC_boss_mograine_and_whitemane();

//Scholomance
extern void AddSC_boss_darkmaster_gandling();
extern void AddSC_boss_death_knight_darkreaver();
extern void AddSC_boss_theolenkrastinov();
extern void AddSC_boss_illuciabarov();
extern void AddSC_boss_instructormalicia();
extern void AddSC_boss_jandicebarov();
extern void AddSC_boss_kormok();
extern void AddSC_boss_lordalexeibarov();
extern void AddSC_boss_lorekeeperpolkelt();
extern void AddSC_boss_rasfrost();
extern void AddSC_boss_theravenian();
extern void AddSC_boss_vectus();
extern void AddSC_instance_scholomance();
extern void AddSC_scholomance();

//Searing gorge
extern void AddSC_searing_gorge();

//Shadowfang keep
extern void AddSC_shadowfang_keep();
extern void AddSC_instance_shadowfang_keep();

//Shadowmoon Valley
extern void AddSC_boss_doomwalker();
extern void AddSC_shadowmoon_valley();

//Shattrath
extern void AddSC_shattrath_city();

//Silithus
extern void AddSC_silithus();

//Silvermoon
extern void AddSC_silvermoon_city();

//Silverpine forest
extern void AddSC_silverpine_forest();

//Stockade
//Stonetalon mountains
extern void AddSC_stonetalon_mountains();

//Stormwind City
extern void AddSC_stormwind_city();

//Stranglethorn Vale
extern void AddSC_stranglethorn_vale();

//Stratholme
extern void AddSC_boss_magistrate_barthilas();
extern void AddSC_boss_maleki_the_pallid();
extern void AddSC_boss_nerubenkan();
extern void AddSC_boss_cannon_master_willey();
extern void AddSC_boss_baroness_anastari();
extern void AddSC_boss_ramstein_the_gorger();
extern void AddSC_boss_timmy_the_cruel();
extern void AddSC_boss_postmaster_malown();
extern void AddSC_boss_baron_rivendare();
extern void AddSC_boss_dathrohan_balnazzar();
extern void AddSC_boss_order_of_silver_hand();
extern void AddSC_instance_stratholme();
extern void AddSC_stratholme();

//Sunken Temple
extern void AddSC_instance_sunken_temple();
extern void AddSC_sunken_temple();

//Sunwell Plateau
extern void AddSC_instance_sunwell_plateau();
extern void AddSC_boss_kalecgos();
extern void AddSC_boss_brutallus();
extern void AddSC_boss_felmyst();
extern void AddSC_boss_eredar_twins();
extern void AddSC_boss_muru();
extern void AddSC_boss_kiljaeden_new();
extern void AddSC_sunwell_plateau();

//Swamp of Sorrows
extern void AddSC_swamp_of_sorrows();

//Tanaris
extern void AddSC_tanaris();

//Teldrassil
extern void AddSC_teldrassil();

//Tempest Keep
//--Arcatraz
extern void AddSC_arcatraz();
extern void AddSC_boss_harbinger_skyriss();
extern void AddSC_instance_arcatraz();

//--Botanica
extern void AddSC_boss_high_botanist_freywinn();
extern void AddSC_boss_laj();
extern void AddSC_boss_warp_splinter();

//--The Eye
extern void AddSC_boss_alar();
extern void AddSC_boss_kaelthas();
extern void AddSC_boss_void_reaver();
extern void AddSC_boss_high_astromancer_solarian();
extern void AddSC_instance_the_eye();
extern void AddSC_the_eye();

//--The Mechanar
extern void AddSC_boss_mechanolord_capacitus();
extern void AddSC_boss_gatewatcher_iron_hand();
extern void AddSC_boss_nethermancer_sepethrea();
extern void AddSC_boss_pathaleon_the_calculator();
extern void AddSC_instance_mechanar();

//Temple of ahn'qiraj
extern void AddSC_boss_cthun();
extern void AddSC_boss_fankriss();
extern void AddSC_boss_huhuran();
extern void AddSC_bug_trio();
extern void AddSC_boss_sartura();
extern void AddSC_boss_skeram();
extern void AddSC_boss_twinemperors();
extern void AddSC_mob_anubisath_sentinel();
extern void AddSC_instance_temple_of_ahnqiraj();
extern void AddSC_boss_ouro();

//Terokkar Forest
extern void AddSC_terokkar_forest();

//Thousand Needles
extern void AddSC_thousand_needles();

//Thunder Bluff
extern void AddSC_thunder_bluff();

//The Balance of Light and Shadow - Epic priest quest
extern void AddSC_the_balance_of_light_and_shadow();

//Tirisfal Glades
extern void AddSC_tirisfal_glades();

//Uldaman
extern void AddSC_boss_archaedas();
extern void AddSC_boss_ironaya();
extern void AddSC_uldaman();
extern void AddSC_instance_uldaman();

//Undercity
extern void AddSC_undercity();

//Un'Goro Crater
extern void AddSC_ungoro_crater();

//Upper blackrock spire
//Wailing caverns
extern void AddSC_instance_wailing_caverns();
extern void AddSC_wailing_caverns();

//Western plaguelands
extern void AddSC_western_plaguelands();

//Westfall
extern void AddSC_westfall();

//Wetlands
extern void AddSC_wetlands();

//Winterspring
extern void AddSC_winterspring();

//Zangarmarsh
extern void AddSC_zangarmarsh();

//Zul'Farrak
extern void AddSC_zulfarrak();
extern void AddSC_instance_zulfarrak();

//Zul'Gurub
extern void AddSC_boss_jeklik();
extern void AddSC_boss_venoxis();
extern void AddSC_boss_marli();
extern void AddSC_boss_mandokir();
extern void AddSC_boss_gahzranka();
extern void AddSC_boss_thekal();
extern void AddSC_boss_arlokk();
extern void AddSC_boss_jindo();
extern void AddSC_boss_hakkar();
extern void AddSC_boss_grilek();
extern void AddSC_boss_hazzarah();
extern void AddSC_boss_renataki();
extern void AddSC_boss_wushoolay();
extern void AddSC_instance_zulgurub();

//Zul'Aman
extern void AddSC_boss_akilzon();
extern void AddSC_boss_halazzi();
extern void AddSC_boss_hex_lord_malacrass();
extern void AddSC_boss_janalai();
extern void AddSC_boss_nalorakk();
extern void AddSC_boss_zuljin();
extern void AddSC_instance_zulaman();
extern void AddSC_zulaman();

// -------------------
void ScriptMgr::LoadDatabase()
{
    //Get db string from file
    char const* dbstring = NULL;

    if (!TScriptConfig.GetString("WorldDatabaseInfo", &dbstring) )
    {
        error_log("TSCR: Missing world database info from configuration file. Load database aborted.");
        return;
    }

    //Initialize connection to DB
    uint8 num_threads = sConfig.GetIntDefault("WorldDatabase.WorkerThreads", 1);
    if (num_threads < 1 || num_threads > 32) {
        sLog.outError("World database: invalid number of worker threads specified. "
            "Please pick a value between 1 and 32.");
        return;
    }
    
    if (TScriptDB.Open(dbstring, num_threads))
        sLog.outString("TSCR: TrinityScript database: %s",dbstring);
    else
    {
        error_log("TSCR: Unable to connect to Database. Load database aborted.");
        return;
    }

    //***Preform all DB queries here***
    QueryResult *result;

    //Get Version information
    result = TScriptDB.PQuery("SELECT script_version FROM version LIMIT 1");

    if (result)
    {
        Field *fields = result->Fetch();
        sLog.outString("TSCR: Database version is: %s\n", fields[0].GetString());
        delete result;

    }else
    {
        error_log("TSCR: Missing `version.script_version` information.\n");
    }

    // Drop Existing Text Map, only done once and we are ready to add data from multiple sources.
    TextMap.clear();

    // Load EventAI Text
    sLog.outString("TSCR: Loading EventAI Texts...");
    LoadTrinityStrings(TScriptDB,"eventai_texts",-1,1+(TEXT_SOURCE_RANGE));

    // Gather Additional data from EventAI Texts
    //result = TScriptDB.PQuery("SELECT entry, sound, type, language, emote FROM eventai_texts");
    result = TScriptDB.PQuery("SELECT entry, sound, type, language FROM eventai_texts");

    sLog.outString("TSCR: Loading EventAI Texts additional data...");
    if (result)
    {
        uint32 count = 0;

        do
        {
            Field* fields = result->Fetch();
            StringTextData temp;

            int32 i             = fields[0].GetInt32();
            temp.SoundId        = fields[1].GetInt32();
            temp.Type           = fields[2].GetInt32();
            temp.Language       = fields[3].GetInt32();
            temp.Emote          = 0;//fields[4].GetInt32();

            if (i >= 0)
            {
                error_db_log("TSCR: Entry %i in table `eventai_texts` is not a negative value.",i);
                continue;
            }

            if (i <= TEXT_SOURCE_RANGE)
            {
                error_db_log("TSCR: Entry %i in table `eventai_texts` is out of accepted entry range for table.",i);
                continue;
            }

            if (temp.SoundId)
            {
                if (!GetSoundEntriesStore()->LookupEntry(temp.SoundId))
                    error_db_log("TSCR: Entry %i in table `eventai_texts` has soundId %u but sound does not exist.",i,temp.SoundId);
            }

            if (!GetLanguageDescByID(temp.Language))
                error_db_log("TSCR: Entry %i in table `eventai_texts` using Language %u but Language does not exist.",i,temp.Language);

            if (temp.Type > CHAT_TYPE_BOSS_WHISPER)
                error_db_log("TSCR: Entry %i in table `eventai_texts` has Type %u but this Chat Type does not exist.",i,temp.Type);

            TextMap[i] = temp;
            ++count;
        } while (result->NextRow());

        delete result;

        sLog.outString("\n>> TSCR: Loaded %u additional EventAI Texts data.", count);
    }else
    {
        sLog.outString("\n>> Loaded 0 additional EventAI Texts data. DB table `eventai_texts` is empty.");
    }

    // Load Script Text
    sLog.outString("TSCR: Loading Script Texts...");
    LoadTrinityStrings(TScriptDB,"script_texts",TEXT_SOURCE_RANGE,1+(TEXT_SOURCE_RANGE*2));

    // Gather Additional data from Script Texts
    result = TScriptDB.PQuery("SELECT entry, sound, type, language, emote FROM script_texts");

    sLog.outString("TSCR: Loading Script Texts additional data...");
    if (result)
    {
        uint32 count = 0;

        do
        {
            Field* fields = result->Fetch();
            StringTextData temp;

            int32 i             = fields[0].GetInt32();
            temp.SoundId        = fields[1].GetInt32();
            temp.Type           = fields[2].GetInt32();
            temp.Language       = fields[3].GetInt32();
            temp.Emote          = fields[4].GetInt32();

            if (i >= 0)
            {
                error_db_log("TSCR: Entry %i in table `script_texts` is not a negative value.",i);
                continue;
            }

            if (i > TEXT_SOURCE_RANGE || i <= TEXT_SOURCE_RANGE*2)
            {
                error_db_log("TSCR: Entry %i in table `script_texts` is out of accepted entry range for table.",i);
                continue;
            }

            if (temp.SoundId)
            {
                if (!GetSoundEntriesStore()->LookupEntry(temp.SoundId))
                    error_db_log("TSCR: Entry %i in table `script_texts` has soundId %u but sound does not exist.",i,temp.SoundId);
            }

            if (!GetLanguageDescByID(temp.Language))
                error_db_log("TSCR: Entry %i in table `script_texts` using Language %u but Language does not exist.",i,temp.Language);

            if (temp.Type >= CHAT_TYPE_END)
                error_db_log("TSCR: Entry %i in table `script_texts` has Type %u but this Chat Type does not exist.",i,temp.Type);

            TextMap[i] = temp;
            ++count;
        } while (result->NextRow());

        delete result;

        sLog.outString("\n>> TSCR: Loaded %u additional Script Texts data.", count);
    }else
    {
        sLog.outString("\n>> Loaded 0 additional Script Texts data. DB table `script_texts` is empty.");
    }

    // Load Custom Text
    sLog.outString("TSCR: Loading Custom Texts...");
    LoadTrinityStrings(TScriptDB,"custom_texts",TEXT_SOURCE_RANGE*2,1+(TEXT_SOURCE_RANGE*3));

    // Gather Additional data from Custom Texts
    result = TScriptDB.PQuery("SELECT entry, sound, type, language, emote FROM custom_texts");

    sLog.outString("TSCR: Loading Custom Texts additional data...");
    if (result)
    {
        uint32 count = 0;

        do
        {
            Field* fields = result->Fetch();
            StringTextData temp;

            int32 i             = fields[0].GetInt32();
            temp.SoundId        = fields[1].GetInt32();
            temp.Type           = fields[2].GetInt32();
            temp.Language       = fields[3].GetInt32();
            temp.Emote          = fields[4].GetInt32();

            if (i >= 0)
            {
                error_db_log("TSCR: Entry %i in table `custom_texts` is not a negative value.",i);
                continue;
            }

            if (i > TEXT_SOURCE_RANGE*2 || i <= TEXT_SOURCE_RANGE*3)
            {
                error_db_log("TSCR: Entry %i in table `custom_texts` is out of accepted entry range for table.",i);
                continue;
            }

            if (temp.SoundId)
            {
                if (!GetSoundEntriesStore()->LookupEntry(temp.SoundId))
                    error_db_log("TSCR: Entry %i in table `custom_texts` has soundId %u but sound does not exist.",i,temp.SoundId);
            }

            if (!GetLanguageDescByID(temp.Language))
                error_db_log("TSCR: Entry %i in table `custom_texts` using Language %u but Language does not exist.",i,temp.Language);

            if (temp.Type > CHAT_TYPE_BOSS_WHISPER)
                error_db_log("TSCR: Entry %i in table `custom_texts` has Type %u but this Chat Type does not exist.",i,temp.Type);

            TextMap[i] = temp;
            ++count;
        } while (result->NextRow());

        delete result;

        sLog.outString("\n>> Loaded %u additional Custom Texts data.", count);
    }else
    {
        sLog.outString("\n>> Loaded 0 additional Custom Texts data. DB table `custom_texts` is empty.");
    }

    //Gather additional data for EventAI
    result = TScriptDB.PQuery("SELECT id, position_x, position_y, position_z, orientation, spawntimesecs FROM eventai_summons");

    //Drop Existing EventSummon Map
    EventAI_Summon_Map.clear();

    sLog.outString("TSCR: Loading EventAI Summons...");
    if (result)
    {
        uint32 Count = 0;

        do
        {
            Field *fields = result->Fetch();

            EventAI_Summon temp;

            uint32 i = fields[0].GetUInt32();
            temp.position_x = fields[1].GetFloat();
            temp.position_y = fields[2].GetFloat();
            temp.position_z = fields[3].GetFloat();
            temp.orientation = fields[4].GetFloat();
            temp.SpawnTimeSecs = fields[5].GetUInt32();

            //Add to map
            EventAI_Summon_Map[i] = temp;
            ++Count;
        }while (result->NextRow());

        delete result;

        sLog.outString("\n>> Loaded %u EventAI summon definitions", Count);
    }else
    {
        sLog.outString("\n>> Loaded 0 EventAI Summon definitions. DB table `eventai_summons` is empty.");
    }

    //Gather event data
    result = TScriptDB.PQuery("SELECT id, creature_id, event_type, event_inverse_phase_mask, event_chance, event_flags, "
        "event_param1, event_param2, event_param3, event_param4, "
        "action1_type, action1_param1, action1_param2, action1_param3, "
        "action2_type, action2_param1, action2_param2, action2_param3, "
        "action3_type, action3_param1, action3_param2, action3_param3 "
        "FROM eventai_scripts");

    //Drop Existing EventAI List
    EventAI_Event_Map.clear();

    sLog.outString("TSCR: Loading EventAI scripts...");
    if (result)
    {
        uint32 Count = 0;

        do
        {
            Field *fields = result->Fetch();

            EventAI_Event temp;

            temp.event_id = fields[0].GetUInt32();
            uint32 i = temp.event_id;
            temp.creature_id = fields[1].GetUInt32();
            uint32 creature_id = temp.creature_id;
            temp.event_type = fields[2].GetUInt16();
            temp.event_inverse_phase_mask = fields[3].GetUInt32();
            temp.event_chance = fields[4].GetUInt8();
            temp.event_flags = fields[5].GetUInt8();
            temp.event_param1 = fields[6].GetUInt32();
            temp.event_param2 = fields[7].GetUInt32();
            temp.event_param3 = fields[8].GetUInt32();
            temp.event_param4 = fields[9].GetUInt32();

            //Creature does not exist in database
            if (!GetCreatureTemplateStore(temp.creature_id))
                error_db_log("TSCR: Event %u has script for non-existing creature.", i);

            //Report any errors in event
            if (temp.event_type >= EVENT_T_END)
                error_db_log("TSCR: Event %u has incorrect event type. Maybe DB requires updated version of SD2.", i);

            //No chance of this event occuring
            if (temp.event_chance == 0)
                error_db_log("TSCR: Event %u has 0 percent chance. Event will never trigger!", i);

            //Chance above 100, force it to be 100
            if (temp.event_chance > 100)
            {
                error_db_log("TSCR: Creature %u are using event %u with more than 100 percent chance. Adjusting to 100 percent.", temp.creature_id, i);
                temp.event_chance = 100;
            }

            //Individual event checks
            switch (temp.event_type)
            {
                case EVENT_T_HP:
                case EVENT_T_MANA:
                case EVENT_T_TARGET_HP:
                    {
                        if (temp.event_param2 > 100)
                            error_db_log("TSCR: Creature %u are using percentage event(%u) with param2 (MinPercent) > 100. Event will never trigger! ", temp.creature_id, i);

                        if (temp.event_param1 <= temp.event_param2)
                            error_db_log("TSCR: Creature %u are using percentage event(%u) with param1 <= param2 (MaxPercent <= MinPercent). Event will never trigger! ", temp.creature_id, i);

                        if (temp.event_flags & EFLAG_REPEATABLE && !temp.event_param3 && !temp.event_param4)
                        {
                            error_db_log("TSCR: Creature %u has param3 and param4=0 (RepeatMin/RepeatMax) but cannot be repeatable without timers. Removing EFLAG_REPEATABLE for event %u.", temp.creature_id, i);
                            temp.event_flags &= ~EFLAG_REPEATABLE;
                        }
                    }
                    break;

                case EVENT_T_SPELLHIT:
                    {
                        if (temp.event_param1)
                        {
                            SpellEntry const* pSpell = spellmgr.LookupSpell(temp.event_param1);
                            if (!pSpell)
                            {
                                error_db_log("TSCR: Creature %u has non-existant SpellID(%u) defined in event %u.", temp.creature_id, temp.event_param1, i);
                                continue;
                            }

                            if (temp.event_param2_s != -1 && temp.event_param2 != pSpell->SchoolMask)
                                error_db_log("TSCR: Creature %u has param1(spellId %u) but param2 is not -1 and not equal to spell's school mask. Event %u can never trigger.", temp.creature_id, temp.event_param1, i);
                        }

                        //TODO: fix this system with SPELL_SCHOOL_MASK. Current complicate things, using int32(-1) instead of just 0
                        //SPELL_SCHOOL_MASK_NONE = 0 and does not exist, thus it can not ever trigger or be used in SpellHit()
                        if (temp.event_param2_s != -1 && temp.event_param2_s > SPELL_SCHOOL_MASK_ALL)
                            error_db_log("TSCR: Creature %u is using invalid SpellSchoolMask(%u) defined in event %u.", temp.creature_id, temp.event_param2, i);

                        if (temp.event_param4 < temp.event_param3)
                            error_db_log("TSCR: Creature %u are using repeatable event(%u) with param4 < param3 (RepeatMax < RepeatMin). Event will never repeat.", temp.creature_id, i);
                    }
                    break;

                case EVENT_T_RANGE:
                case EVENT_T_OOC_LOS:
                case EVENT_T_FRIENDLY_HP:
                case EVENT_T_FRIENDLY_IS_CC:
                case EVENT_T_FRIENDLY_MISSING_BUFF:
                    {
                        if (temp.event_param4 < temp.event_param3)
                            error_db_log("TSCR: Creature %u are using repeatable event(%u) with param4 < param3 (RepeatMax < RepeatMin). Event will never repeat.", temp.creature_id, i);
                    }
                    break;

                case EVENT_T_TIMER:
                case EVENT_T_TIMER_OOC:
                    {
                        if (temp.event_param2 < temp.event_param1)
                            error_db_log("TSCR: Creature %u are using timed event(%u) with param2 < param1 (InitialMax < InitialMin). Event will never repeat.", temp.creature_id, i);

                        if (temp.event_param4 < temp.event_param3)
                            error_db_log("TSCR: Creature %u are using repeatable event(%u) with param4 < param3 (RepeatMax < RepeatMin). Event will never repeat.", temp.creature_id, i);
                    }
                    break;

                case EVENT_T_KILL:
                case EVENT_T_TARGET_CASTING:
                    {
                        if (temp.event_param2 < temp.event_param1)
                            error_db_log("TSCR: Creature %u are using event(%u) with param2 < param1 (RepeatMax < RepeatMin). Event will never repeat.", temp.creature_id, i);
                    }
                    break;

                case EVENT_T_AGGRO:
                case EVENT_T_DEATH:
                case EVENT_T_RESET:
                case EVENT_T_SPAWNED:
                case EVENT_T_REACHED_HOME:
                    {
                        if (temp.event_flags & EFLAG_REPEATABLE)
                        {
                            error_db_log("TSCR: Creature %u has EFLAG_REPEATABLE set. Event can never be repeatable. Removing flag for event %u.", temp.creature_id, i);
                            temp.event_flags &= ~EFLAG_REPEATABLE;
                        }
                    }
                    break;
            }

            for (uint32 j = 0; j < MAX_ACTIONS; j++)
            {
                temp.action[j].type = fields[10+(j*4)].GetUInt16();
                temp.action[j].param1 = fields[11+(j*4)].GetUInt32();
                temp.action[j].param2 = fields[12+(j*4)].GetUInt32();
                temp.action[j].param3 = fields[13+(j*4)].GetUInt32();

                //Report any errors in actions
                switch (temp.action[j].type)
                {
                    case ACTION_T_TEXT:
                        {
                            if (temp.action[j].param1_s < 0)
                            {
                                if (TextMap.find(temp.action[j].param1_s) == TextMap.end())
                                    error_db_log("TSCR: Event %u Action %u param1 refrences non-existing entry in texts table.", i, j+1);
                            }
                            if (temp.action[j].param2_s < 0)
                            {
                                if (TextMap.find(temp.action[j].param2_s) == TextMap.end())
                                    error_db_log("TSCR: Event %u Action %u param2 refrences non-existing entry in texts table.", i, j+1);

                                if (!temp.action[j].param1_s)
                                    error_db_log("TSCR: Event %u Action %u has param2, but param1 is not set. Required for randomized text.", i, j+1);
                            }
                            if (temp.action[j].param3_s < 0)
                            {
                                if (TextMap.find(temp.action[j].param3_s) == TextMap.end())
                                    error_db_log("TSCR: Event %u Action %u param3 refrences non-existing entry in texts table.", i, j+1);

                                if (!temp.action[j].param1_s || !temp.action[j].param2_s)
                                    error_db_log("TSCR: Event %u Action %u has param3, but param1 and/or param2 is not set. Required for randomized text.", i, j+1);
                            }
                        }
                        break;
                    case ACTION_T_SET_FACTION:
                        /*if (temp.action[j].param1 !=0 && !GetFactionStore()->LookupEntry(temp.action[j].param1))
                        {
                            error_db_log("TSCR: Event %u Action %u uses non-existant FactionId %u.", i, j+1, temp.action[j].param1);
                            temp.action[j].param1 = 0;
                        }*/
                        break;
                    case ACTION_T_MORPH_TO_ENTRY_OR_MODEL:
                        if (temp.action[j].param1 !=0 || temp.action[j].param2 !=0)
                        {
                            if (temp.action[j].param1 && !GetCreatureTemplateStore(temp.action[j].param1))
                            {
                                error_db_log("TSCR: Event %u Action %u uses non-existant Creature entry %u.", i, j+1, temp.action[j].param1);
                                temp.action[j].param1 = 0;
                            }

                            /*if (temp.action[j].param2 && !GetCreatureDisplayStore()->LookupEntry(temp.action[j].param2))
                            {
                                error_db_log("TSCR: Event %u Action %u uses non-existant ModelId %u.", i, j+1, temp.action[j].param2);
                                temp.action[j].param2 = 0;
                            }*/
                        }
                        break;
                    case ACTION_T_SOUND:
                        if (!GetSoundEntriesStore()->LookupEntry(temp.action[j].param1))
                            error_db_log("TSCR: Event %u Action %u uses non-existant SoundID %u.", i, j+1, temp.action[j].param1);
                        break;

                    /*case ACTION_T_RANDOM_SOUND:
                        {
                            if(!GetSoundEntriesStore()->LookupEntry(temp.action[j].param1))
                                error_db_log("TSCR: Event %u Action %u param1 uses non-existant SoundID %u.", i, j+1, temp.action[j].param1);
                            if(!GetSoundEntriesStore()->LookupEntry(temp.action[j].param2))
                                error_db_log("TSCR: Event %u Action %u param2 uses non-existant SoundID %u.", i, j+1, temp.action[j].param2);
                            if(!GetSoundEntriesStore()->LookupEntry(temp.action[j].param3))
                                error_db_log("TSCR: Event %u Action %u param3 uses non-existant SoundID %u.", i, j+1, temp.action[j].param3);
                        }
                        break;*/

                    case ACTION_T_CAST:
                        {
                            const SpellEntry *spell = spellmgr.LookupSpell(temp.action[j].param1);
                            if (!spell)
                                error_db_log("TSCR: Event %u Action %u uses non-existant SpellID %u.", i, j+1, temp.action[j].param1);
                            else
                            {
                                if (spell->RecoveryTime > 0 && temp.event_flags & EFLAG_REPEATABLE)
                                {
                                    //output as debug for now, also because there's no general rule all spells have RecoveryTime
                                    if (temp.event_param3 < spell->RecoveryTime)
                                        sLog.outError("Event %u Action %u uses SpellID %u but cooldown is longer (%u) than minumum defined in event param3 (%u).", i, j+1,temp.action[j].param1, spell->RecoveryTime, temp.event_param3);
                                }
                            }

                            if (temp.action[j].param2 >= TARGET_T_END)
                                error_db_log("TSCR: Event %u Action %u uses incorrect Target type", i, j+1);
                        }
                        break;

                    case ACTION_T_REMOVEAURASFROMSPELL:
                        {
                            if (!spellmgr.LookupSpell(temp.action[j].param2))
                                error_db_log("TSCR: Event %u Action %u uses non-existant SpellID %u.", i, j+1, temp.action[j].param2);

                            if (temp.action[j].param1 >= TARGET_T_END)
                                error_db_log("TSCR: Event %u Action %u uses incorrect Target type", i, j+1);
                        }
                        break;
                    case ACTION_T_QUEST_EVENT:
                        {
                            if (Quest const* qid = GetQuestTemplateStore(temp.action[j].param1))
                            {
                                if (!qid->HasFlag(QUEST_TRINITY_FLAGS_EXPLORATION_OR_EVENT))
                                    error_db_log("TSCR: Event %u Action %u. SpecialFlags for quest entry %u does not include |2, Action will not have any effect.", i, j+1, temp.action[j].param1);
                            }
                            else
                                error_db_log("TSCR: Event %u Action %u uses non-existant Quest entry %u.", i, j+1, temp.action[j].param1);

                            if (temp.action[j].param2 >= TARGET_T_END)
                                error_db_log("TSCR: Event %u Action %u uses incorrect Target type", i, j+1);
                        }
                        break;
                    case ACTION_T_QUEST_EVENT_ALL:
                        {
                            if (Quest const* qid = GetQuestTemplateStore(temp.action[j].param1))
                            {
                                if (!qid->HasFlag(QUEST_TRINITY_FLAGS_EXPLORATION_OR_EVENT))
                                    error_db_log("TSCR: Event %u Action %u. SpecialFlags for quest entry %u does not include |2, Action will not have any effect.", i, j+1, temp.action[j].param1);
                            }
                            else
                                error_db_log("TSCR: Event %u Action %u uses non-existant Quest entry %u.", i, j+1, temp.action[j].param1);
                        }
                        break;
                    case ACTION_T_CASTCREATUREGO:
                        {
                            if (!GetCreatureTemplateStore(temp.action[j].param1))
                                error_db_log("TSCR: Event %u Action %u uses non-existant creature entry %u.", i, j+1, temp.action[j].param1);

                            if (!spellmgr.LookupSpell(temp.action[j].param2))
                                error_db_log("TSCR: Event %u Action %u uses non-existant SpellID %u.", i, j+1, temp.action[j].param2);

                            if (temp.action[j].param3 >= TARGET_T_END)
                                error_db_log("TSCR: Event %u Action %u uses incorrect Target type", i, j+1);
                        }
                        break;
                    case ACTION_T_CASTCREATUREGO_ALL:
                        {
                            if (!GetQuestTemplateStore(temp.action[j].param1))
                                error_db_log("TSCR: Event %u Action %u uses non-existant Quest entry %u.", i, j+1, temp.action[j].param1);

                            if (!spellmgr.LookupSpell(temp.action[j].param2))
                                error_db_log("TSCR: Event %u Action %u uses non-existant SpellID %u.", i, j+1, temp.action[j].param2);
                        }
                        break;

                    //2nd param target
                    case ACTION_T_SUMMON_ID:
                        {
                            if (!GetCreatureTemplateStore(temp.action[j].param1))
                                error_db_log("TSCR: Event %u Action %u uses non-existant creature entry %u.", i, j+1, temp.action[j].param1);

                            if (EventAI_Summon_Map.find(temp.action[j].param3) == EventAI_Summon_Map.end())
                                error_db_log("TSCR: Event %u Action %u summons missing EventAI_Summon %u", i, j+1, temp.action[j].param3);

                            if (temp.action[j].param2 >= TARGET_T_END)
                                error_db_log("TSCR: Event %u Action %u uses incorrect Target type", i, j+1);
                        }
                        break;
                    case ACTION_T_KILLED_MONSTER:
                        {
                            if (!GetCreatureTemplateStore(temp.action[j].param1))
                                error_db_log("TSCR: Event %u Action %u uses non-existant creature entry %u.", i, j+1, temp.action[j].param1);

                            if (temp.action[j].param2 >= TARGET_T_END)
                                error_db_log("TSCR: Event %u Action %u uses incorrect Target type", i, j+1);
                        }
                        break;
                    case ACTION_T_SUMMON:
                        {
                            if (!GetCreatureTemplateStore(temp.action[j].param1))
                                error_db_log("TSCR: Event %u Action %u uses non-existant creature entry %u.", i, j+1, temp.action[j].param1);

                            if (temp.action[j].param2 >= TARGET_T_END)
                                error_db_log("TSCR: Event %u Action %u uses incorrect Target type", i, j+1);
                        }
                        break;
                    case ACTION_T_THREAT_SINGLE_PCT:
                    case ACTION_T_SET_UNIT_FLAG:
                    case ACTION_T_REMOVE_UNIT_FLAG:
                        if (temp.action[j].param2 >= TARGET_T_END)
                            error_db_log("TSCR: Event %u Action %u uses incorrect Target type", i, j+1);
                        break;

                    //3rd param target
                    case ACTION_T_SET_UNIT_FIELD:
                        if (temp.action[j].param1 < OBJECT_END || temp.action[j].param1 >= UNIT_END)
                            error_db_log("TSCR: Event %u Action %u param1 (UNIT_FIELD*). Index out of range for intended use.", i, j+1);
                        if (temp.action[j].param3 >= TARGET_T_END)
                            error_db_log("TSCR: Event %u Action %u uses incorrect Target type", i, j+1);
                        break;

                    case ACTION_T_SET_PHASE:
                        if (temp.action[j].param1 > 31)
                            error_db_log("TSCR: Event %u Action %u attempts to set phase > 31. Phase mask cannot be used past phase 31.", i, j+1);
                        break;

                    case ACTION_T_INC_PHASE:
                        if (!temp.action[j].param1)
                            error_db_log("TSCR: Event %u Action %u is incrementing phase by 0. Was this intended?", i, j+1);
                        break;

                    case ACTION_T_SET_INST_DATA:
                        {
                            if (!(temp.event_flags & EFLAG_NORMAL) && !(temp.event_flags & EFLAG_HEROIC))
                                error_db_log("TSCR: Event %u Action %u. Cannot set instance data without event flags (normal/heroic).", i, j+1);

                            if (temp.action[j].param2 > SPECIAL)
                                error_db_log("TSCR: Event %u Action %u attempts to set instance data above encounter state 4. Custom case?", i, j+1);
                        }
                        break;
                    case ACTION_T_SET_INST_DATA64:
                        {
                            if (!(temp.event_flags & EFLAG_NORMAL) && !(temp.event_flags & EFLAG_HEROIC))
                                error_db_log("TSCR: Event %u Action %u. Cannot set instance data without event flags (normal/heroic).", i, j+1);

                            if (temp.action[j].param2 >= TARGET_T_END)
                                error_db_log("TSCR: Event %u Action %u uses incorrect Target type", i, j+1);
                        }
                        break;
                    case ACTION_T_UPDATE_TEMPLATE:
                        {
                            if (!GetCreatureTemplateStore(temp.action[j].param1))
                                error_db_log("TSCR: Event %u Action %u uses non-existant creature entry %u.", i, j+1, temp.action[j].param1);
                        }
                        break;
                    case ACTION_T_RANDOM_SAY:
                    case ACTION_T_RANDOM_YELL:
                    case ACTION_T_RANDOM_TEXTEMOTE:
                        error_db_log("TSCR: Event %u Action %u currently unused ACTION type. Did you forget to update database?", i, j+1);
                        break;

                    default:
                        if (temp.action[j].type >= ACTION_T_END)
                            error_db_log("TSCR: Event %u Action %u has incorrect action type. Maybe DB requires updated version of SD2.", i, j+1);
                        break;
                }
            }

            //Add to list
            EventAI_Event_Map[creature_id].push_back(temp);

            ++Count;
        } while (result->NextRow());

        delete result;

        sLog.outString("\n>> Loaded %u EventAI scripts", Count);
    }else
    {
        sLog.outString("\n>> Loaded 0 EventAI scripts. DB table `eventai_scripts` is empty.");
    }

    //Free database thread and resources
    TScriptDB.Close();

}

struct TSpellSummary {
    uint8 Targets;                                          // set of enum SelectTarget
    uint8 Effects;                                          // set of enum SelectEffect
}extern *SpellSummary;


void ScriptsFree()
{
    // Free Spell Summary
    delete []SpellSummary;

    // Free resources before library unload
    for(int i=0;i<num_sc_scripts;i++)
        delete m_scripts[i];

    num_sc_scripts = 0;
}

ScriptMgr::ScriptMgr()
{
    
}

ScriptMgr::~ScriptMgr()
{
    
}

void ScriptMgr::ScriptsInit(char const* cfg_file)
{
    bool CanLoadDB = true;

    //Trinity Script startup
    sLog.outString(" _____     _       _ _         ____            _       _");
    sLog.outString("|_   _| __(_)_ __ (_) |_ _   _/ ___|  ___ _ __(_)_ __ | |_ ");
    sLog.outString("  | || '__| | '_ \\| | __| | | \\___ \\ / __| \'__| | \'_ \\| __|");
    sLog.outString("  | || |  | | | | | | |_| |_| |___) | (__| |  | | |_) | |_ ");
    sLog.outString("  |_||_|  |_|_| |_|_|\\__|\\__, |____/ \\___|_|  |_| .__/ \\__|");
    sLog.outString("                         |___/                  |_|        ");
    sLog.outString("Trinity Script initializing %s\n", _FULLVERSION);

    //Get configuration file
    if (!TScriptConfig.SetSource(cfg_file))
    {
        CanLoadDB = false;
        error_log("TSCR: Unable to open configuration file. Database will be unaccessible. Configuration values will use default.");
    }
    else sLog.outString("TSCR: Using configuration file %s",cfg_file);

    EAI_ErrorLevel = TScriptConfig.GetIntDefault("EAIErrorLevel", 1);

    switch (EAI_ErrorLevel)
    {
        case 0:
            sLog.outString("TSCR: EventAI Error Reporting level set to 0 (Startup Errors only)");
            break;
        case 1:
            sLog.outString("TSCR: EventAI Error Reporting level set to 1 (Startup errors and Runtime event errors)");
            break;
        case 2:
            sLog.outString("TSCR: EventAI Error Reporting level set to 2 (Startup errors, Runtime event errors, and Creation errors)");
            break;
        default:
            sLog.outString("TSCR: Unknown EventAI Error Reporting level. Defaulting to 1 (Startup errors and Runtime event errors)");
            EAI_ErrorLevel = 1;
            break;
    }

    sLog.outString("");

    //Load database (must be called after TScriptConfig.SetSource). In case it failed, no need to even try load.
    if (CanLoadDB)
        LoadDatabase();

    sLog.outString("TSCR: Loading C++ scripts\n");

    for(int i=0;i<MAX_SCRIPTS;i++)
        m_scripts[i]=NULL;

    FillSpellSummary();

    // -- Scripts to be added --

    // -- Areatrigger --
    AddSC_areatrigger_scripts();

    // -- Outdoors Dragons --
    AddSC_boss_dragonsofnightmare();

    // -- Creature --
    AddSC_mob_event();
    AddSC_generic_creature();

    // -- Custom --
    AddSC_npc_rez();
    AddSC_training_dummy();
    AddSC_zone_silence();
    AddSC_custom_example();
    AddSC_custom_gossip_codebox();
    AddSC_test();
    AddSC_onevents();
    AddSC_npc_lottery();
	AddSC_theinform();
    AddSC_mylittlebombling();
    AddSC_firework_controller();
    AddSC_npc_interpreter();
    AddSC_custom_gnominizer();

    AddSC_npc_teleporter();
    AddSC_npc_teleporter_pvpzone();
    AddSC_catapultmaster();

    // -- GO --
    AddSC_go_scripts();

    // -- Guard --
    AddSC_guards();

    // -- Honor --

    // -- Item --
    AddSC_item_scripts();
    AddSC_item_test();

    // -- NPC --
    AddSC_npc_professions();
    AddSC_npcs_special();
    AddSC_npc_xp_blocker();

    // -- Servers --
    AddSC_SmartSCripts();
    
    // -- Misc --
    AddSC_arena_spectator_script();
    AddSC_hallows_end();
    AddSC_arenabeastmaster();

    //--------------------
    //------ ZONE --------

    //Alterac Mountains
    AddSC_alterac_mountains();
    
    //Alterac Valley
    AddSC_AV_Marshals();
    AddSC_AV_Warmasters();
    AddSC_boss_balinda();
    AddSC_boss_drekthar();
    AddSC_boss_galvangar();
    AddSC_boss_vanndar();
    AddSC_alterac_bowman();

    //Arathi Highlands
    AddSC_arathi_highlands();

    //Ashenvale Forest
    AddSC_ashenvale();

    //Aunchindoun
    //--Auchenai Crypts
    AddSC_boss_exarch_maladaar();
    AddSC_boss_shirrak_the_dead_watcher();

    //--Mana Tombs
    AddSC_boss_nexusprince_shaffar();
    AddSC_boss_pandemonius();

    //--Sekketh Halls
    AddSC_boss_darkweaver_syth();
    AddSC_boss_talon_king_ikiss();
    AddSC_boss_anzu();
    AddSC_instance_sethekk_halls();
    AddSC_sethekk_halls();

    //--Shadow Labyrinth
    AddSC_boss_ambassador_hellmaw();
    AddSC_boss_blackheart_the_inciter();
    AddSC_boss_grandmaster_vorpil();
    AddSC_boss_murmur();
    AddSC_instance_shadow_labyrinth();

    //Azshara
    AddSC_boss_azuregos();
    AddSC_azshara();

    //Azuremyst Isle
    AddSC_azuremyst_isle();

    //Badlands
    //Barrens
    AddSC_the_barrens();

    //Black Temple
    AddSC_black_temple();
    AddSC_boss_illidan();
    AddSC_boss_shade_of_akama();
    AddSC_boss_supremus();
    AddSC_boss_gurtogg_bloodboil();
    AddSC_boss_mother_shahraz();
    AddSC_boss_reliquary_of_souls();
    AddSC_boss_teron_gorefiend();
    AddSC_boss_najentus();
    AddSC_boss_illidari_council();
    AddSC_instance_black_temple();

    //Blackfathom Depths
    AddSC_blackfathom_deeps();
    AddSC_boss_gelihast();
    AddSC_boss_kelris();
    AddSC_boss_aku_mai();
    AddSC_instance_blackfathom_deeps();

    //Blackrock Depths
    AddSC_blackrock_depths();
    AddSC_boss_ambassador_flamelash();
    AddSC_boss_anubshiah();
    AddSC_boss_draganthaurissan();
    AddSC_boss_general_angerforge();
    AddSC_boss_gorosh_the_dervish();
    AddSC_boss_grizzle();
    AddSC_boss_high_interrogator_gerstahn();
    AddSC_boss_magmus();
    AddSC_boss_moira_bronzebeard();
    AddSC_boss_tomb_of_seven();
    AddSC_instance_blackrock_depths();

    //Blackrock Spire
    AddSC_boss_drakkisath();
    AddSC_boss_halycon();
    AddSC_boss_highlordomokk();
    AddSC_boss_mothersmolderweb();
    AddSC_boss_overlordwyrmthalak();
    AddSC_boss_shadowvosh();
    AddSC_boss_thebeast();
    AddSC_boss_warmastervoone();
    AddSC_boss_quatermasterzigris();
    AddSC_boss_pyroguard_emberseer();
    AddSC_boss_gyth();
    AddSC_boss_rend_blackhand();
    AddSC_instance_blackrock_spire();

    //Blackwing lair
    AddSC_boss_razorgore();
    AddSC_boss_vael();
    AddSC_boss_broodlord();
    AddSC_boss_firemaw();
    AddSC_boss_ebonroc();
    AddSC_boss_flamegor();
    AddSC_boss_chromaggus();
    AddSC_boss_nefarian();
    AddSC_boss_victor_nefarius();

    //Blade's Edge Mountains
    AddSC_blades_edge_mountains();
    AddSC_shartuul_event();

    //Blasted lands
    AddSC_boss_kruul();
    AddSC_blasted_lands();

    //Bloodmyst Isle
    AddSC_bloodmyst_isle();

    //Burning steppes
    AddSC_burning_steppes();

    //Caverns of Time
    //--Battle for Mt. Hyjal
    AddSC_hyjal();
    AddSC_boss_archimonde();
    AddSC_boss_archimonde_new();
    AddSC_instance_mount_hyjal();
    AddSC_hyjal_trash();
    AddSC_boss_rage_winterchill();
    AddSC_boss_anetheron();
    AddSC_boss_kazrogal();
    AddSC_boss_azgalor();

    //--Old Hillsbrad
    AddSC_boss_captain_skarloc();
    AddSC_boss_epoch_hunter();
    AddSC_boss_lieutenant_drake();
    AddSC_instance_old_hillsbrad();
    AddSC_old_hillsbrad();

    //--The Dark Portal
    AddSC_boss_aeonus();
    AddSC_boss_chrono_lord_deja();
    AddSC_boss_temporus();
    AddSC_dark_portal();
    AddSC_instance_dark_portal();

    //Coilfang Resevoir
    //--Serpent Shrine Cavern
    AddSC_boss_fathomlord_karathress();
    AddSC_boss_hydross_the_unstable();
    AddSC_boss_lady_vashj();
    AddSC_boss_leotheras_the_blind();
    AddSC_boss_morogrim_tidewalker();
    AddSC_instance_serpentshrine_cavern();
    AddSC_boss_the_lurker_below();

    //--Slave Pens
    AddSC_boss_ahune();
    AddSC_instance_slave_pens();
    
    //--Steam Vault
    AddSC_boss_hydromancer_thespia();
    AddSC_boss_mekgineer_steamrigger();
    AddSC_boss_warlord_kalithresh();
    AddSC_instance_steam_vault();

    //--Underbog
    AddSC_boss_hungarfen();
    AddSC_boss_the_black_stalker();

    //Darkshore
    AddSC_darkshore();
    //Darnassus
    //Deadmines
    AddSC_instance_deadmines();

    //Deadwind pass
    //Desolace
    AddSC_desolace();
    
    //Dire Maul
    AddSC_instance_dire_maul();
    AddSC_dire_maul();
    AddSC_boss_pusillin();
    
    //Dun Morogh
    AddSC_dun_morogh();

    //Durotar
    AddSC_durotar();
    
    //Duskwood
    //Dustwallow marsh
    AddSC_dustwallow_marsh();

    //Eversong Woods
    AddSC_eversong_woods();

    //Exodar
    //Eastern Plaguelands
    AddSC_eastern_plaguelands();

    //Elwynn Forest
    AddSC_elwynn_forest();

    //Felwood
    AddSC_felwood();

    //Feralas
    AddSC_feralas();

    //Ghostlands
    AddSC_ghostlands();

    //Gnomeregan
    //Gruul's Lair
    AddSC_boss_gruul();
    AddSC_boss_high_king_maulgar();
    AddSC_instance_gruuls_lair();

    //Hellfire Citadel
    //--Blood Furnace
    AddSC_boss_broggok();
    AddSC_boss_kelidan_the_breaker();
    AddSC_boss_the_maker();
    AddSC_instance_blood_furnace();

    //--Magtheridon's Lair
    AddSC_boss_magtheridon();
    AddSC_instance_magtheridons_lair();

    //--Shattered Halls
    AddSC_boss_grand_warlock_nethekurse();
    AddSC_boss_warbringer_omrogg();
    AddSC_boss_warchief_kargath_bladefist();
    AddSC_instance_shattered_halls();

    //--Ramparts
    AddSC_boss_watchkeeper_gargolmar();
    AddSC_boss_omor_the_unscarred();
    AddSC_boss_vazruden_the_herald();

    //Hellfire Peninsula
    AddSC_boss_doomlordkazzak();
    AddSC_hellfire_peninsula();

    //Hillsbrad Foothills
    //Hinterlands
    AddSC_hinterlands();
    
    //Ironforge
    AddSC_ironforge();

    //Isle of Quel'Danas
    AddSC_isle_of_queldanas();

    //Karazhan
    AddSC_boss_attumen();
    AddSC_boss_curator();
    AddSC_boss_maiden_of_virtue();
    AddSC_boss_shade_of_aran();
    AddSC_boss_malchezaar();
    AddSC_boss_terestian_illhoof();
    AddSC_boss_moroes();
    AddSC_bosses_opera();
    AddSC_boss_netherspite();
    AddSC_instance_karazhan();
    AddSC_karazhan();
    AddSC_boss_nightbane();
    AddSC_chess_event();

    //Loch Modan
    AddSC_loch_modan();

    //Lower Blackrock Spire

    // Magister's Terrace
    AddSC_boss_felblood_kaelthas();
    AddSC_boss_selin_fireheart();
    AddSC_boss_vexallus();
    AddSC_boss_priestess_delrissa();
    AddSC_instance_magisters_terrace();

    //Maraudon
    AddSC_boss_celebras_the_cursed();
    AddSC_boss_landslide();
    AddSC_boss_noxxion();
    AddSC_boss_ptheradras();

    //Molten core
    AddSC_boss_lucifron();
    AddSC_boss_magmadar();
    AddSC_boss_gehennas();
    AddSC_boss_garr();
    AddSC_boss_baron_geddon();
    AddSC_boss_shazzrah();
    AddSC_boss_golemagg();
    AddSC_boss_sulfuron();
    AddSC_boss_majordomo();
    AddSC_boss_ragnaros();
    AddSC_instance_molten_core();
    AddSC_molten_core();

    //Moonglade
    AddSC_moonglade();

    //Mulgore
    AddSC_mulgore();

    //Nagrand
    AddSC_nagrand();

    //Naxxramas
    AddSC_boss_anubrekhan();
    AddSC_boss_maexxna();
    AddSC_boss_patchwerk();
    AddSC_boss_razuvious();
    AddSC_boss_highlord_mograine();
    AddSC_boss_kelthuzad();
    AddSC_boss_faerlina();
    AddSC_boss_loatheb();
    AddSC_boss_noth();
    AddSC_boss_gluth();
    AddSC_boss_sapphiron();
    AddSC_boss_four_horsemen();

    //Netherstorm
    AddSC_netherstorm();

    //Onyxia's Lair
    AddSC_boss_onyxia();

    //Orgrimmar
    AddSC_orgrimmar();

    //Ragefire Chasm
    //Razorfen Downs
    AddSC_boss_tuten_kash();
    AddSC_boss_plaguemaw_the_rotting();
    AddSC_boss_ragglesnout();
    AddSC_boss_glutton();
    AddSC_boss_mordresh_fire_eye();
    AddSC_instance_razorfen_downs();
    AddSC_boss_amnennar_the_coldbringer();
    AddSC_razorfen_downs();

    //Razorfen Kraul
    AddSC_razorfen_kraul();
    AddSC_instance_razorfen_kraul();

    //Redridge Mountains
    AddSC_redridge_mountains();
    
    //Ruins of Ahn'Qiraj
    AddSC_boss_kurinnaxx();
    AddSC_boss_rajaxx();
    AddSC_boss_moam();
    AddSC_boss_buru();
    AddSC_boss_ayamiss();
    AddSC_boss_ossirian();
    AddSC_instance_ruins_of_ahnqiraj();
    //Scarlet Monastery
    AddSC_boss_arcanist_doan();
    AddSC_boss_azshir_the_sleepless();
    AddSC_boss_bloodmage_thalnos();
    AddSC_boss_headless_horseman();
    AddSC_boss_herod();
    AddSC_boss_high_inquisitor_fairbanks();
    AddSC_boss_houndmaster_loksey();
    AddSC_boss_interrogator_vishas();
    AddSC_boss_scorn();
    AddSC_instance_scarlet_monastery();
    AddSC_boss_mograine_and_whitemane();

    //Scholomance
    AddSC_boss_darkmaster_gandling();
    AddSC_boss_death_knight_darkreaver();
    AddSC_boss_theolenkrastinov();
    AddSC_boss_illuciabarov();
    AddSC_boss_instructormalicia();
    AddSC_boss_jandicebarov();
    AddSC_boss_kormok();
    AddSC_boss_lordalexeibarov();
    AddSC_boss_lorekeeperpolkelt();
    AddSC_boss_rasfrost();
    AddSC_boss_theravenian();
    AddSC_boss_vectus();
    AddSC_instance_scholomance();
    AddSC_scholomance();

    //Searing gorge
    AddSC_searing_gorge();

    //Shadowfang keep
    AddSC_shadowfang_keep();
    AddSC_instance_shadowfang_keep();

    //Shadowmoon Valley
    AddSC_boss_doomwalker();
    AddSC_shadowmoon_valley();

    //Shattrath
    AddSC_shattrath_city();

    //Silithus
    AddSC_silithus();

    //Silvermoon
    AddSC_silvermoon_city();

    //Silverpine forest
    AddSC_silverpine_forest();

    //Stockade
    //Stonetalon mountains
    AddSC_stonetalon_mountains();

    //Stormwind City
    AddSC_stormwind_city();

    //Stranglethorn Vale
    AddSC_stranglethorn_vale();

    //Stratholme
    AddSC_boss_magistrate_barthilas();
    AddSC_boss_maleki_the_pallid();
    AddSC_boss_nerubenkan();
    AddSC_boss_cannon_master_willey();
    AddSC_boss_baroness_anastari();
    AddSC_boss_ramstein_the_gorger();
    AddSC_boss_timmy_the_cruel();
    AddSC_boss_postmaster_malown();
    AddSC_boss_baron_rivendare();
    AddSC_boss_dathrohan_balnazzar();
    AddSC_boss_order_of_silver_hand();
    AddSC_instance_stratholme();
    AddSC_stratholme();

    //Sunken Temple
    AddSC_instance_sunken_temple();
    AddSC_sunken_temple();

    //Sunwell Plateau
    AddSC_instance_sunwell_plateau();
    AddSC_boss_kalecgos();
    AddSC_boss_brutallus();
    AddSC_boss_felmyst();
    AddSC_boss_eredar_twins();
    AddSC_boss_muru();
    AddSC_boss_kiljaeden_new();
    AddSC_sunwell_plateau();
    
    //Swamp of Sorrows
    AddSC_swamp_of_sorrows();

    //Tanaris
    AddSC_tanaris();

    //Teldrassil
    AddSC_teldrassil();
    
    //Tempest Keep
    //--Arcatraz
    AddSC_arcatraz();
    AddSC_boss_harbinger_skyriss();
    AddSC_instance_arcatraz();

    //--Botanica
    AddSC_boss_high_botanist_freywinn();
    AddSC_boss_laj();
    AddSC_boss_warp_splinter();

    //--The Eye
    AddSC_boss_alar();
    AddSC_boss_kaelthas();
    AddSC_boss_void_reaver();
    AddSC_boss_high_astromancer_solarian();
    AddSC_instance_the_eye();
    AddSC_the_eye();

    //--The Mechanar
    AddSC_boss_mechanolord_capacitus();
    AddSC_boss_gatewatcher_iron_hand();
    AddSC_boss_nethermancer_sepethrea();
    AddSC_boss_pathaleon_the_calculator();
    AddSC_instance_mechanar();

    //Temple of ahn'qiraj
    AddSC_boss_cthun();
    AddSC_boss_fankriss();
    AddSC_boss_huhuran();
    AddSC_bug_trio();
    AddSC_boss_sartura();
    AddSC_boss_skeram();
    AddSC_boss_twinemperors();
    AddSC_mob_anubisath_sentinel();
    AddSC_instance_temple_of_ahnqiraj();
    AddSC_boss_ouro();

    //Terokkar Forest
    AddSC_terokkar_forest();

    //Thousand Needles
    AddSC_thousand_needles();

    //Thunder Bluff
    AddSC_thunder_bluff();

    // The Balance of Light and Shadow - Epic priest quest
    AddSC_the_balance_of_light_and_shadow();
    
    //Tirisfal Glades
    AddSC_tirisfal_glades();

    //Uldaman
    AddSC_boss_archaedas();
    AddSC_boss_ironaya();
    AddSC_uldaman();
    AddSC_instance_uldaman();

    //Undercity
    AddSC_undercity();

    //Un'Goro Crater
    AddSC_ungoro_crater();

    //Upper blackrock spire
    //Wailing caverns
    AddSC_instance_wailing_caverns();
    AddSC_wailing_caverns();

    //Western plaguelands
    AddSC_western_plaguelands();

    //Westfall
    AddSC_westfall();

    //Wetlands
    AddSC_wetlands();
    
    //Winterspring
    AddSC_winterspring();

    //Zangarmarsh
    AddSC_zangarmarsh();

    //Zul'Farrak
    AddSC_zulfarrak();
    AddSC_instance_zulfarrak();

    //Zul'Gurub
    AddSC_boss_jeklik();
    AddSC_boss_venoxis();
    AddSC_boss_marli();
    AddSC_boss_mandokir();
    AddSC_boss_gahzranka();
    AddSC_boss_thekal();
    AddSC_boss_arlokk();
    AddSC_boss_jindo();
    AddSC_boss_hakkar();
    AddSC_boss_grilek();
    AddSC_boss_hazzarah();
    AddSC_boss_renataki();
    AddSC_boss_wushoolay();
    AddSC_instance_zulgurub();

    //Zul'Aman
    AddSC_boss_akilzon();
    AddSC_boss_halazzi();
    AddSC_boss_hex_lord_malacrass();
    AddSC_boss_janalai();
    AddSC_boss_nalorakk();
    AddSC_boss_zuljin();
    AddSC_instance_zulaman();
    AddSC_zulaman();

    // -------------------

    sLog.outString(">> Loaded %i C++ Scripts.", num_sc_scripts);

    sLog.outString(">> Load Overriden SQL Data.");
    LoadOverridenSQLData();
    sLog.outString(">> Load Overriden DBC Data.");
    LoadOverridenDBCData();
}

//*********************************
//*** Functions used globally ***

void DoScriptText(int32 textEntry, WorldObject* pSource, Unit* target)
{
    if (!pSource)
    {
        error_log("TSCR: DoScriptText entry %i, invalid Source pointer.",textEntry);
        return;
    }

    if (textEntry >= 0)
    {
        error_log("TSCR: DoScriptText with source entry %u (TypeId=%u, guid=%u) attempts to process text entry %i, but text entry must be negative.",pSource->GetEntry(),pSource->GetTypeId(),pSource->GetGUIDLow(),textEntry);
        return;
    }

    UNORDERED_MAP<int32, StringTextData>::iterator i = TextMap.find(textEntry);

    if (i == TextMap.end())
    {
        error_log("TSCR: DoScriptText with source entry %u (TypeId=%u, guid=%u) could not find text entry %i.",pSource->GetEntry(),pSource->GetTypeId(),pSource->GetGUIDLow(),textEntry);
        return;
    }

    if((*i).second.SoundId)
    {
        if( GetSoundEntriesStore()->LookupEntry((*i).second.SoundId) )
        {
            pSource->SendPlaySound((*i).second.SoundId, false);
        }
        else
            error_log("TSCR: DoScriptText entry %i tried to process invalid sound id %u.",textEntry,(*i).second.SoundId);
    }

    if((*i).second.Emote)
    {
        if (pSource->GetTypeId() == TYPEID_UNIT || pSource->GetTypeId() == TYPEID_PLAYER)
        {
            ((Unit*)pSource)->HandleEmoteCommand((*i).second.Emote);
        }
        else
            error_log("TSCR: DoScriptText entry %i tried to process emote for invalid TypeId (%u).",textEntry,pSource->GetTypeId());
    }

    switch((*i).second.Type)
    {
        case CHAT_TYPE_SAY:
            pSource->MonsterSay(textEntry, (*i).second.Language, target ? target->GetGUID() : 0);
            break;
        case CHAT_TYPE_YELL:
            pSource->MonsterYell(textEntry, (*i).second.Language, target ? target->GetGUID() : 0);
            break;
        case CHAT_TYPE_TEXT_EMOTE:
            pSource->MonsterTextEmote(textEntry, target ? target->GetGUID() : 0);
            break;
        case CHAT_TYPE_BOSS_EMOTE:
            pSource->MonsterTextEmote(textEntry, target ? target->GetGUID() : 0, true);
            break;
        case CHAT_TYPE_WHISPER:
            {
                if (target && target->GetTypeId() == TYPEID_PLAYER)
                    pSource->MonsterWhisper(textEntry, target->GetGUID());
                else error_log("TSCR: DoScriptText entry %i cannot whisper without target unit (TYPEID_PLAYER).", textEntry);
            }break;
        case CHAT_TYPE_BOSS_WHISPER:
            {
                if (target && target->GetTypeId() == TYPEID_PLAYER)
                    pSource->MonsterWhisper(textEntry, target->GetGUID(), true);
                else error_log("TSCR: DoScriptText entry %i cannot whisper without target unit (TYPEID_PLAYER).", textEntry);
            }break;
    }
}

Creature* SelectCreatureInGrid(Creature* origin, uint32 entry, float range)
{
    Creature* pCreature = NULL;

    CellPair pair(Trinity::ComputeCellPair(origin->GetPositionX(), origin->GetPositionY()));
    Cell cell(pair);
    cell.data.Part.reserved = ALL_DISTRICT;
    cell.SetNoCreate();

    Trinity::NearestCreatureEntryWithLiveStateInObjectRangeCheck creature_check(*origin, entry, true, range);
    Trinity::CreatureLastSearcher<Trinity::NearestCreatureEntryWithLiveStateInObjectRangeCheck> searcher(pCreature, creature_check);

    TypeContainerVisitor<Trinity::CreatureLastSearcher<Trinity::NearestCreatureEntryWithLiveStateInObjectRangeCheck>, GridTypeMapContainer> creature_searcher(searcher);

    cell.Visit(pair, creature_searcher, *(origin->GetMap()));

    return pCreature;
}

//*********************************
//*** Functions used internally ***

void Script::RegisterSelf()
{
    int id = GetScriptId(Name.c_str());
    if(id)
    {
        m_scripts[id] = this;
        ++num_sc_scripts;
    }
    else
        sLog.outError("CRASH ALERT! TrinityScript: RegisterSelf, but script named %s does not have ScriptName assigned in database.",(this)->Name.c_str());
}

//********************************
//*** Functions to be Exported ***


void ScriptMgr::OnLogin(Player *pPlayer)
{
    Script *tmpscript = m_scripts[GetScriptId("scripted_on_events")];
    if (!tmpscript || !tmpscript->pOnLogin) return;
    tmpscript->pOnLogin(pPlayer);
}


void ScriptMgr::OnLogout(Player *pPlayer)
{
    Script *tmpscript = m_scripts[GetScriptId("scripted_on_events")];
    if (!tmpscript || !tmpscript->pOnLogout) return;
    tmpscript->pOnLogout(pPlayer);
}


void ScriptMgr::OnPVPKill(Player *killer, Player *killed)
{
    Script *tmpscript = m_scripts[GetScriptId("scripted_on_events")];
    if (!tmpscript || !tmpscript->pOnPVPKill) return;
    tmpscript->pOnPVPKill(killer, killed);
}


char const* ScriptMgr::ScriptsVersion()
{
    return "Default Trinity scripting library";
}

bool ScriptMgr::GossipHello ( Player * player, Creature *_Creature )
{
    Script *tmpscript = m_scripts[_Creature->GetScriptId()];
    if (!tmpscript || !tmpscript->pGossipHello) return false;

    player->PlayerTalkClass->ClearMenus();
    return tmpscript->pGossipHello(player,_Creature);
}


bool ScriptMgr::GossipSelect( Player *player, Creature *_Creature, uint32 sender, uint32 action )
{
    Script *tmpscript = m_scripts[_Creature->GetScriptId()];
    if (!tmpscript || !tmpscript->pGossipSelect) return false;

    player->PlayerTalkClass->ClearMenus();
    return tmpscript->pGossipSelect(player,_Creature,sender,action);
}


bool ScriptMgr::GossipSelectWithCode( Player *player, Creature *_Creature, uint32 sender, uint32 action, const char* sCode )
{
    Script *tmpscript = m_scripts[_Creature->GetScriptId()];
    if (!tmpscript || !tmpscript->pGossipSelectWithCode) return false;

    player->PlayerTalkClass->ClearMenus();
    return tmpscript->pGossipSelectWithCode(player,_Creature,sender,action,sCode);
}


bool ScriptMgr::GOSelect( Player *player, GameObject *_GO, uint32 sender, uint32 action )
{
    if(!_GO)
    return false;

    Script *tmpscript = m_scripts[_GO->GetGOInfo()->ScriptId];
    if(!tmpscript || !tmpscript->pGOSelect) return false;

    player->PlayerTalkClass->ClearMenus();
    return tmpscript->pGOSelect(player,_GO,sender,action);
}


bool ScriptMgr::GOSelectWithCode( Player *player, GameObject *_GO, uint32 sender, uint32 action, const char* sCode )
{
    if(!_GO)
    return false;

    Script *tmpscript = m_scripts[_GO->GetGOInfo()->ScriptId];
    if(!tmpscript || !tmpscript->pGOSelectWithCode) return false;

    player->PlayerTalkClass->ClearMenus();
    return tmpscript->pGOSelectWithCode(player,_GO,sender,action,sCode);
}


bool ScriptMgr::QuestAccept( Player *player, Creature *_Creature, Quest const *_Quest )
{
    Script *tmpscript = m_scripts[_Creature->GetScriptId()];
    if (!tmpscript || !tmpscript->pQuestAccept) return false;

    player->PlayerTalkClass->ClearMenus();
    return tmpscript->pQuestAccept(player,_Creature,_Quest);
}


bool ScriptMgr::QuestSelect( Player *player, Creature *_Creature, Quest const *_Quest )
{
    Script *tmpscript = m_scripts[_Creature->GetScriptId()];
    if (!tmpscript || !tmpscript->pQuestSelect) return false;

    player->PlayerTalkClass->ClearMenus();
    return tmpscript->pQuestSelect(player,_Creature,_Quest);
}


bool ScriptMgr::QuestComplete( Player *player, Creature *_Creature, Quest const *_Quest )
{
    Script *tmpscript = m_scripts[_Creature->GetScriptId()];
    if (!tmpscript || !tmpscript->pQuestComplete) return false;

    player->PlayerTalkClass->ClearMenus();
    return tmpscript->pQuestComplete(player,_Creature,_Quest);
}


bool ScriptMgr::ChooseReward( Player *player, Creature *_Creature, Quest const *_Quest, uint32 opt )
{
    Script *tmpscript = m_scripts[_Creature->GetScriptId()];
    if (!tmpscript || !tmpscript->pChooseReward) return false;

    player->PlayerTalkClass->ClearMenus();
    return tmpscript->pChooseReward(player,_Creature,_Quest,opt);
}


uint32 ScriptMgr::NPCDialogStatus( Player *player, Creature *_Creature )
{
    Script *tmpscript = m_scripts[_Creature->GetScriptId()];
    if (!tmpscript || !tmpscript->pNPCDialogStatus) return 100;

    player->PlayerTalkClass->ClearMenus();
    return tmpscript->pNPCDialogStatus(player,_Creature);
}


uint32 ScriptMgr::GODialogStatus( Player *player, GameObject *_GO )
{
    Script *tmpscript = m_scripts[_GO->GetGOInfo()->ScriptId];
    if (!tmpscript || !tmpscript->pGODialogStatus) return 100;

    player->PlayerTalkClass->ClearMenus();
    return tmpscript->pGODialogStatus(player,_GO);
}


bool ScriptMgr::ItemHello( Player *player, Item *_Item, Quest const *_Quest )
{
    Script *tmpscript = m_scripts[_Item->GetProto()->ScriptId];
    if (!tmpscript || !tmpscript->pItemHello) return false;

    player->PlayerTalkClass->ClearMenus();
    return tmpscript->pItemHello(player,_Item,_Quest);
}


bool ScriptMgr::ItemQuestAccept( Player *player, Item *_Item, Quest const *_Quest )
{
    Script *tmpscript = m_scripts[_Item->GetProto()->ScriptId];
    if (!tmpscript || !tmpscript->pItemQuestAccept) return false;

    player->PlayerTalkClass->ClearMenus();
    return tmpscript->pItemQuestAccept(player,_Item,_Quest);
}


bool ScriptMgr::GOHello( Player *player, GameObject *_GO )
{
    Script *tmpscript = m_scripts[_GO->GetGOInfo()->ScriptId];
    if (!tmpscript || !tmpscript->pGOHello) return false;

    player->PlayerTalkClass->ClearMenus();
    return tmpscript->pGOHello(player,_GO);
}


bool ScriptMgr::GOQuestAccept( Player *player, GameObject *_GO, Quest const *_Quest )
{
    Script *tmpscript = m_scripts[_GO->GetGOInfo()->ScriptId];
    if (!tmpscript || !tmpscript->pGOQuestAccept) return false;

    player->PlayerTalkClass->ClearMenus();
    return tmpscript->pGOQuestAccept(player,_GO,_Quest);
}


bool ScriptMgr::GOChooseReward( Player *player, GameObject *_GO, Quest const *_Quest, uint32 opt )
{
    Script *tmpscript = m_scripts[_GO->GetGOInfo()->ScriptId];
    if (!tmpscript || !tmpscript->pGOChooseReward) return false;

    player->PlayerTalkClass->ClearMenus();
    return tmpscript->pGOChooseReward(player,_GO,_Quest,opt);
}


bool ScriptMgr::AreaTrigger( Player *player, AreaTriggerEntry const* atEntry)
{
    Script *tmpscript = m_scripts[GetAreaTriggerScriptId(atEntry->id)];
    if (!tmpscript || !tmpscript->pAreaTrigger) return false;

    return tmpscript->pAreaTrigger(player, atEntry);
}

CreatureAI* ScriptMgr::GetAI(Creature *_Creature)
{
    Script *tmpscript = m_scripts[_Creature->GetScriptId()];
    if (!tmpscript || !tmpscript->GetAI) return NULL;
    
    return tmpscript->GetAI(_Creature);
}

CreatureAINew* ScriptMgr::getAINew(Creature* creature)
{
    CreatureScriptMap::const_iterator iter = m_creatureScripts.find(creature->getScriptName().c_str());
    if (iter == m_creatureScripts.end())
        return NULL;
    
    return iter->second->getAI(creature);
}

SpellScript* ScriptMgr::getSpellScript(Spell* spell)
{
    if (!spell)
        return NULL;
    
    SpellScriptMap::const_iterator iter = m_spellScripts.find(objmgr.getSpellScriptName(spell->m_spellInfo->Id));
    if (iter == m_spellScripts.end())
        return NULL;
        
    return iter->second->getScript(spell);
}

bool ScriptMgr::ItemUse( Player *player, Item* _Item, SpellCastTargets const& targets)
{
    Script *tmpscript = m_scripts[_Item->GetProto()->ScriptId];
    if (!tmpscript || !tmpscript->pItemUse) return false;

    return tmpscript->pItemUse(player,_Item,targets);
}


bool ScriptMgr::ReceiveEmote( Player *player, Creature *_Creature, uint32 emote )
{
    Script *tmpscript = m_scripts[_Creature->GetScriptId()];
    if (!tmpscript || !tmpscript->pReceiveEmote) return false;

    return tmpscript->pReceiveEmote(player, _Creature, emote);
}

bool ScriptMgr::EffectDummyCreature(Unit *caster, uint32 spellId, uint32 effIndex, Creature *crTarget)
{
    Script *tmpscript = m_scripts[crTarget->GetScriptId()];

    if (!tmpscript || !tmpscript->pEffectDummyCreature) return false;

    return tmpscript->pEffectDummyCreature(caster, spellId, effIndex, crTarget);
}

InstanceData* ScriptMgr::CreateInstanceData(Map *map)
{
    if (!map->IsDungeon()) return NULL;

    Script *tmpscript = m_scripts[((InstanceMap*)map)->GetScriptId()];
    if (!tmpscript || !tmpscript->GetInstanceData) return NULL;

    return tmpscript->GetInstanceData(map);
}
