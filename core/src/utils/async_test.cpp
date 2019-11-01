#include <utest/utest.hpp>

#include <utils/async.hpp>

TEST(UtilsAsync, Base) {
  RunInCoro([]() {
    auto task = utils::Async("async", [] { return 1; });
    EXPECT_EQ(1, task.Get());
  });
}
