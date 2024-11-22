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

// Определяем конкретный тест на основе фикстуры и использующей мока
using SQLiteBase = SQLiteTestFixture<MockSQLiteConnection>;

UTEST_F(SQLiteBase, EmptyResult) {
  EXPECT_CALL(*GetMockConnection(), Execute())
      .Times(1)
      .WillOnce(testing::Return(ResultSet{}));

  auto res = Execute();
  ASSERT_EQ(0, res.Size());
  EXPECT_THROW(res[0], std::out_of_range);
  EXPECT_THROW(res.Front(), std::out_of_range);
  EXPECT_THROW(res.Back(), std::out_of_range);
  // EXPECT_TRUE(res.begin() == res.end());
  // EXPECT_TRUE(res.cbegin() == res.cend());
  // EXPECT_TRUE(res.rbegin() == res.rend());
  // EXPECT_TRUE(res.crbegin() == res.crend());
}

// UTEST_P(SQLiteConnection, ResultEmptyRow) {
//   ResultSet res =
//       Execute();  // TODO: mock non-empty ResultSet with one empty row [()]

//   ASSERT_EQ(1, res.Size());
//   UASSERT_NO_THROW(res[0]);
//   UEXPECT_NO_THROW(res.Front());
//   UEXPECT_NO_THROW(res.Back());
//   // EXPECT_FALSE(res.begin() == res.end());
//   // EXPECT_FALSE(res.cbegin() == res.cend());
//   // EXPECT_FALSE(res.rbegin() == res.rend());
//   // EXPECT_FALSE(res.crbegin() == res.crend());

//   // ASSERT_EQ(0, res[0].Size());
//   // UEXPECT_THROW(res[0][0], std::exception);
//   // EXPECT_TRUE(res[0].begin() == res[0].end());
//   // EXPECT_TRUE(res[0].cbegin() == res[0].cend());
//   // EXPECT_TRUE(res[0].rbegin() == res[0].rend());
//   // EXPECT_TRUE(res[0].crbegin() == res[0].crend());
// }

// UTEST_P(SQLiteConnection, ResultOobAccess) {
//   ResultSet res = Execute();  // TODO: mock non-empty ResultSet with one
//                               // non-empty row [(...)]

//   ASSERT_EQ(1, res.Size());
//   UASSERT_NO_THROW(res[0]);
//   UEXPECT_NO_THROW(res.Front());
//   UEXPECT_NO_THROW(res.Back());
//   // EXPECT_FALSE(res.begin() == res.end());
//   // EXPECT_FALSE(res.cbegin() == res.cend());
//   // EXPECT_FALSE(res.rbegin() == res.rend());
//   // EXPECT_FALSE(res.crbegin() == res.crend());
//   // EXPECT_EQ(++res.begin(), res.end());
//   // EXPECT_EQ(++res.cbegin(), res.cend());
//   // EXPECT_EQ(++res.crbegin(), res.crend());
//   // EXPECT_EQ(++res.rbegin(), res.rend());
//   // EXPECT_EQ(res.cend() - res.cbegin(), 1);
//   // EXPECT_EQ(res.crend() - res.crbegin(), 1);

//   // ASSERT_EQ(1, res[0].Size());
//   // UEXPECT_NO_THROW(res[0][0]);
//   // EXPECT_FALSE(res[0].begin() == res[0].end());
//   // EXPECT_FALSE(res[0].cbegin() == res[0].cend());
//   // EXPECT_FALSE(res[0].rbegin() == res[0].rend());
//   // EXPECT_FALSE(res[0].crbegin() == res[0].crend());
//   // EXPECT_EQ(++res[0].begin(), res[0].end());
//   // EXPECT_EQ(++res[0].cbegin(), res[0].cend());
//   // EXPECT_EQ(++res[0].crbegin(), res[0].crend());
//   // EXPECT_EQ(++res[0].rbegin(), res[0].rend());
//   // EXPECT_EQ(res[0].cend() - res[0].cbegin(), 1);
//   // EXPECT_EQ(res[0].crend() - res[0].crbegin(), 1);

//   // UEXPECT_THROW(res[1], pg::RowIndexOutOfBounds);
//   // UEXPECT_THROW(res[0][1], pg::FieldIndexOutOfBounds);
// }

// UTEST_P(SQLiteConnection, ResultTraverseForward) {
//   ResultSet res = Execute();  // TODO: mock non-empty ResultSet with two
//                               // non-empty row [({}, {}), ({}, {})]

//   ASSERT_EQ(2, res.Size());

//   int num = 1;
//   // for (auto row_it = res.cbegin(); row_it != res.cend(); ++row_it) {
//   //   for (auto col_it = row_it->cbegin(); col_it != row_it->cend();
//   ++col_it)
//   //   {
//   //     EXPECT_EQ(col_it->As<int>(), num++);
//   //   }
//   // }
//   EXPECT_EQ(5, num);
// }

// UTEST_P(SQLiteConnection, ResultTraverseBackward) {
//   ResultSet res = Execute();  // TODO: mock non-empty ResultSet with two
//                               // non-empty row [({}, {}), ({}, {})]

//   ASSERT_EQ(2, res.Size());

