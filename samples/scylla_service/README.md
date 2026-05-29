# scylla_service demo

Enter the ScyllaDB container and open cqlsh

```shell
docker exec -it scylla cqlsh
```

Create the keyspace and table

```sql
CREATE KEYSPACE IF NOT EXISTS examples
    WITH REPLICATION = { 'class' : 'SimpleStrategy', 'replication_factor' : 1 };

CREATE TABLE IF NOT EXISTS examples.basic (
    key text PRIMARY KEY,
    bln boolean,
    flt float,
    dbl double,
    i32 int,
    i64 bigint
);
```


---

```shell
docker exec -it scylla-dev-1 bash
```

## Operations


### InsertOne

```bash
curl -sS -X POST "http://localhost:8080/v1/kv" \
    -H 'Content-Type: application/json' \
    -d '{"key":"alpha","bln":true,"i32":1,"i64":42,"flt":1.5,"dbl":2.5}'
```

### 2. SelectOne

```bash
curl -sS "http://localhost:8080/v1/kv?key=alpha"

# should return 404
curl -sS -w '\n%{http_code}\n' "http://localhost:8080/v1/kv?key=does-not-exist"
```

### 3. UpdateOne

```bash
curl -sS -X PATCH "http://localhost:8080/v1/kv?key=alpha" \
    -H 'Content-Type: application/json' \
    -d '{"bln":false,"i32":99}'

curl -sS "http://localhost:8080/v1/kv?key=alpha"
```

### 4. DeleteOne

```bash
curl -sS -X DELETE "http://localhost:8080/v1/kv?key=alpha"
```

### 5. InsertMany

```bash
curl -sS -X POST "http://localhost:8080/v1/kv/bulk" \
    -H 'Content-Type: application/json' \
    -d '[
          {"key":"bulk-1","i32":1,"bln":true},
          {"key":"bulk-2","i32":2,"bln":true},
          {"key":"bulk-3","i32":3,"bln":false}
        ]'
```

### 6. SelectMany

```bash
curl -sS "http://localhost:8080/v1/kv/list"

curl -sS "http://localhost:8080/v1/kv/list?limit=2"
```

### 7. Count

```bash
curl -sS "http://localhost:8080/v1/kv/count"

curl -sS "http://localhost:8080/v1/kv/count?key=bulk-1"
```

### 8. Truncate

```bash
curl -sS -X POST "http://localhost:8080/v1/kv/truncate"

curl -sS "http://localhost:8080/v1/kv/count"
```

### 9. InsertOne IF NOT EXISTS (LWT)

```bash
curl -sS -X POST "http://localhost:8080/v1/kv/create_if_absent" \
    -H 'Content-Type: application/json' \
    -d '{"key":"lwt-1","i32":100}'

curl -sS -w '\n%{http_code}\n' -X POST "http://localhost:8080/v1/kv/create_if_absent" \
    -H 'Content-Type: application/json' \
    -d '{"key":"lwt-1","i32":999}'
```

### 10. UpdateOne IF col = ? (Compare-And-Set)

```bash
curl -sS -X POST "http://localhost:8080/v1/kv/cas?key=lwt-1" \
    -H 'Content-Type: application/json' \
    -d '{"expect":{"i32":100},"set":{"i32":101}}'

curl -sS -w '\n%{http_code}\n' -X POST "http://localhost:8080/v1/kv/cas?key=lwt-1" \
    -H 'Content-Type: application/json' \
    -d '{"expect":{"i32":100},"set":{"i32":200}}'
```

### 11. DeleteOne IF EXISTS

```bash
curl -sS -X DELETE "http://localhost:8080/v1/kv/delete_if_exists?key=lwt-1"

curl -sS -w '\n%{http_code}\n' -X DELETE "http://localhost:8080/v1/kv/delete_if_exists?key=lwt-1"
```


---