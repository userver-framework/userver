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

  engine::TaskWithResult<void> Execute(mongocxx::write_concern wc = {}) {
    return engine::Async(collection_->GetPool().GetTaskProcessor(),
                         [ self = shared_from_this(), wc = std::move(wc) ] {
                           auto conn = self->collection_->GetPool().Acquire();
                           mongocxx::options::bulk_write options;
                           options.ordered(false);
                           options.write_concern(std::move(wc));
                           self->collection_->GetCollection(conn).bulk_write(
                               self->bulk_, options);
                         });
  }

 private:
  std::shared_ptr<CollectionImpl> collection_;
  std::vector<mongocxx::model::write> bulk_;
};

}  // namespace impl
}  // namespace mongo
}  // namespace storages
