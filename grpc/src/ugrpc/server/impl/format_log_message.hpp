#pragma once

#include <google/protobuf/message.h>
#include <grpcpp/support/string_ref.h>

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/logging/impl/log_extra_tskv_formatter.hpp>
#include <userver/logging/impl/logger_base.hpp>
#include <userver/logging/log_extra.hpp>
#include <userver/ugrpc/status_codes.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

logging::impl::LogExtraTskvFormatter FormatLogMessage(
    const std::multimap<grpc::string_ref, grpc::string_ref>& metadata,
    std::string_view peer,
    std::chrono::system_clock::time_point start_time,
    std::string_view call_name,
    grpc::StatusCode code,
    const logging::LogExtra* log_extra
);

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
