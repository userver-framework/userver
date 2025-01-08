#include <userver/utest/utest.hpp>

#include <exception>

#include <userver/storages/sqlite.hpp>
#include <userver/storages/sqlite/tests/utils.hpp>
#include "userver/storages/sqlite/result_set.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::tests {

// Here we want to test the operation of the ResultSet itself (iterator
// invariants, iteration, row access, container conversion and conversion
// container elements into the correct types, including user-defined types).
// All this can be done without being tied to the way we getting the ResultSet

// Iterators invariants

// using SQLiteBase = SQLiteTestFixture<MockSQLiteConnection>;

// UTEST_F(SQLiteBase, EmptyResult) {
//   EXPECT_CALL(*GetMockConnection(), Execute())
//       .Times(1)
//       .WillOnce(testing::Return(ResultSet{}));

//   auto res = Execute();
//   ASSERT_EQ(0, res.Size());
//   EXPECT_THROW(res[0], std::out_of_range);
//   EXPECT_THROW(res.Front(), std::out_of_range);
//   EXPECT_THROW(res.Back(), std::out_of_range);
// }

}  // namespace storages::sqlite::tests

USERVER_NAMESPACE_END
