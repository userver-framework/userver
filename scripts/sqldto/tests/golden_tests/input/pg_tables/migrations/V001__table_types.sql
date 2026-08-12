BEGIN;

CREATE SCHEMA tables;

CREATE TYPE tables.user_role AS ENUM (
    'admin',
    'user',
    'guest'
);

CREATE TYPE tables.profile_data AS (
    first_name TEXT,
    last_name TEXT,
    birth_year INTEGER,
    is_verified BOOLEAN
);

CREATE TABLE tables.users (
    id BIGSERIAL PRIMARY KEY,
    username VARCHAR(50) NOT NULL UNIQUE,
    email TEXT NOT NULL,
    password_hash CHAR(64),
    age SMALLINT CHECK (age >= 0),
    balance DECIMAL(10, 2) DEFAULT 0.00,
    rating REAL,
    created_at TIMESTAMPTZ DEFAULT NOW(),
    last_login TIMESTAMP,
    birth_date DATE,
    external_id UUID UNIQUE,
    is_active BOOLEAN DEFAULT TRUE,
    is_deleted BOOLEAN DEFAULT FALSE,
    preferences JSONB DEFAULT '{}'::JSONB,
    metadata JSON,
    role tables.user_role DEFAULT 'user',
    profile tables.profile_data,
    tags TEXT[] DEFAULT '{}',
    favorite_numbers INTEGER[],
    avatar BYTEA,
    session_duration INTERVAL,
    valid_age_range INT4RANGE
);

CREATE TABLE tables.posts (
    id BIGSERIAL PRIMARY KEY,
    author_id BIGINT REFERENCES tables.users(id),
    title VARCHAR(200) NOT NULL,
    content TEXT,
    published_at TIMESTAMPTZ,
    updated_at TIMESTAMPTZ DEFAULT NOW(),
    view_count INTEGER DEFAULT 0,
    is_published BOOLEAN DEFAULT FALSE
);

COMMIT;
