#pragma once

#include <cstddef>
#include <vector>

#include <boost/optional.hpp>

#include <formats/bson/document.hpp>
#include <formats/bson/value.hpp>
#include <storages/mongo_ng/mongo_error.hpp>

namespace storages::mongo_ng {

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

  /// @brief `_id` value of upserted document if any, missing value otherwise
  /// @note Not filled for bulk operations.
  formats::bson::Value UpsertedId() const;

  /// The document returned by FindAnd* operation if any
  boost::optional<formats::bson::Document> FoundDocument() const;

  /// @brief Server errors
  /// @see options::SuppressServerExceptions
  std::vector<MongoError> ServerErrors() const;

  /// @brief Write concern errors
  /// @see options::SuppressServerExceptions
  std::vector<MongoError> WriteConcernErrors() const;

 private:
  formats::bson::Document value_;
};

}  // namespace storages::mongo_ng
