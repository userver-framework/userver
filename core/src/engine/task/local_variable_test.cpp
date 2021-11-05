#include <userver/utest/utest.hpp>

#include <optional>
#include <utility>

#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/local_variable.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class LogStringGuard final {
 public:
  LogStringGuard(std::string& destination, std::string source)
      : destination_(destination), source_(std::move(source)) {}

  ~LogStringGuard() { destination_ += source_; }

 private:
  std::string& destination_;
  std::string source_;
};

}  // namespace

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

  auto task = engine::AsyncNoSpan([] {
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

  auto task = engine::AsyncNoSpan([] {
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

    engine::AsyncNoSpan([] { *x; }).Get();

    EXPECT_EQ(2, ctr.load());
    EXPECT_EQ(1, dtr.load());
  }).Get();

  EXPECT_EQ(2, ctr.load());
  EXPECT_EQ(2, dtr.load());
}

UTEST(TaskLocalVariable, DestructionOrder) {
  static engine::TaskLocalVariable<std::optional<LogStringGuard>> x;
  static engine::TaskLocalVariable<std::optional<LogStringGuard>> y;
  static engine::TaskLocalVariable<std::optional<LogStringGuard>> z;

  {
    std::string destruction_order;

    engine::AsyncNoSpan([&] {
      y->emplace(destruction_order, "y");
      x->emplace(destruction_order, "x");
      z->emplace(destruction_order, "z");
    }).Get();

    // variables are destroyed in reverse-initialization order
    EXPECT_EQ(destruction_order, "zxy");
  }

  {
    std::string destruction_order;

    engine::AsyncNoSpan([&] {
      x->emplace(destruction_order, "x");
      y->emplace(destruction_order, "y");
    }).Get();

    // different tasks may have different initialization order and utilize
    // different sets of variables
    EXPECT_EQ(destruction_order, "yx");
  }
}

USERVER_NAMESPACE_END
