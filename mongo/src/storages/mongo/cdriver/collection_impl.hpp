#pragma once

#include <chrono>
#include <memory>
#include <optional>

#include <storages/mongo/cdriver/pool_impl.hpp>
#include <storages/mongo/collection_impl.hpp>
#include <storages/mongo/stats.hpp>
#include <userver/dynamic_config/snapshot.hpp>
#include <userver/tracing/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl::cdriver {

struct RequestContext final {
  std::shared_ptr<stats::OperationStatisticsItem> stats;
  dynamic_config::Snapshot dynamic_config;
  CDriverPoolImpl::BoundClientPtr client;
  CollectionPtr collection;
  tracing::Span span;
  std::optional<std::chrono::milliseconds> inherited_deadline;
};

class CDriverCollectionImpl : public CollectionImpl {
 public:
  CDriverCollectionImpl(PoolImplPtr pool_impl, std::string database_name,
                        std::string collection_name);

  size_t Execute(const operations::Count&) const override;
  size_t Execute(const operations::CountApprox&) const override;
  Cursor Execute(const operations::Find&) const override;
  WriteResult Execute(const operations::InsertOne&) override;
  WriteResult Execute(const operations::InsertMany&) override;
  WriteResult Execute(const operations::ReplaceOne&) override;
  WriteResult Execute(const operations::Update&) override;
  WriteResult Execute(const operations::Delete&) override;
  WriteResult Execute(const operations::FindAndModify&) override;
  WriteResult Execute(const operations::FindAndRemove&) override;
  WriteResult Execute(operations::Bulk&&) override;
  Cursor Execute(const operations::Aggregate&) override;
  void Execute(const operations::Drop&) override;

 private:
  cdriver::CDriverPoolImpl::BoundClientPtr GetClient(
      stats::OperationStatisticsItem& stats) const;

  RequestContext MakeRequestContext(std::string&& span_name,
                                    const stats::OperationKey& stats_key) const;

  template <typename Operation>
  RequestContext MakeRequestContext(std::string&& span_name,
                                    const Operation& operation) const;

  PoolImplPtr pool_impl_;
  std::shared_ptr<stats::CollectionStatistics> statistics_;
};

}  // namespace storages::mongo::impl::cdriver

USERVER_NAMESPACE_END
