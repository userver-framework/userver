#pragma once

#include <userver/logging/impl/formatters/base.hpp>

#include <userver/logging/format.hpp>
#include <userver/logging/impl/logger_base.hpp>
#include <userver/logging/level.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl::formatters {

class Tskv final : public Base {
public:
    Tskv(Level level, Format format, const utils::impl::SourceLocation& source_location);

    void AddTag(std::string_view key, const LogExtra::Value& value) override;

    void AddTag(std::string_view key, std::string_view value) override;

    void SetText(std::string_view text) override;

    LoggerItemBase& ExtractLoggerItem() override {
        Finish();
        return item_;
    }

private:
    void DoAddTag(std::string_view key, std::string_view value, bool value_is_escaped);

    void Finish();

    TextLogItem item_;
    const Format format_;
};

}  // namespace logging::impl::formatters

USERVER_NAMESPACE_END
