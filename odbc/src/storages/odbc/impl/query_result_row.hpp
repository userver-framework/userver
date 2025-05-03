#pragma once

#include <string>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::impl {

class QueryResultRow final {
public:
    QueryResultRow();
    ~QueryResultRow();

    QueryResultRow(const QueryResultRow& other) = delete;
    QueryResultRow(QueryResultRow&& other) noexcept;

    void AppendField(std::string&& field);

    std::size_t FieldsCount() const;

    const std::string& GetField(std::size_t ind) const;
    std::string& GetField(std::size_t ind);

private:
    std::vector<std::string> fields_;
};

}  // namespace storages::odbc::impl

USERVER_NAMESPACE_END