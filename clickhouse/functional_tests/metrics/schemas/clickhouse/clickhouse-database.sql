CREATE TABLE IF NOT EXISTS key_value_table (
  key String,
  value String,
  updated DateTime('Etc/UTC') NOT NULL,
  PRIMARY KEY key
) ENGINE = MergeTree;
