#include <logging/impl/formatters/tskv.hpp>

#include <fmt/chrono.h>
#include <fmt/compile.h>
#include <fmt/format.h>

#include <userver/compiler/thread_local.hpp>
#include <userver/logging/impl/timestamp.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/encoding/tskv.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

template <class... Args>
char* Append(char* output, const Args&... args) {
    const auto append_impl = [&output](std::string_view data) noexcept {
        std::memcpy(output, data.data(), data.size());
        output += data.size();
    };
    (append_impl(args), ...);
    return output;
};

}  // namespace

namespace logging::impl::formatters {

Tskv::Tskv(Level level, Format format, const utils::impl::SourceLocation& location)
    : format_(format)
{
    switch (format) {
        case Format::kTskv: {
            constexpr std::string_view kTemplate = "tskv\ttimestamp=0000-00-00T00:00:00.000000\tlevel=\tmodule= ( : )";
            const auto now = std::chrono::system_clock::now();
            const auto level_string = logging::ToUpperCaseString(level);
            item_.log_line.resize_and_overwrite(
                kTemplate.size() + level_string.size() + location.GetFunctionName().size() +
                    location.GetFileName().size() + location.GetLineString().size(),
                [&](char* output, std::size_t size) {
                    output = Append(output, "tskv\ttimestamp=", GetCurrentLocalTimeString(now).ToStringView(), ".");
                    output = fmt::format_to(output, FMT_COMPILE("{:06}"), FractionalMicroseconds(now));
                    output = Append(output, "\tlevel=", level_string, "\tmodule=", location.GetFunctionName());
                    output = Append(output, " ( ", location.GetFileName(), ":", location.GetLineString(), " )");
                    return size;
                }
            );
            return;
        }
        case Format::kLtsv: {
            constexpr std::string_view kTemplate = "timestamp:0000-00-00T00:00:00.000000\tlevel:\tmodule: ( : )";
            const auto now = std::chrono::system_clock::now();
            const auto level_string = logging::ToUpperCaseString(level);
            item_.log_line.resize_and_overwrite(
                kTemplate.size() + level_string.size() + location.GetFunctionName().size() +
                    location.GetFileName().size() + location.GetLineString().size(),
                [&](char* output, std::size_t size) {
                    output = Append(output, "timestamp:", GetCurrentLocalTimeString(now).ToStringView(), ".");
                    output = fmt::format_to(output, FMT_COMPILE("{:06}"), FractionalMicroseconds(now));
                    output = Append(output, "\tlevel:", level_string, "\tmodule:", location.GetFunctionName());
                    output = Append(output, " ( ", location.GetFileName(), ":", location.GetLineString(), " )");
                    return size;
                }
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
            if constexpr (std::is_same_v<decltype(x), const std::string&>) {
                DoAddTag(key, std::string_view{x}, false);
            } else if constexpr (std::is_same_v<decltype(x), const JsonString&>) {
                DoAddTag(key, x.GetView(), false);
            } else if constexpr (std::is_same_v<decltype(x), const bool&>) {
                DoAddTag(key, (x ? "1" : "0"), true);
            } else {
                DoAddTag(key, fmt::to_string(x), true);
            }
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
        utils::encoding::EncodeTskv(item_.log_line, key, utils::encoding::EncodeTskvMode::kKey);
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
