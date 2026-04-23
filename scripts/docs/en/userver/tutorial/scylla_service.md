## ScyllaDB service

## Before you start

Make sure that you can compile and run core tests and read a basic example
@ref scripts/docs/en/userver/tutorial/hello_service.md.


## Step by step guide

In this tutorial we will write a service that demonstrates the ScyllaDB driver.
Typed CRUD with operation builders, logged batch inserts, lightweight
transactions, opaque-cursor paging, CQL types and raw CQL fallbacks.

### HTTP handler component

Like in @ref scripts/docs/en/userver/tutorial/hello_service.md we create a
component for handling HTTP requests:

```cpp
class KeyValueHandler final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-kv";

    KeyValueHandler(const components::ComponentConfig& config, const components::ComponentContext& context)
        : HttpHandlerBase(config, context),
          session_(context.FindComponent<components::Scylla>("scylla-db").GetSession()) {}

    std::string HandleRequest(server::http::HttpRequest& request, server::request::RequestContext&) const override {
        request.GetHttpResponse().SetContentType(http::content_type::kApplicationJson);
        switch (request.GetMethod()) {
            case server::http::HttpMethod::kGet:  return Get(request);
            case server::http::HttpMethod::kPost: return Post(request);
            default: BadRequest(fmt::format("Unsupported method {}", request.GetMethod()));
        }
    }

private:
    std::string Get(server::http::HttpRequest& request) const;
    std::string Post(const server::http::HttpRequest& request) const;

    storages::scylla::SessionPtr session_;
};
```

The component holds a storages::scylla::SessionPtr, a client to the
ScyllaDB cluster. That client is thread safe, you can use it concurrently from
different threads and tasks. 


### KeyValueHandler::Post

Writes to ScyllaDB are expressed through typed operation builders. For an
insert into basic we bind the partition key explicitly and forward the
optional scalar columns via a small helper that maps JSON fields.

```cpp
storages::scylla::operations::InsertOne op;

op.BindString("key", body["key"].As<std::string>());
BindBasicFields(op, body);

if (body.HasMember("ttl")) {
    op.UsingTtl(body["ttl"].As<std::int32_t>());
}

session_->GetTable("basic").Execute(op);
```

### KeyValueHandler::Get

Reads use storages::scylla::operations::SelectOne. `AddAllColumns` selects
every column declared on the table. `WhereString` adds an equality restriction
on the partition key. An empty Row signals that no row matched.

```cpp
storages::scylla::operations::SelectOne op;

op.AddAllColumns();
op.WhereString("key", RequireArg(request, "key"));

auto row = session_->GetTable("basic").Execute(op);
if (row.Empty()) {
    request.SetResponseStatus(server::http::HttpStatus::kNotFound);
    return R"({"error":"not_found"})";
}

return formats::json::ToString(RowToJson(row).ExtractValue());
```


### KeyValueHandler::Patch and Delete

Updates and deletes follow the same builder pattern. UpdateOne::Set stages
column assignments.

```cpp
storages::scylla::operations::UpdateOne op;

for (auto name : kOptionalFields) {
    if (body.HasMember(name)) {
        op.Set(std::string{name}, JsonToBasicValue(name, body[name]));
    }
}

op.WhereString("key", RequireArg(request, "key"));
if (body.HasMember("ttl")) {
    op.UsingTtl(body["ttl"].As<std::int32_t>());
}

session_->GetTable("basic").Execute(op);
```


### Bulk inserts

Writes to many rows in one round trip go through
storages::scylla::operations::InsertMany. Between rows call NextRow(), so the
driver compiles the whole operation into a single logged batch statement.

```cpp
storages::scylla::operations::InsertMany op;

for (std::size_t i = 0; i < body.GetSize(); ++i) {
    if (i > 0) op.NextRow();
    op.BindString("key", body[i]["key"].As<std::string>());
    BindBasicFields(op, body[i]);
}

session_->GetTable("basic").Execute(op);
```


### Lightweight transactions

