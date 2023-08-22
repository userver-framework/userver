CREATE TABLE IF NOT EXISTS bitstring_table (
    id bigserial PRIMARY KEY,
    varbit bit varying,
    bit8 bit(8),
    bit32 bit(32),
    bit128 bit(128)
);
