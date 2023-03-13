#include <logging/log_message_test.hpp>

#include <map>
#include <numeric>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <boost/filesystem/path.hpp>

#include <logging/logging_test.hpp>
#include <logging/rate_limit.hpp>
#include <userver/decimal64/decimal64.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/utils/traceful_exception.hpp>
#include <utils/encoding/tskv_testdata_bin.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

template <typename T>
std::string ToStringViaStreams(const T& value) {
  std::ostringstream oss;
  oss << value;
  return oss.str();
}

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

struct CountingStruct {
  mutable uint64_t logged_count = 0;

  friend logging::LogHelper& operator<<(
      logging::LogHelper& lh, [[maybe_unused]] const CountingStruct& self) {
    ++self.logged_count;
    return lh << "(CountingStruct)";
  }
};

template <int LogAttempts>
int CountLimitedLoggedTimes() {
  CountingStruct cs;
  for (int i = 0; i < LogAttempts; ++i) {
    LOG_LIMITED_CRITICAL() << cs;
  }
  return cs.logged_count;
}

}  // namespace

TEST_F(LoggingTest, TskvEncode) {
  EXPECT_EQ(ToStringViaLogging("line 1\nline 2"), "line 1\\nline 2")
      << "escaped sequence is present in the message";
}

TEST_F(LoggingTest, TskvEncodeKeyWithDot) {
  logging::LogExtra le;
  le.Extend("http.port.ipv4", "4040");
  LOG_CRITICAL() << "line 1\nline 2" << le;
  EXPECT_THAT(GetStreamString(),
              testing::HasSubstr("line 1\\nline 2\thttp_port_ipv4=4040"));
}

TEST_F(LoggingTest, FloatingPoint) {
  constexpr float f = 3.1415F;
  EXPECT_EQ(ToStringViaLogging(f), ToStringViaStreams(f));

  constexpr double d = 3.1415;
  EXPECT_EQ(ToStringViaLogging(d), ToStringViaStreams(d));

  constexpr long double ld = 3.1415L;
  EXPECT_EQ(ToStringViaLogging(ld), ToStringViaStreams(ld));
}

TEST_F(LoggingTest, NegativeValue) { EXPECT_EQ(ToStringViaLogging(-1), "-1"); }

TEST_F(LoggingTest, MinValue) {
  constexpr auto val = std::numeric_limits<long long>::min();
  EXPECT_EQ(ToStringViaLogging(val), ToStringViaStreams(val));
}

struct UserStructure {};
logging::LogHelper& operator<<(logging::LogHelper& os, UserStructure /*val*/) {
  os << logging::HexShort{0xFF161300};
  os << '\t';
  void* p = nullptr;
  os << logging::Hex{p};
  return os;
}

TEST_F(LoggingTest, UserStruct) {
  EXPECT_EQ(ToStringViaLogging(UserStructure{}),
            "FF161300\\t0x0000000000000000");
}

TEST_F(LoggingTest, Pointer) {
  const auto* const pointer = reinterpret_cast<int*>(0xDEADBEEF);
  EXPECT_EQ(ToStringViaLogging(pointer), "0x00000000DEADBEEF");
}

TEST_F(LoggingTest, NullPointer) {
  const char* const p1 = nullptr;
  EXPECT_EQ(ToStringViaLogging(p1), "(null)");

  const int* const p2 = nullptr;
  EXPECT_EQ(ToStringViaLogging(p2), "(null)");
}

TEST_F(LoggingTest, Decimal) {
  using Dec4 = decimal64::Decimal<4>;
  EXPECT_EQ(ToStringViaLogging(Dec4{"42"}), "42");
  EXPECT_EQ(ToStringViaLogging(Dec4{"12.3"}), "12.3");
}

TEST_F(LoggingTest, PlainException) {
  const std::string kWhat = "test exception";
  EXPECT_EQ(ToStringViaLogging(std::runtime_error(kWhat)),
            kWhat + " (std::runtime_error)");
}

TEST_F(LoggingTest, TracefulExceptionDebug) {
  logging::SetDefaultLoggerLevel(logging::Level::kDebug);

  LOG_CRITICAL() << utils::TracefulException("traceful exception");

  EXPECT_THAT(GetStreamString(), testing::HasSubstr("traceful exception"))
      << "traceful exception is missing its message";
  EXPECT_THAT(GetStreamString(), testing::HasSubstr("\tstacktrace="))
      << "traceful exception is missing its trace";
}