ScyllaDB offers Paxos-based lightweight transactions for conditional writes.
Use ExecuteLwt instead of Execute.

```cpp
storages::scylla::operations::InsertOne op;
op.BindString("key", body["key"].As<std::string>());

BindBasicFields(op, body);
op.IfNotExists();

const auto result = session_->GetTable("basic").ExecuteLwt(op);
auto response = JsonObject()SelectOne;

response["applied"] = result.applied;

if (!result.applied) {
    request.SetResponseStatus(server::http::HttpStatus::kConflict);
    response["existing"] = RowToJson(result.previous);
}
```

### Paging

For large result sets, storages::scylla::operations::SelectMany together with
ExecutePaged gives you one page plus an opaque cursor. 

```cpp
storages::scylla::operations::SelectMany op;

op.AddAllColumns();
op.SetPageSize(page_size);

std::string cursor;
if (request.HasArg("cursor")) {
    cursor = FromHex(request.GetArg("cursor"));
}

auto result = session_->GetTable("basic").ExecutePaged(op, std::move(cursor));
```

```cpp
auto cursor = session_->NewCursor("SELECT * FROM examples.events", {}, page_size);

while (!cursor.Done()) {
    if (auto page = cursor.NextPage()) {
    }
}
```


### Rich CQL types

Examples of the non-scalar CQL types. 

```cpp
storages::scylla::operations::InsertOne op;

op.BindUuid("id", storages::scylla::Uuid::Random());
op.BindString("name", body["name"].As<std::string>());
op.BindTimestamp("created_at", std::chrono::system_clock::now());
op.BindInet("source_ip", storages::scylla::Inet{body["source_ip"].As<std::string>()});

storages::scylla::Set tags;

for (std::size_t i = 0; i < body["tags"].GetSize(); ++i) {
    tags.items.emplace_back(body["tags"][i].As<std::string>());
}

op.BindSet("tags", std::move(tags));
```

```cpp
struct Event {
    storages::scylla::Uuid id;
    std::string name;
};

void DecodeRow(const storages::scylla::Row& row, Event& out) {
    out.id = row.Get<storages::scylla::Uuid>("id");
    out.name = row.Get<std::string>("name");
}

storages::scylla::operations::SelectOne select;

select.AddAllColumns();
select.WhereUuid("id", id);

auto event = session_->GetTable("events").Execute(select).As<Event>();
```

See @ref scripts/docs/en/userver/scylladb.md for ScyllaDB hints and more usage
samples.


### Raw CQL and schema init

Not every statement fits an operation builder. For those the session
exposes a variadic Execute and ExecuteVoid pair.

```cpp
auto rows = session_->Execute(
    "SELECT key, i32 FROM examples.basic WHERE key = ?",
    std::string{"alice"});

session_->ExecuteVoid(
    "CREATE TABLE IF NOT EXISTS examples.basic ("
    "key text PRIMARY KEY, bln boolean, i32 int, i64 bigint, "
    "flt float, dbl double)");
```

The sample uses the ExecuteVoid form inside its handler-schema-init
endpoint so the functional tests can bring the schema up without running
cqlsh.


### Static config

Static configuration of service is quite close to the configuration from
@ref scripts/docs/en/userver/tutorial/hello_service.md except for the handlers
and DB. Secdist carries the  cluster contact points so 
they are not checked into the config file.

```yaml
components_manager:
    components:
        scylla-db:
            dbconnection: scylla
            consistency: local_quorum
            serial_consistency: local_serial
            request_timeout: 10s
            pool:
                num_threads_io: 2
                core_connections_per_host: 4
            app_name: scylla_sample
            retry_policy: default
            load_balancing_policy: round_robin
            default_keyspace: examples

        secdist:
            provider: default-secdist-provider
        default-secdist-provider:
            config: ../secdist.json

        handler-kv:
            path: /v1/kv
            method: GET,POST,PATCH,DELETE
            task_processor: main-task-processor
```

