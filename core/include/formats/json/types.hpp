#pragma once

#include <memory>
#include <type_traits>

#include <formats/common/path.hpp>
#include <formats/common/type.hpp>
#include <utils/void_t.hpp>

// copypasted from rapidjson/fwd.h
namespace rapidjson {
template <typename CharType>
struct UTF8;
class CrtAllocator;
template <typename Encoding, typename Allocator>
class GenericValue;
template <typename Encoding, typename Allocator, typename StackAllocator>
class GenericDocument;
}  // namespace rapidjson

namespace formats {
namespace json {

using formats::common::Path;
using formats::common::Type;

class Value;

namespace impl {
// rapidjson integration
namespace J = ::rapidjson;
using UTF8 = J::UTF8<char>;
using DefaultAllocator = J::CrtAllocator;
using Value = J::GenericValue<UTF8, DefaultAllocator>;
using Document = J::GenericDocument<UTF8, DefaultAllocator, J::CrtAllocator>;

template <typename T, typename = ::utils::void_t<>>
struct HasParseJsonFor : std::false_type {};

template <typename T>
struct HasParseJsonFor<T, ::utils::void_t<decltype(ParseJson(
                              std::declval<const ::formats::json::Value&>(),
                              std::declval<const T*>()))>> : std::true_type {};
}  // namespace impl

using NativeValuePtr = std::shared_ptr<impl::Value>;

template <typename T>
constexpr inline bool kHasParseJsonFor = impl::HasParseJsonFor<T>::value;

}  // namespace json
}  // namespace formats
