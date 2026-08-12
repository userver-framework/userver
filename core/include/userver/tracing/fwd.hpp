#pragma once

/// @file userver/tracing/fwd.hpp
/// @brief Forward declarations of the tracing types.

USERVER_NAMESPACE_BEGIN

namespace tracing {

enum class ReferenceType { kChild, kReference };

class SpanBuilder;
struct SpanEvent;
class Span;
class AnyValue;

}  // namespace tracing

USERVER_NAMESPACE_END
