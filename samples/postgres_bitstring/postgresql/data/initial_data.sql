INSERT INTO bitstring_table(id, varbit, bit8, bit32, bit128) VALUES
(1, '01001111'::bit(8), '01001111'::bit(8), '01001111'::bit(32), '01001111'::bit(128));
INSERT INTO bitstring_table(id, varbit) VALUES
(2, '01001111'::bit(32)),
(3, '01001111'::bit(128));
