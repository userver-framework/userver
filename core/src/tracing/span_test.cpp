#include <boost/algorithm/string.hpp>

#include <logging/logging_test.hpp>
#include <tracing/no_log_spans.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/tracing/noop.hpp>
#include <userver/tracing/opentracing.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/tracer.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

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

UTEST_F(Span, Ctr) {
  {
    logging::LogFlush();
    EXPECT_EQ(std::string::npos, sstream.str().find("stopwatch_name="));

    tracing::Span span("span_name");
    logging::LogFlush();
    EXPECT_EQ(std::string::npos, sstream.str().find("stopwatch_name="));
  }

  logging::LogFlush();
  EXPECT_NE(std::string::npos, sstream.str().find("stopwatch_name=span_name"));
}

UTEST_F(Span, Tag) {
  {
    tracing::Span span("span_name");
    span.AddTag("k", "v");

    logging::LogFlush();
    EXPECT_EQ(std::string::npos, sstream.str().find("k=v"));
  }
  logging::LogFlush();
  EXPECT_NE(std::string::npos, sstream.str().find("k=v"));
}

UTEST_F(Span, InheritTag) {
  tracing::Span span("span_name");
  tracing::Span::CurrentSpan().AddTag("k", "v");

  logging::LogFlush();
  EXPECT_EQ(std::string::npos, sstream.str().find("k=v"));

  tracing::Span span2("subspan");
  LOG_INFO() << "inside";

  logging::LogFlush();
  EXPECT_NE(std::string::npos, sstream.str().find("k=v"));
}

UTEST_F(Span, NonInheritTag) {
  tracing::Span span("span_name");

  tracing::Span::CurrentSpan().AddNonInheritableTag("k", "v");
  LOG_INFO() << "inside";
  logging::LogFlush();

  EXPECT_EQ(std::string::npos, sstream.str().find("k=v"));
}

UTEST_F(OpentracingSpan, Tags) {
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
}

UTEST_F(OpentracingSpan, FromTracerWithServiceName) {
  auto tracer = tracing::MakeNoopTracer("test_service");
  {
    tracing::Span span(tracer, "span_name", nullptr,
                       tracing::ReferenceType::kChild);
  }
  FlushOpentracing();
  auto log_str = opentracing_sstream.str();
  EXPECT_NE(std::string::npos, log_str.find("service_name=test_service"));
}

UTEST_F(OpentracingSpan, TagFormat) {
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
}

UTEST_F(Span, ScopeTime) {
  {
    tracing::Span span("span_name");
    auto st = span.CreateScopeTime("xxx");

    logging::LogFlush();
    EXPECT_EQ(std::string::npos, sstream.str().find("xxx"));
  }

  logging::LogFlush();
  EXPECT_NE(std::string::npos, sstream.str().find("xxx_time="));
}

UTEST_F(Span, ScopeTimeDoesntOverrideTotalTime) {
  const int kSleepMs = 11;
  {
    tracing::Span span("span_name");
    engine::SleepFor(std::chrono::milliseconds(kSleepMs));
    {
      auto st = span.CreateScopeTime("xxx");
      engine::SleepFor(std::chrono::milliseconds(kSleepMs));
    }
  }

  const auto parse_timing = [](const std::string& str,
                               std::string_view name) -> std::optional<double> {
    auto start_pos = str.find(name);
    if (start_pos == std::string::npos) return std::nullopt;
    start_pos += name.size() + 1 /* '=' sign */;

    const auto finish_pos = str.find('\t', start_pos);

    if (finish_pos != std::string::npos)
      return std::stod(str.substr(start_pos, finish_pos - start_pos));
    else
      return std::stod(str.substr(start_pos));
  };

  logging::LogFlush();

  const auto xxx_time = parse_timing(sstream.str(), "xxx_time");
  const auto total_time = parse_timing(sstream.str(), "total_time");

  ASSERT_TRUE(xxx_time.has_value());
  ASSERT_TRUE(total_time.has_value());

  EXPECT_LE(xxx_time.value() + kSleepMs, total_time.value());
}

