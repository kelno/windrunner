/*  */

/* WORLD BOSS */
UPDATE `creature_template` SET `ScriptName`='boss_ysondre' WHERE `entry`=14887;
UPDATE `creature_template` SET `ScriptName`='boss_emeriss' WHERE `entry`=14889;
UPDATE `creature_template` SET `ScriptName`='boss_taerar' WHERE `entry`=14890;
UPDATE `creature_template` SET `ScriptName`='boss_shade_of_taerar' WHERE `entry`=15302;
UPDATE `creature_template` SET `ScriptName`='boss_kruul' WHERE `entry`=18338;
UPDATE `creature_template` SET `ScriptName`='boss_azuregos' WHERE `entry`=6109;
UPDATE `creature_template` SET `ScriptName`='mob_dementeddruids' WHERE `entry`=15260;

/* GO */
UPDATE `gameobject_template` SET `ScriptName`='go_northern_crystal_pylon' WHERE `entry`=164955;
UPDATE `gameobject_template` SET `ScriptName`='go_western_crystal_pylon' WHERE `entry`=164956;
UPDATE `gameobject_template` SET `ScriptName`='go_eastern_crystal_pylon' WHERE `entry`=164957;
UPDATE `gameobject_template` SET `ScriptName`='go_barov_journal' WHERE `entry`=180794;
UPDATE `gameobject_template` SET `ScriptName`='go_field_repair_bot_74A' where `entry`= 179552;
UPDATE `gameobject_template` SET `ScriptName`='go_orb_of_command' WHERE `entry`=179879;
UPDATE `gameobject_template` SET `ScriptName`='go_tablet_of_madness' WHERE `entry`=180368;
UPDATE `gameobject_template` SET `ScriptName`='go_tablet_of_the_seven' WHERE `entry`=169294;
UPDATE `gameobject_template` set `ScriptName`='go_manticron_cube' where entry = 181713;
UPDATE `gameobject_template` SET `ScriptName`='go_mausoleum_trigger' WHERE `entry` = 104593;
UPDATE `gameobject_template` SET `ScriptName`='go_mausoleum_door' WHERE `entry` = 176594;
UPDATE `gameobject_template` SET `scriptname`='go_crystal_prison' WHERE `entry`=185126;
UPDATE `gameobject_template` SET `ScriptName`='go_jump_a_tron' WHERE `entry` = 183146;
UPDATE `gameobject_template` SET `ScriptName`='go_ethereum_prison' WHERE `entry` = 184421;
UPDATE `gameobject_template` SET `ScriptName` = 'go_ethereum_prison' WHERE `entry` = 184418 LIMIT 1;
UPDATE `gameobject_template` SET `ScriptName` = 'go_ethereum_prison' WHERE `entry` = 184419 LIMIT 1;
UPDATE `gameobject_template` SET `ScriptName` = 'go_ethereum_prison' WHERE `entry` = 184420 LIMIT 1;
UPDATE `gameobject_template` SET `ScriptName` = 'go_ethereum_prison' WHERE `entry` = 184421 LIMIT 1;
UPDATE `gameobject_template` SET `ScriptName` = 'go_ethereum_prison' WHERE `entry` = 184422 LIMIT 1;
UPDATE `gameobject_template` SET `ScriptName` = 'go_ethereum_prison' WHERE `entry` = 184423 LIMIT 1;
UPDATE `gameobject_template` SET `ScriptName` = 'go_ethereum_prison' WHERE `entry` = 184424 LIMIT 1;
UPDATE `gameobject_template` SET `ScriptName` = 'go_ethereum_prison' WHERE `entry` = 184425 LIMIT 1;
UPDATE `gameobject_template` SET `ScriptName` = 'go_ethereum_prison' WHERE `entry` = 184426 LIMIT 1;
UPDATE `gameobject_template` SET `ScriptName` = 'go_ethereum_prison' WHERE `entry` = 184427 LIMIT 1;
UPDATE `gameobject_template` SET `ScriptName` = 'go_ethereum_prison' WHERE `entry` = 184428 LIMIT 1;
UPDATE `gameobject_template` SET `ScriptName` = 'go_ethereum_prison' WHERE `entry` = 184429 LIMIT 1;
UPDATE `gameobject_template` SET `ScriptName` = 'go_ethereum_prison' WHERE `entry` = 184430 LIMIT 1;
UPDATE `gameobject_template` SET `ScriptName` = 'go_ethereum_prison' WHERE `entry` = 184431 LIMIT 1;
UPDATE `gameobject_template` SET `scriptname`='go_sacred_fire_of_life' WHERE `entry` = 175944;
UPDATE `gameobject_template` SET `scriptname`='go_skull_pile' WHERE `entry`=185913;

/* GUARD */
UPDATE `creature_template` SET `ScriptName`='guard_azuremyst' WHERE `entry`=18038;
UPDATE `creature_template` SET `ScriptName`='guard_orgrimmar' WHERE `entry`=3296;
UPDATE `creature_template` SET `ScriptName`='guard_stormwind' WHERE `entry` IN (68,1976);
UPDATE `creature_template` SET `ScriptName`='guard_contested' WHERE `entry` IN (9460,4624,3502,11190,15184);
UPDATE `creature_template` SET `ScriptName`='guard_elwynnforest' WHERE `entry`=1423;
UPDATE `creature_template` SET `ScriptName`='guard_eversong' WHERE `entry`=16221;
UPDATE `creature_template` SET `ScriptName`='guard_darnassus' WHERE `entry`=4262;
UPDATE `creature_template` SET `ScriptName`='guard_teldrassil' WHERE `entry`=3571;
UPDATE `creature_template` SET `ScriptName`='guard_ironforge' WHERE `entry`=5595;
UPDATE `creature_template` SET `ScriptName`='guard_dunmorogh' WHERE `entry` IN (727,13076);
UPDATE `creature_template` SET `ScriptName`='guard_undercity' WHERE `entry`=5624;
UPDATE `creature_template` SET `ScriptName`='guard_bluffwatcher' WHERE `entry`=3084;
UPDATE `creature_template` SET `ScriptName`='guard_durotar' WHERE `entry`=5953;
UPDATE `creature_template` SET `ScriptName`='guard_mulgore' WHERE `entry` IN (3212,3215,3217,3218,3219,3220,3221,3222,3223,3224);
UPDATE `creature_template` SET `ScriptName`='guard_dunmorogh' WHERE `entry` IN (727,13076);
UPDATE `creature_template` SET `ScriptName`='guard_tirisfal' WHERE `entry` IN (1735,1738,2210,1744,1745,5725,1743,2209,1746,1742);
UPDATE `creature_template` SET `ScriptName`='guard_silvermoon' WHERE `entry`=16222;
UPDATE `creature_template` SET `ScriptName`='guard_exodar' WHERE `entry`=16733;
UPDATE `creature_template` SET `ScriptName`='guard_shattrath' WHERE `entry`=19687;
UPDATE `creature_template` SET `ScriptName`='guard_shattrath_aldor' WHERE `entry`=18549;
UPDATE `creature_template` SET `ScriptName`='guard_shattrath_scryer' WHERE `entry`=18568;

/* ITEM */
UPDATE `item_template` SET `ScriptName`='item_area_52_special' WHERE `entry`=28132;
UPDATE `item_template` SET `ScriptName`='item_attuned_crystal_cores' WHERE `entry`=34368;
UPDATE `item_template` SET `ScriptName`='item_blackwhelp_net' WHERE `entry`=31129;
UPDATE `item_template` SET `ScriptName`='item_disciplinary_rod' WHERE `entry`=22473;
UPDATE `item_template` SET `ScriptName`='item_draenei_fishing_net' WHERE `entry`=23654;
UPDATE `item_template` SET `ScriptName`='item_flying_machine' WHERE `entry` IN (34060,34061);
UPDATE `item_template` SET `ScriptName`='item_gor_dreks_ointment' WHERE `entry`=30175;
UPDATE `item_template` SET `ScriptName`='item_muiseks_vessel' WHERE `entry` IN (9606,9618,9619,9620,9621);
UPDATE `item_template` SET `ScriptName`='item_nether_wraith_beacon' WHERE `entry`=31742;
UPDATE `item_template` SET `ScriptName`='item_protovoltaic_magneto_collector' WHERE `entry`=30656;
UPDATE `item_template` SET `ScriptName`='item_razorthorn_flayer_gland' WHERE `entry`=34255;
UPDATE `item_template` SET `ScriptName`='item_soul_cannon' WHERE `entry`=32825;
UPDATE `item_template` SET `ScriptName`='item_sparrowhawk_net' WHERE `entry`=32321;
UPDATE `item_template` SET `ScriptName`='item_tainted_core' WHERE `entry`=31088;
UPDATE `item_template` SET `ScriptName`='item_tame_beast_rods' WHERE `entry` IN (15908,15911,15913,15914,15915,15916,15917,15919,15920,15921,15922,15923,23697,23702,23703,23896,23897,23898);
UPDATE `item_template` SET `ScriptName`='item_voodoo_charm' WHERE `entry`=8149;
UPDATE `item_template` SET `ScriptName`='item_vorenthals_presence' WHERE `entry`=30259;
UPDATE `item_template` SET `ScriptName`='item_yehkinyas_bramble' WHERE `entry`=10699;
UPDATE `item_template` SET `ScriptName`='item_zezzaks_shard' WHERE `entry`=31463;
UPDATE `item_template` SET `ScriptName` = "item_only_for_flight" WHERE `entry` IN (34475, 34489, 24538);

/* NPC (usually creatures to be found in more than one specific zone) */
UPDATE `creature_template` SET `ScriptName`='npc_chicken_cluck' WHERE `entry`=620;
UPDATE `creature_template` SET `ScriptName`='npc_dancing_flames' WHERE `entry`=25305;
UPDATE `creature_template` SET `ScriptName`='npc_guardian' WHERE `entry`=5764;
UPDATE `creature_template` SET `ScriptName`='npc_mount_vendor' WHERE `entry` IN (384,1261,1460,2357,3362,3685,4730,4731,4885,7952,7955,16264,17584);
UPDATE `creature_template` SET `ScriptName`='npc_doctor' WHERE `entry` IN (12939,12920);
UPDATE `creature_template` SET `ScriptName`='npc_injured_patient' WHERE `entry` IN (12936,12937,12938,12923,12924,12925);
UPDATE `creature_template` SET `ScriptName`='npc_prof_alchemy' WHERE `entry` IN (17909,19052,22427);
UPDATE `creature_template` SET `ScriptName`='npc_prof_blacksmith' WHERE `entry` IN (5164,11145,11146,11176,11177,11178,11191,11192,11193);
UPDATE `creature_template` SET `ScriptName`='npc_prof_leather' WHERE `entry` IN (7866,7867,7868,7869,7870,7871);
UPDATE `creature_template` SET `ScriptName`='npc_prof_tailor' WHERE `entry` IN (22208,22212,22213);
UPDATE `creature_template` SET `ScriptName`='npc_rogue_trainer' WHERE `entry` IN (918,4163,3328,4583,5165,5167,13283,16684);
UPDATE `creature_template` SET `ScriptName`='npc_sayge' WHERE `entry`=14822;
UPDATE `creature_template` SET `ScriptName`='npc_steam_tonk' WHERE `entry` = 19405;
UPDATE `creature_template` SET `ScriptName`='npc_tonk_mine' WHERE `entry` = 15368;
UPDATE `creature_template` SET `ScriptName`='npc_winter_reveler' WHERE `entry`=15760;
UPDATE `creature_template` SET `ScriptName`='npc_brewfest_reveler' WHERE `entry`=24484;
UPDATE `creature_template` SET `ScriptName`='npc_snake_trap_serpents' where entry in (19921, 19833);

