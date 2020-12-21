#pragma once

#include <type_traits>

#include <utils/meta.hpp>

#include <cache/dump/operations.hpp>

namespace cache::dump {

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
inline constexpr bool kIsDumpable = kIsWritable<T>&& kIsReadable<T>;

}  // namespace cache::dump
