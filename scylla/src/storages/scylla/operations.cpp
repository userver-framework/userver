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

class SelectOne::Impl {
public:
    std::vector<std::string> columns;
    bool select_all{true};
    std::vector<SelectOne::Condition> conditions;
};

SelectOne::SelectOne() = default;
SelectOne::~SelectOne() = default;
SelectOne::SelectOne(const SelectOne&) = default;
SelectOne::SelectOne(SelectOne&&) noexcept = default;
SelectOne& SelectOne::operator=(const SelectOne&) = default;
SelectOne& SelectOne::operator=(SelectOne&&) noexcept = default;

void SelectOne::AddColumn(std::string column_name) {
    impl_->select_all = false;
    impl_->columns.push_back(std::move(column_name));
}

void SelectOne::AddAllColumns() {
    impl_->select_all = true;
    impl_->columns.clear();
}

void SelectOne::WhereString(std::string column_name, std::string value) {
    impl_->conditions.push_back({std::move(column_name), std::move(value)});
}

void SelectOne::WhereInt32(std::string column_name, int32_t value) {
    impl_->conditions.push_back({std::move(column_name), value});
}

void SelectOne::WhereInt64(std::string column_name, int64_t value) {
    impl_->conditions.push_back({std::move(column_name), value});
}

void SelectOne::WhereBool(std::string column_name, bool value) {
    impl_->conditions.push_back({std::move(column_name), value});
}

void SelectOne::WhereFloat(std::string column_name, float value) {
    impl_->conditions.push_back({std::move(column_name), value});
}

void SelectOne::WhereDouble(std::string column_name, double value) {
    impl_->conditions.push_back({std::move(column_name), value});
}

class DeleteOne::Impl {
public:
    std::vector<DeleteOne::Condition> conditions;
};

DeleteOne::DeleteOne() = default;
DeleteOne::~DeleteOne() = default;
DeleteOne::DeleteOne(const DeleteOne&) = default;
DeleteOne::DeleteOne(DeleteOne&&) noexcept = default;
DeleteOne& DeleteOne::operator=(const DeleteOne&) = default;
DeleteOne& DeleteOne::operator=(DeleteOne&&) noexcept = default;

void DeleteOne::WhereString(std::string column_name, std::string value) {
    impl_->conditions.push_back({std::move(column_name), std::move(value)});
}
void DeleteOne::WhereInt32(std::string column_name, int32_t value) {
    impl_->conditions.push_back({std::move(column_name), value});
}
void DeleteOne::WhereInt64(std::string column_name, int64_t value) {
    impl_->conditions.push_back({std::move(column_name), value});
}
void DeleteOne::WhereBool(std::string column_name, bool value) {
    impl_->conditions.push_back({std::move(column_name), value});
}
void DeleteOne::WhereFloat(std::string column_name, float value) {
    impl_->conditions.push_back({std::move(column_name), value});
}
void DeleteOne::WhereDouble(std::string column_name, double value) {
    impl_->conditions.push_back({std::move(column_name), value});
}

class SelectMany::Impl {
public:
    std::vector<std::string> columns;
    bool select_all{true};
    std::vector<SelectMany::Condition> conditions;
    size_t limit{0};
};

SelectMany::SelectMany() = default;
SelectMany::~SelectMany() = default;
SelectMany::SelectMany(const SelectMany&) = default;
SelectMany::SelectMany(SelectMany&&) noexcept = default;
SelectMany& SelectMany::operator=(const SelectMany&) = default;
SelectMany& SelectMany::operator=(SelectMany&&) noexcept = default;

void SelectMany::AddColumn(std::string column_name) {
    impl_->select_all = false;
    impl_->columns.push_back(std::move(column_name));
}

void SelectMany::AddAllColumns() {
    impl_->select_all = true;
    impl_->columns.clear();
}

void SelectMany::WhereString(std::string column_name, std::string value) {
    impl_->conditions.push_back({std::move(column_name), std::move(value)});
}
void SelectMany::WhereInt32(std::string column_name, int32_t value) {
    impl_->conditions.push_back({std::move(column_name), value});
}
void SelectMany::WhereInt64(std::string column_name, int64_t value) {
    impl_->conditions.push_back({std::move(column_name), value});
}
void SelectMany::WhereBool(std::string column_name, bool value) {
    impl_->conditions.push_back({std::move(column_name), value});
}
void SelectMany::WhereFloat(std::string column_name, float value) {
    impl_->conditions.push_back({std::move(column_name), value});
}
void SelectMany::WhereDouble(std::string column_name, double value) {
    impl_->conditions.push_back({std::move(column_name), value});
}

void SelectMany::SetLimit(size_t limit) {
    impl_->limit = limit;
}

