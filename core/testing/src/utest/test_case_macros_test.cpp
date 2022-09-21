#include <userver/utest/utest.hpp>

#include <atomic>
#include <type_traits>

#include <userver/engine/mutex.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

UTEST(TestCaseMacros, UTESTEngine) {
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

UTEST_MT(TestCaseMacros, MultiThreaded, 2) {
  EXPECT_EQ(GetThreadCount(), 2);
  DeadlockUnlessMultiThreaded();
}

TEST(TestCaseMacros, UtestAndTestWithSameTestSuite) { SUCCEED(); }

class TestCaseMacrosFixture : public ::testing::Test {
 public:
  TestCaseMacrosFixture() { CheckEngine(); }

  ~TestCaseMacrosFixture() override { CheckEngine(); }

  static void SetUpTestSuite() {
    is_test_suite_active_ = true;
    CheckEngine();
  }

  static void TearDownTestSuite() {
    CheckEngine();
    is_test_suite_active_ = false;
  }

 protected:
  void SetUp() override { CheckEngine(); }

  void TearDown() override { CheckEngine(); }

  static void CheckEngine() {
    UINVARIANT(is_test_suite_active_, "SetUpTestSuite is ignored");
    std::lock_guard lock(mutex_);
  }

 private:
  static inline bool is_test_suite_active_{false};
  static inline engine::Mutex mutex_;
};

UTEST_F(TestCaseMacrosFixture, UtestFEngine) { CheckEngine(); }

UTEST_F_MT(TestCaseMacrosFixture, UtestFEngine2, 2) {
  EXPECT_EQ(GetThreadCount(), 2);
  DeadlockUnlessMultiThreaded();
}

// Using the same test fixture with both U-macros and vanilla macros leads to
// gtest crashing or failing to start a coroutine environment.
//
// TEST_F(TestCaseMacrosFixture, Foo) {}

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

 private:
  engine::Mutex mutex_;
};

UTEST_P(TestCaseMacrosParametric, UtestPEngine) { CheckEngineAndParam(); }

UTEST_P_MT(TestCaseMacrosParametric, UtestPEngine2, 2) {
  CheckEngineAndParam();
  EXPECT_EQ(GetThreadCount(), 2);
  DeadlockUnlessMultiThreaded();
}

INSTANTIATE_UTEST_SUITE_P(FooBar, TestCaseMacrosParametric,
                          testing::Values("foo", "bar"));

class TestCaseMacrosParametric2 : public TestCaseMacrosParametric {};

INSTANTIATE_UTEST_SUITE_P(TestCaseMacrosParametric2, TestCaseMacrosParametric2,
                          testing::Values("foo"));

UTEST_P(TestCaseMacrosParametric2, InstantiatedBeforeTest) {}

template <typename T>
class TestCaseMacrosTyped : public TestCaseMacrosFixture {
 protected:
  T GetTypeInstance() { return T{}; }
};

using MyTypes = ::testing::Types<char, bool, std::string>;

TYPED_UTEST_SUITE(TestCaseMacrosTyped, MyTypes);

TYPED_UTEST(TestCaseMacrosTyped, TypedUtestEngine) {
  this->CheckEngine();
  static_assert(std::is_same_v<decltype(this->GetTypeInstance()), TypeParam>);
  static_assert(std::is_same_v<TestFixture, TestCaseMacrosTyped<TypeParam>>);
}

TYPED_UTEST_MT(TestCaseMacrosTyped, TypedUtestEngine2, 2) {
  this->CheckEngine();
  EXPECT_EQ(GetThreadCount(), 2);
  DeadlockUnlessMultiThreaded();
}

template <typename T>
class TestCaseMacrosTypedP : public TestCaseMacrosFixture {
 protected:
  T GetTypeInstance() { return T{}; }
};

TYPED_UTEST_SUITE_P(TestCaseMacrosTypedP);

TYPED_UTEST_P(TestCaseMacrosTypedP, TypedUtestPEngine) {
  this->CheckEngine();
  static_assert(std::is_same_v<decltype(this->GetTypeInstance()), TypeParam>);
  static_assert(std::is_same_v<TestFixture, TestCaseMacrosTypedP<TypeParam>>);
}

TYPED_UTEST_P_MT(TestCaseMacrosTypedP, TypedUtestPEngine2, 2) {
  this->CheckEngine();
  EXPECT_EQ(GetThreadCount(), 2);
  DeadlockUnlessMultiThreaded();
}

REGISTER_TYPED_UTEST_SUITE_P(TestCaseMacrosTypedP, TypedUtestPEngine,
                             TypedUtestPEngine2);

INSTANTIATE_TYPED_UTEST_SUITE_P(MyTypes, TestCaseMacrosTypedP, MyTypes);

USERVER_NAMESPACE_END
