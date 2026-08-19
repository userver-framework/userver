#include <storages/odbc/detail/result_chunk.hpp>

#include <fmt/format.h>

#include <userver/storages/odbc/exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

ResultChunk AccountResultChunk(
    SQLRETURN result,
    SQLLEN indicator,
    bool has_truncation_warning,
    std::size_t capacity,
    std::optional<SQLLEN> previous_remaining
) {
    if (indicator < 0 && indicator != SQL_NO_TOTAL) {
        throw ResultSetError("ODBC driver returned an invalid result length indicator");
    }
    if (result == SQL_SUCCESS_WITH_INFO && has_truncation_warning) {
        if (indicator == SQL_NO_TOTAL) {
            return {capacity, true, std::nullopt};
        }
        if (indicator <= static_cast<SQLLEN>(capacity) || (previous_remaining && indicator >= *previous_remaining)) {
            throw ResultSetError("ODBC driver returned inconsistent result chunk length progress");
        }
        return {capacity, true, indicator};
    }
    if (indicator == SQL_NO_TOTAL) {
        throw ResultSetError("ODBC driver did not report the length of the final result chunk");
    }
    if (indicator > static_cast<SQLLEN>(capacity)) {
        throw ResultSetError("ODBC driver returned a result larger than the final chunk");
    }
    return {static_cast<std::size_t>(indicator), false, previous_remaining};
}

void ValidateFixedResultSize(SQLLEN indicator, std::size_t expected_size) {
    if (indicator < 0 || static_cast<std::size_t>(indicator) > expected_size) {
        throw ResultSetError(fmt::format(
            "ODBC driver returned fixed-size result length indicator {}, expected a value in [0, {}]",
            indicator,
            expected_size
        ));
    }
}

void ValidateNumericMagnitude(std::size_t digit_count, std::size_t precision, std::size_t scale) {
    if (precision == 0 || precision > 38 || scale > precision || digit_count == 0) {
        throw ResultSetError("ODBC driver returned invalid Decimal precision, scale, or magnitude metadata");
    }
    const auto integer_digits = digit_count > scale ? digit_count - scale : 0;
    if (integer_digits > precision - scale || digit_count > precision) {
        throw ResultSetError("ODBC driver returned a Decimal magnitude outside the declared precision and scale");
    }
}

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
