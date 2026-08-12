BEGIN;

CREATE SCHEMA queries;

CREATE TYPE queries.user_status AS ENUM (
    'active',
    'blocked',
    'deleted'
);

CREATE TYPE queries.user_profile AS (
    display_name TEXT,
    age INTEGER,
    country VARCHAR(100),
    is_verified BOOLEAN
);

CREATE TABLE queries.users (
    id BIGSERIAL PRIMARY KEY,
    username VARCHAR(50) NOT NULL UNIQUE,
    email TEXT NOT NULL,
    status queries.user_status DEFAULT 'active',
    profile queries.user_profile,
    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW(),
    tags TEXT[]
);

COMMIT;
