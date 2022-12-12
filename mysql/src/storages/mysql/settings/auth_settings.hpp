#pragma once

#include <string>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::settings {

struct AuthSettings final {
  std::string dbname{"test"};
  std::string user{"test"};
  std::string password{"test"};
};

}  // namespace storages::mysql::settings

USERVER_NAMESPACE_END
