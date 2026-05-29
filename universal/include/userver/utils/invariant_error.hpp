#pragma once

/// @file userver/utils/invariant_error.hpp
/// @brief @copybrief utils::InvariantError

#include <userver/utils/traceful_exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// @ingroup userver_universal
///
/// @brief Exception that is thrown on UINVARIANT violation
class InvariantError : public TracefulException {
public:
    using TracefulException::TracefulException;

    InvariantError(InvariantError&&) = default;

    ~InvariantError() override;
};

}  // namespace utils

USERVER_NAMESPACE_END
