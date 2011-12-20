-- Template ID
ALTER TABLE item_instance ADD COLUMN template mediumint(8) unsigned NOT NULL AFTER `data`;
UPDATE item_instance SET template = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 3))+2, length(SUBSTRING_INDEX(data, ' ', 3+1))- length(SUBSTRING_INDEX(data, ' ', 3)) - 1));

-- Container GUID
ALTER TABLE item_instance ADD COLUMN container_guid int(10) unsigned NOT NULL after template;
UPDATE item_instance SET container_guid = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 8))+2, length(SUBSTRING_INDEX(data, ' ', 8+1))- length(SUBSTRING_INDEX(data, ' ', 8)) - 1));

-- Creator GUID
ALTER TABLE item_instance ADD COLUMN creator int(10) unsigned NOT NULL after container_guid;
UPDATE item_instance SET creator = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 10))+2, length(SUBSTRING_INDEX(data, ' ', 10+1))- length(SUBSTRING_INDEX(data, ' ', 10)) - 1));

-- Gift creator
ALTER TABLE item_instance ADD COLUMN gift_creator int(10) unsigned NOT NULL after creator;
UPDATE item_instance SET gift_creator = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 12))+2, length(SUBSTRING_INDEX(data, ' ', 12+1))- length(SUBSTRING_INDEX(data, ' ', 12)) - 1));

-- Stack amount
ALTER TABLE item_instance ADD COLUMN stacks mediumint(8) unsigned NOT NULL after gift_creator;
UPDATE item_instance SET stacks = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 14))+2, length(SUBSTRING_INDEX(data, ' ', 14+1))- length(SUBSTRING_INDEX(data, ' ', 14)) - 1));

-- Duration
ALTER TABLE item_instance ADD COLUMN duration mediumint(8) unsigned NOT NULL after stacks;
UPDATE item_instance SET duration = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 15))+2, length(SUBSTRING_INDEX(data, ' ', 15+1))- length(SUBSTRING_INDEX(data, ' ', 15)) - 1));

-- Spells charges
ALTER TABLE item_instance ADD COLUMN spell1_charges int(10) unsigned NOT NULL after duration;
UPDATE item_instance SET spell1_charges = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 16))+2, length(SUBSTRING_INDEX(data, ' ', 16+1))- length(SUBSTRING_INDEX(data, ' ', 16)) - 1));
ALTER TABLE item_instance ADD COLUMN spell2_charges int(10) unsigned NOT NULL after spell1_charges;
UPDATE item_instance SET spell2_charges = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 17))+2, length(SUBSTRING_INDEX(data, ' ', 17+1))- length(SUBSTRING_INDEX(data, ' ', 17)) - 1));
ALTER TABLE item_instance ADD COLUMN spell3_charges int(10) unsigned NOT NULL after spell2_charges;
UPDATE item_instance SET spell3_charges = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 18))+2, length(SUBSTRING_INDEX(data, ' ', 18+1))- length(SUBSTRING_INDEX(data, ' ', 18)) - 1));
ALTER TABLE item_instance ADD COLUMN spell4_charges int(10) unsigned NOT NULL after spell3_charges;
UPDATE item_instance SET spell4_charges = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 19))+2, length(SUBSTRING_INDEX(data, ' ', 19+1))- length(SUBSTRING_INDEX(data, ' ', 19)) - 1));
ALTER TABLE item_instance ADD COLUMN spell5_charges int(10) unsigned NOT NULL after spell4_charges;
UPDATE item_instance SET spell5_charges = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 20))+2, length(SUBSTRING_INDEX(data, ' ', 20+1))- length(SUBSTRING_INDEX(data, ' ', 20)) - 1));

-- Flags
ALTER TABLE item_instance ADD COLUMN flags mediumint(8) unsigned NOT NULL after spell5_charges;
UPDATE item_instance SET flags = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 21))+2, length(SUBSTRING_INDEX(data, ' ', 21+1))- length(SUBSTRING_INDEX(data, ' ', 21)) - 1));

