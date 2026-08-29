#pragma once

/// @file userver/utils/resource_scopes_fwd.hpp
/// @brief Forward declarations of @ref utils::ResourceScopeStorage and
/// @ref utils::WithResourceScopes

USERVER_NAMESPACE_BEGIN

namespace utils {

class ResourceScopeStorage;

template <typename Wrapped>
class WithResourceScopes;

}  // namespace utils

USERVER_NAMESPACE_END
