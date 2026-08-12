-- +goose Up
-- +goose StatementBegin
CREATE TABLE events (
    id String,
    name Utf8,
    service String,
    channel Int64,
    created Timestamp,
    state Json,
    PRIMARY KEY (id, name),
    INDEX sample_index GLOBAL ON (service, channel, created)
);
-- +goose StatementEnd

-- +goose Down
-- +goose StatementBegin
DROP TABLE events;
-- +goose StatementEnd
