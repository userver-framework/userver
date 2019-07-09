#pragma once

#include <memory>

#include <boost/optional.hpp>

#include <formats/bson/document.hpp>

#include <storages/mongo/collection_impl.hpp>
#include <storages/mongo/pool_impl.hpp>
#include <storages/mongo/stats.hpp>
#include <storages/mongo/wrappers.hpp>

namespace storages::mongo::impl {

class CursorImpl {
 public:
  CursorImpl(PoolImpl::BoundClientPtr, CursorPtr,
             std::shared_ptr<stats::ReadOperationStatistics>);

  bool IsValid() const;
  bool HasMore() const;

  const formats::bson::Document& Current() const;
  void Next();

 private:
  boost::optional<formats::bson::Document> current_;
  PoolImpl::BoundClientPtr client_;
  CursorPtr cursor_;
  std::shared_ptr<stats::ReadOperationStatistics> stats_ptr_;
};

}  // namespace storages::mongo::impl
