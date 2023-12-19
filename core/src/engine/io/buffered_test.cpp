#include <gtest/gtest.h>

#include <cstdlib>
#include <deque>
#include <string>

#include <userver/engine/io/buffered.hpp>
#include <userver/engine/io/common.hpp>
#include <userver/utest/assert_macros.hpp>
#include <userver/utils/rand.hpp>

USERVER_NAMESPACE_BEGIN

using BufferedReader = engine::io::BufferedReader;
using ReadableBase = engine::io::ReadableBase;

class ReadableMock : public ReadableBase {
 public:
  bool IsValid() const override { return true; }

  bool WaitReadable(engine::Deadline) override { std::abort(); }

  size_t ReadSome(void* buf, size_t len, engine::Deadline) override {
    if (!len || buffer_.empty()) return 0;

    auto chars_to_copy = utils::RandRange(std::min(len, buffer_.size())) + 1;
    std::copy(buffer_.begin(), buffer_.begin() + chars_to_copy,
              reinterpret_cast<char*>(buf));
    buffer_.erase(buffer_.begin(), buffer_.begin() + chars_to_copy);
    return chars_to_copy;
  }

  size_t ReadAll(void*, size_t, engine::Deadline) override { std::abort(); }

  void Feed(const std::string& str) {
    buffer_.insert(buffer_.end(), str.begin(), str.end());
  }

 private:
  std::deque<char> buffer_;
};

TEST(BufferedReader, Ctor) {
  BufferedReader reader(std::make_shared<ReadableMock>());
  EXPECT_TRUE(reader.IsValid());
}

TEST(BufferedReader, ReadSome) {
  auto mock_ptr = std::make_shared<ReadableMock>();
  BufferedReader reader(mock_ptr);

  mock_ptr->Feed("test");
  EXPECT_TRUE(reader.ReadSome(0).empty());
  EXPECT_EQ("t", reader.ReadSome(1));
  EXPECT_EQ("e", reader.ReadSome(1));

  auto result = reader.ReadSome(2);
  result += reader.ReadSome(2);
  result += reader.ReadSome(2);
  EXPECT_EQ("st", result);
}

TEST(BufferedReader, ReadAll) {
  auto mock_ptr = std::make_shared<ReadableMock>();
  BufferedReader reader(mock_ptr);

  mock_ptr->Feed("test");
  EXPECT_EQ("tes", reader.ReadAll(3));
  mock_ptr->Feed("TEST");
  EXPECT_EQ("tTEST", reader.ReadAll(10));
}

TEST(BufferedReader, ReadLine) {
  auto mock_ptr = std::make_shared<ReadableMock>();
  BufferedReader reader(mock_ptr);

  mock_ptr->Feed("GET / HTTP/1.0\r\n\r\nblah\n\n");
  EXPECT_EQ("GET / HTTP/1.0", reader.ReadLine());
  EXPECT_EQ("blah", reader.ReadLine());
  mock_ptr->Feed("last line without termination");
  EXPECT_EQ("last line without termination", reader.ReadLine());
}

TEST(BufferedReader, ReadUntilChar) {
  auto mock_ptr = std::make_shared<ReadableMock>();
  BufferedReader reader(mock_ptr);

  mock_ptr->Feed("a;b,c");
  EXPECT_EQ("a;", reader.ReadUntil(';'));
  UEXPECT_THROW(reader.ReadUntil(';'), engine::io::TerminatorNotFoundException);
  EXPECT_EQ("b,", reader.ReadUntil(','));
}

TEST(BufferedReader, GetPeek) {
  auto mock_ptr = std::make_shared<ReadableMock>();
  BufferedReader reader(mock_ptr);

  mock_ptr->Feed("abc");
  EXPECT_EQ('a', reader.Getc());
  EXPECT_EQ('b', reader.Peek());
  EXPECT_EQ('b', reader.Getc());
  EXPECT_EQ('c', reader.Getc());
  EXPECT_EQ(EOF, reader.Getc());
  EXPECT_EQ(EOF, reader.Peek());
}

USERVER_NAMESPACE_END
