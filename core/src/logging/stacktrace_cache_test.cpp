#include <utest/utest.hpp>

#include <logging/stacktrace_cache.hpp>

TEST(stacktrace_cache, frame) {
  auto st = boost::stacktrace::stacktrace();
  auto orig_name = boost::stacktrace::to_string(st[0]);
  auto cache_name = logging::stacktrace_cache::to_string(st[0]);
  EXPECT_EQ(orig_name, cache_name);
}

#ifndef __MACH__
TEST(stacktrace_cache, full) {
  auto st = boost::stacktrace::stacktrace();
  EXPECT_EQ(boost::stacktrace::to_string(st),
            logging::stacktrace_cache::to_string(st));
}
#endif  // __MACH__
