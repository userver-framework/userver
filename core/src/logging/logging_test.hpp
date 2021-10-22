#include <gtest/gtest.h>

#include <logging/spdlog.hpp>

#include <spdlog/sinks/ostream_sink.h>

#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

class LoggingTest : public ::testing::Test {
 protected:
  void SetUp() override {
    sstream.str(std::string());
    old_ = logging::SetDefaultLogger(MakeStreamLogger(sstream));
  }
  void TearDown() override {
    if (old_) {
      logging::SetDefaultLogger(old_);
      old_.reset();
    }
  }
  logging::LoggerPtr MakeStreamLogger(std::ostream& stream) const {
    std::ostringstream os;
    os << this;
    auto sink = std::make_shared<spdlog::sinks::ostream_sink_st>(stream);
    return std::make_shared<spdlog::logger>(os.str(), sink);
  }
  std::ostringstream sstream;

  std::string LoggedText() const {
    logging::LogFlush();
    const std::string str = sstream.str();

    static constexpr std::string_view kTextKey = "text=";
    const auto text_begin = str.find(kTextKey);
    const std::string from_text =
        text_begin == std::string::npos
            ? str
            : str.substr(text_begin + kTextKey.length());

    const auto text_end = from_text.rfind('\n');
    return text_end == std::string::npos ? from_text
                                         : from_text.substr(0, text_end);
  }

  bool LoggedTextContains(const std::string& str) const {
    return LoggedText().find(str) != std::string::npos;
  }

  void ClearLog() { sstream.str({}); }

  template <typename T>
  std::string ToStringViaLogging(const T& value) {
    LOG_CRITICAL() << value;
    const std::string result = LoggedText();
    ClearLog();
    return result;
  }

 private:
  logging::LoggerPtr old_;
};

USERVER_NAMESPACE_END
