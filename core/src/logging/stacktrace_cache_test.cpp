#include <utest/utest.hpp>

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <boost/stacktrace.hpp>

#include <logging/stacktrace_cache.hpp>
#include <utils/text.hpp>

TEST(stacktrace_cache, full) {
  auto st = boost::stacktrace::stacktrace();
  EXPECT_EQ(fmt::to_string(st), logging::stacktrace_cache::to_string(st));
}

UTEST(stacktrace_cache, StartOfCoroutine) {
  auto st = boost::stacktrace::stacktrace();
  auto text = logging::stacktrace_cache::to_string(st);
  EXPECT_TRUE(utils::text::EndsWith(text, "[start of coroutine]\n")) << text;
}
