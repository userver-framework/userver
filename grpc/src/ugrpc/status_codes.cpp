#include <userver/ugrpc/status_codes.hpp>

#include <fmt/format.h>

#include <userver/utils/assert.hpp>
#include <userver/utils/trivial_map.hpp>
#include <userver/utils/underlying_value.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc {

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

grpc::StatusCode StatusCodeFromString(std::string_view str) {
  const auto code = kStatusCodesMap.TryFindBySecond(str);
  if (code) {
    return *code;
  }

  throw std::runtime_error(fmt::format("Invalid grpc status code: {}", str));
}

std::string_view ToString(grpc::StatusCode code) noexcept {
  const auto str = kStatusCodesMap.TryFindByFirst(code);
  if (str) {
    return *str;
  }

  UASSERT_MSG(false, fmt::format("Invalid grpc status code: {}",
                                 utils::UnderlyingValue(code)));
  return "<invalid status>";
}

}  // namespace ugrpc

USERVER_NAMESPACE_END
