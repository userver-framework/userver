#pragma once

#include <optional>
#include <stdexcept>
#include <type_traits>

#include <gtest/gtest.h>

#include <engine/run_standalone.hpp>
#include <utils/scope_guard.hpp>

namespace utest::impl {

// Allows to call private methods of tests that friend it
struct TestLoophole {
  template <typename Test>
  static void SetUp(Test& test) {
    test.SetUp();
  }

  template <typename Test>
  static void TearDown(Test& test) {
    test.TearDown();
  }

  template <typename Test>
  static void TestBody(Test& test) {
    test.TestBody();
  }

  template <typename Test>
  static std::size_t GetThreadCount() {
    return Test::GetThreadCount();
  }
};

void LogFatalException(const std::exception& ex, const char* name);

void LogUnknownFatalException(const char* name);

template <typename WrappedTest>
class CoroutineTestWrapper : public ::testing::Test {
 protected:
  void TestBody() override {
    engine::RunStandalone(TestLoophole::GetThreadCount<WrappedTest>(),
                          [this] { DoRun(); });
  };

  virtual std::unique_ptr<WrappedTest> MakeWrappedTest() {
    return std::make_unique<WrappedTest>();
  }

 private:
  void DoRun() {
    std::unique_ptr<WrappedTest> test = CallLoggingExceptions(
        "the test fixture's constructor", [&] { return MakeWrappedTest(); });
    if (test->HasFatalFailure() || test->IsSkipped()) return;

    utils::ScopeGuard tear_down_guard{[&] {
      // gtest invokes TearDown even if SetUp fails
      CallLoggingExceptions("TearDown()",
                            [&] { TestLoophole::TearDown(*test); });
      CallLoggingExceptions("the test fixture's destructor",
                            [&] { delete test.release(); });
    }};

    CallLoggingExceptions("SetUp()", [&] { TestLoophole::SetUp(*test); });
    if (test->HasFatalFailure() || test->IsSkipped()) return;

    CallLoggingExceptions("the test body",
                          [&] { TestLoophole::TestBody(*test); });
  }

  template <typename Func>
  decltype(auto) CallLoggingExceptions(const char* name, const Func& func) {
    try {
      return func();
    } catch (const std::exception& ex) {
      LogFatalException(ex, name);
      return decltype(func())();
    } catch (...) {
      LogUnknownFatalException(name);
      return decltype(func())();
    }
  }
};

template <typename WrappedTest>
class CoroutineTestWrapperParametric
    : public CoroutineTestWrapper<WrappedTest> {
 public:
  using ParamType = typename WrappedTest::ParamType;

  static void SetParam(const ParamType* parameter) {
    wrapped_test_factory_.emplace(*parameter);
  }

 protected:
  std::unique_ptr<WrappedTest> MakeWrappedTest() override {
    return std::unique_ptr<WrappedTest>{
        dynamic_cast<WrappedTest*>(wrapped_test_factory_->CreateTest())};
  }

 private:
  static inline std::optional<
      testing::internal::ParameterizedTestFactory<WrappedTest>>
      wrapped_test_factory_;
};

template <template <typename> typename Outer,
          template <typename> typename Inner>
struct TemplateComposition final {
  template <typename T>
  using type = Outer<Inner<T>>;
};

template <template <typename> typename TestWrapper,
          template <typename> typename... WrappedTests>
struct WrappedTemplates final {
  using type = typename ::testing::internal::Templates<
      TemplateComposition<TestWrapper, WrappedTests>::template type...>::type;
};

template <typename... Args>
std::size_t GetThreadCount(Args... args) {
  const std::size_t sum = (0 + ... + args.value);
  return sum ? sum : 1;
}

};  // namespace utest::impl

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IMPL_UTEST_NON_PARENTHESIZED(test_name) test_name

