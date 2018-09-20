#include <formats/json/types.hpp>

#include <cassert>

#include <boost/algorithm/string/join.hpp>

namespace formats {
namespace json {

std::string PathToString(const Path& path, const std::string& root,
                         char separator) {
  if (path.empty()) {
    return root;
  }
  return boost::algorithm::join(path, std::string(1, separator));
}

Json::ValueType ToNativeType(Type type) {
  switch (type) {
    case Type::kNull:
      return Json::nullValue;
    case Type::kArray:
      return Json::arrayValue;
    case Type::kObject:
      return Json::objectValue;
    default:
      assert(false &&
             "No mapping from formats::json::Type to Json::ValueType found");
      return Json::nullValue;
  }
}

}  // namespace json
}  // namespace formats
