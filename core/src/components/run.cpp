#include <components/run.hpp>

#include <unistd.h>

#include <csignal>
#include <cstring>

#include <boost/filesystem/operations.hpp>
#include <boost/stacktrace/stacktrace.hpp>

#include <components/manager.hpp>
#include <crypto/openssl.hpp>
#include <fs/blocking/read.hpp>
#include <logging/config.hpp>
#include <logging/log.hpp>
#include <logging/logger.hpp>
#include <utils/assert.hpp>
#include <utils/ignore_signal_scope.hpp>
#include <utils/jemalloc.hpp>
#include <utils/signal_catcher.hpp>
#include <utils/strerror.hpp>
#include "manager_config.hpp"

namespace components {

namespace {

class LogScope final {
 public:
  explicit LogScope(const std::string& init_log_path) {
    if (!init_log_path.empty()) {
      old_default_logger_ = logging::SetDefaultLogger(
          logging::MakeFileLogger("default", init_log_path));
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
  static const std::string kJemallocEnabledPath =
      "/var/run/yandex/userver-jemalloc-profile-enabled-on-start";
  if (!boost::filesystem::exists(kJemallocEnabledPath)) return;

  auto ec = utils::jemalloc::cmd::ProfActivate();
  if (ec) {
    LOG_ERROR() << "Failed to activate jemalloc profiler: " << ec.message();
  }
}

void PreheatStacktraceCollector() {
  // this loads debug info from disk, hopefully preventing this to occur later,
  // e.g. in exception constructor
  to_string(boost::stacktrace::stacktrace{});
}

bool IsDaemon() { return getppid() == 1; }

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

void DoRun(const std::string& config_path, const ComponentList& component_list,
           const std::string& init_log_path, RunMode run_mode) {
  crypto::impl::Openssl::Init();
  HandleJemallocSettings();
  PreheatStacktraceCollector();
  LogScope log_scope{init_log_path};

  LOG_INFO() << "Parsing configs";
  auto config = std::make_unique<ManagerConfig>(
      ManagerConfig::ParseFromFile(config_path));
  LOG_INFO() << "Parsed configs";

  LOG_DEBUG() << "Masking signals";
  utils::SignalCatcher signal_catcher{SIGINT, SIGTERM, SIGQUIT, SIGUSR1,
                                      SIGHUP};
  utils::IgnoreSignalScope ignore_sigpipe_scope(SIGPIPE);
  LOG_DEBUG() << "Masked signals";

  std::unique_ptr<Manager> manager_ptr;
  try {
    manager_ptr = std::make_unique<Manager>(std::move(config), component_list);
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
    } else if (signum == SIGHUP) {
      if (!IsDaemon()) {
        // This is a real HUP
        break;
      }

      LOG_INFO() << "Got reload request";
      std::unique_ptr<ManagerConfig> new_config;
      try {
        new_config = std::make_unique<ManagerConfig>(
            ManagerConfig::ParseFromFile(config_path));
      } catch (const std::exception& ex) {
        LOG_ERROR()
            << "Reload failed, cannot update components manager config: " << ex;
        continue;
      }
      if (new_config->yaml == manager_ptr->GetConfig().yaml &&
          new_config->config_vars_ptr->Yaml() ==
              manager_ptr->GetConfig().config_vars_ptr->Yaml()) {
        LOG_INFO() << "Config unchanged, ignoring request";
        continue;
      }
      std::unique_ptr<Manager> new_manager_ptr;
      try {
        new_manager_ptr =
            std::make_unique<Manager>(std::move(new_config), component_list);
      } catch (const std::exception& ex) {
        LOG_ERROR() << "Reload failed: " << ex;
        continue;
      }
      LOG_INFO() << "New components manager started, shutting down the old one";
      manager_ptr = std::move(new_manager_ptr);
      LOG_INFO() << "Components manager reloaded";
    } else if (signum == SIGUSR1) {
      manager_ptr->OnLogRotate();
      LOG_INFO() << "Log rotated";
    } else {
      LOG_WARNING() << "Got unexpected signal: " << signum << " ("
                    << utils::strsignal(signum) << ')';
      UASSERT(!"unexpected signal");
    }
  }
}

}  // namespace

void Run(const std::string& config_path, const ComponentList& component_list,
         const std::string& init_log_path) {
  DoRun(config_path, component_list, init_log_path, RunMode::kNormal);
}

void RunOnce(const std::string& config_path,
             const ComponentList& component_list,
             const std::string& init_log_path) {
  DoRun(config_path, component_list, init_log_path, RunMode::kOnce);
}

}  // namespace components
