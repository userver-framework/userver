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
/opt/userver/scripts/scylla/entrypoint.sh
```

This builds current userver and runs the `samples/scylla_service` sample.

---

From inside the dev container

```shell
curl -X POST http://localhost:8080/v1/insert

curl http://localhost:8080/v1/select
```