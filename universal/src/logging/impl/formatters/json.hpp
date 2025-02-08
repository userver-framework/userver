#pragma once

#include <userver/formats/json/string_builder.hpp>
#include <userver/logging/format.hpp>
#include <userver/logging/impl/formatters/base.hpp>
#include <userver/logging/impl/logger_base.hpp>
#include <userver/logging/level.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl::formatters {

class Json final : public Base {
public:
    explicit Json(Level level, Format format, const utils::impl::SourceLocation& source_location) noexcept(false);

    Json(const Json&) = delete;

    void AddTag(std::string_view key, const LogExtra::Value& value) override;

    void AddTag(std::string_view key, std::string_view value) override;

    void SetText(std::string_view text) override;

    LoggerItemBase& ExtractLoggerItem() override {
        Finish();
        item_.log_line = sb_.GetString() + "\n";
        return item_;
    }

private:
    void Finish();

    const Format format_;
    formats::json::StringBuilder sb_;
    std::optional<formats::json::StringBuilder::ObjectGuard> object_;
    TextLogItem item_;
};

}  // namespace logging::impl::formatters

USERVER_NAMESPACE_END