UTEST_F(Span, GetElapsedTime) {
  tracing::Span span("span_name");
  auto st = span.CreateScopeTime("xxx");
  st.Reset("yyy");

  auto unknown_elapsed = span.GetTotalElapsedTime("_unregistered_").count();
  auto abs_error = std::numeric_limits<double>::epsilon();

  // unregistered scope time should be zero
  EXPECT_NEAR(unknown_elapsed, tracing::Span::RealMilliseconds{0}.count(),
              abs_error);

  engine::SleepFor(std::chrono::milliseconds(2));

  // registered scope time should not be zero
  ASSERT_NE(span.GetTotalElapsedTime("xxx"),
            tracing::Span::RealMilliseconds{0});
}

UTEST_F(Span, InTest) { tracing::Span::CurrentSpan().AddTag("1", 2); }

UTEST_F(Span, LocalLogLevel) {
  {
    tracing::Span span("span_name");

    LOG_INFO() << "info1";
    logging::LogFlush();
    EXPECT_NE(std::string::npos, sstream.str().find("info1"));

    span.SetLocalLogLevel(logging::Level::kWarning);
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
}

UTEST_F(Span, LowerLocalLogLevel) {
  tracing::Span span("parent_span");
  span.SetLocalLogLevel(logging::Level::kError);

  {
    tracing::Span span("logged_span");
    span.SetLocalLogLevel(logging::Level::kInfo);
    span.AddTag("test_tag", "test_value1");

    LOG_INFO() << "simplelog";
    logging::LogFlush();
    EXPECT_NE(std::string::npos, sstream.str().find("simplelog"));
  }

  logging::LogFlush();
  EXPECT_NE(std::string::npos, sstream.str().find("logged_span"));
}

UTEST_F(Span, ConstructFromTracer) {
  auto tracer = tracing::MakeNoopTracer("test_service");

  tracing::Span span(tracer, "name", nullptr, tracing::ReferenceType::kChild);
  span.SetLink("some_link");

  LOG_INFO() << "tracerlog";
  logging::LogFlush();
  EXPECT_NE(std::string::npos, sstream.str().find("tracerlog"));

  EXPECT_EQ(tracing::Span::CurrentSpanUnchecked(), &span);
}

UTEST_F(Span, NoLogNames) {
  constexpr const char* kLogFirstSpan = "first_span_to_log";
  constexpr const char* kLogSecondSpan = "second_span_to_log";
  constexpr const char* kLogThirdSpan =
      "second_span_to_ignore_is_the_prefix_of_this_span";

  constexpr const char* kIgnoreFirstSpan = "first_span_to_ignore";
  constexpr const char* kIgnoreSecondSpan = "second_span_to_ignore";

  tracing::NoLogSpans no_logs;
  no_logs.names = {
      kIgnoreFirstSpan,
      kIgnoreSecondSpan,
  };
  tracing::Tracer::SetNoLogSpans(std::move(no_logs));

  {
    tracing::Span span0(kLogFirstSpan);
    tracing::Span span1(kIgnoreFirstSpan);
    tracing::Span span2(kLogSecondSpan);
    tracing::Span span3(kIgnoreSecondSpan);
    tracing::Span span4(kLogThirdSpan);
  }

  logging::LogFlush();

  EXPECT_NE(std::string::npos, sstream.str().find(kLogFirstSpan));
  EXPECT_EQ(std::string::npos, sstream.str().find(kIgnoreFirstSpan));
  EXPECT_NE(std::string::npos, sstream.str().find(kLogSecondSpan));
  EXPECT_EQ(std::string::npos,
            sstream.str().find(kIgnoreSecondSpan + std::string("\t")));
  EXPECT_NE(std::string::npos,
            sstream.str().find(kLogThirdSpan + std::string("\t")));

  tracing::Tracer::SetNoLogSpans(tracing::NoLogSpans());
}

UTEST_F(Span, NoLogPrefixes) {
  constexpr const char* kLogSpan0 = "first_span_to_log";
  constexpr const char* kLogSpan1 = "span_to_log_ignore_nolog_prefix";
  constexpr const char* kLogSpan2 = "span";
  constexpr const char* kLogSpan3 = "ign";

  const std::string kIgnorePrefix0 = "ignore_";
  const std::string kIgnorePrefix1 = "ignore1_";
  const std::string kIgnorePrefix2 = "ignore2_";

  const std::string kIgnoreSpan = "ignor5span";

  tracing::NoLogSpans no_logs;
  no_logs.prefixes = {
      kIgnorePrefix0,
      kIgnorePrefix1,
      kIgnorePrefix2,

      "ignor",
      "ignor0",
      "ignor1",
      "ignor2",
      "ignor3",
      "ignor4",
      // intentionally missing
      "ignor6",
      "ignor7",
      "ignor8",
      "ignor9",
  };
  tracing::Tracer::SetNoLogSpans(std::move(no_logs));

  tracing::Span{kIgnorePrefix0 + "foo"};
  tracing::Span{kLogSpan0};
  tracing::Span{kLogSpan1};
  tracing::Span{kIgnorePrefix2 + "XXX"};
  tracing::Span{kLogSpan2};
  tracing::Span{kIgnorePrefix1 + "74dfljzs"};
  tracing::Span{kIgnorePrefix0 + "bar"};
  tracing::Span{kLogSpan3};
  tracing::Span{kIgnorePrefix0};
  tracing::Span{kIgnorePrefix1};
  tracing::Span{kIgnorePrefix2};
  tracing::Span{kIgnoreSpan};

  logging::LogFlush();

  const auto output = sstream.str();
  EXPECT_NE(std::string::npos, output.find(kLogSpan0)) << output;
  EXPECT_NE(std::string::npos, output.find(kLogSpan1)) << output;
  EXPECT_NE(std::string::npos, output.find(kLogSpan2)) << output;
  EXPECT_NE(std::string::npos, output.find(kLogSpan3)) << output;

  EXPECT_EQ(std::string::npos, output.find("=" + kIgnorePrefix0)) << output;
  EXPECT_EQ(std::string::npos, output.find(kIgnorePrefix1)) << output;
  EXPECT_EQ(std::string::npos, output.find(kIgnorePrefix2)) << output;
  EXPECT_EQ(std::string::npos, output.find(kIgnoreSpan)) << output;

  tracing::Tracer::SetNoLogSpans(tracing::NoLogSpans());
}

UTEST_F(Span, NoLogMixed) {
  auto json = formats::json::FromString(R"({
        "names": ["i_am_a_span_to_ignore"],
        "prefixes": ["skip", "ignore", "skip", "do_not_keep", "skip", "skip"]
    })");
  auto no_logs = Parse(json, formats::parse::To<tracing::NoLogSpans>{});
  tracing::Tracer::SetNoLogSpans(std::move(no_logs));

  constexpr const char* kLogSpan0 = "first_span_to_log";
  constexpr const char* kLogSpan1 = "i_am_a_span_to_ignore(not!)";
  constexpr const char* kLogSpan2 = "span";
  constexpr const char* kLogSpan3 = "ign";
  constexpr const char* kLogSpan4 = "i_am_span";

  constexpr const char* kIgnoreSpan = "i_am_a_span_to_ignore";

  const std::string kIgnorePrefix0 = "ignore";
  const std::string kIgnorePrefix1 = "skip";
  const std::string kIgnorePrefix2 = "do_not_keep";

  tracing::Span{kIgnorePrefix0 + "oops"};
  tracing::Span{kLogSpan0};
  tracing::Span{kLogSpan1};
  tracing::Span{kIgnorePrefix2 + "I"};
  tracing::Span{kLogSpan2};
  tracing::Span{kIgnorePrefix1 + "did it"};
  tracing::Span{kIgnorePrefix0 + "again"};
  tracing::Span{kLogSpan3};
  tracing::Span{kLogSpan4};
  tracing::Span{kIgnorePrefix0};
  tracing::Span{kIgnorePrefix1};
  tracing::Span{kIgnorePrefix2};
  tracing::Span{kIgnoreSpan};

  logging::LogFlush();

  const auto output = sstream.str();
  EXPECT_NE(std::string::npos, output.find(kLogSpan0)) << output;
  EXPECT_NE(std::string::npos, output.find(kLogSpan1)) << output;
  EXPECT_NE(std::string::npos, output.find(kLogSpan2)) << output;
  EXPECT_NE(std::string::npos, output.find(kLogSpan3)) << output;
  EXPECT_NE(std::string::npos, output.find(kLogSpan4)) << output;

  EXPECT_EQ(std::string::npos, output.find("=" + kIgnorePrefix0)) << output;
  EXPECT_EQ(std::string::npos, output.find(kIgnorePrefix1)) << output;
  EXPECT_EQ(std::string::npos, output.find(kIgnorePrefix2)) << output;
  EXPECT_EQ(std::string::npos, output.find(kIgnoreSpan + std::string("\t")))
      << output;

  tracing::Tracer::SetNoLogSpans(tracing::NoLogSpans());
}

