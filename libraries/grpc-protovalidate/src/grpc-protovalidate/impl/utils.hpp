#pragma once

#include <userver/grpc-protovalidate/buf_validate.hpp>
#include <userver/logging/log_helper.hpp>

USERVER_NAMESPACE_BEGIN

namespace buf::validate {

USERVER_NAMESPACE::logging::LogHelper& operator<<(
    USERVER_NAMESPACE::logging::LogHelper& lh,
    const Violation& violation
);

}  // namespace buf::validate

USERVER_NAMESPACE_END
