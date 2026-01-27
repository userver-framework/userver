#pragma once

#include <userver/ugrpc/client/call_options.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

class CallOptionsAccessor {
public:
    static std::unique_ptr<grpc::ClientContext> CreateClientContext(const CallOptions& call_options);
};

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
