#pragma once

#include <ydb-cpp-sdk/client/table/table.h>
#include <ydb-cpp-sdk/client/topic/topic.h>

#include <userver/dynamic_config/test_helpers.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/enumerate.hpp>
#include <userver/utils/statistics/storage.hpp>
#include <userver/utils/statistics/testing.hpp>

#include <userver/ydb/coordination.hpp>
#include <userver/ydb/io/supported_types.hpp>
#include <userver/ydb/settings.hpp>
#include <userver/ydb/table.hpp>
#include <userver/ydb/topic.hpp>
#include <ydb/impl/config.hpp>
#include <ydb/impl/driver.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb {

class TestClientsBase {
 public:
  std::string_view GetDatabasePath() const { return driver_->GetDbPath(); }

  utils::statistics::Snapshot GetMetrics() const {
    return utils::statistics::Snapshot{statistics_storage_};
  }

  impl::Driver& GetDriver() { return *driver_; }

  ydb::TableClient& GetTableClient() { return *table_client_; }

  NYdb::NTable::TTableClient& GetNativeTableClient() {
    return table_client_->GetNativeTableClient();
  }

  ydb::TopicClient& GetTopicClient() { return *topic_client_; }

  NYdb::NTopic::TTopicClient& GetNativeTopicClient() {
    return topic_client_->GetNativeTopicClient();
  }

  ydb::CoordinationClient& GetCoordinationClient() {
    return *coordination_client_;
  }

  std::shared_ptr<ydb::CoordinationClient> GetCoordinationClientPtr() {
    return coordination_client_;
  }

  NYdb::NCoordination::TClient& GetNativeCoordinationClient() {
    return coordination_client_->GetNativeCoordinationClient();
  }

 protected:
  void InitializeClients() {
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    const char* endpoint = std::getenv("YDB_ENDPOINT");
    UINVARIANT(endpoint, "YDB_ENDPOINT env missing");
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    const char* database = std::getenv("YDB_DATABASE");
    UINVARIANT(database, "YDB_DATABASE env missing");

    ydb::impl::DriverSettings driver_settings;
    driver_settings.endpoint = endpoint;
    driver_settings.database = database;

    ydb::impl::TableSettings table_settings;

    ydb::OperationSettings query_params = {
        3,                                      // retries
        kMaxYdbBootTimeout,                     // operation_timeout_ms
        kMaxYdbBootTimeout,                     // cancel_after_ms
        kMaxYdbBootTimeout,                     // client_timeout_ms
        ydb::TransactionMode::kSerializableRW,  // tx_mode
        kMaxYdbBootTimeout,                     // get_session_timeout_ms
    };

    driver_ = std::make_shared<ydb::impl::Driver>(database, driver_settings);

    table_client_ = std::make_shared<ydb::TableClient>(
        table_settings, query_params, dynamic_config::GetDefaultSource(),
        driver_);

    topic_client_ =
        std::make_shared<ydb::TopicClient>(driver_, ydb::impl::TopicSettings{});

    coordination_client_ = std::make_shared<ydb::CoordinationClient>(driver_);

    statistics_holder_ = statistics_storage_.RegisterWriter(
        "ydb", [this](utils::statistics::Writer& writer) {
          writer = *driver_;
          writer = *table_client_;
        });
  }

 private:
  static constexpr auto kMaxYdbBootTimeout = std::chrono::minutes{3};

  std::shared_ptr<ydb::impl::Driver> driver_;
  std::shared_ptr<ydb::TableClient> table_client_;
  std::shared_ptr<ydb::TopicClient> topic_client_;
  std::shared_ptr<ydb::CoordinationClient> coordination_client_;
  utils::statistics::Storage statistics_storage_;
  utils::statistics::Entry statistics_holder_;
};

// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class ClientFixtureBase : public TestClientsBase, public ::testing::Test {
 protected:
  ClientFixtureBase() { InitializeClients(); }

  ~ClientFixtureBase() override {
    for (const auto& table : tables_created_) {
      try {
        GetTableClient().DropTable(table);
      } catch (const std::exception& ex) {
        ADD_FAILURE() << "Failed to drop table '" << table
                      << "' after test: " << ex.what();
      }
    }
  }

  void DoCreateTable(std::string_view path,
                     NYdb::NTable::TTableDescription&& table_desc) {
    GetTableClient().CreateTable(path, std::move(table_desc));
    tables_created_.emplace_back(path);
  }

 private:
  std::vector<std::string> tables_created_;
};

}  // namespace ydb

template <typename T>
T GetFutureValueSyncChecked(NThreading::TFuture<T>&& future) {
  auto value = future.ExtractValueSync();
  NYdb::TStatus& status = value;
  if (!value.IsSuccess()) {
    throw ydb::YdbResponseError{"Test", std::move(status)};
  }
  return value;
}

USERVER_NAMESPACE_END
