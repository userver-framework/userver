#pragma once

#include <userver/logging/log_extra.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl::formatters {

/// Base log item, implementation can be as simple as std::string or complex as protobuf
struct LoggerItemBase {
    LoggerItemBase() = default;
    virtual ~LoggerItemBase() = default;

protected:
    LoggerItemBase(LoggerItemBase&&) = default;
    LoggerItemBase(const LoggerItemBase&) = default;
    LoggerItemBase& operator=(LoggerItemBase&&) = default;
    LoggerItemBase& operator=(const LoggerItemBase&) = default;
};

using LoggerItemRef = LoggerItemBase&;

class Base {
public:
    Base() = default;
    Base(Base&&) = delete;
    Base(const Base&) = delete;

    virtual ~Base() = default;

    virtual void AddTag(std::string_view key, const LogExtra::Value& value) = 0;

    virtual void AddTag(std::string_view key, std::string_view value) = 0;

    virtual void SetText(std::string_view text) = 0;

    virtual LoggerItemRef ExtractLoggerItem() = 0;
};

using BasePtr = std::unique_ptr<Base>;

}  // namespace logging::impl::formatters

USERVER_NAMESPACE_END
