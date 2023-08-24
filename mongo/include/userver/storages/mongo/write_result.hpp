#pragma once

#include <cstddef>
#include <optional>
#include <unordered_map>
#include <vector>

#include <userver/formats/bson/document.hpp>
#include <userver/formats/bson/value.hpp>
#include <userver/storages/mongo/mongo_error.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo {

/// MongoDB write operation result
class WriteResult {
 public:
  /// Creates an empty write result
  WriteResult() = default;

  /// @cond
  /// Wraps provided write result, internal use only
  explicit WriteResult(formats::bson::Document);
  /// @endcond

  /// @name Affected document counters
  /// Valid only for acknowledged writes, i.e. non-zero write concern
  /// @{
  size_t InsertedCount() const;
  size_t MatchedCount() const;
  size_t ModifiedCount() const;
  size_t UpsertedCount() const;
  size_t DeletedCount() const;
  /// @}

  /// Map of `_id` values of upserted documents by operation (document) index
  std::unordered_map<size_t, formats::bson::Value> UpsertedIds() const;

  /// The document returned by FindAnd* operation if any
  std::optional<formats::bson::Document> FoundDocument() const;

  /// @brief Map of server errors by operation (document) index.
  ///
  /// For example, storages::mongo::Collection::InsertMany() with
  /// storages::mongo::options::Unordered and
  /// storages::mongo::options::SuppressServerExceptions option would produce a
  /// WriteResult with WriteResult::ServerErrors() containing information about
  /// failed insertions.
  ///
  /// @see options::SuppressServerExceptions
  std::unordered_map<size_t, MongoError> ServerErrors() const;

  /// @brief Write concern errors
  /// @see options::SuppressServerExceptions
  std::vector<MongoError> WriteConcernErrors() const;

 private:
  formats::bson::Document value_;
};

}  // namespace storages::mongo

USERVER_NAMESPACE_END
