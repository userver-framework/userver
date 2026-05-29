#pragma once

/// @file userver/utils/strong_typedef_fwd.hpp
/// @brief Forward declarations and options for StrongTypedef.
/// @ingroup userver_universal

#include <type_traits>

USERVER_NAMESPACE_BEGIN

namespace utils {

enum class StrongTypedefOps {
    kNoCompare = 0,  /// Forbid all comparisons for StrongTypedef

    kCompareStrong = 1,           /// Allow comparing two StrongTypedef<Tag, T>
    kCompareTransparentOnly = 2,  /// Allow comparing StrongTypedef<Tag, T> and T
    kCompareTransparent = 3,      /// Allow both of the above

    kNonLoggable = 4,  /// Forbid logging and serializing for StrongTypedef
};

template <class Tag, class T, StrongTypedefOps Ops = StrongTypedefOps::kCompareStrong>
class StrongTypedef;

// Helpers
namespace impl::strong_typedef {

struct StrongTypedefTag {};

template <typename T>
concept IsStrongTypedef = std::is_base_of_v<StrongTypedefTag, T> && std::is_convertible_v<T&, StrongTypedefTag&>;

}  // namespace impl::strong_typedef

template <typename T>
using IsStrongTypedef = std::bool_constant<impl::strong_typedef::IsStrongTypedef<T>>;

}  // namespace utils

USERVER_NAMESPACE_END
