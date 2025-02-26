#include <userver/ugrpc/impl/static_service_metadata.hpp>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

std::optional<std::size_t>
FindMethod(const ugrpc::impl::StaticServiceMetadata& metadata, std::string_view method_full_name) {
    for (std::size_t method_id = 0; method_id < GetMethodsCount(metadata); ++method_id) {
        if (GetMethodFullName(metadata, method_id) == method_full_name) {
            return method_id;
        }
    }
    return std::nullopt;
}

std::optional<std::size_t> FindMethod(
    const ugrpc::impl::StaticServiceMetadata& metadata,
    std::string_view service_name,
    std::string_view method_name
) {
    return FindMethod(metadata, fmt::format("{}/{}", service_name, method_name));
}

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