//   int num = 1;
//   // for (auto row_it = res.crbegin(); row_it != res.crend(); ++row_it) {
//   //   for (auto col_it = row_it->crbegin(); col_it != row_it->crend();
//   //   ++col_it) {
//   //     EXPECT_EQ(col_it->As<int>(), num++);
//   //   }
//   // }
//   EXPECT_EQ(5, num);
// }

// UTEST_P(SQLiteConnection, ResultAsOptionalSingleRow) {
//   ResultSet res;

//   // Size() == 0
//   UEXPECT_NO_THROW(res = Execute());
//   ASSERT_TRUE(res.IsEmpty());

//   UEXPECT_NO_THROW(res.AsOptionalSingleRow<int>());
//   EXPECT_FALSE(res.AsOptionalSingleRow<int>().has_value());

//   // Size() == 1
//   UEXPECT_NO_THROW(res = Execute());
//   ASSERT_EQ(1, res.Size());

//   UEXPECT_NO_THROW(res.AsOptionalSingleRow<int>());
//   EXPECT_TRUE(res.AsOptionalSingleRow<int>().has_value());
//   EXPECT_TRUE(res.AsOptionalSingleRow<int>() == 1);
//   EXPECT_EQ(1, res.AsOptionalSingleRow<int>());

//   // Size() > 1
//   UEXPECT_NO_THROW(res = Execute());
//   ASSERT_EQ(2, res.Size());

//   UEXPECT_THROW(res.AsOptionalSingleRow<int>(), std::exception);
// }

// // Container conversations

// namespace {
// struct SampleRow final {
//   std::int32_t id;
//   std::string value;

//   bool operator<(const SampleRow& other) const {
//     return id < other.id || (id == other.id && value < other.value);
//   }
//   bool operator==(const SampleRow& other) const {
//     return id == other.id && value == other.value;
//   }
// };
// }  // namespace

// namespace as_vector_sample {

// // TODO: mock creating ResultSet with
// // expected_data schema

// UTEST_P(SQLiteConnection, AsVector) {
//   const std::vector<SampleRow> expected_data{{1, "first"}, {2, "second"}};
//   const auto db_rows = Execute().AsVector<SampleRow>();
//   static_assert(
//       std::is_same_v<const std::vector<SampleRow>, decltype(db_rows)>);
//   EXPECT_EQ(expected_data, db_rows);
// }

// }  // namespace as_vector_sample

// namespace as_vector_field_sample {

// UTEST_P(SQLiteConnection, AsVector) {
//   const std::vector<std::string> expected_data{"first", "second"};
//   const auto vector_of_field_values =
//       Execute().AsVector<std::string>(kFieldTag);

//   static_assert(std::is_same_v<const std::vector<std::string>,
//                                decltype(vector_of_field_values)>);
//   EXPECT_EQ(expected_data, vector_of_field_values);
// }

// }  // namespace as_vector_field_sample

// namespace as_single_row_sample {

// UTEST_P(SQLiteConnection, AsSingleRow) {
//   const SampleRow data = {1, "first"};
//   const auto db_row = Execute().AsSingleRow<SampleRow>();
//   EXPECT_EQ(data, db_row);
// }

// }  // namespace as_single_row_sample

// namespace as_single_field_sample {

// UTEST_P(SQLiteConnection, AsSingleField) {
//   const auto value = Execute().AsSingleRow<std::string>(kFieldTag);
//   EXPECT_EQ(value, "some value");
// }

// }  // namespace as_single_field_sample

// namespace as_container_sample {

// UTEST_P(SQLiteConnection, AsContainer) {
//   const std::vector<SampleRow> data = {{1, "first"}, {2, "second"}};
//   const auto set_or_rows = Execute().AsContainer<std::set<SampleRow>>();
//   static_assert(
//       std::is_same_v<const std::set<SampleRow>, decltype(set_or_rows)>);
//   const auto input_as_set = std::set<SampleRow>{data.begin(), data.end()};
//   EXPECT_EQ(set_or_rows, input_as_set);
// }

// }  // namespace as_container_sample

// namespace as_container_field_sample {

// UTEST_P(SQLiteConnection, AsContainerFieldTag) {
//   const auto set_of_values =
//       Execute().AsContainer<std::set<std::string>>(kFieldTag);

//   static_assert(
//       std::is_same_v<const std::set<std::string>, decltype(set_of_values)>);
//   ASSERT_EQ(set_of_values.size(), 1);
//   EXPECT_EQ(*set_of_values.begin(), "some value");
// }

// }  // namespace as_container_field_sample

// namespace as_optional_single_row_sample {

// UTEST_P(SQLiteConnection, AsOptionalSingleRow) {
//   const auto row_optional = Execute().AsOptionalSingleRow<SampleRow>();

//   static_assert(
//       std::is_same_v<const std::optional<SampleRow>,
//       decltype(row_optional)>);
//   EXPECT_FALSE(row_optional.has_value());
// }

// }  // namespace as_optional_single_row_sample

// namespace as_optional_single_field_sample {

// UTEST_P(SQLiteConnection, AsOptionalSingleField) {
//   const auto field_optional =
//       Execute().AsOptionalSingleRow<std::string>(kFieldTag);

//   static_assert(std::is_same_v<const std::optional<std::string>,
//                                decltype(field_optional)>);
//   EXPECT_FALSE(field_optional.has_value());
// }

// }  // namespace as_optional_single_field_sample

}  // namespace storages::sqlite::tests

USERVER_NAMESPACE_END