/*  */
/* ZONE */
/* */

/* ALTERAC MOUNTAINS */
UPDATE `creature_template` SET `ScriptName`='npc_ravenholdt' WHERE `entry`=13936;

/* ALTERAC VALLEY */


/* ARATHI HIGHLANDS */
UPDATE `creature_template` SET `ScriptName`='npc_professor_phizzlethorpe' where entry=2768;

/* ASHENVALE */
UPDATE `creature_template` SET `ScriptName`='npc_torek' WHERE `entry`=12858;
UPDATE `creature_template` SET `ScriptName`='npc_ruul_snowhoof' WHERE `entry` = 12818;

/*  */
/* AUCHINDOUN */
/*  */

/* MANA TOMBS */
UPDATE `creature_template` SET `ScriptName`='boss_pandemonius' WHERE `entry`=18341;
UPDATE `creature_template` SET `ScriptName`='boss_nexusprince_shaffar' WHERE `entry`=18344;
UPDATE `creature_template` SET `ScriptName`='mob_ethereal_beacon' WHERE `entry`=18431;
UPDATE `creature_template` SET `ScriptName`='mob_ethereal_apprentice' WHERE `entry`=18430;

/* AUCHENAI CRYPTS */
UPDATE `creature_template` SET `ScriptName`='boss_exarch_maladaar' WHERE `entry`=18373;
UPDATE `creature_template` SET `ScriptName`='mob_avatar_of_martyred' WHERE `entry`=18478;
UPDATE `creature_template` SET `ScriptName`='mob_stolen_soul' WHERE `entry`=18441;
UPDATE `creature_template` SET `ScriptName`='boss_shirrak_the_dead_watcher' WHERE `entry` = '18371';
UPDATE `creature_template` SET `ScriptName`='mob_focus_fire' WHERE `entry` = '18374';

/* SETHEKK HALLS */
UPDATE `instance_template` SET `script`='instance_sethekk_halls' WHERE `map`=556;
UPDATE `creature_template` SET `ScriptName`='mob_syth_fire' WHERE `entry`=19203;
UPDATE `creature_template` SET `ScriptName`='mob_syth_arcane' WHERE `entry`=19205;
UPDATE `creature_template` SET `ScriptName`='mob_syth_frost' WHERE `entry`=19204;
UPDATE `creature_template` SET `ScriptName`='mob_syth_shadow' WHERE `entry`=19206;
UPDATE `creature_template` SET `ScriptName`='boss_talon_king_ikiss' WHERE `entry`=18473;
UPDATE `creature_template` SET `ScriptName`='boss_darkweaver_syth' WHERE `entry`=18472;

/* SHADOW LABYRINTH */
UPDATE `instance_template` SET `script`='instance_shadow_labyrinth' WHERE `map`=555;
UPDATE `creature_template` SET `ScriptName`='boss_murmur' WHERE `entry`=18708;
UPDATE `creature_template` SET `ScriptName`='boss_grandmaster_vorpil' WHERE `entry`=18732;
UPDATE `creature_template` SET `ScriptName`='boss_blackheart_the_inciter' WHERE `entry`=18667;
UPDATE `creature_template` SET `ScriptName`='boss_ambassador_hellmaw' WHERE `entry`=18731;

/* AZSHARA */
UPDATE `creature_template` SET `ScriptName`='mobs_spitelashes' WHERE `entry` IN (6190,6193,6194,6195,6196,7885,7886,12204,12205);
UPDATE `creature_template` SET `ScriptName`='npc_loramus_thalipedes' WHERE `entry`=7783;
UPDATE `creature_template` SET `ScriptName`='mob_rizzle_sprysprocket' WHERE `entry`=23002;
UPDATE `creature_template` SET `ScriptName`='mob_depth_charge' WHERE `entry`=23025;

/* AZUREMYST ISLE */
UPDATE `creature_template` SET `ScriptName`='npc_engineer_spark_overgrind' WHERE `entry`=17243;
UPDATE `creature_template` SET `ScriptName`='npc_injured_draenei' WHERE `entry`=16971;
UPDATE `creature_template` SET `ScriptName`='npc_susurrus' WHERE `entry`=17435;
UPDATE `creature_template` SET `ScriptName`='npc_magwin' WHERE `entry`=17312;
UPDATE `creature_template` SET `ScriptName`='npc_geezle' where entry=17318;
UPDATE `creature_template` SET `ScriptName`='mob_nestlewood_owlkin' where entry=16518;

/* BADLANDS */


/* BARRENS */
UPDATE `creature_template` SET `ScriptName`='npc_beaten_corpse' WHERE `entry`=10668;
UPDATE `creature_template` SET `ScriptName`='npc_sputtervalve' WHERE `entry`=3442;
UPDATE `creature_template` SET `ScriptName`='npc_taskmaster_fizzule' WHERE `entry`=7233;
UPDATE `creature_template` SET `ScriptName`='npc_twiggy_flathead' WHERE `entry` =6248;
UPDATE `creature_template` SET `ScriptName`='npc_wizzlecrank_shredder' where entry=3439;

/* BLACK TEMPLE */
UPDATE `instance_template` SET `script`='instance_black_temple' WHERE `map`=564;
UPDATE `creature_template` SET `ScriptName`='npc_akama_shade' WHERE `entry`=22990; -- Akama at Shade of Akama
UPDATE `creature_template` SET `ScriptName`='npc_akama_illidan' WHERE `entry`=23089; -- Akama at Illidan
UPDATE `creature_template` SET `ScriptName`='mob_illidari_council' WHERE `entry`=23426; -- Illidari Council controller mob
UPDATE `creature_template` SET `ScriptName`='mob_blood_elf_council_voice_trigger' WHERE `entry` = 23499; -- Voice Trigger Mob (Controls Aggro + Enrage yells)
UPDATE `creature_template` SET `ScriptName`='boss_veras_darkshadow' WHERE `entry`=22952; -- Rogue of Illidari Council
UPDATE `creature_template` SET `ScriptName`='boss_teron_gorefiend' WHERE `entry`=22871; -- Teron Gorefiend
UPDATE `creature_template` SET `ScriptName`='boss_supremus' WHERE `entry`=22898; -- Supremus
UPDATE `creature_template` SET `ScriptName`='boss_shade_of_akama' WHERE `entry`=22841; -- Shade of Akama
UPDATE `creature_template` SET `ScriptName`='boss_reliquary_of_souls' WHERE `entry`=22856; -- Reliquary Controller Mob
UPDATE `creature_template` SET `ScriptName`='boss_essence_of_suffering' WHERE `entry`=23418; -- Essence Of Suffering
UPDATE `creature_template` SET `ScriptName`='boss_essence_of_desire' WHERE `entry`=23419; -- Essence of Desire
UPDATE `creature_template` SET `ScriptName`='boss_essence_of_anger' WHERE `entry`=23420; -- Essence of Anger
UPDATE `creature_template` SET `ScriptName`='boss_najentus' WHERE `entry`=22887; -- High Warlord Naj'entus
UPDATE `creature_template` SET `ScriptName`='boss_gurtogg_bloodboil' WHERE `entry`=22948; -- Gurtogg Bloodboil
UPDATE `creature_template` SET `ScriptName`='boss_mother_shahraz' WHERE `entry`=22947; -- Mother Shahraz
UPDATE `creature_template` SET `ScriptName`='boss_lady_malande' WHERE `entry`=22951; -- Priest <3 at Illidari Council
UPDATE `creature_template` SET `ScriptName`='boss_illidan_stormrage' WHERE `entry`=22917; -- Illidan The Betrayer!
UPDATE `creature_template` SET `ScriptName`='boss_high_nethermancer_zerevor' WHERE `entry`=22950; -- Mage at Illidari Council
UPDATE `creature_template` SET `ScriptName`='boss_gathios_the_shatterer' WHERE `entry`=22949; -- Paladin at Illidari Council
UPDATE `creature_template` SET `ScriptName`='boss_maiev_shadowsong' WHERE `entry`=23197; -- Maiev Shadowsong
UPDATE `gameobject_template` SET `ScriptName`='gameobject_cage_trap' WHERE `entry`=185916; -- Cage Trap GO in Illidan Encounter
UPDATE `creature_template` SET `ScriptName`='mob_blaze' WHERE `entry`=23259; -- Blaze mob in Illidan Phase 2
UPDATE `creature_template` SET `ScriptName`='mob_flame_of_azzinoth' WHERE `entry`=22997; -- Flame of Azzinoth (Illidan Phase 2)
UPDATE `creature_template` SET `ScriptName`='mob_blade_of_azzinoth' WHERE `entry`=22996; -- Blade of Azzinoth (Illidan Phase 2)
UPDATE `creature_template` SET `ScriptName`='mob_demon_fire' WHERE `entry`=23069; -- Demon Fire in Illidan Phase 2
UPDATE `creature_template` SET `ScriptName`='mob_flame_crash' WHERE `entry`=23336; -- Flame Crash in Illidan Normal Form
UPDATE `creature_template` SET `ScriptName`='mob_cage_trap_trigger' WHERE `entry`=23304; -- Cage Trap mob in Illidan Phase 3/4 Normal
UPDATE `creature_template` SET `ScriptName`='mob_shadow_demon' WHERE `entry`=23375; -- Shadow Demon in Illidan Demon Form
UPDATE `creature_template` SET `ScriptName`='npc_volcano' WHERE `entry`=23085; -- Supremus Volcano
UPDATE `creature_template` SET `ScriptName`='molten_flame' WHERE `entry`=23095; -- Molten Flame in SUpremus
UPDATE `creature_template` SET `ScriptName`='mob_ashtongue_channeler' WHERE `entry`=23421; -- Ashtongue CHanneler in Shade of AKama
UPDATE `creature_template` SET `ScriptName`='mob_ashtongue_sorcerer' WHERE `entry`=23215; -- Ashtongue Sorcerer in Shade of Akama
UPDATE `creature_template` SET `ScriptName`='npc_enslaved_soul' WHERE `entry`=23469; -- Enslaved Soul in Reliquary Event
UPDATE `creature_template` SET `ScriptName`='mob_doom_blossom' WHERE `entry`=23123; -- Doom Blossoms in Teron Gorefiend's encounter
UPDATE `creature_template` SET `ScriptName`='npc_spirit_of_olum' WHERE `entry`=23411;
-- UPDATE `creature_template` SET `ScriptName`='mob_shadowy_construct' WHERE `entry`=23111; -- Shadowy Construct in Teron Gorefiend's encounter. Commented until Mind Control is implemented.

/* BLACKFATHOM DEPTHS */


