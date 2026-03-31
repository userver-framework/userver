#include <userver/protobuf/json/exceptions.hpp>

#include <type_traits>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace protobuf::json {

namespace {

template <typename TErrorCode>
[[nodiscard]] constexpr const char* GetConversionErrorCodeStr(const TErrorCode code) noexcept {
    if constexpr (std::is_same_v<TErrorCode, ParseErrorCode>) {
        switch (code) {
            case TErrorCode::kUnknownField:
                return "unknown field";
            case TErrorCode::kUnknownEnum:
                return "unknown enum";
            case TErrorCode::kMultipleOneofFields:
                return "multiple oneof fields";
            case TErrorCode::kInvalidType:
                return "invalid type";
            case TErrorCode::kInvalidValue:
                return "invalid value";
            default:
                return "unexpected";
        }
    } else {
        if (code == TErrorCode::kInvalidValue) {
            return "invalid value";
        } else {
            return "unexpected";
        }
    }
}

template <typename TErrorCode>
[[nodiscard]] constexpr const char* GetConversionErrorDescription(bool with_description) noexcept {
    if constexpr (std::is_same_v<TErrorCode, ParseErrorCode>) {
        return with_description
                   ? "Failed to convert JSON field '{}' with error '{}' ({})"
                   : "Failed to convert JSON field '{}' with error '{}'";
    } else {
        return with_description
                   ? "Failed to convert protobuf message field '{}' with error '{}' ({})"
                   : "Failed to convert protobuf message field '{}' with error '{}'";
    }
}

}  // namespace

template <typename TErrorCode>
ConversionError<TErrorCode>::ConversionError(
    const ConversionError<TErrorCode>::ErrorCodeType code,
    std::string path,
    std::string_view description
)
    : ConversionErrorBase(
          description.empty()
              ? fmt::format(GetConversionErrorDescription<TErrorCode>(false), path, GetConversionErrorCodeStr(code))
              : fmt::format(
                    GetConversionErrorDescription<TErrorCode>(true),
                    path,
                    GetConversionErrorCodeStr(code),
                    description
                )
      ),
      error_info_(code, std::move(path))
{}

template class ConversionError<ParseErrorCode>;
template class ConversionError<PrintErrorCode>;

}  // namespace protobuf::json

USERVER_NAMESPACE_END
