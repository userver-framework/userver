#include <fmt/format.h>
#include <gmock/gmock.h>

#include <logging/log_helper_impl.hpp>
#include <logging/logging_test.hpp>
#include <tracing/no_log_spans.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/tracer.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/regex.hpp>
#include <userver/utils/text_light.hpp>

using testing::HasSubstr;
using testing::Not;

USERVER_NAMESPACE_BEGIN

class Span : public LoggingTest {};

UTEST_F(Span, Ctr) {
  {
    logging::LogFlush();
    EXPECT_THAT(GetStreamString(), Not(HasSubstr("stopwatch_name=")));

    tracing::Span span("span_name");
    logging::LogFlush();
    EXPECT_THAT(GetStreamString(), Not(HasSubstr("stopwatch_name=")));
  }

  logging::LogFlush();
  EXPECT_THAT(GetStreamString(), HasSubstr("stopwatch_name=span_name"));
}

UTEST_F(Span, LogFormat) {
  // Note: this is a golden test. The order and content of tags is stable, which
  // is an implementation detail, but it makes this test possible. If the order
  // or content of tags change, this test should be fixed to reflect the
  // changes.
  constexpr std::string_view kExpectedPattern =
      R"(tskv\t)"
      R"(timestamp=\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\.\d{6}\t)"
      R"(level=[A-Z]+\t)"
      R"(module=[\w\d ():./]+\t)"
      R"(task_id=[0-9A-F]+\t)"
      R"(thread_id=0x[0-9A-F]+\t)"
      R"(text=\t)"
      R"(trace_id=[0-9a-f]+\t)"
      R"(span_id=[0-9a-f]+\t)"
      R"(parent_id=[0-9a-f]+\t)"
      R"(stopwatch_name=span_name\t)"
      R"(total_time=\d+(\.\d+)?\t)"
      R"(span_ref_type=child\t)"
      R"(stopwatch_units=ms\t)"
      R"(start_timestamp=\d+(\.\d+)?\t)"
      R"(my_timer_time=\d+(\.\d+)?\t)"
      R"(link=[0-9a-f]+\t)"
      R"(my_tag_key=my_tag_value\n)";
  {
    tracing::Span span("span_name");
    span.AddTag("my_tag_key", "my_tag_value");
    span.CreateScopeTime("my_timer");
  }
  logging::LogFlush();

  const auto log_line = GetStreamString();
  EXPECT_TRUE(utils::regex_match(log_line, utils::regex(kExpectedPattern)))
      << log_line;
}

UTEST_F(Span, LogBufferSize) {
  tracing::Span span("http/my-glorious-http-handler-name");
  span.AddTag("meta_type", "my-glorious-http-handler-name");
  span.AddTag("meta_code", 500);
  span.AddTag("http_method", "DELETE");
  span.AddTag("uri", "https://example.com/some/modest/uri?with=some;more=args");
  span.AddTag("type", "request");
  span.AddTag("request_body_length", 42);
  span.AddTag("body", "just some modest sample request body, not too long");
  span.AddTag("request_application", "my-userver-service");
  span.AddTag("lang", "en");
  span.AddTag("useragent", "what is that?");
  span.AddTag("battery_type", "AAAAA");

  LOG_ERROR() << "An exception occurred in 'my-glorious-http-handler-name' "
                 "handler, we found this unacceptable thing and just couldn't "
                 "really continue";
  logging::LogFlush();

  EXPECT_LE(GetStreamString().size(), logging::kInitialLogBufferSize)
      << "A typical log, which a handler would write, caused a buffer "
         "reallocation. Please adjust the initial buffer size.";
}

UTEST_F(Span, SourceLocation) {
  { tracing::Span span("span_name"); }
  logging::LogFlush();
  EXPECT_THAT(GetStreamString(), HasSubstr("module=TestBody ( "));
  EXPECT_THAT(GetStreamString(), HasSubstr("userver/core/src/tracing"));
}

