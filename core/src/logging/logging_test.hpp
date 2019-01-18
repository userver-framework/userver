#include <gtest/gtest.h>

#include <logging/spdlog.hpp>

#include <spdlog/sinks/ostream_sink.h>

#include <logging/log.hpp>

class LoggingTest : public ::testing::Test {
 protected:
  void SetUp() override {
    sstream.str(std::string());
    old_ = logging::SetDefaultLogger(MakeStreamLogger());
  }
  void TearDown() override {
    if (old_) {
      logging::SetDefaultLogger(old_);
      old_.reset();
    }
  }
  logging::LoggerPtr MakeStreamLogger() {
    std::ostringstream os;
    os << this;
    auto sink = std::make_shared<spdlog::sinks::ostream_sink_st>(sstream);
    return std::make_shared<spdlog::logger>(os.str(), sink);
  }
  std::ostringstream sstream;

 private:
  logging::LoggerPtr old_;
};
