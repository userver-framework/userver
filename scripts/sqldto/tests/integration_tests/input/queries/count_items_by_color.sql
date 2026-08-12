SELECT color, COUNT(*)::BIGINT AS amount
FROM smoke.items
GROUP BY color
ORDER BY amount DESC, color;