UTEST_F(Span, Tag) {
  {
    tracing::Span span("span_name");
    span.AddTag("k", "v");

    logging::LogFlush();
    EXPECT_THAT(GetStreamString(), Not(HasSubstr("k=v")));
  }
  logging::LogFlush();
  EXPECT_THAT(GetStreamString(), HasSubstr("k=v"));
}

UTEST_F(Span, InheritTag) {
  tracing::Span span("span_name");
  tracing::Span::CurrentSpan().AddTag("k", "v");

  logging::LogFlush();
  EXPECT_THAT(GetStreamString(), Not(HasSubstr("k=v")));

  tracing::Span span2("subspan");
  LOG_INFO() << "inside";

  logging::LogFlush();
  EXPECT_THAT(GetStreamString(), HasSubstr("k=v"));
}

UTEST_F(Span, NonInheritTag) {
  tracing::Span span("span_name");

  tracing::Span::CurrentSpan().AddNonInheritableTag("k", "v");
  LOG_INFO() << "inside";
  logging::LogFlush();

  EXPECT_THAT(GetStreamString(), Not(HasSubstr("k=v")));
}

UTEST_F(Span, ScopeTime) {
  {
    tracing::Span span("span_name");
    auto st = span.CreateScopeTime("xxx");

    logging::LogFlush();
    EXPECT_THAT(GetStreamString(), Not(HasSubstr("xxx")));
  }

  logging::LogFlush();
  EXPECT_THAT(GetStreamString(), HasSubstr("xxx_time="));
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

  const auto xxx_time = parse_timing(GetStreamString(), "xxx_time");
  const auto total_time = parse_timing(GetStreamString(), "total_time");

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
  EXPECT_NEAR(unknown_elapsed, 0.0, abs_error);

  engine::SleepFor(std::chrono::milliseconds(2));

  // registered scope time should not be zero
  ASSERT_NE(span.GetTotalElapsedTime("xxx"), std::chrono::milliseconds{0});
}

UTEST_F(Span, InTest) { tracing::Span::CurrentSpan().AddTag("1", 2); }

UTEST_F(Span, LocalLogLevel) {
  {
    tracing::Span span("span_name");

    LOG_INFO() << "info1";
    logging::LogFlush();
    EXPECT_THAT(GetStreamString(), HasSubstr("info1"));

    span.SetLocalLogLevel(logging::Level::kWarning);
    LOG_INFO() << "info2";
    LOG_WARNING() << "warning2";
    logging::LogFlush();
    EXPECT_THAT(GetStreamString(), Not(HasSubstr("info2")));
    EXPECT_THAT(GetStreamString(), HasSubstr("warning2"));

    {
      tracing::Span span("span2");

      LOG_WARNING() << "warning3";
      LOG_INFO() << "info3";
      logging::LogFlush();
      EXPECT_THAT(GetStreamString(), HasSubstr("warning3"));
      EXPECT_THAT(GetStreamString(), Not(HasSubstr("info3")));
    }
  }

  LOG_INFO() << "info4";
  logging::LogFlush();
  EXPECT_THAT(GetStreamString(), HasSubstr("info4"));
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
    EXPECT_THAT(GetStreamString(), HasSubstr("simplelog"));
  }

  logging::LogFlush();
  EXPECT_THAT(GetStreamString(), HasSubstr("logged_span"));
}

