#include <boost/algorithm/string.hpp>

#include <formats/json/serialize.hpp>
#include <logging/logging_test.hpp>
#include <tracing/noop.hpp>
#include <tracing/opentracing.hpp>
#include <tracing/span.hpp>
#include <utest/utest.hpp>

class Span : public LoggingTest {};

class OpentracingSpan : public Span {
 protected:
  void SetUp() override {
    opentracing_sstream.str(std::string());
    opentracing_logger_ = MakeStreamLogger(opentracing_sstream);
    tracing::SetOpentracingLogger(opentracing_logger_);
    Span::SetUp();
  }

  void TearDown() override {
    Span::TearDown();
    if (opentracing_logger_) {
      tracing::SetOpentracingLogger({});
      opentracing_logger_.reset();
    }
  }

  void FlushOpentracing() { opentracing_logger_->flush(); }

  static void CheckTagFormat(const formats::json::Value& tag) {
    EXPECT_TRUE(tag.HasMember("key"));
    EXPECT_TRUE(tag.HasMember("value"));
    EXPECT_TRUE(tag.HasMember("type"));
  }

  static void CheckTagTypeAndValue(const formats::json::Value& tag,
                                   const std::string& type,
                                   const std::string& value) {
    EXPECT_EQ(type, tag["type"].As<std::string>());
    EXPECT_EQ(value, tag["value"].As<std::string>());
  }

  static formats::json::Value GetTagsJson(const std::string& log_output) {
    auto tags_start = log_output.find("tags=");
    if (tags_start == std::string::npos) return {};
    tags_start += 5;
    auto tags_str = log_output.substr(tags_start);
    auto const tags_end = tags_str.find(']');
    if (tags_end == std::string::npos) return {};
    return formats::json::FromString(tags_str.substr(0, tags_end + 1));
  }

  std::ostringstream opentracing_sstream;

 private:
  logging::LoggerPtr opentracing_logger_;
};

TEST_F(Span, Ctr) {
  RunInCoro([this] {
    {
      logging::LogFlush();
      EXPECT_EQ(std::string::npos, sstream.str().find("stopwatch_name="));

      tracing::Span span("span_name");
      logging::LogFlush();
      EXPECT_EQ(std::string::npos, sstream.str().find("stopwatch_name="));
    }

    logging::LogFlush();
    EXPECT_NE(std::string::npos,
              sstream.str().find("stopwatch_name=span_name"));
  });
}

TEST_F(Span, Tag) {
  RunInCoro([this] {
    {
      tracing::Span span("span_name");
      span.AddTag("k", "v");

      logging::LogFlush();
      EXPECT_EQ(std::string::npos, sstream.str().find("k=v"));
    }
    logging::LogFlush();
    EXPECT_NE(std::string::npos, sstream.str().find("k=v"));
  });
}

TEST_F(Span, InheritTag) {
  RunInCoro([this] {
    tracing::Span span("span_name");
    tracing::Span::CurrentSpan().AddTag("k", "v");

    logging::LogFlush();
    EXPECT_EQ(std::string::npos, sstream.str().find("k=v"));

    tracing::Span span2("subspan");
    LOG_INFO() << "inside";

    logging::LogFlush();
    EXPECT_NE(std::string::npos, sstream.str().find("k=v"));
  });
}

TEST_F(Span, NonInheritTag) {
  RunInCoro([this] {
    tracing::Span span("span_name");

    tracing::Span::CurrentSpan().AddNonInheritableTag("k", "v");
    LOG_INFO() << "inside";
    logging::LogFlush();

    EXPECT_EQ(std::string::npos, sstream.str().find("k=v"));
  });
}

TEST_F(OpentracingSpan, Tags) {
  RunInCoro([this] {
    {
      tracing::Span span("span_name");
      span.AddTag("k", "v");
      span.AddTag("meta_code", 200);
      span.AddTag("error", false);
      span.AddTag("method", "POST");
      span.AddTag("db.type", "postgres");
      span.AddTag("db.statement", "SELECT * ");
      span.AddTag("peer.address", "127.0.0.1:8080");
      span.AddTag("http.url", "http://example.com/example");
    }
    FlushOpentracing();
    auto log_str = opentracing_sstream.str();
    EXPECT_EQ(std::string::npos, log_str.find("k=v"));
    EXPECT_NE(std::string::npos, log_str.find("http.status_code"));
    EXPECT_NE(std::string::npos, log_str.find("error"));
    EXPECT_NE(std::string::npos, log_str.find("http.method"));
    EXPECT_NE(std::string::npos, log_str.find("db.type"));
    EXPECT_NE(std::string::npos, log_str.find("db.statement"));
    EXPECT_NE(std::string::npos, log_str.find("peer.address"));
    EXPECT_NE(std::string::npos, log_str.find("http.url"));
  });
}

