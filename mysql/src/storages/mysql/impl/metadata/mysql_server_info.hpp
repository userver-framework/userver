#pragma once

#include <storages/mysql/impl/metadata/mysql_semver.hpp>

#include <storages/mysql/impl/mariadb_include.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::metadata {

struct MySQLServerInfo final {
  enum class Type { kMySQL, kMariaDB, kUnknown };

  std::string server_type_str;
  MySQLSemVer server_version;
  Type server_type;

  MySQLServerInfo();

  static MySQLServerInfo Get(MYSQL& mysql);

 private:
  MySQLServerInfo(const char* server_type_str, std::uint64_t server_version);
};

}  // namespace storages::mysql::impl::metadata

USERVER_NAMESPACE_END
