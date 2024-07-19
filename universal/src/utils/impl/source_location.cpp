#include <userver/utils/impl/source_location.hpp>

#include <userver/utils/algo.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

std::string ToString(const SourceLocation& location) {
  return StrCat(location.GetFunctionName(), " (", location.GetFileName(), ":",
                location.GetLineString(), ")");
}

}  // namespace utils::impl

USERVER_NAMESPACE_END
