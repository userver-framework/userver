#include <userver/engine/subprocess/process_starter.hpp>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <csignal>
#include <cstdio>
#include <cstring>
#include <iostream>

#include <fmt/format.h>
#include <boost/range/adaptor/transformed.hpp>

#include <engine/ev/child_process_map.hpp>
#include <engine/ev/thread_control.hpp>
#include <engine/ev/thread_pool.hpp>
#include <engine/subprocess/child_process_impl.hpp>
#include <engine/task/task_processor.hpp>
#include <userver/engine/future.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/logging/log.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/algo.hpp>
#include <utils/check_syscall.hpp>

extern char** environ;

USERVER_NAMESPACE_BEGIN

namespace engine::subprocess {
namespace {

void DoExec(const std::string& command, const std::vector<std::string>& args,
            const EnvironmentVariables& env,
            const std::optional<std::string>& stdout_file,
            const std::optional<std::string>& stderr_file, bool use_path) {
  if (stdout_file) {
    if (!std::freopen(stdout_file->c_str(), "a", stdout)) {
      utils::CheckSyscall(-1, "freopen stdout to {}", *stdout_file);
    }
  }
  if (stderr_file) {
    if (!std::freopen(stderr_file->c_str(), "a", stderr)) {
      utils::CheckSyscall(-1, "freopen stderr to {}", *stdout_file);
    }
  }
  std::vector<char*> argv_ptrs;
  std::vector<std::string> envp_buf;
  std::vector<char*> envp_ptrs;
  argv_ptrs.reserve(args.size() + 2);
  envp_buf.reserve(env.size());
  envp_ptrs.reserve(env.size() + 1);

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  argv_ptrs.push_back(const_cast<char*>(command.c_str()));
  for (const auto& arg : args) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    argv_ptrs.push_back(const_cast<char*>(arg.c_str()));
  }
  argv_ptrs.push_back(nullptr);

  for (const auto& [key, value] : env) {
    envp_buf.emplace_back(utils::StrCat(key, "=", value));
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    envp_ptrs.push_back(const_cast<char*>(envp_buf.back().c_str()));
  }
  envp_ptrs.push_back(nullptr);

  environ = envp_ptrs.data();  // The variable is assigned to the environment to
                               // use the execv, execvp functions

  if (!use_path) {
    utils::CheckSyscall(execv(command.c_str(), argv_ptrs.data()), "execv");
  } else {
    utils::CheckSyscall(execvp(command.c_str(), argv_ptrs.data()), "execvp");
  }
}

EnvironmentVariables ApplyEnvironmentUpdate(
    std::optional<EnvironmentVariables>&& env,
    std::optional<EnvironmentVariablesUpdate>&& env_update) {
  if (env) {
    if (env_update) {
      return env->UpdateWith(std::move(env_update.value()));
    } else {
      return std::move(env.value());
    }
  } else {
    if (env_update) {
      return GetCurrentEnvironmentVariables().UpdateWith(
          std::move(env_update.value()));
    } else {
      return GetCurrentEnvironmentVariables();
    }
  }
}

}  // namespace

ProcessStarter::ProcessStarter(TaskProcessor& task_processor)
    : thread_control_(
          task_processor.EventThreadPool().GetEvDefaultLoopThread()) {}

ChildProcess ProcessStarter::Exec(const std::string& command,
                                  const std::vector<std::string>& args,
                                  ExecOptions&& options) {
  EnvironmentVariables env = ApplyEnvironmentUpdate(
      std::move(options.env), std::move(options.env_update));

  if (options.use_path && command.find('/') != std::string::npos &&
      !env.GetValueOptional("PATH")) {
    throw std::runtime_error(
        "execvp potential vulnerability. more details "
        "https://github.com/userver-framework/userver/issues/588");
  }

  tracing::Span span("ProcessStarter::Exec");
  span.AddTag("command", command);
  Promise<ChildProcess> promise;
  auto future = promise.get_future();

  thread_control_.RunInEvLoopAsync([&, promise = std::move(promise)]() mutable {
    const auto keys =
        env | boost::adaptors::transformed([](const auto& key_value) {
          return key_value.first + '=' + key_value.second;
        });
    LOG_DEBUG() << fmt::format(
        "do fork() + {}(), command={}, args=[\'{}\'], env=[]",
        options.use_path ? "execv" : "execvp", fmt::join(args, "' '"),
        fmt::join(keys, ", "));

    const auto pid = utils::CheckSyscall(fork(), "fork");
    if (pid) {
      // in parent thread
      span.AddTag("child-process-pid", pid);
      LOG_DEBUG() << "Started child process with pid=" << pid;
      Promise<ChildProcessStatus> exec_result_promise;
      auto res = ChildProcessMapSet(
          pid, ev::ChildProcessMapValue(std::move(exec_result_promise)));
      if (res.second) {
        promise.set_value(ChildProcess{
            ChildProcessImpl{pid, res.first->status_promise.get_future()}});
      } else {
        const auto msg = fmt::format(
            "process with pid={} already exists in child_process_map", pid);
        LOG_ERROR() << msg << ", send SIGKILL";
        ChildProcessImpl(pid, Future<ChildProcessStatus>{}).SendSignal(SIGKILL);
        promise.set_exception(std::make_exception_ptr(std::runtime_error(msg)));
      }
    } else {
      // in child thread
      try {
        try {
          DoExec(command, args, env, options.stdout_file, options.stderr_file,
                 options.use_path);
        } catch (const std::exception& ex) {
          std::cerr << "Cannot execute child: " << ex.what();
        }
      } catch (...) {
        // must not do anything in a child
        std::abort();
      }
      // on success execve or execvp does not return
      std::abort();
    }
  });

  const TaskCancellationBlocker cancel_blocker;
  return future.get();
}

ChildProcess ProcessStarter::Exec(
    const std::string& command, const std::vector<std::string>& args,
    const EnvironmentVariables& env,
    const std::optional<std::string>& stdout_file,
    const std::optional<std::string>& stderr_file) {
  ExecOptions options{std::move(env), std::nullopt, std::move(stdout_file),
                      std::move(stderr_file), false};
  return Exec(command, args, std::move(options));
}

ChildProcess ProcessStarter::Exec(
    const std::string& command, const std::vector<std::string>& args,
    EnvironmentVariablesUpdate env_update,
    const std::optional<std::string>& stdout_file,
    const std::optional<std::string>& stderr_file) {
  ExecOptions options{std::nullopt, std::move(env_update),
                      std::move(stdout_file), std::move(stderr_file), false};
  return Exec(command, args, std::move(options));
}

}  // namespace engine::subprocess

USERVER_NAMESPACE_END
