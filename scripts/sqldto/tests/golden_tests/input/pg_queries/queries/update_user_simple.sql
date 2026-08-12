UPDATE queries.users
SET status = $2
WHERE id = $1;
