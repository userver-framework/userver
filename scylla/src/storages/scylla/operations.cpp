#include <userver/storages/scylla/operations.hpp>

#include <utility>

#include <storages/scylla/operations_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::operations {

namespace {

NamedValue Make(std::string name, Value value) { return NamedValue{std::move(name), std::move(value)}; }

}  // namespace

InsertOne::InsertOne() = default;
InsertOne::~InsertOne() = default;
InsertOne::InsertOne(const InsertOne&) = default;
InsertOne::InsertOne(InsertOne&&) noexcept = default;
InsertOne& InsertOne::operator=(const InsertOne&) = default;
InsertOne& InsertOne::operator=(InsertOne&&) noexcept = default;

void InsertOne::Bind(std::string column_name, Value value) {
    impl_->bindings.push_back(Make(std::move(column_name), std::move(value)));
}
void InsertOne::BindString(std::string c, std::string v) { Bind(std::move(c), Value{std::move(v)}); }
void InsertOne::BindInt32(std::string c, std::int32_t v) { Bind(std::move(c), Value{v}); }
void InsertOne::BindInt64(std::string c, std::int64_t v) { Bind(std::move(c), Value{v}); }
void InsertOne::BindBool(std::string c, bool v) { Bind(std::move(c), Value{v}); }
void InsertOne::BindFloat(std::string c, float v) { Bind(std::move(c), Value{v}); }
void InsertOne::BindDouble(std::string c, double v) { Bind(std::move(c), Value{v}); }
void InsertOne::BindUuid(std::string c, Uuid v) { Bind(std::move(c), Value{v}); }
void InsertOne::BindTimestamp(std::string c, Timestamp v) { Bind(std::move(c), Value{v}); }
void InsertOne::BindBlob(std::string c, Blob v) { Bind(std::move(c), Value{std::move(v)}); }
void InsertOne::BindInet(std::string c, Inet v) { Bind(std::move(c), Value{std::move(v)}); }
void InsertOne::BindList(std::string c, List v) { Bind(std::move(c), Value{std::move(v)}); }
void InsertOne::BindMap(std::string c, Map v) { Bind(std::move(c), Value{std::move(v)}); }
void InsertOne::BindSet(std::string c, Set v) { Bind(std::move(c), Value{std::move(v)}); }
void InsertOne::BindNull(std::string c) { Bind(std::move(c), Value::Null()); }

void InsertOne::IfNotExists() { impl_->if_not_exists = true; }
void InsertOne::UsingTtl(std::int32_t s) { impl_->using_clause.ttl_seconds = s; }
void InsertOne::UsingTimestamp(std::int64_t us) { impl_->using_clause.timestamp_micros = us; }

SelectOne::SelectOne() = default;
SelectOne::~SelectOne() = default;
SelectOne::SelectOne(const SelectOne&) = default;
SelectOne::SelectOne(SelectOne&&) noexcept = default;
SelectOne& SelectOne::operator=(const SelectOne&) = default;
SelectOne& SelectOne::operator=(SelectOne&&) noexcept = default;

void SelectOne::AddColumn(std::string c) {
    impl_->select_all = false;
    impl_->columns.push_back(std::move(c));
}
void SelectOne::AddAllColumns() {
    impl_->select_all = true;
    impl_->columns.clear();
}
void SelectOne::Where(std::string c, Value v) { impl_->conditions.push_back(Make(std::move(c), std::move(v))); }
void SelectOne::WhereString(std::string c, std::string v) { Where(std::move(c), Value{std::move(v)}); }
void SelectOne::WhereInt32(std::string c, std::int32_t v) { Where(std::move(c), Value{v}); }
void SelectOne::WhereInt64(std::string c, std::int64_t v) { Where(std::move(c), Value{v}); }
void SelectOne::WhereBool(std::string c, bool v) { Where(std::move(c), Value{v}); }
void SelectOne::WhereFloat(std::string c, float v) { Where(std::move(c), Value{v}); }
void SelectOne::WhereDouble(std::string c, double v) { Where(std::move(c), Value{v}); }
void SelectOne::WhereUuid(std::string c, Uuid v) { Where(std::move(c), Value{v}); }
void SelectOne::WhereTimestamp(std::string c, Timestamp v) { Where(std::move(c), Value{v}); }
void SelectOne::WhereBlob(std::string c, Blob v) { Where(std::move(c), Value{std::move(v)}); }
void SelectOne::WhereInet(std::string c, Inet v) { Where(std::move(c), Value{std::move(v)}); }

DeleteOne::DeleteOne() = default;
DeleteOne::~DeleteOne() = default;
DeleteOne::DeleteOne(const DeleteOne&) = default;
DeleteOne::DeleteOne(DeleteOne&&) noexcept = default;
DeleteOne& DeleteOne::operator=(const DeleteOne&) = default;
DeleteOne& DeleteOne::operator=(DeleteOne&&) noexcept = default;

