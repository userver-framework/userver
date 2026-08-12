DELETE FROM queries.users
WHERE id = $1
RETURNING id, username;