UTEST_F(Span, NoLogWithSetLogLevel) {
  constexpr const char* kIgnoreFirstSpan = "first_span_to_ignore";
  constexpr const char* kIgnoreSecondSpan = "second_span_to_ignore";

  tracing::NoLogSpans no_logs;
  no_logs.names = {
      kIgnoreFirstSpan,
      kIgnoreSecondSpan,
  };
  tracing::Tracer::SetNoLogSpans(std::move(no_logs));

  {
    tracing::Span span1(kIgnoreFirstSpan);
    span1.SetLogLevel(logging::Level::kError);
    span1.SetLocalLogLevel(logging::Level::kTrace);
  }

  logging::LogFlush();

  EXPECT_EQ(std::string::npos, sstream.str().find(kIgnoreFirstSpan));

  {
    tracing::Span span2(kIgnoreSecondSpan);
    span2.SetLogLevel(logging::Level::kInfo);
  }

  logging::LogFlush();

  EXPECT_EQ(std::string::npos, sstream.str().find(kIgnoreSecondSpan));

  tracing::Tracer::SetNoLogSpans(tracing::NoLogSpans());
}

UTEST_F(Span, ForeignSpan) {
  auto tracer = tracing::MakeNoopTracer("test_service");

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
}

UTEST_F(Span, DocsData) {
  {  /// [Example using Span tracing]
    tracing::Span span("big block");
    span.AddTag("tag", "simple tag that can be changed in subspan");
    span.AddTagFrozen("frozen",
                      "it is not possible to change this tag value in subspan");
    span.AddNonInheritableTag("local", "this tag is not visible in subspans");
    /// [Example using Span tracing]
  }
  {
    std::string user = "user";
    /// [Example span hierarchy]
    tracing::Span span("big block");
    span.AddTag("city", "moscow");

    LOG_INFO() << "User " << user << " logged in";  // logs "city" tag

    {
      tracing::Span span_local("small block");
      span_local.AddTag("request_id", 12345);

      LOG_INFO() << "Making request";  // logs "city", "request_id" tags
    }
    LOG_INFO() << "After request";  // logs "city", no "request_id"
    /// [Example span hierarchy]
  }
  {
    /// [Example get current span]
    tracing::Span::CurrentSpan().AddTag("key", "value");
    /// [Example get current span]
  }
}

USERVER_NAMESPACE_END
