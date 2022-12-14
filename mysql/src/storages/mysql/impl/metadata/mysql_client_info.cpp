#include <storages/mysql/impl/metadata/mysql_client_info.hpp>

#include <storages/mysql/impl/mariadb_include.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::metadata {

MySQLClientInfo MySQLClientInfo::Get() {
  return MySQLClientInfo(mysql_get_client_info(), mysql_get_client_version());
}

MySQLClientInfo::MySQLClientInfo(const char* client_version,
                                 std::uint64_t client_version_id)
    : client_version{client_version}, client_version_id{client_version_id} {}

}  // namespace storages::mysql::impl::metadata

USERVER_NAMESPACE_END
