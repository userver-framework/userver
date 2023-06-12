#include <userver/utils/statistics/testing.hpp>

#include <atomic>

#include <userver/utest/utest.hpp>
#include <userver/utils/statistics/storage.hpp>

USERVER_NAMESPACE_BEGIN

UTEST(Snapshot, Printable) {
  utils::statistics::Storage storage;
  utils::statistics::Snapshot snap1{storage};

  EXPECT_TRUE(true) << snap1;
}

USERVER_NAMESPACE_END
