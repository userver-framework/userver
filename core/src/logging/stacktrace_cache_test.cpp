#include <utest/utest.hpp>

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <boost/stacktrace.hpp>

#include <logging/stacktrace_cache.hpp>
#include <utils/text.hpp>

TEST(stacktrace_cache, frame) {
  auto st = boost::stacktrace::stacktrace();
  auto orig_name = boost::stacktrace::to_string(st[0]);
  auto cache_name = logging::stacktrace_cache::to_string(st[0]);
  EXPECT_EQ(orig_name, cache_name);
}

TEST(stacktrace_cache, full) {
  auto st = boost::stacktrace::stacktrace();
  EXPECT_EQ(fmt::to_string(st), logging::stacktrace_cache::to_string(st));
}

TEST(stacktrace_cache, StartOfCoroutine) {
  RunInCoro([] {
    auto st = boost::stacktrace::stacktrace();
    auto text = logging::stacktrace_cache::to_string(st);
    EXPECT_TRUE(utils::text::EndsWith(text, "[start of coroutine]\n")) << text;
  });
}
