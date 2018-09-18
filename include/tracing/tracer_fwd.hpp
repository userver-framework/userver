#pragma once

#include <memory>

namespace tracing {

class Tracer;
using TracerPtr = std::shared_ptr<Tracer>;

}  // namespace tracing
