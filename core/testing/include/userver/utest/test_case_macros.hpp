#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace utest::impl {

class EnrichedTestBase {
 public:
  virtual ~EnrichedTestBase() = default;
  virtual void SetUp() = 0;
  virtual void TearDown() = 0;
  virtual void TestBody() = 0;

  std::size_t GetThreadCount() const { return utest_thread_count_; }
  void SetThreadCount(std::size_t count) { utest_thread_count_ = count; }

 private:
  std::size_t utest_thread_count_ = 1;
};

// The work horse of the test suite. Takes a test class returned by 'factory'
// in a 'std::unique_ptr' and runs the usual test cycle (mimicking gtest),
// all in a coroutine environment.
void DoRunTest(std::size_t thread_count,
               std::function<std::unique_ptr<EnrichedTestBase>()> factory);

// Analogue of `DoRunTest` for DEATH-tests.
void DoRunDeathTest(std::size_t thread_count,
                    std::function<std::unique_ptr<EnrichedTestBase>()> factory);

void RunSetUpTestSuite(void (*set_up_test_suite)());
void RunTearDownTestSuite(void (*tear_down_test_suite)());

// Inherits from the user's fixture (or '::testing::Test') and provides some
// niceties to the test body ('GetThreadCount') while making the test methods
// public ('SetUp', 'TearDown'). The fixture is further inherited from
// (and "enriched") in inline-created test classes
// (IMPL_UTEST_HIDE_ENRICHED_FROM_IDE).
template <typename UserFixture>
// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class EnrichedFixture : public UserFixture, public EnrichedTestBase {
 protected:
  void SetUp() override { return UserFixture::SetUp(); }
  void TearDown() override { return UserFixture::TearDown(); }

 private:
  using EnrichedTestBase::SetThreadCount;
  using EnrichedTestBase::TestBody;
};

template <typename Base, typename UserFixture>
class WithStaticMethods : public Base {
 public:
  static void SetUpTestSuite() {
    RunSetUpTestSuite(&UserFixture::SetUpTestSuite);
  }

  static void TearDownTestSuite() {
    RunTearDownTestSuite(&UserFixture::TearDownTestSuite);
  }
};

// 'TestLauncher' and 'TestLauncherParametric' take the enriched user's test
// class and run it in a coroutine environment via 'DoRunTest'.
template <typename UserFixture>
class TestLauncher : public WithStaticMethods<::testing::Test, UserFixture> {
 public:
  // Called from UTEST_F, TYPED_UTEST and TYPED_UTEST_P macros
  template <typename EnrichedTest>
  static void RunTest(std::size_t thread_count) {
    utest::impl::DoRunTest(thread_count,
                           [] { return std::make_unique<EnrichedTest>(); });
  }
};

template <typename UserFixture>
class TestLauncherParametric
    : public WithStaticMethods<
          ::testing::TestWithParam<typename UserFixture::ParamType>,
          UserFixture> {
 public:
  // Called from the UTEST_P macro
  template <typename EnrichedTest>
  static void RunTest(std::size_t thread_count) {
    using ParamType = typename UserFixture::ParamType;
    const auto& parameter = ::testing::TestWithParam<ParamType>::GetParam();

    // It seems impossible to seamlessly proxy 'ParamType' from the launcher to
    // the enriched fixture without using gtest internals.
    auto factory = std::make_unique<
        testing::internal::ParameterizedTestFactory<EnrichedTest>>(parameter);

    utest::impl::DoRunTest(thread_count, [&] {
      return std::unique_ptr<EnrichedTest>{
          dynamic_cast<EnrichedTest*>(factory->CreateTest())};
    });
  }
};

// 'TestLauncherDeath' takes the enriched user's test class and runs it in a
// coroutine environment via 'DoRunDeathTest'.
template <typename UserFixture>
class TestLauncherDeath
    : public WithStaticMethods<::testing::Test, UserFixture> {
 public:
  // Called from UTEST_DEATH macros
  template <typename EnrichedTest>
  static void RunTest(std::size_t thread_count) {
    testing::FLAGS_gtest_death_test_style = "threadsafe";
    utest::impl::DoRunDeathTest(
        thread_count, [] { return std::make_unique<EnrichedTest>(); });
  }
};

template <template <typename> typename UserFixture, typename T>
using TestLauncherTyped = TestLauncher<UserFixture<T>>;

// For TYPED_TEST_SUITE and INSTANTIATE_TYPED_TEST_SUITE_P
struct DefaultNameGenerator final {
  template <typename T>
  static std::string GetName(int i) {
    return std::to_string(i);
  }
};

constexpr bool CheckTestSuiteNameSuffix(std::string_view str,
                                        std::string_view suffix) {
  return str.size() >= suffix.size() &&
         str.substr(str.size() - suffix.size()) == suffix;
}

}  // namespace utest::impl

