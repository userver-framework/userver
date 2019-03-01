#include <gtest/gtest.h>

#include <memory>
#include <sstream>
#include <stdexcept>

#include <logging/logging_test.hpp>
#include <utils/traceful_exception.hpp>

TEST_F(LoggingTest, TskvEncode) {
  LOG_CRITICAL() << "line 1\nline 2";
  logging::LogFlush();

  EXPECT_NE(std::string::npos, sstream.str().find("\\n"))
      << "escaped sequence is present in the message";
}

TEST_F(LoggingTest, TskvEncodeKeyWithDot) {
  logging::LogExtra le;
  le.Extend("http.port.ipv4", "4040");
  LOG_CRITICAL() << "line 1\nline 2" << le;
  logging::LogFlush();

  EXPECT_NE(std::string::npos, sstream.str().find("\\n"))
      << "escaped sequence is present in the message";

  EXPECT_NE(std::string::npos, sstream.str().find("http_port_ipv4"))
      << "periods were not escaped";
}

TEST_F(LoggingTest, FloatingPoint) {
  constexpr float f = 3.1415f;
  LOG_CRITICAL() << f;
  logging::LogFlush();

  std::ostringstream oss;
  oss << f;

  EXPECT_NE(std::string::npos, sstream.str().find(oss.str()))
      << f << " was represented as " << sstream.str();
  sstream.str({});
  oss.str({});

  constexpr double d = 3.1415;
  LOG_CRITICAL() << d;
  logging::LogFlush();
  oss << d;

  EXPECT_NE(std::string::npos, sstream.str().find(oss.str()))
      << d << " was represented as " << sstream.str();
  sstream.str({});
  oss.str({});

  constexpr long double ld = 3.1415;
  LOG_CRITICAL() << ld;
  logging::LogFlush();
  oss << ld;

  EXPECT_NE(std::string::npos, sstream.str().find(oss.str()))
      << ld << " was represented as " << sstream.str();
  sstream.str({});
  oss.str({});
}

TEST_F(LoggingTest, NegativeValue) {
  LOG_CRITICAL() << -1;
  logging::LogFlush();

  EXPECT_NE(std::string::npos, sstream.str().find("-1"))
      << "-1 was represented as " << sstream.str();
}

TEST_F(LoggingTest, MinValue) {
  constexpr auto val = std::numeric_limits<long long>::min();
  LOG_CRITICAL() << val;
  logging::LogFlush();

  std::ostringstream oss;
  oss << val;

  EXPECT_NE(std::string::npos, sstream.str().find(oss.str()))
      << val << " was represented as " << sstream.str();
}

struct UserStructure {};
logging::LogHelper& operator<<(logging::LogHelper& os, UserStructure /*val*/) {
  os.PutHexShort(0xFF161300);
  os << '\t';
  void* p = nullptr;
  os.PutHex(p);
  return os;
}

TEST_F(LoggingTest, UserStruct) {
  LOG_CRITICAL() << UserStructure{};
  logging::LogFlush();

  EXPECT_NE(std::string::npos, sstream.str().find("FF161300\\t0x00000000"))
      << "FF161300\\t0x00000000 was represented as " << sstream.str();
}

TEST_F(LoggingTest, PlainException) {
  LOG_CRITICAL() << std::runtime_error("test exception");
  logging::LogFlush();

  EXPECT_NE(std::string::npos, sstream.str().find("test exception"))
      << "exception was not printed correctly";
}

TEST_F(LoggingTest, TracefulException) {
  LOG_CRITICAL() << utils::TracefulException("traceful exception");
  logging::LogFlush();

  EXPECT_NE(std::string::npos, sstream.str().find("traceful exception"))
      << "traceful exception missing its message";
  EXPECT_NE(std::string::npos, sstream.str().find("\tstacktrace="))
      << "traceful exception missing its trace";
}

TEST_F(LoggingTest, AttachedException) {
  try {
    throw utils::impl::AttachTraceToException(
        std::logic_error("plain exception"))
        << " with additional info";
  } catch (const std::exception& ex) {
    LOG_CRITICAL() << ex;
  }
  logging::LogFlush();

  EXPECT_NE(std::string::npos, sstream.str().find("plain exception"))
      << "missing plain exception message";
  EXPECT_NE(std::string::npos,
            sstream.str().find("plain exception with additional info"))
      << "traceful exception message malformed";
  EXPECT_NE(std::string::npos, sstream.str().find("\tstacktrace="))
      << "traceful exception missing its trace";
}

TEST_F(LoggingTest, IfExpressionWithoutBraces) {
  if (true)
    LOG(::logging::Level::kNone) << "test";
  else
    FAIL() << "Logging affected the else statement";
}
