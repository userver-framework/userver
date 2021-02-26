#include <cache/dump/operations_mock.hpp>

#include <utility>

#include <cache/dump/unsafe.hpp>
#include <utest/utest.hpp>

TEST(CacheDumpOperationsMock, WriteReadRaw) {
  constexpr std::size_t kMaxLength = 10;
  std::size_t total_length = 0;

  cache::dump::MockWriter writer;
  for (std::size_t i = 0; i <= kMaxLength; ++i) {
    WriteStringViewUnsafe(writer, std::string(i, 'a'));
    total_length += i;
  }
  std::string data = std::move(writer).Extract();

  EXPECT_EQ(data, std::string(total_length, 'a'));

  cache::dump::MockReader reader(std::move(data));
  for (std::size_t i = 0; i <= kMaxLength; ++i) {
    EXPECT_EQ(ReadStringViewUnsafe(reader, i), std::string(i, 'a'));
  }
  reader.Finish();
}

TEST(CacheDumpOperationsMock, EmptyDump) {
  cache::dump::MockWriter writer;
  std::string data = std::move(writer).Extract();

  EXPECT_EQ(data, "");

  cache::dump::MockReader reader(std::move(data));
  reader.Finish();
}

TEST(CacheDumpOperationsMock, EmptyStringDump) {
  cache::dump::MockWriter writer;
  WriteStringViewUnsafe(writer, {});
  std::string data = std::move(writer).Extract();

  EXPECT_EQ(data, "");

  cache::dump::MockReader reader(std::move(data));
  EXPECT_EQ(ReadStringViewUnsafe(reader, 0), "");
  reader.Finish();
}

TEST(CacheDumpOperationsMock, Overread) {
  cache::dump::MockReader reader(std::string(10, 'a'));
  EXPECT_THROW(ReadStringViewUnsafe(reader, 11), cache::dump::Error);
}

TEST(CacheDumpOperationsMock, Underread) {
  cache::dump::MockReader reader(std::string(10, 'a'));
  EXPECT_EQ(ReadStringViewUnsafe(reader, 9), std::string(9, 'a'));
  EXPECT_THROW(reader.Finish(), cache::dump::Error);
}
