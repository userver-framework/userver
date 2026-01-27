#pragma once

#include <chrono>
#include <map>
#include <optional>
#include <string_view>

#include <grpcpp/support/status_code_enum.h>
#include <grpcpp/support/string_ref.h>

#include <userver/logging/impl/log_extra_tskv_formatter.hpp>
#include <userver/logging/log_extra.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

logging::impl::LogExtraTskvFormatter FormatLogMessage(
    const std::multimap<grpc::string_ref, grpc::string_ref>& metadata,
    std::string_view peer,
    std::chrono::system_clock::time_point start_time,
    std::string_view call_name,
    std::optional<grpc::StatusCode> code,
    const logging::LogExtra* log_extra
);

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
