#include <tracing/tracing_variant.hpp>

namespace tracing {

opentracing::Value ToOpentracingValue(logging::LogExtra::Value value) {
  return boost::apply_visitor(
      [](auto& value) -> opentracing::Value { return std::move(value); },
      value);
}

}  // namespace tracing
