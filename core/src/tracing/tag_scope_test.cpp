#include <userver/tracing/tag_scope.hpp>

#include <gmock/gmock.h>

#include <logging/logging_test.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

struct TagScopeTest : public LoggingTest {};

UTEST_F(TagScopeTest, Basic) {
  tracing::Span myspan{"myspan"};
  const tracing::TagScope tag{myspan, "key", "value"};
  LOG_INFO() << "1";
  logging::LogFlush();
  ASSERT_THAT(GetStreamString(), testing::HasSubstr("key=value"));
}

UTEST_F(TagScopeTest, BasicFrozen) {
  tracing::Span myspan{"myspan"};
  const tracing::TagScope tag{myspan, "key", "value",
                              logging::LogExtra::ExtendType::kFrozen};
  LOG_INFO() << "1";
  logging::LogFlush();
  ASSERT_THAT(GetStreamString(), testing::HasSubstr("key=value"));
}

UTEST_F(TagScopeTest, IntegerType) {
  tracing::Span myspan{"myspan"};
  const tracing::TagScope tag{myspan, "key", 123};
  LOG_INFO() << "1";
  logging::LogFlush();
  ASSERT_THAT(GetStreamString(), testing::HasSubstr("key=123"));
}

UTEST_F(TagScopeTest, IntegerTypeFrozen) {
  tracing::Span myspan{"myspan"};
  const tracing::TagScope tag{myspan, "key", 123,
                              logging::LogExtra::ExtendType::kFrozen};
  LOG_INFO() << "1";
  logging::LogFlush();
  ASSERT_THAT(GetStreamString(), testing::HasSubstr("key=123"));
}

UTEST_F(TagScopeTest, AutomaticSpanFinding) {
  tracing::Span myspan{"myspan"};
  const tracing::TagScope tag{"key", 123};
  LOG_INFO() << "1";
  logging::LogFlush();
  ASSERT_THAT(GetStreamString(), testing::HasSubstr("key=123"));
}

UTEST_F(TagScopeTest, AutomaticSpanFindingFrozen) {
  tracing::Span myspan{"myspan"};
  const tracing::TagScope tag{"key", 123,
                              logging::LogExtra::ExtendType::kFrozen};
  LOG_INFO() << "1";
  logging::LogFlush();
  ASSERT_THAT(GetStreamString(), testing::HasSubstr("key=123"));
}

UTEST_F(TagScopeTest, MissedItsScope) {
  tracing::Span myspan{"myspan"};
  { const tracing::TagScope tag{myspan, "key", "value"}; }

  LOG_INFO() << "1";
  logging::LogFlush();
  const auto logs = GetStreamString();
  ASSERT_THAT(logs, testing::Not(testing::HasSubstr("value")));
  ASSERT_THAT(logs, testing::Not(testing::HasSubstr("key")));
}

UTEST_F(TagScopeTest, MissedItsScopeFrozen) {
  tracing::Span myspan{"myspan"};
  {
    const tracing::TagScope tag{myspan, "key", "value",
                                logging::LogExtra::ExtendType::kFrozen};
  }

  LOG_INFO() << "1";
  logging::LogFlush();
  const auto logs = GetStreamString();
  ASSERT_THAT(logs, testing::Not(testing::HasSubstr("value")));
  ASSERT_THAT(logs, testing::Not(testing::HasSubstr("key")));
}

UTEST_F(TagScopeTest, DestructorRollbacksToPreviousValue) {
  /// [TagScope - sample]
  tracing::Span myspan{"myspan"};
  myspan.AddTag("key", "value");
  {
    const tracing::TagScope tag{myspan, "key", "2value2"};

    LOG_INFO() << "1";
    logging::LogFlush();
    const auto logs = GetStreamString();
    ASSERT_THAT(logs, testing::Not(testing::HasSubstr("key=value")));
    ASSERT_THAT(logs, testing::HasSubstr("key=2value2"));
  }
  ClearLog();

  LOG_INFO() << "1";
  logging::LogFlush();
  const auto logs = GetStreamString();
  ASSERT_THAT(logs, testing::HasSubstr("key=value"));
  ASSERT_THAT(logs, testing::Not(testing::HasSubstr("key=2value2")));
  /// [TagScope - sample]
}

