#include <userver/utest/utest.hpp>

#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/local_variable.hpp>
#include <userver/utils/async.hpp>

UTEST(TaskLocalVariable, SetGet) {
  static engine::TaskLocalVariable<int> x;

  *x = 1;
  EXPECT_EQ(1, *x);

  engine::Yield();
  EXPECT_EQ(1, *x);

  *x = 2;
  EXPECT_EQ(2, *x);

  engine::Yield();
  EXPECT_EQ(2, *x);
}

UTEST(TaskLocalVariable, TwoTask) {
  static engine::TaskLocalVariable<int> x;

  *x = 1;

  auto task = engine::impl::Async([] {
    *x = 2;
    EXPECT_EQ(2, *x);

    engine::Yield();
    EXPECT_EQ(2, *x);

    *x = 3;
    EXPECT_EQ(3, *x);

    engine::Yield();
    EXPECT_EQ(3, *x);
  });

  engine::Yield();
  EXPECT_EQ(1, *x);

  *x = 10;
  EXPECT_EQ(10, *x);

  engine::Yield();
  EXPECT_EQ(10, *x);
}

UTEST(TaskLocalVariable, MultipleThreads) {
  static engine::TaskLocalVariable<int> x;

  *x = 1;

  auto task = engine::impl::Async([] {
    *x = 2;
    EXPECT_EQ(2, *x);

    engine::Yield();
    EXPECT_EQ(2, *x);

    *x = 3;
    EXPECT_EQ(3, *x);

    engine::Yield();
    EXPECT_EQ(3, *x);
  });

  engine::Yield();
  EXPECT_EQ(1, *x);

  *x = 10;
  EXPECT_EQ(10, *x);

  engine::Yield();
  EXPECT_EQ(10, *x);
}

UTEST(TaskLocalVariable, Destructor) {
  static std::atomic<int> ctr{0};
  static std::atomic<int> dtr{0};

  struct A {
    A() { ctr++; }
    ~A() { dtr++; }
  };

  static engine::TaskLocalVariable<A> x;

  EXPECT_EQ(0, ctr.load());
  EXPECT_EQ(0, dtr.load());

  utils::Async("test", [] {
    *x;
    EXPECT_EQ(1, ctr.load());
    EXPECT_EQ(0, dtr.load());

    engine::impl::Async([] { *x; }).Get();

    EXPECT_EQ(2, ctr.load());
    EXPECT_EQ(1, dtr.load());
  }).Get();

  EXPECT_EQ(2, ctr.load());
  EXPECT_EQ(2, dtr.load());
}
