#pragma once

#include <cstdio>

#include <mutex>
#include <string>
#include <vector>

#include <userver/logging/impl/logger_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

// kMaxLogItems is a hardcoded const and not config value as it is used before
// the main logger initialization and before yaml config is parsed.
inline constexpr auto kMaxLogItems = 10000;

class MemLogger final : public LoggerBase {
public:
    MemLogger() noexcept : LoggerBase(Format::kTskv) { SetLevel(Level::kDebug); }
    MemLogger(const MemLogger&) = delete;
    MemLogger(MemLogger&&) = delete;

    ~MemLogger() override {
        for (const auto& data : data_) {
            fputs(data.msg.c_str(), stderr);
        }
    }

    static MemLogger& GetMemLogger() noexcept {
        static MemLogger logger;
        return logger;
    }

    struct Data {
        Level level;
        std::string msg;
    };

    void Log(Level level, std::string_view msg) override {
        std::unique_lock lock(mutex_);
        if (forward_logger_) {
            forward_logger_->Log(level, msg);
            return;
        }

        if (data_.size() > kMaxLogItems) return;
        data_.push_back({level, std::string{msg}});
    }

    void Flush() override {}

    void ForwardTo(LoggerBase* logger_to) override {
        std::unique_lock lock(mutex_);
        if (logger_to) {
            for (const auto& log : data_) {
                logger_to->Log(log.level, log.msg);
            }
            data_.clear();
        }
        forward_logger_ = logger_to;
    }

protected:
    bool DoShouldLog(Level) const noexcept override { return true; }

private:
    std::mutex mutex_;
    std::vector<Data> data_;
    LoggerBase* forward_logger_{nullptr};
};

}  // namespace logging::impl

USERVER_NAMESPACE_END