USERVER_NAMESPACE_END

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IMPL_UTEST_NON_PARENTHESIZED(identifier) identifier

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IMPL_UTEST_NAMESPACE_NAME(test_suite_name) test_suite_name##_##Utest

// Enriched fixtures are nested into IMPL_UTEST_HIDE_ENRICHED_FROM_IDE namespace
// so that IDEs don't find them and don't show in "Run a single Test".
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IMPL_UTEST_HIDE_ENRICHED_FROM_IDE(test_suite_name, test_name) \
  test_suite_name##_##test_name##_##Utest

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IMPL_UTEST_USER_FIXTURE(test_suite_name) \
  test_suite_name##_##UtestFixture

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IMPL_UTEST_MAKE_USER_FIXTURE_ALIAS(test_suite_name) \
  using IMPL_UTEST_USER_FIXTURE(test_suite_name) =          \
      IMPL_UTEST_NON_PARENTHESIZED(test_suite_name)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IMPL_UTEST_MAKE_USER_FIXTURE_ALIAS_TYPED(test_suite_name) \
  template <typename UtestTypeParamImpl>                          \
  using IMPL_UTEST_USER_FIXTURE(test_suite_name) =                \
      IMPL_UTEST_NON_PARENTHESIZED(test_suite_name)<UtestTypeParamImpl>

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IMPL_UTEST_HIDE_USER_FIXTURE_BY_TEST_LAUNCHER(test_suite_name,        \
                                                      test_launcher_template) \
  using IMPL_UTEST_NON_PARENTHESIZED(test_suite_name) =                       \
      IMPL_UTEST_NON_PARENTHESIZED(                                           \
          test_launcher_template)<IMPL_UTEST_USER_FIXTURE(test_suite_name)>;

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IMPL_UTEST_HIDE_USER_FIXTURE_BY_TEST_LAUNCHER_TYPED(test_suite_name) \
  template <typename UtestTypeParamImpl>                                     \
  using IMPL_UTEST_NON_PARENTHESIZED(test_suite_name) =                      \
      USERVER_NAMESPACE::utest::impl::TestLauncherTyped<                     \
          IMPL_UTEST_USER_FIXTURE(test_suite_name), UtestTypeParamImpl>;

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IMPL_UTEST_TEST(test_suite_name, test_name, thread_count)              \
  struct IMPL_UTEST_HIDE_ENRICHED_FROM_IDE(test_suite_name, test_name) final { \
    class EnrichedTest final                                                   \
        : public USERVER_NAMESPACE::utest::impl::EnrichedFixture<              \
              ::testing::Test> {                                               \
      void TestBody() override;                                                \
    };                                                                         \
  };                                                                           \
  TEST(test_suite_name, test_name) {                                           \
    using EnrichedTest = IMPL_UTEST_HIDE_ENRICHED_FROM_IDE(                    \
        test_suite_name, test_name)::EnrichedTest;                             \
    USERVER_NAMESPACE::utest::impl::TestLauncher<::testing::Test>::RunTest<    \
        EnrichedTest>(thread_count);                                           \
  }                                                                            \
  void IMPL_UTEST_HIDE_ENRICHED_FROM_IDE(test_suite_name,                      \
                                         test_name)::EnrichedTest::TestBody()

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IMPL_UTEST_DEATH_TEST(test_suite_name, test_name, thread_count)        \
  static_assert(USERVER_NAMESPACE::utest::impl::CheckTestSuiteNameSuffix(      \
                    #test_suite_name, "DeathTest"),                            \
                "test_suite_name for death test should be '*DeathTest'");      \
  struct IMPL_UTEST_HIDE_ENRICHED_FROM_IDE(test_suite_name, test_name) final { \
    class EnrichedTest final                                                   \
        : public USERVER_NAMESPACE::utest::impl::EnrichedFixture<              \
              ::testing::Test> {                                               \
      void TestBody() override;                                                \
    };                                                                         \
  };                                                                           \
  TEST(test_suite_name, test_name) {                                           \
    using EnrichedTest = IMPL_UTEST_HIDE_ENRICHED_FROM_IDE(                    \
        test_suite_name, test_name)::EnrichedTest;                             \
    USERVER_NAMESPACE::utest::impl::TestLauncherDeath<                         \
        ::testing::Test>::RunTest<EnrichedTest>(thread_count);                 \
  }                                                                            \
  void IMPL_UTEST_HIDE_ENRICHED_FROM_IDE(test_suite_name,                      \
                                         test_name)::EnrichedTest::TestBody()

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IMPL_UTEST_ANY_BEGIN(test_suite_name, test_name,                       \
                             test_launcher_template)                           \
  IMPL_UTEST_MAKE_USER_FIXTURE_ALIAS(test_suite_name);                         \
  struct IMPL_UTEST_HIDE_ENRICHED_FROM_IDE(test_suite_name, test_name) final { \
    class EnrichedTest final                                                   \
        : public USERVER_NAMESPACE::utest::impl::EnrichedFixture<              \
              IMPL_UTEST_USER_FIXTURE(test_suite_name)> {                      \
      void TestBody() override;                                                \
    };                                                                         \
  };                                                                           \
  /* The 'namespace' trick is used to make gtest use our 'test_launcher'       \
   * instead of 'test_suite_name' fixture */                                   \
  namespace IMPL_UTEST_NAMESPACE_NAME(test_suite_name) {                       \
  IMPL_UTEST_HIDE_USER_FIXTURE_BY_TEST_LAUNCHER(test_suite_name,               \
                                                test_launcher_template)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IMPL_UTEST_ANY_END(test_suite_name, test_name, thread_count) \
  /* test header goes here */ {                                      \
    using EnrichedTest = IMPL_UTEST_HIDE_ENRICHED_FROM_IDE(          \
        test_suite_name, test_name)::EnrichedTest;                   \
    this->RunTest<EnrichedTest>(thread_count);                       \
  }                                                                  \
  } /* namespace */                                                  \
  void IMPL_UTEST_HIDE_ENRICHED_FROM_IDE(test_suite_name,            \
                                         test_name)::EnrichedTest::TestBody()

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IMPL_UTEST_TEST_F(test_suite_name, test_name, thread_count)  \
  IMPL_UTEST_ANY_BEGIN(test_suite_name, test_name,                   \
                       USERVER_NAMESPACE::utest::impl::TestLauncher) \
  TEST_F(test_suite_name, test_name)                                 \
  IMPL_UTEST_ANY_END(test_suite_name, test_name, thread_count)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IMPL_UTEST_TEST_P(test_suite_name, test_name, thread_count)            \
  IMPL_UTEST_ANY_BEGIN(test_suite_name, test_name,                             \
                       USERVER_NAMESPACE::utest::impl::TestLauncherParametric) \
  TEST_P(test_suite_name, test_name)                                           \
  IMPL_UTEST_ANY_END(test_suite_name, test_name, thread_count)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IMPL_UTEST_TYPED_ANY_BEGIN(test_suite_name, test_name)                 \
  IMPL_UTEST_MAKE_USER_FIXTURE_ALIAS_TYPED(test_suite_name);                   \
  struct IMPL_UTEST_HIDE_ENRICHED_FROM_IDE(test_suite_name, test_name) final { \
    template <typename UtestTypeParamImpl>                                     \
    using UtestUserFixture =                                                   \
        IMPL_UTEST_USER_FIXTURE(test_suite_name)<UtestTypeParamImpl>;          \
                                                                               \
    template <typename UtestTypeParamImpl>                                     \
    class EnrichedTest                                                         \
        : public USERVER_NAMESPACE::utest::impl::EnrichedFixture<              \
              UtestUserFixture<UtestTypeParamImpl>> {                          \
      using TypeParam = UtestTypeParamImpl;                                    \
      using TestFixture =                                                      \
          IMPL_UTEST_NON_PARENTHESIZED(test_suite_name)<TypeParam>;            \
      using USERVER_NAMESPACE::utest::impl::EnrichedTestBase::GetThreadCount;  \
      void TestBody() override;                                                \
    };                                                                         \
  };                                                                           \
  /* The 'namespace' trick is used to make gtest use our 'TestLauncher'        \
   * instead of 'test_suite_name' fixture */                                   \
  namespace IMPL_UTEST_NAMESPACE_NAME(test_suite_name) {

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IMPL_UTEST_TYPED_ANY_END(test_suite_name, test_name, thread_count) \
  /* test header goes here */ {                                            \
    using EnrichedTest = IMPL_UTEST_HIDE_ENRICHED_FROM_IDE(                \
        test_suite_name, test_name)::EnrichedTest<TypeParam>;              \
    this->template RunTest<EnrichedTest>(thread_count);                    \
  }                                                                        \
  } /* namespace */                                                        \
  template <typename UtestTypeParamImpl>                                   \
  void IMPL_UTEST_HIDE_ENRICHED_FROM_IDE(                                  \
      test_suite_name,                                                     \
      test_name)::EnrichedTest<UtestTypeParamImpl>::TestBody()

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IMPL_UTEST_TYPED_TEST(test_suite_name, test_name, thread_count) \
  IMPL_UTEST_TYPED_ANY_BEGIN(test_suite_name, test_name)                \
  TYPED_TEST(test_suite_name, test_name)                                \
  IMPL_UTEST_TYPED_ANY_END(test_suite_name, test_name, thread_count)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IMPL_UTEST_TYPED_TEST_P(test_suite_name, test_name, thread_count) \
  IMPL_UTEST_TYPED_ANY_BEGIN(test_suite_name, test_name)                  \
  TYPED_TEST_P(test_suite_name, test_name)                                \
  IMPL_UTEST_TYPED_ANY_END(test_suite_name, test_name, thread_count)
