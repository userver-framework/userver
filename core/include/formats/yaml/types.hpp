#pragma once

#include <memory>
#include <vector>

#include <formats/common/path.hpp>

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
using formats::common::PathToString;

enum class Type {
  kNull,
  kArray,
  kObject,
  kMap,
  kMissing,
};

}  // namespace yaml
}  // namespace formats
