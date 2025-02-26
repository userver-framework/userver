#pragma once

#include <optional>
#include <string_view>

#include <userver/utils/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

/// Per-gRPC-service statically generated data
struct StaticServiceMetadata final {
    std::string_view service_full_name;
    utils::span<const std::string_view> method_full_names;
};

template <typename GrpcppService>
constexpr StaticServiceMetadata MakeStaticServiceMetadata(utils::span<const std::string_view> method_full_names
) noexcept {
    return {GrpcppService::service_full_name(), method_full_names};
}

constexpr std::size_t GetMethodsCount(const StaticServiceMetadata& metadata) noexcept {
    return metadata.method_full_names.size();
}

constexpr std::string_view GetMethodFullName(const StaticServiceMetadata& metadata, std::size_t method_id) {
    return metadata.method_full_names[method_id];
}

constexpr std::string_view GetMethodName(const StaticServiceMetadata& metadata, std::size_t method_id) {
    const auto& method_full_name = metadata.method_full_names[method_id];
    return method_full_name.substr(metadata.service_full_name.size() + 1);
}

std::optional<std::size_t>
FindMethod(const ugrpc::impl::StaticServiceMetadata& metadata, std::string_view method_full_name);

std::optional<std::size_t> FindMethod(
    const ugrpc::impl::StaticServiceMetadata& metadata,
    std::string_view service_name,
    std::string_view method_name
);

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
