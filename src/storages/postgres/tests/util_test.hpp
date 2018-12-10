#pragma once

#include <gtest/gtest.h>

#include <cctype>
#include <cstdlib>
#include <vector>

#include <logging/log.hpp>
#include <logging/spdlog.hpp>

#include <spdlog/sinks/stdout_sinks.h>

#include <engine/task/task.hpp>
#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/dsn.hpp>

#include <utest/utest.hpp>

constexpr static const char* kPostgresDsn = "POSTGRES_DSN_TEST";
constexpr uint32_t kConnectionId = 0;

std::vector<std::string> GetDsnFromEnv();
std::vector<storages::postgres::DSNList> GetDsnListFromEnv();

std::string DsnToString(const ::testing::TestParamInfo<std::string>& info);
std::string DsnListToString(
    const ::testing::TestParamInfo<storages::postgres::DSNList>& info);

void PrintBuffer(std::ostream&, const std::uint8_t* buffer, std::size_t size);
inline void PrintBuffer(std::ostream& os, const std::string& buffer) {
  PrintBuffer(os, reinterpret_cast<const std::uint8_t*>(buffer.data()),
              buffer.size());
}

class PostgreSQLBase : public ::testing::Test {
 protected:
  void SetUp() override {
    // TODO Check env if logging is requested
    old_ = logging::SetDefaultLogger(MakeCerrLogger());
    logging::SetDefaultLoggerLevel(logging::Level::kDebug);
    ReadParam();
  }

  void TearDown() override {
    if (old_) {
      logging::SetDefaultLogger(old_);
      old_.reset();
    }
  }

  virtual void ReadParam() = 0;

  static logging::LoggerPtr MakeCerrLogger() {
    static logging::LoggerPtr cerr_logger = spdlog::stderr_logger_mt("cerr");
    return cerr_logger;
  }

  static engine::TaskProcessor& GetTaskProcessor();

  void CheckConnection(storages::postgres::detail::ConnectionPtr conn);

 private:
  logging::LoggerPtr old_;
};

class PostgreConnection
    : public PostgreSQLBase,
      public ::testing::WithParamInterface<storages::postgres::DSNList> {
  void ReadParam() override { dsn_list_ = GetParam(); }

 protected:
  void RunConnectionTest(
      std::function<void(storages::postgres::detail::ConnectionPtr)> func) {
    namespace pg = storages::postgres;
    RunInCoro([this, func] {
      pg::detail::ConnectionPtr conn;

      EXPECT_NO_THROW(conn = pg::detail::Connection::Connect(
                          dsn_list_[0], GetTaskProcessor(), kConnectionId))
          << "Connect to correct DSN";
      func(std::move(conn));
    });
  }
  storages::postgres::DSNList dsn_list_;
};

#define POSTGRE_TEST_P(test_name)                                              \
  class GTEST_TEST_CLASS_NAME_(PostgreConnection, test_name)                   \
      : public PostgreConnection {                                             \
   public:                                                                     \
    GTEST_TEST_CLASS_NAME_(PostgreConnection, test_name)() {}                  \
    virtual void TestBody() {                                                  \
      RunConnectionTest(std::bind(                                             \
          &GTEST_TEST_CLASS_NAME_(PostgreConnection, test_name)::test_name,    \
          this, std::placeholders::_1));                                       \
    }                                                                          \
    void test_name(storages::postgres::detail::ConnectionPtr conn);            \
                                                                               \
   private:                                                                    \
    static int AddToRegistry() {                                               \
      ::testing::UnitTest::GetInstance()                                       \
          ->parameterized_test_registry()                                      \
          .GetTestCasePatternHolder<PostgreConnection>(                        \
              "PostgreConnection",                                             \
              ::testing::internal::CodeLocation(__FILE__, __LINE__))           \
          ->AddTestPattern(                                                    \
              "PostgreConnection", #test_name,                                 \
              new ::testing::internal::TestMetaFactory<GTEST_TEST_CLASS_NAME_( \
                  PostgreConnection, test_name)>());                           \
      return 0;                                                                \
    }                                                                          \
    static int gtest_registering_dummy_ GTEST_ATTRIBUTE_UNUSED_;               \
    GTEST_DISALLOW_COPY_AND_ASSIGN_(GTEST_TEST_CLASS_NAME_(PostgreConnection,  \
                                                           test_name));        \
  };                                                                           \
  int GTEST_TEST_CLASS_NAME_(PostgreConnection,                                \
                             test_name)::gtest_registering_dummy_ =            \
      GTEST_TEST_CLASS_NAME_(PostgreConnection, test_name)::AddToRegistry();   \
  void GTEST_TEST_CLASS_NAME_(PostgreConnection, test_name)::test_name(        \
      storages::postgres::detail::ConnectionPtr conn)
