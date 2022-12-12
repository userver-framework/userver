#include "utils_mysqltest.hpp"

#include <fmt/format.h>

#include <userver/engine/task/task.hpp>
#include <userver/utils/uuid4.hpp>

#include <storages/mysql/settings/host_settings.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::tests {

namespace {

std::string GenerateTableName() {
  auto uuid = utils::generators::GenerateUuid();

  std::string name{"tmp_"};
  name.reserve(4 + uuid.size());
  for (const auto c : uuid) {
    if (c != '-') {
      name.push_back(c);
    }
  }

  return name;
}

std::shared_ptr<Cluster> CreateCluster(clients::dns::Resolver& resolver) {
  settings::HostSettings settings{resolver, "localhost", 3306};

  std::vector<settings::HostSettings> hosts{std::move(settings)};

  return std::make_shared<Cluster>(std::move(hosts));
}

}  // namespace

std::chrono::system_clock::time_point ToMariaDBPrecision(
    std::chrono::system_clock::time_point tp) {
  return std::chrono::time_point_cast<std::chrono::microseconds>(tp);
}

std::string TestsHelper::EscapeString(const Cluster& cluster,
                                      std::string_view source) {
  return cluster.EscapeString(source);
}

ClusterWrapper::ClusterWrapper()
    : resolver_(engine::current_task::GetTaskProcessor(), {}),
      cluster_{CreateCluster(resolver_)},
      // TODO : return to normal
      deadline_{engine::Deadline::FromDuration(std::chrono::seconds{500})} {}
// deadline_{engine::Deadline::FromDuration(utest::kMaxTestWaitTime)} {

ClusterWrapper::~ClusterWrapper() = default;

storages::mysql::Cluster& ClusterWrapper::operator*() const {
  return *cluster_;
}

storages::mysql::Cluster* ClusterWrapper::operator->() const {
  return cluster_.get();
}

engine::Deadline ClusterWrapper::GetDeadline() const { return deadline_; }

TmpTable::TmpTable(ClusterWrapper& cluster, std::string_view definition)
    : cluster_{cluster}, table_name_{GenerateTableName()} {
  auto create_table_query =
      fmt::format(kCreateTableQueryTemplate, table_name_, definition);

  cluster_->Execute(ClusterHostType::kMaster, cluster_.GetDeadline(),
                    create_table_query);
}

TmpTable::~TmpTable() {
  auto drop_table_query = TestsHelper::EscapeString(
      *cluster_, fmt::format(kDropTableQueryTemplate, table_name_));

  try {
    cluster_->Execute(ClusterHostType::kMaster, cluster_.GetDeadline(),
                      drop_table_query);
  } catch (const std::exception& ex) {
    LOG_ERROR() << "Failed to drop temp table " << table_name_ << ": "
                << ex.what();
  }
}

}  // namespace storages::mysql::tests

USERVER_NAMESPACE_END
