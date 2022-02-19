#pragma once

/// @file userver/shared/include/userver/formats/json/utils.hpp
/// @brief Utils for formats::json::Value and formats::json::ValueBuilder

#include <string>
#include <string_view>
#include <vector>

#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::common {

/// @brief Get the formats::json::Value at `path` in `parent`.
/// @note For empty `path` this function returns `parent`.
/// @throws TypeMismatchException if there is a non-object node in the middle of
/// `path`
formats::json::Value GetAtPath(formats::json::Value parent,
                               const std::vector<std::string>& path);

/// @brief Get the formats::json::ValueBuilder at `path` in `parent`.
/// @note `path` must not be empty.
/// @throws TypeMismatchException if there is a non-object node in the middle of
/// `path`
formats::json::ValueBuilder GetAtPath(formats::json::ValueBuilder& parent,
                                      std::vector<std::string>&& path);

/// @brief Set the `new_value` along the `path` in the `parent`.
/// @note If `path` is empty it sets `new_value` to `parent`.
/// @throws TypeMismatchException if there is a non-object node in the middle of
/// `path`
void SetAtPath(formats::json::ValueBuilder& parent,
               std::vector<std::string>&& path, formats::json::Value new_value);

/// @brief Remove the element along the `path` in the `parent`.
/// @note If `path` is empty or there is no node at `path`, this function does
/// nothing.
/// @throws TypeMismatchException if there is a non-object node in the middle of
/// `path`
void RemoveAtPath(formats::json::ValueBuilder& parent,
                  std::vector<std::string>&& path);

/// @brief Split `path` to `vector<std::string>` by dots.
/// @note If `path` has a double dot or a dot at the beginning or end, the
/// result will contain an empty string.
/// @note Returns an empty `vector` if `path` is empty
std::vector<std::string> SplitPathString(std::string_view path);

}  // namespace formats::common

USERVER_NAMESPACE_END
