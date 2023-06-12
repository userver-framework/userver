#pragma once

#include <storages/mysql/impl/metadata/semver.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::metadata {

struct NativeClientInfo final {
  std::string client_version;
  SemVer client_version_id;

  static NativeClientInfo Get();

 private:
  NativeClientInfo(const char* client_version, std::uint64_t client_version_id);
};

}  // namespace storages::mysql::impl::metadata

USERVER_NAMESPACE_END