/* BLACKROCK DEPTHS */
UPDATE `creature_template` SET `ScriptName`='boss_emperor_dagran_thaurissan' WHERE `entry`=9019;
UPDATE `creature_template` SET `ScriptName`='boss_moira_bronzebeard' WHERE `entry`=8929;
UPDATE `creature_template` SET `ScriptName`='boss_ambassador_flamelash' WHERE `entry`=9156;
UPDATE `creature_template` SET `ScriptName`='boss_angerrel' WHERE `entry`=9035;
UPDATE `creature_template` SET `ScriptName`='boss_anubshiah' WHERE `entry`=9031;
UPDATE `creature_template` SET `ScriptName`='boss_doomrel' WHERE `entry`=9039;
UPDATE `creature_template` SET `ScriptName`='boss_doperel' WHERE `entry`=9040;
UPDATE `creature_template` SET `ScriptName`='boss_general_angerforge' WHERE `entry`=9033;
UPDATE `creature_template` SET `ScriptName`='boss_gloomrel' WHERE `entry`=9037;
UPDATE `creature_template` SET `ScriptName`='boss_gorosh_the_dervish' WHERE `entry`=9027;
UPDATE `creature_template` SET `ScriptName`='boss_grizzle' WHERE `entry`=9028;
UPDATE `creature_template` SET `ScriptName`='boss_haterel' WHERE `entry`=9034;
UPDATE `creature_template` SET `ScriptName`='boss_high_interrogator_gerstahn' WHERE `entry`=9018;
UPDATE `creature_template` SET `ScriptName`='boss_magmus' WHERE `entry`=9938;
UPDATE `creature_template` SET `ScriptName`='boss_seethrel' WHERE `entry`=9038;
UPDATE `creature_template` SET `ScriptName`='boss_vilerel' WHERE `entry`=9036;
UPDATE `creature_template` SET `ScriptName`='phalanx' WHERE `entry`=9502;
UPDATE `creature_template` SET `ScriptName`='npc_lokhtos_darkbargainer' WHERE `entry`=12944;
UPDATE `creature_template` SET `ScriptName`='npc_kharan_mighthammer' WHERE `entry`=9021;
UPDATE `creature_template` SET `ScriptName`='npc_grimstone' WHERE `entry`=10096;
UPDATE `creature_template` SET `ScriptName`='npc_rocknot' WHERE `entry`=9503;

/* BLACKROCK SPIRE */
/* BLACKROCK SPIRE Lower bosses */
UPDATE `creature_template` SET `ScriptName`='boss_highlord_omokk' WHERE `entry`=9196;
UPDATE `creature_template` SET `ScriptName`='boss_shadow_hunter_voshgajin' WHERE `entry`=9236;
UPDATE `creature_template` SET `ScriptName`='boss_warmaster_voone' WHERE `entry`=9237;
UPDATE `creature_template` SET `ScriptName`='boss_mother_smolderweb' WHERE `entry`=10596;
UPDATE `creature_template` SET `ScriptName`='quartermaster_zigris' WHERE `entry`=9736;
UPDATE `creature_template` SET `ScriptName`='boss_halycon' WHERE `entry`=10220;
UPDATE `creature_template` SET `ScriptName`='boss_overlord_wyrmthalak' WHERE `entry`=9568;
/* BLACKROCK SPIRE Upper bosses */
UPDATE `creature_template` SET `ScriptName`='boss_the_beast' WHERE `entry`=10430;
UPDATE `creature_template` SET `ScriptName`='boss_drakkisath' WHERE `entry`=10363;
UPDATE `creature_template` SET `ScriptName`='boss_gyth' WHERE `entry`=10339;
UPDATE `creature_template` SET `ScriptName`='boss_rend_blackhand' WHERE `entry`=10429;
UPDATE `creature_template` SET `ScriptName`='boss_pyroguard_emberseer' WHERE `entry`=9816;

/* BLACKWING LAIR */
UPDATE `instance_template` SET `script`='instance_blackwing_lair' WHERE `map`=469;
UPDATE `creature_template` SET `ScriptName`='boss_razorgore' WHERE `entry`=12435;
UPDATE `creature_template` SET `ScriptName`='boss_vaelastrasz' WHERE `entry`=13020;
UPDATE `creature_template` SET `ScriptName`='boss_broodlord' WHERE `entry`=12017;
UPDATE `creature_template` SET `ScriptName`='boss_firemaw' WHERE `entry`=11983;
UPDATE `creature_template` SET `ScriptName`='boss_ebonroc' WHERE `entry`=14601;
UPDATE `creature_template` SET `ScriptName`='boss_flamegor' WHERE `entry`=11981;
UPDATE `creature_template` SET `ScriptName`='boss_chromaggus' WHERE `entry`=14020;
UPDATE `creature_template` SET `ScriptName`='boss_victor_nefarius' WHERE `entry`=10162;
UPDATE `creature_template` SET `ScriptName`='boss_nefarian' WHERE `entry`=11583;

/* BLADE'S EDGE MOUNTAINS */
UPDATE `creature_template` SET `ScriptName`='mobs_bladespire_ogre' WHERE `entry` IN (19998,20334,21296,21975);
UPDATE `creature_template` SET `ScriptName`='mobs_nether_drake' WHERE `entry` IN (20021,21817,21820,21821,21823);
UPDATE `creature_template` SET `ScriptName`='npc_daranelle' WHERE `entry`=21469;
UPDATE `creature_template` SET `ScriptName`='npc_overseer_nuaar' WHERE `entry`=21981;
UPDATE `creature_template` SET `ScriptName`='npc_saikkal_the_elder' WHERE `entry`=22932;
UPDATE `creature_template` SET `ScriptName`='npc_skyguard_handler_deesak' WHERE `entry`=23415;

/* BLASTED LANDS */
UPDATE `creature_template` SET `ScriptName`='npc_deathly_usher' WHERE `entry`=8816;
UPDATE `creature_template` SET `ScriptName`='npc_fallen_hero_of_horde' WHERE `entry`=7572;

/* BLOODMYST ISLE */
UPDATE `creature_template` SET `ScriptName`='mob_webbed_creature' WHERE `entry`=17680;
UPDATE `creature_template` SET `ScriptName`='npc_captured_sunhawk_agent' WHERE `entry`=17824;

/* BURNING STEPPES */
UPDATE `creature_template` SET `ScriptName`='npc_ragged_john' WHERE `entry`=9563;

/*  */
/* CAVERNS OF TIME */
/*  */

/* MT. HYJAL */
UPDATE `instance_template` SET `script`='instance_hyjal' WHERE `map`=534;
UPDATE `creature_template` SET `ScriptName`='npc_tyrande_whisperwind' WHERE `entry`=17948;
UPDATE `creature_template` SET `ScriptName`='npc_thrall' WHERE `entry` =17852;
UPDATE `creature_template` SET `ScriptName`='npc_jaina_proudmoore' WHERE `entry`=17772;
UPDATE `creature_template` SET `ScriptName`='boss_archimonde' WHERE `entry`=17968;
UPDATE `creature_template` SET `ScriptName`='mob_doomfire' WHERE `entry`=18095;
UPDATE `creature_template` SET `ScriptName`='mob_doomfire_targetting' WHERE `entry`=18104;
UPDATE `creature_template` SET `ScriptName`='mob_ancient_wisp' WHERE `entry`=17946;
UPDATE `creature_template` SET `ScriptName`='mob_giant_infernal' WHERE `entry`=17908;
UPDATE `creature_template` SET `ScriptName`='mob_abomination' WHERE `entry`=17898;
UPDATE `creature_template` SET `ScriptName`='mob_ghoul' WHERE `entry`=17895;
UPDATE `creature_template` SET `ScriptName`='mob_necromancer' WHERE `entry`=17899;
UPDATE `creature_template` SET `ScriptName`='mob_banshee' WHERE `entry`=17905;
UPDATE `creature_template` SET `ScriptName`='mob_crypt_fiend' WHERE `entry`=17897;
UPDATE `creature_template` SET `ScriptName`='mob_fel_stalker' WHERE `entry`=17916;
UPDATE `creature_template` SET `ScriptName`='mob_frost_wyrm' WHERE `entry`=17907;
UPDATE `creature_template` SET `ScriptName`='mob_gargoyle' WHERE `entry`=17906;
UPDATE `creature_template` SET `ScriptName`='alliance_rifleman' WHERE `entry`=17921;
UPDATE `creature_template` SET `ScriptName`='mob_towering_infernal' WHERE `entry`=17818;
UPDATE `creature_template` SET `ScriptName`='boss_anetheron' WHERE `entry`=17808;
UPDATE `creature_template` SET `ScriptName`='boss_azgalor' WHERE `entry`=17842;
UPDATE `creature_template` SET `ScriptName`='mob_lesser_doomguard' WHERE `entry`=17864;
UPDATE `creature_template` SET `ScriptName`='boss_kazrogal' WHERE `entry`=17888;
UPDATE `creature_template` SET `ScriptName`='boss_rage_winterchill' WHERE `entry`=17767;

/* OLD HILLSBRAD */
UPDATE `instance_template` SET `script`='instance_old_hillsbrad' WHERE `map`=560;
UPDATE `creature_template` SET `ScriptName`='boss_lieutenant_drake' WHERE `entry`=17848;
UPDATE `creature_template` SET `ScriptName`='boss_epoch_hunter' WHERE `entry`=18096;
UPDATE `creature_template` SET `ScriptName`='boss_captain_skarloc' WHERE `entry`=17862;
UPDATE `creature_template` SET `ScriptName`='npc_brazen' WHERE `entry`=18725;
UPDATE `creature_template` SET `ScriptName`='npc_erozion' WHERE `entry`=18723;
UPDATE `creature_template` SET `ScriptName`='npc_taretha' WHERE `entry`=18887;
UPDATE `creature_template` SET `ScriptName`='npc_thrall_old_hillsbrad' WHERE `entry`=17876;
UPDATE `gameobject_template` SET `ScriptName`='go_barrel_old_hillsbrad' WHERE `entry`=182589;

/* THE DARK PORTAL */
UPDATE `instance_template` SET `script`='instance_dark_portal' WHERE `map`=269;
UPDATE `creature_template` SET `ScriptName`='boss_chrono_lord_deja' WHERE `entry`=17879;
UPDATE `creature_template` SET `ScriptName`='boss_aeonus' WHERE `entry`=17881;
UPDATE `creature_template` SET `ScriptName`='boss_temporus' WHERE `entry`=17880;
UPDATE `creature_template` SET `ScriptName`='npc_medivh_bm' WHERE `entry`=15608;
UPDATE `creature_template` SET `ScriptName`='npc_time_rift' WHERE `entry`=17838;
UPDATE `creature_template` SET `ScriptName`='npc_saat' WHERE `entry`=20201;


/*  */
/* COILFANG RESERVOIR */
/*  */

/* THE SLAVE PENS */

/* THE UNDERBOG */
UPDATE `creature_template` SET `ScriptName`='mob_underbog_mushroom' WHERE `entry`=17990;
UPDATE `creature_template` SET `ScriptName`='boss_hungarfen' WHERE `entry`=17770;
UPDATE `creature_template` SET `ScriptName`='boss_the_black_stalker' WHERE `entry`='17882';

/* THE STEAMVAULT */
UPDATE `instance_template` SET `script`='instance_steam_vault' WHERE `map`=545;
UPDATE `creature_template` SET `ScriptName`='boss_hydromancer_thespia' WHERE `entry`=17797;
UPDATE `creature_template` SET `ScriptName`='boss_mekgineer_steamrigger' WHERE `entry`=17796;
UPDATE `creature_template` SET `ScriptName`='boss_warlord_kalithresh' WHERE `entry`=17798;
UPDATE `creature_template` SET `ScriptName`='mob_coilfang_waterelemental' WHERE `entry`=17917;
UPDATE `creature_template` SET `ScriptName`='mob_naga_distiller' WHERE `entry`=17954;
UPDATE `creature_template` SET `ScriptName`='mob_steamrigger_mechanic' WHERE `entry`=17951;