-- Enchantments (11, 3 fields for each)
ALTER TABLE item_instance ADD COLUMN enchant1_id mediumint(8) unsigned NOT NULL after flags;
UPDATE item_instance SET enchant1_id = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 22))+2, length(SUBSTRING_INDEX(data, ' ', 22+1))- length(SUBSTRING_INDEX(data, ' ', 22)) - 1));
ALTER TABLE item_instance ADD COLUMN enchant1_duration int(10) unsigned NOT NULL after enchant1_id;
UPDATE item_instance SET enchant1_duration = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 23))+2, length(SUBSTRING_INDEX(data, ' ', 23+1))- length(SUBSTRING_INDEX(data, ' ', 23)) - 1));
ALTER TABLE item_instance ADD COLUMN enchant1_charges mediumint(8) unsigned NOT NULL after enchant1_duration;
UPDATE item_instance SET enchant1_charges = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 24))+2, length(SUBSTRING_INDEX(data, ' ', 24+1))- length(SUBSTRING_INDEX(data, ' ', 24)) - 1));

ALTER TABLE item_instance ADD COLUMN enchant2_id mediumint(8) unsigned NOT NULL after enchant1_charges;
UPDATE item_instance SET enchant2_id = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 25))+2, length(SUBSTRING_INDEX(data, ' ', 25+1))- length(SUBSTRING_INDEX(data, ' ', 25)) - 1));
ALTER TABLE item_instance ADD COLUMN enchant2_duration int(10) unsigned NOT NULL after enchant2_id;
UPDATE item_instance SET enchant2_duration = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 26))+2, length(SUBSTRING_INDEX(data, ' ', 26+1))- length(SUBSTRING_INDEX(data, ' ', 26)) - 1));
ALTER TABLE item_instance ADD COLUMN enchant2_charges mediumint(8) unsigned NOT NULL after enchant2_duration;
UPDATE item_instance SET enchant1_charges = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 27))+2, length(SUBSTRING_INDEX(data, ' ', 27+1))- length(SUBSTRING_INDEX(data, ' ', 27)) - 1));

ALTER TABLE item_instance ADD COLUMN enchant3_id mediumint(8) unsigned NOT NULL after enchant2_charges;
UPDATE item_instance SET enchant3_id = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 28))+2, length(SUBSTRING_INDEX(data, ' ', 28+1))- length(SUBSTRING_INDEX(data, ' ', 28)) - 1));
ALTER TABLE item_instance ADD COLUMN enchant3_duration int(10) unsigned NOT NULL after enchant3_id;
UPDATE item_instance SET enchant3_duration = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 29))+2, length(SUBSTRING_INDEX(data, ' ', 29+1))- length(SUBSTRING_INDEX(data, ' ', 29)) - 1));
ALTER TABLE item_instance ADD COLUMN enchant3_charges mediumint(8) unsigned NOT NULL after enchant3_duration;
UPDATE item_instance SET enchant3_charges = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 30))+2, length(SUBSTRING_INDEX(data, ' ', 30+1))- length(SUBSTRING_INDEX(data, ' ', 30)) - 1));

ALTER TABLE item_instance ADD COLUMN enchant4_id mediumint(8) unsigned NOT NULL after enchant3_charges;
UPDATE item_instance SET enchant4_id = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 31))+2, length(SUBSTRING_INDEX(data, ' ', 31+1))- length(SUBSTRING_INDEX(data, ' ', 31)) - 1));
ALTER TABLE item_instance ADD COLUMN enchant4_duration int(10) unsigned NOT NULL after enchant4_id;
UPDATE item_instance SET enchant4_duration = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 32))+2, length(SUBSTRING_INDEX(data, ' ', 32+1))- length(SUBSTRING_INDEX(data, ' ', 32)) - 1));
ALTER TABLE item_instance ADD COLUMN enchant4_charges mediumint(8) unsigned NOT NULL after enchant4_duration;
UPDATE item_instance SET enchant4_charges = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 33))+2, length(SUBSTRING_INDEX(data, ' ', 33+1))- length(SUBSTRING_INDEX(data, ' ', 33)) - 1));

ALTER TABLE item_instance ADD COLUMN enchant5_id mediumint(8) unsigned NOT NULL after enchant4_charges;
UPDATE item_instance SET enchant5_id = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 34))+2, length(SUBSTRING_INDEX(data, ' ', 34+1))- length(SUBSTRING_INDEX(data, ' ', 34)) - 1));
ALTER TABLE item_instance ADD COLUMN enchant5_duration int(10) unsigned NOT NULL after enchant5_id;
UPDATE item_instance SET enchant5_duration = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 35))+2, length(SUBSTRING_INDEX(data, ' ', 35+1))- length(SUBSTRING_INDEX(data, ' ', 35)) - 1));
ALTER TABLE item_instance ADD COLUMN enchant5_charges mediumint(8) unsigned NOT NULL after enchant5_duration;
UPDATE item_instance SET enchant5_charges = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 36))+2, length(SUBSTRING_INDEX(data, ' ', 36+1))- length(SUBSTRING_INDEX(data, ' ', 36)) - 1));

