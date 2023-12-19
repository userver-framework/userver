#pragma once

/// @file userver/storages/clickhouse/io/columns/uuid_column.hpp
/// @brief UUID column support
/// @ingroup userver_clickhouse_types

#include <boost/uuid/uuid.hpp>

#include <userver/storages/clickhouse/io/columns/column_includes.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::columns {

/// @brief Legacy broken ClickHouse UUID column representation.
///
/// @warning This implementation is broken in a way that uuids endianness is
/// mismatched:
/// uuid "30eec7b3-0b5c-451e-8976-98f62b4c4448" will be stored as
/// uuid "1e455c0b-b3c7-ee30-4844-4c2bf6987689" in ClickHouse (notice that each
/// of 8 hex bytes group is bytewise reversed).
///
/// Requires **both** writes and reads to be performed via userver to work.
class MismatchedEndiannessUuidColumn final
    : public ClickhouseColumn<MismatchedEndiannessUuidColumn> {
 public:
  using cpp_type = boost::uuids::uuid;
  using container_type = std::vector<cpp_type>;

  MismatchedEndiannessUuidColumn(ColumnRef column);

  static ColumnRef Serialize(const container_type& from);
};

/// @brief Represents ClickHouse UUID column.
class UuidRfc4122Column final : public ClickhouseColumn<UuidRfc4122Column> {
 public:
  using cpp_type = boost::uuids::uuid;
  using container_type = std::vector<cpp_type>;

  UuidRfc4122Column(ColumnRef column);

  static ColumnRef Serialize(const container_type& from);
};

// Dummy implementation to force a compile-time error on usage attempt.
class UuidColumn final : public ClickhouseColumn<UuidColumn> {
 public:
  using cpp_type = boost::uuids::uuid;
  using container_type = std::vector<cpp_type>;

  template <typename T>
  UuidColumn(T) {
    ReportMisuse<T>();
  }

  template <typename T>
  static ColumnRef Serialize(const T&) {
    ReportMisuse<T>();
    return {};
  }

 private:
  template <typename T>
  static void ReportMisuse() {
    static_assert(!sizeof(T),
                  "UuidColumn is deprecated: for old code rename it to "
                  "MismatchedEndiannessUuidColumn, for new code we encourage "
                  "you to use UuidRfc4122Column instead. See the "
                  "MismatchedEndiannessUuidColumn docs for explanation.");
  }
};

}  // namespace storages::clickhouse::io::columns

USERVER_NAMESPACE_END
