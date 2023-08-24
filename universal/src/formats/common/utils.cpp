#include <userver/formats/common/utils.hpp>

#include <utility>

#include <boost/algorithm/string/split.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::common {

std::vector<std::string> SplitPathString(std::string_view path) {
  std::vector<std::string> split_result;
  if (path.empty()) return split_result;
  boost::algorithm::split(split_result, path, [](char c) { return c == '.'; });
  return split_result;
}

}  // namespace formats::common

USERVER_NAMESPACE_END
