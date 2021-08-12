#pragma once

/// @file userver/utils/invariant_error.hpp
/// @brief @copybrief utils::InvariantError

#include <userver/utils/traceful_exception.hpp>

namespace utils {

/// @brief Exception that is thrown on UINVARIANT violation
class InvariantError : public TracefulException {
  using TracefulException::TracefulException;
};

}  // namespace utils
