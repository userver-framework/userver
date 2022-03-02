#pragma once

#include <cstddef>
#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

/// Per-gRPC-service statically generated data
struct StaticServiceMetadata final {
  std::string_view service_full_name;
  const std::string_view* method_full_names;
  std::size_t method_count;
};

template <typename GrpcppService, std::size_t MethodCount>
constexpr StaticServiceMetadata MakeStaticServiceMetadata(
    const std::string_view (&method_full_names)[MethodCount]) noexcept {
  return {GrpcppService::service_full_name(), method_full_names, MethodCount};
}

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
