#pragma once

#include <userver/logging/impl/formatters/base.hpp>
#include <userver/logging/impl/logger_base.hpp>
#include <userver/logging/level.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl::formatters {

struct LogItem : formatters::LoggerItemBase {
    Level level{Level::kNone};
    LogClass log_class{LogClass::kLog};
    utils::impl::SourceLocation location{utils::impl::SourceLocation::Custom(0, {}, {})};
    // Could be std::unordered_map, but it is slow
    boost::container::small_vector<std::pair<std::string, LogExtra::Value>, 20> tags;
    std::string text;
};

class Struct final : public Base {
public:
    Struct(Level level, LogClass log_class, const utils::impl::SourceLocation& location) noexcept(false);

    Struct(const Struct&) = delete;

    void AddTag(std::string_view key, const LogExtra::Value& value) override;

    void AddTag(std::string_view key, std::string_view value) override;

    void SetText(std::string_view text) override;

    LoggerItemRef ExtractLoggerItem() override;

private:
    void Finish();

    LogItem item_;
};

}  // namespace logging::impl::formatters

USERVER_NAMESPACE_END
