SELECT id, username, email, status, created_at
FROM queries.users
WHERE id = $1;
