#include <userver/chaotic/io/std/chrono/milliseconds.hpp>

#include <userver/utils/string_to_duration.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::convert {

std::string Convert(const std::chrono::milliseconds& value, chaotic::convert::To<std::string>) {
    return std::to_string(value.count()) + "ms";
}

std::chrono::milliseconds Convert(const std::string& str, chaotic::convert::To<std::chrono::milliseconds>) {
    return std::chrono::milliseconds{utils::StringToDuration(str)};
}

std::chrono::milliseconds Convert(std::string_view str, chaotic::convert::To<std::chrono::milliseconds>) {
    return std::chrono::milliseconds{utils::StringToDuration(str)};
}

}  // namespace chaotic::convert

USERVER_NAMESPACE_END
