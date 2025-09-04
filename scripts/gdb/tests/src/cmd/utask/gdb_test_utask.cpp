#include <boost/stacktrace.hpp>

#include <userver/engine/condition_variable.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/wait_all_checked.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/async.hpp>

#include <userver/gdb_tests/stub.hpp>

USERVER_NAMESPACE_BEGIN

__attribute__((noinline)) static void TestNonCoroutineCtx() {
    TEST_COMMAND("assert_matches('Task\\s+State\\s+Span', gdb.execute('utask list', to_string=True))");
    TEST_COMMAND("assert_matches('No tasks found', gdb.execute('utask apply all bt', to_string=True))");
}

__attribute__((noinline)) static void TestSingleCoroutine() {
    engine::RunStandalone([&] {
        void* volatile current_task = engine::current_task::impl::GetRawCurrentTaskContext();
        auto span_name = tracing::Span::CurrentSpan().GetName();
        TEST_COMMAND(
            "current_task = str(gdb.parse_and_eval('current_task'))\n"
            "span_name = re.escape(str(gdb.parse_and_eval('span_name')).strip('\"'))\n"
            "assert_matches(\n"
            "   f'Task\\\\s+State\\\\s+Span\\n{current_task}\\\\s+Running\\\\s+{span_name}',\n"
            "   gdb.execute('utask list', to_string=True)\n"
            ")"
        );
        DoNotOptimize(current_task);
        DoNotOptimize(span_name);

        auto current_frames = boost::stacktrace::stacktrace().as_vector();
        std::vector<const void*> addresses;
        addresses.reserve(current_frames.size());
        for (auto& frame : current_frames) {
            addresses.push_back(frame.address());
        }
        TEST_COMMAND(
            "def get_backtrace(task: str):\n"
            "   return gdb.execute(f'utask apply {task} bt -frame-info location-and-address', to_string=True)\n"
            "bracktrace_all = get_backtrace('all')\n"
            "backtrace_by_id = get_backtrace(str(gdb.parse_and_eval('current_task')))\n"
            "backtrace_by_span = get_backtrace('span')\n"
            "addresses_regexp = '.*'.join(\n"
            "   f'{int(addr):#0{18}x}'\n"
            "   for _, addr in list(gdb.default_visualizer(gdb.parse_and_eval('addresses')).children())[1:]\n"
            ")\n"
            "assert_matches(addresses_regexp, bracktrace_all, re.DOTALL)\n"
            "assert_matches(addresses_regexp, backtrace_by_id, re.DOTALL)\n"
            "assert_matches(addresses_regexp, backtrace_by_span, re.DOTALL)\n"
            "try:\n"
            "   gdb.execute('utask apply non_existent_task bt')\n"
            "except gdb.error as e:\n"
            "   assert_matches('Task \"non_existent_task\" not found', str(e))\n"
            "else:\n"
            "   assert False, 'error was expected'"
        );
        DoNotOptimize(addresses);
    });
}

__attribute__((noinline)) static void TestSleepingCoroutine(bool no_span = false) {
    engine::RunStandalone([&] {
        void* volatile root_coro = engine::current_task::impl::GetRawCurrentTaskContext();
        const std::string root_coro_name{tracing::Span::CurrentSpan().GetName()};
        auto root_stacktrace = boost::stacktrace::stacktrace();
        auto payload = [&] {
            void* volatile new_coro = engine::current_task::impl::GetRawCurrentTaskContext();
            void* volatile root_coro_addr = root_coro;
            bool volatile no_span_ = no_span;
            TEST_COMMAND(
                "no_span = bool(gdb.parse_and_eval('no_span_'))\n"
                "new_coro_span = 'new_coro' if not no_span else ''\n"
                "assert_matches(\n"
                "   'Task\\\\s+State\\\\s+Span\\n'\n"
                "   f'{gdb.parse_and_eval(\"root_coro_addr\")}\\\\s+Suspended\\\\s+span\\n'\n"
                "   f'{gdb.parse_and_eval(\"new_coro\")}\\\\s+Running\\\\s+{new_coro_span}',\n"
                "   gdb.execute('utask list', to_string=True)\n"
                ")"
            );
            std::vector<const void*> root_frames_addrs;
            root_frames_addrs.reserve(root_stacktrace.size());
            for (auto& frame : root_stacktrace.as_vector()) {
                root_frames_addrs.push_back(frame.address());
            }
            TEST_COMMAND(
                "root_coro_addr = gdb.parse_and_eval('root_coro_addr')\n"
                "backtrace = gdb.execute(\n"
                "   f'utask apply {root_coro_addr} bt -frame-info location-and-address', to_string=True\n"
                ")\n"
                "root_frames_addrs = gdb.parse_and_eval('root_frames_addrs')\n"
                "addresses_regexp = '.*'.join(\n"
                "   f'{int(addr):#0{18}x}'\n"
                "   for _, addr in list(gdb.default_visualizer(root_frames_addrs).children())[1:]\n"
                ")\n"
                "assert_matches(addresses_regexp, backtrace, re.DOTALL)"
            );
            DoNotOptimize(new_coro);
            DoNotOptimize(root_coro_addr);
            DoNotOptimize(no_span_);
            DoNotOptimize(root_frames_addrs);
        };
        if (no_span) {
            engine::AsyncNoSpan(payload).Wait();
        } else {
            utils::Async("new_coro", payload).Wait();
        }
    });
}