TEST_F(LoggingTest, TracefulExceptionInfo) {
  LOG_CRITICAL() << utils::TracefulException("traceful exception");

  EXPECT_TRUE(LoggedTextContains("traceful exception"))
      << "traceful exception is missing its message";
  EXPECT_FALSE(LoggedTextContains("\tstacktrace="))
      << "traceful exception should not have trace";
}

TEST_F(LoggingTest, AttachedException) {
  logging::SetDefaultLoggerLevel(logging::Level::kDebug);

  try {
    throw utils::impl::AttachTraceToException(
        std::logic_error("plain exception"))
        << " with additional info";
  } catch (const std::exception& ex) {
    LOG_CRITICAL() << ex;
  }

  EXPECT_TRUE(LoggedTextContains("plain exception"))
      << "missing plain exception message";
  EXPECT_TRUE(LoggedTextContains("plain exception with additional info"))
      << "traceful exception message malformed";
  EXPECT_THAT(GetStreamString(), testing::HasSubstr("\tstacktrace="))
      << "traceful exception missing its trace";
}

TEST_F(LoggingTest, IfExpressionWithoutBraces) {
  bool true_flag = true;
  if (true_flag)
    LOG(logging::Level::kNone) << "test";
  else
    FAIL() << "Logging affected the else statement";

  {
    bool passed = false;
    if (true_flag)
      LOG_LIMITED_CRITICAL() << (passed = true);
    else
      FAIL() << "Logging affected the else statement";
    EXPECT_TRUE(passed);
  }

  {
    bool passed = false;
    if (!true_flag)
      LOG_LIMITED_CRITICAL() << "test";
    else
      passed = true;
    EXPECT_TRUE(passed);
  }

  {
    bool passed = false;
    if (true_flag) LOG_LIMITED_CRITICAL() << (passed = true);
    EXPECT_TRUE(passed);
  }

  {
    bool passed = true;
    if (!true_flag) LOG_LIMITED_CRITICAL() << (passed = false);
    EXPECT_TRUE(passed);
  }
}

TEST_F(LoggingTest, CppModulePath) {
  LOG_CRITICAL();
  logging::LogFlush();
  CheckModulePath(GetStreamString(), "core/src/logging/log_message_test.cpp");
}

TEST_F(LoggingTest, HppModulePath) {
  LoggingHeaderFunction();
  logging::LogFlush();
  CheckModulePath(GetStreamString(), "core/src/logging/log_message_test.hpp");
}

TEST_F(LoggingTest, ExternalModulePath) {
  static const std::string kPath = "/somewhere_else/src/test.cpp";

  {
    logging::LogHelper a(logging::impl::DefaultLoggerRef(),
                         logging::Level::kCritical, kPath.c_str(), __LINE__,
                         __func__);
  }
  logging::LogFlush();

  CheckModulePath(GetStreamString(), kPath);
}

TEST_F(LoggingTest, LogHelperNullptr) {
  static const std::string kPath = "/somewhere_else/src/test.cpp";

  // LogHelper must survive nullptr
  logging::LogHelper(logging::LoggerPtr{}, logging::Level::kCritical,
                     kPath.c_str(), __LINE__, __func__)
          .AsLvalue()
      << "Test";
  logging::LogFlush();

  EXPECT_EQ(GetStreamString(), "");
}

TEST_F(LoggingTest, LogHelperNullLogger) {
  static const std::string kPath = "/somewhere_else/src/test.cpp";

  // logging::impl::DefaultLoggerRef() may return NullLogger
  logging::LogHelper(logging::GetNullLogger(), logging::Level::kCritical,
                     kPath.c_str(), __LINE__, __func__)
          .AsLvalue()
      << "Test";
  logging::LogFlush();

  EXPECT_EQ(GetStreamString(), "");
}

TEST_F(LoggingTest, PartialPrefixModulePath) {
  static const std::string kRealPath = __FILE__;
  static const std::string kPath =
      kRealPath.substr(0, kRealPath.find('/', 1) + 1) +
      "somewhere_else/src/test.cpp";

  {
    logging::LogHelper a(logging::impl::DefaultLoggerRef(),
                         logging::Level::kCritical, kPath.c_str(), __LINE__,
                         __func__);
  }
  logging::LogFlush();

  CheckModulePath(GetStreamString(), kPath);
}

