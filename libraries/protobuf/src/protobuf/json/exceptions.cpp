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
[[nodiscard]] constexpr const char* GetConversionErrorDescription() noexcept {
    if constexpr (std::is_same_v<TErrorCode, ParseErrorCode>) {
        return "Failed to convert JSON field '{}' with error '{}'";
    } else {
        return "Failed to convert protobuf message field '{}' with error '{}'";
    }
}

}  // namespace

template <typename TErrorCode>
ConversionError<TErrorCode>::ConversionError(const ConversionError<TErrorCode>::ErrorCodeType code, std::string path)
    : ConversionErrorBase(
          fmt::format(GetConversionErrorDescription<TErrorCode>(), path, GetConversionErrorCodeStr(code))
      ),
      error_info_(code, std::move(path))
{}

template class ConversionError<ParseErrorCode>;
template class ConversionError<PrintErrorCode>;

}  // namespace protobuf::json

USERVER_NAMESPACE_END
