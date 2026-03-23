#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::impl::driver {
class DriverTableImpl;
}  // namespace storages::scylla::impl::driver

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

}  // namespace storages::scylla::operations

USERVER_NAMESPACE_END