#include <userver/utest/utest.hpp>

#include <stdexcept>
#include <vector>

#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/wait_all_checked.hpp>
#include <userver/utest/stress.hpp>
#include <userver/utils/rand.hpp>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

namespace {

engine::TaskWithResult<void> FastFailingTask() {
    return engine::AsyncNoTracing([] {
        engine::InterruptibleSleepFor(20ms);
        throw std::runtime_error{"failfast_exception"};
    });
}

engine::TaskWithResult<void> FastSuccessfulTask() {
    return engine::AsyncNoTracing([] { engine::InterruptibleSleepFor(20ms); });
}

}  // namespace

UTEST_MT(WaitAllChecked, ExceptionStressTest, 16) {
    for (auto _ : utest::StressLoop()) {
        const std::size_t task_count = utils::RandRange(100u, 500u);
        const std::size_t failing = utils::RandRange(std::size_t{0}, task_count);
        std::vector<engine::Task> tasks;
        tasks.reserve(task_count);
        for (std::size_t i = 0; i < failing; ++i) {
            tasks.push_back(FastSuccessfulTask());
        }
        tasks.push_back(FastFailingTask());
        for (std::size_t i = failing + 1; i < task_count; ++i) {
            tasks.push_back(FastSuccessfulTask());
        }
        UEXPECT_THROW(engine::WaitAllChecked(tasks), std::runtime_error);
    }
}

USERVER_NAMESPACE_END