TEST_F(LoggingTest, LogExtraTAXICOMMON1362) {
  const char* str = reinterpret_cast<const char*>(tskv_test::data_bin);
  std::string input(str, str + sizeof(tskv_test::data_bin));

  LOG_CRITICAL() << input;
  logging::LogFlush();
  std::string result = GetStreamString();

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

TEST_F(LoggingTest, TAXICOMMON1362) {
  const char* str = reinterpret_cast<const char*>(tskv_test::data_bin);
  std::string input(str, str + sizeof(tskv_test::data_bin));

  LOG_CRITICAL() << logging::LogExtra{{"body", input}};
  logging::LogFlush();
  std::string result = GetStreamString();

  const auto ascii_pos = result.find(tskv_test::ascii_part);
  EXPECT_TRUE(ascii_pos != std::string::npos) << "Result: " << result;

  const auto body_pos = result.find("body=");
  EXPECT_TRUE(body_pos != std::string::npos) << "Result: " << result;

  EXPECT_EQ(result.substr(body_pos, ascii_pos - body_pos)
                .find_first_of({'\n', '\t', '\0'}),
            std::string::npos)
      << "Result: " << result;
}

TEST_F(LoggingTest, Ranges) {
  const std::vector<std::set<int>> nested = {{1, 2, 3}, {4}, {5, 6}};
  EXPECT_EQ(ToStringViaLogging(nested), "[[1, 2, 3], [4], [5, 6]]");

  const std::unordered_set<int> set = {42};
  EXPECT_EQ(ToStringViaLogging(set), "[42]");
}

TEST_F(LoggingTest, CustomRanges) {
  struct MyRange {
    std::vector<int> impl;
    auto begin() const { return impl.begin(); }
    auto end() const { return impl.end(); }
  };
  const MyRange range = {{1, 2, 3}};

  EXPECT_EQ(ToStringViaLogging(range), "[1, 2, 3]");
}

TEST_F(LoggingTest, Maps) {
  const std::map<int, std::string> map1 = {{1, "a"}, {2, "b"}, {3, "c"}};
  EXPECT_EQ(ToStringViaLogging(map1), R"([1: "a", 2: "b", 3: "c"])");

  const std::unordered_map<int, std::string> map2 = {{42, "a"}};
  EXPECT_EQ(ToStringViaLogging(map2), R"([42: "a"])");
}

namespace logging {

template <typename T, typename U>
logging::LogHelper& operator<<(logging::LogHelper& lh,
                               const std::pair<T, U>& value) {
  lh << "(" << value.first << ", " << value.second << ")";
  return lh;
}

}  // namespace logging

TEST_F(LoggingTest, CustomMaps) {
  struct MyMap {
    std::vector<std::pair<const std::string, int>> impl;

    using key_type [[maybe_unused]] = std::string;
    using mapped_type [[maybe_unused]] = int;

    auto begin() const { return impl.begin(); }
    auto end() const { return impl.end(); }
  };
  const MyMap map{{{"b", 1}, {"a", 2}}};
  EXPECT_EQ(ToStringViaLogging(map), R"(["b": 1, "a": 2])");

  struct MyPseudoMap {
    std::vector<std::pair<const std::string, int>> impl;

    auto begin() const { return impl.begin(); }
    auto end() const { return impl.end(); }
  };
  const MyPseudoMap pseudo_map{{{"b", 1}, {"a", 2}}};
  EXPECT_EQ(ToStringViaLogging(pseudo_map), R"([(b, 1), (a, 2)])");
}

TEST_F(LoggingTest, RangeOverflow) {
  std::vector<int> range(10000);
  std::iota(range.begin(), range.end(), 0);

  LOG_CRITICAL() << range;
  EXPECT_TRUE(LoggedTextContains("1850, 1851, ...8148 more"))
      << "Actual logged text: " << LoggedText();
}

TEST_F(LoggingTest, ImmediateRangeOverflow) {
  const std::string filler(100000, 'A');
  std::vector<int> range{42};

  LOG_CRITICAL() << filler << range;
  EXPECT_TRUE(LoggedTextContains("[...1 more]"))
      << "Actual logged text: " << LoggedText();
}

TEST_F(LoggingTest, NestedRangeOverflow) {
  std::vector<std::vector<std::string>> range{
      {"1", "2", "3"}, {std::string(100000, 'A'), "4", "5"}, {"6", "7"}};

  LOG_CRITICAL() << range;
  EXPECT_TRUE(LoggedTextContains(R"([["1", "2", "3"], ["AAA)"))
      << "Actual logged text: " << LoggedText();
  EXPECT_TRUE(LoggedTextContains(R"(AAA...", ...2 more], ...1 more])"))
      << "Actual logged text: " << LoggedText();
}

TEST_F(LoggingTest, MapOverflow) {
  std::map<int, int> map;
  for (int i = 0; i < 10000; ++i) {
    map.emplace(i, i);
  }

  LOG_CRITICAL() << map;
  EXPECT_TRUE(LoggedTextContains("[0: 0, 1: 1,"))
      << "Actual logged text: " << LoggedText();
  EXPECT_TRUE(LoggedTextContains("1017: 1017, 1018: 1018, ...8981 more]"))
      << "Actual logged text: " << LoggedText();
}

TEST_F(LoggingTest, FilesystemPath) {
  EXPECT_EQ(ToStringViaLogging(boost::filesystem::path("a/b/c")), R"("a/b/c")");
}

TEST_F(LoggingTest, StringEscaping) {
  std::vector<std::string> range{"", "A", "\"", "\\", "\n"};
  EXPECT_EQ(ToStringViaLogging(range), R"(["", "A", "\\"", "\\\\", "\n"])");
}

