#include <userver/clients/dns/component.hpp>
#include <userver/fs/blocking/temp_file.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/utils/assert.hpp>

#include <userver/utest/using_namespace_userver.hpp>

#include <userver/testsuite/testsuite_support.hpp>

/// [Postgres service sample - component]
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/components/run.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>

namespace samples::pg {

class KeyValue final : public server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-key-value";

  KeyValue(const components::ComponentConfig& config,
           const components::ComponentContext& context);

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&) const override;

 private:
  std::string GetValue(std::string_view key,
                       const server::http::HttpRequest& request) const;
  std::string PostValue(std::string_view key,
                        const server::http::HttpRequest& request) const;
  std::string DeleteValue(std::string_view key) const;

  storages::postgres::ClusterPtr pg_cluster_;
};

}  // namespace samples::pg
/// [Postgres service sample - component]

namespace samples::pg {

/// [Postgres service sample - component constructor]
KeyValue::KeyValue(const components::ComponentConfig& config,
                   const components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_cluster_(
          context.FindComponent<components::Postgres>("key-value-database")
              .GetCluster()) {
  constexpr auto kCreateTable = R"~(
      CREATE TABLE IF NOT EXISTS key_value_table (
        key VARCHAR PRIMARY KEY,
        value VARCHAR
      )
    )~";

  using storages::postgres::ClusterHostType;
  pg_cluster_->Execute(ClusterHostType::kMaster, kCreateTable);
}
/// [Postgres service sample - component constructor]

/// [Postgres service sample - HandleRequestThrow]
std::string KeyValue::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext&) const {
  const auto& key = request.GetArg("key");
  if (key.empty()) {
    throw server::handlers::ClientError(
        server::handlers::ExternalBody{"No 'key' query argument"});
  }

  switch (request.GetMethod()) {
    case server::http::HttpMethod::kGet:
      return GetValue(key, request);
    case server::http::HttpMethod::kPost:
      return PostValue(key, request);
    case server::http::HttpMethod::kDelete:
      return DeleteValue(key);
    default:
      throw server::handlers::ClientError(server::handlers::ExternalBody{
          fmt::format("Unsupported method {}", request.GetMethod())});
  }
}
/// [Postgres service sample - HandleRequestThrow]

/// [Postgres service sample - GetValue]
const storages::postgres::Query kSelectValue{
    "SELECT value FROM key_value_table WHERE key=$1",
    storages::postgres::Query::Name{"sample_select_value"},
};

std::string KeyValue::GetValue(std::string_view key,
                               const server::http::HttpRequest& request) const {
  storages::postgres::ResultSet res = pg_cluster_->Execute(
      storages::postgres::ClusterHostType::kSlave, kSelectValue, key);
  if (res.IsEmpty()) {
    request.SetResponseStatus(server::http::HttpStatus::kNotFound);
    return {};
  }

  return res.AsSingleRow<std::string>();
}
/// [Postgres service sample - GetValue]

/// [Postgres service sample - PostValue]
const storages::postgres::Query kInsertValue{
    "INSERT INTO key_value_table (key, value) "
    "VALUES ($1, $2) "
    "ON CONFLICT DO NOTHING",
    storages::postgres::Query::Name{"sample_insert_value"},
};

std::string KeyValue::PostValue(
    std::string_view key, const server::http::HttpRequest& request) const {
  const auto& value = request.GetArg("value");

  storages::postgres::Transaction transaction =
      pg_cluster_->Begin("sample_transaction_insert_key_value",
                         storages::postgres::ClusterHostType::kMaster, {});

  auto res = transaction.Execute(kInsertValue, key, value);
  if (res.RowsAffected()) {
    transaction.Commit();
    request.SetResponseStatus(server::http::HttpStatus::kCreated);
    return std::string{value};
  }

  res = transaction.Execute(kSelectValue, key);
  transaction.Rollback();

  auto result = res.AsSingleRow<std::string>();
  if (result != value) {
    request.SetResponseStatus(server::http::HttpStatus::kConflict);
  }

  return res.AsSingleRow<std::string>();
}
/// [Postgres service sample - PostValue]

/// [Postgres service sample - DeleteValue]
std::string KeyValue::DeleteValue(std::string_view key) const {
  auto res =
      pg_cluster_->Execute(storages::postgres::ClusterHostType::kMaster,
                           "DELETE FROM key_value_table WHERE key=$1", key);
  return std::to_string(res.RowsAffected());
}
/// [Postgres service sample - DeleteValue]

}  // namespace samples::pg

