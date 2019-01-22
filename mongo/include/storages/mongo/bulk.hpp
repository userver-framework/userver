#pragma once

/// @file storages/mongo/bulk.hpp
/// @brief @copybrief storages::mongo::BulkOperationBuilder

#include <memory>

#include <mongocxx/bulk_write.hpp>
#include <mongocxx/write_concern.hpp>

#include <engine/task/task_with_result.hpp>
#include <storages/mongo/mongo.hpp>

namespace storages {
namespace mongo {

namespace impl {
class BulkOperationBuilderImpl;
}  // namespace impl

/// Bulk upsert suboperation interface
class BulkUpsertBuilder {
 public:
  /// @cond
  /// Constructor for internal use
  BulkUpsertBuilder(std::shared_ptr<impl::BulkOperationBuilderImpl>&& bulk,
                    DocumentValue query);
  /// @endcond

  /// Inserts or replaces a single matching document as a part of bulk operation
  void ReplaceOne(DocumentValue) &&;

  /// Inserts or updates a single matching document as a part of bulk operation
  void UpdateOne(DocumentValue) &&;

  /// Inserts or updates all matching documents as a part of bulk operation
  void UpdateMany(DocumentValue) &&;

 private:
  std::shared_ptr<impl::BulkOperationBuilderImpl> bulk_;
  DocumentValue query_;
};

/// Bulk update suboperation interface
class BulkUpdateBuilder {
 public:
  /// @cond
  /// Constructor for internal use
  BulkUpdateBuilder(std::shared_ptr<impl::BulkOperationBuilderImpl> bulk,
                    DocumentValue query);
  /// @endcond

  /// Converts this suboperation into an upsert
  BulkUpsertBuilder Upsert() &&;

  /// Replaces a single matching document as a part of bulk operation
  void ReplaceOne(DocumentValue) &&;

  /// Updates a single matching document as a part of bulk operation
  void UpdateOne(DocumentValue) &&;

  /// Updates all matching documents as a part of bulk operation
  void UpdateMany(DocumentValue) &&;

  /// Deletes a single matching document as a part of bulk operation
  void DeleteOne() &&;

  /// Deletes all matching documents as a part of bulk operation
  void DeleteMany() &&;

 private:
  std::shared_ptr<impl::BulkOperationBuilderImpl> bulk_;
  DocumentValue query_;
};

/// Bulk operation interface
class BulkOperationBuilder {
 public:
  /// @cond
  /// Constructor for internal use
  explicit BulkOperationBuilder(
      std::shared_ptr<impl::BulkOperationBuilderImpl>&&);
  /// @endcond

  /// Interface for update suboperations
  BulkUpdateBuilder Find(DocumentValue) const;

  /// Inserts a single document as a part of bulk operation
  void InsertOne(DocumentValue);

  /// Runs the operation
  engine::TaskWithResult<boost::optional<mongocxx::result::bulk_write>> Execute(
      mongocxx::write_concern);

 private:
  std::shared_ptr<impl::BulkOperationBuilderImpl> bulk_;
};

}  // namespace mongo
}  // namespace storages
