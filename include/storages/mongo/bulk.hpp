#pragma once

#include <memory>

#include <mongocxx/write_concern.hpp>

#include <engine/task/task_with_result.hpp>
#include <storages/mongo/mongo.hpp>

namespace storages {
namespace mongo {

namespace impl {
class BulkOperationBuilderImpl;
}  // namespace impl

class BulkUpsertBuilder {
 public:
  BulkUpsertBuilder(std::shared_ptr<impl::BulkOperationBuilderImpl>&& bulk,
                    DocumentValue query);

  void ReplaceOne(DocumentValue) &&;
  void UpdateOne(DocumentValue) &&;
  void UpdateMany(DocumentValue) &&;

 private:
  std::shared_ptr<impl::BulkOperationBuilderImpl> bulk_;
  DocumentValue query_;
};

class BulkUpdateBuilder {
 public:
  BulkUpdateBuilder(std::shared_ptr<impl::BulkOperationBuilderImpl> bulk,
                    DocumentValue query);

  BulkUpsertBuilder Upsert() &&;

  void ReplaceOne(DocumentValue) &&;
  void UpdateOne(DocumentValue) &&;
  void UpdateMany(DocumentValue) &&;
  void DeleteOne() &&;
  void DeleteMany() &&;

 private:
  std::shared_ptr<impl::BulkOperationBuilderImpl> bulk_;
  DocumentValue query_;
};

class BulkOperationBuilder {
 public:
  explicit BulkOperationBuilder(
      std::shared_ptr<impl::BulkOperationBuilderImpl>&&);

  BulkUpdateBuilder Find(DocumentValue) const;
  void InsertOne(DocumentValue);
  engine::TaskWithResult<void> Execute(mongocxx::write_concern);

 private:
  std::shared_ptr<impl::BulkOperationBuilderImpl> bulk_;
};

}  // namespace mongo
}  // namespace storages
