#include <userver/engine/subprocess/process_starter.hpp>

#include <fcntl.h>
#include <spawn.h>
#include <unistd.h>

#include <csignal>
#include <cstring>
#include <iterator>
#include <ranges>
#include <system_error>

#include <fmt/format.h>
#include <fmt/ranges.h>

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
#include <userver/utils/fmt_compat.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::subprocess {
namespace {

// posix_spawn* APIs return 0 on success and an errno value on failure (not -1).
template <typename Format, typename... Args>
void CheckPosixSpawn(int rc, const Format& format, const Args&... args) {
    if (rc == 0) {
        return;
    }
    fmt::memory_buffer msg_buf;
    fmt::format_to(std::back_inserter(msg_buf), "Error while ");
    fmt::format_to(std::back_inserter(msg_buf), fmt::runtime(format), args...);
    msg_buf.push_back('\0');
    throw std::system_error(std::error_code(rc, std::system_category()), msg_buf.data());
}

class PosixSpawnFileActions final {
public:
    PosixSpawnFileActions() {
        CheckPosixSpawn(posix_spawn_file_actions_init(&actions_), "posix_spawn_file_actions_init");
    }

    ~PosixSpawnFileActions() { posix_spawn_file_actions_destroy(&actions_); }

    PosixSpawnFileActions(const PosixSpawnFileActions&) = delete;
    PosixSpawnFileActions& operator=(const PosixSpawnFileActions&) = delete;

    posix_spawn_file_actions_t* Get() noexcept { return &actions_; }

    void AddOpen(int fd, const std::string& path, int oflag, mode_t mode) {
        CheckPosixSpawn(
            posix_spawn_file_actions_addopen(&actions_, fd, path.c_str(), oflag, mode),
            "posix_spawn_file_actions_addopen fd={} path={}",
            fd,
            path
        );
    }

private:
    posix_spawn_file_actions_t actions_{};
};

class PosixSpawnAttr final {
public:
    PosixSpawnAttr() { CheckPosixSpawn(posix_spawnattr_init(&attr_), "posix_spawnattr_init"); }

    ~PosixSpawnAttr() { posix_spawnattr_destroy(&attr_); }

    PosixSpawnAttr(const PosixSpawnAttr&) = delete;
    PosixSpawnAttr& operator=(const PosixSpawnAttr&) = delete;

    posix_spawnattr_t* Get() noexcept { return &attr_; }

    void SetFlags(short flags) { CheckPosixSpawn(posix_spawnattr_setflags(&attr_, flags), "posix_spawnattr_setflags"); }

private:
    posix_spawnattr_t attr_{};
};

pid_t DoPosixSpawn(
    const std::string& executable_path,
    const std::vector<std::string>& args,
    const EnvironmentVariables& env,
    const std::optional<std::string>& stdout_file,
    const std::optional<std::string>& stderr_file,
    bool use_path
) {
    std::vector<char*> argv_ptrs;
    std::vector<std::string> envp_buf;
    std::vector<char*> envp_ptrs;
    argv_ptrs.reserve(args.size() + 2);
    envp_buf.reserve(env.size());
    envp_ptrs.reserve(env.size() + 1);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    argv_ptrs.push_back(const_cast<char*>(executable_path.c_str()));
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

    PosixSpawnFileActions file_actions;
    constexpr int kRedirectFlags = O_WRONLY | O_CREAT | O_APPEND;
    constexpr mode_t kRedirectMode = 0644;
    if (stdout_file) {
        file_actions.AddOpen(STDOUT_FILENO, *stdout_file, kRedirectFlags, kRedirectMode);
    }
    if (stderr_file) {
        file_actions.AddOpen(STDERR_FILENO, *stderr_file, kRedirectFlags, kRedirectMode);
    }

    PosixSpawnAttr attr;
#ifdef POSIX_SPAWN_CLOEXEC_DEFAULT
    // macOS: do not inherit non-stdio fds (e.g. logger files) into the child.
    attr.SetFlags(POSIX_SPAWN_CLOEXEC_DEFAULT);
#endif

    pid_t pid = -1;
    if (!use_path) {
        CheckPosixSpawn(
            posix_spawn(
                &pid,
                executable_path.c_str(),
                file_actions.Get(),
                attr.Get(),
                argv_ptrs.data(),
                envp_ptrs.data()
            ),
            "posix_spawn executable_path={}",
            executable_path
        );
    } else {
        CheckPosixSpawn(
            posix_spawnp(
                &pid,
                executable_path.c_str(),
                file_actions.Get(),
                attr.Get(),
                argv_ptrs.data(),
                envp_ptrs.data()
            ),
            "posix_spawnp executable_path={}",
            executable_path
        );
    }
    return pid;
}

EnvironmentVariables ApplyEnvironmentUpdate(
    std::optional<EnvironmentVariables>&& env,
    std::optional<EnvironmentVariablesUpdate>&& env_update
) {
    if (env) {
        if (env_update) {
            return env->UpdateWith(std::move(env_update.value()));
        } else {
            return std::move(env.value());
        }
    } else {
        if (env_update) {
            return GetCurrentEnvironmentVariables().UpdateWith(std::move(env_update.value()));
        } else {
            return GetCurrentEnvironmentVariables();
        }
    }
}

}  // namespace

