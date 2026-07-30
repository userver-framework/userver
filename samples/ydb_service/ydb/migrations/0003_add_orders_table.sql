-- +goose Up
CREATE TABLE `orders` (
    id String,
    doc String,
    PRIMARY KEY(id)
);
