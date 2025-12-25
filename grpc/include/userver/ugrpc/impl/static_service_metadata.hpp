#pragma once

#include <optional>
#include <string_view>

#include <userver/utils/span.hpp>
#include <userver/utils/string_literal.hpp>

#include <userver/ugrpc/rpc_type.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

/// Descriptor of an RPC method
struct MethodDescriptor final {
    utils::StringLiteral method_full_name;
    RpcType method_type{RpcType::kUnary};
};

/// Per-gRPC-service statically generated data
struct StaticServiceMetadata final {
    utils::StringLiteral service_full_name;
    utils::span<const MethodDescriptor> methods;
};

template <typename GrpcppService>
constexpr StaticServiceMetadata MakeStaticServiceMetadata(utils::span<const MethodDescriptor> methods) noexcept {
    return {GrpcppService::service_full_name(), methods};
}

constexpr std::size_t GetMethodsCount(const StaticServiceMetadata& metadata) noexcept {
    return metadata.methods.size();
}

constexpr utils::StringLiteral GetMethodFullName(const StaticServiceMetadata& metadata, std::size_t method_id) {
    return metadata.methods[method_id].method_full_name;
}

constexpr utils::StringLiteral GetMethodName(const StaticServiceMetadata& metadata, std::size_t method_id) {
    auto result = ugrpc::impl::GetMethodFullName(metadata, method_id);
    result.remove_prefix(metadata.service_full_name.size() + 1);
    return result;
}

constexpr RpcType GetMethodType(const StaticServiceMetadata& metadata, std::size_t method_id) {
    return metadata.methods[method_id].method_type;
}

std::optional<std::size_t> FindMethod(
    const ugrpc::impl::StaticServiceMetadata& metadata,
    std::string_view method_full_name
);

std::optional<std::size_t> FindMethod(
    const ugrpc::impl::StaticServiceMetadata& metadata,
    std::string_view service_name,
    std::string_view method_name
);

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
