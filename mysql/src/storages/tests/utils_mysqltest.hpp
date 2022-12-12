#pragma once

// TODO : drop
#include <iostream>

#include <userver/utest/utest.hpp>

#include <userver/clients/dns/resolver.hpp>
#include <userver/engine/deadline.hpp>
// TODO : drop
#include <userver/engine/sleep.hpp>
#include <userver/utils/uuid4.hpp>

#include <userver/storages/mysql.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::tests {

std::uint32_t GetMysqlPort();

std::chrono::system_clock::time_point ToMariaDBPrecision(
    std::chrono::system_clock::time_point tp);

class TestsHelper final {
 public:
  static std::string EscapeString(const Cluster& cluster,
                                  std::string_view source);
};

class ClusterWrapper final {
 public:
  ClusterWrapper();
  ~ClusterWrapper();

  storages::mysql::Cluster& operator*() const;
  storages::mysql::Cluster* operator->() const;

  engine::Deadline GetDeadline() const;

  template <typename... Args>
  StatementResultSet DefaultExecute(const std::string& query,
                                    const Args&... args) const;

 private:
  clients::dns::Resolver resolver_;
  std::shared_ptr<storages::mysql::Cluster> cluster_;

  engine::Deadline deadline_;
};

template <typename... Args>
StatementResultSet ClusterWrapper::DefaultExecute(const std::string& query,
                                                  const Args&... args) const {
  return cluster_->Execute(ClusterHostType::kMaster, GetDeadline(), query,
                           args...);
}

class TmpTable final {
 public:
  TmpTable(ClusterWrapper& cluster, std::string_view definition);
  ~TmpTable();

  template <typename... Args>
  std::string FormatWithTableName(std::string_view source, const Args&... args);

 private:
  static constexpr std::string_view kCreateTableQueryTemplate =
      "CREATE TABLE {} {}";
  static constexpr std::string_view kDropTableQueryTemplate = "DROP TABLE {}";

  ClusterWrapper& cluster_;
  std::string table_name_;
};

template <typename... Args>
std::string TmpTable::FormatWithTableName(std::string_view source,
                                          const Args&... args) {
  return fmt::format(source, table_name_, args...);
}

}  // namespace storages::mysql::tests

USERVER_NAMESPACE_END
