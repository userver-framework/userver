#include <userver/storages/sqlite/query.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

Query::Query(const char* statement)
    : statement_{statement} {}

Query::Query(std::string statement)
    : statement_{std::move(statement)} {}

const std::string& Query::GetStatement() const { return statement_; }

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
