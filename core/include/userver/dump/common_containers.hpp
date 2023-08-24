#pragma once

/// @file userver/dump/common_containers.hpp
/// @brief Dump support for C++ Standard Library and Boost containers,
/// `std::optional`, utils::StrongTypedef, `std::{unique,shared}_ptr`
///
/// @note There are no traits in `CachingComponentBase`. If `T`
/// is writable/readable, we have to generate the code for dumps
/// regardless of `dump: enabled`. So it's important that all Read-Write
/// operations for containers are SFINAE-correct.
///
/// @ingroup userver_dump_read_write

#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include <userver/utils/lazy_prvalue.hpp>
#include <userver/utils/meta.hpp>
#include <userver/utils/strong_typedef.hpp>

#include <userver/dump/common.hpp>
#include <userver/dump/meta.hpp>
#include <userver/dump/meta_containers.hpp>
#include <userver/dump/operations.hpp>

/// @cond
namespace boost {

namespace bimaps {

template <typename KeyTypeA, typename KeyTypeB, typename AP1, typename AP2,
          typename AP3>
class bimap;

}  // namespace bimaps

namespace multi_index {

template <typename Value, typename IndexSpecifierList, typename Allocator>
class multi_index_container;

}  // namespace multi_index

using bimaps::bimap;
using multi_index::multi_index_container;

}  // namespace boost
/// @endcond

USERVER_NAMESPACE_BEGIN

