#include <userver/ugrpc/status_codes.hpp>

#include <fmt/format.h>

#include <userver/formats/json.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/trivial_map.hpp>
#include <userver/utils/underlying_value.hpp>
#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc {

namespace {

// See grpcpp StatusCode documentation for the list of possible values:
// https://grpc.github.io/grpc/cpp/namespacegrpc.html#aff1730578c90160528f6a8d67ef5c43b
constexpr utils::TrivialBiMap kStatusCodesMap([](auto selector) {
    return selector()
        .Case(grpc::StatusCode::OK, "OK")
        .Case(grpc::StatusCode::CANCELLED, "CANCELLED")
        .Case(grpc::StatusCode::UNKNOWN, "UNKNOWN")
        .Case(grpc::StatusCode::INVALID_ARGUMENT, "INVALID_ARGUMENT")
        .Case(grpc::StatusCode::DEADLINE_EXCEEDED, "DEADLINE_EXCEEDED")
        .Case(grpc::StatusCode::NOT_FOUND, "NOT_FOUND")
        .Case(grpc::StatusCode::ALREADY_EXISTS, "ALREADY_EXISTS")
        .Case(grpc::StatusCode::PERMISSION_DENIED, "PERMISSION_DENIED")
        .Case(grpc::StatusCode::RESOURCE_EXHAUSTED, "RESOURCE_EXHAUSTED")
        .Case(grpc::StatusCode::FAILED_PRECONDITION, "FAILED_PRECONDITION")
        .Case(grpc::StatusCode::ABORTED, "ABORTED")
        .Case(grpc::StatusCode::OUT_OF_RANGE, "OUT_OF_RANGE")
        .Case(grpc::StatusCode::UNIMPLEMENTED, "UNIMPLEMENTED")
        .Case(grpc::StatusCode::INTERNAL, "INTERNAL")
        .Case(grpc::StatusCode::UNAVAILABLE, "UNAVAILABLE")
        .Case(grpc::StatusCode::DATA_LOSS, "DATA_LOSS")
        .Case(grpc::StatusCode::UNAUTHENTICATED, "UNAUTHENTICATED");
});

}  // namespace

grpc::StatusCode StatusCodeFromString(std::string_view str) {
    const auto code = kStatusCodesMap.TryFindBySecond(str);
    if (code) {
        return *code;
    }

    throw std::runtime_error(fmt::format("Invalid grpc status code: {}", str));
}

std::string ToString(grpc::StatusCode code) noexcept {
    const auto str = kStatusCodesMap.TryFindByFirst(code);
    if (str) {
        return std::string{*str};
    }

    return fmt::format("CODE({})", utils::UnderlyingValue(code));
}

// See https://opentelemetry.io/docs/specs/semconv/rpc/grpc/
// Except that we don't mark DEADLINE_EXCEEDED as a server error.
bool IsServerError(grpc::StatusCode status) noexcept {
    switch (status) {
        case grpc::StatusCode::UNKNOWN:
        case grpc::StatusCode::UNIMPLEMENTED:
        case grpc::StatusCode::INTERNAL:
        case grpc::StatusCode::UNAVAILABLE:
        case grpc::StatusCode::DATA_LOSS:
            return true;
        default:
            return false;
    }
}

}  // namespace ugrpc

namespace formats::parse {

grpc::StatusCode Parse(const yaml_config::YamlConfig& value, To<grpc::StatusCode>) {
    return utils::ParseFromValueString(value, ugrpc::kStatusCodesMap);
}

grpc::StatusCode Parse(std::string_view value, To<grpc::StatusCode>) {
    const auto result = ugrpc::kStatusCodesMap.TryFind(value);
    if (!result) {
        throw std::runtime_error(fmt::format(
            "Invalid value of grpc::StatusCode: '{}' is not one of {}",
            value,
            ugrpc::kStatusCodesMap.DescribeSecond()
        ));
    }
    return *result;
}

}  // namespace formats::parse

namespace formats::serialize {

formats::json::Value Serialize(const grpc::StatusCode& value, formats::serialize::To<formats::json::Value>) {
    return formats::json::ValueBuilder(ugrpc::ToString(value)).ExtractValue();
}

}  // namespace formats::serialize

USERVER_NAMESPACE_END
