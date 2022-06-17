#include <userver/engine/subprocess/process_starter.hpp>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <csignal>
#include <cstdio>
#include <cstring>
#include <iostream>

#include <fmt/format.h>
#include <boost/algorithm/string.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include <userver/logging/log.hpp>
#include <userver/tracing/span.hpp>
#include <utils/check_syscall.hpp>

#include <engine/ev/child_process_map.hpp>
#include <engine/ev/thread_control.hpp>
#include <engine/ev/thread_pool.hpp>
#include <engine/task/task_processor.hpp>
#include <userver/engine/future.hpp>
#include <userver/engine/task/cancel.hpp>

#include <engine/subprocess/child_process_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::subprocess {
namespace {

void DoExecve(const std::string& command, const std::vector<std::string>& args,
              const EnvironmentVariables& env,
              const std::optional<std::string>& stdout_file,
              const std::optional<std::string>& stderr_file) {
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

  for (const auto& elem : env) {
    envp_buf.emplace_back(elem.first + '=' + elem.second);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    envp_ptrs.push_back(const_cast<char*>(envp_buf.back().c_str()));
  }
  envp_ptrs.push_back(nullptr);

  utils::CheckSyscall(
      execve(command.c_str(), argv_ptrs.data(), envp_ptrs.data()), "execve");
}

}  // namespace

ProcessStarter::ProcessStarter(TaskProcessor& task_processor)
    : thread_control_(
          task_processor.EventThreadPool().GetEvDefaultLoopThread()) {}

ChildProcess ProcessStarter::Exec(
    const std::string& command, const std::vector<std::string>& args,
    const EnvironmentVariables& env,
    const std::optional<std::string>& stdout_file,
    const std::optional<std::string>& stderr_file) {
  tracing::Span span("ProcessStarter::Exec");
  span.AddTag("command", command);
  Promise<ChildProcess> promise;
  auto future = promise.get_future();
  thread_control_.RunInEvLoopAsync([&, promise = std::move(promise)]() mutable {
    LOG_DEBUG() << "do fork() + execve(), command=" << command << ", args=["
                << (args.empty() ? "" : '\'' + boost::join(args, "' '") + '\'')
                << "], env=["
                << (env.empty()
                        ? ""
                        : boost::join(env | boost::adaptors::transformed(
                                                [](const auto& key_value) {
                                                  return key_value.first + '=' +
                                                         key_value.second;
                                                }),
                                      ", "))
                << ']';
    auto pid = utils::CheckSyscall(fork(), "fork");
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
        std::string msg = "process with pid=" + std::to_string(pid) +
                          " already exists in child_process_map";
        LOG_ERROR() << msg << ", send SIGKILL";
        ChildProcessImpl(pid, Future<ChildProcessStatus>{}).SendSignal(SIGKILL);
        promise.set_exception(std::make_exception_ptr(std::runtime_error(msg)));
      }
    } else {
      // in child thread
      try {
        try {
          DoExecve(command, args, env, stdout_file, stderr_file);
        } catch (const std::exception& ex) {
          std::cerr << "Cannot execute child: " << ex.what();
        }
      } catch (...) {
        // must not do anything in a child
      }
      // on success execve does not return
      std::abort();
    }
  });

  engine::TaskCancellationBlocker cancel_blocker;
  return future.get();
}

ChildProcess ProcessStarter::Exec(
    const std::string& command, const std::vector<std::string>& args,
    EnvironmentVariablesUpdate env_update,
    const std::optional<std::string>& stdout_file,
    const std::optional<std::string>& stderr_file) {
  return Exec(command, args,
              EnvironmentVariables{GetCurrentEnvironmentVariables()}.UpdateWith(
                  std::move(env_update)),
              stdout_file, stderr_file);
}

ChildProcess ProcessStarter::Exec(
    const std::string& command, const std::vector<std::string>& args,
    const std::optional<std::string>& stdout_file,
    const std::optional<std::string>& stderr_file) {
  return Exec(command, args, EnvironmentVariablesUpdate{{}}, stdout_file,
              stderr_file);
}

}  // namespace engine::subprocess

USERVER_NAMESPACE_END