/* SERPENTSHRINE CAVERN */
UPDATE `instance_template` SET `script`='instance_serpent_shrine' WHERE `map`=548;
UPDATE `creature_template` SET `ScriptName`='boss_hydross_the_unstable' WHERE `entry`=21216;
/* Leotheras the Blind event */
UPDATE `creature_template` SET `ScriptName`='boss_leotheras_the_blind' WHERE `entry`=21215;
UPDATE `creature_template` SET `ScriptName`='boss_leotheras_the_blind_demonform' WHERE `entry`=21845;
UPDATE `creature_template` SET `ScriptName`='mob_greyheart_spellbinder' WHERE `entry` = '21806';
/* Fathom-lord Karathress event */
UPDATE `creature_template` SET `ScriptName`='boss_fathomlord_karathress' WHERE `entry`=21214;
UPDATE `creature_template` SET `ScriptName`='boss_fathomguard_sharkkis' WHERE `entry`=21966;
UPDATE `creature_template` SET `ScriptName`='boss_fathomguard_tidalvess' WHERE `entry`=21965;
UPDATE `creature_template` SET `ScriptName`='boss_fathomguard_caribdis' WHERE `entry`=21964;
/* Morogrim Tidewalker event */
UPDATE `creature_template` SET `ScriptName`='boss_morogrim_tidewalker' WHERE `entry`=21213;
UPDATE `creature_template` SET `ScriptName`='mob_water_globule' WHERE `entry`=21913;
/* The Lurker Below */
UPDATE `creature_template` SET `ScriptName`='boss_the_lurker_below' WHERE `entry` = '21217';
UPDATE `creature_template` SET `ScriptName`='mob_coilfang_ambusher' WHERE `entry` = '21865';
UPDATE `creature_template` SET `ScriptName`='mob_coilfang_guardian' WHERE `entry` = '21873';
/* Lady Vashj event */
UPDATE `creature_template` SET `ScriptName`='boss_lady_vashj' WHERE `entry`=21212;
UPDATE `creature_template` SET `ScriptName`='mob_enchanted_elemental' WHERE `entry`=21958;
UPDATE `creature_template` SET `ScriptName`='mob_tainted_elemental' WHERE `entry`=22009;
UPDATE `creature_template` SET `ScriptName`='mob_coilfang_elite' WHERE `entry`=22055;
UPDATE `creature_template` SET `ScriptName`='mob_coilfang_strider' WHERE `entry`=22056;
UPDATE `creature_template` SET `ScriptName`='mob_toxic_sporebat' WHERE entry=22140;
UPDATE `creature_template` SET `ScriptName`='mob_shield_generator_channel' WHERE `entry`=19870;


/* DARKSHORE */


/* DEADMINES */
UPDATE `instance_template` SET `script`='instance_deadmines' WHERE `map`=36;
UPDATE `item_template` SET `ScriptName`='item_defias_gunpowder' WHERE `entry`=5397;
UPDATE `gameobject_template` SET `ScriptName`='go_defias_cannon' WHERE `entry`=16398;
UPDATE `gameobject_template` SET `ScriptName`='go_door_lever_dm' WHERE `entry`=101833;
UPDATE `gameobject_template` SET `ScriptName`='go_main_chambers_access_panel' WHERE `entry` IN (184125,184126);

/* DEADWIND PASS */


/* DESOLACE */


/* DIRE MAUL */


/* DUN MOROGH */
UPDATE `creature_template` SET `ScriptName`='npc_narm_faulk' WHERE `entry`=6177;


/* DUROTAR */


/* DUSKWOOD */


/* DUSTWALLOW MARSH */
UPDATE `creature_template` SET `ScriptName`='mobs_risen_husk_spirit' WHERE `entry` IN (23554,23555);
UPDATE `creature_template` SET `ScriptName`='npc_deserter_agitator' WHERE `entry`=23602;
UPDATE `creature_template` SET `ScriptName`='npc_lady_jaina_proudmoore' WHERE `entry`=4968;
UPDATE `creature_template` SET `ScriptName`='npc_nat_pagle' WHERE `entry`=12919;
UPDATE `creature_template` SET `ScriptName`='npc_restless_apparition' WHERE `entry`=23861;

/* EASTERN PLAGUELANDS */
UPDATE `creature_template` SET `ScriptName`='mobs_ghoul_flayer' WHERE `entry` IN (8530,8531,8532);
UPDATE `creature_template` SET `ScriptName`='npc_augustus_the_touched' WHERE `entry`=12384;
UPDATE `creature_template` SET `ScriptName`='npc_darrowshire_spirit' WHERE `entry`=11064;
UPDATE `creature_template` SET `ScriptName`='npc_tirion_fordring' WHERE `entry`=1855;

/* ELWYNN FOREST */
UPDATE `creature_template` SET `ScriptName`='npc_henze_faulk' WHERE `entry`=6172;

/* EVERSONG WOODS */
UPDATE `creature_template` SET `ScriptName`='mobs_mana_tapped' WHERE `entry` IN (15273,15274,15294,15298,15367);
UPDATE `creature_template` SET `ScriptName`='npc_prospector_anvilward' WHERE `entry`=15420;
UPDATE `creature_template` SET `ScriptName`='npc_second_trial_paladin' WHERE `entry` IN (17809,17810,17811,17812);
UPDATE `creature_template` SET `ScriptName`='npc_second_trial_controller' WHERE `entry` IN (17807);
UPDATE `creature_template` SET `ScriptName`='npc_apprentice_mirveda' where entry=15402;
UPDATE `creature_template` SET `ScriptName`='npc_infused_crystal' where entry=16364;
UPDATE `gameobject_template` SET `ScriptName`='go_second_trial' WHERE `entry` IN (182052);

/* FELWOOD */
UPDATE `creature_template` SET `ScriptName`='npcs_riverbreeze_and_silversky' WHERE `entry` IN (9528,9529);

/* FERALAS */
UPDATE `creature_template` SET `ScriptName`='npc_gregan_brewspewer' WHERE `entry`=7775;
UPDATE `creature_template` SET `ScriptName`='npc_screecher_spirit' WHERE `entry`=8612;

/* GHOSTLANDS */
UPDATE `creature_template` SET `ScriptName`='npc_blood_knight_dawnstar' WHERE `entry`=17832;
UPDATE `creature_template` SET `ScriptName`='npc_budd_nedreck' WHERE `entry`=23559;
UPDATE `creature_template` SET `ScriptName`='npc_rathis_tomber' WHERE `entry`=16224;
UPDATE `creature_template` SET `Scriptname`='npc_ranger_lilatha' WHERE entry=16295;
update `gameobject_template` SET `ScriptName`='go_gilded_brazier' where entry = 181956;

/* GNOMEREGAN */


/* GRUUL'S LAIR */
UPDATE `instance_template` SET `script`='instance_gruuls_lair' WHERE `map` =565;
UPDATE `creature_template` SET `ScriptName`='boss_gruul' WHERE `entry`=19044;
/* Maulgar and Event */
UPDATE `creature_template` SET `ScriptName`='boss_high_king_maulgar' WHERE `entry`=18831;
UPDATE `creature_template` SET `ScriptName`='boss_kiggler_the_crazed' WHERE `entry`=18835;
UPDATE `creature_template` SET `ScriptName`='boss_blindeye_the_seer' WHERE `entry`=18836;
UPDATE `creature_template` SET `ScriptName`='boss_olm_the_summoner' WHERE `entry`=18834;
UPDATE `creature_template` SET `ScriptName`='boss_krosh_firehand' WHERE `entry`=18832;

/*  */
/* HELLFIRE CITADEL */
/*  */

/* BLOOD FURNACE */
/* The Maker,Broggok,Kelidan,Broggok's cloud */
UPDATE `instance_template` SET `script` = 'instance_blood_furnace' WHERE `map` = '542';
UPDATE `creature_template` SET `ScriptName`='boss_the_maker' WHERE `entry`=17381;
UPDATE `creature_template` SET `ScriptName`='boss_broggok' WHERE `entry`=17380;
UPDATE `creature_template` SET `ScriptName`='boss_kelidan_the_breaker' WHERE `entry`=17377;
UPDATE `creature_template` SET `ScriptName`='mob_broggok_poisoncloud' WHERE `entry`=17662;
UPDATE `creature_template` SET `ScriptName`='mob_shadowmoon_channeler' WHERE `entry`=17653;

/* HELLFIRE RAMPARTS */
/* Vazruden,Omor the Unscarred,Watchkeeper Gargolmar */
UPDATE `creature_template` SET `ScriptName`='boss_omor_the_unscarred' WHERE `entry`=17308;
UPDATE `creature_template` SET `ScriptName`='boss_watchkeeper_gargolmar' WHERE `entry`=17306;
UPDATE `creature_template` SET `ScriptName`='boss_vazruden_the_herald' WHERE `entry` = '17307';
UPDATE `creature_template` SET `ScriptName`='boss_vazruden' WHERE `entry` = '17537';
UPDATE `creature_template` SET `ScriptName`='boss_nazan' WHERE `entry` = '17536';
UPDATE `creature_template` SET `ScriptName`='mob_hellfire_sentry' WHERE `entry` = '17517';
	
/* SHATTERED HALLS */
/* Nethekurse and his spawned shadowfissure */
UPDATE `creature_template` SET `ScriptName`='boss_grand_warlock_nethekurse' WHERE `entry`=16807;
UPDATE `creature_template` SET `ScriptName`='boss_warchief_kargath_bladefist' WHERE `entry`=16808;
UPDATE `creature_template` SET `ScriptName`='boss_warbringer_omrogg' WHERE `entry`=16809;
UPDATE `creature_template` SET `ScriptName`='mob_fel_orc_convert' WHERE `entry`=17083;
UPDATE `creature_template` SET `ScriptName`='mob_lesser_shadow_fissure' WHERE `entry`=17471;
UPDATE `creature_template` SET `ScriptName`='mob_omrogg_heads' WHERE `entry` IN (19523,19524);
UPDATE `instance_template` SET `script`='instance_shattered_halls' WHERE `map`=540;

/* MAGTHERIDON'S LAIR */
UPDATE `instance_template` SET `script`='instance_magtheridons_lair' WHERE `map`=544;
UPDATE `gameobject_template` SET `ScriptName`='go_manticron_cube' WHERE `entry`=181713;
UPDATE `creature_template` SET `ScriptName`='boss_magtheridon' WHERE `entry` =17257;
UPDATE `creature_template` SET `ScriptName`='mob_hellfire_channeler' WHERE `entry`=17256;
update `creature_template` SET `ScriptName`='mob_abyssal' where entry = 17454;

/* HELLFIRE PENINSULA */
UPDATE `creature_template` SET `ScriptName`='boss_doomlord_kazzak' WHERE `entry`=18728;
UPDATE `creature_template` SET `ScriptName`='npc_wing_commander_brack' WHERE `entry`=19401;
UPDATE `creature_template` SET `ScriptName`='npc_wing_commander_dabiree' WHERE `entry`=19409;
UPDATE `creature_template` SET `ScriptName`='npc_gryphoneer_windbellow' WHERE `entry`=20235;
UPDATE `creature_template` SET `ScriptName`='npc_wounded_blood_elf' WHERE `entry`=16993;

/* HILLSBRAD FOOTHILLS */


/* HINTERLANDS */


/* IRONFORGE */
UPDATE `creature_template` SET `ScriptName`='npc_royal_historian_archesonus' WHERE `entry`=8879;

/* ISLE OF QUEL'DANAS */
UPDATE `creature_template` SET `ScriptName`='npc_ayren_cloudbreaker' WHERE `entry`=25059;
UPDATE `creature_template` SET `ScriptName`='npc_converted_sentry' WHERE `entry`=24981;
UPDATE `creature_template` SET `ScriptName`='npc_unrestrained_dragonhawk' WHERE `entry`=25236;
UPDATE `creature_template` SET `ScriptName`='npc_greengill_slave' where entry=25084;