TEST_F(LoggingTest, Json) {
  const auto* const json_str = R"({"a":"b","c":1,"d":0.25,"e":[],"f":{}})";
  EXPECT_EQ(ToStringViaLogging(formats::json::FromString(json_str)), json_str);
}

TEST_F(LoggingTest, LogLimited) {
  EXPECT_EQ(CountLimitedLoggedTimes<1>(), 1);
  EXPECT_EQ(CountLimitedLoggedTimes<2>(), 2);
  EXPECT_EQ(CountLimitedLoggedTimes<3>(), 2);
  EXPECT_EQ(CountLimitedLoggedTimes<4>(), 3);
  EXPECT_EQ(CountLimitedLoggedTimes<5>(), 3);
  EXPECT_EQ(CountLimitedLoggedTimes<6>(), 3);
  EXPECT_EQ(CountLimitedLoggedTimes<7>(), 3);
  EXPECT_EQ(CountLimitedLoggedTimes<8>(), 4);
  EXPECT_EQ(CountLimitedLoggedTimes<1024>(), 11);
}

TEST_F(LoggingTest, LogLimitedDisabled) {
  logging::impl::SetLogLimitedEnable(false);
  EXPECT_EQ(CountLimitedLoggedTimes<1024>(), 1024);
  logging::impl::SetLogLimitedEnable(true);
}

TEST_F(LoggingTest, CustomLoggerLevel) {
  auto sstream_data = std::make_shared<LoggingSinkWithStream>();
  auto logger = MakeNamedStreamLogger("other-logger", sstream_data,
                                      logging::Format::kTskv);

  // LOG_*_TO() must use its own log level, not default logger's one
  logging::SetDefaultLoggerLevel(logging::Level::kCritical);
  logging::SetLoggerLevel(*logger, logging::Level::kInfo);

  LOG_INFO_TO(*logger) << "test";
  LOG_LIMITED_INFO_TO(*logger) << "mest";
  LOG_DEBUG_TO(*logger) << "tost";
  LOG_LIMITED_DEBUG_TO(*logger) << "most";
  logging::LogFlush(*logger);

  auto result = sstream_data->sstream.str();
  EXPECT_NE(result.find("test"), std::string::npos);
  EXPECT_NE(result.find("mest"), std::string::npos);
  EXPECT_EQ(result.find("tost"), std::string::npos);
  EXPECT_EQ(result.find("most"), std::string::npos);
}

