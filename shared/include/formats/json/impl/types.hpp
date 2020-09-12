#pragma once

#include <memory>
#include <type_traits>

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

namespace formats::json {

using formats::common::Type;

class Value;

namespace impl {
// rapidjson integration
using UTF8 = ::rapidjson::UTF8<char>;
using Value = ::rapidjson::GenericValue<UTF8, ::rapidjson::CrtAllocator>;
using Document = ::rapidjson::GenericDocument<UTF8, ::rapidjson::CrtAllocator,
                                              ::rapidjson::CrtAllocator>;

template <typename T, typename = ::utils::void_t<>>
struct HasParseJsonFor : std::false_type {};

template <typename T>
struct HasParseJsonFor<T, ::utils::void_t<decltype(ParseJson(
                              std::declval<const ::formats::json::Value&>(),
                              std::declval<const T*>()))>> : std::true_type {};

class VersionedValuePtr final {
 public:
  static constexpr size_t kInvalidVersion = -1;

  VersionedValuePtr() noexcept;

  template <typename... Args>
  static VersionedValuePtr Create(Args&&... args);

  VersionedValuePtr(const VersionedValuePtr&) = default;
  VersionedValuePtr(VersionedValuePtr&&) = default;
  VersionedValuePtr& operator=(const VersionedValuePtr&) = default;
  VersionedValuePtr& operator=(VersionedValuePtr&&) noexcept = default;

  ~VersionedValuePtr();

  explicit operator bool() const;
  bool IsUnique() const;

  const impl::Value* Get() const;
  impl::Value* Get();

  const impl::Value& operator*() const;
  impl::Value& operator*();
  const impl::Value* operator->() const;
  impl::Value* operator->();

  size_t Version() const;
  void BumpVersion();

 private:
  struct Data;

  explicit VersionedValuePtr(std::shared_ptr<Data>&&) noexcept;

  std::shared_ptr<Data> data_;
};

template <typename T>
constexpr inline bool kHasParseJsonFor = impl::HasParseJsonFor<T>::value;

}  // namespace impl
}  // namespace formats::json
