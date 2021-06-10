#include <utest/utest.hpp>

#include <atomic>
#include <type_traits>

#include <engine/mutex.hpp>
#include <utils/async.hpp>

UTEST(TestCaseMacros, UTEST_Engine) {
  EXPECT_EQ(GetThreadCount(), 1);

  engine::Mutex mutex;
  std::lock_guard lock(mutex);
}

namespace {

void DeadlockUnlessMultiThreaded() {
  std::atomic<bool> keep_running1{true};
  std::atomic<bool> keep_running2{true};

  auto task1 = utils::Async("task1", [&] {
    keep_running2 = false;
    while (keep_running1) {
    }
  });
  auto task2 = utils::Async("task2", [&] {
    keep_running1 = false;
    while (keep_running2) {
    }
  });

  task1.Get();
  task2.Get();
}

}  // namespace

UTEST(TestCaseMacros, MultiThreaded, utest::Threads{2}) {
  EXPECT_EQ(GetThreadCount(), 2);
  DeadlockUnlessMultiThreaded();
}

class TestCaseMacrosFixture : public ::testing::Test {
 public:
  TestCaseMacrosFixture() { CheckEngine(); }

  ~TestCaseMacrosFixture() override { CheckEngine(); }

 protected:
  void SetUp() override { CheckEngine(); }

  void TearDown() override { CheckEngine(); }

  void CheckEngine() { std::lock_guard lock(mutex_); }

  engine::Mutex mutex_;
};

UTEST_F(TestCaseMacrosFixture, UTEST_F_Engine) { CheckEngine(); }

class TestCaseMacrosParametric : public ::testing::TestWithParam<std::string> {
 public:
  static_assert(std::is_same_v<ParamType, std::string>);

  TestCaseMacrosParametric() { CheckEngineAndParam(); }

  ~TestCaseMacrosParametric() override { CheckEngineAndParam(); }

 protected:
  void SetUp() override { CheckEngineAndParam(); }

  void TearDown() override { CheckEngineAndParam(); }

  void CheckEngineAndParam() {
    std::lock_guard lock(mutex_);
    EXPECT_TRUE(GetParam() == "foo" || GetParam() == "bar");
  }

  engine::Mutex mutex_;
};

UTEST_P(TestCaseMacrosParametric, UTEST_P_Engine) { CheckEngineAndParam(); }

using std::string_literals::operator""s;

INSTANTIATE_UTEST_SUITE_P(FooBar, TestCaseMacrosParametric,
                          testing::Values("foo"s, "bar"s));

template <typename T>
class TestCaseMacrosTyped : public TestCaseMacrosFixture {
 protected:
  T GetTypeInstance() { return T{}; }
};

using MyTypes = ::testing::Types<char, bool, std::string>;

TYPED_UTEST_SUITE(TestCaseMacrosTyped, MyTypes);

TYPED_UTEST(TestCaseMacrosTyped, TYPED_UTEST_Engine) {
  this->CheckEngine();
  static_assert(std::is_same_v<decltype(this->GetTypeInstance()), TypeParam>);
  static_assert(std::is_same_v<TestFixture, TestCaseMacrosTyped<TypeParam>>);
}

template <typename T>
class TestCaseMacrosTypedP : public TestCaseMacrosFixture {
 protected:
  T GetTypeInstance() { return T{}; }
};

TYPED_UTEST_SUITE_P(TestCaseMacrosTypedP);

TYPED_UTEST_P(TestCaseMacrosTypedP, TYPED_UTEST_P_Engine) {
  this->CheckEngine();
  static_assert(std::is_same_v<decltype(this->GetTypeInstance()), TypeParam>);
  static_assert(std::is_same_v<TestFixture, TestCaseMacrosTypedP<TypeParam>>);
}

REGISTER_TYPED_UTEST_SUITE_P(TestCaseMacrosTypedP, TYPED_UTEST_P_Engine);

INSTANTIATE_TYPED_UTEST_SUITE_P(MyTypes, TestCaseMacrosTypedP, MyTypes);
