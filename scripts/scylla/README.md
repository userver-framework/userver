Start the containers

```shell
docker compose up --build -d
```

Enter the ScyllaDB container and open cqlsh:

```shell
docker exec -it scylla cqlsh
```

Create the keyspace and table:

```sql
CREATE KEYSPACE examples WITH replication = {
    'class': 'SimpleStrategy',
    'replication_factor': '1'
};

CREATE TABLE examples.basic (
    key text,
    bln boolean,
    flt float,
    dbl double,
    i32 int,
    i64 bigint,
    PRIMARY KEY (key)
);
```

#### Build and run the example

```shell
docker exec -it scylla-dev-1 bash
```

```shell
/opt/userver/scripts/dev/scylla/entrypoint.sh
```

This builds and runs the `samples/scylla_service` sample.

---

From inside the dev container

```shell
curl -X POST http://localhost:8080/v1/example
```

Back in cqlsh

```sql
SELECT * FROM examples.basic;
```

Expected output:

```
 key  | bln  | dbl    | flt   | i32 | i64
------+------+--------+-------+-----+-----
 test | True | 0.0002 | 0.001 |   1 |   2

(1 rows)
```