#include <userver/utest/utest.hpp>

#include <memory>
#include <string>
#include <utility>

#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/task/inherited_variable.hpp>
#include <userver/engine/task/shared_task_with_result.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

engine::TaskInheritedVariable<std::string> kStringVariable;
engine::TaskInheritedVariable<std::string> kStringVariable2;
engine::TaskInheritedVariable<std::string> kStringVariable3;

engine::TaskInheritedVariable<std::unique_ptr<int>> kPtrVariable;

engine::TaskInheritedVariable<std::pair<std::string, int>> kPairVariable;

}  // namespace

UTEST(TaskInheritedVariable, Empty) {
  EXPECT_EQ(kStringVariable.GetOptional(), nullptr);
  UEXPECT_THROW(kStringVariable.Get(), std::runtime_error);
}

UTEST(TaskInheritedVariable, MoveAndShared) {
  constexpr int kValue = 42;

  EXPECT_EQ(kPtrVariable.GetOptional(), nullptr);
  kPtrVariable.Set(std::make_unique<int>(kValue));

  const auto* data_opt = kPtrVariable.GetOptional();
  ASSERT_NE(data_opt, nullptr);
  EXPECT_EQ(**data_opt, kValue);
  EXPECT_EQ(*kPtrVariable.Get(), kValue);

  auto sub_task = utils::Async("subtask", [&] {
    const auto* data_opt = kPtrVariable.GetOptional();
    ASSERT_NE(data_opt, nullptr);
    EXPECT_EQ(**data_opt, kValue);
    EXPECT_EQ(*kPtrVariable.Get(), kValue);
  });
  sub_task.Get();
}

UTEST(TaskInheritedVariable, Emplace) {
  EXPECT_EQ(kPairVariable.GetOptional(), nullptr);
  kPairVariable.Emplace("aaa", 12);

  const auto* data_opt = kPairVariable.GetOptional();
  ASSERT_NE(data_opt, nullptr);
  const std::pair<std::string, int> expected("aaa", 12);
  EXPECT_EQ(*data_opt, expected);
  EXPECT_EQ(kPairVariable.Get(), expected);
}

UTEST(TaskInheritedVariable, IndependenceFromChildChanges) {
  constexpr std::string_view kValue1 = "value1";
  constexpr std::string_view kValue2 = "value2";
  constexpr std::string_view kValue3 = "value3";
  constexpr std::string_view kNewValue1 = "new_value1";

  kStringVariable.Emplace(kValue1);
  kStringVariable2.Emplace(kValue2);

  const auto check_data_unchanged = [&] {
    EXPECT_EQ(kStringVariable.Get(), kValue1);
    EXPECT_EQ(kStringVariable2.Get(), kValue2);
    UEXPECT_THROW(kStringVariable3.Get(), std::runtime_error);
  };

  check_data_unchanged();

  auto sub_task = utils::Async("subtask", [&] {
    // Test inheritance
    check_data_unchanged();

    kStringVariable.Emplace(kNewValue1);
    kStringVariable2.Erase();
    kStringVariable3.Emplace(kValue3);

    EXPECT_EQ(kStringVariable.Get(), kNewValue1);
    UEXPECT_THROW(kStringVariable2.Get(), std::runtime_error);
    EXPECT_EQ(kStringVariable3.Get(), kValue3);
  });

  sub_task.Get();

  // Subtask does not change data in parent task.
  check_data_unchanged();
}

UTEST(TaskInheritedVariable, IndependenceFromParentChanges) {
  constexpr std::string_view kValue2 = "value2";

  engine::SingleConsumerEvent signaled;

  kStringVariable2.Emplace(kValue2);

  const auto check_data_unchanged = [&] {
    EXPECT_EQ(kStringVariable2.Get(), kValue2);
  };

  check_data_unchanged();

  auto sub_task = utils::Async("subtask", [&] {
    // Test inheritance
    check_data_unchanged();

    EXPECT_TRUE(signaled.WaitForEvent());

    // Data in subtask does not change if parent task data was changed.
    check_data_unchanged();
  });

  kStringVariable2.Erase();

  UEXPECT_THROW(kStringVariable2.Get(), std::runtime_error);

  signaled.Send();

  sub_task.Get();
}

UTEST(TaskInheritedVariable, Overwrite) {
  constexpr std::string_view kValue1 = "value1";
  constexpr std::string_view kValue2 = "value2";

  kStringVariable.Emplace(kValue1);
  EXPECT_EQ(kStringVariable.Get(), kValue1);

  kStringVariable.Emplace(kValue2);
  EXPECT_EQ(kStringVariable.Get(), kValue2);

  kStringVariable.Erase();
  EXPECT_FALSE(kStringVariable.GetOptional());
  UEXPECT_THROW(kStringVariable.Get(), std::runtime_error);
}

UTEST(TaskInheritedVariable, SameObjectIsInherited) {
  kStringVariable.Set("foo");
  const auto* const kParentVariablePtr = &kStringVariable.Get();

  utils::Async("subtask", [&] {
    EXPECT_EQ(&kStringVariable.Get(), kParentVariablePtr);
  }).Get();
}

UTEST_MT(TaskInheritedVariable, VariablesAfterParentTaskDeath, 4) {
  using Event = engine::SingleConsumerEvent;
  Event assigned_a{Event::NoAutoReset{}};
  Event assigned_b{Event::NoAutoReset{}};
  Event assigned_c{Event::NoAutoReset{}};

  engine::SharedTaskWithResult<void> task_a;
  engine::SharedTaskWithResult<void> task_b;
  engine::SharedTaskWithResult<void> task_c;

  task_a = utils::SharedAsync("a", [&] {
    ASSERT_TRUE(assigned_a.WaitForEvent());
    kStringVariable.Emplace("1");

    task_b = utils::SharedAsync("b", [&] {
      ASSERT_TRUE(assigned_b.WaitForEvent());
      kStringVariable2.Emplace("2");

      task_c = utils::SharedAsync("c", [&] {
        ASSERT_TRUE(assigned_c.WaitForEvent());
        kStringVariable3.Emplace("3");

        task_a.Wait();
        task_b.Wait();

        EXPECT_EQ(kStringVariable.Get(), "1");
        EXPECT_EQ(kStringVariable2.Get(), "2");
        EXPECT_EQ(kStringVariable3.Get(), "3");
      });
      assigned_c.Send();

      task_a.Wait();

      EXPECT_EQ(kStringVariable.Get(), "1");
      EXPECT_EQ(kStringVariable2.Get(), "2");
      EXPECT_FALSE(kStringVariable3.GetOptional());
    });
    assigned_b.Send();

    EXPECT_EQ(kStringVariable.Get(), "1");
    EXPECT_FALSE(kStringVariable2.GetOptional());
    EXPECT_FALSE(kStringVariable3.GetOptional());
  });
  assigned_a.Send();

  engine::SharedTaskWithResult<void>* tasks[3] = {&task_a, &task_b, &task_c};
  for (auto* task : tasks) task->Wait();
  for (auto* task : tasks) task->Get();
}

USERVER_NAMESPACE_END
