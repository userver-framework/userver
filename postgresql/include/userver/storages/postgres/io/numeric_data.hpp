#pragma once

#include <userver/storages/postgres/io/buffer_io.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::io::detail {

/// All digits packed in a single integer value, if the size of int64 is enough
struct IntegralRepresentation {
  std::int64_t value;
  int fractional_digit_count;
};

/// A helper function to read PostgreSQL binary buffer containing a
/// numeric/decimal to string representation
std::string NumericBufferToString(const FieldBuffer& buffer);
/// A helper function to write string representation of a numeric/decimal to
/// PostgreSQL binary buffer representation
std::string StringToNumericBuffer(const std::string& str_rep);

/// A helper function to read PostgreSQL numeric/decimal binary buffer and pack
/// the value in a single int64 value.
/// @throw NumericOverflow if the int64 is not enough to represent value in the
///                        buffer
/// @throw ValueIsNaN      if the value in the buffer is NaN
IntegralRepresentation NumericBufferToInt64(const FieldBuffer& buffer);
/// A helper function to write PostgreSQL numeric/decimal binary buffer using
/// a value packed into a single int64 value.
/// @throw InvalidRepresentation if the dec_digits field of binary
///                              representation is unreasonable
std::string Int64ToNumericBuffer(const IntegralRepresentation& rep);

}  // namespace storages::postgres::io::detail

USERVER_NAMESPACE_END
