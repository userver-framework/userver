#pragma once

/// @file userver/utils/resources.hpp
/// @brief Functions to embed and retrieve blobs from binary

#include <string>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// Registeres a resource; used by `userver_embed_file` CMake function
void RegisterResource(std::string_view name, std::string_view value);

/// Returns a resource by name, registered by `userver_embed_file` CMake function (or RESOURCE() macro in some
/// other build systems)
std::string FindResource(std::string_view name);

}  // namespace utils

USERVER_NAMESPACE_END
