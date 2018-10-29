#include <storages/postgres/cluster.hpp>

#include <storages/postgres/detail/cluster_impl.hpp>

namespace storages {
namespace postgres {

Cluster::Cluster(detail::ClusterImplPtr&& impl) : pimpl_(std::move(impl)) {}

Cluster::~Cluster() = default;

Transaction Cluster::Begin(const TransactionOptions& options) {
  return Begin(ClusterHostType::kAny, options);
}

Transaction Cluster::Begin(ClusterHostType ht,
                           const TransactionOptions& options) {
  return pimpl_->Begin(ht, options);
}

ResultSet Cluster::Execute(const TransactionOptions& options,
                           const std::string& statement) {
  return Execute(ClusterHostType::kAny, options, statement);
}

ResultSet Cluster::Execute(ClusterHostType ht,
                           const TransactionOptions& options,
                           const std::string& statement) {
  return Execute<>(ht, options, statement);
}

}  // namespace postgres
}  // namespace storages
