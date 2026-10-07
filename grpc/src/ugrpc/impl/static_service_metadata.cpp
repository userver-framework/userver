#include <userver/ugrpc/impl/static_service_metadata.hpp>

#include <fmt/format.h>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

std::optional<std::size_t> FindMethod(
    const ugrpc::impl::StaticServiceMetadata& metadata,
    std::string_view method_full_name
) {
    for (std::size_t method_id = 0; method_id < GetMethodsCount(metadata); ++method_id) {
        if (GetMethodFullNameWithoutSlash(metadata, method_id) == method_full_name) {
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

MethodNameParts ParseMethodName(std::string_view method_full_name) {
    UINVARIANT(!method_full_name.empty() && method_full_name[0] == '/', "Method full name must start with a '/'");
    const auto slash_pos = method_full_name.find('/', 1);
    UINVARIANT(slash_pos != std::string_view::npos, "Method full name must contain a '/'");

    /*
     expected method_full_name format:

     "/<service-name>/<method-name>"

    */
    auto service_name = method_full_name.substr(1, slash_pos - 1);
    auto method_name = method_full_name.substr(slash_pos + 1);

    return MethodNameParts{
        .call_name = impl::GetCallName(method_full_name),
        .service_name = service_name,
        .method_name = method_name,
    };
}

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
