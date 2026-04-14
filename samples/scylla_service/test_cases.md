# ScyllaDB Driver — Demo Test Cases

All examples assume the service is running on `localhost:8080`.

---

## 1. Schema Initialization (Raw CQL — DDL)

Creates both tables via `Session::ExecuteVoid()`.

```bash
curl -sS -X POST "http://localhost:8080/v1/schema/init" -w '\n%{http_code}\n'
```

```
{"status":"ok","tables":["basic","events"]}
200
```

---

## 2. Basic CRUD — `/v1/kv`

### Insert

```bash
curl -sS -X POST "http://localhost:8080/v1/kv" \
    -H 'Content-Type: application/json' \
    -d '{"key":"alpha","bln":true,"i32":1,"i64":42,"flt":1.5,"dbl":2.5}' \
    -w '\n%{http_code}\n'
```

```
{"status":"ok"}
200
```

### Insert with TTL (row expires after 60 seconds)

```bash
curl -sS -X POST "http://localhost:8080/v1/kv" \
    -H 'Content-Type: application/json' \
    -d '{"key":"ephemeral","i32":999,"ttl":60}' \
    -w '\n%{http_code}\n'
```

```
{"status":"ok"}
200
```

### Insert minimal

```bash
curl -sS -X POST "http://localhost:8080/v1/kv" \
    -H 'Content-Type: application/json' \
    -d '{"key":"beta","bln":false}' \
    -w '\n%{http_code}\n'
```

```
{"status":"ok"}
200
```

### Insert — missing key → 400

```bash
curl -sS -X POST "http://localhost:8080/v1/kv" \
    -H 'Content-Type: application/json' \
    -d '{"bln":true}' \
    -w '\n%{http_code}\n'
```

```
JSON field 'key' is required
400
```

### Get by key

```bash
curl -sS "http://localhost:8080/v1/kv?key=alpha" -w '\n%{http_code}\n'
```

```
{"key":"alpha","bln":true,"dbl":2.5,"flt":1.5,"i32":1,"i64":42}
200
```

### Get — not found → 404

```bash
curl -sS "http://localhost:8080/v1/kv?key=ghost" -w '\n%{http_code}\n'
```

```
{"error":"not_found"}
404
```

### Update

```bash
curl -sS -X PATCH "http://localhost:8080/v1/kv?key=alpha" \
    -H 'Content-Type: application/json' \
    -d '{"i32":99,"bln":false}' \
    -w '\n%{http_code}\n'
```

```
{"status":"ok"}
200
```

### Update with TTL (modified columns expire after 300 seconds)

```bash
curl -sS -X PATCH "http://localhost:8080/v1/kv?key=alpha" \
    -H 'Content-Type: application/json' \
    -d '{"i64":100,"ttl":300}' \
    -w '\n%{http_code}\n'
```

```
{"status":"ok"}
200
```

### Delete

```bash
curl -sS -X DELETE "http://localhost:8080/v1/kv?key=beta" -w '\n%{http_code}\n'
```

```
{"status":"ok"}
200
```

---

## 3. Bulk Insert — `/v1/kv/bulk`

```bash
curl -sS -X POST "http://localhost:8080/v1/kv/bulk" \
    -H 'Content-Type: application/json' \
    -d '[
      {"key":"bulk-1","i32":1,"bln":true},
      {"key":"bulk-2","i32":2,"bln":true},
      {"key":"bulk-3","i32":3,"bln":false}
    ]' \
    -w '\n%{http_code}\n'
```

```
{"inserted":3}
200
```

---

## 4. List All — `/v1/kv/list`

```bash
curl -sS "http://localhost:8080/v1/kv/list" -w '\n%{http_code}\n'
```

```json
{"items":[
  {"key":"alpha","bln":false,"dbl":2.5,"flt":1.5,"i32":99,"i64":100},
  {"key":"bulk-1","bln":true,"i32":1},
  {"key":"bulk-2","bln":true,"i32":2},
  {"key":"bulk-3","bln":false,"i32":3}
],"count":4}
```