UTEST_F(TagScopeTest, DestructorRollbacksToPreviousValueFrozen) {
  tracing::Span myspan{"myspan"};
  myspan.AddTag("key", "value");
  {
    const tracing::TagScope tag{myspan, "key", "2value2",
                                logging::LogExtra::ExtendType::kFrozen};

    LOG_INFO() << "1";
    logging::LogFlush();
    const auto logs = GetStreamString();
    ASSERT_THAT(logs, testing::Not(testing::HasSubstr("key=value")));
    ASSERT_THAT(logs, testing::HasSubstr("key=2value2"));
  }
  ClearLog();

  LOG_INFO() << "1";
  logging::LogFlush();
  const auto logs = GetStreamString();
  ASSERT_THAT(logs, testing::HasSubstr("key=value"));
  ASSERT_THAT(logs, testing::Not(testing::HasSubstr("key=2value2")));
}

UTEST_F(TagScopeTest, AttemptToOverwriteFrozenTag) {
  tracing::Span myspan{"myspan"};
  myspan.AddTagFrozen("key", "value");
  {
    const tracing::TagScope ignored{myspan, "key", "value2"};
    LOG_INFO() << "1";
    logging::LogFlush();
    auto logs = GetStreamString();
    ASSERT_THAT(logs, testing::HasSubstr("key=value"));
    ASSERT_THAT(logs, testing::Not(testing::HasSubstr("key=value2")));
  }
  ClearLog();

  LOG_INFO() << "1";
  logging::LogFlush();
  auto logs = GetStreamString();
  ASSERT_THAT(logs, testing::HasSubstr("key=value"));
  ASSERT_THAT(logs, testing::Not(testing::HasSubstr("key=value2")));
}

UTEST_F(TagScopeTest, AttemptToOverwriteFrozenTagFrozen) {
  tracing::Span myspan{"myspan"};
  myspan.AddTagFrozen("key", "value");
  {
    const tracing::TagScope ignored{myspan, "key", "value2",
                                    logging::LogExtra::ExtendType::kFrozen};
    LOG_INFO() << "1";
    logging::LogFlush();
    auto logs = GetStreamString();
    ASSERT_THAT(logs, testing::HasSubstr("key=value"));
    ASSERT_THAT(logs, testing::Not(testing::HasSubstr("key=value2")));
  }
  ClearLog();

  LOG_INFO() << "1";
  logging::LogFlush();
  auto logs = GetStreamString();
  ASSERT_THAT(logs, testing::HasSubstr("key=value"));
  ASSERT_THAT(logs, testing::Not(testing::HasSubstr("key=value2")));
}

UTEST_F(TagScopeTest, Forget) {
  tracing::Span myspan{"myspan"};
  const tracing::TagScope forgotten_tag{myspan, "key", "value"};
  const tracing::TagScope overwriting_tag{myspan, "key", "2value2"};

  LOG_INFO() << "1";
  logging::LogFlush();
  auto logs = GetStreamString();
  ASSERT_THAT(logs, testing::HasSubstr("key=2value2"));
  ASSERT_THAT(logs, testing::Not(testing::HasSubstr("key=value")));
}

UTEST_F(TagScopeTest, RegularTagBroken) {
  tracing::Span myspan{"myspan"};
  {
    const tracing::TagScope tag{"key", "value"};
    myspan.AddTag("key", "another_value");
  }
  LOG_INFO() << "1";
  logging::LogFlush();
  auto logs = GetStreamString();
  ASSERT_THAT(logs, testing::Not(testing::HasSubstr("key")));
  ASSERT_THAT(logs, testing::Not(testing::HasSubstr("value")));
  // this happens because TagScope does not check value.
}

