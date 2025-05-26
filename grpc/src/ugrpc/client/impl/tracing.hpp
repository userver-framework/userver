#pragma once

#include <optional>
#include <string_view>

#include <grpcpp/client_context.h>
#include <grpcpp/support/status.h>

#include <userver/tracing/in_place_span.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

class CallState;

void SetupSpan(
    std::optional<tracing::InPlaceSpan>& span_holder,
    grpc::ClientContext& context,
    std::string_view call_name
);

void SetErrorAndResetSpan(CallState& state, std::string_view error_message);

void SetStatusAndResetSpan(CallState& state, const grpc::Status& status);

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
