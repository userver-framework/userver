#include <userver/storages/clickhouse.hpp>

namespace {
namespace columns = userver::storages::clickhouse::io::columns;

template <typename... Args>
struct IteratorDefaultConstructorInstantiator final {
  ~IteratorDefaultConstructorInstantiator() {
    (..., typename Args::iterator{});
    (..., typename columns::NullableColumn<Args>::iterator{});
  }
};

[[maybe_unused]] const IteratorDefaultConstructorInstantiator<
    columns::UInt8Column, columns::UInt32Column, columns::UInt64Column,
    columns::DateTime64ColumnMilli, columns::DateTime64ColumnMicro,
    columns::DateTime64ColumnNano, columns::StringColumn>
    validator{};

}  // namespace