Note: row order is not guaranteed by Scylla.

### List with limit

```bash
curl -sS "http://localhost:8080/v1/kv/list?limit=2" -w '\n%{http_code}\n'
```

```
{"items":[{...},{...}],"count":2}
200
```

---

## 5. Count — `/v1/kv/count`

```bash
curl -sS "http://localhost:8080/v1/kv/count" -w '\n%{http_code}\n'
```

```
{"count":4}
200
```

---

## 6. Paginated Listing — `/v1/kv/pages`

Demonstrates `Table::ExecutePaged()` with client-controlled cursor tokens.

### First page

```bash
curl -sS "http://localhost:8080/v1/kv/pages?page_size=2" -w '\n%{http_code}\n'
```

```json
{
  "items": [{...}, {...}],
  "count": 2,
  "has_more": true,
  "next_cursor": "0a0b0c..."
}
```

### Next page (pass the cursor back)

```bash
curl -sS "http://localhost:8080/v1/kv/pages?page_size=2&cursor=0a0b0c..." \
    -w '\n%{http_code}\n'
```

```json
{
  "items": [{...}, {...}],
  "count": 2,
  "has_more": false
}
```

When `has_more` is `false`, there are no more pages. The `next_cursor` field
is only present when more pages exist.

---

## 7. Lightweight Transactions (LWT)

### Conditional insert 

```bash
# First call succeeds
curl -sS -X POST "http://localhost:8080/v1/kv/create_if_absent" \
    -H 'Content-Type: application/json' \
    -d '{"key":"lwt-1","i32":100}' \
    -w '\n%{http_code}\n'

# Second call fails — row already exists
curl -sS -X POST "http://localhost:8080/v1/kv/create_if_absent" \
    -H 'Content-Type: application/json' \
    -d '{"key":"lwt-1","i32":999}' \
    -w '\n%{http_code}\n'
```

```
{"applied":true}
200
{"applied":false,"existing":{"key":"lwt-1","i32":100}}
409
```

### Compare-and-set 

```bash
# Succeeds: i32 is currently 100
curl -sS -X POST "http://localhost:8080/v1/kv/cas?key=lwt-1" \
    -H 'Content-Type: application/json' \
    -d '{"expect":{"i32":100},"set":{"i32":101}}' \
    -w '\n%{http_code}\n'

# Fails: i32 is now 101, not 100
curl -sS -X POST "http://localhost:8080/v1/kv/cas?key=lwt-1" \
    -H 'Content-Type: application/json' \
    -d '{"expect":{"i32":100},"set":{"i32":200}}' \
    -w '\n%{http_code}\n'
```

```
{"applied":true}
200
{"applied":false,"current":{"i32":101}}
409
```

### Conditional delete

```bash
curl -sS -X DELETE "http://localhost:8080/v1/kv/delete_if_exists?key=lwt-1" \
    -w '\n%{http_code}\n'
curl -sS -X DELETE "http://localhost:8080/v1/kv/delete_if_exists?key=lwt-1" \
    -w '\n%{http_code}\n'
```

```
{"applied":true}
200
{"applied":false}
404
```

---

## 8. Events

The `events` table uses: **uuid**, **timestamp**, **inet**, **set\<text\>**,
**map\<text, text\>**, **list\<int\>**.

### Create event with all rich types

```bash
curl -sS -X POST "http://localhost:8080/v1/events" \
    -H 'Content-Type: application/json' \
    -d '{
      "name": "deploy-v2.1",
      "source_ip": "10.0.0.42",
      "tags": ["production", "critical"],
      "metadata": {"env": "prod", "region": "us-east-1"},
      "scores": [95, 88, 72]
    }' \
    -w '\n%{http_code}\n'
```

