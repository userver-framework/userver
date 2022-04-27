#pragma once

#include <userver/utest/utest.hpp>

#include <cctype>
#include <cstdlib>
#include <vector>

#include <userver/engine/async.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/logging/log.hpp>
#include <userver/logging/logger.hpp>

#include <storages/postgres/default_command_controls.hpp>
#include <storages/postgres/detail/connection.hpp>
#include <userver/storages/postgres/detail/connection_ptr.hpp>
#include <userver/storages/postgres/dsn.hpp>

USERVER_NAMESPACE_BEGIN

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
inline const storages::postgres::ConnectionSettings kPipelineEnabled{
    storages::postgres::ConnectionSettings::kCachePreparedStatements,
    storages::postgres::ConnectionSettings::kUserTypesEnabled,
    storages::postgres::ConnectionSettings::kCheckUnused,
    storages::postgres::kDefaultMaxPreparedCacheSize,
    storages::postgres::ConnectionSettings::kPipelineEnabled,
};

engine::Deadline MakeDeadline();

void PrintBuffer(std::ostream&, const std::uint8_t* buffer, std::size_t size);
void PrintBuffer(std::ostream&, const std::string& buffer);

class PostgreSQLBase : public ::testing::Test {
 protected:
  PostgreSQLBase();
  ~PostgreSQLBase() override;

  static storages::postgres::Dsn GetDsnFromEnv();
  static storages::postgres::DsnList GetDsnListFromEnv();
  static engine::TaskProcessor& GetTaskProcessor();

  static storages::postgres::detail::ConnectionPtr MakeConnection(
      const storages::postgres::Dsn& dsn, engine::TaskProcessor& task_processor,
      storages::postgres::ConnectionSettings settings =
          kCachePreparedStatements);

  static void CheckConnection(
      const storages::postgres::detail::ConnectionPtr& conn);
  static void FinalizeConnection(
      storages::postgres::detail::ConnectionPtr conn);

 private:
  logging::LoggerPtr old_;
};

class PostgreConnection : public PostgreSQLBase,
                          public ::testing::WithParamInterface<
                              storages::postgres::ConnectionSettings> {
 protected:
  PostgreConnection();
  ~PostgreConnection();

  storages::postgres::detail::ConnectionPtr conn;
};

USERVER_NAMESPACE_END
