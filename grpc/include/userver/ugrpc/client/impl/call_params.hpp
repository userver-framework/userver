#pragma once

#include <optional>
#include <string_view>

#include <grpcpp/completion_queue.h>

#include <userver/dynamic_config/snapshot.hpp>

#include <userver/ugrpc/client/call_options.hpp>
#include <userver/ugrpc/client/generic_options.hpp>
#include <userver/ugrpc/client/impl/stub_handle.hpp>
#include <userver/ugrpc/client/middlewares/fwd.hpp>
#include <userver/ugrpc/impl/maybe_owned_string.hpp>
#include <userver/ugrpc/impl/statistics.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

class ClientData;

struct CallParams {
    std::string_view client_name;
    grpc::CompletionQueue& queue;
    dynamic_config::Snapshot config;
    ugrpc::impl::MaybeOwnedString call_name;
    CallOptions call_options;
    StubHandle stub;
    const Middlewares& middlewares;
    ugrpc::impl::MethodStatistics& statistics;
    const testsuite::GrpcControl& testsuite_grpc;
};

CallParams CreateCallParams(const ClientData& client_data, std::size_t method_id, CallOptions&& call_options);

CallParams CreateGenericCallParams(
    const ClientData& client_data,
    std::string_view call_name,
    CallOptions&& call_options,
    GenericOptions&& generic_options
);

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
