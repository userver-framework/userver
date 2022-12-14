#include <storages/mysql/assert_when_implemented.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

void AssertWhenImplemented([[maybe_unused]] bool is_implemented,
                           [[maybe_unused]] std::string_view what) {
#ifdef USERVER_MYSQL_ASSERT_WHEN_IMPLEMENTED
  UINVARIANT(!is_implemented,
             fmt::format("Set USERVER_MYSQL_ASSERT_WHEN_IMPLEMENTED=0 to "
                         "disable this assert.\n"
                         "'{}' got implemented in mariadb, which the driver "
                         "ideally should support.\n"
                         "If you see this message please file an issue to "
                         "notify maintainers.\n"));
#endif
}

}  // namespace storages::mysql

USERVER_NAMESPACE_END
