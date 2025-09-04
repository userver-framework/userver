#include <userver/ugrpc/server/impl/service_worker_impl.hpp>

#include <chrono>

#include <grpc/support/time.h>

#include <userver/logging/log.hpp>
#include <userver/tracing/opentelemetry.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/from_string.hpp>
#include <userver/utils/impl/source_location.hpp>
#include <userver/utils/impl/userver_experiments.hpp>

#include <ugrpc/impl/rpc_metadata.hpp>
#include <userver/ugrpc/impl/to_string.hpp>
#include <userver/ugrpc/status_codes.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

void ParseGenericCallName(
    std::string_view generic_call_name,
    std::string_view& call_name,
    std::string_view& service_name,
    std::string_view& method_name
) {
    UINVARIANT(
        !generic_call_name.empty() && generic_call_name[0] == '/', "Generic service call name must start with a '/'"
    );
    generic_call_name.remove_prefix(1);
    const auto slash_pos = generic_call_name.find('/');
    UINVARIANT(slash_pos != std::string_view::npos, "Generic service call name must contain a '/'");
    call_name = generic_call_name;
    service_name = generic_call_name.substr(0, slash_pos);
    method_name = generic_call_name.substr(slash_pos + 1);
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
