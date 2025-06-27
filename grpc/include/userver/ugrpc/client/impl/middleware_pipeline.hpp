#pragma once

#include <google/protobuf/message.h>
#include <grpcpp/support/status.h>

#include <userver/ugrpc/client/impl/call_state.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

struct MiddlewarePipeline {
    static void PreStartCall(CallState& state);

    static void PreSendMessage(CallState& state, const google::protobuf::Message& message);

    static void PostRecvMessage(CallState& state, const google::protobuf::Message& message);

    static void PostFinish(CallState& state);
};

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
