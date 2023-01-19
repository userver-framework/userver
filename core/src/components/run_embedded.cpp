#include <userver/components/run.hpp>

#include <unistd.h>

#include <csignal>
#include <cstring>
#include <variant>

#include <boost/stacktrace/stacktrace.hpp>

#include <crypto/openssl.hpp>
#include <logging/config.hpp>

#include <fmt/format.h>

#include <components/manager_config.hpp>
#include <userver/components/manager.hpp>
#include <userver/formats/yaml/serialize.hpp>
#include <userver/fs/blocking/read.hpp>
#include <userver/logging/log.hpp>
#include <userver/logging/logger.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/traceful_exception.hpp>
#include <utils/ignore_signal_scope.hpp>
#include <utils/jemalloc.hpp>
#include <utils/signal_catcher.hpp>
#include <utils/strerror.hpp>
#include <utils/userver_experiment.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth::apikey {
extern int auth_checker_apikey_module_activation;
}  // namespace server::handlers::auth::apikey

namespace components {

namespace {

class LogScope final {
 public:
  LogScope(const std::string& init_log_path, logging::Format format) {
    if (init_log_path.empty()) {
      return;
    }

    try {
      old_default_logger_ = logging::SetDefaultLogger(
          logging::MakeFileLogger("default", init_log_path, format));
    } catch (const std::exception& e) {
      auto error_message = fmt::format(
          "Setting initial logging path to '{}' failed. ", init_log_path);
      LOG_ERROR() << error_message << e;
      throw std::runtime_error(error_message + e.what());
    }
  }

  ~LogScope() noexcept(false) {
    if (old_default_logger_) {
      logging::SetDefaultLogger(std::move(old_default_logger_));
    }
  }

 private:
  logging::LoggerPtr old_default_logger_;
};

void HandleJemallocSettings() {
  static constexpr size_t kDefaultMaxBgThreads = 1;

  auto ec = utils::jemalloc::SetMaxBgThreads(kDefaultMaxBgThreads);
  if (ec) {
    LOG_WARNING() << "Failed to set max_background_threads to "
                  << kDefaultMaxBgThreads;
  }

  if (utils::IsUserverExperimentEnabled(
          utils::UserverExperiment::kJemallocBgThread)) {
    utils::jemalloc::EnableBgThreads();
  }
}

void PreheatStacktraceCollector() {
  // If DEBUG logging is enabled the following line loads debug info from disk,
  // hopefully preventing this to occur later, e.g. in exception constructor.
  LOG_DEBUG() << utils::TracefulException{"Preheating stacktrace"};
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

void DoRun(std::mutex& stop_mutex,
	   const PathOrConfig& config,
           const std::optional<std::string>& config_vars_path,
           const std::optional<std::string>& config_vars_override_path,
           const ComponentList& component_list,
           const std::string& init_log_path, logging::Format format,
           RunMode run_mode) {

  ++server::handlers::auth::apikey::auth_checker_apikey_module_activation;
  crypto::impl::Openssl::Init();

  LogScope log_scope{init_log_path, format};

  LOG_INFO() << "Parsing configs";
  if (config_vars_path) {
    LOG_INFO() << "Using config_vars from cmdline: " << *config_vars_path
               << ". The config_vars filepath in config.yaml is ignored.";
  } else {
    LOG_INFO() << "Using config_vars from config.yaml.";
  }
  if (config_vars_override_path) {
    LOG_INFO() << "Overriding config_vars with values from file: "
               << *config_vars_override_path;
  }
  auto parsed_config = std::make_unique<ManagerConfig>(std::visit(
      ConfigToManagerVisitor{config_vars_path, config_vars_override_path},
      config));
  LOG_INFO() << "Parsed configs";

  HandleJemallocSettings();
  PreheatStacktraceCollector();

  std::optional<Manager> manager;
  try {
    manager.emplace(std::move(parsed_config), component_list);
  } catch (const std::exception& ex) {
    LOG_ERROR() << "Loading failed: " << ex;
    throw;
  }

  stop_mutex.lock();
}

}  // namespace

void RunEmbedded(std::mutex& stop_mutex,
		 const InMemoryConfig& config,
		 const ComponentList& component_list,
                 const std::string& init_log_path, logging::Format format) {
  DoRun(stop_mutex, config, {}, {}, component_list, init_log_path, format,
        RunMode::kNormal);
}

}  // namespace components

USERVER_NAMESPACE_END
