#include <logging/impl/formatters/tskv.hpp>

#include <iostream>

#include <fmt/chrono.h>
#include <fmt/compile.h>
#include <fmt/format.h>

#include <logging/timestamp.hpp>

#include <userver/compiler/thread_local.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/encoding/tskv.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl::formatters {

Tskv::Tskv(Level level, Format format, const utils::impl::SourceLocation& location) : format_(format) {
    switch (format) {
        case Format::kTskv: {
            constexpr std::string_view kTemplate = "tskv\ttimestamp=0000-00-00T00:00:00.000000\tlevel=";
            const auto now = std::chrono::system_clock::now();
            const auto level_string = logging::ToUpperCaseString(level);
            item_.log_line.resize(kTemplate.size() + level_string.size());
            fmt::format_to(
                item_.log_line.data(),
                FMT_COMPILE("tskv\ttimestamp={}.{:06}\tlevel={}"),
                GetCurrentLocalTimeString(now).ToStringView(),
                FractionalMicroseconds(now),
                level_string
            );
            fmt::format_to(
                std::back_inserter(item_.log_line),
                FMT_COMPILE("\tmodule={} ( {}:{} )"),
                location.GetFunctionName(),
                location.GetFileName(),
                location.GetLineString()
            );
            return;
        }
        case Format::kLtsv: {
            constexpr std::string_view kTemplate = "timestamp:0000-00-00T00:00:00.000000\tlevel:";
            const auto now = TimePoint::clock::now();
            const auto level_string = logging::ToUpperCaseString(level);
            item_.log_line.resize(kTemplate.size() + level_string.size());
            fmt::format_to(
                item_.log_line.data(),
                FMT_COMPILE("timestamp:{}.{:06}\tlevel:{}"),
                GetCurrentLocalTimeString(now).ToStringView(),
                FractionalMicroseconds(now),
                level_string
            );
            fmt::format_to(
                std::back_inserter(item_.log_line),
                FMT_COMPILE("\tmodule:{} ( {}:{} )"),
                location.GetFunctionName(),
                location.GetFileName(),
                location.GetLineString()
            );
            return;
        }
        case Format::kRaw: {
            item_.log_line.append(std::string_view{"tskv"});
            return;
        }
        default:
            UINVARIANT(false, "Invalid format");
    }
}

void Tskv::AddTag(std::string_view key, const LogExtra::Value& value) {
    std::visit(
        [&, this](const auto& x) {
            if constexpr (std::is_same_v<decltype(x), const std::string&>)
                DoAddTag(key, std::string_view{x}, false);
            else if constexpr (std::is_same_v<decltype(x), const JsonString&>)
                DoAddTag(key, x.GetView(), false);
            else
                DoAddTag(key, fmt::to_string(x), true);
        },
        value
    );
}

void Tskv::AddTag(std::string_view key, std::string_view value) { return DoAddTag(key, value, false); }

void Tskv::DoAddTag(std::string_view key, std::string_view value, bool value_is_escaped) {
    // TODO: repeated tags
    item_.log_line += utils::encoding::kTskvPairsSeparator;
    if (!utils::encoding::ShouldKeyBeEscaped(key)) {
        item_.log_line += key;
    } else {
        utils::encoding::EncodeTskv(item_.log_line, key, utils::encoding::EncodeTskvMode::kKeyReplacePeriod);
    }
    switch (format_) {
        case Format::kTskv:
        case Format::kRaw:
            item_.log_line += "=";
            break;
        case Format::kLtsv:
            item_.log_line += ":";
            break;
        default:
            UINVARIANT(false, "Invalid format");
    }

    if (!value_is_escaped) {
        utils::encoding::EncodeTskv(item_.log_line, value, utils::encoding::EncodeTskvMode::kValue);
    } else {
        item_.log_line += value;
    }
}

void Tskv::SetText(std::string_view text) { AddTag("text", text); }

void Tskv::Finish() { item_.log_line += "\n"; }

}  // namespace logging::impl::formatters

USERVER_NAMESPACE_END
