BEGIN;

CREATE SCHEMA nested;

CREATE TYPE nested.priority AS ENUM ('low', 'high');
CREATE TYPE nested.category AS ENUM ('work', 'personal', 'other');

CREATE TYPE nested.contact AS (
    phone TEXT,
    email VARCHAR(255),
    is_primary BOOLEAN
);

CREATE TYPE nested.address AS (
    street TEXT,
    city VARCHAR(100),
    postal_code VARCHAR(20)
);

CREATE TYPE nested.person AS (
    name TEXT,
    age INTEGER,
    contacts nested.contact,
    home_address nested.address,
    work_address nested.address,
    priority nested.priority
);

CREATE TYPE nested.organization AS (
    org_id BIGINT,
    org_name TEXT,
    founded DATE,
    headquarters nested.address,
    phone_numbers TEXT[],
    employee_counts INTEGER[],
    locations nested.address[],
    key_contacts nested.contact[],
    categories nested.category[],
    ceo nested.person,
    employees nested.person[]
);

COMMIT;
