#pragma once

#include <memory>
#include <tuple>

#include <storages/mongo/cdriver/pool_impl.hpp>
#include <storages/mongo/collection_impl.hpp>
#include <storages/mongo/stats.hpp>
#include <userver/tracing/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl::cdriver {

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
  DropResult Execute(const operations::Drop&) override;

 private:
  cdriver::CDriverPoolImpl::BoundClientPtr GetCDriverClient() const;
  std::tuple<cdriver::CDriverPoolImpl::BoundClientPtr, cdriver::CollectionPtr>
  GetCDriverCollection() const;
  std::chrono::milliseconds GetDefaultMaxServerTime() const;
  void SetDefaultMaxServerTime(formats::bson::impl::BsonBuilder& builder,
                               bool& has_max_server_time_option) const;
  void SetDefaultMaxServerTime(mongoc_find_and_modify_opts_t* options,
                               bool& has_max_server_time_option) const;

  PoolImplPtr pool_impl_;
  std::shared_ptr<stats::CollectionStatistics> statistics_;
};

}  // namespace storages::mongo::impl::cdriver

USERVER_NAMESPACE_END
