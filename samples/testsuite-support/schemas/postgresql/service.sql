CREATE TABLE distlocks
(
     key             TEXT PRIMARY KEY,
     owner           TEXT,
     expiration_time TIMESTAMPTZ
);
