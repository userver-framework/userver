#include <fmt/format.h>
#include <gmock/gmock.h>

#include <logging/log_helper_impl.hpp>
#include <logging/logging_test.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/tracing/opentelemetry.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/span_event.hpp>
#include <userver/tracing/tracer.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/regex.hpp>
#include <userver/utils/text_light.hpp>

using testing::HasSubstr;
using testing::Not;

USERVER_NAMESPACE_BEGIN

class Span : public LoggingTest {
private:
    tracing::TracerCleanupScope tracer_scope_;
};

class OpentracingSpan : public Span {
protected:
    OpentracingSpan() : opentracing_logger_(MakeNamedStreamLogger("openstracing", logging::Format::kTskv)) {
        tracing::Tracer::SetTracer(tracing::Tracer("service-test-name", opentracing_logger_.logger));

        // Discard logs
        logging::LogFlush(*opentracing_logger_.logger);
        opentracing_logger_.stream.str({});
    }

    // NOLINTNEXTLINE(readability-make-member-function-const)
    void FlushOpentracing() { logging::LogFlush(*opentracing_logger_.logger); }

    static void CheckTagFormat(const formats::json::Value& tag) {
        EXPECT_TRUE(tag.HasMember("key"));
        EXPECT_TRUE(tag.HasMember("value"));
        EXPECT_TRUE(tag.HasMember("type"));
    }

    static void
    CheckTagTypeAndValue(const formats::json::Value& tag, const std::string& type, const std::string& value) {
        EXPECT_EQ(type, tag["type"].As<std::string>());
        EXPECT_EQ(value, tag["value"].As<std::string>());
    }

    static formats::json::Value GetTagsJson(const std::string& log_output) {
        auto tags_start = log_output.find("tags=");
        if (tags_start == std::string::npos) return {};
        tags_start += 5;
        auto tags_str = log_output.substr(tags_start);
        const auto tags_end = tags_str.find(']');
        if (tags_end == std::string::npos) return {};
        return formats::json::FromString(tags_str.substr(0, tags_end + 1));
    }

    std::string GetOtStreamString() const { return opentracing_logger_.stream.str(); }

private:
    StringStreamLogger opentracing_logger_;
};

