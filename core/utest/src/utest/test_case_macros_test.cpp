#include <userver/utest/utest.hpp>

#include <type_traits>

#include <userver/engine/mutex.hpp>
#include <userver/engine/task/current_task.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

UTEST(TestCaseMacros, UTESTEngine) {
    EXPECT_EQ(engine::current_task::GetWorkerCount(), 1);

    engine::Mutex mutex;
    const std::lock_guard lock(mutex);
}

UTEST_MT(TestCaseMacros, MultiThreaded, 2) { EXPECT_EQ(engine::current_task::GetWorkerCount(), 2); }

TEST(TestCaseMacros, UtestAndTestWithSameTestSuite) { SUCCEED(); }

class TestCaseMacrosFixture : public ::testing::Test {
public:
    TestCaseMacrosFixture() { CheckEngine(); }

    ~TestCaseMacrosFixture() override { CheckEngine(); }

    static void SetUpTestSuite() {
        is_test_suite_active = true;
        CheckEngine();
    }

    static void TearDownTestSuite() {
        CheckEngine();
        is_test_suite_active = false;
    }

protected:
    void SetUp() override { CheckEngine(); }

    void TearDown() override { CheckEngine(); }

    static void CheckEngine() {
        UINVARIANT(is_test_suite_active, "SetUpTestSuite is ignored");
        const std::lock_guard lock(mutex);
    }

private:
    static inline bool is_test_suite_active{false};
    static inline engine::Mutex mutex;
};

UTEST_F(TestCaseMacrosFixture, UtestFEngine) { CheckEngine(); }

UTEST_F_MT(TestCaseMacrosFixture, UtestFEngine2, 2) { EXPECT_EQ(engine::current_task::GetWorkerCount(), 2); }

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
        const std::lock_guard lock(mutex_);
        EXPECT_TRUE(GetParam() == "foo" || GetParam() == "bar");
    }

private:
    engine::Mutex mutex_;
};

UTEST_P(TestCaseMacrosParametric, UtestPEngine) { CheckEngineAndParam(); }

UTEST_P_MT(TestCaseMacrosParametric, UtestPEngine2, 2) {
    CheckEngineAndParam();
    EXPECT_EQ(engine::current_task::GetWorkerCount(), 2);
}

INSTANTIATE_UTEST_SUITE_P(FooBar, TestCaseMacrosParametric, testing::Values("foo", "bar"));

class TestCaseMacrosParametric2 : public TestCaseMacrosParametric {};

INSTANTIATE_UTEST_SUITE_P(TestCaseMacrosParametric2, TestCaseMacrosParametric2, testing::Values("foo"));

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
    EXPECT_EQ(engine::current_task::GetWorkerCount(), 2);
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
    EXPECT_EQ(engine::current_task::GetWorkerCount(), 2);
}

REGISTER_TYPED_UTEST_SUITE_P(TestCaseMacrosTypedP, TypedUtestPEngine, TypedUtestPEngine2);

INSTANTIATE_TYPED_UTEST_SUITE_P(MyTypes, TestCaseMacrosTypedP, MyTypes);

USERVER_NAMESPACE_END
