#include <userver/logging/component.hpp>

#include <chrono>
#include <cstdio>
#include <stdexcept>

#include <fmt/chrono.h>
#include <fmt/ranges.h>

#include <userver/alerts/source.hpp>
#include <userver/components/component.hpp>
#include <userver/components/scope.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/logging/format.hpp>
#include <userver/logging/log.hpp>
#include <userver/logging/logger.hpp>
#include <userver/os_signals/component.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/statistics/writer.hpp>
#include <userver/utils/thread_name.hpp>
#include <userver/yaml_config/map_to_array.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <logging/config.hpp>
#include <logging/impl/tcp_socket_sink.hpp>
#include <logging/tp_logger.hpp>
#include <logging/tp_logger_utils.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/logging/component.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace components {

namespace {

constexpr std::chrono::seconds kDefaultFlushInterval{2};

void ReopenLoggerFile(const std::shared_ptr<logging::impl::TpLogger>& logger) {
    logger->Reopen(logging::impl::ReopenMode::kAppend);
}

/// [alert_declaration]
const alerts::Source kLogReopeningAlert{"log_reopening_error"};
/// [alert_declaration]

void ReportReopeningErrorAndThrow(
    const std::vector<std::string_view>& failed_loggers,
    const std::string& result_messages
) {
    std::fputs(
        fmt::format(
            "[{:%Y-%m-%d %H:%M:%S %Z}] loggers [{}] failed to reopen the log file: logs are getting lost now",
            std::chrono::system_clock::now(),
            fmt::join(failed_loggers, ", ")
        )
            .c_str(),
        stderr
    );

    throw std::runtime_error("ReopenAll errors: " + result_messages);
}

}  // namespace

Logging::Logging(const ComponentConfig& config, const ComponentContext& context)
    : fs_task_processor_{GetFsTaskProcessor(config, context)},
      metrics_storage_(context.FindComponent<components::StatisticsStorage>().GetMetricsStorage())
{
    try {
        Init(config, context);

        /// [Signals sample - init]
        auto& signals_processor = context.FindComponent<os_signals::ProcessorComponent>().Get();
        context.RegisterScope(MakeScope([this, &signals_processor] {
            auto holder = signals_processor.AddListener(this, kName, os_signals::kSigUsr1, &Logging::OnLogRotate);

            // Force logrotate just after signal subscription to be sure we haven't lost signals during the loading.
            // If there were no signals, it's OK to re-open log files one more time.
            OnLogRotate();

            return holder;
        }));
        /// [Signals sample - init]
    } catch (const std::exception&) {
        Stop();
        throw;
    }
    // Please put all Logging component initialization code inside Init for
    // correct exception handling.
}

void Logging::Init(const ComponentConfig& config, const ComponentContext& context) {
    const auto logger_configs = yaml_config::ParseMapToArray<logging::LoggerConfig>(config["loggers"]);

    if (logger_configs.empty()) {
        // We don't own default logger, we have no stats to report
        return;
    }

    for (const auto& logger_config : logger_configs) {
        const bool is_default_logger = (logger_config.logger_name == "default");

        if (logger_config.testsuite_capture && !is_default_logger) {
            throw std::runtime_error(
                "Testsuite capture can only currently be enabled "
                "for the default logger"
            );
        }

        auto logger = logging::impl::GetDefaultLoggerOrMakeTpLogger(logger_config);

        if (is_default_logger) {
            if (logger_config.queue_overflow_behavior == logging::QueueOverflowBehavior::kBlock) {
                throw std::runtime_error(
                    "'default' logger should not be set to 'overflow_behavior: "
                    "block'! "
                    "Default logger is used by the userver internals, including the "
                    "logging internals. Blocking inside the engine internals could "
                    "lead to hardly reproducible hangups in some border cases of "
                    "error "
                    "reporting."
                );
            }

            socket_sink_ = logging::impl::GetTcpSocketSink(*logger);
        }

        logger->StartConsumerTask(
            logger_config.fs_task_processor
                ? context.GetTaskProcessor(*logger_config.fs_task_processor)
                : fs_task_processor_,
            logger_config.message_queue_size,
            logger_config.queue_overflow_behavior
        );

        auto insertion_result = loggers_.emplace(logger_config.logger_name, std::move(logger));
        if (!insertion_result.second) {
            throw std::runtime_error("duplicate logger '" + insertion_result.first->first + '\'');
        }
    }

    flush_task_.Start(
        "log_flusher",
        utils::PeriodicTask::Settings(
            std::chrono::duration_cast<std::chrono::milliseconds>(kDefaultFlushInterval),
            {},
            logging::Level::kTrace
        ),
        [this] { FlushLogs(); }
    );

    auto* const statistics_storage = context.FindComponentOptional<components::StatisticsStorage>();
    if (statistics_storage) {
        utils::statistics::RegisterWriterScope(context, "logger", [this](utils::statistics::Writer& writer) {
            return WriteStatistics(writer);
        });
    }
}