ALTER TABLE item_instance ADD COLUMN enchant6_id mediumint(8) unsigned NOT NULL after enchant5_charges;
UPDATE item_instance SET enchant6_id = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 37))+2, length(SUBSTRING_INDEX(data, ' ', 37+1))- length(SUBSTRING_INDEX(data, ' ', 37)) - 1));
ALTER TABLE item_instance ADD COLUMN enchant6_duration int(10) unsigned NOT NULL after enchant6_id;
UPDATE item_instance SET enchant6_duration = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 38))+2, length(SUBSTRING_INDEX(data, ' ', 38+1))- length(SUBSTRING_INDEX(data, ' ', 38)) - 1));
ALTER TABLE item_instance ADD COLUMN enchant6_charges mediumint(8) unsigned NOT NULL after enchant6_duration;
UPDATE item_instance SET enchant6_charges = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 39))+2, length(SUBSTRING_INDEX(data, ' ', 39+1))- length(SUBSTRING_INDEX(data, ' ', 39)) - 1));

ALTER TABLE item_instance ADD COLUMN enchant7_id mediumint(8) unsigned NOT NULL after enchant6_charges;
UPDATE item_instance SET enchant7_id = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 40))+2, length(SUBSTRING_INDEX(data, ' ', 40+1))- length(SUBSTRING_INDEX(data, ' ', 40)) - 1));
ALTER TABLE item_instance ADD COLUMN enchant7_duration int(10) unsigned NOT NULL after enchant7_id;
UPDATE item_instance SET enchant7_duration = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 41))+2, length(SUBSTRING_INDEX(data, ' ', 41+1))- length(SUBSTRING_INDEX(data, ' ', 41)) - 1));
ALTER TABLE item_instance ADD COLUMN enchant7_charges mediumint(8) unsigned NOT NULL after enchant7_duration;
UPDATE item_instance SET enchant7_charges = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 42))+2, length(SUBSTRING_INDEX(data, ' ', 42+1))- length(SUBSTRING_INDEX(data, ' ', 42)) - 1));

ALTER TABLE item_instance ADD COLUMN enchant8_id mediumint(8) unsigned NOT NULL after enchant7_charges;
UPDATE item_instance SET enchant8_id = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 43))+2, length(SUBSTRING_INDEX(data, ' ', 43+1))- length(SUBSTRING_INDEX(data, ' ', 43)) - 1));
ALTER TABLE item_instance ADD COLUMN enchant8_duration int(10) unsigned NOT NULL after enchant8_id;
UPDATE item_instance SET enchant8_duration = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 44))+2, length(SUBSTRING_INDEX(data, ' ', 44+1))- length(SUBSTRING_INDEX(data, ' ', 44)) - 1));
ALTER TABLE item_instance ADD COLUMN enchant8_charges mediumint(8) unsigned NOT NULL after enchant8_duration;
UPDATE item_instance SET enchant8_charges = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 45))+2, length(SUBSTRING_INDEX(data, ' ', 45+1))- length(SUBSTRING_INDEX(data, ' ', 45)) - 1));

ALTER TABLE item_instance ADD COLUMN enchant9_id mediumint(8) unsigned NOT NULL after enchant8_charges;
UPDATE item_instance SET enchant9_id = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 46))+2, length(SUBSTRING_INDEX(data, ' ', 46+1))- length(SUBSTRING_INDEX(data, ' ', 46)) - 1));
ALTER TABLE item_instance ADD COLUMN enchant9_duration int(10) unsigned NOT NULL after enchant9_id;
UPDATE item_instance SET enchant9_duration = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 47))+2, length(SUBSTRING_INDEX(data, ' ', 47+1))- length(SUBSTRING_INDEX(data, ' ', 47)) - 1));
ALTER TABLE item_instance ADD COLUMN enchant9_charges mediumint(8) unsigned NOT NULL after enchant9_duration;
UPDATE item_instance SET enchant9_charges = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 48))+2, length(SUBSTRING_INDEX(data, ' ', 48+1))- length(SUBSTRING_INDEX(data, ' ', 48)) - 1));

