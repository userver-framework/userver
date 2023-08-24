#include <userver/dump/operations_mock.hpp>

#include <utility>

#include <userver/dump/unsafe.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

TEST(CacheDumpOperationsMock, WriteReadRaw) {
  constexpr std::size_t kMaxLength = 10;
  std::size_t total_length = 0;

  dump::MockWriter writer;
  for (std::size_t i = 0; i <= kMaxLength; ++i) {
    WriteStringViewUnsafe(writer, std::string(i, 'a'));
    total_length += i;
  }
  std::string data = std::move(writer).Extract();

  EXPECT_EQ(data, std::string(total_length, 'a'));

  dump::MockReader reader(std::move(data));
  for (std::size_t i = 0; i <= kMaxLength; ++i) {
    EXPECT_EQ(ReadStringViewUnsafe(reader, i), std::string(i, 'a'));
  }
  reader.Finish();
}

TEST(CacheDumpOperationsMock, EmptyDump) {
  dump::MockWriter writer;
  std::string data = std::move(writer).Extract();

  EXPECT_EQ(data, "");

  dump::MockReader reader(std::move(data));
  reader.Finish();
}

TEST(CacheDumpOperationsMock, EmptyStringDump) {
  dump::MockWriter writer;
  WriteStringViewUnsafe(writer, {});
  std::string data = std::move(writer).Extract();

  EXPECT_EQ(data, "");

  dump::MockReader reader(std::move(data));
  EXPECT_EQ(ReadStringViewUnsafe(reader, 0), "");
  reader.Finish();
}

TEST(CacheDumpOperationsMock, Overread) {
  dump::MockReader reader(std::string(10, 'a'));
  UEXPECT_THROW(ReadStringViewUnsafe(reader, 11), dump::Error);
}

TEST(CacheDumpOperationsMock, Underread) {
  dump::MockReader reader(std::string(10, 'a'));
  EXPECT_EQ(ReadStringViewUnsafe(reader, 9), std::string(9, 'a'));
  UEXPECT_THROW(reader.Finish(), dump::Error);
}

USERVER_NAMESPACE_END
