INSERT INTO key_value_table (key, value)
VALUES ($1, $2)
ON CONFLICT DO NOTHING
