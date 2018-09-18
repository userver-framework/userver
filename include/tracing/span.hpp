#pragma once

#include <opentracing/span.h>

#include <tracing/tracer_fwd.hpp>

namespace tracing {

class Span {
 public:
  explicit Span(std::unique_ptr<opentracing::Span>, TracerPtr tracer);

  Span(Span&& other) noexcept;

  ~Span();

  Span CreateChild(const std::string& name);

  opentracing::Span& GetOtSpan();

 private:
  std::unique_ptr<opentracing::Span> span_;
  std::shared_ptr<Tracer> tracer_;
};

}  // namespace tracing