constexpr std::string_view kDynamicConfig =
    /** [Postgres service sample - dynamic config] */ R"~(
{
  "USERVER_TASK_PROCESSOR_PROFILER_DEBUG": {},
  "USERVER_LOG_REQUEST": true,
  "USERVER_LOG_REQUEST_HEADERS": false,
  "USERVER_CHECK_AUTH_IN_HANDLERS": false,
  "USERVER_CANCEL_HANDLE_REQUEST_BY_DEADLINE": false,
  "USERVER_RPS_CCONTROL_CUSTOM_STATUS": {},
  "USERVER_HTTP_PROXY": "",
  "USERVER_TASK_PROCESSOR_QOS": {
    "default-service": {
      "default-task-processor": {
        "wait_queue_overload": {
          "action": "ignore",
          "length_limit": 5000,
          "time_limit_us": 3000
        }
      }
    }
  },
  "USERVER_CACHES": {},
  "USERVER_LRU_CACHES": {},
  "USERVER_DUMPS": {},
  "POSTGRES_CONNECTION_POOL_SETTINGS": {
    "key-value-database": {
      "min_pool_size": 8,
      "max_pool_size": 15,
      "max_queue_size": 200
    }
  },
  "POSTGRES_STATEMENT_METRICS_SETTINGS": {
    "key-value-database": {
      "max_statement_metrics": 5
    }
  },
  "POSTGRES_DEFAULT_COMMAND_CONTROL": {
    "network_timeout_ms": 750,
    "statement_timeout_ms": 500
  },
  "POSTGRES_HANDLERS_COMMAND_CONTROL": {
    "/v1/key-value": {
      "DELETE": {
        "network_timeout_ms": 500,
        "statement_timeout_ms": 250
      }
    }
  },
  "POSTGRES_QUERIES_COMMAND_CONTROL": {
    "sample_select_value": {
      "network_timeout_ms": 70,
      "statement_timeout_ms": 40
    },
    "sample_transaction_insert_key_value": {
      "network_timeout_ms": 200,
      "statement_timeout_ms": 150
    }
  }
}
)~"; /** [Postgres service sample - dynamic config] */

// clang-format off
constexpr std::string_view kStaticConfigSample = R"~(
# /// [Postgres service sample - static config]
# yaml
components_manager:
    components:                       # Configuring components that were registered via component_list
        handler-key-value:
            path: /v1/key-value                  # Registering handler by URL '/v1/key-value'.
            task_processor: main-task-processor  # Run it on CPU bound task processor

        key-value-database:
            dbconnection: 'postgresql://testsuite@localhost:5433/postgres'
            blocking_task_processor: fs-task-processor
            handlers_cmd_ctl_task_data_path_key: http-handler-path      # required for POSTGRES_HANDLERS_COMMAND_CONTROL
            handlers_cmd_ctl_task_data_method_key: http-request-method  # required for POSTGRES_HANDLERS_COMMAND_CONTROL
            dns_resolver: async

        testsuite-support:

        server:
            # ...
# /// [Postgres service sample - static config]
            listener:                 # configuring the main listening socket...
                port: 8086            # ...to listen on this port and...
                task_processor: main-task-processor    # ...process incomming requests on this task processor.
        logging:
            fs-task-processor: fs-task-processor
            loggers:
                default:
                    file_path: '@stdout'
                    level: debug
                    overflow_behavior: discard  # Drop logs if the system is too busy to write them down.

        tracer:                           # Component that helps to trace execution times and requests in logs.
            service-name: hello-service   # "You know. You all know exactly who I am. Say my name. " (c)

        taxi-config:                      # Dynamic config storage options, do nothing
            fs-cache-path: ''
        taxi-config-fallbacks:            # Load options from file and push them into the dynamic config storage.
            fallback-path: /etc/postgres-service/dynamic_cfg.json
        manager-controller:
        statistics-storage:
        auth-checker-settings:
        dns-client:
            fs-task-processor: fs-task-processor
    coro_pool:
        initial_size: 500             # Preallocate 500 coroutines at startup.
        max_size: 1000                # Do not keep more than 1000 preallocated coroutines.

    task_processors:                  # Task processor is an executor for coroutine tasks

        main-task-processor:          # Make a task processor for CPU-bound couroutine tasks.
            worker_threads: 4         # Process tasks in 4 threads.
            thread_name: main-worker  # OS will show the threads of this task processor with 'main-worker' prefix.

        fs-task-processor:            # Make a separate task processor for filesystem bound tasks.
            thread_name: fs-worker
            worker_threads: 4

    default_task_processor: main-task-processor
)~";
// clang-format on

// Ad-hoc solution to prepare the environment for tests
const std::string kStaticConfig = [](std::string static_conf) {
  static const auto conf_cache = fs::blocking::TempFile::Create();

  const std::string_view kOrigPath{"/etc/postgres-service/dynamic_cfg.json"};
  const auto replacement_pos = static_conf.find(kOrigPath);
  UASSERT(replacement_pos != std::string::npos);
  static_conf.replace(replacement_pos, kOrigPath.size(), conf_cache.GetPath());

  // Use a proper persistent file in production with manually filled values!
  fs::blocking::RewriteFileContents(conf_cache.GetPath(), kDynamicConfig);

  return static_conf;
}(std::string{kStaticConfigSample});

/// [Postgres service sample - main]
int main() {
  const auto component_list =
      components::MinimalServerComponentList()
          .Append<samples::pg::KeyValue>()
          .Append<components::Postgres>("key-value-database")
          .Append<components::TestsuiteSupport>()
          .Append<clients::dns::Component>();
  components::Run(components::InMemoryConfig{kStaticConfig}, component_list);
}
/// [Postgres service sample - main]
