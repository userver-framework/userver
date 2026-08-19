#pragma once

#include <cstddef>
#include <optional>

#include <sqlext.h>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

struct ResultChunk final {
    std::size_t size{};
    bool has_more{};
    std::optional<SQLLEN> known_remaining;
};

/// Applies the portable SQLGetData variable-length indicator rules. The
/// indicator is the bytes remaining before the current call. Character
/// capacity must exclude the terminating NUL; binary capacity must include the
/// whole target buffer.
ResultChunk AccountResultChunk(
    SQLRETURN result,
    SQLLEN indicator,
    bool has_truncation_warning,
    std::size_t capacity,
    std::optional<SQLLEN> previous_remaining
);

void ValidateFixedResultSize(SQLLEN indicator, std::size_t expected_size);

void ValidateNumericMagnitude(std::size_t digit_count, std::size_t precision, std::size_t scale);

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
