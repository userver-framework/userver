#include <storages/mysql/impl/metadata/native_client_info.hpp>

#include <storages/mysql/impl/mariadb_include.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::metadata {

NativeClientInfo NativeClientInfo::Get() {
  return NativeClientInfo(mysql_get_client_info(), mysql_get_client_version());
}

NativeClientInfo::NativeClientInfo(const char* client_version,
                                   std::uint64_t client_version_id)
    : client_version{client_version}, client_version_id{client_version_id} {}

}  // namespace storages::mysql::impl::metadata

USERVER_NAMESPACE_END