/* KARAZHAN */
UPDATE `instance_template` SET `script`='instance_karazhan' WHERE `map`=532;
UPDATE `creature_template` SET `ScriptName`='boss_midnight' WHERE `entry`=16151;
UPDATE `creature_template` SET `ScriptName`='boss_attumen' WHERE `entry`=15550;
UPDATE `creature_template` SET `ScriptName`='boss_moroes' WHERE `entry`=15687;
UPDATE `creature_template` SET `ScriptName`='boss_maiden_of_virtue' WHERE `entry`=16457;
UPDATE `creature_template` SET `ScriptName`='boss_curator' WHERE `entry`=15691;
UPDATE `creature_template` SET `ScriptName`='boss_julianne' WHERE `entry`=17534;
UPDATE `creature_template` SET `ScriptName`='boss_romulo' WHERE `entry`=17533;
UPDATE `creature_template` SET `ScriptName`='boss_dorothee' WHERE `entry`=17535;
UPDATE `creature_template` SET `ScriptName`='boss_strawman' WHERE `entry`=17543;
UPDATE `creature_template` SET `ScriptName`='boss_tinhead' WHERE `entry`=17547;
UPDATE `creature_template` SET `ScriptName`='mob_tito' WHERE `entry`=17548;
UPDATE `creature_template` SET `ScriptName`='boss_roar' WHERE `entry`=17546;
UPDATE `creature_template` SET `ScriptName`='boss_crone' WHERE `entry`=18168;
UPDATE `creature_template` SET `ScriptName`='boss_terestian_illhoof' WHERE `entry`=15688;
UPDATE `creature_template` SET `ScriptName`='boss_shade_of_aran' WHERE `entry`=16524;
UPDATE `creature_template` SET `ScriptName`='boss_netherspite' WHERE `entry`=15689;
UPDATE `creature_template` SET `ScriptName`='boss_malchezaar' WHERE `entry`=15690;
UPDATE `creature_template` SET `ScriptName`='boss_nightbane' WHERE `entry`=17225;
UPDATE `creature_template` SET `ScriptName`='boss_baroness_dorothea_millstipe' WHERE `entry`=19875;
UPDATE `creature_template` SET `ScriptName`='boss_baron_rafe_dreuger' WHERE `entry`=19874;
UPDATE `creature_template` SET `ScriptName`='boss_lady_catriona_von_indi' WHERE `entry`=19872;
UPDATE `creature_template` SET `ScriptName`='boss_lady_keira_berrybuck' WHERE `entry`=17007;
UPDATE `creature_template` SET `ScriptName`='boss_lord_robin_daris' WHERE `entry`=19876;
UPDATE `creature_template` SET `ScriptName`='boss_lord_crispin_ference' WHERE `entry`=19873;
UPDATE `creature_template` SET `ScriptName`='boss_bigbadwolf' WHERE `entry`=17521;
UPDATE `creature_template` SET `ScriptName`='mob_shadow_of_aran' WHERE `entry`=18254;
UPDATE `creature_template` SET `ScriptName`='mob_aran_elemental' WHERE `entry`=17167;
UPDATE `creature_template` SET `ScriptName`='mob_aran_blizzard' WHERE `entry`=17161;
UPDATE `creature_template` SET `ScriptName`='mob_homunculus' WHERE `entry`=16539;
UPDATE `creature_template` SET `ScriptName`='mob_fiendish_imp' WHERE `entry`=17267;
UPDATE `creature_template` SET `ScriptName`='mob_kilrek' WHERE `entry`=17229;
UPDATE `creature_template` SET `ScriptName`='mob_demon_chain' WHERE `entry`=17248;
UPDATE `creature_template` SET `ScriptName`='mob_cyclone' WHERE `entry`=18412;
UPDATE `creature_template` SET `ScriptName`='netherspite_infernal' WHERE `entry`=17646;
UPDATE `creature_template` SET `ScriptName`='npc_berthold' WHERE `entry`=16153;
UPDATE `creature_template` SET `ScriptName`='npc_barnes' WHERE `entry`=16812;
UPDATE `creature_template` SET `ScriptName`='npc_grandmother' WHERE `entry`=17603;
UPDATE `creature_template` SET `ScriptName`='npc_image_of_medivh' where entry = 17651;

/* LOCH MODAN */
UPDATE `creature_template` SET `ScriptName`='npc_mountaineer_pebblebitty' WHERE `entry`=3836;

/* Magister's Terrace */
UPDATE `instance_template` SET `script`='instance_magisters_terrace' WHERE `map`=585;
UPDATE `creature_template` SET `ScriptName`='boss_selin_fireheart' WHERE `entry`=24723;
UPDATE `creature_template` SET `ScriptName`='mob_fel_crystal' WHERE `entry`=24722;
UPDATE `creature_template` SET `ScriptName`='boss_vexallus' WHERE `entry`=24744;
UPDATE `creature_template` SET `ScriptName`='mob_pure_energy' WHERE `entry`=24745;
UPDATE `creature_template` SET `ScriptName`='boss_priestess_delrissa' WHERE `entry`=24560;
UPDATE `creature_template` SET `ScriptName`='boss_kagani_nightstrike' WHERE `entry`=24557;
UPDATE `creature_template` SET `ScriptName`='boss_ellris_duskhallow' WHERE `entry`=24558;
UPDATE `creature_template` SET `ScriptName`='boss_eramas_brightblaze' WHERE `entry`=24554;
UPDATE `creature_template` SET `ScriptName`='boss_yazzai' WHERE `entry`=24561;
UPDATE `creature_template` SET `ScriptName`='boss_warlord_salaris' WHERE `entry`=24559;
UPDATE `creature_template` SET `ScriptName`='boss_garaxxas' WHERE `entry`=24555;
UPDATE `creature_template` SET `ScriptName`='mob_sliver' WHERE `entry`=24552;
UPDATE `creature_template` SET `ScriptName`='boss_apoko' WHERE `entry`=24553;
UPDATE `creature_template` SET `ScriptName`='boss_zelfan' WHERE `entry`=24556;
UPDATE `creature_template` SET `ScriptName`='boss_felblood_kaelthas' WHERE `entry`=24664;
UPDATE `creature_template` SET `ScriptName`='mob_arcane_sphere' WHERE `entry`=24708;
UPDATE `creature_template` SET `ScriptName`='mob_felkael_phoenix' WHERE `entry`=24674;
UPDATE `creature_template` SET `ScriptName`='mob_felkael_phoenix_egg' WHERE `entry`=24675;
UPDATE `creature_template` SET `ScriptName`='mob_felkael_flamestrike' WHERE `entry`=24666;

/* MARAUDON */
UPDATE `creature_template` SET `ScriptName`='boss_princess_theradras' WHERE `entry`=12201;
UPDATE `creature_template` SET `ScriptName`='boss_noxxion' WHERE `entry`=13282;
UPDATE `creature_template` SET `ScriptName`='boss_landslide' WHERE `entry`=12203;
UPDATE `creature_template` SET `ScriptName`='celebras_the_cursed' WHERE `entry`=12225;

/* MOLTEN CORE */
UPDATE `instance_template` SET `script`='instance_molten_core' WHERE `map`=409;
UPDATE `creature_template` SET `ScriptName`='boss_lucifron' WHERE `entry`=12118;
UPDATE `creature_template` SET `ScriptName`='boss_magmadar' WHERE `entry`=11982;
UPDATE `creature_template` SET `ScriptName`='boss_gehennas' WHERE `entry`=12259;
UPDATE `creature_template` SET `ScriptName`='boss_garr' WHERE `entry`=12057;
UPDATE `creature_template` SET `ScriptName`='boss_baron_geddon' WHERE `entry`=12056;
UPDATE `creature_template` SET `ScriptName`='boss_shazzrah' WHERE `entry`=12264;
UPDATE `creature_template` SET `ScriptName`='boss_golemagg' WHERE `entry`=11988;
UPDATE `creature_template` SET `ScriptName`='boss_sulfuron' WHERE `entry`=12098;
UPDATE `creature_template` SET `ScriptName`='boss_majordomo' WHERE `entry`=12018;
UPDATE `creature_template` SET `ScriptName`='boss_ragnaros' WHERE `entry`=11502;
UPDATE `creature_template` SET `ScriptName`='mob_ancient_core_hound' WHERE `entry`=11673;
UPDATE `creature_template` SET `ScriptName`='mob_firesworn' WHERE `entry`=12099;
UPDATE `creature_template` SET `ScriptName`='mob_core_rager' WHERE `entry`=11672;
UPDATE `creature_template` SET `ScriptName`='mob_flamewaker_priest' WHERE `entry`=11662;

/* MOONGLADE */
UPDATE `creature_template` SET `ScriptName`='npc_bunthen_plainswind' WHERE `entry`=11798;
UPDATE `creature_template` SET `ScriptName`='npc_great_bear_spirit' WHERE `entry`=11956;
UPDATE `creature_template` SET `ScriptName`='npc_silva_filnaveth' WHERE `entry`=11800;
UPDATE `creature_template` SET `ScriptName`='npc_clintar_dreamwalker' WHERE `entry`=22834;
UPDATE `creature_template` SET `ScriptName`='npc_clintar_spirit' WHERE `entry` = 22916;

/* MULGORE */
UPDATE `creature_template` SET `ScriptName`='npc_skorn_whitecloud' WHERE `entry`=3052;
UPDATE `creature_template` SET `ScriptName`='npc_kyle_frenzied' WHERE `entry`='23616';
UPDATE `creature_template` SET `ScriptName`='npc_plains_vision' WHERE `entry`='2983';

/* NAGRAND */
UPDATE `creature_template` SET `ScriptName`='mob_lump' WHERE `entry`=18351;
UPDATE `creature_template` SET `ScriptName`='mob_shattered_rumbler' WHERE `entry`=17157;
UPDATE `creature_template` SET `ScriptName`='mob_sunspring_villager' WHERE `entry`=18240;
UPDATE `creature_template` SET `ScriptName`='npc_altruis_the_sufferer' WHERE `entry`=18417;
UPDATE `creature_template` SET `ScriptName`='npc_greatmother_geyah' WHERE `entry`=18141;
UPDATE `creature_template` SET `ScriptName`='npc_lantresor_of_the_blade' WHERE `entry`=18261;
UPDATE `creature_template` SET `ScriptName`='npc_creditmarker_visit_with_ancestors' WHERE `entry` IN (18840,18841,18842,18843);
UPDATE `creature_template` SET `ScriptName`='mob_sparrowhawk' WHERE `entry`=22979;

/* NAXXRAMAS */
UPDATE `instance_template` SET `script`='instance_naxxramas' WHERE `map`=533;
UPDATE `creature_template` SET `ScriptName`='boss_anubrekhan' WHERE `entry`=15956;
UPDATE `creature_template` SET `ScriptName`='boss_faerlina' WHERE `entry`=15953;
UPDATE `creature_template` SET `ScriptName`='boss_maexxna' WHERE `entry`=15952;
UPDATE `creature_template` SET `ScriptName`='boss_noth' WHERE `entry`=15954;
UPDATE `creature_template` SET `ScriptName`='boss_heigan' WHERE `entry`=15936;
UPDATE `creature_template` SET `ScriptName`='boss_loatheb' WHERE `entry`=16011;
UPDATE `creature_template` SET `ScriptName`='boss_razuvious' WHERE `entry`=16061;
UPDATE `creature_template` SET `ScriptName`='boss_gothik' WHERE `entry`=16060;
UPDATE `creature_template` SET `ScriptName`='boss_highlord_mograine' WHERE `entry`=16062;
UPDATE `creature_template` SET `ScriptName`='boss_thane_korthazz' WHERE `entry`=16064;
UPDATE `creature_template` SET `ScriptName`='boss_sir_zeliek' WHERE `entry`=16063;
UPDATE `creature_template` SET `ScriptName`='boss_lady_blaumeux' WHERE `entry`=16065;
UPDATE `creature_template` SET `ScriptName`='boss_patchwerk' WHERE `entry`=16028;
UPDATE `creature_template` SET `ScriptName`='boss_grobbulus' WHERE `entry`=15931;
UPDATE `creature_template` SET `ScriptName`='boss_gluth' WHERE `entry`=15932;
UPDATE `creature_template` SET `ScriptName`='boss_thaddius' WHERE `entry`=15928;
UPDATE `creature_template` SET `ScriptName`='boss_stalagg' WHERE `entry`=15929;
UPDATE `creature_template` SET `ScriptName`='boss_fugen' WHERE `entry`=15930;
UPDATE `creature_template` SET `ScriptName`='boss_sapphiron' WHERE `entry`=15989;
UPDATE `creature_template` SET `ScriptName`='boss_kelthuzad' WHERE `entry`=15990;

