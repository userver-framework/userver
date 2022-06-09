#pragma once

/// @file include/userver/formats/common/utils.hpp
/// @brief Utils for `Value` and `ValueBuilder`

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <userver/formats/common/meta.hpp>
#include <userver/formats/common/transfer_tag.hpp>
#include <userver/formats/common/type.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::common {

namespace impl {

template <typename ValueBuilder>
ValueBuilder GetAtPath(ValueBuilder& parent, std::vector<std::string>&& path,
                       std::size_t path_size) {
  UINVARIANT(path_size != 0,
             "attempt to get a ValueBuilder element on an empty path");
  if (path_size == 1) {
    return parent[std::move(path[0])];
  }
  std::optional<ValueBuilder> current_element;

  current_element.emplace(TransferTag(), parent[std::move(path[0])]);
  for (std::size_t i = 1; i < path_size - 1; i++) {
    current_element.emplace(TransferTag(),
                            (*current_element)[std::move(path[i])]);
  }
  return (*current_element)[std::move(path[path_size - 1])];
}

}  // namespace impl

/// @brief Get the `Value` at `path` in `parent`.
/// @note For empty `path` this function returns `parent`.
/// @throws TypeMismatchException if there is a non-object node in the middle of
/// `path`
template <typename Value>
std::enable_if_t<common::kIsFormatValue<Value>, Value> GetAtPath(
    Value parent, const std::vector<std::string>& path) {
  auto current_value = std::move(parent);
  for (const auto& current_key : path) {
    current_value = current_value[current_key];
  }
  return current_value;
}

/// @brief Get the `ValueBuilder` at `path` in `parent`.
/// @note `path` must not be empty.
/// @throws TypeMismatchException if there is a non-object node in the middle of
/// `path`
template <typename ValueBuilder>
std::enable_if_t<!common::kIsFormatValue<ValueBuilder>, ValueBuilder> GetAtPath(
    ValueBuilder& parent, std::vector<std::string>&& path) {
  return impl::GetAtPath(parent, std::move(path), path.size());
}

/// @brief Set the `new_value` along the `path` in the `parent`.
/// @note If `path` is empty it sets `new_value` to `parent`.
/// @throws TypeMismatchException if there is a non-object node in the middle of
/// `path`
template <typename Value>
void SetAtPath(typename Value::Builder& parent, std::vector<std::string>&& path,
               Value new_value) {
  if (path.empty()) {
    parent = std::move(new_value);
  } else {
    GetAtPath(parent, std::move(path)) = std::move(new_value);
  }
}

/// @brief Remove the element along the `path` in the `parent`.
/// @note If `path` is empty or there is no node at `path`, this function does
/// nothing.
/// @throws TypeMismatchException if there is a non-object node in the middle of
/// `path`
template <typename ValueBuilder>
void RemoveAtPath(ValueBuilder& parent, std::vector<std::string>&& path) {
  if (path.empty()) {
    parent = ValueBuilder();
  } else if (path.size() == 1) {
    parent.Remove(path[0]);
  } else {
    auto& key = path[path.size() - 1];
    impl::GetAtPath(parent, std::move(path), path.size() - 1).Remove(key);
  }
}

/// @brief Split `path` to `vector<std::string>` by dots.
/// @note If `path` has a double dot or a dot at the beginning or end, the
/// result will contain an empty string.
/// @note Returns an empty `vector` if `path` is empty
std::vector<std::string> SplitPathString(std::string_view path);

}  // namespace formats::common

USERVER_NAMESPACE_END
