select RunNumber from RunNumber where ConfigSchema=0 and ConfigData=0 order by RunNumber;

SELECT a.RunNumber,a.SchemaVersion,a.DataVersion,b.ConfigSchema,b.ConfigData
FROM atlas_oks_archive.OksArchive a, atlas_run_number.RunNumber b
WHERE a.RunNumber=b.RunNumber AND b.ConfigSchema=0 AND b.ConfigData=0
ORDER BY a.RunNumber;

UPDATE atlas_run_number.RunNumber t SET (ConfigSchema,ConfigData) = (
  SELECT a.SchemaVersion,a.DataVersion
  FROM atlas_oks_archive.OksArchive a
  WHERE a.RunNumber=t.RunNumber
)
WHERE t.ConfigSchema=0 AND t.ConfigData=0 AND t.RunNumber IN (
  SELECT a.RunNumber
  FROM atlas_oks_archive.OksArchive a, atlas_run_number.RunNumber b
  WHERE a.RunNumber=b.RunNumber AND b.ConfigSchema=0 AND b.ConfigData=0
);