ALTER TABLE item_instance ADD COLUMN enchant10_id mediumint(8) unsigned NOT NULL after enchant9_charges;
UPDATE item_instance SET enchant10_id = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 49))+2, length(SUBSTRING_INDEX(data, ' ', 49+1))- length(SUBSTRING_INDEX(data, ' ', 49)) - 1));
ALTER TABLE item_instance ADD COLUMN enchant10_duration int(10) unsigned NOT NULL after enchant10_id;
UPDATE item_instance SET enchant10_duration = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 50))+2, length(SUBSTRING_INDEX(data, ' ', 50+1))- length(SUBSTRING_INDEX(data, ' ', 50)) - 1));
ALTER TABLE item_instance ADD COLUMN enchant10_charges mediumint(8) unsigned NOT NULL after enchant10_duration;
UPDATE item_instance SET enchant10_charges = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 51))+2, length(SUBSTRING_INDEX(data, ' ', 51+1))- length(SUBSTRING_INDEX(data, ' ', 51)) - 1));

ALTER TABLE item_instance ADD COLUMN enchant11_id mediumint(8) unsigned NOT NULL after enchant10_charges;
UPDATE item_instance SET enchant11_id = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 52))+2, length(SUBSTRING_INDEX(data, ' ', 52+1))- length(SUBSTRING_INDEX(data, ' ', 52)) - 1));
ALTER TABLE item_instance ADD COLUMN enchant11_duration int(10) unsigned NOT NULL after enchant11_id;
UPDATE item_instance SET enchant11_duration = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 53))+2, length(SUBSTRING_INDEX(data, ' ', 53+1))- length(SUBSTRING_INDEX(data, ' ', 53)) - 1));
ALTER TABLE item_instance ADD COLUMN enchant11_charges mediumint(8) unsigned NOT NULL after enchant11_duration;
UPDATE item_instance SET enchant11_charges = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 54))+2, length(SUBSTRING_INDEX(data, ' ', 54+1))- length(SUBSTRING_INDEX(data, ' ', 54)) - 1));

-- Property seed
ALTER TABLE item_instance ADD COLUMN property_seed int(10) unsigned NOT NULL after enchant11_charges;
UPDATE item_instance SET property_seed = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 55))+2, length(SUBSTRING_INDEX(data, ' ', 55+1))- length(SUBSTRING_INDEX(data, ' ', 55)) - 1));

-- Random properties ID
ALTER TABLE item_instance ADD COLUMN random_prop_id int(10) unsigned NOT NULL after property_seed;
UPDATE item_instance SET random_prop_id = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 56))+2, length(SUBSTRING_INDEX(data, ' ', 56+1))- length(SUBSTRING_INDEX(data, ' ', 56)) - 1));

-- Text ID
ALTER TABLE item_instance ADD COLUMN text_id int(10) unsigned NOT NULL after random_prop_id;
UPDATE item_instance SET text_id = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 57))+2, length(SUBSTRING_INDEX(data, ' ', 57+1))- length(SUBSTRING_INDEX(data, ' ', 57)) - 1));

-- Durability
ALTER TABLE item_instance ADD COLUMN durability mediumint(8) unsigned NOT NULL after text_id;
UPDATE item_instance SET durability = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 58))+2, length(SUBSTRING_INDEX(data, ' ', 58+1))- length(SUBSTRING_INDEX(data, ' ', 58)) - 1));

-- Max durability
ALTER TABLE item_instance ADD COLUMN max_durability mediumint(8) unsigned NOT NULL after durability;
UPDATE item_instance SET max_durability = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 59))+2, length(SUBSTRING_INDEX(data, ' ', 59+1))- length(SUBSTRING_INDEX(data, ' ', 59)) - 1));

-- CONTAINERS
-- Num slots
ALTER TABLE item_instance ADD COLUMN num_slots mediumint(8) unsigned NOT NULL after max_durability;
UPDATE item_instance SET num_slots = (SUBSTRING(data, length(SUBSTRING_INDEX(data, ' ', 60))+2, length(SUBSTRING_INDEX(data, ' ', 60+1))- length(SUBSTRING_INDEX(data, ' ', 60)) - 1));
