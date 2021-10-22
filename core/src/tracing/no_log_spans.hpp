#pragma once

#include <string>

#include <boost/container/flat_set.hpp>

#include <userver/formats/parse/to.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json {
class Value;
}

namespace tracing {

struct NoLogSpans {
  boost::container::flat_set<std::string> prefixes;
  boost::container::flat_set<std::string> names;
};

NoLogSpans Parse(const formats::json::Value&, formats::parse::To<NoLogSpans>);

}  // namespace tracing

USERVER_NAMESPACE_END
