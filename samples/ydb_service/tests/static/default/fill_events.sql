UPSERT INTO events (id, name, service, channel, created, state) VALUES
("id-1", "name-1", "srv", 1, CAST("2019-10-31T11:20:00.000000Z" AS Timestamp), null),
("id-2", "name-2", "srv-state", 2, CAST("2019-10-31T11:20:00.000000Z" AS Timestamp), '{"qwe":"asd","zxc":123}')
