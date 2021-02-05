#include <storages/postgres/query.hpp>

namespace storages::postgres {

Query::Query(const char* statement, std::optional<Name> name)
    : statement_(statement), name_(std::move(name)) {}

Query::Query(std::string statement, std::optional<Name> name)
    : statement_(std::move(statement)), name_(std::move(name)) {}

const std::optional<Query::Name>& Query::GetName() const { return name_; }

const std::string& Query::Statement() const { return statement_; }

}  // namespace storages::postgres