UTEST_F(TagScopeTest, RegularTagBrokenDouble) {
  tracing::Span myspan{"myspan"};
  myspan.AddTag("key", "value");
  {
    const tracing::TagScope tag1{"key", "2value2"};
    myspan.AddTag("key", "3value3");
    {
      const tracing::TagScope tag2{"key", "4value4"};
      myspan.AddTag("key", "5value5");

      LOG_INFO() << "1";
      logging::LogFlush();
      auto logs = GetStreamString();
      ASSERT_THAT(logs, testing::Not(testing::HasSubstr("key=value")));
      ASSERT_THAT(logs, testing::Not(testing::HasSubstr("key=2value2")));
      ASSERT_THAT(logs, testing::Not(testing::HasSubstr("key=3value3")));
      ASSERT_THAT(logs, testing::Not(testing::HasSubstr("key=4value4")));
      ASSERT_THAT(logs, testing::HasSubstr("key=5value5"));
    }
    ClearLog();

    LOG_INFO() << "1";
    logging::LogFlush();
    auto logs = GetStreamString();
    ASSERT_THAT(logs, testing::Not(testing::HasSubstr("key=value")));
    ASSERT_THAT(logs, testing::Not(testing::HasSubstr("key=2value2")));
    ASSERT_THAT(logs, testing::HasSubstr("key=3value3"));
    ASSERT_THAT(logs, testing::Not(testing::HasSubstr("key=4value4")));
    ASSERT_THAT(logs, testing::Not(testing::HasSubstr("key=5value5")));
  }
  ClearLog();

  LOG_INFO() << "1";
  logging::LogFlush();
  auto logs = GetStreamString();
  ASSERT_THAT(logs, testing::HasSubstr("key=value"));
  ASSERT_THAT(logs, testing::Not(testing::HasSubstr("key=2value2")));
  ASSERT_THAT(logs, testing::Not(testing::HasSubstr("key=3value3")));
  ASSERT_THAT(logs, testing::Not(testing::HasSubstr("key=4value4")));
  ASSERT_THAT(logs, testing::Not(testing::HasSubstr("key=5value5")));
}

UTEST_F(TagScopeTest, NewRegularTagInScope) {
  tracing::Span myspan{"myspan"};
  {
    const tracing::TagScope tag{"key", "value"};
    myspan.AddTag("foo", "bar");

    LOG_INFO() << "1";
    logging::LogFlush();
    auto logs = GetStreamString();
    ASSERT_THAT(logs, testing::HasSubstr("key=value"));
    ASSERT_THAT(logs, testing::HasSubstr("foo=bar"));
  }
  ClearLog();

  LOG_INFO() << "1";
  logging::LogFlush();
  auto logs = GetStreamString();
  ASSERT_THAT(logs, testing::Not(testing::HasSubstr("key=value")));
  ASSERT_THAT(logs, testing::HasSubstr("foo=bar"));
}

UTEST_F(TagScopeTest, NewRegularTagInScopeDouble) {
  tracing::Span myspan{"myspan"};
  {
    const tracing::TagScope tag1{"key", "value"};
    myspan.AddTag("foo", "bar");
    {
      const tracing::TagScope tag2{"key", "2value2"};
      myspan.AddTag("baz", "zoo");

      LOG_INFO() << "1";
      logging::LogFlush();
      auto logs = GetStreamString();
      ASSERT_THAT(logs, testing::Not(testing::HasSubstr("key=value")));
      ASSERT_THAT(logs, testing::HasSubstr("key=2value2"));
      ASSERT_THAT(logs, testing::HasSubstr("foo=bar"));
      ASSERT_THAT(logs, testing::HasSubstr("baz=zoo"));
    }
    ClearLog();

    LOG_INFO() << "1";
    logging::LogFlush();
    auto logs = GetStreamString();
    ASSERT_THAT(logs, testing::HasSubstr("key=value"));
    ASSERT_THAT(logs, testing::Not(testing::HasSubstr("key=2value2")));
    ASSERT_THAT(logs, testing::HasSubstr("foo=bar"));
    ASSERT_THAT(logs, testing::HasSubstr("baz=zoo"));
  }
  ClearLog();

  LOG_INFO() << "1";
  logging::LogFlush();
  auto logs = GetStreamString();
  ASSERT_THAT(logs, testing::Not(testing::HasSubstr("key=value")));
  ASSERT_THAT(logs, testing::Not(testing::HasSubstr("key=2value2")));
  ASSERT_THAT(logs, testing::HasSubstr("foo=bar"));
  ASSERT_THAT(logs, testing::HasSubstr("baz=zoo"));
}