__attribute__((noinline)) static void TestMultipleCoroutines(int tasks_cnt = 11, int threads = 1) {
    engine::RunStandalone(threads, [&] {
        engine::Mutex mutex;
        std::atomic<int> cnt = 0;
        std::atomic<bool> finished = false;
        engine::ConditionVariable cv;
        std::vector<engine::TaskWithResult<void>> tasks;
        tasks.reserve(tasks_cnt);

        std::unique_lock lock(mutex);
        for (int i = 0; i < tasks_cnt; ++i) {
            tasks.emplace_back(utils::Async("task_" + std::to_string(i), [&] {
                engine::Yield();
                if (cnt.fetch_add(1) == tasks_cnt - 1) {
                    const std::lock_guard lock(mutex);
                    void* volatile current_task = engine::current_task::impl::GetRawCurrentTaskContext();
                    auto& tasks_ref = tasks;
                    auto& threads_ref = threads;
                    MAKE_COREDUMP_AND_SWITCH_TO();
                    TEST_COMMAND(
                        "tasks_list_output = gdb.execute('utask list', to_string=True)\n"
                        "for i, (_, task) in enumerate(\n"
                        "   gdb.default_visualizer(gdb.parse_and_eval('tasks_ref')).children()\n"
                        "):\n"
                        "   task_ctx = task['pimpl_'].cast(\n"
                        "      gdb.lookup_type(f'{USERVER_NAMESPACE}engine::TaskBase::Impl')\n"
                        "   )['context']['px']\n"
                        "   if gdb.parse_and_eval('threads_ref') == 1:\n"
                        "      if int(task_ctx) == int(gdb.parse_and_eval('current_task')): state = 'Running'\n"
                        "      else: state = '(Suspended|Queued)'\n"
                        "   else: state = '(Running|Suspended|Queued)'\n"
                        "   assert_matches(f'{task_ctx}\\\\s+{state}\\\\s+task_{i}', tasks_list_output)\n"
                        "if gdb.parse_and_eval('threads_ref') != 1:\n"
                        "   assert_matches('(Running.*){1,4}', tasks_list_output)\n",
                        test_in_coredump = True,
                    );
                    cv.NotifyAll();
                    finished.store(true);
                    DoNotOptimize(current_task);
                    DoNotOptimize(tasks_ref);
                    DoNotOptimize(threads_ref);
                } else {
                    std::unique_lock lock(mutex);
                    if (!finished.load()) {
                        (void)cv.Wait(lock);
                    }
                }
            }));
        }
        lock.unlock();
        engine::WaitAllChecked(tasks);
    });
}

__attribute__((noinline)) void BenchmarkHeavyService(size_t tasks_cnt, size_t threads_cnt, size_t memory_per_task) {
    engine::RunStandalone(threads_cnt, [&] {
        const struct RecursivePayload {
            void operator()(size_t index) const {
                std::vector<std::vector<int>> some_used_memory(1000, std::vector<int>(memory_per_task / 1000, 1));
                if (index == tasks_cnt) {
                    auto volatile tasks_cnt_ = tasks_cnt;
                    auto volatile threads_cnt_ = threads_cnt;
                    auto volatile memory_per_task_ = memory_per_task;
                    TEST_COMMAND(
                        "tasks_cnt = int(gdb.parse_and_eval('tasks_cnt_'))\n"
                        "threads_cnt = int(gdb.parse_and_eval('threads_cnt_'))\n"
                        "memory_per_task = int(gdb.parse_and_eval('memory_per_task_'))\n"
                        "start_benchmark(tasks_cnt, threads_cnt, memory_per_task)\n"
                        "with measure_time('`utask list` (first call)'):\n"
                        "   gdb.execute('utask list', to_string=True)\n"
                        "with measure_time('`utask list` (second call)'):\n"
                        "   gdb.execute('utask list', to_string=True)\n"
                        "with measure_time('`utask apply all bt` (first call)'):\n"
                        "   gdb.execute('utask apply all bt', to_string=True)\n"
                        "with measure_time('`utask apply all bt` (second call)'):\n"
                        "   gdb.execute('utask apply all bt', to_string=True)\n"
                    );
                    DoNotOptimize(tasks_cnt_);
                    DoNotOptimize(threads_cnt_);
                    DoNotOptimize(memory_per_task_);
                } else {
                    utils::Async("task_" + std::to_string(index + 1), &RecursivePayload::operator(), *this, index + 1)
                        .Wait();
                }
                DoNotOptimize(some_used_memory);
            }
            size_t tasks_cnt{};
            size_t threads_cnt{};
            size_t memory_per_task{};
        } payload{tasks_cnt, threads_cnt, memory_per_task};

        utils::Async("task_1", payload, 1).Wait();
    });
}

void RunTests() {
    TestNonCoroutineCtx();
    TestSingleCoroutine();
    TestSleepingCoroutine();
    TestSleepingCoroutine(true);
    TestMultipleCoroutines();
    TestMultipleCoroutines(100, 8);
    TestMultipleCoroutines(1000, 32);
}

void RunBenchmarks() {
    BenchmarkHeavyService(100, 8, 1'000);
    BenchmarkHeavyService(100, 8, 10'000'000);
    BenchmarkHeavyService(500, 32, 1'000'000);
    BenchmarkHeavyService(1000, 32, 100'000);
    BenchmarkHeavyService(2000, 32, 100'000);
}

USERVER_NAMESPACE_END

int main() {
    USERVER_NAMESPACE::RunTests();
    USERVER_NAMESPACE::RunBenchmarks();
}
