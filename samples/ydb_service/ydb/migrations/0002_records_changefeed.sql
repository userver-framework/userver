-- +goose Up
CREATE TABLE `records` (
      id Utf8,
      name Utf8,
      str String,
      num Uint64,
      PRIMARY KEY(id, name)
);

ALTER TABLE `records` ADD CHANGEFEED `changefeed`
WITH (
    FORMAT = 'JSON',
    MODE = 'NEW_AND_OLD_IMAGES'
);

ALTER TOPIC `records/changefeed` ADD CONSUMER `sample-consumer`;
