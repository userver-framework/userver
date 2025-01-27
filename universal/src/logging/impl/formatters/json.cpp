#include <logging/impl/formatters/json.hpp>

#include <chrono>

#include <fmt/compile.h>
#include <fmt/format.h>

#include <logging/timestamp.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl::formatters {

Json::Json(Level level, Format format) : format_(format) {
    const auto now = std::chrono::system_clock::now();

    object_.emplace(sb_);

    sb_.Key((format_ == Format::kJson) ? "timestamp" : "@timestamp");
    sb_.WriteString(
        fmt::format(FMT_COMPILE("{}.{:06}"), GetCurrentTimeString(now).ToStringView(), FractionalMicroseconds(now))
    );

    sb_.Key((format_ == Format::kJson) ? "level" : "levelStr");
    sb_.WriteString(logging::ToUpperCaseString(level));
}

void Json::AddTag(std::string_view key, const LogExtra::Value& value) {
    std::visit(
        [&, this](const auto& x) {
            sb_.Key(key);
            WriteToStream(x, sb_);
        },
        value
    );
}

void Json::AddTag(std::string_view key, std::string_view value) {
    sb_.Key(key);
    sb_.WriteString(value);
}

void Json::SetText(std::string_view text) { AddTag((format_ == Format::kJson) ? "text" : "message", text); }

void Json::Finish() { object_.reset(); }

}  // namespace logging::impl::formatters

USERVER_NAMESPACE_END
