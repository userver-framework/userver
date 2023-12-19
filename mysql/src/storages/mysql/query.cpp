#include <userver/storages/mysql/query.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

Query::Query(const char* statement, std::optional<Name> name)
    : statement_{statement}, name_{std::move(name)} {}

Query::Query(std::string statement, std::optional<Name> name)
    : statement_{std::move(statement)}, name_{std::move(name)} {}

const std::string& Query::GetStatement() const { return statement_; }

const std::optional<Query::Name>& Query::GetName() const { return name_; }

}  // namespace storages::mysql

USERVER_NAMESPACE_END