UTEST_F(TagScopeTest, Useless) {
  tracing::Span myspan{"myspan"};
  const tracing::TagScope tag{myspan, "key", "value",
                              logging::LogExtra::ExtendType::kFrozen};
  const tracing::TagScope useless_tag{myspan, "key", "value2",
                                      logging::LogExtra::ExtendType::kFrozen};

  LOG_INFO() << "1";
  logging::LogFlush();
  auto logs = GetStreamString();
  ASSERT_THAT(logs, testing::HasSubstr("key=value"));
  ASSERT_THAT(logs, testing::Not(testing::HasSubstr("key=value2")));
}

UTEST_F(TagScopeTest, FrozenOverwritesRegular) {
  tracing::Span myspan{"myspan"};
  const tracing::TagScope to_be_overwritten{myspan, "key", "value"};
  const tracing::TagScope tag{myspan, "key", "2value2",
                              logging::LogExtra::ExtendType::kFrozen};

  LOG_INFO() << "1";
  logging::LogFlush();
  auto logs = GetStreamString();
  ASSERT_THAT(logs, testing::HasSubstr("key=2value2"));
  ASSERT_THAT(logs, testing::Not(testing::HasSubstr("key=value")));
}

UTEST_F(TagScopeTest, RegularTriesToOverwriteFrozen) {
  tracing::Span myspan{"myspan"};
  const tracing::TagScope strong_tag{myspan, "key", "value",
                                     logging::LogExtra::ExtendType::kFrozen};
  const tracing::TagScope weak_tag{myspan, "key", "2value2"};

  LOG_INFO() << "1";
  logging::LogFlush();
  auto logs = GetStreamString();
  ASSERT_THAT(logs, testing::HasSubstr("key=value"));
  ASSERT_THAT(logs, testing::Not(testing::HasSubstr("key=2value2")));
}

UTEST_F(TagScopeTest, BasicLogExtra) {
  tracing::Span myspan{"myspan"};
  const tracing::TagScope tag_scope{
      myspan, logging::LogExtra{std::make_pair("key", "value"),
                                std::make_pair("foo", 123)}};

  LOG_INFO() << "1";
  logging::LogFlush();
  auto logs = GetStreamString();
  ASSERT_THAT(logs, testing::HasSubstr("key=value"));
  ASSERT_THAT(logs, testing::HasSubstr("foo=123"));
}

UTEST_F(TagScopeTest, BasicLogExtraFrozen) {
  tracing::Span myspan{"myspan"};
  const tracing::TagScope tag_scope{
      myspan, logging::LogExtra(
                  {std::make_pair("key", "value"), std::make_pair("foo", 123)},
                  logging::LogExtra::ExtendType::kFrozen)};

  LOG_INFO() << "1";
  logging::LogFlush();
  auto logs = GetStreamString();
  ASSERT_THAT(logs, testing::HasSubstr("key=value"));
  ASSERT_THAT(logs, testing::HasSubstr("foo=123"));
}

UTEST_F(TagScopeTest, AutomaticSpanFindingLogExtra) {
  tracing::Span myspan{"myspan"};
  const tracing::TagScope tag_scope{logging::LogExtra{
      std::make_pair("key", "value"), std::make_pair("foo", 123)}};

  LOG_INFO() << "1";
  logging::LogFlush();
  auto logs = GetStreamString();
  ASSERT_THAT(logs, testing::HasSubstr("key=value"));
  ASSERT_THAT(logs, testing::HasSubstr("foo=123"));
}