Logging::~Logging() { Stop(); }

void Logging::Stop() noexcept {
    flush_task_.Stop();

    // Loggers could be used from non coroutine environments and should be
    // available even after task processors are down.
    for (const auto& [logger_name, logger] : loggers_) {
        logger->StopConsumerTask();
    }
}

void Logging::SetLogger(const std::string& name, logging::LoggerPtr logger) {
    auto [_, inserted] = extra_loggers_.Emplace(name, std::move(logger));
    if (!inserted) {
        throw std::runtime_error(fmt::format("Duplicate logger name: {}", name));
    }
}

logging::LoggerPtr Logging::GetLogger(const std::string& name) {
    auto it = loggers_.find(name);
    if (it == loggers_.end()) {
        auto logger = extra_loggers_.Get(name);
        if (logger) {
            return *logger;
        }

        throw std::runtime_error("logger '" + name + "' not found");
    }
    return it->second;
}

logging::TextLoggerPtr Logging::GetTextLogger(const std::string& name) {
    auto logger = GetLogger(name);
    auto text_logger = std::dynamic_pointer_cast<logging::impl::TextLogger>(logger);
    if (!text_logger) {
        throw std::runtime_error(fmt::format("Invalid logger '{}' type, not a text logger", name));
    }
    return text_logger;
}

logging::LoggerPtr Logging::GetLoggerOptional(const std::string& name) {
    return utils::FindOrDefault(loggers_, name, nullptr);
}

void Logging::StartSocketLoggingDebug(const std::optional<logging::Level>& log_level) {
    UASSERT(socket_sink_);
    logging::LogFlush();
    if (log_level.has_value()) {
        logging::SetDefaultLoggerLevel(log_level.value());
        socket_sink_->SetLevel(log_level.value());
    } else {
        socket_sink_->SetLevel(logging::Level::kTrace);
    }
}

void Logging::StopSocketLoggingDebug(const std::optional<logging::Level>& log_level) {
    UASSERT(socket_sink_);
    logging::LogFlush();
    socket_sink_->SetLevel(logging::Level::kNone);
    socket_sink_->Close();
    if (log_level.has_value()) {
        logging::SetDefaultLoggerLevel(log_level.value());
    }
}

void Logging::OnLogRotate() {
    try {
        TESTPOINT("on-logrotate-called", formats::json::Value{});
        TryReopenFiles();
    } catch (const std::exception& e) {
        LOG_ERROR() << "An error occurred while ReopenAll: " << e;
    }
}

void Logging::TryReopenFiles() {
    std::unordered_map<std::string_view, engine::TaskWithResult<void>> tasks;
    tasks.reserve(loggers_.size() + 1);
    for (const auto& [name, logger] : loggers_) {
        tasks.emplace(name, engine::CriticalAsyncNoSpan(fs_task_processor_, ReopenLoggerFile, logger));
    }

    std::string result_messages;
    std::vector<std::string_view> failed_loggers;

    for (auto& [name, task] : tasks) {
        try {
            task.Get();
        } catch (const std::exception& e) {
            result_messages += e.what();
            result_messages += ";";
            failed_loggers.push_back(name);
        }
    }
    LOG_INFO() << "Log rotated";

    const bool error_happened = !result_messages.empty();
    /// [alert_usage]
    if (error_happened) {
        kLogReopeningAlert.FireAlert(*metrics_storage_);
        ReportReopeningErrorAndThrow(failed_loggers, result_messages);
    } else {
        kLogReopeningAlert.StopAlertNow(*metrics_storage_);
    }
    /// [alert_usage]
}

void Logging::WriteStatistics(utils::statistics::Writer& writer) const {
    for (const auto& [name, logger] : loggers_) {
        writer.ValueWithLabels(logger->GetStatistics(), {"logger", logger->GetLoggerName()});
    }
}

void Logging::FlushLogs() {
    logging::LogFlush();
    for (auto& item : loggers_) {
        item.second->Flush();
    }
}

yaml_config::Schema Logging::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<RawComponentBase>("src/logging/component.yaml");
}

}  // namespace components

USERVER_NAMESPACE_END
