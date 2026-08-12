-- @cardinality: one
INSERT INTO smoke.items (name, color, tags, profile)
VALUES ($1, $2, $3, $4)
RETURNING id;
