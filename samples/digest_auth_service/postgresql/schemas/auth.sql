DROP SCHEMA IF EXISTS auth_schema CASCADE;

CREATE SCHEMA IF NOT EXISTS auth_schema;

CREATE SEQUENCE IF NOT EXISTS nonce_id_seq START 1 INCREMENT 1;

CREATE TABLE IF NOT EXISTS auth_schema.users (
    username TEXT NOT NULL,
    nonce TEXT NOT NULL,
    timestamp TIMESTAMP NOT NULL DEFAULT NOW(),
    nonce_count integer NOT NULL DEFAULT 0,
    ha1 TEXT NOT NULL,
    PRIMARY KEY(username)
);

CREATE TABLE IF NOT EXISTS auth_schema.unnamed_nonce (
    id integer NOT NULL,
    nonce TEXT NOT NULL,
    creation_time TIMESTAMP NOT NULL,
    PRIMARY KEY(id),
    UNIQUE(nonce)
);