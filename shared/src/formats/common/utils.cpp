#include <userver/formats/common/utils.hpp>

#include <algorithm>
#include <cstddef>
#include <optional>
#include <utility>

#include <boost/algorithm/string/split.hpp>

#include <userver/utils/assert.hpp>
#include <userver/utils/lazy_prvalue.hpp>

USERVER_NAMESPACE_BEGIN

using formats::json::Value;
using formats::json::ValueBuilder;

namespace formats::common {

namespace {

/// This struct allows to use utils::LazyPrvalue for ValueBuilder
struct ValueBuilderWrapper final {
  ValueBuilder value;
};

void AssignChild(ValueBuilder& parent, std::string&& key,
                 std::optional<ValueBuilderWrapper>& destination) {
  destination.emplace(utils::LazyPrvalue(
      [&] { return ValueBuilderWrapper{parent[std::move(key)]}; }));
}

ValueBuilder GetAtPath(ValueBuilder& parent, std::vector<std::string>&& path,
                       std::size_t path_size) {
  if (path_size == 1) {
    return parent[std::move(path[0])];
  }
  UINVARIANT(path_size != 0,
             "attempt to get a ValueBuilder element on an empty path");
  std::optional<ValueBuilderWrapper> first;
  std::optional<ValueBuilderWrapper> second;
  AssignChild(parent, std::move(path[0]), second);

  for (std::size_t i = 1; i + 2 < path_size; i += 2) {
    AssignChild(second->value, std::move(path[i]), first);
    AssignChild(first->value, std::move(path[i + 1]), second);
  }
  if (path_size % 2 == 1) {
    AssignChild(second->value, std::move(path[path_size - 2]), first);
    return first->value[std::move(path[path_size - 1])];
  } else {
    return second->value[std::move(path[path_size - 1])];
  }
}

}  // namespace

Value GetAtPath(Value parent, const std::vector<std::string>& path) {
  auto current_value = std::move(parent);
  for (const auto& current_key : path) {
    current_value = current_value[current_key];
  }
  return current_value;
}

ValueBuilder GetAtPath(ValueBuilder& parent, std::vector<std::string>&& path) {
  return GetAtPath(parent, std::move(path), path.size());
}

void SetAtPath(ValueBuilder& parent, std::vector<std::string>&& path,
               Value new_value) {
  if (path.empty()) {
    parent = std::move(new_value);
  } else {
    GetAtPath(parent, std::move(path)) = std::move(new_value);
  }
}

void RemoveAtPath(ValueBuilder& parent, std::vector<std::string>&& path) {
  if (path.empty()) {
    parent = ValueBuilder();
  } else if (path.size() == 1) {
    parent.Remove(path[0]);
  } else {
    auto& key = path[path.size() - 1];
    GetAtPath(parent, std::move(path), path.size() - 1).Remove(key);
  }
}

std::vector<std::string> SplitPathString(std::string_view path) {
  std::vector<std::string> split_result;
  if (path.empty()) return split_result;
  boost::algorithm::split(split_result, path, [](char c) { return c == '.'; });
  return split_result;
}

}  // namespace formats::common

USERVER_NAMESPACE_END
