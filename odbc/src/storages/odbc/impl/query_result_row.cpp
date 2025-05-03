#include <storages/odbc/impl/query_result_row.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::impl {

QueryResultRow::QueryResultRow() = default;

QueryResultRow::~QueryResultRow() = default;

QueryResultRow::QueryResultRow(QueryResultRow&& other) noexcept
    : fields_(std::move(other.fields_)) {}

void QueryResultRow::AppendField(std::string&& field) {
    fields_.push_back(std::move(field));
}

std::size_t QueryResultRow::FieldsCount() const {
    return fields_.size();
}

const std::string& QueryResultRow::GetField(std::size_t ind) const {
    return fields_.at(ind);
}

std::string& QueryResultRow::GetField(std::size_t ind) {
    return fields_.at(ind);
}

}  // namespace storages::odbc::impl

USERVER_NAMESPACE_END 