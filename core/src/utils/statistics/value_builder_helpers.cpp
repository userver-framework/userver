#include <utils/statistics/value_builder_helpers.hpp>

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

namespace {

void Merge(formats::json::ValueBuilder& original,
           formats::json::ValueBuilder&& patch) {
  if (original.IsObject() && patch.IsObject()) {
    for (const auto& [key, value] : Items(patch.ExtractValue())) {
      original[key] = value;
    }
  } else {
    original = std::move(patch);
  }
}

void SetSubField(formats::json::ValueBuilder& object,
                 std::vector<std::string>&& path, std::size_t path_idx,
                 formats::json::ValueBuilder&& value) {
  if (path_idx == path.size()) {
    Merge(object, value.ExtractValue());
  } else {
    auto child = object[std::move(path[path_idx])];
    SetSubField(child, std::move(path), path_idx + 1, std::move(value));
  }
}

}  // namespace

void SetSubField(formats::json::ValueBuilder& object,
                 std::vector<std::string>&& path,
                 formats::json::ValueBuilder&& value) {
  SetSubField(object, std::move(path), 0, std::move(value));
}

std::vector<std::string> SplitPath(std::string_view path) {
  if (path.empty()) return {};
  std::vector<std::string> split_path;
  boost::algorithm::split(split_path, path, [](char c) { return c == '.'; });
  return split_path;
}

std::string JoinPath(const std::vector<std::string>& path) {
  return boost::algorithm::join(path, ".");
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
