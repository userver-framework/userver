#pragma once

#include <cstddef>
#include <unordered_map>
#include <vector>

#include <boost/optional.hpp>

#include <formats/bson/document.hpp>
#include <formats/bson/value.hpp>
#include <storages/mongo/mongo_error.hpp>

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
  boost::optional<formats::bson::Document> FoundDocument() const;

  /// @brief Map of server errors by operation (document) index
  /// @see options::SuppressServerExceptions
  std::unordered_map<size_t, MongoError> ServerErrors() const;

  /// @brief Write concern errors
  /// @see options::SuppressServerExceptions
  std::vector<MongoError> WriteConcernErrors() const;

 private:
  formats::bson::Document value_;
};

}  // namespace storages::mongo
