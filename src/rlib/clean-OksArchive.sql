rem ############################################################################################
rem ### SqlPlus settings
rem ############################################################################################

SET lin 150;
SET PAGES 25;


rem ############################################################################################
rem ### Get runs with duplicated records
rem ############################################################################################

SELECT DISTINCT e.RunNumber FROM OksArchive e, OksArchive m
WHERE e.RunNumber=m.RunNumber AND e.Time != m.Time
ORDER BY e.RunNumber;


rem ############################################################################################
rem ### Show first record for each run (useful in case of duplicated records)
rem ############################################################################################

SELECT RunNumber, min(Time) FROM OksArchive GROUP BY RunNumber ORDER BY RunNumber;


rem ############################################################################################
rem ### Show duplicated records
rem ############################################################################################

SELECT a.RunNumber,a.Time,a.SchemaVersion,a.DataVersion
FROM OksArchive a, (SELECT RunNumber, min(Time) MinTime FROM OksArchive GROUP BY RunNumber) b
WHERE a.Time > b.MinTime AND a.RunNumber=b.RunNumber
ORDER BY a.RunNumber,a.Time;


rem ############################################################################################
rem ### Delete duplicated records
rem ############################################################################################

DELETE FROM OksArchive
WHERE (runnumber, time, schemaversion, dataversion) IN 
( SELECT a.RunNumber,a.Time,a.SchemaVersion,a.DataVersion
  FROM OksArchive a, (SELECT RunNumber, min(Time) MinTime FROM OksArchive GROUP BY RunNumber) b
  WHERE a.Time > b.MinTime AND a.RunNumber=b.RunNumber
);
