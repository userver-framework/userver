#pragma once

#include <utest/utest.hpp>

#include <cctype>
#include <cstdlib>
#include <vector>

#include <engine/async.hpp>
#include <engine/task/task.hpp>
#include <logging/log.hpp>
#include <logging/logger.hpp>

#include <storages/postgres/default_command_controls.hpp>
#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/detail/connection_ptr.hpp>
#include <storages/postgres/dsn.hpp>

inline constexpr std::uint32_t kConnectionId = 0;

inline constexpr storages::postgres::CommandControl kTestCmdCtl{
    // TODO: lower execute timeout after TAXICOMMON-1313
    std::chrono::seconds{1}, std::chrono::milliseconds{500}};

storages::postgres::DefaultCommandControls GetTestCmdCtls();

class DefaultCommandControlScope {
 public:
  explicit DefaultCommandControlScope(
      storages::postgres::CommandControl default_cmd_ctl);

  ~DefaultCommandControlScope();

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

engine::Deadline MakeDeadline();

storages::postgres::detail::ConnectionPtr MakeConnection(
    const storages::postgres::Dsn& dsn, engine::TaskProcessor& task_processor,
    storages::postgres::ConnectionSettings settings = kCachePreparedStatements);

std::vector<storages::postgres::Dsn> GetDsnFromEnv();
std::vector<storages::postgres::DsnList> GetDsnListsFromEnv();

std::string DsnToString(
    const ::testing::TestParamInfo<storages::postgres::Dsn>& info);
std::string DsnListToString(
    const ::testing::TestParamInfo<storages::postgres::DsnList>& info);

void PrintBuffer(std::ostream&, const std::uint8_t* buffer, std::size_t size);
void PrintBuffer(std::ostream&, const std::string& buffer);

class PostgreSQLBase : public ::testing::Test {
 protected:
  PostgreSQLBase();
  ~PostgreSQLBase() override;

  static engine::TaskProcessor& GetTaskProcessor();

  void CheckConnection(storages::postgres::detail::ConnectionPtr conn);

 private:
  logging::LoggerPtr old_;
};

class PostgreConnection
    : public PostgreSQLBase,
      public ::testing::WithParamInterface<storages::postgres::DsnList> {
 protected:
  PostgreConnection();
  ~PostgreConnection();

  storages::postgres::detail::ConnectionPtr conn;
};
