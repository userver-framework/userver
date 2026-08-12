-- @cardinality: optional
DELETE FROM smoke.items WHERE id = $1 RETURNING id, name;
