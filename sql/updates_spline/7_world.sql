DELETE FROM  `spell_linked_spell` WHERE `spell_trigger` = 45856;
INSERT INTO `spell_linked_spell` VALUES (45856, 49725, 0, 'Breath: Haste');

DELETE FROM `creature_scripts` WHERE `entryorguid` = 25653;

UPDATE `creature_template` SET `minmana` = '3387', `maxmana` = '3387' WHERE `entry` = 25038;
