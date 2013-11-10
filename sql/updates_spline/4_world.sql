UPDATE `creature_template` SET `AIName` = 'NullCreatureAI', `ScriptName` = '' WHERE `entry` = 25265;
UPDATE `creature_template` SET `AIName` = 'NullCreatureAI', `ScriptName` = '' WHERE `entry` = 25267;
UPDATE `creature_template` SET `AIName` = 'NullCreatureAI', `ScriptName` = '' WHERE `entry` = 25268;

DELETE FROM `creature_scripts` WHERE `entryorguid` = 25265;
INSERT INTO `creature_scripts` VALUES (25265, 'mob_felmyst_vapor');
DELETE FROM `creature_scripts` WHERE `entryorguid` = 25267;
INSERT INTO `creature_scripts` VALUES (25267, 'mob_felmyst_trail');
DELETE FROM `creature_scripts` WHERE `entryorguid` = 25268;
INSERT INTO `creature_scripts` VALUES (25268, 'mob_unyielding_dead');

DELETE FROM `spell_script_target` WHERE `entry` IN (45388, 45389);
INSERT INTO `spell_script_target` VALUES (45388, 1, 25038);
INSERT INTO `spell_script_target` VALUES (45389, 1, 25265);

DELETE FROM `creature` WHERE `id` = 25265;

