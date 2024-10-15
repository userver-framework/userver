#include <userver/storages/mysql/query.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

Query::Query(const char* statement)
    : statement_{statement} {}

Query::Query(std::string statement)
    : statement_{std::move(statement)} {}

const std::string& Query::GetStatement() const { return statement_; }

const std::optional<Query::Name>& Query::GetName() const { return name_; }

}  // namespace storages::mysql

USERVER_NAMESPACE_END