UTEST_F(Span, ConstructFromTracer) {
  auto tracer = tracing::MakeTracer("test_service");

  tracing::Span span(tracer, "name", nullptr, tracing::ReferenceType::kChild);
  span.SetLink("some_link");

  LOG_INFO() << "tracerlog";
  logging::LogFlush();
  EXPECT_THAT(GetStreamString(), HasSubstr("tracerlog"));

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

  EXPECT_THAT(GetStreamString(), HasSubstr(kLogFirstSpan));
  EXPECT_THAT(GetStreamString(), Not(HasSubstr(kIgnoreFirstSpan)));
  EXPECT_THAT(GetStreamString(), HasSubstr(kLogSecondSpan));
  EXPECT_THAT(GetStreamString(),
              Not(HasSubstr(kIgnoreSecondSpan + std::string("\t"))));
  EXPECT_THAT(GetStreamString(), HasSubstr(kLogThirdSpan + std::string("\t")));

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

  { tracing::Span a{kIgnorePrefix0 + "foo"}; }
  { tracing::Span a{kLogSpan0}; }
  { tracing::Span a{kLogSpan1}; }
  { tracing::Span a{kIgnorePrefix2 + "XXX"}; }
  { tracing::Span a{kLogSpan2}; }
  { tracing::Span a{kIgnorePrefix1 + "74dfljzs"}; }
  { tracing::Span a{kIgnorePrefix0 + "bar"}; }
  { tracing::Span a{kLogSpan3}; }
  { tracing::Span a{kIgnorePrefix0}; }
  { tracing::Span a{kIgnorePrefix1}; }
  { tracing::Span a{kIgnorePrefix2}; }
  { tracing::Span a{kIgnoreSpan}; }

  logging::LogFlush();

  const auto output = GetStreamString();
  EXPECT_THAT(output, HasSubstr(kLogSpan0));
  EXPECT_THAT(output, HasSubstr(kLogSpan1));
  EXPECT_THAT(output, HasSubstr(kLogSpan2));
  EXPECT_THAT(output, HasSubstr(kLogSpan3));

  EXPECT_THAT(output, Not(HasSubstr("=" + kIgnorePrefix0)));
  EXPECT_THAT(output, Not(HasSubstr(kIgnorePrefix1)));
  EXPECT_THAT(output, Not(HasSubstr(kIgnorePrefix2)));
  EXPECT_THAT(output, Not(HasSubstr(kIgnoreSpan)));

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

  { tracing::Span a{kIgnorePrefix0 + "oops"}; }
  { tracing::Span a{kLogSpan0}; }
  { tracing::Span a{kLogSpan1}; }
  { tracing::Span a{kIgnorePrefix2 + "I"}; }
  { tracing::Span a{kLogSpan2}; }
  { tracing::Span a{kIgnorePrefix1 + "did it"}; }
  { tracing::Span a{kIgnorePrefix0 + "again"}; }
  { tracing::Span a{kLogSpan3}; }
  { tracing::Span a{kLogSpan4}; }
  { tracing::Span a{kIgnorePrefix0}; }
  { tracing::Span a{kIgnorePrefix1}; }
  { tracing::Span a{kIgnorePrefix2}; }
  { tracing::Span a{kIgnoreSpan}; }

  logging::LogFlush();

  const auto output = GetStreamString();
  EXPECT_THAT(output, HasSubstr(kLogSpan0));
  EXPECT_THAT(output, HasSubstr(kLogSpan1));
  EXPECT_THAT(output, HasSubstr(kLogSpan2));
  EXPECT_THAT(output, HasSubstr(kLogSpan3));
  EXPECT_THAT(output, HasSubstr(kLogSpan4));

  EXPECT_THAT(output, Not(HasSubstr("=" + kIgnorePrefix0)));
  EXPECT_THAT(output, Not(HasSubstr(kIgnorePrefix1)));
  EXPECT_THAT(output, Not(HasSubstr(kIgnorePrefix2)));
  EXPECT_THAT(output, Not(HasSubstr(kIgnoreSpan + std::string("\t"))));

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

  EXPECT_THAT(GetStreamString(), Not(HasSubstr(kIgnoreFirstSpan)));

  {
    tracing::Span span2(kIgnoreSecondSpan);
    span2.SetLogLevel(logging::Level::kInfo);
  }

  logging::LogFlush();

  EXPECT_THAT(GetStreamString(), Not(HasSubstr(kIgnoreSecondSpan)));

  tracing::Tracer::SetNoLogSpans(tracing::NoLogSpans());
}

