#include <boost/algorithm/string.hpp>

#include <logging/logging_test.hpp>
#include <tracing/no_log_spans.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/tracing/opentracing.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/tracer.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

class JsonSpan : public LoggingJsonTest {};

class OpentracingJsonSpan : public JsonSpan {
 protected:
  void SetUp() override {
    opentracing_data_ = std::make_shared<LoggingSinkWithStream>();
    opentracing_logger_ = MakeNamedStreamLogger(
        "openstracing", opentracing_data_, logging::Format::kJson);
    tracing::SetOpentracingLogger(opentracing_logger_);

    // Discard logs from SetOpentracingLogger
    logging::LogFlush(opentracing_logger_);
    opentracing_data_->sstream.str({});

    JsonSpan::SetUp();
  }

  void TearDown() override {
    JsonSpan::TearDown();
    if (opentracing_logger_) {
      tracing::SetOpentracingLogger({});
      opentracing_logger_.reset();
    }
  }

 private:
  std::shared_ptr<LoggingSinkWithStream> opentracing_data_;
  std::shared_ptr<logging::impl::TpLogger> opentracing_logger_;
};

UTEST_F(JsonSpan, DocsData) {
  {
    tracing::Span span("big block");
    span.AddTag("tag", "tag value");
    span.AddTagFrozen("frozen", "frozen tag value");
    span.AddNonInheritableTag("local", "local tag value");
  }

  logging::LogFlush();
  const std::string str = GetStreamString();
  EXPECT_NE(str.find(R"("timestamp":)"), std::string::npos) << str;
  EXPECT_NE(str.find(R"("level":)"), std::string::npos) << str;
  EXPECT_NE(str.find("module:"), std::string::npos) << str;
  EXPECT_NE(str.find("task_id:"), std::string::npos) << str;
  EXPECT_NE(str.find("thread_id:"), std::string::npos) << str;
  EXPECT_NE(str.find("text:"), std::string::npos) << str;
  // check for span
  EXPECT_NE(str.find("big block"), std::string::npos) << str;
  EXPECT_NE(str.find("tag:"), std::string::npos) << str;
  EXPECT_NE(str.find("tag value"), std::string::npos) << str;
  EXPECT_NE(str.find("frozen:"), std::string::npos) << str;
  EXPECT_NE(str.find("frozen tag value"), std::string::npos) << str;
  EXPECT_NE(str.find("local:"), std::string::npos) << str;
  EXPECT_NE(str.find("local tag value"), std::string::npos) << str;

  ASSERT_NO_THROW(formats::json::FromString(str));
}

USERVER_NAMESPACE_END