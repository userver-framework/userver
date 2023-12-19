#include <userver/utest/utest.hpp>

#include <boost/stacktrace.hpp>

#include <userver/logging/stacktrace_cache.hpp>
#include <userver/utils/text.hpp>

USERVER_NAMESPACE_BEGIN

TEST(StacktraceCache, full) {
  logging::stacktrace_cache::StacktraceGuard guard(true);
  auto st = boost::stacktrace::stacktrace();
  EXPECT_EQ(boost::stacktrace::to_string(st),
            logging::stacktrace_cache::to_string(st));
}

UTEST(StacktraceCache, StartOfCoroutine) {
  logging::stacktrace_cache::StacktraceGuard guard(true);
  auto st = boost::stacktrace::stacktrace();
  auto text = logging::stacktrace_cache::to_string(st);
  EXPECT_TRUE(utils::text::EndsWith(text, "[start of coroutine]\n")) << text;
}

USERVER_NAMESPACE_END
