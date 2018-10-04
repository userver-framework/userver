#pragma once

/// @file storages/mongo/collection.hpp
/// @brief @copybrief storages::mongo::Collection

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

/// Query projection set holder
class FieldsToReturn {
 public:
  /// Constructor from existing projection document
  /*implicit*/ FieldsToReturn(DocumentValue);

  /// Constructor from list of fields to return
  FieldsToReturn(std::initializer_list<std::string>);

  /// Projection document access
  operator bsoncxx::document::view_or_value() &&;

 private:
  DocumentValue fields_;
};

/// Database collection interface
class Collection {
 public:
  /// @cond
  /// Constructor for internal use
  explicit Collection(std::shared_ptr<impl::CollectionImpl>&&);
  /// @endcond

  /// @{
  /// Finds a single document that match the provided filter
  engine::TaskWithResult<boost::optional<DocumentValue>> FindOne(
      DocumentValue query, mongocxx::options::find options = {}) const;
  engine::TaskWithResult<boost::optional<DocumentValue>> FindOne(
      DocumentValue query, FieldsToReturn fields,
      mongocxx::options::find options = {}) const;
  /// @}

  /// @{
  /// Finds the documents matching the provided filter
  engine::TaskWithResult<Cursor> Find(
      DocumentValue query, mongocxx::options::find options = {}) const;
  engine::TaskWithResult<Cursor> Find(
      DocumentValue query, FieldsToReturn fields,
      mongocxx::options::find options = {}) const;
  /// @}

  /// Counts the number of documents matching the provided filter
  engine::TaskWithResult<size_t> Count(
      DocumentValue query, mongocxx::options::count options = {}) const;

  /// Deletes a single document matching the filter and returns the original
  engine::TaskWithResult<boost::optional<DocumentValue>> FindOneAndDelete(
      DocumentValue query, mongocxx::options::find_one_and_delete options = {});

  /// @brief Replaces a single document matching the filter and returns it
  /// @note Returns new value by default, you need to set the `return_document`
  /// option to get the original
  engine::TaskWithResult<boost::optional<DocumentValue>> FindOneAndReplace(
      DocumentValue query, DocumentValue replacement,
      mongocxx::options::find_one_and_replace options = {});

  /// @brief Updates a single document matching the filter and returns it
  /// @note Returns new value by default, you need to set the `return_document`
  /// option to get the original
  engine::TaskWithResult<boost::optional<DocumentValue>> FindOneAndUpdate(
      DocumentValue query, DocumentValue update,
      mongocxx::options::find_one_and_update options = {});

  /// Inserts a single document
  engine::TaskWithResult<void> InsertOne(
      DocumentValue obj, mongocxx::options::insert options = {});

  /// @brief Deletes a single matching document
  /// @returns whether a document was deleted
  engine::TaskWithResult<bool> DeleteOne(
      DocumentValue query, mongocxx::options::delete_options options = {});

  /// @brief Deletes all matching documents
  /// @returns number of deleted documents
  engine::TaskWithResult<size_t> DeleteMany(
      DocumentValue query, mongocxx::options::delete_options options = {});

  /// @brief Replaces a single matching document
  /// @returns whether a document was matched
  engine::TaskWithResult<bool> ReplaceOne(
      DocumentValue query, DocumentValue replacement,
      mongocxx::options::update options = {});

  /// @brief Updates a single matching document
  /// @returns whether a document was matched
  engine::TaskWithResult<bool> UpdateOne(
      DocumentValue query, DocumentValue update,
      mongocxx::options::update options = {});

  /// @brief Updates all matching documents
  /// @returns number of matched documents
  engine::TaskWithResult<size_t> UpdateMany(
      DocumentValue query, DocumentValue update,
      mongocxx::options::update options = {});

  /// Unordered bulk operation interface
  BulkOperationBuilder UnorderedBulk();

 private:
  std::shared_ptr<impl::CollectionImpl> impl_;
};

}  // namespace mongo
}  // namespace storages
