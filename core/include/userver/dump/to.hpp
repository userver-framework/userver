#pragma once

/// @file userver/dump/to.hpp
/// @brief @copybrief dump::To

#include <userver/dump/to.hpp>

USERVER_NAMESPACE_BEGIN

namespace dump {

/// @brief A marker type used in ADL-found `Read`
template <typename T>
struct To {};

}  // namespace dump

USERVER_NAMESPACE_END
