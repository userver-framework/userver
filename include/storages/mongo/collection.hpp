#pragma once

#include <initializer_list>
#include <memory>
#include <string>

#include <boost/optional.hpp>
#include <bsoncxx/document/view_or_value.hpp>
#include <mongocxx/collection.hpp>

#include <engine/task/task_with_result.hpp>
#include <storages/mongo/bulk.hpp>
#include <storages/mongo/cursor.hpp>
#include <storages/mongo/mongo.hpp>
#include <utils/flags.hpp>

namespace storages {
namespace mongo {

namespace impl {
class CollectionImpl;
}  // namespace impl

class FieldsToReturn {
 public:
  /*implicit*/ FieldsToReturn(DocumentValue);
  FieldsToReturn(std::initializer_list<std::string>);

  operator bsoncxx::document::view_or_value() &&;

 private:
  DocumentValue fields_;
};

class Collection {
 public:
  explicit Collection(std::shared_ptr<impl::CollectionImpl>&&);

  engine::TaskWithResult<boost::optional<DocumentValue>> FindOne(
      DocumentValue query, mongocxx::options::find options = {}) const;
  engine::TaskWithResult<boost::optional<DocumentValue>> FindOne(
      DocumentValue query, FieldsToReturn fields,
      mongocxx::options::find options = {}) const;

  engine::TaskWithResult<Cursor> Find(
      DocumentValue query, mongocxx::options::find options = {}) const;
  engine::TaskWithResult<Cursor> Find(
      DocumentValue query, FieldsToReturn fields,
      mongocxx::options::find options = {}) const;

  engine::TaskWithResult<size_t> Count(
      DocumentValue query, mongocxx::options::count options = {}) const;

  engine::TaskWithResult<boost::optional<DocumentValue>> FindOneAndDelete(
      DocumentValue query, mongocxx::options::find_one_and_delete options = {});

  // Returns new value by default
  engine::TaskWithResult<boost::optional<DocumentValue>> FindOneAndReplace(
      DocumentValue query, DocumentValue replacement,
      mongocxx::options::find_one_and_replace options = {});
  engine::TaskWithResult<boost::optional<DocumentValue>> FindOneAndUpdate(
      DocumentValue query, DocumentValue update,
      mongocxx::options::find_one_and_update options = {});

  engine::TaskWithResult<void> InsertOne(
      DocumentValue obj, mongocxx::options::insert options = {});

  engine::TaskWithResult<bool> DeleteOne(
      DocumentValue query, mongocxx::options::delete_options options = {});
  engine::TaskWithResult<size_t> DeleteMany(
      DocumentValue query, mongocxx::options::delete_options options = {});

  engine::TaskWithResult<bool> ReplaceOne(
      DocumentValue query, DocumentValue replacement,
      mongocxx::options::update options = {});
  engine::TaskWithResult<bool> UpdateOne(
      DocumentValue query, DocumentValue update,
      mongocxx::options::update options = {});
  engine::TaskWithResult<size_t> UpdateMany(
      DocumentValue query, DocumentValue update,
      mongocxx::options::update options = {});

  BulkOperationBuilder UnorderedBulk();

 private:
  std::shared_ptr<impl::CollectionImpl> impl_;
};

}  // namespace mongo
}  // namespace storages
