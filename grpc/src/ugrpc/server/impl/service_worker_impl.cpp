#include <userver/ugrpc/server/impl/service_worker_impl.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

GenericMethodParseResults ParseGenericMethodName(std::string_view generic_method_name) {
    UINVARIANT(
        !generic_method_name.empty() && generic_method_name[0] == '/',
        "Generic service method name must start with a '/'"
    );
    const auto slash_pos = generic_method_name.find('/', 1);
    UINVARIANT(slash_pos != std::string_view::npos, "Generic service method name must contain a '/'");

    /*
     expected generic_method_name format:

     "/<service-name>/<method-name>"

    */
    auto call_name = generic_method_name.substr(1);
    auto service_name = generic_method_name.substr(1, slash_pos - 1);
    auto method_name = generic_method_name.substr(slash_pos + 1);

    return GenericMethodParseResults{
        .call_name = call_name,
        .service_name = service_name,
        .method_name = method_name,
    };
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
