UPDATE version SET core_version = "Winrunner Rev: 6224";

UPDATE creature_template
SET flags_extra = flags_extra + 0x10000
WHERE entry IN (
SELECT entry FROM (
SELECT entry, CASE
WHEN flags_extra & 0x10000
THEN 1
ELSE 0
END AS immunetaunt
FROM creature_template
) AS lol
WHERE immunetaunt = 0
AND entry IN (19044, 17257, 16151, 15550, 16152, 15687, 16457, 17533, 17534, 17535, 17547, 17543, 17546, 18168, 17521, 15691, 16524, 15688, 15689, 17225, 15690, 21216, 21215, 21213, 21965, 21214, 19516, 20064, 20060, 20063, 19622, 17767, 17808, 17888, 17842, 17968, 22887, 22898, 23574, 23578, 24239 )
);