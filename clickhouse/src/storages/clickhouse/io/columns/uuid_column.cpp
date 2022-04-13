#include <userver/storages/clickhouse/io/columns/uuid_column.hpp>

#include <storages/clickhouse/io/columns/impl/column_includes.hpp>

#include <clickhouse/columns/uuid.h>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::columns {

namespace {
using NativeType = clickhouse::impl::clickhouse_cpp::ColumnUUID;
}

static_assert(
    std::is_trivially_copyable_v<decltype(UuidColumn::cpp_type::data)>,
    "Can not use memcpy.");
static_assert(sizeof(UuidColumn::cpp_type::data) == sizeof(uint64_t) * 2,
              "Unexpected uuid size.");

UuidColumn::UuidColumn(ColumnRef column)
    : ClickhouseColumn{impl::GetTypedColumn<UuidColumn, NativeType>(column)} {}

template <>
UuidColumn::cpp_type ColumnIterator<UuidColumn>::DataHolder::Get() const {
  const auto ch_uuid = impl::NativeGetAt<NativeType>(column_, ind_);

  UuidColumn::cpp_type result;
  const auto format = [&result](size_t offset, uint64_t data) {
    std::memcpy(&result.data[offset], &data, sizeof(data));
  };

  format(0, ch_uuid.first);
  format(8, ch_uuid.second);
  return result;
}

ColumnRef UuidColumn::Serialize(const container_type& from) {
  std::vector<uint64_t> data;
  data.reserve(from.size() * 2);
  const auto convert = [](const cpp_type& uuid, size_t offset) {
    uint64_t result{};
    std::memcpy(&result, &uuid.data[offset], sizeof(result));
    return result;
  };
  for (const auto& uuid : from) {
    data.push_back(convert(uuid, 0));
    data.push_back(convert(uuid, 8));
  }

  return std::make_shared<NativeType>(
      std::make_shared<clickhouse::impl::clickhouse_cpp::ColumnUInt64>(
          std::move(data)));
}

}  // namespace storages::clickhouse::io::columns

USERVER_NAMESPACE_END
