#include <tracing/tracing_variant.hpp>

#include <opentracing/variant/variant.hpp>

namespace tracing {

opentracing::Value ToOpentracingValue(logging::LogExtra::Value value) {
  return boost::apply_visitor(
      [](auto& value) -> opentracing::Value { return std::move(value); },
      value);
}

namespace {

class Visitor {
 public:
  logging::LogExtra::Value operator()(opentracing::Values&) { return ""; }

  logging::LogExtra::Value operator()(opentracing::Dictionary&) { return ""; }

  template <typename T>
  logging::LogExtra::Value operator()(T& value) {
    return std::move(value);
  }
};

}  // namespace

logging::LogExtra::Value FromOpentracingValue(opentracing::Value value) {
  return opentracing::util::apply_visitor(Visitor(), value);
}

}  // namespace tracing
