#include <gtest/gtest-typed-test.h>

#include <cstring>
#include <fstream>
#include <sstream>

USERVER_NAMESPACE_BEGIN

template <class T>
struct Serialization : public ::testing::Test {};
TYPED_TEST_SUITE_P(Serialization);

TYPED_TEST_P(Serialization, StringToString) {
  auto&& doc = this->FromString(this->kDoc);

  auto str = ToString(doc);
  EXPECT_EQ(str.size(), std::strlen(this->kDoc));
  EXPECT_EQ(str, ToString(doc));
  EXPECT_TRUE(str.find("key1") != std::string::npos);
  EXPECT_TRUE(str.find("key2") != std::string::npos);
  EXPECT_TRUE(str.find("val") != std::string::npos);
}

TYPED_TEST_P(Serialization, StreamToString) {
  std::istringstream is(this->kDoc);
  auto&& doc = this->FromStream(is);

  auto str = ToString(doc);
  EXPECT_EQ(str.size(), std::strlen(this->kDoc));
  EXPECT_EQ(str, ToString(doc));
  EXPECT_TRUE(str.find("key1") != std::string::npos);
  EXPECT_TRUE(str.find("key2") != std::string::npos);
  EXPECT_TRUE(str.find("val") != std::string::npos);
}

TYPED_TEST_P(Serialization, StringToStream) {
  auto&& doc = this->FromString(this->kDoc);
  std::ostringstream os;
  Serialize(doc, os);
  const auto str = os.str();
  EXPECT_TRUE(str.find("key1") != std::string::npos);
  EXPECT_TRUE(str.find("key2") != std::string::npos);
  EXPECT_TRUE(str.find("val") != std::string::npos);
}

TYPED_TEST_P(Serialization, StreamReadException) {
  using BadStreamException = typename TestFixture::BadStreamException;

  std::fstream is("some-missing-doc");
  // possible false positive because of conditional in catch?
  // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
  EXPECT_THROW(this->FromStream(is), BadStreamException);
}

TYPED_TEST_P(Serialization, StreamWriteException) {
  using BadStreamException = typename TestFixture::BadStreamException;

  auto&& doc = this->FromString(this->kDoc);
  std::ostringstream os;
  os.setstate(std::ios::failbit);
  // possible false positive because of conditional in catch?
  // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
  EXPECT_THROW(Serialize(doc, os), BadStreamException);
}

TYPED_TEST_P(Serialization, ParsingException) {
  using ParseException = typename TestFixture::ParseException;

  // Differs from JSON
  // possible false positive because of conditional in catch?
  // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
  EXPECT_THROW(this->FromString("&"), ParseException);
}

TYPED_TEST_P(Serialization, EmptyDocException) {
  using ParseException = typename TestFixture::ParseException;
  // possible false positive because of conditional in catch?
  // NOLINTNEXTLINE(misc-throw-by-value-catch-by-reference)
  EXPECT_THROW(this->FromString(""), ParseException);
}

REGISTER_TYPED_TEST_SUITE_P(Serialization,

                            StringToString, StreamToString, StringToStream,
                            StreamReadException, StreamWriteException,
                            ParsingException, EmptyDocException);

USERVER_NAMESPACE_END
