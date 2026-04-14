#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::impl::driver {
class DriverTableImpl;
}

namespace storages::scylla::operations {

class InsertOne {
public:
    InsertOne();
    ~InsertOne();

    InsertOne(const InsertOne&);
    InsertOne(InsertOne&&) noexcept;
    InsertOne& operator=(const InsertOne&);
    InsertOne& operator=(InsertOne&&) noexcept;

    void BindString(std::string column_name, std::string value);
    void BindInt32(std::string column_name, int32_t value);
    void BindInt64(std::string column_name, int64_t value);
    void BindBool(std::string column_name, bool value);
    void BindFloat(std::string column_name, float value);
    void BindDouble(std::string column_name, double value);

private:
    friend class storages::scylla::impl::driver::DriverTableImpl;

    struct Binding {
        std::string column_name;
        std::variant<std::string, int32_t, int64_t, bool, float, double> value;
    };

    class Impl;
    static constexpr size_t kSize = 64;
    static constexpr size_t kAlignment = 8;
    utils::FastPimpl<Impl, kSize, kAlignment, false> impl_;
};

class SelectOne {
public:
    using Value = std::variant<std::string, int32_t, int64_t, bool, float, double>;
    using Row = std::vector<std::pair<std::string, Value>>;

    SelectOne();
    ~SelectOne();

    SelectOne(const SelectOne&);
    SelectOne(SelectOne&&) noexcept;
    SelectOne& operator=(const SelectOne&);
    SelectOne& operator=(SelectOne&&) noexcept;

    void AddColumn(std::string column_name);
    void AddAllColumns();

    void WhereString(std::string column_name, std::string value);
    void WhereInt32(std::string column_name, int32_t value);
    void WhereInt64(std::string column_name, int64_t value);
    void WhereBool(std::string column_name, bool value);
    void WhereFloat(std::string column_name, float value);
    void WhereDouble(std::string column_name, double value);

private:
    friend class storages::scylla::impl::driver::DriverTableImpl;

    struct Condition {
        std::string column_name;
        std::variant<std::string, int32_t, int64_t, bool, float, double> value;
    };

    class Impl;
    static constexpr size_t kSize = 128;
    static constexpr size_t kAlignment = 8;
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

    void WhereString(std::string column_name, std::string value);
    void WhereInt32(std::string column_name, int32_t value);
    void WhereInt64(std::string column_name, int64_t value);
    void WhereBool(std::string column_name, bool value);
    void WhereFloat(std::string column_name, float value);
    void WhereDouble(std::string column_name, double value);

private:
    friend class storages::scylla::impl::driver::DriverTableImpl;

    struct Condition {
        std::string column_name;
        std::variant<std::string, int32_t, int64_t, bool, float, double> value;
    };

    class Impl;
    static constexpr size_t kSize = 64;
    static constexpr size_t kAlignment = 8;
    utils::FastPimpl<Impl, kSize, kAlignment, false> impl_;
};

class SelectMany {
public:
    using Value = std::variant<std::string, int32_t, int64_t, bool, float, double>;
    using Row = std::vector<std::pair<std::string, Value>>;
    using ResultSet = std::vector<Row>;

    SelectMany();
    ~SelectMany();

    SelectMany(const SelectMany&);
    SelectMany(SelectMany&&) noexcept;
    SelectMany& operator=(const SelectMany&);
    SelectMany& operator=(SelectMany&&) noexcept;

    void AddColumn(std::string column_name);
    void AddAllColumns();

    void WhereString(std::string column_name, std::string value);
    void WhereInt32(std::string column_name, int32_t value);
    void WhereInt64(std::string column_name, int64_t value);
    void WhereBool(std::string column_name, bool value);
    void WhereFloat(std::string column_name, float value);
    void WhereDouble(std::string column_name, double value);

    void SetLimit(size_t limit);

private:
    friend class storages::scylla::impl::driver::DriverTableImpl;

    struct Condition {
        std::string column_name;
        std::variant<std::string, int32_t, int64_t, bool, float, double> value;
    };

    class Impl;
    static constexpr size_t kSize = 128;
    static constexpr size_t kAlignment = 8;
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

    void SetString(std::string column_name, std::string value);
    void SetInt32(std::string column_name, int32_t value);
    void SetInt64(std::string column_name, int64_t value);
    void SetBool(std::string column_name, bool value);
    void SetFloat(std::string column_name, float value);
    void SetDouble(std::string column_name, double value);

    void WhereString(std::string column_name, std::string value);
    void WhereInt32(std::string column_name, int32_t value);
    void WhereInt64(std::string column_name, int64_t value);
    void WhereBool(std::string column_name, bool value);
    void WhereFloat(std::string column_name, float value);
    void WhereDouble(std::string column_name, double value);

private:
    friend class storages::scylla::impl::driver::DriverTableImpl;

    struct Assignment {
        std::string column_name;
        std::variant<std::string, int32_t, int64_t, bool, float, double> value;
    };
    struct Condition {
        std::string column_name;
        std::variant<std::string, int32_t, int64_t, bool, float, double> value;
    };

    class Impl;
    static constexpr size_t kSize = 128;
    static constexpr size_t kAlignment = 8;
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

    void WhereString(std::string column_name, std::string value);
    void WhereInt32(std::string column_name, int32_t value);
    void WhereInt64(std::string column_name, int64_t value);
    void WhereBool(std::string column_name, bool value);
    void WhereFloat(std::string column_name, float value);
    void WhereDouble(std::string column_name, double value);

private:
    friend class storages::scylla::impl::driver::DriverTableImpl;

    struct Condition {
        std::string column_name;
        std::variant<std::string, int32_t, int64_t, bool, float, double> value;
    };

    class Impl;
    static constexpr size_t kSize = 64;
    static constexpr size_t kAlignment = 8;
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

    void BindString(std::string column_name, std::string value);
    void BindInt32(std::string column_name, int32_t value);
    void BindInt64(std::string column_name, int64_t value);
    void BindBool(std::string column_name, bool value);
    void BindFloat(std::string column_name, float value);
    void BindDouble(std::string column_name, double value);

private:
    friend class storages::scylla::impl::driver::DriverTableImpl;

    struct Binding {
        std::string column_name;
        std::variant<std::string, int32_t, int64_t, bool, float, double> value;
    };

    class Impl;
    static constexpr size_t kSize = 64;
    static constexpr size_t kAlignment = 8;
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

}

USERVER_NAMESPACE_END
