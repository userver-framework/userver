#pragma once

/// @file storages/mongo_ng/options.hpp
/// @brief Query options

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <formats/bson/document.hpp>
#include <formats/bson/value_builder.hpp>

namespace storages::mongo_ng::options {

/// @brief Read preference
/// @see https://docs.mongodb.com/manual/reference/read-preference/
class ReadPreference {
 public:
  enum Mode {
    /// read from primary, default mode
    kPrimary,
    /// read from secondary
    kSecondary,
    /// read from primary if available, fallback to secondary
    kPrimaryPreferred,
    /// read from secondary if available, fallback to primary
    kSecondaryPreferred,
    /// read from any host with the lowest latency
    kNearest,
  };

  explicit ReadPreference(Mode mode);
  ReadPreference(Mode mode, std::vector<formats::bson::Document> tags);

  Mode GetMode() const;
  const std::vector<formats::bson::Document>& GetTags() const;

  /// @brief Adds a tag to the tag set.
  /// @note Cannot be used with kPrimary mode.
  ReadPreference& AddTag(formats::bson::Document tag);

 private:
  Mode mode_;
  std::vector<formats::bson::Document> tags_;
};

/// @brief Read concern
/// @see https://docs.mongodb.org/manual/reference/readConcern/
enum class ReadConcern {
  /// no replication checks, default level
  kLocal,
  /// return data replicated to a majority of RS members
  kMajority,
  /// waits for all running majority writes to finish before read
  kLinearizable,
  /// no replication checks, may return orphaned documents if sharded; since 3.6
  kAvailable,
};

/// Specifies the number of documents to skip
class Skip {
 public:
  explicit Skip(size_t value) : value_(value) {}

  size_t Value() const { return value_; }

 public:
  size_t value_;
};

/// @brief Specifies the number of documents to request from the server
/// @note The value of `0` means "no limit".
class Limit {
 public:
  explicit Limit(size_t value) : value_(value) {}

  size_t Value() const { return value_; }

 private:
  size_t value_;
};

/// @brief Selects fields to be returned
/// @note `_id` field is always included by default
/// @see
/// https://docs.mongodb.com/manual/tutorial/project-fields-from-query-results/
class Projection {
 public:
  /// Creates an empty projection including all fields
  Projection();

  /// Creates a projection including only specified fields
  Projection(std::initializer_list<std::string> fields_to_include);

  /// @brief Adopts an existing projection document
  /// @warning No client-side checks are performed.
  explicit Projection(formats::bson::Document);

  /// Includes a field into the projection
  Projection& Include(const std::string& field);

  /// @brief Excludes a field from the projection
  /// @warning Projection cannot have a mix of inclusion and exclusion.
  /// Only the `_id` field can always be excluded.
  Projection& Exclude(const std::string& field);

  /// @brief Setups an array slice in the projection
  /// @param field name of the array field to slice
  /// @param limit the number of items to return
  /// @param skip the following number of items
  /// @note `skip` can be negative, this corresponds to counting from the end
  /// backwards.
  /// @note `limit < 0, skip == 0` is equivalent to `limit' = -limit, skip' =
  /// limit`.
  /// @warning Cannot be applied to views.
  Projection& Slice(const std::string& field, int32_t limit, int32_t skip = 0);

  /// @brief Matches the first element of an array satisfying a predicate
  /// @param field name of the array to search
  /// @param pred predicate to apply to elements
  /// @note Array field will be absent from the result if no elements match.
  /// @note Empty document as a predicate will only match empty documents.
  Projection& ElemMatch(const std::string& field, formats::bson::Document pred);

  /// @cond
  /// Returns whether none of projections are set
  bool IsEmpty() const;

  /// Retrieves projection document
  formats::bson::Document Extract() &&;
  /// @endcond

 private:
  formats::bson::ValueBuilder builder_;
};

/// Sorts the results
class Sort {
 public:
  enum Direction {
    kAscending,
    kDescending,
  };
  using Order = std::vector<std::pair<std::string, Direction>>;

  /// Creates an empty ordering specification
  Sort() = default;

  /// Stores the specified ordering specification
  Sort(std::initializer_list<Order::value_type>);

  /// Stores the specified ordering specification
  explicit Sort(Order);

  /// Appends a field to the ordering specification
  Sort& By(std::string field, Direction direction);

  const Order& GetOrder() const;

 private:
  Order order_;
};

/// @brief Specifies an index to use for the query
/// @warning Only plans using the index will be considered.
class Hint {
 public:
  /// Specifies an index by name
  explicit Hint(std::string index_name);

  /// Specifies an index by fields covered
  explicit Hint(formats::bson::Document index_spec);

  /// @cond
  /// Retrieves a hint value
  const formats::bson::Value& Value() const;
  /// @endcond

 private:
  formats::bson::Value value_;
};

/// Suppresses errors on querying a sharded collection with unavailable shards
class AllowPartialResults {};

/// @brief Enables tailable cursor, which block at the end of capped collections
/// @note Automatically sets `awaitData`.
/// @see https://docs.mongodb.com/manual/core/tailable-cursors/
class Tailable {};

/// Sets a comment for the operation, which would be visible in profile data
class Comment {
 public:
  explicit Comment(std::string);

  const std::string& Value() const;

 private:
  std::string value_;
};

/// @brief Specifies the number of documents to retrive at a time
/// @note The value of `0` uses the default batch size
class BatchSize {
 public:
  explicit BatchSize(size_t value) : value_(value) {}

  size_t Value() const { return value_; }

 private:
  size_t value_;
};

/// @brief Specifies the server-side time limit for the operation
/// @warning This does not set any client-side timeouts.
class MaxServerTime {
 public:
  template <typename Rep, typename Period>
  explicit MaxServerTime(const std::chrono::duration<Rep, Period>& value)
      : value_(value) {}

  const std::chrono::milliseconds& Value() const { return value_; }

 private:
  std::chrono::milliseconds value_;
};

}  // namespace storages::mongo_ng::options