UTEST_F(TagScopeTest, DestructorRollbacksToPreviousValueLogExtra) {
  tracing::Span myspan{"myspan"};
  myspan.AddTag("foo", 123);
  {
    myspan.AddTag("key", "value");
    const tracing::TagScope tag_scope{
        myspan, logging::LogExtra{std::make_pair("key", "2value2"),
                                  std::make_pair("foo", 321)}};
  }
  LOG_INFO() << "1";
  logging::LogFlush();
  const auto logs = GetStreamString();
  ASSERT_THAT(logs, testing::HasSubstr("key=value"));
  ASSERT_THAT(logs, testing::Not(testing::HasSubstr("key=2value2")));
  ASSERT_THAT(logs, testing::HasSubstr("foo=123"));
  ASSERT_THAT(logs, testing::Not(testing::HasSubstr("foo=321")));
}

UTEST_F(TagScopeTest, ForgetLogExtra) {
  tracing::Span myspan{"myspan"};
  const tracing::TagScope forgotten_tag_scope{
      myspan, logging::LogExtra{std::make_pair("key", "value"),
                                std::make_pair("foo", 123)}};
  const tracing::TagScope overwriting_tag_scope{
      myspan, logging::LogExtra{std::make_pair("key", "2value2"),
                                std::make_pair("foo", 321)}};

  LOG_INFO() << "1";
  logging::LogFlush();
  auto logs = GetStreamString();
  ASSERT_THAT(logs, testing::HasSubstr("key=2value2"));
  ASSERT_THAT(logs, testing::Not(testing::HasSubstr("key=value")));
  ASSERT_THAT(logs, testing::HasSubstr("foo=321"));
  ASSERT_THAT(logs, testing::Not(testing::HasSubstr("foo=123")));
}

UTEST_F(TagScopeTest, RegularTagBrokenLogExtra) {
  tracing::Span myspan{"myspan"};
  {
    const tracing::TagScope tag_scope{logging::LogExtra{
        std::make_pair("key", "value"), std::make_pair("foo", 123)}};
    myspan.AddTag("key", "another_value");
    myspan.AddTagFrozen("foo", "bar");
  }
  LOG_INFO() << "1";
  logging::LogFlush();
  auto logs = GetStreamString();
  ASSERT_THAT(logs, testing::Not(testing::HasSubstr("key")));
  ASSERT_THAT(logs, testing::Not(testing::HasSubstr("value")));
  ASSERT_THAT(logs, testing::Not(testing::HasSubstr("foo")));
  ASSERT_THAT(logs, testing::Not(testing::HasSubstr("bar")));
  // this happens because TagScope does not check value.
}

UTEST_F(TagScopeTest, RegularAndFrozenTagLogExtra) {
  tracing::Span myspan{"myspan"};
  logging::LogExtra extra1;
  extra1.Extend(std::make_pair("key", "value"));
  extra1.Extend(std::make_pair("foo", 123),
                logging::LogExtra::ExtendType::kFrozen);
  tracing::TagScope tag_scope1{std::move(extra1)};
  {
    logging::LogExtra extra2;
    extra2.Extend(std::make_pair("key", "2value2"),
                  logging::LogExtra::ExtendType::kFrozen);
    extra2.Extend(std::make_pair("foo", 321));
    tracing::TagScope tag_scope2{std::move(extra2)};

    LOG_INFO() << "1";
    logging::LogFlush();
    auto logs = GetStreamString();
    ASSERT_THAT(logs, testing::Not(testing::HasSubstr("key=value")));
    ASSERT_THAT(logs, testing::HasSubstr("key=2value2"));
    ASSERT_THAT(logs, testing::HasSubstr("foo=123"));
    ASSERT_THAT(logs, testing::Not(testing::HasSubstr("foo=321")));
  }
  ClearLog();

  LOG_INFO() << "1";
  logging::LogFlush();
  auto logs = GetStreamString();
  ASSERT_THAT(logs, testing::HasSubstr("key=value"));
  ASSERT_THAT(logs, testing::Not(testing::HasSubstr("key=2value2")));
  ASSERT_THAT(logs, testing::HasSubstr("foo=123"));
  ASSERT_THAT(logs, testing::Not(testing::HasSubstr("foo=321")));
}

USERVER_NAMESPACE_END
