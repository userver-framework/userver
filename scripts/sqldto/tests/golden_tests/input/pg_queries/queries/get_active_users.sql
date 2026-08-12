SELECT id, username, email, created_at
FROM queries.users
WHERE status = 'active'
ORDER BY created_at DESC;
