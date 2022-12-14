#include <storages/mysql/impl/metadata/mysql_server_info.hpp>

#include <boost/algorithm/string/find.hpp>
#include <iostream>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::metadata {

namespace {

MySQLServerInfo::Type ParseServerType(const std::string& server_type_str) {
  using Type = MySQLServerInfo::Type;
  if (boost::algorithm::ifind_first(server_type_str, "mysql")) {
    return Type::kMySQL;
  }
  if (boost::algorithm::ifind_first(server_type_str, "mariadb")) {
    return Type::kMariaDB;
  }

  return Type::kUnknown;
}

}  // namespace

MySQLServerInfo::MySQLServerInfo()
    : server_version{0}, server_type{Type::kUnknown} {}

MySQLServerInfo MySQLServerInfo::Get(MYSQL& mysql) {
  const char* server_type{nullptr};
  mariadb_get_info(&mysql, MARIADB_CONNECTION_SERVER_TYPE, &server_type);
  std::uint64_t server_version{0};
  mariadb_get_info(&mysql, MARIADB_CONNECTION_SERVER_VERSION_ID,
                   &server_version);

  return MySQLServerInfo{server_type, server_version};
}

MySQLServerInfo::MySQLServerInfo(const char* server_type_str,
                                 std::uint64_t server_version)
    : server_type_str{server_type_str},
      server_version{server_version},
      server_type{ParseServerType(this->server_type_str)} {}

}  // namespace storages::mysql::impl::metadata

USERVER_NAMESPACE_END