/* NETHERSTORM */
UPDATE `gameobject_template` SET `ScriptName`='go_manaforge_control_console' WHERE `entry` IN (183770,183956,184311,184312);
UPDATE `creature_template` SET `ScriptName`='npc_manaforge_control_console' WHERE `entry` IN (20209,20417,20418,20440);
UPDATE `creature_template` SET `ScriptName`='npc_protectorate_nether_drake' WHERE `entry`=20903;
UPDATE `creature_template` SET `ScriptName`='npc_veronia' WHERE `entry`=20162;
UPDATE `creature_template` SET `ScriptName`='mob_phase_hunter' WHERE `entry` =18879;
UPDATE `creature_template` SET `ScriptName`='npc_professor_dabiri' where entry=20907;
UPDATE `creature_template` SET `ScriptName`='npc_bessy' WHERE entry = 20415;

/* ONYXIA'S LAIR */
UPDATE `creature_template` SET `ScriptName`='boss_onyxia' WHERE `entry`=10184;

/* ORGRIMMAR */
UPDATE `creature_template` SET `ScriptName`='npc_neeru_fireblade' WHERE `entry`=3216;
UPDATE `creature_template` SET `ScriptName`='npc_shenthul' WHERE `entry`=3401;
UPDATE `creature_template` SET `ScriptName`='npc_thrall_warchief' WHERE `entry`=4949;

/* RAGEFIRE CHASM */


/* RAZORFEN DOWNS */
UPDATE `creature_template` SET `ScriptName`='boss_amnennar_the_coldbringer' WHERE `entry`=7358;

/* RAZORFEN KRAUL */
UPDATE `creature_template` SET `Scriptname`='npc_willix' WHERE entry=4508;
/* REDRIDGE MOUNTAINS */


/* RUINS OF AHN'QIRAJ */
UPDATE `instance_template` SET `script`='instance_ruins_of_ahnqiraj' WHERE `map`=509;

/* SCARLET MONASTERY */
UPDATE `instance_template` SET `script`='instance_scarlet_monastery' WHERE `map`=189;
UPDATE `creature_template` SET `ScriptName`='boss_arcanist_doan' WHERE `entry`=6487;
UPDATE `creature_template` SET `ScriptName`='boss_azshir_the_sleepless' WHERE `entry`=6490;
UPDATE `creature_template` SET `ScriptName`='boss_bloodmage_thalnos' WHERE `entry`=4543;
UPDATE `creature_template` SET `ScriptName`='boss_herod' WHERE `entry`=3975;
UPDATE `creature_template` SET `ScriptName`='boss_high_inquisitor_fairbanks' WHERE `entry`=4542;
UPDATE `creature_template` SET `ScriptName`='boss_high_inquisitor_whitemane' WHERE `entry`=3977;
UPDATE `creature_template` SET `ScriptName`='boss_houndmaster_loksey' WHERE `entry`=3974;
UPDATE `creature_template` SET `ScriptName`='boss_interrogator_vishas' WHERE `entry`=3983;
UPDATE `creature_template` SET `ScriptName`='boss_scarlet_commander_mograine' WHERE `entry`=3976;
UPDATE `creature_template` SET `ScriptName`='boss_scorn' WHERE `entry`=14693;
UPDATE `creature_template` SET `ScriptName`='boss_headless_horseman' WHERE `entry` = 23682;
UPDATE `creature_template` SET `ScriptName`='mob_head' WHERE `entry` = 23775;
UPDATE `creature_template` SET `ScriptName`='mob_pulsing_pumpkin' WHERE `entry` = 23694;
UPDATE `creature_template` SET `ScriptName`='mob_wisp_invis' WHERE `entry`=23686;
UPDATE `creature_template` SET `ScriptName`='mob_wisp_invis' WHERE `entry`=24034;
UPDATE `creature_template` SET `ScriptName`='mob_scarlet_trainee' WHERE `entry`=6575;
UPDATE `gameobject_template` SET `ScriptName`='go_loosely_turned_soil' WHERE `entry`=186314;

/* SCHOLOMANCE */
UPDATE `instance_template` SET `script`='instance_scholomance' WHERE `map`=289;
UPDATE `creature_template` SET `ScriptName`='boss_darkmaster_gandling' WHERE `entry`=1853;
UPDATE `creature_template` SET `ScriptName`='boss_death_knight_darkreaver' WHERE `entry`=14516;
UPDATE `creature_template` SET `ScriptName`='boss_lord_alexei_barov' WHERE `entry`=10504;
UPDATE `creature_template` SET `ScriptName`='boss_instructor_malicia' WHERE `entry`=10505;
UPDATE `creature_template` SET `ScriptName`='boss_boss_ras_frostwhisper' WHERE `entry`=10508;
UPDATE `creature_template` SET `ScriptName`='boss_the_ravenian' WHERE `entry`=10507;
UPDATE `creature_template` SET `ScriptName`='boss_vectus' WHERE `entry`=10432;
UPDATE `creature_template` SET `ScriptName`='boss_illucia_barov' WHERE `entry`=10502;
UPDATE `creature_template` SET `ScriptName`='boss_doctor_theolen_krastinov' WHERE `entry`=11261;
UPDATE `creature_template` SET `ScriptName`='boss_jandice_barov' WHERE `entry`=10503;
UPDATE `creature_template` SET `ScriptName`='boss_lorekeeper_polkelt' WHERE `entry`=10901;
UPDATE `creature_template` SET `ScriptName`='boss_kormok' WHERE `entry`=16118;
UPDATE `creature_template` SET `ScriptName`='mob_illusionofjandicebarov' WHERE `entry`=11439;

/* SEARING GORGE */
UPDATE `creature_template` SET `ScriptName`='npc_kalaran_windblade' WHERE `entry`=8479;
UPDATE `creature_template` SET `ScriptName`='npc_lothos_riftwaker' WHERE `entry`=14387;
UPDATE `creature_template` SET `ScriptName`='npc_zamael_lunthistle' WHERE `entry`=8436;

/* SHADOWFANG KEEP */
UPDATE `instance_template` SET `script`='instance_shadowfang_keep' WHERE `map`=33;
UPDATE `creature_template` SET `ScriptName`='npc_shadowfang_prisoner' WHERE `entry` IN (3849,3850);

/* SHADOWMOON VALLEY */
UPDATE `creature_template` SET `ScriptName`='boss_doomwalker' WHERE `entry`=17711;
UPDATE `creature_template` SET `ScriptName`='npc_drake_dealer_hurlunk' WHERE `entry`=23489;
UPDATE `creature_template` SET `ScriptName`='npc_invis_legion_teleporter' WHERE `entry`=21807;
UPDATE `creature_template` SET `ScriptName`='npcs_flanis_swiftwing_and_kagrosh' WHERE `entry` IN (21725,21727);
UPDATE `creature_template` SET `ScriptName`='npc_murkblood_overseer' WHERE `entry`=23309;
UPDATE `creature_template` SET `ScriptName`='npc_neltharaku' WHERE `entry`=21657;
UPDATE `creature_template` SET `ScriptName`='npc_oronok_tornheart' WHERE `entry`=21183;
UPDATE `creature_template` SET `ScriptName`='mob_mature_netherwing_drake' WHERE `entry`=21648;
UPDATE `creature_template` SET `ScriptName`='mob_enslaved_netherwing_drake' WHERE `entry`=21722;
UPDATE `creature_template` SET `ScriptName`='npc_overlord_morghor' WHERE entry=	23139;
UPDATE `creature_template` SET `ScriptName`='mob_illidari_spawn' WHERE `entry` IN (22075, 22074, 19797);
UPDATE `creature_template` SET `ScriptName`='mob_torloth_the_magnificent' WHERE `entry`='22076';
UPDATE `creature_template` SET `ScriptName`='npc_lord_illidan_stormrage' WHERE `entry`='22083';
UPDATE `creature_template` SET `ScriptName`='npc_earthmender_wilda' WHERE `entry`='21027';
UPDATE `creature_template` SET `ScriptName`='npc_enraged_spirit' WHERE `entry` IN (21050, 21061, 21060, 21059);

/* SHATTRATH */
UPDATE `creature_template` SET `ScriptName`='npc_raliq_the_drunk' WHERE `entry`='18585';
UPDATE `creature_template` SET `ScriptName`='npc_salsalabim' WHERE `entry`='18584';
UPDATE `creature_template` SET `ScriptName`='npc_shattrathflaskvendors' WHERE `entry` IN (23483,23484);
UPDATE `creature_template` SET `ScriptName`='npc_zephyr' WHERE `entry`='25967';
UPDATE `creature_template` SET `ScriptName`='npc_kservant' WHERE `entry`='19685';
UPDATE `creature_template` SET `ScriptName`='npc_dirty_larry' WHERE `entry`='19720';
UPDATE `creature_template` SET `ScriptName`='npc_ishanah' WHERE `entry`=18538;
UPDATE `creature_template` SET `ScriptName`='npc_khadgar' where entry=18166;

/* SILITHUS */
UPDATE `creature_template` SET `ScriptName`='npcs_rutgar_and_frankal' WHERE `entry` IN (15170,15171);
UPDATE `creature_template` SET `ScriptName`='npc_highlord_demitrian' WHERE `entry`=14347;
UPDATE `gameobject_template` SET `ScriptName`='go_gauntlet_gate' WHERE `entry`=175357;

/* SILVERMOON */
UPDATE `creature_template` SET `ScriptName`='npc_blood_knight_stillblade' WHERE `entry`=17768;

/* SILVERPINE FOREST */
UPDATE `creature_template` SET `ScriptName`='npc_astor_hadren' WHERE `entry`=6497;
UPDATE `creature_template` SET `ScriptName`='npc_deathstalker_erland' WHERE `entry`=1978;

/* STOCKADES */

/* STONETALON MOUNTAINS */
UPDATE `creature_template` SET `ScriptName`='npc_braug_dimspirit' WHERE `entry`=4489;
UPDATE `creature_template` SET `ScriptName`='npc_kaya_flathoof' WHERE `entry`=11856;

/* STORMWIND CITY */
UPDATE `creature_template` SET `ScriptName`='npc_archmage_malin' WHERE `entry`=2708;
UPDATE `creature_template` SET `ScriptName`='npc_bartleby' WHERE `entry`=6090;
UPDATE `creature_template` SET `ScriptName`='npc_dashel_stonefist' WHERE `entry`=4961;
UPDATE `creature_template` SET `ScriptName`='npc_general_marcus_jonathan' WHERE `entry`=466;
UPDATE `creature_template` SET `ScriptName`='npc_lady_katrana_prestor' WHERE `entry`=1749;

/* STRANGLETHORN VALE */
UPDATE `creature_template` SET `ScriptName`='mob_yenniku' WHERE `entry`=2530;

