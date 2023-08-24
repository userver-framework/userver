#include <gtest/gtest.h>

#include <algorithm>
#include <string>

#include <boost/algorithm/string/predicate.hpp>

#include <engine/io/impl/buffer.hpp>

USERVER_NAMESPACE_BEGIN

static const std::string kSmallString = "test";
static const std::string kLargeString(1024 * 1024, 'A');

using Buffer = engine::io::impl::Buffer;

TEST(Buffer, Ctor) { Buffer buffer; }

TEST(Buffer, Reserve) {
  Buffer buffer(0);
  ASSERT_LE(buffer.AvailableWriteBytes(), 1024 * 1024);
  buffer.Reserve(1);
  buffer.Reserve(1);
  buffer.Reserve(1);
  ASSERT_GT(buffer.AvailableWriteBytes(), 0);
  buffer.Reserve(1024 * 1024);
  ASSERT_GE(buffer.AvailableWriteBytes(), 1024 * 1024);
}

TEST(Buffer, Smoke) {
  Buffer buffer;
  buffer.Reserve(2 * kSmallString.size());
  const size_t initial_size = buffer.AvailableWriteBytes();
  ASSERT_GE(initial_size, 2 * kSmallString.size());
  EXPECT_EQ(0, buffer.AvailableReadBytes());
  std::copy(kSmallString.begin(), kSmallString.end(), buffer.WritePtr());
  buffer.ReportWritten(kSmallString.size());
  EXPECT_EQ(kSmallString.size(), buffer.AvailableReadBytes());
  EXPECT_EQ(initial_size - kSmallString.size(), buffer.AvailableWriteBytes());
  EXPECT_STREQ(buffer.ReadPtr(), kSmallString.c_str());

  std::copy(kSmallString.begin(), kSmallString.end(), buffer.WritePtr());
  buffer.ReportWritten(kSmallString.size());
  EXPECT_EQ(2 * kSmallString.size(), buffer.AvailableReadBytes());
  EXPECT_EQ(initial_size - 2 * kSmallString.size(),
            buffer.AvailableWriteBytes());
  EXPECT_TRUE(boost::starts_with(buffer.ReadPtr(), kSmallString));

  buffer.ReportRead(kSmallString.size());
  EXPECT_EQ(kSmallString.size(), buffer.AvailableReadBytes());
  EXPECT_TRUE(boost::starts_with(buffer.ReadPtr(), kSmallString));

  EXPECT_LT(buffer.AvailableWriteBytes(), kLargeString.size());
  buffer.Reserve(kLargeString.size());
  ASSERT_GE(buffer.AvailableWriteBytes(), kLargeString.size());
  std::copy(kLargeString.begin(), kLargeString.end(), buffer.WritePtr());
  buffer.ReportWritten(kLargeString.size());
  EXPECT_EQ(kSmallString.size() + kLargeString.size(),
            buffer.AvailableReadBytes());
  EXPECT_TRUE(boost::starts_with(buffer.ReadPtr(), kSmallString));
  buffer.ReportRead(kSmallString.size());
  EXPECT_EQ(kLargeString.size(), buffer.AvailableReadBytes());
  EXPECT_TRUE(boost::starts_with(buffer.ReadPtr(), kLargeString));
  buffer.ReportRead(kLargeString.size());
  EXPECT_EQ(0, buffer.AvailableReadBytes());
}

USERVER_NAMESPACE_END