namespace dump {

namespace impl {

template <typename T>
auto ReadLazyPrvalue(Reader& reader) {
  return utils::LazyPrvalue([&reader] { return reader.Read<T>(); });
}

[[noreturn]] void ThrowInvalidVariantIndex(const std::type_info& type,
                                           std::size_t index);

template <std::size_t... Indices, typename Func>
void ForEachIndex(std::index_sequence<Indices...>, Func func) {
  (func(std::integral_constant<std::size_t, Indices>{}), ...);
}

template <typename VariantType>
VariantType ReadVariant(Reader& reader, std::size_t index) {
  std::optional<VariantType> result;
  using Indices = std::make_index_sequence<std::variant_size_v<VariantType>>;

  ForEachIndex(Indices{}, [&](auto index_constant) {
    static constexpr auto kIndex = decltype(index_constant)::value;
    using Alternative = std::variant_alternative_t<kIndex, VariantType>;
    if (index == kIndex) {
      // Not using ReadLazyPrvalue because of stdlib issues on some compilers.
      result.emplace(std::in_place_index<kIndex>, reader.Read<Alternative>());
    }
  });

  return std::move(*result);
}

}  // namespace impl

/// @brief Container serialization support
template <typename T>
std::enable_if_t<kIsContainer<T> && kIsWritable<meta::RangeValueType<T>>> Write(
    Writer& writer, const T& value) {
  writer.Write(std::size(value));
  for (const auto& item : value) {
    // explicit cast for vector<bool> shenanigans
    writer.Write(static_cast<const meta::RangeValueType<T>&>(item));
  }
}

/// @brief Container deserialization support
template <typename T>
std::enable_if_t<kIsContainer<T> && kIsReadable<meta::RangeValueType<T>>, T>
Read(Reader& reader, To<T>) {
  const auto size = reader.Read<std::size_t>();
  T result{};
  if constexpr (meta::kIsReservable<T>) {
    result.reserve(size);
  }
  for (std::size_t i = 0; i < size; ++i) {
    dump::Insert(result, reader.Read<meta::RangeValueType<T>>());
  }
  return result;
}

/// @brief Pair serialization support (for maps)
template <typename T, typename U>
std::enable_if_t<kIsWritable<T> && kIsWritable<U>, void> Write(
    Writer& writer, const std::pair<T, U>& value) {
  writer.Write(value.first);
  writer.Write(value.second);
}

/// @brief Pair deserialization support (for maps)
template <typename T, typename U>
std::enable_if_t<kIsReadable<T> && kIsReadable<U>, std::pair<T, U>> Read(
    Reader& reader, To<std::pair<T, U>>) {
  return {reader.Read<T>(), reader.Read<U>()};
}

/// @brief `std::optional` serialization support
template <typename T>
std::enable_if_t<kIsWritable<T>> Write(Writer& writer,
                                       const std::optional<T>& value) {
  writer.Write(value.has_value());
  if (value) writer.Write(*value);
}

/// @brief `std::optional` deserialization support
template <typename T>
std::enable_if_t<kIsReadable<T>, std::optional<T>> Read(Reader& reader,
                                                        To<std::optional<T>>) {
  if (!reader.Read<bool>()) return std::nullopt;
  return impl::ReadLazyPrvalue<T>(reader);
}

/// @brief `std::variant` serialization support
template <typename... Args>
std::enable_if_t<(true && ... && kIsWritable<Args>)> Write(
    Writer& writer, const std::variant<Args...>& value) {
  writer.Write(value.index());
  std::visit([&writer](const auto& inner) { writer.Write(inner); }, value);
}

/// @brief `std::variant` deserialization support
template <typename... Args>
std::enable_if_t<(true && ... &&
                  (std::is_move_constructible_v<Args> && kIsReadable<Args>)),
                 std::variant<Args...>>
Read(Reader& reader, To<std::variant<Args...>>) {
  const auto index = reader.Read<std::size_t>();
  if (index >= sizeof...(Args)) {
    impl::ThrowInvalidVariantIndex(typeid(std::variant<Args...>), index);
  }
  return impl::ReadVariant<std::variant<Args...>>(reader, index);
}

/// Allows reading `const T`, which is usually encountered as a member of some
/// container
template <typename T>
std::enable_if_t<kIsReadable<T>, T> Read(Reader& reader, To<const T>) {
  return Read(reader, To<T>{});
}

/// @brief utils::StrongTypedef serialization support
template <typename Tag, typename T, utils::StrongTypedefOps Ops>
std::enable_if_t<kIsWritable<T>> Write(
    Writer& writer, const utils::StrongTypedef<Tag, T, Ops>& object) {
  writer.Write(object.GetUnderlying());
}

/// @brief utils::StrongTypedef deserialization support
template <typename Tag, typename T, utils::StrongTypedefOps Ops>
std::enable_if_t<kIsReadable<T>, utils::StrongTypedef<Tag, T, Ops>> Read(
    Reader& reader, To<utils::StrongTypedef<Tag, T, Ops>>) {
  return utils::StrongTypedef<Tag, T, Ops>{reader.Read<T>()};
}

/// @brief `std::unique_ptr` serialization support
template <typename T>
std::enable_if_t<kIsWritable<T>> Write(Writer& writer,
                                       const std::unique_ptr<T>& ptr) {
  writer.Write(static_cast<bool>(ptr));
  if (ptr) writer.Write(*ptr);
}

/// @brief `std::unique_ptr` deserialization support
template <typename T>
std::enable_if_t<kIsReadable<T>, std::unique_ptr<T>> Read(
    Reader& reader, To<std::unique_ptr<T>>) {
  if (!reader.Read<bool>()) return {};
  return std::make_unique<T>(impl::ReadLazyPrvalue<T>(reader));
}

/// @brief `std::shared_ptr` serialization support
/// @warning If two or more `shared_ptr` within a single dumped entity point to
/// the same object, they will point to its distinct copies after loading a dump
template <typename T>
std::enable_if_t<kIsWritable<T>> Write(Writer& writer,
                                       const std::shared_ptr<T>& ptr) {
  writer.Write(static_cast<bool>(ptr));
  if (ptr) writer.Write(*ptr);
}

/// @brief `std::shared_ptr` deserialization support
/// @warning If two or more `shared_ptr` within a single dumped entity point to
/// the same object, they will point to its distinct copies after loading a dump
template <typename T>
std::enable_if_t<kIsReadable<T>, std::shared_ptr<T>> Read(
    Reader& reader, To<std::shared_ptr<T>>) {
  if (!reader.Read<bool>()) return {};
  return std::make_shared<T>(impl::ReadLazyPrvalue<T>(reader));
}

/// @brief `boost::bimap` serialization support
template <typename L, typename R, typename AP1, typename AP2, typename AP3>
std::enable_if_t<kIsReadable<L> && kIsReadable<R>> Write(
    Writer& writer, const boost::bimap<L, R, AP1, AP2, AP3>& map) {
  writer.Write(map.left.size());
  for (const auto& [left, right] : map) {
    writer.Write(left);
    writer.Write(right);
  }
}

/// @brief `boost::bimap` deserialization support
template <typename L, typename R, typename AP1, typename AP2, typename AP3>
std::enable_if_t<kIsWritable<L> && kIsWritable<R>,
                 boost::bimap<L, R, AP1, AP2, AP3>>
Read(Reader& reader, To<boost::bimap<L, R, AP1, AP2, AP3>>) {
  const auto size = reader.Read<std::size_t>();

  boost::bimap<L, R, AP1, AP2, AP3> map;
  // bimap doesn't have reserve :(

  for (std::size_t i = 0; i < size; ++i) {
    // `Read`s are guaranteed to occur left-to-right in brace-init
    map.left.insert({reader.Read<L>(), reader.Read<R>()});
  }

  return map;
}

/// @brief `boost::multi_index_container` serialization support
template <typename T, typename Index, typename Alloc>
std::enable_if_t<kIsWritable<T>> Write(
    Writer& writer,
    const boost::multi_index_container<T, Index, Alloc>& container) {
  writer.Write(container.template get<0>().size());
  for (auto& item : container.template get<0>()) {
    writer.Write(item);
  }
}

/// @brief `boost::multi_index_container` deserialization support
template <typename T, typename Index, typename Alloc>
std::enable_if_t<kIsReadable<T>, boost::multi_index_container<T, Index, Alloc>>
Read(Reader& reader, To<boost::multi_index_container<T, Index, Alloc>>) {
  const auto size = reader.Read<std::size_t>();
  boost::multi_index_container<T, Index, Alloc> container;

  // boost::multi_index_container has reserve() with some, but not all, configs
  if constexpr (meta::kIsReservable<
                    boost::multi_index_container<T, Index, Alloc>>) {
    container.reserve(size);
  }

  for (std::size_t i = 0; i < size; ++i) {
    container.insert(reader.Read<T>());
  }

  return container;
}

}  // namespace dump

USERVER_NAMESPACE_END
