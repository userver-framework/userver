# benchmark_service

Sample HTTP service used by [HttpArena](https://github.com/MDA2AV/HttpArena)
benchmarks. It exercises common userver features under load: plaintext and
baseline handlers, JSON responses with optional gzip middleware, static files,
multipart/upload body handling, and PostgreSQL queries.

Source: [frameworks/userver](https://github.com/MDA2AV/HttpArena/tree/main/frameworks/userver).

## Endpoints

| Path | Method | Description |
|------|--------|-------------|
| `/pipeline` | GET | Plaintext `"ok"` |
| `/baseline11` | GET, POST | Sum of query args `a`, `b` and optional POST body |
| `/baseline2` | GET | Sum of query args `a`, `b` |
| `/json/{count}` | GET | JSON items from `data/dataset.json`; supports `Accept-Encoding: gzip` |
| `/upload` | POST | Returns request body size |
| `/async-db` | GET | Filtered items from PostgreSQL (`min`, `max`, `limit`) |
| `/static/*` | GET | Static files from `data/static` |

An additional TLS listener is configured on port `8081` (`cert.crt` /
`private_key.key`).

## Build (CMake)

Requires `USERVER_FEATURE_POSTGRESQL=ON` and `USERVER_FEATURE_CHAOTIC=ON`.

```bash
cmake -S . -B build -DUSERVER_FEATURE_POSTGRESQL=ON -DUSERVER_FEATURE_CHAOTIC=ON
cmake --build build -j \
  --target userver-samples-benchmark_service
ctest --test-dir build -R userver-samples-benchmark_service -V
```

## Run manually

Start PostgreSQL, apply `schemas/postgresql/admin.sql`, then:

```bash
./userver-samples-benchmark_service -c static_config.yaml
```

Optional HttpArena-style environment:

- `DATABASE_URL` — PostgreSQL DSN
- `DATABASE_MAX_CONN` — pool size (`min_pool_size` / `max_pool_size`)

## Local benchmark (HttpArena-style)

Cap all cores at 2.1 GHz for reproducible runs (needs root / `linux-tools`):

```bash
sudo cpupower frequency-set -d 2.1GHz -u 2.1GHz
# restore later, e.g.: sudo cpupower frequency-set -d 400MHz -u 4.4GHz
```

```bash
./bench.sh --list
./bench.sh --out ~/out --bin ./userver-functional-test-service -- json
./bench.sh --out ~/out --flame --bin ./userver-functional-test-service --duration 10s --conns 128 --runs 3 -- json
```

Flags are `--long` only; `--out DIR` is required (artifacts go there, not into
the repo). Profiles after `--` (or trailing). Needs `wrk` and `curl`.
