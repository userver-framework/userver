#include <userver/utils/statistics/testing.hpp>

#include <userver/utest/utest.hpp>
#include <userver/utils/statistics/storage.hpp>

USERVER_NAMESPACE_BEGIN

UTEST(Snapshot, Printable) {
  const utils::statistics::Storage storage;
  const utils::statistics::Snapshot snapshot{storage};

  EXPECT_TRUE(true) << testing::PrintToString(snapshot);
}

USERVER_NAMESPACE_END
