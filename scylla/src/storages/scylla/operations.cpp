#include <userver/storages/scylla/operations.hpp>

#include <variant>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::operations {

class InsertOne::Impl {
public:
    struct Binding {
        std::string column_name;
        std::variant<std::string, int32_t, int64_t, bool, float, double> value;
    };

    std::vector<Binding> bindings;
};

InsertOne::InsertOne() = default;
InsertOne::~InsertOne() = default;
InsertOne::InsertOne(const InsertOne&) = default;
InsertOne::InsertOne(InsertOne&&) noexcept = default;
InsertOne& InsertOne::operator=(const InsertOne&) = default;
InsertOne& InsertOne::operator=(InsertOne&&) noexcept = default;

void InsertOne::BindString(std::string column_name, std::string value) {
    impl_->bindings.push_back({std::move(column_name), std::move(value)});
}

void InsertOne::BindInt32(std::string column_name, int32_t value) {
    impl_->bindings.push_back({std::move(column_name), value});
}

void InsertOne::BindInt64(std::string column_name, int64_t value) {
    impl_->bindings.push_back({std::move(column_name), value});
}

void InsertOne::BindBool(std::string column_name, bool value) {
    impl_->bindings.push_back({std::move(column_name), value});
}

void InsertOne::BindFloat(std::string column_name, float value) {
    impl_->bindings.push_back({std::move(column_name), value});
}

void InsertOne::BindDouble(std::string column_name, double value) {
    impl_->bindings.push_back({std::move(column_name), value});
}

}  // namespace storages::scylla::operations

USERVER_NAMESPACE_END