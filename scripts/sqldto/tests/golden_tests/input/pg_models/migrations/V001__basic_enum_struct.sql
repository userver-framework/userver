BEGIN;

CREATE SCHEMA test;

CREATE TYPE test.status_type AS ENUM (
    'active',
    'inactive',
    'pending'
);

CREATE TYPE test.user_data AS (
    small_num SMALLINT,
    regular_num INTEGER,
    big_num BIGINT,
    name TEXT,
    code VARCHAR(50),
    single_char CHAR(1),
    is_enabled BOOLEAN,
    rate REAL,
    precise_rate DOUBLE PRECISION,
    balance DECIMAL(15, 2),
    score NUMERIC(10, 4),
    created_at TIMESTAMPTZ,
    updated_at TIMESTAMP,
    birth_date DATE,
    work_time TIME,
    duration INTERVAL,
    user_uuid UUID,
    metadata JSONB,
    settings JSON,
    avatar BYTEA,
    age_range INT4RANGE,
    id_range INT8RANGE,
    status test.status_type
);

COMMIT;
