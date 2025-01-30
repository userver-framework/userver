#include <userver/utest/utest.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <userver/storages/sqlite.hpp>
#include <userver/storages/sqlite/tests/utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::tests {

// Here we check the high-level operation of transactions; this requires a test
// connection to the database

// TODO: Add tests on transactions

UTEST(Transaction, OK) { EXPECT_TRUE(true); }

}  // namespace storages::sqlite::tests

USERVER_NAMESPACE_END