// Copied from 'GTEST_TEST_' implementation with the following modifications:
// - it registers 'test_wrapper<ThisTestClass>' in gtest
// - it friends 'TestLoophole'
// - it defines 'GetThreadCount()'
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IMPL_UTEST_GTEST_TEST_(test_suite_name, test_name, parent_class,      \
                               parent_id, test_wrapper, thread_count)         \
  class GTEST_TEST_CLASS_NAME_(test_suite_name, test_name)                    \
      : public IMPL_UTEST_NON_PARENTHESIZED(parent_class) {                   \
   private:                                                                   \
    void TestBody() override;                                                 \
    static std::size_t GetThreadCount() { return thread_count; }              \
    friend struct ::utest::impl::TestLoophole;                                \
    [[maybe_unused]] static const bool test_info_;                            \
  };                                                                          \
  const bool GTEST_TEST_CLASS_NAME_(test_suite_name, test_name)::test_info_ = \
      ::testing::internal::MakeAndRegisterTestInfo(                           \
          #test_suite_name, #test_name, nullptr, nullptr,                     \
          ::testing::internal::CodeLocation(__FILE__, __LINE__), (parent_id), \
          ::testing::internal::SuiteApiResolver<                              \
              parent_class>::GetSetUpCaseOrSuite(__FILE__, __LINE__),         \
          ::testing::internal::SuiteApiResolver<                              \
              parent_class>::GetTearDownCaseOrSuite(__FILE__, __LINE__),      \
          new ::testing::internal::TestFactoryImpl<                           \
              IMPL_UTEST_NON_PARENTHESIZED(test_wrapper) <                    \
              GTEST_TEST_CLASS_NAME_(test_suite_name, test_name)>>);          \
  void GTEST_TEST_CLASS_NAME_(test_suite_name, test_name)::TestBody()

// Copied from 'TEST_P' implementation with the following modifications:
// - it registers 'test_wrapper<ThisTestClass>' in gtest
// - it friends 'TestLoophole'
// - it defines 'GetThreadCount()'
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IMPL_UTEST_TEST_P(test_suite_name, test_name, test_wrapper,            \
                          thread_count)                                        \
  class GTEST_TEST_CLASS_NAME_(test_suite_name, test_name)                     \
      : public IMPL_UTEST_NON_PARENTHESIZED(test_suite_name) {                 \
    void TestBody() override;                                                  \
    static std::size_t GetThreadCount() { return thread_count; }               \
    friend struct ::utest::impl::TestLoophole;                                 \
    [[maybe_unused]] static const bool gtest_registering_dummy_;               \
                                                                               \
    static bool AddToRegistry() {                                              \
      ::testing::UnitTest::GetInstance()                                       \
          ->parameterized_test_registry()                                      \
          .GetTestSuitePatternHolder<test_suite_name>(                         \
              #test_suite_name,                                                \
              ::testing::internal::CodeLocation(__FILE__, __LINE__))           \
          ->AddTestPattern(                                                    \
              #test_suite_name, #test_name,                                    \
              new ::testing::internal::TestMetaFactory<                        \
                  IMPL_UTEST_NON_PARENTHESIZED(test_wrapper) <                 \
                  GTEST_TEST_CLASS_NAME_(test_suite_name, test_name)>> ());    \
      return false;                                                            \
    }                                                                          \
  };                                                                           \
  const bool GTEST_TEST_CLASS_NAME_(                                           \
      test_suite_name, test_name)::gtest_registering_dummy_ = AddToRegistry(); \
  void GTEST_TEST_CLASS_NAME_(test_suite_name, test_name)::TestBody()

