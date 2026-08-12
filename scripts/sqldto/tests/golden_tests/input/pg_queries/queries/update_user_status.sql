UPDATE queries.users
SET status = $2, updated_at = NOW()
WHERE id = $1
RETURNING id, username, status, updated_at;
