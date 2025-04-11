#include <userver/easy.hpp>

#include <fstream>
#include <iostream>
#include <unordered_map>

#include <fmt/ranges.h>
#include <boost/algorithm/string/replace.hpp>
#include <boost/program_options.hpp>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/components/run.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

USERVER_NAMESPACE_BEGIN

namespace easy {

namespace {

constexpr std::string_view kConfigBase = R"~(# yaml
components_manager:
    task_processors:                  # Task processor is an executor for coroutine tasks
        main-task-processor:          # Make a task processor for CPU-bound coroutine tasks.
            worker_threads: 4         # Process tasks in 4 threads.

        fs-task-processor:            # Make a separate task processor for filesystem bound tasks.
            worker_threads: 1

    default_task_processor: main-task-processor  # Task processor in which components start.

    components:                       # Configuring components that were registered via component_list)~";

constexpr std::string_view kConfigServerTemplate = R"~(
        server:
            listener:                 # configuring the main listening socket...
                port: {}            # ...to listen on this port and...
                task_processor: main-task-processor    # ...process incoming requests on this task processor.
)~";

constexpr std::string_view kConfigLoggingTemplate = R"~(
        logging:
            fs-task-processor: fs-task-processor
            loggers:
                default:
                    file_path: '@stderr'
                    level: {}
                    overflow_behavior: discard  # Drop logs if the system is too busy to write them down.
)~";

constexpr std::string_view kConfigHandlerTemplate{
    "path: {0}                  # Registering handler by URL '{0}'.\n"
    "method: {1}\n"
    "task_processor: main-task-processor  # Run it on CPU bound task processor\n"};

struct SharedPyaload {
    std::unordered_map<std::string, HttpBase::Callback> http_functions;
    std::optional<http::ContentType> default_content_type;
    std::string db_schema;
};

SharedPyaload globals{};

}  // anonymous namespace

namespace impl {

DependenciesBase::~DependenciesBase() = default;

}  // namespace impl

class HttpBase::Handle final : public server::handlers::HttpHandlerBase {
public:
    Handle(const components::ComponentConfig& config, const components::ComponentContext& context)
        : HttpHandlerBase(config, context),
          deps_{context.FindComponent<impl::DependenciesBase>()},
          callback_{globals.http_functions.at(config.Name())} {}

    std::string HandleRequestThrow(const server::http::HttpRequest& request, server::request::RequestContext&)
        const override {
        if (globals.default_content_type) {
            request.GetHttpResponse().SetContentType(*globals.default_content_type);
        }
        return callback_(request, deps_);
    }

private:
    const impl::DependenciesBase& deps_;
    HttpBase::Callback& callback_;
};

HttpBase::HttpBase(int argc, const char* const argv[])
    : argc_{argc},
      argv_{argv},
      static_config_{kConfigBase},
      component_list_{components::MinimalServerComponentList()} {}

HttpBase::~HttpBase() {
    static_config_.append(fmt::format(kConfigServerTemplate, port_));
    static_config_.append(fmt::format(kConfigLoggingTemplate, ToString(level_)));

    namespace po = boost::program_options;
    po::variables_map vm;
    auto desc = utils::BaseRunOptions();
    std::string config_dump;
    std::string schema_dump;

    // clang-format off
    desc.add_options()
      ("dump-config", po::value(&config_dump)->implicit_value(""), "path to dump the server config")
      ("dump-db-schema", po::value(&schema_dump)->implicit_value(""), "path to dump the DB schema")
      ("config,c", po::value<std::string>(), "path to server config")
    ;
    // clang-format on

    po::store(po::parse_command_line(argc_, argv_, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cerr << desc << '\n';
        return;
    }

    if (vm.count("dump-config")) {
        if (config_dump.empty()) {
            std::cout << static_config_ << std::endl;
        } else {
            std::ofstream(config_dump) << static_config_;
        }
        return;
    }

    if (vm.count("dump-db-schema")) {
        if (schema_dump.empty()) {
            std::cout << schema_dump << std::endl;
        } else {
            std::ofstream(schema_dump) << globals.db_schema;
        }
        return;
    }

    if (argc_ <= 1) {
        components::Run(components::InMemoryConfig{static_config_}, component_list_);
    } else {
        const auto ret = utils::DaemonMain(vm, component_list_);
        if (ret != 0) {
            std::exit(ret);  // NOLINT(concurrency-mt-unsafe)
        }
    }
}

void HttpBase::DefaultContentType(http::ContentType content_type) { globals.default_content_type = content_type; }

void HttpBase::Route(std::string_view path, Callback&& func, std::initializer_list<server::http::HttpMethod> methods) {
    auto component_name = fmt::format("{}-{}", path, fmt::join(methods, ","));

    globals.http_functions.emplace(component_name, std::move(func));
    component_list_.Append<Handle>(component_name);
    AddComponentConfig(component_name, fmt::format(kConfigHandlerTemplate, path, fmt::join(methods, ",")));
}

void HttpBase::AddComponentConfig(std::string_view component, std::string_view config) {
    static_config_ += fmt::format("\n        {}:", component);
    if (config.empty()) {
        static_config_ += " {}\n";
    } else {
        if (config.back() == '\n') {
            config = std::string_view{config.data(), config.size() - 1};
        }
        static_config_ += boost::algorithm::replace_all_copy("\n" + std::string{config}, "\n", "\n            ");
        static_config_ += '\n';
    }
}

void HttpBase::DbSchema(std::string_view schema) { globals.db_schema = schema; }

const std::string& HttpBase::GetDbSchema() noexcept { return globals.db_schema; }

void HttpBase::Port(std::uint16_t port) { port_ = port; }

void HttpBase::LogLevel(logging::Level level) { level_ = level; }

PgDep::PgDep(const components::ComponentContext& context)
    : pg_cluster_(context.FindComponent<components::Postgres>("postgres").GetCluster()) {
    const auto& db_schema = HttpBase::GetDbSchema();
    if (!db_schema.empty()) {
        pg_cluster_->Execute(storages::postgres::ClusterHostType::kMaster, db_schema);
    }
}

void PgDep::RegisterOn(HttpBase& app) {
    app.TryAddComponent<components::Postgres>(
        "postgres",
        "dbconnection#env: POSTGRESQL\n"
        "dbconnection#fallback: 'postgresql://testsuite@localhost:15433/postgres'\n"
        "blocking_task_processor: fs-task-processor\n"
        "dns_resolver: async\n"
    );

    app.TryAddComponent<components::TestsuiteSupport>(components::TestsuiteSupport::kName, "");
    app.TryAddComponent<clients::dns::Component>(
        clients::dns::Component::kName, "fs-task-processor: fs-task-processor"
    );
}

HttpDep::HttpDep(const components::ComponentContext& context)
    : http_(context.FindComponent<components::HttpClient>().GetHttpClient()) {}

void HttpDep::RegisterOn(easy::HttpBase& app) {
    app.TryAddComponent<components::HttpClient>(
        components::HttpClient::kName,
        "pool-statistics-disable: false\n"
        "thread-name-prefix: http-client\n"
        "threads: 2\n"
        "fs-task-processor: fs-task-processor\n"
    );
}

}  // namespace easy

USERVER_NAMESPACE_END
