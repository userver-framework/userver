#include <tracing/no_log_spans.hpp>

#include <userver/formats/json/value.hpp>
#include <userver/formats/parse/boost_flat_containers.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

NoLogSpans Parse(const formats::json::Value& value,
                 formats::parse::To<NoLogSpans>) {
  using Container = boost::container::flat_set<std::string>;

  NoLogSpans ret;
  ret.names = value["names"].As<Container>();
  ret.prefixes = value["prefixes"].As<Container>();

  return ret;
}

}  // namespace tracing

USERVER_NAMESPACE_END
