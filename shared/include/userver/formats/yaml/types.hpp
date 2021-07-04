#pragma once

/// @file userver/formats/yaml/types.hpp
/// @brief Forward declarations of the internal YAML types.

#include <type_traits>
#include <userver/utils/void_t.hpp>

#include <userver/formats/common/path.hpp>
#include <userver/formats/common/type.hpp>

// Forward declarations
namespace YAML {
class Node;
namespace detail {
struct iterator_value;
template <typename V>
class iterator_base;
}  // namespace detail

using iterator = detail::iterator_base<detail::iterator_value>;
using const_iterator = detail::iterator_base<const detail::iterator_value>;
}  // namespace YAML

namespace formats {
namespace yaml {

using formats::common::Path;
using formats::common::Type;

class Value;

}  // namespace yaml
}  // namespace formats
