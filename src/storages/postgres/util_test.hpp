#pragma once

#include <gtest/gtest.h>

#include <cctype>
#include <cstdlib>
#include <vector>

#include <logging/log.hpp>
#include <logging/spdlog.hpp>

#include <spdlog/sinks/stdout_sinks.h>

#include <engine/task/task.hpp>
#include <storages/postgres/dsn.hpp>
#include <storages/postgres/postgres_fwd.hpp>

constexpr static const char* kPostgresDsn = "POSTGRES_DSN_TEST";

std::vector<storages::postgres::DSNList> GetDsnFromEnv();

std::string DsnToString(
    const ::testing::TestParamInfo<storages::postgres::DSNList>& info);

class PostgreSQLBase : public ::testing::Test {
 protected:
  void SetUp() override {
    // TODO Check env if logging is requested
    old_ = logging::SetDefaultLogger(MakeCerrLogger());
    logging::SetDefaultLoggerLevel(logging::Level::kTrace);
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
