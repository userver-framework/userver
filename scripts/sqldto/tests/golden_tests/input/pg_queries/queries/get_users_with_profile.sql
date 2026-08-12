SELECT id, username, email, profile, tags
FROM queries.users
WHERE profile IS NOT NULL
  AND (profile).is_verified = true
ORDER BY id;
