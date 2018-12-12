#pragma once

#include <memory>

namespace tracing {

enum class ReferenceType { kChild, kReference };

class Tracer;
using TracerPtr = std::shared_ptr<Tracer>;

}  // namespace tracing