// Copied from 'TYPED_TEST' implementation with the following modifications:
// - it registers 'test_wrapper<ThisTestClass>' in gtest
// - it friends 'TestLoophole'
// - it defines 'GetThreadCount()'
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IMPL_UTEST_TYPED_TEST(test_suite_name, test_name, test_wrapper,        \
                              thread_count)                                    \
  template <typename gtest_TypeParam_>                                         \
  class GTEST_TEST_CLASS_NAME_(test_suite_name, test_name)                     \
      : public IMPL_UTEST_NON_PARENTHESIZED(                                   \
            test_suite_name)<gtest_TypeParam_> {                               \
    using TypeParam = gtest_TypeParam_;                                        \
    using TestFixture =                                                        \
        IMPL_UTEST_NON_PARENTHESIZED(test_suite_name)<gtest_TypeParam_>;       \
                                                                               \
    void TestBody() override;                                                  \
    static std::size_t GetThreadCount() { return thread_count; }               \
    friend struct ::utest::impl::TestLoophole;                                 \
  };                                                                           \
  [[maybe_unused]] static const bool                                           \
      gtest_##test_suite_name##_##test_name##_registered_ =                    \
          ::testing::internal::TypeParameterizedTest<                          \
              test_suite_name,                                                 \
              ::testing::internal::TemplateSel<                                \
                  utest::impl::TemplateComposition<                            \
                      test_wrapper, GTEST_TEST_CLASS_NAME_(test_suite_name,    \
                                                           test_name)>::type>, \
              GTEST_TYPE_PARAMS_(test_suite_name)>::                           \
              Register("",                                                     \
                       ::testing::internal::CodeLocation(__FILE__, __LINE__),  \
                       #test_suite_name, #test_name, 0,                        \
                       ::testing::internal::GenerateNames<                     \
                           GTEST_NAME_GENERATOR_(test_suite_name),             \
                           GTEST_TYPE_PARAMS_(test_suite_name)>());            \
  template <typename gtest_TypeParam_>                                         \
  void GTEST_TEST_CLASS_NAME_(test_suite_name,                                 \
                              test_name)<gtest_TypeParam_>::TestBody()

// Copied from 'TYPED_TEST_P' implementation with the following modifications:
// - it friends 'TestLoophole'
// - it defines 'GetThreadCount()'
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IMPL_UTEST_TYPED_TEST_P(test_suite_name, test_name, thread_count)   \
  namespace GTEST_SUITE_NAMESPACE_(test_suite_name) {                       \
    template <typename gtest_TypeParam_>                                    \
    class IMPL_UTEST_NON_PARENTHESIZED(test_name)                           \
        : public IMPL_UTEST_NON_PARENTHESIZED(                              \
              test_suite_name)<gtest_TypeParam_> {                          \
      using TestFixture =                                                   \
          IMPL_UTEST_NON_PARENTHESIZED(test_suite_name)<gtest_TypeParam_>;  \
      using TypeParam = gtest_TypeParam_;                                   \
                                                                            \
      void TestBody() override;                                             \
      static std::size_t GetThreadCount() { return thread_count; }          \
      friend struct ::utest::impl::TestLoophole;                            \
    };                                                                      \
    [[maybe_unused]] static const bool gtest_##test_name##_defined_ =       \
        GTEST_TYPED_TEST_SUITE_P_STATE_(test_suite_name)                    \
            .AddTestName(__FILE__, __LINE__, #test_suite_name, #test_name); \
  }                                                                         \
  template <typename gtest_TypeParam_>                                      \
  void GTEST_SUITE_NAMESPACE_(                                              \
      test_suite_name)::test_name<gtest_TypeParam_>::TestBody()

// Copied from 'TYPED_TEST_P' implementation with the following modifications:
// - it registers 'test_wrapper<ThisTestClass>' in gtest
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IMPL_UTEST_REGISTER_TYPED_TEST_SUITE_P(test_suite_name, test_wrapper, \
                                               ...)                           \
  namespace GTEST_SUITE_NAMESPACE_(test_suite_name) {                         \
    using gtest_AllTests_ =                                                   \
        ::utest::impl::WrappedTemplates<test_wrapper, __VA_ARGS__>::type;     \
  }                                                                           \
  [[maybe_unused]] static const char* const GTEST_REGISTERED_TEST_NAMES_(     \
      test_suite_name) =                                                      \
      GTEST_TYPED_TEST_SUITE_P_STATE_(test_suite_name)                        \
          .VerifyRegisteredTestNames(__FILE__, __LINE__, #__VA_ARGS__)