TEST_F(LoggingTest, NullLogger) {
  LOG_INFO_TO(logging::GetNullLogger()) << "1";
  logging::GetNullLogger().SetLevel(logging::Level::kTrace);
  LOG_INFO_TO(logging::GetNullLogger()) << "2";
  EXPECT_FALSE(logging::GetNullLogger().ShouldLog(logging::Level::kInfo));

  EXPECT_THAT(GetStreamString(), testing::Not(testing::HasSubstr("text=1")));
  EXPECT_THAT(GetStreamString(), testing::Not(testing::HasSubstr("text=2")));
}

TEST_F(LoggingTest, NullLoggerPtr) {
  auto ptr = logging::MakeNullLogger();
  LOG_INFO_TO(ptr) << "1";
  ptr->SetLevel(logging::Level::kTrace);
  LOG_INFO_TO(ptr) << "2";
  EXPECT_FALSE(ptr->ShouldLog(logging::Level::kInfo));

  EXPECT_THAT(GetStreamString(), testing::Not(testing::HasSubstr("text=1")));
  EXPECT_THAT(GetStreamString(), testing::Not(testing::HasSubstr("text=2")));
}

TEST_F(LoggingTest, Noexceptness) {
  static_assert(noexcept(logging::ShouldLog(logging::Level::kCritical)));
  static_assert(noexcept(logging::impl::Noop{}));
  static_assert(noexcept(logging::impl::DefaultLoggerRef()));
  static_assert(noexcept(logging::LogExtra()));
  static_assert(noexcept(logging::LogExtra::Stacktrace()));
  static_assert(noexcept(logging::LogHelper(
      logging::GetNullLogger(), logging::Level::kCritical, {}, 1, {})));

  if constexpr (noexcept(std::string_view{"some string"})) {
    EXPECT_TRUE(noexcept(
        logging::LogHelper(logging::GetNullLogger(), logging::Level::kCritical,
                           USERVER_FILEPATH, __LINE__, __func__)));

    EXPECT_TRUE(noexcept(
        logging::LogHelper(logging::LoggerPtr{}, logging::Level::kCritical,
                           USERVER_FILEPATH, __LINE__, __func__)));

    EXPECT_TRUE(noexcept(LOG_TRACE_TO(logging::GetNullLogger())));
    EXPECT_TRUE(noexcept(LOG_DEBUG_TO(logging::GetNullLogger())));
    EXPECT_TRUE(noexcept(LOG_INFO_TO(logging::GetNullLogger())));
    EXPECT_TRUE(noexcept(LOG_WARNING_TO(logging::GetNullLogger())));
    EXPECT_TRUE(noexcept(LOG_ERROR_TO(logging::GetNullLogger())));
    EXPECT_TRUE(noexcept(LOG_CRITICAL_TO(logging::GetNullLogger())));

    /*
    EXPECT_TRUE(noexcept(LOG_TRACE_TO(logging::GetNullLogger()) << "Test"));
    EXPECT_TRUE(noexcept(LOG_DEBUG_TO(logging::GetNullLogger()) << "Test"));
    EXPECT_TRUE(noexcept(LOG_INFO_TO(logging::GetNullLogger()) << "Test"));
    EXPECT_TRUE(noexcept(LOG_WARNING_TO(logging::GetNullLogger()) << "Test"));
    EXPECT_TRUE(noexcept(LOG_ERROR_TO(logging::GetNullLogger()) << "Test"));
    EXPECT_TRUE(noexcept(LOG_CRITICAL_TO(logging::GetNullLogger()) << "Test"));
    */

    using logging::LogExtra;
    EXPECT_TRUE(noexcept(LOG_TRACE_TO(logging::GetNullLogger())
                         << LogExtra::Stacktrace()));
    EXPECT_TRUE(noexcept(LOG_DEBUG_TO(logging::GetNullLogger())
                         << LogExtra::Stacktrace()));
    EXPECT_TRUE(noexcept(LOG_INFO_TO(logging::GetNullLogger())
                         << LogExtra::Stacktrace()));
    EXPECT_TRUE(noexcept(LOG_WARNING_TO(logging::GetNullLogger())
                         << LogExtra::Stacktrace()));
    EXPECT_TRUE(noexcept(LOG_ERROR_TO(logging::GetNullLogger())
                         << LogExtra::Stacktrace()));
    EXPECT_TRUE(noexcept(LOG_CRITICAL_TO(logging::GetNullLogger())
                         << LogExtra::Stacktrace()));
  }
}

USERVER_NAMESPACE_END
