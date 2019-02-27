#pragma once

#include <memory>
#include <vector>

#include <mongocxx/model/write.hpp>

#include <engine/async.hpp>
#include <engine/task/task_with_result.hpp>

#include "collection_impl.hpp"
#include "pool_impl.hpp"

namespace storages {
namespace mongo {
namespace impl {

class BulkOperationBuilderImpl
    : public std::enable_shared_from_this<BulkOperationBuilderImpl> {
 public:
  explicit BulkOperationBuilderImpl(std::shared_ptr<CollectionImpl> collection)
      : collection_(std::move(collection)) {}

  void Append(mongocxx::model::write subop) {
    bulk_.push_back(std::move(subop));
  }

  engine::TaskWithResult<boost::optional<mongocxx::result::bulk_write>> Execute(
      mongocxx::write_concern wc = {}) {
    return engine::impl::Async(
        collection_->GetPool().GetTaskProcessor(),
        [self = shared_from_this(),
         wc = std::move(wc)](impl::PoolImpl::ConnectionToken&& token)
            -> boost::optional<mongocxx::result::bulk_write> {
          auto conn = token.GetConnection();
          mongocxx::options::bulk_write options;
          options.ordered(false);
          options.write_concern(std::move(wc));
          auto result = self->collection_->GetCollection(conn).bulk_write(
              self->bulk_, options);
          if (!result) return boost::none;
          return std::move(*result);
        },
        collection_->GetPool().AcquireToken());
  }

 private:
  std::shared_ptr<CollectionImpl> collection_;
  std::vector<mongocxx::model::write> bulk_;
};

}  // namespace impl
}  // namespace mongo
}  // namespace storages
