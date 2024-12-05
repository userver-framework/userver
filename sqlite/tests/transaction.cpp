#include <userver/utest/utest.hpp>

#include <userver/storages/sqlite.hpp>
#include <userver/storages/sqlite/tests/utils.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::tests {

// Here we check the high-level operation of transactions; this requires a test connection to the database

UTEST(Transaction, OK) { EXPECT_TRUE(false); }

}  // namespace storages::sqlite::tests

USERVER_NAMESPACE_END
