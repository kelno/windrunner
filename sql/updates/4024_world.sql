UPDATE `creature_template` SET `minhealth` = '125460' WHERE `entry` = '21504';
UPDATE `creature_template` SET `maxhealth` = '125460' WHERE `entry` = '21504';
UPDATE `creature_template` SET `ScriptName` = 'npc_pathaleon_image' WHERE `entry` = '21504';

DELETE FROM `event_scripts` WHERE `id` = '13256';
INSERT INTO `event_scripts` VALUES (13256, 0, 10, 21504, 240000, 0, 129.02, 4833.29, 76.2765, 0);
INSERT INTO `script_texts` (`entry`,`content_default`,`content_loc1`,`content_loc2`,`content_loc3`,`content_loc4`,`content_loc5`,`content_loc6`,`content_loc7`,`content_loc8`,`sound`,`type`,`language`,`emote`,`comment`) VALUES
 (-1900165,'And just what do you think you are doing? You dare to interfere with my master''s experiment?',NULL,'Et que croyez-vous être en train de faire ? Vous osez interférer avec les expériences de mon maître ?',NULL,NULL,NULL,NULL,NULL,NULL,0,0,0,0,'PATHALEON1'),
 (-1900166,'Do you like what we''ve done here? Perhaps we will drop these crystals from the sky all over Outland.',NULL,'Vous appréciez notre travail ici ? Peut-être ferons-nous tomber ces cristaux du ciel sur toute l''Outreterre.',NULL,NULL,NULL,NULL,NULL,NULL,0,0,0,0,'PATHALEON2'),
 (-1900167,'I grow bored with your attempt to heal the land and quell the energies summoning and driving the colossi mad. Goliathon, King of the Colossi, Prince Kael''thas and I demand that you defend the crystal!',NULL,'Vos tentatives de guérir cette terre et d''endiguer les énergies qui invoquent les colosses et les rendent fous commencent à m''ennuyer profondément. Goliathon, roi des colosses, Kael''thas et moi-même t''ordonnons de défendre le cristal !',NULL,NULL,NULL,NULL,NULL,NULL,0,0,0,0,'PATHALEON3'),
 (-1900168,'We will meet again soon.',NULL,'Nous nous reverrons bientôt.',NULL,NULL,NULL,NULL,NULL,NULL,0,0,0,0,'PATHALEON4');
