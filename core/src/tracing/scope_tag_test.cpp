#include <logging/logging_test.hpp>
#include <userver/tracing/scope_tag.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

void CheckContains(std::string_view doc, std::string_view to_find) {
  ASSERT_NE(doc.find(to_find), std::string_view::npos)
      << std::quoted(to_find) << " was not found inside:\n"
      << std::quoted(doc);
}
void CheckNotContains(std::string_view doc, std::string_view to_find) {
  ASSERT_EQ(doc.find(to_find), std::string_view::npos)
      << std::quoted(to_find) << " was found inside:\n"
      << std::quoted(doc);
}

template <typename ScopeTagsType>
struct ScopeTagsTest : public LoggingTest {
  using TagType = ScopeTagsType;
};
using TagTypes = ::testing::Types<tracing::ScopeTag, tracing::FrozenScopeTag>;
TYPED_UTEST_SUITE(ScopeTagsTest, TagTypes);

TYPED_UTEST(ScopeTagsTest, Basic) {
  tracing::Span myspan{"myspan"};
  const typename TestFixture::TagType s{myspan, "key", "value"};
  LOG_INFO() << "1";
  logging::LogFlush();
  CheckContains(this->GetStreamString(), "key=value");
}

TYPED_UTEST(ScopeTagsTest, IntegerType) {
  tracing::Span myspan{"myspan"};
  const typename TestFixture::TagType s{myspan, "key", 123};
  LOG_INFO() << "1";
  logging::LogFlush();
  CheckContains(this->GetStreamString(), "key=123");
}
TYPED_UTEST(ScopeTagsTest, AutomaticSpanFinding) {
  tracing::Span myspan{"myspan"};
  const typename TestFixture::TagType s{"key", 123};
  LOG_INFO() << "1";
  logging::LogFlush();
  CheckContains(this->GetStreamString(), "key=123");
}

TYPED_UTEST(ScopeTagsTest, MissedItsScope) {
  tracing::Span myspan{"myspan"};
  { const typename TestFixture::TagType s{myspan, "key", "value"}; }

  LOG_INFO() << "1";
  logging::LogFlush();
  this->GetStreamString();
  const std::string log = this->GetStreamString();
  CheckNotContains(log, "value");
  CheckNotContains(log, "key");
}

TYPED_UTEST(ScopeTagsTest, DestructorRollbacksToPreviousValue) {
  tracing::Span myspan{"myspan"};
  {
    myspan.AddTag("key", "value");
    const typename TestFixture::TagType tag{myspan, "key", "value2"};
  }
  LOG_INFO() << "1";
  logging::LogFlush();
  const auto logs = this->GetStreamString();
  CheckContains(logs, "key=value");
  CheckNotContains(logs, "key=value2");
}

TYPED_UTEST(ScopeTagsTest, AttemptToOverwriteFrozenTag) {
  tracing::Span myspan{"myspan"};
  myspan.AddTagFrozen("key", "value");
  {
    const typename TestFixture::TagType ignored{myspan, "key", "value2"};
    LOG_INFO() << "1";
    logging::LogFlush();
    auto logs = this->GetStreamString();
    CheckContains(logs, "key=value");
    CheckNotContains(logs, "key=value2");
  }
  this->ClearLog();

  LOG_INFO() << "1";
  logging::LogFlush();
  auto logs = this->GetStreamString();
  CheckContains(logs, "key=value");
  CheckNotContains(logs, "key=value2");
}


struct ScopeTagTest : LoggingTest {};
struct FrozenScopeTagTest : LoggingTest {};

UTEST_F(ScopeTagTest, Forget) {
  tracing::Span myspan{"myspan"};
  const tracing::ScopeTag forgotten_tag{myspan, "key", "value"};
  const tracing::ScopeTag overwriting_tag{myspan, "key", "2value2"};

  LOG_INFO() << "1";
  logging::LogFlush();
  auto logs = GetStreamString();
  CheckContains(logs, "key=2value2");
  CheckNotContains(logs, "key=value");
}

UTEST_F(ScopeTagTest, RegularTagBroken) {
  tracing::Span myspan{"myspan"};
  {
    const tracing::ScopeTag tag{"key", "value"};
    myspan.AddTag("key", "value");
  }
  LOG_INFO() << "1";
  logging::LogFlush();
  auto logs = GetStreamString();
  CheckNotContains(logs, "key");
  CheckNotContains(logs, "value");
  // this happens because there is no way for ScopeTag::~ScopeTag() to detect that it doesn't own anything already.
}



UTEST_F(FrozenScopeTagTest, Useless) {
  tracing::Span myspan{"myspan"};
  const tracing::FrozenScopeTag tag{myspan, "key", "value"};
  const tracing::FrozenScopeTag useless_tag{myspan, "key", "value2"};

  LOG_INFO() << "1";
  logging::LogFlush();
  auto logs = GetStreamString();
  CheckContains(logs, "key=value");
  CheckNotContains(logs, "key=value2");
}

struct MixedTagsTest : LoggingTest {};

UTEST_F(MixedTagsTest, FrozenOverwritesRegular) {
  tracing::Span myspan{"myspan"};
  const tracing::ScopeTag to_be_overwritten{myspan, "key", "value"};
  const tracing::FrozenScopeTag tag{myspan, "key", "2value2"};

  LOG_INFO() << "1";
  logging::LogFlush();
  auto logs = GetStreamString();
  CheckContains(logs, "key=2value2");
  CheckNotContains(logs, "key=value");
}
UTEST_F(MixedTagsTest, RegularTriesToOverwriteFrozen) {
  tracing::Span myspan{"myspan"};
  const tracing::FrozenScopeTag strong{myspan, "key", "value"};
  const tracing::ScopeTag weak{myspan, "key", "2value2"};

  LOG_INFO() << "1";
  logging::LogFlush();
  auto logs = GetStreamString();
  CheckContains(logs, "key=value");
  CheckNotContains(logs, "key=2value2");
}

USERVER_NAMESPACE_END