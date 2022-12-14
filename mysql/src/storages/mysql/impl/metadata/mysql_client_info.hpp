#pragma once

#include <storages/mysql/impl/metadata/mysql_semver.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::metadata {

struct MySQLClientInfo final {
  std::string client_version;
  MySQLSemVer client_version_id;

  static MySQLClientInfo Get();

 private:
  MySQLClientInfo(const char* client_version, std::uint64_t client_version_id);
};

}  // namespace storages::mysql::impl::metadata

USERVER_NAMESPACE_END
