#include <gtest/gtest.h>

#include <memory>
#include <sstream>

#include <logging/logging_test.hpp>

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
