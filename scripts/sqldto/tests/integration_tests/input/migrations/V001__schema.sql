CREATE SCHEMA smoke;

CREATE TYPE smoke.color AS ENUM (
    'red',
    'green',
    'blue'
);

CREATE TYPE smoke.contact AS (
    kind TEXT,
    value TEXT,
    is_primary BOOLEAN
);

CREATE TYPE smoke.profile AS (
    display_name TEXT,
    age INTEGER,
    favourite smoke.color,
    contacts smoke.contact[]
);

CREATE TABLE smoke.items (
    id BIGSERIAL PRIMARY KEY,
    name TEXT NOT NULL,
    color smoke.color NOT NULL,
    tags TEXT[],
    profile smoke.profile
);
