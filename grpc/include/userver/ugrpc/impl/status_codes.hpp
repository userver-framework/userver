#pragma once

#include <string_view>

#include <grpcpp/support/status.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

std::string_view ToString(grpc::StatusCode code) noexcept;

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
