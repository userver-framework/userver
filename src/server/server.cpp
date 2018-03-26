#include "server.hpp"

#include <signal.h>

#include <cstring>
#include <initializer_list>
#include <stdexcept>

#include <logging/config.hpp>
#include <logging/log.hpp>
#include <logging/logger.hpp>
#include <utils/check_syscall.hpp>

#include "server_config.hpp"
#include "server_impl.hpp"

namespace server {

namespace {

class ServerLogScope {
 public:
  explicit ServerLogScope(const std::string& init_log_path) {
    old_default_logger_ = logging::Log();
    if (!init_log_path.empty()) {
      logging::Log() = logging::MakeFileLogger("default", init_log_path);
    }
  }

  ~ServerLogScope() noexcept(false) {
    logging::Log() = std::move(old_default_logger_);
  }

 private:
  logging::LoggerPtr old_default_logger_;
};

class IgnoreSigpipeScope {
 public:
  IgnoreSigpipeScope() {
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = SIG_IGN;
    utils::CheckSyscall(sigaction(SIGPIPE, &action, &old_action_),
                        "setting ignore handler for SIGPIPE");
  }

  ~IgnoreSigpipeScope() noexcept(false) {
    utils::CheckSyscall(sigaction(SIGPIPE, &old_action_, nullptr),
                        "restoring SIGPIPE handler");
  }

 private:
  struct sigaction old_action_;
};

class SignalCatcher {
 public:
  SignalCatcher(std::initializer_list<int> signals) {
    utils::CheckSyscall(sigemptyset(&sigset_), "initializing signal set");
    for (int signum : signals) {
      utils::CheckSyscall(sigaddset(&sigset_, signum), "adding signal to set");
    }
    utils::CheckSyscall(pthread_sigmask(SIG_BLOCK, &sigset_, &old_sigset_),
                        "blocking signals");
  }

  ~SignalCatcher() noexcept(false) {
    utils::CheckSyscall(pthread_sigmask(SIG_SETMASK, &old_sigset_, nullptr),
                        "restoring signal mask");
  }

  int Catch() {
    int signum = -1;
    utils::CheckSyscall(sigwait(&sigset_, &signum), "waiting for signal");
    assert(signum != -1);
    return signum;
  }

 private:
  sigset_t sigset_;
  sigset_t old_sigset_;
};

}  // namespace

void Run(const std::string& config_path, const ComponentList& component_list,
         const std::string& init_log_path) {
  ServerLogScope server_log_scope{init_log_path};

  LOG_INFO() << "Parsing configs";
  auto config = ServerConfig::ParseFromFile(config_path);
  LOG_INFO() << "Parsed configs";

  LOG_DEBUG() << "Masking signals";
  SignalCatcher signal_catcher{SIGINT, SIGTERM, SIGQUIT, SIGUSR1, SIGHUP};
  IgnoreSigpipeScope ignore_sigpipe_scope;
  LOG_DEBUG() << "Masked signals";

  auto server_ptr =
      std::make_unique<ServerImpl>(std::move(config), component_list);

  for (;;) {
    auto signum = signal_catcher.Catch();
    if (signum == SIGINT || signum == SIGTERM || signum == SIGQUIT) {
      break;
    } else if (signum == SIGHUP) {
      LOG_INFO() << "Got reload request";
      ServerConfig new_config;
      try {
        new_config = ServerConfig::ParseFromFile(config_path);
      } catch (const std::exception& ex) {
        LOG_ERROR() << "Reload failed, cannot update server config: "
                    << ex.what();
        continue;
      }
      if (new_config.json == server_ptr->GetConfig().json) {
        LOG_INFO() << "Config unchanged, ignoring request";
        continue;
      }
      std::unique_ptr<ServerImpl> new_server_ptr;
      try {
        new_server_ptr =
            std::make_unique<ServerImpl>(std::move(new_config), component_list);
      } catch (const std::exception& ex) {
        LOG_ERROR() << "Reload failed: " << ex.what();
        continue;
      }
      LOG_INFO() << "New server started, shutting down the old one";
      server_ptr = std::move(new_server_ptr);
      LOG_INFO() << "Server reloaded";
    } else if (signum == SIGUSR1) {
      server_ptr->OnLogRotate();
    } else {
      LOG_WARNING() << "Got unexpected signal: " << signum << " ("
                    << strsignal(signum) << ')';
      assert(!"unexpected signal");
    }
  }
}

}  // namespace server
