#include <userver/components/run.hpp>

#include <unistd.h>

#include <csignal>
#include <cstring>
#include <iostream>
#include <variant>

#include <fmt/format.h>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/stacktrace/stacktrace.hpp>

#include <components/manager.hpp>
#include <components/manager_config.hpp>
#include <crypto/openssl.hpp>
#include <logging/config.hpp>
#include <logging/tp_logger_utils.hpp>
#include <userver/dynamic_config/snapshot.hpp>
#include <userver/dynamic_config/value.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/fs/blocking/read.hpp>
#include <userver/logging/log.hpp>
#include <userver/logging/null_logger.hpp>
#include <userver/logging/stacktrace_cache.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/utils/impl/static_registration.hpp>
#include <userver/utils/impl/userver_experiments.hpp>
#include <userver/utils/overloaded.hpp>
#include <userver/utils/traceful_exception.hpp>
#include <utils/ignore_signal_scope.hpp>
#include <utils/jemalloc.hpp>
#include <utils/signal_catcher.hpp>
#include <utils/strerror.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth::apikey {
extern int auth_checker_apikey_module_activation;
}  // namespace server::handlers::auth::apikey

namespace components {

namespace {

class LogScope final {
 public:
  LogScope()
      : logger_prev_{logging::GetDefaultLogger()},
        level_scope_{logging::GetDefaultLoggerLevel()} {}

  ~LogScope() { logging::impl::SetDefaultLoggerRef(logger_prev_); }

  void SetLogger(logging::LoggerPtr logger) {
    UASSERT(logger);
    logging::impl::SetDefaultLoggerRef(*logger);
    // Destroys the old logger_new_
    logger_new_ = std::move(logger);
  }

 private:
  logging::LoggerPtr logger_new_;
  logging::LoggerRef logger_prev_;
  logging::DefaultLoggerLevelScope level_scope_;
};

const utils::impl::UserverExperiment kJemallocBgThread{"jemalloc-bg-thread"};

void HandleJemallocSettings() {
  static constexpr size_t kDefaultMaxBgThreads = 1;

  auto ec = utils::jemalloc::SetMaxBgThreads(kDefaultMaxBgThreads);
  if (ec) {
    LOG_WARNING() << "Failed to set max_background_threads to "
                  << kDefaultMaxBgThreads;
  }

  if (kJemallocBgThread.IsEnabled()) {
    utils::jemalloc::EnableBgThreads();
  }
}

void PreheatStacktraceCollector() {
  const auto now = [] { return std::chrono::steady_clock::now(); };

  const auto start = now();
  const auto dummy_stacktrace =
      logging::stacktrace_cache::to_string(boost::stacktrace::stacktrace{});
  const auto finish = now();

  const auto initialization_duration_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(finish - start)
          .count();
  if (dummy_stacktrace.size() == 0) {
    LOG_WARNING()
        << "Failed to initialize stacktrace collector, an attempt took "
        << initialization_duration_ms << "ms";
  } else {
    LOG_INFO() << "Initialized stacktrace collector within "
               << initialization_duration_ms << "ms";
  }
}

bool IsTraced() {
  static const std::string kTracerField = "TracerPid:\t";

  try {
    // /proc is only available on linux,
    // on macos this will always throw and return false.
    const std::string proc_status =
        fs::blocking::ReadFileContents("/proc/self/status");
    auto tracer_pos = proc_status.find(kTracerField);
    if (tracer_pos != std::string::npos) {
      return proc_status[tracer_pos + kTracerField.size()] != '0';
    }
  } catch (const std::exception&) {
    // ignore
  }
  return false;
}

enum class RunMode { kNormal, kOnce };

using PathOrConfig = std::variant<std::string, components::InMemoryConfig>;

struct ConfigToManagerVisitor {
  const std::optional<std::string>& config_vars_path;
  const std::optional<std::string>& config_vars_override_path;

  ManagerConfig operator()(const std::string& path) const {
    return ManagerConfig::FromFile(path, config_vars_path,
                                   config_vars_override_path);
  }

