#include <userver/utils/debugging.hpp>

#ifdef __cpp_lib_debugging
#define USERVER_DEBUGGING_USE_STD
#elif defined(__linux__)
#define USERVER_DEBUGGING_USE_LINUX
#elif defined(__APPLE__)
#define USERVER_DEBUGGING_USE_APPLE
#endif

#ifdef USERVER_DEBUGGING_USE_STD
#include <debugging>  // Y_IGNORE
#endif

#ifdef USERVER_DEBUGGING_USE_LINUX
#include <algorithm>
#include <array>
#include <fstream>
#include <string>

#include <fmt/core.h>
#endif

#ifdef USERVER_DEBUGGING_USE_APPLE
#include <array>
#include <system_error>

#include <sys/sysctl.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace {

#ifdef USERVER_DEBUGGING_USE_LINUX
constexpr std::string_view kSelfStatusPath = "/proc/self/status";
constexpr std::string_view kProcCommPattern = "/proc/{}/comm";  // format with pid
constexpr std::string_view kTracerPrefix = "TracerPid:\t";
constexpr std::array<std::string_view, 3> kDebuggers = {"gdb", "gdbserver", "lldb-server"};

/// @see man proc_pid_status(5)
/// @see man proc_pid_comm(5)
bool IsDebuggerPresentLinux() {
    // Read process status from kernel
    std::ifstream status(std::string{kSelfStatusPath});
    std::string line;

    // Find TracerPid line (kernel sets this when process is traced via ptrace)
    while (std::getline(status, line)) {
        if (line.compare(0, kTracerPrefix.size(), kTracerPrefix) != 0) {
            continue;
        }

        const auto pid = line.substr(kTracerPrefix.size());

        // TracerPid == 0 means no tracing
        if (pid == "0") {
            return false;
        }

        // Read tracer process name from /proc/[pid]/comm
        std::ifstream comm(fmt::format(kProcCommPattern, pid));
        if (!std::getline(comm, line)) {
            return false;
        }

        // Filter known debuggers to avoid false positives from other tracers
        // (for example `strace` could be there)
        return std::any_of(kDebuggers.begin(), kDebuggers.end(), [&](const auto& dbg) {
            return line.find(dbg) != std::string::npos;
        });
    }

    return false;
}
#endif

#ifdef USERVER_DEBUGGING_USE_APPLE
/// @see https://developer.apple.com/library/archive/qa/qa1361/_index.html
bool IsDebuggerPresentApple() noexcept {
    // Initialize process info structure
    kinfo_proc info{};

    // Initialize mib, which tells sysctl the info we want, in this case
    // we're looking for information about a specific process ID.
    std::array<int, 4> mib = {CTL_KERN, KERN_PROC, KERN_PROC_PID, getpid()};

    std::size_t size = sizeof(info);

    // Query process information via sysctl
    if (sysctl(mib.data(), mib.size(), &info, &size, nullptr, 0) != 0) {
        return false;
    }

    // We're being debugged if the P_TRACED flag is set.
    return (info.kp_proc.p_flag & P_TRACED) != 0;
}
#endif

}  // namespace

bool IsDebuggerPresent() noexcept {
    try {
#ifdef USERVER_DEBUGGING_USE_STD
        return std::is_debugger_present();
#elif defined(USERVER_DEBUGGING_USE_LINUX)
        return IsDebuggerPresentLinux();
#elif defined(USERVER_DEBUGGING_USE_APPLE)
        return IsDebuggerPresentApple();
#endif
    } catch (const std::exception& e) {
        LOG_ERROR() << "Exception occurred during the debugger detection: " << e;
    }
    return false;
}

}  // namespace utils

USERVER_NAMESPACE_END