```json
{
    "scylla_settings": {
        "scylla_example": {
            "hosts": "scylla"
        }
    }
}
```

There are more static options for the ScyllaDB component configuration, all of
them are described at components::Scylla.


### int main()

Finally, we add our component to the `components::MinimalServerComponentList()`.

```cpp
int main(int argc, char* argv[]) {
    const auto component_list =
        components::MinimalServerComponentList()
            .Append<clients::dns::Component>()
            .Append<components::Scylla>("scylla-db")
            .Append<samples::scylladb::KeyValueHandler>();
    return utils::DaemonMain(argc, argv, component_list);
}
```


### Build and Run

To build the sample, execute the following build steps at the userver root directory:
```
mkdir build_release
cd build_release
cmake -DCMAKE_BUILD_TYPE=Release ..
make userver-samples-scylla_service
```

The sample could be started by running
`make start-userver-samples-scylla_service`. The command would invoke
@ref scripts/docs/en/userver/functional_testing.md "testsuite start target" that sets proper
paths in the configuration files, prepares and starts the DB, and starts the
service.

To start the service manually start the DB server and run
`./samples/scylla_service/userver-samples-scylla_service -c </path/to/static_config.yaml>`.

Now you can send a request to your service from another terminal:
```
bash
$ curl -sS -X POST 'http://localhost:8080/v1/schema/init'
{"status":"ok","tables":["basic","events"]}
$ curl -sS -X POST 'http://localhost:8080/v1/kv' \
    -H 'Content-Type: application/json' \
    -d '{"key":"alpha","bln":true,"i32":1,"i64":42,"flt":1.5,"dbl":2.5}'
{"status":"ok"}
$ curl -sS 'http://localhost:8080/v1/kv?key=alpha' | jq
{
  "key": "alpha",
  "bln": true,
  "i32": 1,
  "i64": 42,
  "flt": 1.5,
  "dbl": 2.5
}
$ curl -sS -X POST 'http://localhost:8080/v1/kv/create_if_absent' \
    -H 'Content-Type: application/json' \
    -d '{"key":"alpha","i32":0}' -w '\n%{http_code}\n'
{"applied":false,"existing":{"key":"alpha","i32":1,...}}
409
```


### Functional testing
@ref scripts/docs/en/userver/functional_testing.md "Functional tests" for the service could be
implemented using the testsuite. To do that you have to:

* Turn on the `pytest_userver.plugins.scylla` plugin and provide ScyllaDB
  connection info for the testsuite:
  @snippet samples/scylla_service/testsuite/conftest.py scylla setup
  The `pytest_userver.plugins.service.auto_client_deps()` fixture already knows
  about the scylla fixture, so there's no need to override the
  `extra_client_deps()` fixture. The sample's `conftest.py` additionally calls
  `/v1/schema/init` and truncates both tables before every test so each case
  starts from a clean slate.

* Write the test:
  @snippet samples/scylla_service/testsuite/test_scylla.py  Functional test

## Full sources

See the full example:
* @ref samples/scylla_service/src/main.cpp
* @ref samples/scylla_service/src/helpers.hpp
* @ref samples/scylla_service/src/helpers.cpp
* @ref samples/scylla_service/static_config.yaml
* @ref samples/scylla_service/secdist.json
* @ref samples/scylla_service/CMakeLists.txt
* @ref samples/scylla_service/testsuite/conftest.py
* @ref samples/scylla_service/testsuite/test_scylla.py

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/tutorial/redis_service.md | @ref scripts/docs/en/userver/scylladb.md ⇨
@htmlonly </div> @endhtmlonly


@example samples/scylla_service/src/main.cpp
@example samples/scylla_service/src/helpers.hpp
@example samples/scylla_service/src/helpers.cpp
@example samples/scylla_service/static_config.yaml
@example samples/scylla_service/secdist.json
@example samples/scylla_service/CMakeLists.txt
@example samples/scylla_service/testsuite/conftest.py
@example samples/scylla_service/testsuite/test_scylla.py
