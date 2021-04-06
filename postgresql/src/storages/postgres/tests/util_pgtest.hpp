#pragma once

#include <utest/utest.hpp>

#include <cctype>
#include <cstdlib>
#include <vector>

#include <engine/async.hpp>
#include <engine/task/task.hpp>
#include <logging/log.hpp>
#include <logging/logger.hpp>
#include <utils/scope_guard.hpp>

#include <storages/postgres/default_command_controls.hpp>
#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/detail/connection_ptr.hpp>
#include <storages/postgres/dsn.hpp>

constexpr static const char* kPostgresDsn = "POSTGRES_TEST_DSN";
constexpr static const char* kPostgresLog = "POSTGRES_TEST_LOG";
constexpr uint32_t kConnectionId = 0;

constexpr storages::postgres::CommandControl kTestCmdCtl{
    // TODO: lower execute timeout after TAXICOMMON-1313
    std::chrono::seconds{1}, std::chrono::milliseconds{500}};

storages::postgres::DefaultCommandControls GetTestCmdCtls();

class DefaultCommandControlScope {
 public:
  explicit DefaultCommandControlScope(
      storages::postgres::CommandControl default_cmd_ctl)
      : old_cmd_ctl_(GetTestCmdCtls().GetDefaultCmdCtl()) {
    GetTestCmdCtls().UpdateDefaultCmdCtl(default_cmd_ctl);
  }

  ~DefaultCommandControlScope() {
    GetTestCmdCtls().UpdateDefaultCmdCtl(old_cmd_ctl_);
  }

 private:
  storages::postgres::CommandControl old_cmd_ctl_;
};

inline const storages::postgres::ConnectionSettings kCachePreparedStatements{
    storages::postgres::ConnectionSettings::kCachePreparedStatements};
inline const storages::postgres::ConnectionSettings kNoPreparedStatements{
    storages::postgres::ConnectionSettings::kNoPreparedStatements};
inline const storages::postgres::ConnectionSettings kNoUserTypes{
    storages::postgres::ConnectionSettings::kCachePreparedStatements,
    storages::postgres::ConnectionSettings::kPredefinedTypesOnly,
};

inline engine::Deadline MakeDeadline() {
  return engine::Deadline::FromDuration(kTestCmdCtl.execute);
}

inline storages::postgres::detail::ConnectionPtr MakeConnection(
    const storages::postgres::Dsn& dsn, engine::TaskProcessor& task_processor,
    storages::postgres::ConnectionSettings settings =
        kCachePreparedStatements) {
  namespace pg = storages::postgres;
  std::unique_ptr<pg::detail::Connection> conn;

  pg::detail::Connection::Connect(dsn, task_processor, kConnectionId, settings,
                                  GetTestCmdCtls(), {}, {});
  EXPECT_NO_THROW(conn = pg::detail::Connection::Connect(
                      dsn, task_processor, kConnectionId, settings,
                      GetTestCmdCtls(), {}, {}))
      << "Connect to correct DSN";
  if (!conn) {
    ADD_FAILURE() << "Expected non-empty connection pointer";
  }
  return pg::detail::ConnectionPtr(std::move(conn));
}

std::vector<storages::postgres::Dsn> GetDsnFromEnv();
std::vector<storages::postgres::DsnList> GetDsnListsFromEnv();

std::string DsnToString(
    const ::testing::TestParamInfo<storages::postgres::Dsn>& info);
std::string DsnListToString(
    const ::testing::TestParamInfo<storages::postgres::DsnList>& info);

void PrintBuffer(std::ostream&, const std::uint8_t* buffer, std::size_t size);
inline void PrintBuffer(std::ostream& os, const std::string& buffer) {
  PrintBuffer(os, reinterpret_cast<const std::uint8_t*>(buffer.data()),
              buffer.size());
}

class PostgreSQLBase : public ::testing::Test {
 protected:
  void SetUp() override {
    if (std::getenv(kPostgresLog)) {
      old_ = logging::SetDefaultLogger(
          logging::MakeStderrLogger("cerr", logging::Level::kDebug));
    }
  }

  void TearDown() override {
    if (old_) {
      logging::SetDefaultLogger(std::move(old_));
    }
  }

  static engine::TaskProcessor& GetTaskProcessor();

  void CheckConnection(storages::postgres::detail::ConnectionPtr conn);

 private:
  logging::LoggerPtr old_;
};

class PostgreConnection
    : public PostgreSQLBase,
      public ::testing::WithParamInterface<storages::postgres::DsnList> {
 protected:
  void RunConnectionTest(
      std::function<void(storages::postgres::detail::ConnectionPtr)> func) {
    RunInCoro([this, func] {
      // force connection cleanup to avoid leaving detached tasks behind
      utils::ScopeGuard sync_with_bg(
          [this] { engine::impl::Async(GetTaskProcessor(), [] {}).Wait(); });
      func(MakeConnection(GetParam()[0], GetTaskProcessor()));
    });
  }
};

#define POSTGRE_CASE_TEST_P(test_case_name, test_name)                         \
  class GTEST_TEST_CLASS_NAME_(test_case_name, test_name)                      \
      : public test_case_name {                                                \
   public:                                                                     \
    GTEST_TEST_CLASS_NAME_(test_case_name, test_name)() {}                     \
    virtual void TestBody() {                                                  \
      RunConnectionTest(std::bind(                                             \
          &GTEST_TEST_CLASS_NAME_(test_case_name, test_name)::test_name, this, \
          std::placeholders::_1));                                             \
    }                                                                          \
    void test_name(storages::postgres::detail::ConnectionPtr conn);            \
                                                                               \
   private:                                                                    \
    static int AddToRegistry() {                                               \
      ::testing::UnitTest::GetInstance()                                       \
          ->parameterized_test_registry()                                      \
          .GetTestCasePatternHolder<test_case_name>(                           \
              #test_case_name,                                                 \
              ::testing::internal::CodeLocation(__FILE__, __LINE__))           \
          ->AddTestPattern(                                                    \
              #test_case_name, #test_name,                                     \
              new ::testing::internal::TestMetaFactory<GTEST_TEST_CLASS_NAME_( \
                  test_case_name, test_name)>());                              \
      return 0;                                                                \
    }                                                                          \
    static int gtest_registering_dummy_ GTEST_ATTRIBUTE_UNUSED_;               \
    GTEST_DISALLOW_COPY_AND_ASSIGN_(GTEST_TEST_CLASS_NAME_(test_case_name,     \
                                                           test_name));        \
  };                                                                           \
  int GTEST_TEST_CLASS_NAME_(test_case_name,                                   \
                             test_name)::gtest_registering_dummy_ =            \
      GTEST_TEST_CLASS_NAME_(test_case_name, test_name)::AddToRegistry();      \
  void GTEST_TEST_CLASS_NAME_(test_case_name, test_name)::test_name(           \
      storages::postgres::detail::ConnectionPtr conn)

#define INSTANTIATE_POSTGRE_CASE_P(test_case_name)                    \
  INSTANTIATE_TEST_SUITE_P(/*empty*/, test_case_name,                 \
                           ::testing::ValuesIn(GetDsnListsFromEnv()), \
                           DsnListToString)

#define POSTGRE_TEST_P(test_name) \
  POSTGRE_CASE_TEST_P(PostgreConnection, test_name)
