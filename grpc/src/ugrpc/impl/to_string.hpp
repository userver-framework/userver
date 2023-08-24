#pragma once

#include <type_traits>

#include <grpcpp/support/config.h>
#include <grpcpp/support/string_ref.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

inline std::string ToString(grpc::string_ref str) {
  return {str.data(), str.size()};
}

inline decltype(auto) ToGrpcString(const std::string& str) {
  if constexpr (std::is_same_v<grpc::string, std::string>) {
    return str;
  } else {
    return grpc::string(str);
  }
}

inline grpc::string ToGrpcString(grpc::string_ref str) {
  return {str.data(), str.size()};
}

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
