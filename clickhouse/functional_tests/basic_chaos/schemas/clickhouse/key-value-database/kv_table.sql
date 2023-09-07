CREATE TABLE IF NOT EXISTS kv (
  key String,
  value String,
  PRIMARY KEY key
) ENGINE = MergeTree;
