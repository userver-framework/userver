#pragma once

/// @file userver/dump/meta.hpp
/// @brief Provides dump::kIsDumpable and includes userver/dump/fwd.hpp

#include <concepts>
#include <type_traits>

#include <userver/dump/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace dump {

/// Check if `writer.Write(T)` is available
template <typename T>
// NOLINTNEXTLINE(readability-identifier-naming)
concept IsWritable = requires(Writer& writer, const T& value) {
    {
        Write(writer, value)
    } -> std::same_as<void>;
};

/// Check if `reader.Read<T>()` is available
template <typename T>
// NOLINTNEXTLINE(readability-identifier-naming)
concept IsReadable = requires(Reader& reader) {
    {
        Read(reader, To<T>{})
    } -> std::same_as<std::remove_const_t<T>>;
};

/// Check if `T` is both writable and readable
template <typename T>
// NOLINTNEXTLINE(readability-identifier-naming)
concept IsDumpable = IsWritable<T> && IsReadable<T>;

template <typename T>
constexpr bool CheckDumpable() {
    static_assert(
        IsDumpable<T>,
        "Type is not dumpable. Probably you forgot to include "
        "<userver/dump/common.hpp>, <userver/dump/common_containers.hpp> or "
        "other headers with Read and Write declarations"
    );

    return true;
}

/// @deprecated Use @ref dump::IsWritable instead.
template <typename T>
// NOLINTNEXTLINE(readability-identifier-naming)
concept kIsWritable = IsWritable<T>;

/// @deprecated Use @ref dump::IsReadable instead.
template <typename T>
// NOLINTNEXTLINE(readability-identifier-naming)
concept kIsReadable = IsReadable<T>;

/// @deprecated Use @ref dump::IsDumpable instead.
template <typename T>
// NOLINTNEXTLINE(readability-identifier-naming)
concept kIsDumpable = IsDumpable<T>;

}  // namespace dump

USERVER_NAMESPACE_END