UTEST_F(Span, Ctr) {
    {
        logging::LogFlush();
        EXPECT_THAT(GetStreamString(), Not(HasSubstr("stopwatch_name=")));

        const tracing::Span span("span_name");
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
    constexpr std::string_view kExpectedPattern = R"(tskv\t)"
                                                  R"(timestamp=\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\.\d{6}\t)"
                                                  R"(level=[A-Z]+\t)"
                                                  R"(module=[\w\d ():./]+\t)"
                                                  R"(trace_id=[0-9a-f]+\t)"
                                                  R"(span_id=[0-9a-f]+\t)"
                                                  R"(parent_id=[0-9a-f]+\t)"
                                                  R"(link=[0-9a-f]+\t)"
                                                  R"(stopwatch_name=span_name\t)"
                                                  R"(total_time=\d+(\.\d+)?\t)"
                                                  R"(span_ref_type=child\t)"
                                                  R"(stopwatch_units=ms\t)"
                                                  R"(start_timestamp=\d+(\.\d+)?\t)"
                                                  R"(my_timer_time=\d+(\.\d+)?\t)"
                                                  R"(my_tag_key=my_tag_value\t)"
                                                  R"(span_kind=internal\t)"
                                                  R"(text=\n)";
    const tracing::Span parent("parent_span_name");
    {
        tracing::Span span("span_name");
        span.AddTag("my_tag_key", "my_tag_value");
        span.CreateScopeTime("my_timer");
    }
    logging::LogFlush();

    const auto log_line = GetStreamString();
    EXPECT_TRUE(utils::regex_match(log_line, utils::regex(kExpectedPattern))) << log_line;
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
    // clang-format off
    { const tracing::Span span("span_name"); }
    // clang-format on

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
    const tracing::Span span("span_name");
    tracing::Span::CurrentSpan().AddTag("k", "v");

    logging::LogFlush();
    EXPECT_THAT(GetStreamString(), Not(HasSubstr("k=v")));

    const tracing::Span span2("subspan");
    LOG_INFO() << "inside";

    logging::LogFlush();
    EXPECT_THAT(GetStreamString(), HasSubstr("k=v"));
}

UTEST_F(Span, NonInheritTag) {
    const tracing::Span span("span_name");

    tracing::Span::CurrentSpan().AddNonInheritableTag("k", "v");
    LOG_INFO() << "inside";
    logging::LogFlush();

    EXPECT_THAT(GetStreamString(), Not(HasSubstr("k=v")));
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
    const auto log_str = GetOtStreamString();
    EXPECT_THAT(log_str, Not(HasSubstr("k=v")));
    EXPECT_THAT(log_str, HasSubstr("http.status_code"));
    EXPECT_THAT(log_str, HasSubstr("error"));
    EXPECT_THAT(log_str, HasSubstr("http.method"));
    EXPECT_THAT(log_str, HasSubstr("db.type"));
    EXPECT_THAT(log_str, HasSubstr("db.statement"));
    EXPECT_THAT(log_str, HasSubstr("peer.address"));
    EXPECT_THAT(log_str, HasSubstr("http.url"));
}

UTEST_F(OpentracingSpan, FromTracerWithServiceName) {
    tracing::Tracer::SetTracer({"test_service", tracing::Tracer::CopyCurrentTracer().GetOptionalLogger()});
    // clang-format off
    { const tracing::Span span("span_name", nullptr, tracing::ReferenceType::kChild); }
    // clang-format on

    FlushOpentracing();
    const auto log_str = GetOtStreamString();
    EXPECT_THAT(log_str, HasSubstr("service_name=test_service"));
}

UTEST_F(OpentracingSpan, TagFormat) {
    {
        tracing::Span span("span_name");
        span.AddTag("meta_code", 200);
        span.AddTag("error", false);
        span.AddTag("method", "POST");
    }
    FlushOpentracing();
    const auto tags = GetTagsJson(GetOtStreamString());
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

    const auto parse_timing = [](const std::string& str, std::string_view name) -> std::optional<double> {
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
            const tracing::Span span("span2");

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

UTEST_F(Span, LocalLogLevelLowerThanGlobal) {
    tracing::Span span("parent_span");
    // This currently cannot overcome global log level, which is "info" for this test.
    span.SetLocalLogLevel(logging::Level::kDebug);

    LOG_DEBUG() << "message";
    logging::LogFlush();

    EXPECT_THAT(GetStreamString(), Not(HasSubstr("message")));
}

UTEST_F(Span, SpanLogLevelLowerThanGlobal) {
    {
        EXPECT_EQ(logging::GetDefaultLoggerLevel(), logging::Level::kInfo);
        const tracing::Span parent_span("parent_span");

        tracing::Span span("not_logged_span");
        // The span itself will be hidden.
        span.SetLogLevel(logging::Level::kDebug);

        LOG_INFO() << "message";
        logging::LogFlush();

        EXPECT_THAT(GetStreamString(), HasSubstr("message"));
        // Make sure there are no logs written with "non-existent" spans.
        EXPECT_THAT(GetStreamString(), Not(HasSubstr("span_id=" + std::string{span.GetSpanId()})));
        EXPECT_THAT(GetStreamString(), HasSubstr("span_id=" + std::string{parent_span.GetSpanId()}));
    }

    EXPECT_THAT(GetStreamString(), Not(HasSubstr("not_logged_span")));
    EXPECT_THAT(GetStreamString(), HasSubstr("parent_span"));
}

UTEST_F(Span, NoLogNames) {
    constexpr const char* kLogFirstSpan = "first_span_to_log";
    constexpr const char* kLogSecondSpan = "second_span_to_log";
    constexpr const char* kLogThirdSpan = "second_span_to_ignore_is_the_prefix_of_this_span";

    constexpr const char* kIgnoreFirstSpan = "first_span_to_ignore";
    constexpr const char* kIgnoreSecondSpan = "second_span_to_ignore";

    tracing::NoLogSpans no_logs;
    no_logs.names = {
        kIgnoreFirstSpan,
        kIgnoreSecondSpan,
    };
    tracing::SetNoLogSpans(std::move(no_logs));

    {
        const tracing::Span span0(kLogFirstSpan);
        const tracing::Span span1(kIgnoreFirstSpan);
        const tracing::Span span2(kLogSecondSpan);
        const tracing::Span span3(kIgnoreSecondSpan);
        const tracing::Span span4(kLogThirdSpan);
    }

    logging::LogFlush();

    EXPECT_THAT(GetStreamString(), HasSubstr(kLogFirstSpan));
    EXPECT_THAT(GetStreamString(), Not(HasSubstr(kIgnoreFirstSpan)));
    EXPECT_THAT(GetStreamString(), HasSubstr(kLogSecondSpan));
    EXPECT_THAT(GetStreamString(), Not(HasSubstr(kIgnoreSecondSpan + std::string("\t"))));
    EXPECT_THAT(GetStreamString(), HasSubstr(kLogThirdSpan + std::string("\t")));
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
    tracing::SetNoLogSpans(std::move(no_logs));

    // clang-format off
    { const tracing::Span a{kIgnorePrefix0 + "foo"}; }
    { const tracing::Span a{kLogSpan0}; }
    { const tracing::Span a{kLogSpan1}; }
    { const tracing::Span a{kIgnorePrefix2 + "XXX"}; }
    { const tracing::Span a{kLogSpan2}; }
    { const tracing::Span a{kIgnorePrefix1 + "74dfljzs"}; }
    { const tracing::Span a{kIgnorePrefix0 + "bar"}; }
    { const tracing::Span a{kLogSpan3}; }
    { const tracing::Span a{kIgnorePrefix0}; }
    { const tracing::Span a{kIgnorePrefix1}; }
    { const tracing::Span a{kIgnorePrefix2}; }
    { const tracing::Span a{kIgnoreSpan}; }
    // clang-format on

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
}

UTEST_F(Span, NoLogMixed) {
    auto json = formats::json::FromString(R"({
        "names": ["i_am_a_span_to_ignore"],
        "prefixes": ["skip", "ignore", "skip", "do_not_keep", "skip", "skip"]
    })");
    auto no_logs = Parse(json, formats::parse::To<tracing::NoLogSpans>{});
    tracing::SetNoLogSpans(std::move(no_logs));

    constexpr const char* kLogSpan0 = "first_span_to_log";
    constexpr const char* kLogSpan1 = "i_am_a_span_to_ignore(not!)";
    constexpr const char* kLogSpan2 = "span";
    constexpr const char* kLogSpan3 = "ign";
    constexpr const char* kLogSpan4 = "i_am_span";

    constexpr const char* kIgnoreSpan = "i_am_a_span_to_ignore";

    const std::string kIgnorePrefix0 = "ignore";
    const std::string kIgnorePrefix1 = "skip";
    const std::string kIgnorePrefix2 = "do_not_keep";

    // clang-format off
    { const tracing::Span a{kIgnorePrefix0 + "oops"}; }
    { const tracing::Span a{kLogSpan0}; }
    { const tracing::Span a{kLogSpan1}; }
    { const tracing::Span a{kIgnorePrefix2 + "I"}; }
    { const tracing::Span a{kLogSpan2}; }
    { const tracing::Span a{kIgnorePrefix1 + "did it"}; }
    { const tracing::Span a{kIgnorePrefix0 + "again"}; }
    { const tracing::Span a{kLogSpan3}; }
    { const tracing::Span a{kLogSpan4}; }
    { const tracing::Span a{kIgnorePrefix0}; }
    { const tracing::Span a{kIgnorePrefix1}; }
    { const tracing::Span a{kIgnorePrefix2}; }
    { const tracing::Span a{kIgnoreSpan}; }
    // clang-format on

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
}

UTEST_F(Span, NoLogWithSetLogLevel) {
    constexpr const char* kIgnoreFirstSpan = "first_span_to_ignore";
    constexpr const char* kIgnoreSecondSpan = "second_span_to_ignore";

    tracing::NoLogSpans no_logs;
    no_logs.names = {
        kIgnoreFirstSpan,
        kIgnoreSecondSpan,
    };
    tracing::SetNoLogSpans(std::move(no_logs));

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
}

UTEST_F(Span, ForeignSpan) {
    tracing::Span local_span("local", nullptr, tracing::ReferenceType::kChild);
    local_span.SetLink("local_link");

    {
        tracing::Span foreign_span("foreign", nullptr, tracing::ReferenceType::kChild);
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
        span.AddTagFrozen("frozen", "it is not possible to change this tag value in subspan");
        span.AddNonInheritableTag("local", "this tag is not visible in subspans");
        /// [Example using Span tracing]
    }
    {
        const std::string user = "user";
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
    {
        tracing::Span child_span{"child"};
        child_span.SetLogLevel(logging::Level::kTrace);
        {
            const tracing::Span grandchild_span{"grandchild"};
            EXPECT_TRUE(grandchild_span.GetParentId().empty());
        }
        logging::LogFlush();
        EXPECT_THAT(GetStreamString(), HasSubstr("parent_id=\t"));
    }
}

UTEST_F(Span, SetLogLevelDoesntBreakGenealogyLoggableParent) {
    const tracing::Span root_span{"root_span"};
    {
        tracing::Span child_span{"child"};
        child_span.SetLogLevel(logging::Level::kTrace);
        {
            const tracing::Span grandchild_span{"grandchild"};
            EXPECT_EQ(grandchild_span.GetParentId(), root_span.GetSpanId());
        }
        logging::LogFlush();
        EXPECT_THAT(GetStreamString(), HasSubstr(fmt::format("parent_id={}", root_span.GetSpanId())));
    }
}

UTEST_F(Span, SetLogLevelDoesntBreakGenealogyMultiSkip) {
    const tracing::Span root_span{"root_span"};
    {
        tracing::Span span_no_log{"no_log"};
        span_no_log.SetLogLevel(logging::Level::kTrace);
        {
            tracing::Span span_no_log_2{"no_log_2"};
            span_no_log_2.SetLogLevel(logging::Level::kTrace);
            {
                const tracing::Span child{"child"};
                EXPECT_EQ(child.GetParentId(), root_span.GetSpanId());
            }
            logging::LogFlush();
            EXPECT_THAT(GetStreamString(), HasSubstr(fmt::format("parent_id={}", root_span.GetSpanId())));
        }
    }
}

UTEST_F(Span, SetLogLevelDoesntBreakGenealogyAsync) {
    tracing::Span root_span{"root_span"};
    utils::Async("no_log", [&] {
        tracing::Span::CurrentSpan().SetLogLevel(logging::Level::kTrace);
        const tracing::Span child{"child"};
        EXPECT_EQ(child.GetParentId(), root_span.GetSpanId());
    }).Get();
}

UTEST_F(Span, SetLogLevelDoesntBreakGenealogyAsyncMultiSkip) {
    tracing::Span root_span{"root_span"};
    utils::Async("no_log", [&] {
        tracing::Span::CurrentSpan().SetLogLevel(logging::Level::kTrace);
        utils::Async("no_log_2", [&] {
            tracing::Span::CurrentSpan().SetLogLevel(logging::Level::kTrace);
            const tracing::Span child{"child"};
            EXPECT_EQ(child.GetParentId(), root_span.GetSpanId());
        }).Get();
    }).Get();
}

UTEST_F(Span, SetLogLevelDoesNotDetachLogs) {
    const tracing::Span root_span{"root_span"};
    {
        tracing::Span span_no_log{"no_log"};
        span_no_log.SetLogLevel(logging::Level::kTrace);

        LOG_INFO() << "test";
        logging::LogFlush();

        EXPECT_THAT(
            GetStreamString(),
            testing::AllOf(
                HasSubstr("span_id=" + std::string{root_span.GetSpanId()}),
                HasSubstr("link=" + std::string{root_span.GetLink()}),
                HasSubstr("trace_id=" + std::string{root_span.GetTraceId()}),
                Not(HasSubstr(span_no_log.GetSpanId()))
            )
        );
    }
}

UTEST_F(Span, SetLogLevelDoesNotDetachLogsMultiSkip) {
    const tracing::Span root_span{"root_span"};
    {
        tracing::Span span_no_log1{"no_log_1"};
        span_no_log1.SetLogLevel(logging::Level::kTrace);
        {
            tracing::Span span_no_log2{"no_log_2"};
            span_no_log2.SetLogLevel(logging::Level::kTrace);

            LOG_INFO() << "test";
            logging::LogFlush();

            EXPECT_THAT(
                GetStreamString(),
                testing::AllOf(
                    HasSubstr("span_id=" + std::string{root_span.GetSpanId()}),
                    HasSubstr("link=" + std::string{root_span.GetLink()}),
                    HasSubstr("trace_id=" + std::string{root_span.GetTraceId()}),
                    Not(HasSubstr(span_no_log1.GetSpanId())),
                    Not(HasSubstr(span_no_log2.GetSpanId()))
                )
            );
        }
    }
}

UTEST_F(Span, SetLogLevelInheritsHiddenSpanTags) {
    // userver spans can be used to attach tags to a group of logs regardless of tracing.
    // Even if the span is hidden from tracing, we should still propagate its tags.
    const tracing::Span root_span{"root_span"};
    {
        tracing::Span span_no_log{"no_log"};
        span_no_log.SetLogLevel(logging::Level::kTrace);
        span_no_log.AddTag("custom_tag_key", "custom_tag_value");

        LOG_INFO() << "test";
        logging::LogFlush();

        EXPECT_THAT(GetStreamString(), HasSubstr("custom_tag_key"));
    }
}

UTEST_F(Span, OnlyUpstreamSpanIsEnabled) {
    // Let's imagine that we've got a request with tracing, but all of our own spans are not loggable -
    // for example, because of a restrictive global log level or because of NO_LOG_SPANS config.
    // Then we've got a log that has high enough log level to be written. What span do we bind it to?
    //
    // Possible options:
    // 1. Bind the log to a local non-loggable ("non-existent") span. This has no practical benefits over (2) and
    //    will only confuse the tracing system.
    // 2. Don't bind the log to any span. This makes it almost impossible to find the log when investigating issues
    //    with a request, especially if it's not an error log.
    // 3. Bind our log to the external span.
    //
    // userver uses option 3. So, logs of the current service are directly bound to the span of an upstream service.
    // Weird, but workable.
    //
    // Now, there is an extra quirk. For this log userver currently specifies:
    // * span_id = upstream span_id (OK)
    // * trace_id = local trace_id = upstream trace_id (OK)
    // * link = local link != upstream link (?!)
    //
    // So essentially we bind the log to a non-existent span with upstream span_id but local link.
    // This works as long as the tracing system does not rely on links to bind spans together.
    // This even somewhat makes sense if someone wishes to find neighbouring logs from our service.

    const tracing::Span upstream_span{"upstream_span"};

    auto handler_span_no_log = tracing::Span::MakeSpan(
        "handler_span", upstream_span.GetTraceId(), upstream_span.GetSpanId(), "1234567890-link"
    );
    handler_span_no_log.SetLogLevel(logging::Level::kTrace);

    tracing::Span span_no_log1{"span_no_log1"};
    span_no_log1.SetLogLevel(logging::Level::kTrace);

    tracing::Span span_no_log2{"span_no_log2"};
    span_no_log2.SetLogLevel(logging::Level::kTrace);

    EXPECT_EQ(span_no_log2.GetSpanIdForChildLogs(), upstream_span.GetSpanId());
    EXPECT_NE(handler_span_no_log.GetLink(), upstream_span.GetLink());
    EXPECT_EQ(span_no_log2.GetLink(), handler_span_no_log.GetLink());
    EXPECT_EQ(span_no_log2.GetTraceId(), upstream_span.GetTraceId());

    LOG_INFO() << "test";
    logging::LogFlush();

    EXPECT_THAT(
        GetStreamString(),
        testing::AllOf(
            HasSubstr("span_id=" + std::string{upstream_span.GetSpanId()}),
            Not(HasSubstr(handler_span_no_log.GetSpanId())),
            Not(HasSubstr(span_no_log1.GetSpanId())),
            Not(HasSubstr(span_no_log2.GetSpanId()))
        )
    );

    EXPECT_THAT(
        GetStreamString(),
        testing::AllOf(
            HasSubstr("link=" + std::string{span_no_log2.GetLink()}), Not(HasSubstr(upstream_span.GetLink()))
        )
    );

    EXPECT_THAT(GetStreamString(), HasSubstr("trace_id=" + std::string{span_no_log2.GetTraceId()}));
}

UTEST_F(Span, MakeSpanWithParentIdTraceIdLink) {
    const std::string trace_id = "1234567890-trace-id";
    const std::string parent_id = "1234567890-parent-id";
    const std::string link = "1234567890-link";

    const tracing::Span span = tracing::Span::MakeSpan("span", trace_id, parent_id, link);

    EXPECT_EQ(span.GetTraceId(), trace_id);
    EXPECT_EQ(span.GetParentId(), parent_id);
    EXPECT_EQ(span.GetLink(), link);
}

UTEST_F(Span, MakeSpanWithParentIdTraceIdLinkWithExisting) {
    const std::string trace_id = "1234567890-trace-id";
    const std::string parent_id = "1234567890-parent-id";
    const std::string link = "1234567890-link";

    const tracing::Span root_span("root_span");
    {
        const tracing::Span span = tracing::Span::MakeSpan("span", trace_id, parent_id, link);

        EXPECT_EQ(span.GetTraceId(), trace_id);
        EXPECT_EQ(span.GetParentId(), parent_id);
        EXPECT_EQ(span.GetLink(), link);
    }
}

UTEST_F(Span, MakeSpanEvent) {
    {
        tracing::Span root_span("root_span");
        root_span.AddEvent("important_event");
    }

    logging::LogFlush();

    const auto logs_raw = GetStreamString();

    EXPECT_THAT(logs_raw, HasSubstr(R"(events=[{"name":"important_event")"));
    EXPECT_THAT(logs_raw, HasSubstr("root_span"));
}

UTEST_F(Span, MakeSpanEventWithAttributes) {
    {
        tracing::Span root_span("root_span");
        root_span.AddEvent({"important_event_0", {{"int", tracing::AnyValue{42}}}});
        root_span.AddEvent({"important_event_1", {{"string", tracing::AnyValue{"value"}}}});
        root_span.AddEvent(
            {"event_with_many_attributes",
             {
                 {"string", tracing::AnyValue{"another_value"}},
                 {"int", tracing::AnyValue{123}},
                 {"float", tracing::AnyValue{123.456}},
                 {"bool", tracing::AnyValue{true}},
             }}
        );
    }

    logging::LogFlush();

    const auto logs_raw = GetStreamString();

    EXPECT_THAT(logs_raw, HasSubstr(R"(events=[{"name":"important_event_0","time_unix_nano")"));
    EXPECT_THAT(logs_raw, HasSubstr(R"({"name":"important_event_1","time_unix_nano")"));
    EXPECT_THAT(logs_raw, HasSubstr(R"({"name":"event_with_many_attributes","time_unix_nano")"));
    EXPECT_THAT(logs_raw, HasSubstr("root_span"));

    // Attributes
    EXPECT_THAT(logs_raw, HasSubstr(R"("attributes":{"int":42})"));
    EXPECT_THAT(logs_raw, HasSubstr(R"("attributes":{"string":"value"})"));
    EXPECT_THAT(logs_raw, HasSubstr(R"("string":"another_value")"));
    EXPECT_THAT(logs_raw, HasSubstr(R"("int":123)"));
    EXPECT_THAT(logs_raw, HasSubstr(R"("float":123.456)"));
    EXPECT_THAT(logs_raw, HasSubstr(R"("bool":true)"));
}

UTEST_F(Span, IsCompatibleWithOpentelemetry) {
    auto span = tracing::Span::MakeRootSpan("span-name");
    EXPECT_NE(span.GetTraceId(), "");
    EXPECT_NE(span.GetSpanId(), "");

    static constexpr std::string_view kFlags = "01";

    const auto otel_header =
        tracing::opentelemetry::BuildTraceParentHeader(span.GetTraceId(), span.GetSpanId(), kFlags);
    ASSERT_TRUE(otel_header.has_value()) << otel_header.error();

    const auto otel_data = tracing::opentelemetry::ExtractTraceParentData(otel_header.value());
    ASSERT_TRUE(otel_data.has_value()) << otel_data.error();

    EXPECT_EQ(otel_data.value().trace_id, span.GetTraceId());
    EXPECT_EQ(otel_data.value().span_id, span.GetSpanId());
    EXPECT_EQ(otel_data.value().trace_flags, kFlags);
}

USERVER_NAMESPACE_END
