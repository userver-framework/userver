#include <userver/storages/clickhouse/io/columns/uuid_column.hpp>

#include <storages/clickhouse/io/columns/impl/column_includes.hpp>

#include <clickhouse/columns/uuid.h>

#include <boost/endian/conversion.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::columns {

namespace {
using NativeType = clickhouse::impl::clickhouse_cpp::ColumnUUID;

enum class Endianness { kLE, kBE };

std::uint64_t ApplyEndianness(std::uint64_t value, Endianness endianness) {
  switch (endianness) {
    case Endianness::kLE:
      // the driver only works with LE host systems, so this is a no-op
      return value;
    case Endianness::kBE: {
      return boost::endian::endian_reverse(value);
    }
  }

  UINVARIANT(false, "Unreachable");
}

std::pair<std::uint64_t, std::uint64_t> SerializeToNative(
    const boost::uuids::uuid& user_uuid, Endianness endianness) {
  std::uint64_t first{};
  std::uint64_t second{};

  std::memcpy(&first, &user_uuid.data[0], 8);
  std::memcpy(&second, &user_uuid.data[8], 8);

  return {ApplyEndianness(first, endianness),
          ApplyEndianness(second, endianness)};
}

boost::uuids::uuid ParseFromNative(const ::clickhouse::UUID& native_uuid,
                                   Endianness endianness) {
  boost::uuids::uuid result{};
  const auto format = [&result, endianness](std::size_t offset,
                                            std::uint64_t data) {
    data = ApplyEndianness(data, endianness);
    std::memcpy(&result.data[offset], &data, sizeof(data));
  };

  format(0, native_uuid.first);
  format(8, native_uuid.second);

  return result;
}

ColumnRef SerializeToNative(const std::vector<boost::uuids::uuid>& what,
                            Endianness endianness) {
  std::vector<std::uint64_t> data;
  data.reserve(what.size() * 2);

  for (const auto& user_uuid : what) {
    const auto native_representation = SerializeToNative(user_uuid, endianness);
    data.push_back(native_representation.first);
    data.push_back(native_representation.second);
  }

  return std::make_shared<NativeType>(
      std::make_shared<clickhouse::impl::clickhouse_cpp::ColumnUInt64>(
          std::move(data)));
}

}  // namespace

static_assert(std::is_trivially_copyable_v<
                  decltype(MismatchedEndiannessUuidColumn::cpp_type::data)>,
              "Can not use memcpy.");
static_assert(sizeof(MismatchedEndiannessUuidColumn::cpp_type::data) ==
                  sizeof(std::uint64_t) * 2,
              "Unexpected uuid size.");

MismatchedEndiannessUuidColumn::MismatchedEndiannessUuidColumn(ColumnRef column)
    : ClickhouseColumn{
          impl::GetTypedColumn<MismatchedEndiannessUuidColumn, NativeType>(
              column)} {}

template <>
MismatchedEndiannessUuidColumn::cpp_type
ColumnIterator<MismatchedEndiannessUuidColumn>::DataHolder::Get() const {
  const auto ch_uuid = impl::NativeGetAt<NativeType>(column_, ind_);

  return ParseFromNative(ch_uuid, Endianness::kLE);
}

ColumnRef MismatchedEndiannessUuidColumn::Serialize(
    const container_type& from) {
  return SerializeToNative(from, Endianness::kLE);
}

UuidRfc4122Column::UuidRfc4122Column(ColumnRef column)
    : ClickhouseColumn{
          impl::GetTypedColumn<UuidRfc4122Column, NativeType>(column)} {}

template <>
UuidRfc4122Column::cpp_type ColumnIterator<UuidRfc4122Column>::DataHolder::Get()
    const {
  const auto ch_uuid = impl::NativeGetAt<NativeType>(column_, ind_);

  return ParseFromNative(ch_uuid, Endianness::kBE);
}

ColumnRef UuidRfc4122Column::Serialize(const container_type& from) {
  return SerializeToNative(from, Endianness::kBE);
}

}  // namespace storages::clickhouse::io::columns

USERVER_NAMESPACE_END
