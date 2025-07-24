#pragma once

#include <optional>
#include <string_view>

#include <grpcpp/client_context.h>
#include <grpcpp/support/status.h>

#include <userver/tracing/in_place_span.hpp>
#include <userver/tracing/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

void SetupSpan(std::optional<tracing::InPlaceSpan>& span_holder, std::string_view call_name);

void AddTracingMetadata(grpc::ClientContext& client_context, tracing::Span& span);

void SetStatusForSpan(tracing::Span& span, const grpc::Status& status) noexcept;

void SetErrorForSpan(tracing::Span& span, std::string_view error_message) noexcept;

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