TEST_F(OpentracingSpan, TagFormat) {
  RunInCoro([this] {
    {
      tracing::Span span("span_name");
      span.AddTag("meta_code", 200);
      span.AddTag("error", false);
      span.AddTag("method", "POST");
    }
    FlushOpentracing();
    auto tags = GetTagsJson(opentracing_sstream.str());
    EXPECT_EQ(3, tags.GetSize());
    for (const auto& tag : tags) {
      CheckTagFormat(tag);
      const auto key = tag["key"].As<std::string>();
      if (key == "http.status_code") {
        CheckTagTypeAndValue(tag, "int64", "200");
      } else if (key == "error") {
        CheckTagTypeAndValue(tag, "bool", "0");
      } else if (key == "http.method") {
        CheckTagTypeAndValue(tag, "string", "POST");
      } else {
        FAIL() << "Got unknown key in tags: " << key;
      }
    }
  });
}

TEST_F(Span, ScopeTime) {
  RunInCoro([this] {
    {
      tracing::Span span("span_name");
      auto st = span.CreateScopeTime("xxx");

      logging::LogFlush();
      EXPECT_EQ(std::string::npos, sstream.str().find("xxx"));
    }

    logging::LogFlush();
    EXPECT_NE(std::string::npos, sstream.str().find("xxx_time="));
  });
}

TEST_F(Span, InTest) {
  RunInCoro([] { tracing::Span::CurrentSpan().AddTag("1", 2); });
}

TEST_F(Span, LocalLogLevel) {
  RunInCoro([this] {
    {
      tracing::Span span("span_name");

      LOG_INFO() << "info1";
      logging::LogFlush();
      EXPECT_NE(std::string::npos, sstream.str().find("info1"));

      span.SetLocalLogLevel(::logging::Level::kWarning);
      LOG_INFO() << "info2";
      LOG_WARNING() << "warning2";
      logging::LogFlush();
      EXPECT_EQ(std::string::npos, sstream.str().find("info2"));
      EXPECT_NE(std::string::npos, sstream.str().find("warning2"));

      {
        tracing::Span span("span2");

        LOG_WARNING() << "warning3";
        LOG_INFO() << "info3";
        logging::LogFlush();
        EXPECT_NE(std::string::npos, sstream.str().find("warning3"));
        EXPECT_EQ(std::string::npos, sstream.str().find("info3"));
      }
    }

    LOG_INFO() << "info4";
    logging::LogFlush();
    EXPECT_NE(std::string::npos, sstream.str().find("info4"));
  });
}

TEST_F(Span, LowerLocalLogLevel) {
  RunInCoro([this] {
    tracing::Span span("parent_span");
    span.SetLocalLogLevel(::logging::Level::kError);

    {
      tracing::Span span("logged_span");
      span.SetLocalLogLevel(::logging::Level::kInfo);
      span.AddTag("test_tag", "test_value1");

      LOG_INFO() << "simplelog";
      logging::LogFlush();
      EXPECT_NE(std::string::npos, sstream.str().find("simplelog"));
    }

    logging::LogFlush();
    EXPECT_NE(std::string::npos, sstream.str().find("logged_span"));
  });
}

TEST_F(Span, ConstructFromTracer) {
  RunInCoro([this] {
    auto tracer = tracing::MakeNoopTracer();

    tracing::Span span(tracer, "name", nullptr, tracing::ReferenceType::kChild);
    span.SetLink("some_link");

    LOG_INFO() << "tracerlog";
    logging::LogFlush();
    EXPECT_NE(std::string::npos, sstream.str().find("tracerlog"));

    EXPECT_EQ(tracing::Span::CurrentSpanUnchecked(), &span);
  });
}

TEST_F(Span, ForeignSpan) {
  RunInCoro([this] {
    auto tracer = tracing::MakeNoopTracer();

    tracing::Span local_span(tracer, "local", nullptr,
                             tracing::ReferenceType::kChild);
    local_span.SetLink("local_link");

    {
      tracing::Span foreign_span(tracer, "foreign", nullptr,
                                 tracing::ReferenceType::kChild);
      foreign_span.SetLink("foreign_link");

      auto st = foreign_span.CreateScopeTime("from_foreign_span");
    }

    LOG_INFO() << "tracerlog";

    logging::LogFlush();

    auto logs_raw = sstream.str();

    std::vector<std::string> logs;
    boost::algorithm::split(logs, logs_raw, boost::is_any_of("\n"));

    bool found_sw = false;
    bool found_tr = false;

    for (const auto& log : logs) {
      if (log.find("stopwatch_name=foreign") != std::string::npos) {
        found_sw = true;
        EXPECT_NE(std::string::npos, log.find("link=foreign_link"));
      }

      // check unlink
      if (log.find("tracerlog") != std::string::npos) {
        found_tr = true;
        EXPECT_NE(std::string::npos, log.find("link=local_link"));
      }
    }

    EXPECT_TRUE(found_sw);
    EXPECT_TRUE(found_tr);
  });
}
