#pragma once

#include <optional>
#include <string_view>

#include <userver/utils/span.hpp>

#include <userver/ugrpc/impl/rpc_type.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

/// Descriptor of an RPC method
struct MethodDescriptor final {
    std::string_view method_full_name;
    RpcType method_type;
};

/// Per-gRPC-service statically generated data
struct StaticServiceMetadata final {
    std::string_view service_full_name;
    utils::span<const MethodDescriptor> methods;
};

template <typename GrpcppService>
constexpr StaticServiceMetadata MakeStaticServiceMetadata(utils::span<const MethodDescriptor> methods) noexcept {
    return {GrpcppService::service_full_name(), methods};
}

constexpr std::size_t GetMethodsCount(const StaticServiceMetadata& metadata) noexcept {
    return metadata.methods.size();
}

constexpr std::string_view GetMethodFullName(const StaticServiceMetadata& metadata, std::size_t method_id) {
    return metadata.methods[method_id].method_full_name;
}

constexpr std::string_view GetMethodName(const StaticServiceMetadata& metadata, std::size_t method_id) {
    auto method_full_name = GetMethodFullName(metadata, method_id);
    return method_full_name.substr(metadata.service_full_name.size() + 1);
}

constexpr RpcType GetMethodType(const StaticServiceMetadata& metadata, std::size_t method_id) {
    return metadata.methods[method_id].method_type;
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
