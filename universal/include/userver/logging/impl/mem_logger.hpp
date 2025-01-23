#pragma once

#include <userver/logging/impl/formatters/base.hpp>
#include <userver/logging/impl/logger_base.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

namespace formatters {
struct LogItem;
}

class MemLogger final : public LoggerBase {
public:
    MemLogger() noexcept;
    MemLogger(const MemLogger&) = delete;
    MemLogger(MemLogger&&) = delete;

    ~MemLogger() override;

    static MemLogger& GetMemLogger() noexcept;

    void Log(Level, formatters::LoggerItemRef item) override;

    formatters::BasePtr MakeFormatter(Level level, LogClass log_class) override;

    void ForwardTo(LoggerBase* logger_to) override;

    void DropLogs();

protected:
    bool DoShouldLog(Level) const noexcept override;

private:
    void DispatchItem(formatters::LogItem& msg, formatters::Base& formatter);

    struct Impl;
    utils::FastPimpl<Impl, 96, 8> pimpl_;
};

}  // namespace logging::impl

USERVER_NAMESPACE_END
