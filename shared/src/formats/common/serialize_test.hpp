#include <gtest/gtest-typed-test.h>

#include <fstream>
#include <sstream>

USERVER_NAMESPACE_BEGIN

template <class T>
struct Serialization : public ::testing::Test {};
TYPED_TEST_SUITE_P(Serialization);

TYPED_TEST_P(Serialization, StringToString) {
  auto&& doc = this->FromString(this->kDoc);
  EXPECT_EQ(ToString(doc), this->kDoc);
}

TYPED_TEST_P(Serialization, StreamToString) {
  std::istringstream is(this->kDoc);
  auto&& doc = this->FromStream(is);
  EXPECT_EQ(ToString(doc), this->kDoc);
}

TYPED_TEST_P(Serialization, StringToStream) {
  auto&& doc = this->FromString(this->kDoc);
  std::ostringstream os;
  Serialize(doc, os);
  EXPECT_EQ(os.str(), this->kDoc);
}

TYPED_TEST_P(Serialization, StreamReadException) {
  using BadStreamException = typename TestFixture::BadStreamException;

  std::fstream is("some-missing-doc");
  EXPECT_THROW(this->FromStream(is), BadStreamException);
}

TYPED_TEST_P(Serialization, StreamWriteException) {
  using BadStreamException = typename TestFixture::BadStreamException;

  auto&& doc = this->FromString(this->kDoc);
  std::ostringstream os;
  os.setstate(std::ios::failbit);
  EXPECT_THROW(Serialize(doc, os), BadStreamException);
}

TYPED_TEST_P(Serialization, ParsingException) {
  using ParseException = typename TestFixture::ParseException;

  // Differs from JSON
  EXPECT_THROW(this->FromString("&"), ParseException);
}

TYPED_TEST_P(Serialization, EmptyDocException) {
  using ParseException = typename TestFixture::ParseException;
  EXPECT_THROW(this->FromString(""), ParseException);
}

REGISTER_TYPED_TEST_SUITE_P(Serialization,

                            StringToString, StreamToString, StringToStream,
                            StreamReadException, StreamWriteException,
                            ParsingException, EmptyDocException);

USERVER_NAMESPACE_END
