#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <userver/storages/scylla/row.hpp>
#include <userver/storages/scylla/value.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::impl::driver {
class DriverTableImpl;
class DriverSessionImpl;
}  // namespace storages::scylla::impl::driver

namespace storages::scylla::operations {

struct LwtResult {
    bool applied{false};
    Row previous;
};

class InsertOne {
public:
    InsertOne();
    ~InsertOne();

    InsertOne(const InsertOne&);
    InsertOne(InsertOne&&) noexcept;
    InsertOne& operator=(const InsertOne&);
    InsertOne& operator=(InsertOne&&) noexcept;

    void Bind(std::string column_name, Value value);

    void BindString(std::string column_name, std::string value);
    void BindInt32(std::string column_name, std::int32_t value);
    void BindInt64(std::string column_name, std::int64_t value);
    void BindBool(std::string column_name, bool value);
    void BindFloat(std::string column_name, float value);
    void BindDouble(std::string column_name, double value);
    void BindUuid(std::string column_name, Uuid value);
    void BindTimestamp(std::string column_name, Timestamp value);
    void BindBlob(std::string column_name, Blob value);
    void BindInet(std::string column_name, Inet value);
    void BindList(std::string column_name, List value);
    void BindMap(std::string column_name, Map value);
    void BindSet(std::string column_name, Set value);
    void BindNull(std::string column_name);

    void IfNotExists();
    void UsingTtl(std::int32_t seconds);
    void UsingTimestamp(std::int64_t microseconds);

private:
    friend class storages::scylla::impl::driver::DriverTableImpl;

    class Impl;
    static constexpr std::size_t kSize = 64;
    static constexpr std::size_t kAlignment = 8;
    utils::FastPimpl<Impl, kSize, kAlignment, false> impl_;
};

class SelectOne {
public:
    SelectOne();
    ~SelectOne();

    SelectOne(const SelectOne&);
    SelectOne(SelectOne&&) noexcept;
    SelectOne& operator=(const SelectOne&);
    SelectOne& operator=(SelectOne&&) noexcept;

    void AddColumn(std::string column_name);
    void AddAllColumns();

    void Where(std::string column_name, Value value);

    void WhereString(std::string column_name, std::string value);
    void WhereInt32(std::string column_name, std::int32_t value);
    void WhereInt64(std::string column_name, std::int64_t value);
    void WhereBool(std::string column_name, bool value);
    void WhereFloat(std::string column_name, float value);
    void WhereDouble(std::string column_name, double value);
    void WhereUuid(std::string column_name, Uuid value);
    void WhereTimestamp(std::string column_name, Timestamp value);
    void WhereBlob(std::string column_name, Blob value);
    void WhereInet(std::string column_name, Inet value);

private:
    friend class storages::scylla::impl::driver::DriverTableImpl;

    class Impl;
    static constexpr std::size_t kSize = 128;
    static constexpr std::size_t kAlignment = 8;
    utils::FastPimpl<Impl, kSize, kAlignment, false> impl_;
};

class DeleteOne {
public:
    DeleteOne();
    ~DeleteOne();

    DeleteOne(const DeleteOne&);
    DeleteOne(DeleteOne&&) noexcept;
    DeleteOne& operator=(const DeleteOne&);
    DeleteOne& operator=(DeleteOne&&) noexcept;

    void Where(std::string column_name, Value value);

    void WhereString(std::string column_name, std::string value);
    void WhereInt32(std::string column_name, std::int32_t value);
    void WhereInt64(std::string column_name, std::int64_t value);
    void WhereBool(std::string column_name, bool value);
    void WhereFloat(std::string column_name, float value);
    void WhereDouble(std::string column_name, double value);
    void WhereUuid(std::string column_name, Uuid value);
    void WhereTimestamp(std::string column_name, Timestamp value);

    void If(std::string column_name, Value value);
    void IfExists();
    void IfString(std::string column_name, std::string value);
    void IfInt32(std::string column_name, std::int32_t value);
    void IfInt64(std::string column_name, std::int64_t value);
    void IfBool(std::string column_name, bool value);
    void IfFloat(std::string column_name, float value);
    void IfDouble(std::string column_name, double value);

private:
    friend class storages::scylla::impl::driver::DriverTableImpl;

    class Impl;
    static constexpr std::size_t kSize = 128;
    static constexpr std::size_t kAlignment = 8;
    utils::FastPimpl<Impl, kSize, kAlignment, false> impl_;
};

class SelectMany {
public:
    SelectMany();
    ~SelectMany();

    SelectMany(const SelectMany&);
    SelectMany(SelectMany&&) noexcept;
    SelectMany& operator=(const SelectMany&);
    SelectMany& operator=(SelectMany&&) noexcept;

    void AddColumn(std::string column_name);
    void AddAllColumns();

    void Where(std::string column_name, Value value);

    void WhereString(std::string column_name, std::string value);
    void WhereInt32(std::string column_name, std::int32_t value);
    void WhereInt64(std::string column_name, std::int64_t value);
    void WhereBool(std::string column_name, bool value);
    void WhereFloat(std::string column_name, float value);
    void WhereDouble(std::string column_name, double value);
    void WhereUuid(std::string column_name, Uuid value);
    void WhereTimestamp(std::string column_name, Timestamp value);

