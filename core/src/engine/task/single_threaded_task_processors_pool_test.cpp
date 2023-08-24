#include <userver/components/single_threaded_task_processors.hpp>

#include <engine/task/task_processor_config.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/strong_typedef.hpp>

#include <array>
#include <thread>

#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {
using Pool = engine::SingleThreadedTaskProcessorsPool;

std::thread::id GetThreadId(engine::TaskProcessor& tp) {
  return utils::Async(tp, "test", [] { return std::this_thread::get_id(); })
      .Get();
}

using FourThreadIds = utils::StrongTypedef<class FourThreadIdsTag,
                                           std::array<std::thread::id, 4>>;

FourThreadIds GetArrayOfThreadIds(Pool& pool) {
  return FourThreadIds{std::array<std::thread::id, 4>{
      GetThreadId(pool.At(0)),
      GetThreadId(pool.At(1)),
      GetThreadId(pool.At(2)),
      GetThreadId(pool.At(3)),
  }};
}

std::ostream& operator<<(std::ostream& os, FourThreadIds v) {
  return os << '[' << v[0] << ',' << v[1] << ',' << v[2] << ',' << v[3] << ']';
}
}  // namespace

UTEST_MT(SingleThreadedTaskprocessor, ConstructionWithoutComponentSystem, 2) {
  constexpr unsigned kRepetitions = 10;
  engine::TaskProcessorConfig config;
  config.name = "test";
  config.worker_threads = 4;

  Pool pool{config};
  EXPECT_EQ(pool.GetSize(), config.worker_threads);

  const auto ethalon_ids = GetArrayOfThreadIds(pool);
  {
    // Checking for unique IDs
    auto ids = ethalon_ids;
    std::sort(ids.begin(), ids.end());
    ASSERT_TRUE(std::unique(ids.begin(), ids.end()) == ids.end())
        << "Thread IDs returned from Pool have same values " << ethalon_ids
        << ". Logic error, IDs should be different!";
  }

  for (unsigned i = 0; i < kRepetitions; ++i) {
    EXPECT_EQ(ethalon_ids, GetArrayOfThreadIds(pool)) << "on iteration " << i;
  }
}

USERVER_NAMESPACE_END
