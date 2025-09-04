#include <ugrpc/impl/grpc_native_logging.hpp>

#include <stdexcept>

#include <fmt/format.h>

#if defined(USERVER_IMPL_FEATURE_OLD_GRPC_NATIVE_LOGGING) || defined(ARCADIA_ROOT)
#include <grpc/support/log.h>
#else
#include <absl/log/log_sink.h>
#include <absl/log/log_sink_registry.h>
#endif

#include <userver/engine/mutex.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/underlying_value.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

namespace {

#if defined(USERVER_IMPL_FEATURE_OLD_GRPC_NATIVE_LOGGING) || defined(ARCADIA_ROOT)
logging::Level ToLogLevel(::gpr_log_severity severity) noexcept {
    switch (severity) {
        case ::GPR_LOG_SEVERITY_DEBUG:
            return logging::Level::kDebug;
        case ::GPR_LOG_SEVERITY_INFO:
            return logging::Level::kInfo;
        case ::GPR_LOG_SEVERITY_ERROR:
            [[fallthrough]];
        default:
            return logging::Level::kError;
    }
}

::gpr_log_severity ToGprLogSeverity(logging::Level level) {
    switch (level) {
        case logging::Level::kDebug:
            return ::GPR_LOG_SEVERITY_DEBUG;
        case logging::Level::kInfo:
            return ::GPR_LOG_SEVERITY_INFO;
        case logging::Level::kError:
            return ::GPR_LOG_SEVERITY_ERROR;
        default:
            throw std::logic_error(fmt::format(
                "grpcpp log level {} is not allowed. Allowed options: "
                "debug, info, error.",
                logging::ToString(level)
            ));
    }
}

void LogFunction(::gpr_log_func_args* args) noexcept {
    UASSERT(args);
    const auto lvl = ToLogLevel(args->severity);
    if (!logging::ShouldLog(lvl)) return;

    auto& logger = logging::GetDefaultLogger();
    const auto location = utils::impl::SourceLocation::Custom(args->line, args->file, "");
    logging::LogHelper(logger, lvl, logging::LogClass::kLog, location) << args->message;

    // We used to call LogFlush for kError logging level here,
    // but that might lead to a thread switch (there is a coroutine-aware
    // .Wait somewhere down the call chain), which breaks the grpc-core badly:
    // its ExecCtx/ApplicationCallbackExecCtx are attached to a current thread
    // (thread_local that is), and switching threads violates that, obviously.
}

engine::Mutex native_log_level_mutex;
auto native_log_level = logging::Level::kNone;

#else

logging::Level ToLogLevel(absl::LogSeverity severity) noexcept {
    switch (severity) {
        case absl::LogSeverity::kInfo:
            return logging::Level::kInfo;
        case absl::LogSeverity::kWarning:
            return logging::Level::kWarning;
        case absl::LogSeverity::kError:
            [[fallthrough]];
        default:
            return logging::Level::kError;
    }
}

#endif

}  // namespace

void SetupNativeLogging() {
#if defined(USERVER_IMPL_FEATURE_OLD_GRPC_NATIVE_LOGGING) || defined(ARCADIA_ROOT)
    ::gpr_set_log_function(&LogFunction);
#else
    class AbslLogSink final : public absl::LogSink {
        void Send(const absl::LogEntry& entry) override {
            const auto lvl = ToLogLevel(entry.log_severity());
            if (!logging::ShouldLog(lvl)) return;

            auto& logger = logging::GetDefaultLogger();
            const auto location = utils::impl::SourceLocation::Custom(entry.source_line(), entry.source_filename(), "");
            logging::LogHelper(logger, lvl, logging::LogClass::kLog, location) << entry.text_message();
        }
    };

    static AbslLogSink grpc_sink{};
    [[maybe_unused]] static auto init_once = []() {
        absl::AddLogSink(&grpc_sink);
        return 0;
    }();
#endif
}

void UpdateNativeLogLevel(logging::Level min_log_level_override) {
#if defined(USERVER_IMPL_FEATURE_OLD_GRPC_NATIVE_LOGGING) || defined(ARCADIA_ROOT)
    const std::lock_guard lock(native_log_level_mutex);

    if (utils::UnderlyingValue(min_log_level_override) < utils::UnderlyingValue(native_log_level)) {
        ::gpr_set_log_verbosity(ToGprLogSeverity(min_log_level_override));
        native_log_level = min_log_level_override;
    }
#endif
}

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
