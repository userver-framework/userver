#pragma once

#include <google/protobuf/message.h>
#include <grpcpp/support/string_ref.h>

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/ugrpc/status_codes.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

std::string FormatLogMessage(
    const std::multimap<grpc::string_ref, grpc::string_ref>& metadata,
    std::string_view peer, std::chrono::system_clock::time_point start_time,
    std::string_view call_name, grpc::StatusCode code);

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
