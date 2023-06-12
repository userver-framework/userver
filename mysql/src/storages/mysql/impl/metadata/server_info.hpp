#pragma once

#include <storages/mysql/impl/metadata/semver.hpp>

#include <storages/mysql/impl/mariadb_include.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::metadata {

struct ServerInfo final {
  enum class Type { kMySQL, kMariaDB, kUnknown };

  std::string server_type_str;
  SemVer server_version{0};
  Type server_type{Type::kUnknown};

  static ServerInfo Get(MYSQL& mysql);
};

}  // namespace storages::mysql::impl::metadata

USERVER_NAMESPACE_END
