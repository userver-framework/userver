/* /// [postgresql schema] */
DROP SCHEMA IF EXISTS auth_schema CASCADE;

CREATE SCHEMA IF NOT EXISTS auth_schema;

CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

CREATE TABLE IF NOT EXISTS auth_schema.users (
    username TEXT NOT NULL,
    nonce TEXT NOT NULL,
    timestamp TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    nonce_count integer NOT NULL DEFAULT 0,
    ha1 TEXT NOT NULL,
    PRIMARY KEY(username)
);

CREATE TABLE IF NOT EXISTS auth_schema.unnamed_nonce (
    id uuid NOT NULL,
    nonce TEXT NOT NULL,
    creation_time TIMESTAMPTZ NOT NULL,
    PRIMARY KEY(id),
    UNIQUE(nonce)
);
/* /// [postgresql schema] */
