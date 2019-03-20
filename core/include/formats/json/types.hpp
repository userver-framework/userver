#pragma once

#include <memory>

#include <type_traits>
#include <utils/void_t.hpp>

#include <formats/common/path.hpp>
#include <formats/common/type.hpp>

// Forward declarations
namespace Json {
class Value;
class ValueConstIterator;
class ValueIterator;
}  // namespace Json

namespace formats {
namespace json {

using formats::common::Path;
using formats::common::PathToString;
using formats::common::Type;

using NativeValuePtr = std::shared_ptr<Json::Value>;

class Value;

namespace impl {
template <typename T, typename = ::utils::void_t<>>
struct HasParseJsonFor : std::false_type {};

template <typename T>
struct HasParseJsonFor<
    T, ::utils::void_t<decltype(
           ParseJson(std::declval<const Value&>(), std::declval<const T*>()))>>
    : std::true_type {};
}  // namespace impl

template <typename T>
constexpr inline bool kHasParseJsonFor = impl::HasParseJsonFor<T>::value;

}  // namespace json
}  // namespace formats