UTEST_F(Span, ForeignSpan) {
  auto tracer = tracing::MakeTracer("test_service");

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

  auto logs_raw = GetStreamString();

  auto logs = utils::text::Split(logs_raw, "\n");

  bool found_sw = false;
  bool found_tr = false;

  for (const auto& log : logs) {
    if (log.find("stopwatch_name=foreign") != std::string::npos) {
      found_sw = true;
      EXPECT_THAT(log, HasSubstr("link=foreign_link"));
    }

    // check unlink
    if (log.find("tracerlog") != std::string::npos) {
      found_tr = true;
      EXPECT_THAT(log, HasSubstr("link=local_link"));
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

UTEST_F(Span, SetLogLevelDoesntBreakGenealogyRoot) {
  auto& root_span = tracing::Span::CurrentSpan();
  {
    tracing::Span child_span{"child"};
    child_span.SetLogLevel(logging::Level::kTrace);
    {
      tracing::Span grandchild_span{"grandchild"};
      EXPECT_EQ(grandchild_span.GetParentId(), root_span.GetSpanId());
    }
    logging::LogFlush();
    EXPECT_THAT(GetStreamString(),
                HasSubstr(fmt::format("parent_id={}", root_span.GetSpanId())));
  }
}

UTEST_F(Span, SetLogLevelDoesntBreakGenealogyLoggableParent) {
  tracing::Span root_span{"root_span"};
  {
    tracing::Span child_span{"child"};
    child_span.SetLogLevel(logging::Level::kTrace);
    {
      tracing::Span grandchild_span{"grandchild"};
      EXPECT_EQ(grandchild_span.GetParentId(), root_span.GetSpanId());
    }
    logging::LogFlush();
    EXPECT_THAT(GetStreamString(),
                HasSubstr(fmt::format("parent_id={}", root_span.GetSpanId())));
  }
}

UTEST_F(Span, SetLogLevelDoesntBreakGenealogyMultiSkip) {
  tracing::Span root_span{"root_span"};
  {
    tracing::Span span_no_log{"no_log", tracing::ReferenceType::kChild,
                              logging::Level::kTrace};
    {
      tracing::Span span_no_log_2{"no_log_2", tracing::ReferenceType::kChild,
                                  logging::Level::kTrace};
      {
        tracing::Span child{"child"};
        EXPECT_EQ(child.GetParentId(), root_span.GetSpanId());
      }
      logging::LogFlush();
      EXPECT_THAT(
          GetStreamString(),
          HasSubstr(fmt::format("parent_id={}", root_span.GetSpanId())));
    }
  }
}

UTEST_F(Span, MakeSpanWithParentIdTraceIdLink) {
  std::string trace_id = "1234567890-trace-id";
  std::string parent_id = "1234567890-parent-id";
  std::string link = "1234567890-link";

  tracing::Span span =
      tracing::Span::MakeSpan("span", trace_id, parent_id, link);

  EXPECT_EQ(span.GetTraceId(), trace_id);
  EXPECT_EQ(span.GetParentId(), parent_id);
  EXPECT_EQ(span.GetLink(), link);
}

UTEST_F(Span, MakeSpanWithParentIdTraceIdLinkWithExisting) {
  std::string trace_id = "1234567890-trace-id";
  std::string parent_id = "1234567890-parent-id";
  std::string link = "1234567890-link";

  tracing::Span root_span("root_span");
  {
    tracing::Span span =
        tracing::Span::MakeSpan("span", trace_id, parent_id, link);

    EXPECT_EQ(span.GetTraceId(), trace_id);
    EXPECT_EQ(span.GetParentId(), parent_id);
    EXPECT_EQ(span.GetLink(), link);
  }
}

USERVER_NAMESPACE_END
