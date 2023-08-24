#pragma once

#include <string_view>

#include <userver/utils/impl/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

/// Per-gRPC-service statically generated data
struct StaticServiceMetadata final {
  std::string_view service_full_name;
  utils::impl::Span<const std::string_view> method_full_names;
};

template <typename GrpcppService>
constexpr StaticServiceMetadata MakeStaticServiceMetadata(
    utils::impl::Span<const std::string_view> method_full_names) noexcept {
  return {GrpcppService::service_full_name(), method_full_names};
}

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
