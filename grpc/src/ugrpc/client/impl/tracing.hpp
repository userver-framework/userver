#pragma once

#include <string_view>

#include <grpcpp/support/status.h>

#include <userver/tracing/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

void SetStatusForSpan(tracing::Span& span, const grpc::Status& status) noexcept;

void SetErrorForSpan(tracing::Span& span, std::string_view error_message) noexcept;

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
