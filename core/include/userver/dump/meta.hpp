#pragma once

/// @file userver/dump/meta.hpp
/// @brief Provides dump::kIsDumpable and includes userver/dump/fwd.hpp

#include <type_traits>

#include <userver/dump/fwd.hpp>
#include <userver/utils/meta.hpp>

USERVER_NAMESPACE_BEGIN

namespace dump {

namespace impl {

template <typename T>
using WritableResult =
    decltype(Write(std::declval<Writer&>(), std::declval<const T&>()));

template <typename T>
using ReadableResult = decltype(Read(std::declval<Reader&>(), To<T>{}));

}  // namespace impl

/// Check if `writer.Write(T)` is available
template <typename T>
inline constexpr bool kIsWritable =
    std::is_same_v<meta::DetectedType<impl::WritableResult, T>, void>;

/// Check if `reader.Read<T>()` is available
template <typename T>
inline constexpr bool kIsReadable =
    std::is_same_v<meta::DetectedType<impl::ReadableResult, T>,
                   std::remove_const_t<T>>;

/// Check if `T` is both writable and readable
template <typename T>
inline constexpr bool kIsDumpable = kIsWritable<T> && kIsReadable<T>;

template <typename T>
constexpr bool CheckDumpable() {
  static_assert(
      kIsDumpable<T>,
      "Type is not dumpable. Probably you forgot to include "
      "<userver/dump/common.hpp>, <userver/dump/common_containers.hpp> or "
      "other headers with Read and Write declarations");

  return true;
}

}  // namespace dump

USERVER_NAMESPACE_END
