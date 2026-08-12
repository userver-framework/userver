SELECT status, COUNT(*) as user_count
FROM queries.users
WHERE status IS NOT NULL
GROUP BY status
ORDER BY user_count DESC;
