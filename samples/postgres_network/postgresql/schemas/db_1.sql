DROP SCHEMA IF EXISTS example_schema CASCADE;

CREATE SCHEMA IF NOT EXISTS example_schema;

CREATE TABLE IF NOT EXISTS example_schema.network (
    id BIGSERIAL PRIMARY KEY,
    ipv4 CIDR,
    ipv6 CIDR,
    netv4 CIDR,
    netv6 CIDR,
    mac MACADDR,
    mac8 MACADDR8
);
