SELECT profile
FROM queries.users
WHERE id = $1 AND profile IS NOT NULL;
