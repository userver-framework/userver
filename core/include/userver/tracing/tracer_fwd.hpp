#pragma once

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace tracing {

enum class ReferenceType { kChild, kReference };

class Tracer;
using TracerPtr = std::shared_ptr<Tracer>;

}  // namespace tracing

USERVER_NAMESPACE_END
