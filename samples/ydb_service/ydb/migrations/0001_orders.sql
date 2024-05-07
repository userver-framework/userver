-- +goose Up
CREATE TABLE orders (
    id String,
    doc String,
    PRIMARY KEY(id)
);

CREATE TABLE events (
      id String,
      name Utf8,
      service String,
      channel Int64,
      created Timestamp,
      state Json,
    INDEX sample_index GLOBAL ON (service, channel, created),
    PRIMARY KEY(id, name)
);