ProcessStarter::ProcessStarter(TaskProcessor& task_processor)
    : thread_control_(task_processor.EventThreadPool().GetEvDefaultLoopThread())
{}

ChildProcess ProcessStarter::Exec(
    const std::string& executable_path,
    const std::vector<std::string>& args,
    ExecOptions&& options
) {
    EnvironmentVariables env = ApplyEnvironmentUpdate(std::move(options.env), std::move(options.env_update));

    if (options.use_path && executable_path.find('/') != std::string::npos && !env.GetValueOptional("PATH")) {
        throw std::runtime_error(
            "execvp potential vulnerability. more details "
            "https://github.com/userver-framework/userver/issues/588"
        );
    }

    tracing::Span span("ProcessStarter::Exec");
    span.AddTag("executable_path", executable_path);
    Promise<ChildProcess> promise;
    auto future = promise.get_future();

    LOG_DEBUG() << fmt::format(
        "do posix_spawn{}(), executable_path={}, args=['{}'], env=[{}]",
        options.use_path ? "p" : "",
        executable_path,
        fmt::join(args, "' '"),
        fmt::join(
            env | std::views::transform([](const auto& key_value) { return key_value.first + '=' + key_value.second; }),
            ", "
        )
    );
    thread_control_.RunInEvLoopAsync([&, promise = std::move(promise)]() mutable {
        try {
            // posix_spawn is safe in multithreaded processes, unlike fork()+exec().
            const auto pid =
                DoPosixSpawn(executable_path, args, env, options.stdout_file, options.stderr_file, options.use_path);
            span.AddTag("child-process-pid", pid);
            LOG_DEBUG() << "Started child process with pid=" << pid;
            Promise<ChildProcessStatus> exec_result_promise;
            auto res = ChildProcessMapSet(pid, ev::ChildProcessMapValue(std::move(exec_result_promise)));
            if (res.second) {
                promise.set_value(ChildProcess{ChildProcessImpl{pid, res.first->status_promise.get_future()}});
            } else {
                const auto msg = fmt::format("process with pid={} already exists in child_process_map", pid);
                LOG_ERROR() << msg << ", send SIGKILL";
                ChildProcessImpl(pid, Future<ChildProcessStatus>{}).SendSignal(SIGKILL);
                promise.set_exception(std::make_exception_ptr(std::runtime_error(msg)));
            }
        } catch (const std::exception& /*e*/) {
            // CheckPosixSpawn may throw and without the following line a useless "Broken promise" is reported
            promise.set_exception(std::current_exception());
        }
    });

    const TaskCancellationBlocker cancel_blocker;
    return future.get();
}

}  // namespace engine::subprocess

USERVER_NAMESPACE_END
