#include <userver/utils/from_string.hpp>

#include <stdexcept>

#include <fmt/format.h>

#include <userver/compiler/demangle.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace impl {

void ThrowFromStringException(FromStringErrorCode code, std::string_view input, const std::type_info& result_type) {
    throw FromStringException(
        code,
        fmt::format(
            R"(utils::FromString error: "{}" while converting "{}" to {})",
            ToString(code),
            input,
            compiler::GetTypeName(result_type)
        )
    );
}

}  // namespace impl

FromStringException::FromStringException(FromStringErrorCode code, const std::string& what)
    : std::runtime_error(what),
      code_(code)
{}

std::int64_t FromHexString(std::string_view str) {
    std::int64_t result{};
    const auto* str_begin = str.data();
    const auto* str_end = str_begin + str.size();
    const auto [end, error_code] = std::from_chars(str_begin, str_end, result, 16);
    if (error_code != std::errc{} || end != str_end) {
        throw std::runtime_error(
            fmt::format(R"(utils::FromHexString error: Error while converting "{}" to std::int64_t)", str)
        );
    }

    return result;
}

}  // namespace utils

USERVER_NAMESPACE_END
