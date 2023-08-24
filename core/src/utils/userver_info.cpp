#include <userver/utils/userver_info.hpp>

#include <build_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

const char* GetUserverVersion() noexcept { return USERVER_VERSION; }

const char* GetUserverVcsRevision() noexcept { return USERVER_HASH; }

const char* GetUserverIdentifier() noexcept {
  return "userver/" USERVER_VERSION " (" USERVER_BUILD_TIME "; rv:" USERVER_HASH
         ")";
}

}  // namespace utils

USERVER_NAMESPACE_END
