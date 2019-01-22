#include <storages/mongo/bulk.hpp>

#include <mongocxx/model/delete_many.hpp>
#include <mongocxx/model/delete_one.hpp>
#include <mongocxx/model/insert_one.hpp>
#include <mongocxx/model/replace_one.hpp>
#include <mongocxx/model/update_many.hpp>
#include <mongocxx/model/update_one.hpp>

#include "bulk_impl.hpp"

namespace storages {
namespace mongo {

BulkUpsertBuilder::BulkUpsertBuilder(
    std::shared_ptr<impl::BulkOperationBuilderImpl>&& bulk, DocumentValue query)
    : bulk_(std::move(bulk)), query_(std::move(query)) {}

void BulkUpsertBuilder::ReplaceOne(DocumentValue obj) && {
  bulk_->Append(mongocxx::model::replace_one(std::move(query_), std::move(obj))
                    .upsert(true));
}

void BulkUpsertBuilder::UpdateOne(DocumentValue obj) && {
  bulk_->Append(mongocxx::model::update_one(std::move(query_), std::move(obj))
                    .upsert(true));
}

void BulkUpsertBuilder::UpdateMany(DocumentValue obj) && {
  bulk_->Append(mongocxx::model::update_many(std::move(query_), std::move(obj))
                    .upsert(true));
}

BulkUpdateBuilder::BulkUpdateBuilder(
    std::shared_ptr<impl::BulkOperationBuilderImpl> bulk, DocumentValue query)
    : bulk_(std::move(bulk)), query_(std::move(query)) {}

BulkUpsertBuilder BulkUpdateBuilder::Upsert() && {
  return BulkUpsertBuilder(std::move(bulk_), std::move(query_));
}

void BulkUpdateBuilder::ReplaceOne(DocumentValue obj) && {
  bulk_->Append(mongocxx::model::replace_one(std::move(query_), std::move(obj))
                    .upsert(false));
}

void BulkUpdateBuilder::UpdateOne(DocumentValue obj) && {
  bulk_->Append(mongocxx::model::update_one(std::move(query_), std::move(obj))
                    .upsert(false));
}

void BulkUpdateBuilder::UpdateMany(DocumentValue obj) && {
  bulk_->Append(mongocxx::model::update_many(std::move(query_), std::move(obj))
                    .upsert(false));
}

void BulkUpdateBuilder::DeleteOne() && {
  bulk_->Append(mongocxx::model::delete_one(std::move(query_)));
}

void BulkUpdateBuilder::DeleteMany() && {
  bulk_->Append(mongocxx::model::delete_many(std::move(query_)));
}

BulkOperationBuilder::BulkOperationBuilder(
    std::shared_ptr<impl::BulkOperationBuilderImpl>&& bulk)
    : bulk_(std::move(bulk)) {}

BulkUpdateBuilder BulkOperationBuilder::Find(DocumentValue query) const {
  return BulkUpdateBuilder(bulk_, std::move(query));
}

void BulkOperationBuilder::InsertOne(DocumentValue obj) {
  bulk_->Append(mongocxx::model::insert_one(std::move(obj)));
}

engine::TaskWithResult<boost::optional<mongocxx::result::bulk_write>>
BulkOperationBuilder::Execute(mongocxx::write_concern wc) {
  return bulk_->Execute(std::move(wc));
}

}  // namespace mongo
}  // namespace storages
