-- @cardinality: optional
SELECT id, name, color, tags, profile FROM smoke.items WHERE id = $1;
