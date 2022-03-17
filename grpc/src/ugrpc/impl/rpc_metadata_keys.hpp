#pragma once

#include <string>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

extern const std::string kXYaTraceId;
extern const std::string kXYaSpanId;
extern const std::string kXYaRequestId;

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