void DeleteOne::Where(std::string c, Value v) { impl_->conditions.push_back(Make(std::move(c), std::move(v))); }
void DeleteOne::WhereString(std::string c, std::string v) { Where(std::move(c), Value{std::move(v)}); }
void DeleteOne::WhereInt32(std::string c, std::int32_t v) { Where(std::move(c), Value{v}); }
void DeleteOne::WhereInt64(std::string c, std::int64_t v) { Where(std::move(c), Value{v}); }
void DeleteOne::WhereBool(std::string c, bool v) { Where(std::move(c), Value{v}); }
void DeleteOne::WhereFloat(std::string c, float v) { Where(std::move(c), Value{v}); }
void DeleteOne::WhereDouble(std::string c, double v) { Where(std::move(c), Value{v}); }
void DeleteOne::WhereUuid(std::string c, Uuid v) { Where(std::move(c), Value{v}); }
void DeleteOne::WhereTimestamp(std::string c, Timestamp v) { Where(std::move(c), Value{v}); }

void DeleteOne::If(std::string c, Value v) { impl_->if_conditions.push_back(Make(std::move(c), std::move(v))); }
void DeleteOne::IfExists() { impl_->if_exists = true; }
void DeleteOne::IfString(std::string c, std::string v) { If(std::move(c), Value{std::move(v)}); }
void DeleteOne::IfInt32(std::string c, std::int32_t v) { If(std::move(c), Value{v}); }
void DeleteOne::IfInt64(std::string c, std::int64_t v) { If(std::move(c), Value{v}); }
void DeleteOne::IfBool(std::string c, bool v) { If(std::move(c), Value{v}); }
void DeleteOne::IfFloat(std::string c, float v) { If(std::move(c), Value{v}); }
void DeleteOne::IfDouble(std::string c, double v) { If(std::move(c), Value{v}); }

SelectMany::SelectMany() = default;
SelectMany::~SelectMany() = default;
SelectMany::SelectMany(const SelectMany&) = default;
SelectMany::SelectMany(SelectMany&&) noexcept = default;
SelectMany& SelectMany::operator=(const SelectMany&) = default;
SelectMany& SelectMany::operator=(SelectMany&&) noexcept = default;

void SelectMany::AddColumn(std::string c) {
    impl_->select_all = false;
    impl_->columns.push_back(std::move(c));
}
void SelectMany::AddAllColumns() {
    impl_->select_all = true;
    impl_->columns.clear();
}
void SelectMany::Where(std::string c, Value v) { impl_->conditions.push_back(Make(std::move(c), std::move(v))); }
void SelectMany::WhereString(std::string c, std::string v) { Where(std::move(c), Value{std::move(v)}); }
void SelectMany::WhereInt32(std::string c, std::int32_t v) { Where(std::move(c), Value{v}); }
void SelectMany::WhereInt64(std::string c, std::int64_t v) { Where(std::move(c), Value{v}); }
void SelectMany::WhereBool(std::string c, bool v) { Where(std::move(c), Value{v}); }
void SelectMany::WhereFloat(std::string c, float v) { Where(std::move(c), Value{v}); }
void SelectMany::WhereDouble(std::string c, double v) { Where(std::move(c), Value{v}); }
void SelectMany::WhereUuid(std::string c, Uuid v) { Where(std::move(c), Value{v}); }
void SelectMany::WhereTimestamp(std::string c, Timestamp v) { Where(std::move(c), Value{v}); }

void SelectMany::SetLimit(std::size_t n) { impl_->limit = n; }
void SelectMany::SetPageSize(std::size_t n) { impl_->page_size = n; }
void SelectMany::AllowFiltering() { impl_->allow_filtering = true; }

UpdateOne::UpdateOne() = default;
UpdateOne::~UpdateOne() = default;
UpdateOne::UpdateOne(const UpdateOne&) = default;
UpdateOne::UpdateOne(UpdateOne&&) noexcept = default;
UpdateOne& UpdateOne::operator=(const UpdateOne&) = default;
UpdateOne& UpdateOne::operator=(UpdateOne&&) noexcept = default;

void UpdateOne::Set(std::string c, Value v) { impl_->assignments.push_back(Make(std::move(c), std::move(v))); }
void UpdateOne::SetString(std::string c, std::string v) { Set(std::move(c), Value{std::move(v)}); }
void UpdateOne::SetInt32(std::string c, std::int32_t v) { Set(std::move(c), Value{v}); }
void UpdateOne::SetInt64(std::string c, std::int64_t v) { Set(std::move(c), Value{v}); }
void UpdateOne::SetBool(std::string c, bool v) { Set(std::move(c), Value{v}); }
void UpdateOne::SetFloat(std::string c, float v) { Set(std::move(c), Value{v}); }
void UpdateOne::SetDouble(std::string c, double v) { Set(std::move(c), Value{v}); }
void UpdateOne::SetUuid(std::string c, Uuid v) { Set(std::move(c), Value{v}); }
void UpdateOne::SetTimestamp(std::string c, Timestamp v) { Set(std::move(c), Value{v}); }
void UpdateOne::SetBlob(std::string c, Blob v) { Set(std::move(c), Value{std::move(v)}); }
void UpdateOne::SetList(std::string c, List v) { Set(std::move(c), Value{std::move(v)}); }
void UpdateOne::SetMap(std::string c, Map v) { Set(std::move(c), Value{std::move(v)}); }
void UpdateOne::SetNull(std::string c) { Set(std::move(c), Value::Null()); }

