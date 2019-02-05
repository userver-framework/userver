#include <logging/logging_test.hpp>
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
    tracing::Span::CurrentSpan()->AddTag("k", "v");

    logging::LogFlush();
    EXPECT_EQ(std::string::npos, sstream.str().find("k=v"));

    tracing::Span span2("subspan");
    LOG_INFO() << "inside";

    logging::LogFlush();
    EXPECT_NE(std::string::npos, sstream.str().find("k=v"));
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
  RunInCoro([] { tracing::Span::CurrentSpan()->AddTag("1", 2); });
}
