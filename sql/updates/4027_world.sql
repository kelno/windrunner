UPDATE `creature_template` SET `npcflag` = '1' WHERE entry = '22258';
UPDATE `creature_template` SET `minhealth` = '9958' WHERE entry = '22258';
UPDATE `creature_template` SET `maxhealth` = '9958' WHERE entry = '22258';
UPDATE `creature_template` SET `minhealth` = '2987' WHERE entry = '22259';
UPDATE `creature_template` SET `maxhealth` = '3095' WHERE entry = '22259';
UPDATE `creature_template` SET `AIName` = '', ScriptName = 'EventAI' WHERE entry = '22273';
UPDATE `creature_template` SET `flags_extra` = '128' WHERE entry = '22267';
UPDATE `creature_template` SET `flags_extra` = '128' WHERE entry = '22260';
UPDATE `creature_template` SET `ScriptName` = 'npc_demoniac_scryer' WHERE entry = '22258';
UPDATE `creature_template` SET `ScriptName` = 'npc_magic_sucker_device_spawner' WHERE entry = '22260';

DELETE FROM `spell_script_target` WHERE `entry` = 38691;
INSERT INTO `spell_script_target` VALUES ('38691','1','22267');
INSERT INTO `spell_script_target` VALUES ('38691','1','22260');

DELETE FROM `eventai_scripts` WHERE `creature_id` = 22273;
INSERT INTO `eventai_scripts` VALUES ('2227301', '22273', '1', '0', '100', '0', '0', '0', '0', '0', '21', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', 'Fel Warden - Prevent Combat Movement on Spawn');
INSERT INTO `eventai_scripts` VALUES ('2227302', '22273', '4', '0', '100', '0', '0', '0', '0', '0', '11', '14034', '1', '0', '23', '1', '0', '0', '0', '0', '0', '0', 'Fel Warden - Cast Fireball and Set Phase 1 on Aggro');
INSERT INTO `eventai_scripts` VALUES ('2227303', '22273', '9', '13', '100', '1', '0', '40', '2400', '3800', '11', '14034', '1', '0', '0', '0', '0', '0', '0', '0', '0', '0', 'Fel Warden - Cast Fireball (Phase 1)');
INSERT INTO `eventai_scripts` VALUES ('2227304', '22273', '3', '13', '100', '0', '15', '0', '0', '0', '21', '1', '0', '0', '23', '1', '0', '0', '0', '0', '0', '0', 'Fel Warden - Start Combat Movement and Set Phase 2 when Mana is at 15% (Phase 1)');
INSERT INTO `eventai_scripts` VALUES ('2227305', '22273', '9', '13', '100', '1', '35', '80', '0', '0', '21', '1', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', 'Fel Warden - Start Combat Movement at 35 Yards (Phase 1)');
INSERT INTO `eventai_scripts` VALUES ('2227306', '22273', '9', '13', '100', '1', '5', '15', '0', '0', '21', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', 'Fel Warden - Prevent Combat Movement at 15 Yards (Phase 1)');
INSERT INTO `eventai_scripts` VALUES ('2227307', '22273', '9', '13', '100', '1', '0', '5', '0', '0', '21', '1', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', 'Fel Warden - Start Combat Movement Below 5 Yards');
INSERT INTO `eventai_scripts` VALUES ('2227308', '22273', '3', '11', '100', '1', '100', '30', '100', '100', '23', '-1', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', 'Fel Warden - Set Phase 1 when Mana is above 30% (Phase 2)');
INSERT INTO `eventai_scripts` VALUES ('2227309', '22273', '0', '0', '100', '1', '6000', '9000', '12000', '16000', '11', '11831', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', 'Fel Warden - Cast Frost Nova');
INSERT INTO `eventai_scripts` VALUES ('2227310', '22273', '2', '0', '100', '0', '15', '0', '0', '0', '22', '3', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', 'Fel Warden - Set Phase 3 at 15% HP');
INSERT INTO `eventai_scripts` VALUES ('2227311', '22273', '2', '7', '100', '0', '15', '0', '0', '0', '21', '1', '0', '0', '25', '0', '0', '0', '1', '-47', '0', '0', 'Fel Warden - Start Combat Movement and Flee at 15% HP (Phase 3)');
INSERT INTO `eventai_scripts` VALUES ('2227312', '22273', '7', '0', '100', '0', '0', '0', '0', '0', '22', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', 'Fel Warden - Set Phase to 0 on Evade');