```json
{"status":"ok","id":"550e8400-e29b-41d4-a716-446655440000","created_at":1713100800000}
200
```

The response includes the auto-generated UUID and epoch-millisecond timestamp.

### Create event with TTL (auto-deletes after 1 hour)

```bash
curl -sS -X POST "http://localhost:8080/v1/events" \
    -H 'Content-Type: application/json' \
    -d '{"name": "temp-alert", "ttl": 3600}' \
    -w '\n%{http_code}\n'
```

```json
{"status":"ok","id":"...","created_at":...}
200
```

### Create event with explicit write timestamp

```bash
curl -sS -X POST "http://localhost:8080/v1/events" \
    -H 'Content-Type: application/json' \
    -d '{"name": "backfill", "write_timestamp": 1700000000000000}' \
    -w '\n%{http_code}\n'
```

### Get event by UUID — typed deserialization with `row.As<Event>()`

```bash
curl -sS "http://localhost:8080/v1/events?id=550e8400-e29b-41d4-a716-446655440000" \
    -w '\n%{http_code}\n'
```

```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "name": "deploy-v2.1",
  "created_at": 1713100800000,
  "source_ip": "10.0.0.42",
  "tags": ["production", "critical"],
  "metadata": {"env": "prod", "region": "us-east-1"},
  "scores": [95, 88, 72]
}
```

---

## 9. Event Listing with Cursor — `/v1/events/list`

```bash
curl -sS "http://localhost:8080/v1/events/list?page_size=10" -w '\n%{http_code}\n'
```

```json
{
  "items": [
    {"id":"...","name":"deploy-v2.1","created_at":...,"source_ip":"10.0.0.42","tags":["production","critical"],...},
    {"id":"...","name":"temp-alert","created_at":...}
  ],
  "count": 2
}
```

---

## 10. Raw CQL Execution — `/v1/raw`

Escape hatch for hand-written CQL: aggregations, system tables, DDL, etc.

### SELECT with parameters

```bash
curl -sS -X POST "http://localhost:8080/v1/raw" \
    -H 'Content-Type: application/json' \
    -d '{"query": "SELECT key, i32 FROM basic WHERE key = ?", "params": ["alpha"]}' \
    -w '\n%{http_code}\n'
```

```json
{"rows":[{"key":"alpha","i32":99}],"count":1}
```

### Full table scan

```bash
curl -sS -X POST "http://localhost:8080/v1/raw" \
    -H 'Content-Type: application/json' \
    -d '{"query": "SELECT * FROM basic"}' \
    -w '\n%{http_code}\n'
```

### System table query

```bash
curl -sS -X POST "http://localhost:8080/v1/raw" \
    -H 'Content-Type: application/json' \
    -d '{"query": "SELECT cluster_name, release_version FROM system.local"}' \
    -w '\n%{http_code}\n'
```

```json
{"rows":[{"cluster_name":"Test Cluster","release_version":"5.4.0"}],"count":1}
```

### DDL via raw CQL

```bash
curl -sS -X POST "http://localhost:8080/v1/raw" \
    -H 'Content-Type: application/json' \
    -d '{"query": "CREATE INDEX IF NOT EXISTS idx_events_name ON examples.events (name)", "void": true}' \
    -w '\n%{http_code}\n'
```

```
{"status":"ok"}
200
```

### Aggregation query

```bash
curl -sS -X POST "http://localhost:8080/v1/raw" \
    -H 'Content-Type: application/json' \
    -d '{"query": "SELECT COUNT(*) FROM events"}' \
    -w '\n%{http_code}\n'
```

```json
{"rows":[{"count":2}],"count":1}
```

---

## 11. Truncate 

```bash
curl -sS -X POST "http://localhost:8080/v1/kv/truncate" -w '\n%{http_code}\n'
curl -sS "http://localhost:8080/v1/kv/count" -w '\n%{http_code}\n'
```

```
{"status":"ok"}
200
{"count":0}
200
```

---
---