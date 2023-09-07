CREATE TABLE IF NOT EXISTS uuids (
  uuid_mismatched UUID,
  uuid_correct UUID,
  PRIMARY KEY uuid_mismatched
) ENGINE = MergeTree;
