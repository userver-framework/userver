#include "thread_control.hpp"

#include <memory>
#include <stdexcept>

#include <engine/ev/thread_pool.hpp>
#include <engine/ev/thread_pool_config.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::ev {

UTEST(ThreadControl, RunInEvLoopSyncWithResult) {
    engine::ev::ThreadPool thread_pool{engine::ev::ThreadPoolConfig{1, "thread_control_test"}};
    auto thread_control = thread_pool.NextThread();

    EXPECT_FALSE(thread_control.IsInEvThread());
    bool invoked = false;
    thread_control.RunInEvLoopSyncWithResult([&] {
        EXPECT_TRUE(thread_control.IsInEvThread());
        invoked = true;
    });
    EXPECT_TRUE(invoked);

    auto result = thread_control.RunInEvLoopSyncWithResult([] { return std::make_unique<int>(42); });
    ASSERT_TRUE(result);
    EXPECT_EQ(*result, 42);

    const auto inline_result = thread_control.RunInEvLoopSyncWithResult([&] {
        EXPECT_TRUE(thread_control.IsInEvThread());
        return thread_control.RunInEvLoopSyncWithResult([] { return 17; });
    });
    EXPECT_EQ(inline_result, 17);

    UEXPECT_THROW(
        thread_control.RunInEvLoopSyncWithResult([] { throw std::runtime_error{"test error"}; }),
        std::runtime_error
    );
}

}  // namespace engine::ev

USERVER_NAMESPACE_END
