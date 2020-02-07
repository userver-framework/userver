#include <boost/algorithm/string.hpp>

#include <logging/logging_test.hpp>
#include <tracing/noop.hpp>
#include <tracing/span.hpp>
#include <utest/utest.hpp>

class Span : public LoggingTest {};

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
