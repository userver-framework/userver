#pragma once

#include <functional>
#include <iosfwd>

#include <utils/strong_typedef.hpp>

namespace utils {

// TODO TAXICOMMON-2209: clean up in uservices and remove

/// Helper class for data that MUST NOT be logged or outputted in some other
/// way.
template <class T>
using NonLoggable [[deprecated("Use StrongNonLoggable")]] =
    StrongNonLoggable<class DefaultNonLoggableTag, T>;

}  // namespace utils
