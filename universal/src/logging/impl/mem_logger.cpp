#include <userver/logging/impl/mem_logger.hpp>

#include <mutex>
#include <vector>

#include <logging/impl/formatters/struct.hpp>

#include <cstdio>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

namespace {
// kMaxLogItems is a hardcoded const and not config value as it is used before
// the main logger initialization and before yaml config is parsed.
constexpr auto kMaxLogItems = 10000;
}  // namespace

struct MemLogger::Impl {
    std::mutex mutex;
    std::vector<formatters::LogItem> data;
    LoggerBase* forward_logger{nullptr};
};

MemLogger::MemLogger() noexcept { SetLevel(Level::kDebug); }

MemLogger::~MemLogger() {
    for (const auto& data : pimpl_->data) {
        std::fputs(data.text.c_str(), stderr);
    }
}

MemLogger& MemLogger::GetMemLogger() noexcept {
    static MemLogger logger;
    return logger;
}

void MemLogger::DropLogs() {
    const std::lock_guard lock(pimpl_->mutex);
    pimpl_->data.clear();
}

size_t MemLogger::GetPendingLogsCount() {
    const std::lock_guard lock(pimpl_->mutex);
    return pimpl_->data.size();
}

void MemLogger::Log(Level level, formatters::LoggerItemRef msg) {
    UASSERT(dynamic_cast<formatters::LogItem*>(&msg));
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    auto& item = static_cast<formatters::LogItem&>(msg);

    const std::lock_guard lock(pimpl_->mutex);
    if (pimpl_->forward_logger) {
        auto formatter = pimpl_->forward_logger->MakeFormatter(level, item.log_class, item.location);
        DispatchItem(item, *formatter);

        auto& li = formatter->ExtractLoggerItem();
        pimpl_->forward_logger->Log(level, li);
        return;
    }

    if (pimpl_->data.size() > kMaxLogItems) return;

    pimpl_->data.push_back(std::move(item));
}

formatters::BasePtr
MemLogger::MakeFormatter(Level level, LogClass log_class, const utils::impl::SourceLocation& location) {
    return std::make_unique<formatters::Struct>(level, log_class, location);
}

void MemLogger::ForwardTo(LoggerBase* logger_to) {
    const std::lock_guard lock(pimpl_->mutex);
    if (logger_to) {
        for (auto& log : pimpl_->data) {
            auto formatter = logger_to->MakeFormatter(log.level, log.log_class, log.location);
            DispatchItem(log, *formatter);

            auto& li = formatter->ExtractLoggerItem();
            logger_to->Log(log.level, li);
        }
        pimpl_->data.clear();
    }
    pimpl_->forward_logger = logger_to;
}

void MemLogger::DispatchItem(formatters::LogItem& msg, formatters::Base& formatter) {
    formatter.SetText(msg.text);
    for (auto&& [key, value] : msg.tags) {
        formatter.AddTag(key, value);
    }
}

bool MemLogger::DoShouldLog(Level) const noexcept { return true; }

}  // namespace logging::impl

USERVER_NAMESPACE_END