    void SetLimit(std::size_t limit);
    void SetPageSize(std::size_t page_size);
    void AllowFiltering();

private:
    friend class storages::scylla::impl::driver::DriverTableImpl;

    class Impl;
    static constexpr std::size_t kSize = 160;
    static constexpr std::size_t kAlignment = 8;
    utils::FastPimpl<Impl, kSize, kAlignment, false> impl_;
};

class UpdateOne {
public:
    UpdateOne();
    ~UpdateOne();

    UpdateOne(const UpdateOne&);
    UpdateOne(UpdateOne&&) noexcept;
    UpdateOne& operator=(const UpdateOne&);
    UpdateOne& operator=(UpdateOne&&) noexcept;

    void Set(std::string column_name, Value value);

    void SetString(std::string column_name, std::string value);
    void SetInt32(std::string column_name, std::int32_t value);
    void SetInt64(std::string column_name, std::int64_t value);
    void SetBool(std::string column_name, bool value);
    void SetFloat(std::string column_name, float value);
    void SetDouble(std::string column_name, double value);
    void SetUuid(std::string column_name, Uuid value);
    void SetTimestamp(std::string column_name, Timestamp value);
    void SetBlob(std::string column_name, Blob value);
    void SetList(std::string column_name, List value);
    void SetMap(std::string column_name, Map value);
    void SetNull(std::string column_name);

    void Where(std::string column_name, Value value);

    void WhereString(std::string column_name, std::string value);
    void WhereInt32(std::string column_name, std::int32_t value);
    void WhereInt64(std::string column_name, std::int64_t value);
    void WhereBool(std::string column_name, bool value);
    void WhereFloat(std::string column_name, float value);
    void WhereDouble(std::string column_name, double value);
    void WhereUuid(std::string column_name, Uuid value);

    void If(std::string column_name, Value value);
    void IfExists();
    void IfString(std::string column_name, std::string value);
    void IfInt32(std::string column_name, std::int32_t value);
    void IfInt64(std::string column_name, std::int64_t value);
    void IfBool(std::string column_name, bool value);
    void IfFloat(std::string column_name, float value);
    void IfDouble(std::string column_name, double value);

    void UsingTtl(std::int32_t seconds);
    void UsingTimestamp(std::int64_t microseconds);

private:
    friend class storages::scylla::impl::driver::DriverTableImpl;

    class Impl;
    static constexpr std::size_t kSize = 192;
    static constexpr std::size_t kAlignment = 8;
    utils::FastPimpl<Impl, kSize, kAlignment, false> impl_;
};

class Count {
public:
    Count();
    ~Count();

    Count(const Count&);
    Count(Count&&) noexcept;
    Count& operator=(const Count&);
    Count& operator=(Count&&) noexcept;

    void Where(std::string column_name, Value value);

    void WhereString(std::string column_name, std::string value);
    void WhereInt32(std::string column_name, std::int32_t value);
    void WhereInt64(std::string column_name, std::int64_t value);
    void WhereBool(std::string column_name, bool value);
    void WhereFloat(std::string column_name, float value);
    void WhereDouble(std::string column_name, double value);

private:
    friend class storages::scylla::impl::driver::DriverTableImpl;

    class Impl;
    static constexpr std::size_t kSize = 64;
    static constexpr std::size_t kAlignment = 8;
    utils::FastPimpl<Impl, kSize, kAlignment, false> impl_;
};

class InsertMany {
public:
    InsertMany();
    ~InsertMany();

    InsertMany(const InsertMany&);
    InsertMany(InsertMany&&) noexcept;
    InsertMany& operator=(const InsertMany&);
    InsertMany& operator=(InsertMany&&) noexcept;

    void NextRow();

    void Bind(std::string column_name, Value value);

    void BindString(std::string column_name, std::string value);
    void BindInt32(std::string column_name, std::int32_t value);
    void BindInt64(std::string column_name, std::int64_t value);
    void BindBool(std::string column_name, bool value);
    void BindFloat(std::string column_name, float value);
    void BindDouble(std::string column_name, double value);
    void BindUuid(std::string column_name, Uuid value);
    void BindTimestamp(std::string column_name, Timestamp value);
    void BindBlob(std::string column_name, Blob value);

    void UsingTtl(std::int32_t seconds);
    void UsingTimestamp(std::int64_t microseconds);

private:
    friend class storages::scylla::impl::driver::DriverTableImpl;

    class Impl;
    static constexpr std::size_t kSize = 64;
    static constexpr std::size_t kAlignment = 8;
    utils::FastPimpl<Impl, kSize, kAlignment, false> impl_;
};

class Truncate {
public:
    Truncate() = default;
    ~Truncate() = default;

    Truncate(const Truncate&) = default;
    Truncate(Truncate&&) noexcept = default;
    Truncate& operator=(const Truncate&) = default;
    Truncate& operator=(Truncate&&) noexcept = default;
};

}  // namespace storages::scylla::operations

USERVER_NAMESPACE_END