void UpdateOne::Where(std::string c, Value v) { impl_->conditions.push_back(Make(std::move(c), std::move(v))); }
void UpdateOne::WhereString(std::string c, std::string v) { Where(std::move(c), Value{std::move(v)}); }
void UpdateOne::WhereInt32(std::string c, std::int32_t v) { Where(std::move(c), Value{v}); }
void UpdateOne::WhereInt64(std::string c, std::int64_t v) { Where(std::move(c), Value{v}); }
void UpdateOne::WhereBool(std::string c, bool v) { Where(std::move(c), Value{v}); }
void UpdateOne::WhereFloat(std::string c, float v) { Where(std::move(c), Value{v}); }
void UpdateOne::WhereDouble(std::string c, double v) { Where(std::move(c), Value{v}); }
void UpdateOne::WhereUuid(std::string c, Uuid v) { Where(std::move(c), Value{v}); }

void UpdateOne::If(std::string c, Value v) { impl_->if_conditions.push_back(Make(std::move(c), std::move(v))); }
void UpdateOne::IfExists() { impl_->if_exists = true; }
void UpdateOne::IfString(std::string c, std::string v) { If(std::move(c), Value{std::move(v)}); }
void UpdateOne::IfInt32(std::string c, std::int32_t v) { If(std::move(c), Value{v}); }
void UpdateOne::IfInt64(std::string c, std::int64_t v) { If(std::move(c), Value{v}); }
void UpdateOne::IfBool(std::string c, bool v) { If(std::move(c), Value{v}); }
void UpdateOne::IfFloat(std::string c, float v) { If(std::move(c), Value{v}); }
void UpdateOne::IfDouble(std::string c, double v) { If(std::move(c), Value{v}); }

void UpdateOne::UsingTtl(std::int32_t s) { impl_->using_clause.ttl_seconds = s; }
void UpdateOne::UsingTimestamp(std::int64_t us) { impl_->using_clause.timestamp_micros = us; }

Count::Count() = default;
Count::~Count() = default;
Count::Count(const Count&) = default;
Count::Count(Count&&) noexcept = default;
Count& Count::operator=(const Count&) = default;
Count& Count::operator=(Count&&) noexcept = default;

void Count::Where(std::string c, Value v) { impl_->conditions.push_back(Make(std::move(c), std::move(v))); }
void Count::WhereString(std::string c, std::string v) { Where(std::move(c), Value{std::move(v)}); }
void Count::WhereInt32(std::string c, std::int32_t v) { Where(std::move(c), Value{v}); }
void Count::WhereInt64(std::string c, std::int64_t v) { Where(std::move(c), Value{v}); }
void Count::WhereBool(std::string c, bool v) { Where(std::move(c), Value{v}); }
void Count::WhereFloat(std::string c, float v) { Where(std::move(c), Value{v}); }
void Count::WhereDouble(std::string c, double v) { Where(std::move(c), Value{v}); }

InsertMany::InsertMany() = default;
InsertMany::~InsertMany() = default;
InsertMany::InsertMany(const InsertMany&) = default;
InsertMany::InsertMany(InsertMany&&) noexcept = default;
InsertMany& InsertMany::operator=(const InsertMany&) = default;
InsertMany& InsertMany::operator=(InsertMany&&) noexcept = default;

void InsertMany::NextRow() { impl_->rows.emplace_back(); }
void InsertMany::Bind(std::string c, Value v) { impl_->rows.back().push_back(Make(std::move(c), std::move(v))); }
void InsertMany::BindString(std::string c, std::string v) { Bind(std::move(c), Value{std::move(v)}); }
void InsertMany::BindInt32(std::string c, std::int32_t v) { Bind(std::move(c), Value{v}); }
void InsertMany::BindInt64(std::string c, std::int64_t v) { Bind(std::move(c), Value{v}); }
void InsertMany::BindBool(std::string c, bool v) { Bind(std::move(c), Value{v}); }
void InsertMany::BindFloat(std::string c, float v) { Bind(std::move(c), Value{v}); }
void InsertMany::BindDouble(std::string c, double v) { Bind(std::move(c), Value{v}); }
void InsertMany::BindUuid(std::string c, Uuid v) { Bind(std::move(c), Value{v}); }
void InsertMany::BindTimestamp(std::string c, Timestamp v) { Bind(std::move(c), Value{v}); }
void InsertMany::BindBlob(std::string c, Blob v) { Bind(std::move(c), Value{std::move(v)}); }

void InsertMany::UsingTtl(std::int32_t s) { impl_->using_clause.ttl_seconds = s; }
void InsertMany::UsingTimestamp(std::int64_t us) { impl_->using_clause.timestamp_micros = us; }

}  // namespace storages::scylla::operations

USERVER_NAMESPACE_END
