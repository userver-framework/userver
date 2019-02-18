#include <utest/utest.hpp>

#include <engine/async.hpp>
#include <engine/sleep.hpp>
#include <engine/task/local_variable.hpp>

TEST(TaskLocalVariable, SetGet) {
  RunInCoro([] {
    static engine::TaskLocalVariable<int> x;

    *x = 1;
    EXPECT_EQ(1, *x);

    engine::Yield();
    EXPECT_EQ(1, *x);

    *x = 2;
    EXPECT_EQ(2, *x);

    engine::Yield();
    EXPECT_EQ(2, *x);
  });
}

TEST(TaskLocalVariable, TwoTask) {
  RunInCoro([] {
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
  });
}

TEST(TaskLocalVariable, MultipleThreads) {
  RunInCoro([] {
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
  });
}

TEST(TaskLocalVariable, Destructor) {
  static std::atomic<int> ctr{0};
  static std::atomic<int> dtr{0};

  struct A {
    A() { ctr++; }
    ~A() { dtr++; }
  };

  static engine::TaskLocalVariable<A> x;

  EXPECT_EQ(0, ctr.load());
  EXPECT_EQ(0, dtr.load());

  RunInCoro([] {
    *x;
    EXPECT_EQ(1, ctr.load());
    EXPECT_EQ(0, dtr.load());

    engine::impl::Async([] { *x; }).Get();

    EXPECT_EQ(2, ctr.load());
    EXPECT_EQ(1, dtr.load());
  });

  EXPECT_EQ(2, ctr.load());
  EXPECT_EQ(2, dtr.load());
}