/* STRATHOLME */
UPDATE `instance_template` SET `script`='instance_stratholme' WHERE `map`=329;
UPDATE `creature_template` SET `ScriptName`='boss_dathrohan_balnazzar' WHERE `entry`=10812;
UPDATE `creature_template` SET `ScriptName`='boss_magistrate_barthilas' WHERE `entry`=10435;
UPDATE `creature_template` SET `ScriptName`='boss_maleki_the_pallid' WHERE `entry`=10438;
UPDATE `creature_template` SET `ScriptName`='boss_nerubenkan' WHERE `entry`=10437;
UPDATE `creature_template` SET `ScriptName`='boss_cannon_master_willey' WHERE `entry`=10997;
UPDATE `creature_template` SET `ScriptName`='boss_baroness_anastari' WHERE `entry`=10436;
UPDATE `creature_template` SET `ScriptName`='boss_ramstein_the_gorger' WHERE `entry`=10439;
UPDATE `creature_template` SET `ScriptName`='boss_timmy_the_cruel' WHERE `entry`=10808;
UPDATE `creature_template` SET `ScriptName`='boss_silver_hand_bosses' WHERE `entry` IN (17910,17911,17912,17913,17914);
UPDATE `creature_template` SET `ScriptName`='boss_postmaster_malown' WHERE `entry`=11143;
UPDATE `creature_template` SET `ScriptName`='boss_baron_rivendare' WHERE `entry`=10440;
UPDATE `creature_template` SET `ScriptName`='mob_mindless_skeleton' WHERE `entry`=11197;
UPDATE `creature_template` SET `ScriptName`='mob_thuzadin_acolyte' WHERE `entry`=10399;
UPDATE `creature_template` SET `ScriptName`='mobs_spectral_ghostly_citizen' WHERE `entry` IN (10384,10385);
UPDATE `creature_template` SET `ScriptName`='mob_restless_soul' WHERE `entry`=11122;
UPDATE `creature_template` SET `ScriptName`='mob_freed_soul' WHERE `entry`=11136;

/* SUNKEN TEMPLE */

/* SUNWELL PLATEAU */
UPDATE `instance_template` SET `script`='instance_sunwell_plateau' WHERE `map`=580;
UPDATE `creature_template` SET `ScriptName`='boss_brutallus' WHERE `entry`=24882;
UPDATE `creature_template` SET `ScriptName`='boss_felmyst' WHERE `entry`=25038;
UPDATE `creature_template` SET `ScriptName`='mob_felmyst_vapor' WHERE `entry`=25265;
UPDATE `creature_template` SET `ScriptName`='mob_felmyst_trail' WHERE `entry`=25267;
UPDATE `creature_template` SET `ScriptName`='boss_sacrolash' WHERE `entry`=25165;
UPDATE `creature_template` SET `ScriptName`='boss_alythess' WHERE `entry`=25166;
UPDATE `creature_template` SET `ScriptName`='mob_shadow_image' WHERE `entry`=25214;
UPDATE `creature_template` SET `ScriptName`='boss_kiljaeden' WHERE `entry`=25315;
UPDATE `creature_template` SET `ScriptName`='boss_kalecgos_kj' WHERE `entry`=25319;
UPDATE `creature_template` SET `ScriptName`='mob_kiljaeden_controller' WHERE `entry`=25608;
UPDATE `creature_template` SET `ScriptName`='mob_hand_of_the_deceiver' WHERE `entry`=25588;
UPDATE `creature_template` SET `ScriptName`='mob_felfire_portal' WHERE `entry`=25603;
UPDATE `creature_template` SET `ScriptName`='mob_volatile_felfire_fiend' WHERE `entry`=25598;
UPDATE `creature_template` SET `ScriptName`='mob_armageddon' WHERE `entry`=25735;
UPDATE `creature_template` SET `ScriptName`='mob_shield_orb' WHERE `entry`=25502;
UPDATE `creature_template` SET `ScriptName`='mob_sinster_reflection' WHERE `entry`=25708;
UPDATE `gameobject_template` SET `ScriptName`='go_orb_of_the_blue_flight' WHERE `entry`=188415;
UPDATE `creature_template` SET `ScriptName`='npc_void_sentinel' WHERE `entry`=25772;
UPDATE `creature_template` SET `ScriptName`='npc_dark_fiend' WHERE `entry`=25744;
UPDATE `creature_template` SET `ScriptName`='boss_muru' WHERE `entry`=25741;
UPDATE `creature_template` SET `ScriptName`='boss_entropius' WHERE `entry`=25840;
UPDATE `creature_template` SET `ScriptName`='npc_muru_portal' WHERE `entry`=25770;
UPDATE `creature_template` SET `ScriptName`='boss_kalecgos' WHERE `entry`=24850;
UPDATE `creature_template` SET `ScriptName`='boss_sathrovarr' WHERE `entry`=24892;
UPDATE `creature_template` SET `ScriptName`='boss_kalec' WHERE `entry`=24891;
UPDATE `gameobject_template` SET `ScriptName`='kalecgos_teleporter' WHERE `entry`=187055;
UPDATE `creature_template` SET `ScriptName`='npc_blackhole' WHERE `entry`=25855;

/* SWAMP OF SORROWS */


/* TANARIS */
UPDATE `creature_template` SET `ScriptName`='mob_aquementas' WHERE `entry`=9453;
UPDATE `creature_template` SET `ScriptName`='npc_custodian_of_time' WHERE `entry`=20129;
UPDATE `creature_template` SET `ScriptName`='npc_marin_noggenfogger' WHERE `entry`=7564;
UPDATE `creature_template` SET `ScriptName`='npc_steward_of_time' WHERE `entry`=20142;
UPDATE `creature_template` SET `ScriptName`='npc_stone_watcher_of_norgannon' WHERE `entry`=7918;
UPDATE `creature_template` SET `ScriptName` = 'npc_OOX17' WHERE `entry` = 7784;

/* TELDRASSIL */


/*  */
/* TEMPEST KEEP */
/*  */

/* THE MECHANAR */
UPDATE `creature_template` SET `ScriptName`='boss_gatewatcher_iron_hand' WHERE `entry`=19710;
UPDATE `creature_template` SET `ScriptName`='boss_nethermancer_sepethrea' WHERE `entry`=19221;
UPDATE `creature_template` SET `ScriptName`='mob_ragin_flames' WHERE `entry`=20481;
UPDATE `creature_template` SET `ScriptName`='boss_pathaleon_the_calculator' WHERE `entry`=19220;
UPDATE `creature_template` SET `ScriptName`='mob_nether_wraith' WHERE `entry`=21062;
UPDATE `instance_template` SET `script`='instance_mechanar' WHERE `map`=554;

/* THE BOTANICA */
UPDATE `creature_template` SET `ScriptName`='boss_high_botanist_freywinn' WHERE `entry`=17975;
UPDATE `creature_template` SET `ScriptName`='boss_laj' WHERE `entry`=17980;
UPDATE `creature_template` SET `ScriptName`='boss_warp_splinter' WHERE `entry`=17977;
UPDATE `creature_template` SET `ScriptName`='mob_warp_splinter_treant' WHERE `entry`=19949;

/* THE ARCATRAZ */
UPDATE `instance_template` SET `script`='instance_arcatraz' WHERE `map`=552;
UPDATE `creature_template` SET `ScriptName`='mob_zerekethvoidzone' WHERE `entry`=21101;
UPDATE `creature_template` SET `ScriptName`='boss_harbinger_skyriss' WHERE `entry`=20912;
UPDATE `creature_template` SET `ScriptName`='boss_harbinger_skyriss_illusion' WHERE `entry` IN (21466,21467);
UPDATE `creature_template` SET `ScriptName`='npc_warden_mellichar' WHERE `entry`=20904;
UPDATE `creature_template` SET `ScriptName`='npc_millhouse_manastorm' WHERE `entry`=20977;

/* THE EYE */
UPDATE `instance_template` SET `script`='instance_the_eye' WHERE `map`=550;
UPDATE `gameobject_template` SET `ScriptName` = 'go_kael_orb' WHERE `entry` = 188173;
UPDATE `gameobject_template` SET `ScriptName` = 'go_movie_orb' WHERE `entry` = 187578;
/* The Eye Trash Mobs */
UPDATE `creature_template` SET `ScriptName`='mob_crystalcore_devastator' WHERE `entry`=20040;
/* Void Reaver event */
UPDATE `creature_template` SET `ScriptName`='boss_void_reaver' WHERE `entry`=19516;
/* Astromancer event */
UPDATE `creature_template` SET `ScriptName`='boss_high_astromancer_solarian' WHERE `entry`=18805;
UPDATE `creature_template` SET `ScriptName`='mob_solarium_priest' WHERE `entry`=18806;
/* Kael'thas event */
UPDATE `creature_template` SET `ScriptName`='boss_kaelthas' WHERE `entry`=19622;
UPDATE `creature_template` SET `ScriptName`='boss_thaladred_the_darkener' WHERE `entry`=20064;
UPDATE `creature_template` SET `ScriptName`='boss_lord_sanguinar' WHERE `entry`=20060;
UPDATE `creature_template` SET `ScriptName`='boss_grand_astromancer_capernian' WHERE `entry`=20062;
UPDATE `creature_template` SET `ScriptName`='boss_master_engineer_telonicus' WHERE `entry`=20063;
UPDATE `creature_template` SET `ScriptName`='mob_phoenix_tk' WHERE `entry`=21362;
UPDATE `creature_template` SET `ScriptName`='mob_phoenix_egg_tk' WHERE `entry`=21364;
UPDATE `creature_template` SET `ScriptName`='mob_nether_vapor' WHERE `entry`=21002;
UPDATE `creature_template` SET `ScriptName`='mob_kael_flamestrike' WHERE `entry`=21369;
update `creature_template` set `ScriptName`='boss_alar' where `entry`=19514;
update `creature_template` set `ScriptName`='mob_ember_of_alar' where `entry`=19551;
update `creature_template` set `ScriptName`='mob_flame_patch_alar' where `entry`=20602;

/* TEMPLE OF AHN'QIRAJ */
UPDATE `instance_template` SET `script`='instance_temple_of_ahnqiraj' WHERE `map`=531;
UPDATE `creature_template` SET `ScriptName`='boss_cthun' WHERE `entry`=15727;
UPDATE `creature_template` SET `ScriptName`='boss_skeram' WHERE `entry`=15263;
UPDATE `creature_template` SET `ScriptName`='boss_vem' WHERE `entry`=15544;
UPDATE `creature_template` SET `ScriptName`='boss_yauj' WHERE `entry`=15543;
UPDATE `creature_template` SET `ScriptName`='boss_kri' WHERE `entry`=15511;
UPDATE `creature_template` SET `ScriptName`='boss_sartura' WHERE `entry`=15516;
UPDATE `creature_template` SET `ScriptName`='boss_fankriss' WHERE `entry`=15510;
UPDATE `creature_template` SET `ScriptName`='boss_viscidus' WHERE `entry`=15299;
UPDATE `creature_template` SET `ScriptName`='boss_glob_of_viscidus' WHERE `entry`=15667;
UPDATE `creature_template` SET `ScriptName`='boss_huhuran' WHERE `entry`=15509;
UPDATE `creature_template` SET `ScriptName`='boss_veklor' WHERE `entry`=15276;
UPDATE `creature_template` SET `ScriptName`='boss_veknilash' WHERE `entry`=15275;
UPDATE `creature_template` SET `ScriptName`='boss_ouro' WHERE `entry`=15517;
UPDATE `creature_template` SET `ScriptName`='boss_eye_of_cthun' WHERE `entry`=15589;
UPDATE `creature_template` SET `ScriptName`='mob_sartura_royal_guard' WHERE `entry`=15984;
UPDATE `creature_template` SET `ScriptName`='mob_yauj_brood' WHERE `entry`=15621;
UPDATE `creature_template` SET `ScriptName`='mob_claw_tentacle' WHERE `entry`=15725;
UPDATE `creature_template` SET `ScriptName`='mob_eye_tentacle' WHERE `entry`=15726;
UPDATE `creature_template` SET `ScriptName`='mob_giant_claw_tentacle' WHERE `entry`=15728;
UPDATE `creature_template` SET `ScriptName`='mob_giant_eye_tentacle' WHERE `entry`=15334;
UPDATE `creature_template` SET `ScriptName`='mob_giant_flesh_tentacle' WHERE `entry`=15802;
UPDATE `creature_template` SET `ScriptName`='mob_anubisath_sentinel' WHERE `entry`=15264;

