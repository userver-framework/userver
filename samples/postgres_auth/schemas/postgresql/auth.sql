/* /// [postgresql schema] */
DROP SCHEMA IF EXISTS auth_schema CASCADE;

CREATE SCHEMA IF NOT EXISTS auth_schema;

CREATE TABLE IF NOT EXISTS auth_schema.tokens (
    token TEXT PRIMARY KEY NOT NULL,
    user_id INTEGER NOT NULL,
    scopes TEXT[] NOT NULL,
    updated TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    name TEXT NOT NULL
);
/* /// [postgresql schema] */
