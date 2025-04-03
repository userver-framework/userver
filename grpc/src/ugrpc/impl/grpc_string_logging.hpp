#pragma once

#include <grpcpp/support/string_ref.h>

#include <userver/logging/log_helper.hpp>

namespace grpc {

inline USERVER_NAMESPACE::logging::LogHelper&
operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, grpc::string_ref s) {
    return lh << std::string_view{s.data(), s.size()};
}

}  // namespace grpc