/* TEROKKAR FOREST */
UPDATE `creature_template` SET `ScriptName`='mob_infested_root_walker' WHERE `entry`=22095;
UPDATE `creature_template` SET `ScriptName`='mob_netherweb_victim' WHERE `entry`=22355;
UPDATE `creature_template` SET `ScriptName`='mob_rotting_forest_rager' WHERE `entry`=22307;
UPDATE `creature_template` SET `ScriptName`='npc_floon' WHERE `entry`=18588;
UPDATE `creature_template` SET `ScriptName`='npc_skyguard_handler_irena' WHERE `entry`=23413;
UPDATE `creature_template` SET `ScriptName`='npc_isla_starmane' where entry=18760;
UPDATE `creature_template` SET `ScriptName` = 'mob_unkor_the_ruthless' WHERE `entry` = 18262;

/* THOUSAND NEEDLES */
UPDATE `creature_template` SET `ScriptName` = 'npc_swiftmountain' WHERE `entry` = 10427;
UPDATE `creature_template` SET `ScriptName` = 'npc_plucky' WHERE `entry` = '6626';

/* THUNDER BLUFF */
UPDATE `creature_template` SET `ScriptName`='npc_cairne_bloodhoof' WHERE `entry`=3057;

/* TIRISFAL GLADES */
UPDATE `creature_template` SET `ScriptName`='npc_calvin_montague' WHERE `entry`=6784;

/* ULDAMAN */
UPDATE `instance_template` SET `script`='instance_uldaman' WHERE `map`=70;
UPDATE `creature_template` SET `ScriptName`='boss_ironaya' WHERE `entry`=7228;
UPDATE `creature_template` SET `ScriptName`='mob_jadespine_basilisk' WHERE `entry`=4863;
UPDATE `creature_template` SET `ScriptName`='npc_lore_keeper_of_norgannon' WHERE `entry`=7172;
UPDATE `creature_template` SET `ScriptName`='boss_archaedas' WHERE `entry`=2748;
UPDATE `creature_template` SET `ScriptName`='mob_archaedas_minions' WHERE `entry` IN ('7309', '7077', '7076', '10120');
UPDATE `creature_template` SET `ScriptName`='mob_stonekeepers' WHERE `entry`=4857;
UPDATE `gameobject_template` SET `ScriptName`='go_altar_of_the_keepers' WHERE `entry`=130511;
UPDATE `gameobject_template` SET `ScriptName`='go_altar_of_archaedas' WHERE `entry`=133234;

/* UN'GORO CRATER */
UPDATE `creature_template` SET `ScriptName` = 'npc_ame' WHERE `entry` = 9623;

/* UNDERCITY */
UPDATE `creature_template` SET `ScriptName`='npc_lady_sylvanas_windrunner' WHERE `entry`=10181;
UPDATE `creature_template` SET `ScriptName`='npc_highborne_lamenter' WHERE `entry`=21628;
UPDATE `creature_template` SET `ScriptName`='npc_parqual_fintallas' WHERE `entry`=4488;

/* WAILING CAVERNS */


/* WESTERN PLAGUELANDS */
UPDATE `creature_template` SET `ScriptName`='npcs_dithers_and_arbington' WHERE `entry` IN (11056,11057);
UPDATE `creature_template` SET `ScriptName`='npc_the_scourge_cauldron' WHERE `entry`=11152;

/* WESTFALL */
UPDATE `creature_template` SET `ScriptName`='npc_defias_traitor' WHERE `entry`='467';

/* WETLANDS */


/* WINTERSPRING */
UPDATE `creature_template` SET `ScriptName`='npc_lorax' WHERE `entry`=10918;
UPDATE `creature_template` SET `ScriptName`='npc_rivern_frostwind' WHERE `entry`=10618;
UPDATE `creature_template` SET `ScriptName`='npc_witch_doctor_mauari' WHERE `entry`=10307;

/* ZANGARMARSH */
UPDATE `creature_template` SET `ScriptName`='npcs_ashyen_and_keleth' WHERE `entry` IN (17900,17901);
UPDATE `creature_template` SET `ScriptName`='npc_cooshcoosh' WHERE `entry`=18586;
UPDATE `creature_template` SET `ScriptName`='npc_elder_kuruti' WHERE `entry`=18197;
UPDATE `creature_template` SET `ScriptName`='npc_mortog_steamhead' WHERE `entry`=23373;
UPDATE `creature_template` SET `ScriptName`='npc_kayra_longmane' where entry=17969;

/* ZUL'AMAN */
UPDATE `instance_template` SET `script` = 'instance_zulaman' WHERE `map`=568;
UPDATE `creature_template` SET `ScriptName` = 'boss_janalai' WHERE `entry`=23578;
UPDATE `creature_template` SET `ScriptName` = 'boss_nalorakk' WHERE `entry`=23576;
UPDATE `creature_template` SET `ScriptName` = 'mob_janalai_firebomb' WHERE `entry`=23920;
UPDATE `creature_template` SET `ScriptName` = 'mob_janalai_hatcher' WHERE `entry`=23818;
UPDATE `creature_template` SET `ScriptName` = 'mob_janalai_hatchling' WHERE `entry`=23598;
UPDATE `creature_template` SET `ScriptName` = 'mob_janalai_egg' WHERE `entry`=23817;
UPDATE `creature_template` SET `ScriptName` = 'npc_forest_frog' WHERE `entry`=24396;
UPDATE `creature_template` SET `ScriptName` = 'boss_akilzon' where entry = 23574;
UPDATE `creature_template` SET `ScriptName` = 'mob_akilzon_eagle' where `entry`=24858;
UPDATE `creature_template` SET `ScriptName` = 'boss_halazzi' WHERE `entry` = '23577';
UPDATE `creature_template` SET `ScriptName` = 'mob_halazzi_lynx' WHERE `entry` = '24143';
UPDATE `creature_template` SET `ScriptName` = 'boss_hexlord_malacrass' WHERE `entry` =24239;
UPDATE `creature_template` SET `ScriptName` = 'boss_alyson_antille' WHERE `entry` =24240;
UPDATE `creature_template` SET `ScriptName` = 'boss_thurg' WHERE `entry` =24241;
UPDATE `creature_template` SET `ScriptName` = 'boss_slither' WHERE `entry` =24242;
UPDATE `creature_template` SET `ScriptName` = 'boss_lord_raadan' WHERE `entry` =24243;
UPDATE `creature_template` SET `ScriptName` = 'boss_gazakroth' WHERE `entry` =24244;
UPDATE `creature_template` SET `ScriptName` = 'boss_fenstalker' WHERE `entry` =24245;
UPDATE `creature_template` SET `ScriptName` = 'boss_darkheart' WHERE `entry` =24246;
UPDATE `creature_template` SET `ScriptName` = 'boss_koragg' WHERE `entry` =24247;
UPDATE `creature_template` SET `ScriptName` = 'boss_zuljin' WHERE `entry` =23863;
UPDATE `creature_template` SET `ScriptName` = 'do_nothing' WHERE `entry` = '24187';
UPDATE `creature_template` SET `ScriptName` = 'mob_zuljin_vortex' WHERE `entry` = '24136';
UPDATE `creature_template` SET `ScriptName`='npc_zulaman_hostage' WHERE entry IN ('24001','23790','23999','24024');

/* ZUL'FARRAK */
UPDATE `creature_template` SET `ScriptName`='npc_sergeant_bly' WHERE `entry`=7604;
UPDATE `creature_template` SET `ScriptName`='npc_weegli_blastfuse' WHERE `entry`=7607;

/* ZUL'GURUB */
UPDATE `instance_template` SET `script`='instance_zulgurub' WHERE `map`=309;
UPDATE `creature_template` SET `ScriptName`='boss_jeklik' WHERE `entry`=14517;
UPDATE `creature_template` SET `ScriptName`='boss_venoxis' WHERE `entry`=14507;
UPDATE `creature_template` SET `ScriptName`='boss_marli' WHERE `entry`=14510;
UPDATE `creature_template` SET `ScriptName`='boss_mandokir' WHERE `entry`=11382;
UPDATE `creature_template` SET `ScriptName`='boss_gahzranka' WHERE `entry`=15114;
UPDATE `creature_template` SET `ScriptName`='boss_jindo' WHERE `entry`=11380;
UPDATE `creature_template` SET `ScriptName`='boss_hakkar' WHERE `entry`=14834;
UPDATE `creature_template` SET `ScriptName`='boss_thekal' WHERE `entry`=14509;
UPDATE `creature_template` SET `ScriptName`='boss_arlokk' WHERE `entry`=14515;
UPDATE `creature_template` SET `ScriptName`='boss_grilek' WHERE `entry`=15082;
UPDATE `creature_template` SET `ScriptName`='boss_hazzarah' WHERE `entry`=15083;
UPDATE `creature_template` SET `ScriptName`='boss_renataki' WHERE `entry`=15084;
UPDATE `creature_template` SET `ScriptName`='boss_wushoolay' WHERE `entry`=15085;
UPDATE `creature_template` SET `ScriptName`='mob_zealot_lorkhan' WHERE `entry`=11347;
UPDATE `creature_template` SET `ScriptName`='mob_zealot_zath' WHERE `entry`=11348;
UPDATE `creature_template` SET `ScriptName`='mob_healing_ward' WHERE `entry`=14987;
UPDATE `creature_template` SET `ScriptName`='mob_spawn_of_marli' WHERE `entry`=15041;
UPDATE `creature_template` SET `ScriptName`='mob_batrider' WHERE `entry`=14965;
UPDATE `creature_template` SET `ScriptName`='mob_shade_of_jindo' WHERE `entry`=14986;

/* EOF */

/* to do: find correct section */
UPDATE `instance_template` SET `script` = 'instance_blackrock_depths' WHERE `map` = 230;
UPDATE `creature_template` SET `ScriptName` = 'mob_voidtraveler' WHERE `entry` = 19226;
UPDATE `creature_template` SET `ScriptName` = 'mob_shadowy_construct' WHERE `entry` = 23111;
UPDATE `creature_template` SET `ScriptName` = 'mob_phalanx' WHERE `entry` = 9502;
UPDATE `creature_template` SET `ScriptName` = 'npc_draenei_survivor' WHERE `entry` = 16483;
UPDATE `creature_template` SET `ScriptName` = 'mob_inner_demon' WHERE entry = 21857;
UPDATE `creature_template` SET `ScriptName` = 'mob_webwrap' WHERE `entry` = 16486;
UPDATE `creature_template` SET `ScriptName` = 'mob_dragonmaw_peon' WHERE `entry` = 22252;
UPDATE `creature_template` SET `ScriptName` = 'npc_karynaku' WHERE `entry` = 22112;
UPDATE `creature_template` SET `ScriptName` = 'mob_ohgan' WHERE `entry` = 14988;
