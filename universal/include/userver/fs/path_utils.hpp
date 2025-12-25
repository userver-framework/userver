#pragma once

/// @file userver/fs/path_utils.hpp
/// @brief file path operations and utilities

#include <string>

USERVER_NAMESPACE_BEGIN

/// @brief filesystem support
namespace fs {

/// @brief Returns relative path from full path
/// @param path full path, must start with `dir`
/// @param dir directory path to get relative path
/// @note it does not access filesystem, the relative path is calculated
/// lexically.
std::string GetLexicallyRelative(std::string_view path, std::string_view dir);

}  // namespace fs

USERVER_NAMESPACE_END