class UpdateOne::Impl {
public:
    std::vector<UpdateOne::Assignment> assignments;
    std::vector<UpdateOne::Condition> conditions;
};

UpdateOne::UpdateOne() = default;
UpdateOne::~UpdateOne() = default;
UpdateOne::UpdateOne(const UpdateOne&) = default;
UpdateOne::UpdateOne(UpdateOne&&) noexcept = default;
UpdateOne& UpdateOne::operator=(const UpdateOne&) = default;
UpdateOne& UpdateOne::operator=(UpdateOne&&) noexcept = default;

void UpdateOne::SetString(std::string column_name, std::string value) {
    impl_->assignments.push_back({std::move(column_name), std::move(value)});
}
void UpdateOne::SetInt32(std::string column_name, int32_t value) {
    impl_->assignments.push_back({std::move(column_name), value});
}
void UpdateOne::SetInt64(std::string column_name, int64_t value) {
    impl_->assignments.push_back({std::move(column_name), value});
}
void UpdateOne::SetBool(std::string column_name, bool value) {
    impl_->assignments.push_back({std::move(column_name), value});
}
void UpdateOne::SetFloat(std::string column_name, float value) {
    impl_->assignments.push_back({std::move(column_name), value});
}
void UpdateOne::SetDouble(std::string column_name, double value) {
    impl_->assignments.push_back({std::move(column_name), value});
}

void UpdateOne::WhereString(std::string column_name, std::string value) {
    impl_->conditions.push_back({std::move(column_name), std::move(value)});
}
void UpdateOne::WhereInt32(std::string column_name, int32_t value) {
    impl_->conditions.push_back({std::move(column_name), value});
}
void UpdateOne::WhereInt64(std::string column_name, int64_t value) {
    impl_->conditions.push_back({std::move(column_name), value});
}
void UpdateOne::WhereBool(std::string column_name, bool value) {
    impl_->conditions.push_back({std::move(column_name), value});
}
void UpdateOne::WhereFloat(std::string column_name, float value) {
    impl_->conditions.push_back({std::move(column_name), value});
}
void UpdateOne::WhereDouble(std::string column_name, double value) {
    impl_->conditions.push_back({std::move(column_name), value});
}

class Count::Impl {
public:
    std::vector<Count::Condition> conditions;
};

Count::Count() = default;
Count::~Count() = default;
Count::Count(const Count&) = default;
Count::Count(Count&&) noexcept = default;
Count& Count::operator=(const Count&) = default;
Count& Count::operator=(Count&&) noexcept = default;

void Count::WhereString(std::string column_name, std::string value) {
    impl_->conditions.push_back({std::move(column_name), std::move(value)});
}
void Count::WhereInt32(std::string column_name, int32_t value) {
    impl_->conditions.push_back({std::move(column_name), value});
}
void Count::WhereInt64(std::string column_name, int64_t value) {
    impl_->conditions.push_back({std::move(column_name), value});
}
void Count::WhereBool(std::string column_name, bool value) {
    impl_->conditions.push_back({std::move(column_name), value});
}
void Count::WhereFloat(std::string column_name, float value) {
    impl_->conditions.push_back({std::move(column_name), value});
}
void Count::WhereDouble(std::string column_name, double value) {
    impl_->conditions.push_back({std::move(column_name), value});
}

class InsertMany::Impl {
public:

    std::vector<std::vector<InsertMany::Binding>> rows{{}};
};

InsertMany::InsertMany() = default;
InsertMany::~InsertMany() = default;
InsertMany::InsertMany(const InsertMany&) = default;
InsertMany::InsertMany(InsertMany&&) noexcept = default;
InsertMany& InsertMany::operator=(const InsertMany&) = default;
InsertMany& InsertMany::operator=(InsertMany&&) noexcept = default;

void InsertMany::NextRow() { impl_->rows.emplace_back(); }

void InsertMany::BindString(std::string column_name, std::string value) {
    impl_->rows.back().push_back({std::move(column_name), std::move(value)});
}
void InsertMany::BindInt32(std::string column_name, int32_t value) {
    impl_->rows.back().push_back({std::move(column_name), value});
}
void InsertMany::BindInt64(std::string column_name, int64_t value) {
    impl_->rows.back().push_back({std::move(column_name), value});
}
void InsertMany::BindBool(std::string column_name, bool value) {
    impl_->rows.back().push_back({std::move(column_name), value});
}
void InsertMany::BindFloat(std::string column_name, float value) {
    impl_->rows.back().push_back({std::move(column_name), value});
}
void InsertMany::BindDouble(std::string column_name, double value) {
    impl_->rows.back().push_back({std::move(column_name), value});
}

}

USERVER_NAMESPACE_END
