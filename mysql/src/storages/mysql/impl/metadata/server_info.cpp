#include <storages/mysql/impl/metadata/server_info.hpp>

#include <boost/algorithm/string/find.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::metadata {

namespace {

ServerInfo::Type ParseServerType(const std::string& server_type_str) {
  using Type = ServerInfo::Type;
  if (boost::algorithm::ifind_first(server_type_str, "mysql")) {
    return Type::kMySQL;
  }
  if (boost::algorithm::ifind_first(server_type_str, "mariadb")) {
    return Type::kMariaDB;
  }

  return Type::kUnknown;
}

}  // namespace

ServerInfo ServerInfo::Get(MYSQL& mysql) {
  const char* server_type_c_str{nullptr};
  mariadb_get_info(&mysql, MARIADB_CONNECTION_SERVER_TYPE, &server_type_c_str);
  std::uint64_t server_version{0};
  mariadb_get_info(&mysql, MARIADB_CONNECTION_SERVER_VERSION_ID,
                   &server_version);

  std::string server_type_str{server_type_c_str};
  const auto server_type = ParseServerType(server_type_str);

  return ServerInfo{std::move(server_type_str), server_version, server_type};
}

}  // namespace storages::mysql::impl::metadata

USERVER_NAMESPACE_END
