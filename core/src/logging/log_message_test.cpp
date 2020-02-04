#include <logging/log_message_test.hpp>

#include <gtest/gtest.h>

#include <memory>
#include <sstream>
#include <stdexcept>

#include <logging/logging_test.hpp>
#include <utils/encoding/tskv_testdata_bin.hpp>
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
      << f << " (oss=" << oss.str() << ") was represented as '" << sstream.str()
      << "'";
  sstream.str({});
  oss.str({});

  constexpr double d = 3.1415;
  LOG_CRITICAL() << d;
  logging::LogFlush();
  oss << d;

  EXPECT_NE(std::string::npos, sstream.str().find(oss.str()))
      << d << " (oss=" << oss.str() << ") was represented as '" << sstream.str()
      << "'";
  sstream.str({});
  oss.str({});

  constexpr long double ld = 3.1415;
  LOG_CRITICAL() << ld;
  logging::LogFlush();
  oss << ld;

  EXPECT_NE(std::string::npos, sstream.str().find(oss.str()))
      << ld << " (oss=" << oss.str() << ") was represented as '"
      << sstream.str() << "'";
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

namespace {

void CheckModulePath(const std::string& message, const std::string& expected) {
  static const std::string kUservicesUserverRoot = "userver/";

  auto module_pos = message.find("module=");
  ASSERT_NE(std::string::npos, module_pos) << "no module logged";
  auto path_pos = message.find("( " + expected + ':', module_pos);
  if (path_pos == std::string::npos) {
    path_pos =
        message.find("( " + kUservicesUserverRoot + expected + ':', module_pos);
  }
  auto delim_pos = message.find('\t', module_pos);
  ASSERT_LT(path_pos, delim_pos)
      << "module mismatch, expected path '" << expected << "', found '"
      << message.substr(module_pos, delim_pos - module_pos) << '\'';
}

}  // namespace

TEST_F(LoggingTest, CppModulePath) {
  LOG_CRITICAL();
  logging::LogFlush();

  CheckModulePath(sstream.str(), "core/src/logging/log_message_test.cpp");
}

TEST_F(LoggingTest, HppModulePath) {
  LoggingHeaderFunction();
  logging::LogFlush();

  CheckModulePath(sstream.str(), "core/src/logging/log_message_test.hpp");
}

TEST_F(LoggingTest, ExternalModulePath) {
  static const std::string kPath = "/somewhere_else/src/test.cpp";

  logging::LogHelper(logging::DefaultLogger(), logging::Level::kCritical,
                     kPath.c_str(), __LINE__, __func__);
  logging::LogFlush();

  CheckModulePath(sstream.str(), kPath);
}

TEST_F(LoggingTest, PartialPrefixModulePath) {
  static const std::string kRealPath = __FILE__;
  static const std::string kPath =
      kRealPath.substr(0, kRealPath.find('/', 1) + 1) +
      "somewhere_else/src/test.cpp";

  logging::LogHelper(logging::DefaultLogger(), logging::Level::kCritical,
                     kPath.c_str(), __LINE__, __func__);
  logging::LogFlush();

  CheckModulePath(sstream.str(), kPath);
}

TEST_F(LoggingTest, LogExtra_TAXICOMMON_1362) {
  const char* str = reinterpret_cast<const char*>(tskv_test::data_bin);
  std::string input(str, str + sizeof(tskv_test::data_bin));

  LOG_CRITICAL() << input;
  logging::LogFlush();
  std::string result = sstream.str();

  ASSERT_GT(result.size(), 1);
  EXPECT_EQ(result.back(), '\n');
  result.pop_back();

  EXPECT_TRUE(result.find(tskv_test::ascii_part) != std::string::npos)
      << "Result: " << result;

  const auto png_pos = result.find("PNG");
  EXPECT_TRUE(png_pos != std::string::npos) << "Result: " << result;

  EXPECT_EQ(0, std::count(result.begin() + png_pos, result.end(), '\n'))
      << "Result: " << result;
  EXPECT_EQ(0, std::count(result.begin() + png_pos, result.end(), '\t'))
      << "Result: " << result;
  EXPECT_EQ(0, std::count(result.begin() + png_pos, result.end(), '\0'))
      << "Result: " << result;
}

TEST_F(LoggingTest, TAXICOMMON_1362) {
  const char* str = reinterpret_cast<const char*>(tskv_test::data_bin);
  std::string input(str, str + sizeof(tskv_test::data_bin));

  LOG_CRITICAL() << logging::LogExtra{{"body", input}};
  logging::LogFlush();
  std::string result = sstream.str();

  const auto ascii_pos = result.find(tskv_test::ascii_part);
  EXPECT_TRUE(ascii_pos != std::string::npos) << "Result: " << result;

  const auto body_pos = result.find("body=");
  EXPECT_TRUE(body_pos != std::string::npos) << "Result: " << result;

  auto begin = result.begin() + body_pos;
  auto end = result.begin() + ascii_pos;
  EXPECT_EQ(0, std::count(begin, end, '\n')) << "Result: " << result;
  EXPECT_EQ(0, std::count(begin, end, '\t')) << "Result: " << result;
  EXPECT_EQ(0, std::count(begin, end, '\0')) << "Result: " << result;
}
