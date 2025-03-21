#include <userver/storages/sqlite/tests/utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::tests {

std::string TestParamNameJournalMode(
    const ::testing::TestParamInfo<
        ::userver::storages::sqlite::settings::SQLiteSettings::JournalMode>&
        info) {
  return JournalModeToString(info.param);
}

}  // namespace storages::sqlite::tests

USERVER_NAMESPACE_END
