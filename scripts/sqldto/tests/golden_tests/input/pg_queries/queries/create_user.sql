-- @arg1: TEXT
-- @arg2: TEXT
INSERT INTO queries.users (username, email, status)
VALUES ($1, $2, $3)
RETURNING id, username, email, status, created_at;
