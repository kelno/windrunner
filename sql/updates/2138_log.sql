DROP TABLE IF EXISTS `arena_match`;
CREATE TABLE `arena_match` (
  `id` mediumint(8) unsigned NOT NULL auto_increment,
  `type` tinyint(3) unsigned NOT NULL default '0' COMMENT 'Arena type : 2,3 or 5',
  `team1` mediumint(8) unsigned NOT NULL default '0' COMMENT 'Team 1 GUID',
  `team2` mediumint(8) unsigned NOT NULL default '0' COMMENT 'Team 2 GUID',
  `start_time` timestamp NOT NULL default '0000-00-00 00:00:00' COMMENT 'Start time',
  `end_time` timestamp NOT NULL default '0000-00-00 00:00:00' COMMENT 'End time',
  `winner` mediumint(8) unsigned NOT NULL default '0' COMMENT 'Winning team GUID',
  `rating_change` tinyint(3) unsigned NOT NULL default '0' COMMENT 'Rating change for both teams',
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;
