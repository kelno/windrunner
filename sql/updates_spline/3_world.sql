UPDATE `creature_template` SET `AIName` = 'NullCreatureAI', `ScriptName` = '' WHERE `entry` = 25038;
DELETE FROM `creature_scripts` WHERE `entryorguid` = 25038;
INSERT INTO `creature_scripts` VALUES (25038, 'boss_felmyst');

