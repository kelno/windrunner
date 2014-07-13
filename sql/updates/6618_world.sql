UPDATE quest_template SET SkillOrClass = -1488 WHERE entry IN (80014,80015);

ALTER TABLE pack58
	ADD COLUMN count mediumint(8) NOT NULL DEFAULT 1;
	
#armes shaman, guerrier dps
UPDATE pack58 SET count = 2 WHERE item IN (12621,13361) and class IN (1,7);

UPDATE pack58 SET item = 12929 where item = 12229;