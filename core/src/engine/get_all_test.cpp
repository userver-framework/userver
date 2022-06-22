#include <userver/engine/get_all.hpp>

#include <type_traits>
#include <vector>

#include <userver/engine/async.hpp>
#include <userver/engine/future.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

UTEST(GetAll, Empty) {
  static_assert(std::is_void_v<decltype(engine::GetAll())>);
  UEXPECT_NO_THROW(engine::GetAll());
}

UTEST(GetAll, SingleVoid) {
  auto task = engine::AsyncNoSpan([] {});
  static_assert(std::is_void_v<decltype(engine::GetAll(task))>);
  UEXPECT_NO_THROW(engine::GetAll(task));
  EXPECT_FALSE(task.IsValid());
}

UTEST(GetAll, SingleResult) {
  auto task = engine::AsyncNoSpan([] { return 42; });
  EXPECT_EQ(engine::GetAll(task), std::vector{42});
  EXPECT_FALSE(task.IsValid());
}

UTEST(GetAll, MultipleVoid) {
  auto task1 = engine::AsyncNoSpan([] {});
  auto task2 = engine::AsyncNoSpan([] {});
  static_assert(std::is_void_v<decltype(engine::GetAll(task1, task2))>);
  UEXPECT_NO_THROW(engine::GetAll(task1, task2));
  EXPECT_FALSE(task1.IsValid());
  EXPECT_FALSE(task2.IsValid());
}

UTEST(GetAll, MultipleResult) {
  auto task1 = engine::AsyncNoSpan([] { return 1; });
  auto task2 = engine::AsyncNoSpan([] { return 2; });
  EXPECT_EQ(engine::GetAll(task1, task2), (std::vector{1, 2}));
  EXPECT_FALSE(task1.IsValid());
  EXPECT_FALSE(task2.IsValid());
}

UTEST(GetAll, ContainerVoid) {
  for (std::size_t task_count = 0; task_count < 10; ++task_count) {
    std::vector<engine::TaskWithResult<void>> tasks;
    for (std::size_t i = 0; i < task_count; ++i) {
      tasks.push_back(engine::AsyncNoSpan([] {}));
    }
    static_assert(std::is_void_v<decltype(engine::GetAll(tasks))>);
    UEXPECT_NO_THROW(engine::GetAll(tasks));
    for (auto& task : tasks) {
      EXPECT_FALSE(task.IsValid());
    }
  }
}

UTEST(GetAll, ContainerResult) {
  for (std::size_t task_count = 0; task_count < 10; ++task_count) {
    std::vector<engine::TaskWithResult<std::size_t>> tasks;
    std::vector<std::size_t> expected_result;
    for (std::size_t i = 0; i < task_count; ++i) {
      tasks.push_back(engine::AsyncNoSpan([i] { return i; }));
      expected_result.push_back(i);
    }
    EXPECT_EQ(engine::GetAll(tasks), expected_result);
    for (auto& task : tasks) {
      EXPECT_FALSE(task.IsValid());
    }
  }
}

USERVER_NAMESPACE_END
