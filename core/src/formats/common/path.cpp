#include <formats/common/path.hpp>

#include <boost/algorithm/string/join.hpp>

namespace formats::common {

std::string PathToString(const Path& path, const std::string& root,
                         char separator) {
  if (path.empty()) {
    return root;
  }
  return boost::algorithm::join(path, std::string(1, separator));
}

}  // namespace formats::common