  ManagerConfig operator()(const components::InMemoryConfig& config) const {
    return ManagerConfig::FromString(config.GetUnderlying(), config_vars_path,
                                     config_vars_override_path);
  }
};

ManagerConfig ParseManagerConfigAndSetupLogging(
    LogScope& log_scope, const PathOrConfig& config,
    const std::optional<std::string>& config_vars_path,
    const std::optional<std::string>& config_vars_override_path) {
  std::string details = "configs from ";
  details +=
      std::visit(utils::Overloaded{[](const std::string& path) {
                                     return fmt::format("file '{}'", path);
                                   },
                                   [](const InMemoryConfig&) {
                                     return std::string{"in-memory config"};
                                   }},
                 config);
  if (config_vars_path) {
    details += fmt::format(" using config_vars from cmdline in file '{}'",
                           *config_vars_path);
  }
  if (config_vars_override_path) {
    details += fmt::format(" overriding config_vars with values from file '{}'",
                           *config_vars_override_path);
  }

  try {
    auto manager_config = std::visit(
        ConfigToManagerVisitor{config_vars_path, config_vars_override_path},
        config);

    const auto default_logger_config =
        logging::impl::ExtractDefaultLoggerConfig(manager_config);
    auto default_logger = logging::impl::MakeTpLogger(default_logger_config);

    // This line enables basic logging. Any LOG_XXX before it is meaningless,
    // because it would typically go straight to a NullLogger.
    log_scope.SetLogger(std::move(default_logger));

    LOG_INFO() << "Parsed " << details;
    return manager_config;
  } catch (const std::exception& ex) {
    throw std::runtime_error(
        fmt::format("Error while parsing {}. Details: {}", details, ex.what()));
  }
}

void DoRun(const PathOrConfig& config,
           const std::optional<std::string>& config_vars_path,
           const std::optional<std::string>& config_vars_override_path,
           const ComponentList& component_list, RunMode run_mode) {
  utils::impl::FinishStaticRegistration();

  utils::SignalCatcher signal_catcher{SIGINT, SIGTERM, SIGQUIT, SIGUSR1,
                                      SIGUSR2};
  const utils::IgnoreSignalScope ignore_sigpipe_scope(SIGPIPE);

  ++server::handlers::auth::apikey::auth_checker_apikey_module_activation;
  crypto::impl::Openssl::Init();

  LogScope log_scope;
  auto manager_config = ParseManagerConfigAndSetupLogging(
      log_scope, config, config_vars_path, config_vars_override_path);

  utils::impl::UserverExperimentsScope experiments_scope;
  std::optional<Manager> manager;

  try {
    experiments_scope.EnableOnly(manager_config.enabled_experiments,
                                 manager_config.experiments_force_enabled);

    HandleJemallocSettings();
    if (manager_config.preheat_stacktrace_collector) {
      PreheatStacktraceCollector();
    }

    manager.emplace(std::make_unique<ManagerConfig>(std::move(manager_config)),
                    component_list);
  } catch (const std::exception& ex) {
    LOG_ERROR() << "Loading failed: " << ex;
    throw;
  }

  if (run_mode == RunMode::kOnce) return;

  for (;;) {
    auto signum = signal_catcher.Catch();
    if (signum == SIGTERM || signum == SIGQUIT) {
      break;
    } else if (signum == SIGINT) {
      if (IsTraced()) {
        // SIGINT is masked and cannot be used
        std::raise(SIGTRAP);
      } else {
        break;
      }
    } else if (signum == SIGUSR1 || signum == SIGUSR2) {
      LOG_INFO() << "Signal caught: " << utils::strsignal(signum);
      manager->OnSignal(signum);
    } else {
      LOG_WARNING() << "Got unexpected signal: " << signum << " ("
                    << utils::strsignal(signum) << ')';
      UASSERT_MSG(false, "unexpected signal");
    }
  }
}

}  // namespace

void Run(const std::string& config_path,
         const std::optional<std::string>& config_vars_path,
         const std::optional<std::string>& config_vars_override_path,
         const ComponentList& component_list) {
  DoRun(config_path, config_vars_path, config_vars_override_path,
        component_list, RunMode::kNormal);
}

void RunOnce(const std::string& config_path,
             const std::optional<std::string>& config_vars_path,
             const std::optional<std::string>& config_vars_override_path,
             const ComponentList& component_list) {
  DoRun(config_path, config_vars_path, config_vars_override_path,
        component_list, RunMode::kOnce);
}

void Run(const InMemoryConfig& config, const ComponentList& component_list) {
  DoRun(config, {}, {}, component_list, RunMode::kNormal);
}

void RunOnce(const InMemoryConfig& config,
             const ComponentList& component_list) {
  DoRun(config, {}, {}, component_list, RunMode::kOnce);
}

namespace impl {

std::string GetStaticConfigSchema(const ComponentList& component_list) {
  auto manager_schema = components::GetManagerConfigSchema();
  UASSERT(manager_schema.properties);
  (*manager_schema.properties)
      .insert_or_assign(
          "components",
          yaml_config::SchemaPtr(component_list.GetStaticConfigSchema()));

  auto schema = yaml_config::Schema::EmptyObject();
  schema.UpdateDescription("Root object");
  schema.properties.emplace();
  schema.properties->emplace("components_manager",
                             yaml_config::SchemaPtr(std::move(manager_schema)));

  return ToString(formats::yaml::ValueBuilder{schema}.ExtractValue()) + "\n";
}

std::string GetDynamicConfigDefaults() {
  return formats::json::ToPrettyString(
      dynamic_config::impl::MakeDefaultDocsMap().AsJson());
}

}  // namespace impl

}  // namespace components

USERVER_NAMESPACE_END
