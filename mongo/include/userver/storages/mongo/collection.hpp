#pragma once

/// @file userver/storages/mongo/collection.hpp
/// @brief @copybrief storages::mongo::Collection

#include <memory>
#include <optional>
#include <type_traits>
#include <vector>

#include <userver/formats/bson/document.hpp>
#include <userver/formats/bson/value.hpp>
#include <userver/storages/mongo/bulk.hpp>
#include <userver/storages/mongo/cursor.hpp>
#include <userver/storages/mongo/operations.hpp>
#include <userver/storages/mongo/write_result.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo {

namespace impl {
class CollectionImpl;
}  // namespace impl

/// @brief MongoDB collection handle, the main way to operate with MongoDB.
///
/// Usually retrieved from storages::mongo::Pool
///
/// ## Example:
///
/// @snippet storages/mongo/collection_mongotest.hpp  Sample Mongo usage
class Collection {
 public:
  /// @cond
  // For internal use only.
  explicit Collection(std::shared_ptr<impl::CollectionImpl>);
  /// @endcond

  /// @brief Returns the number of documents matching the query
  /// @warning Unless explicitly overridden, runs CountApprox for empty filters
  /// @see options::ForceCountImpl
  template <typename... Options>
  size_t Count(formats::bson::Document filter, Options&&... options) const;

  /// @brief Returns an approximated count of all documents in the collection
  /// @note This method uses collection metadata and should be faster
  template <typename... Options>
  size_t CountApprox(Options&&... options) const;

  /// Performs a query on the collection
  template <typename... Options>
  Cursor Find(formats::bson::Document filter, Options&&... options) const;

  /// Retrieves a single document from the collection
  template <typename... Options>
  std::optional<formats::bson::Document> FindOne(formats::bson::Document filter,
                                                 Options&&... options) const;

  /// Inserts a single document into the collection
  template <typename... Options>
  WriteResult InsertOne(formats::bson::Document document, Options&&... options);

  /// Inserts multiple documents into the collection
  template <typename... Options>
  WriteResult InsertMany(std::vector<formats::bson::Document> documents,
                         Options&&... options);

  /// @brief Replaces a single matching document
  /// @see options::Upsert
  template <typename... Options>
  WriteResult ReplaceOne(formats::bson::Document selector,
                         formats::bson::Document replacement,
                         Options&&... options);

  /// @brief Updates a single matching document
  /// @see options::Upsert
  template <typename... Options>
  WriteResult UpdateOne(formats::bson::Document selector,
                        formats::bson::Document update, Options&&... options);

  /// @brief Updates all matching documents
  /// @see options::Upsert
  template <typename... Options>
  WriteResult UpdateMany(formats::bson::Document selector,
                         formats::bson::Document update, Options&&... options);

  /// Deletes a single matching document
  template <typename... Options>
  WriteResult DeleteOne(formats::bson::Document selector, Options&&... options);

  /// Deletes all matching documents
  template <typename... Options>
  WriteResult DeleteMany(formats::bson::Document selector,
                         Options&&... options);

  /// @brief Atomically updates a single matching document
  /// @see options::ReturnNew
  /// @see options::Upsert
  template <typename... Options>
  WriteResult FindAndModify(formats::bson::Document query,
                            const formats::bson::Document& update,
                            Options&&... options);

  /// Atomically removes a single matching document
  template <typename... Options>
  WriteResult FindAndRemove(formats::bson::Document query,
                            Options&&... options);

  /// Drop collection
  template <typename... Options>
  void Drop(Options&&... options);

  /// Efficiently executes multiple operations in order, stops on error
  template <typename... Options>
  operations::Bulk MakeOrderedBulk(Options&&... options);

  /// Efficiently executes multiple operations out of order, continues on error
  template <typename... Options>
  operations::Bulk MakeUnorderedBulk(Options&&... options);

  /// @brief Executes an aggregation pipeline
  /// @param pipeline an array of aggregation operations
  template <typename... Options>
  Cursor Aggregate(formats::bson::Value pipeline, Options&&... options);

  /// @name Prepared operation executors
  /// @{
  size_t Execute(const operations::Count&) const;
  size_t Execute(const operations::CountApprox&) const;
  Cursor Execute(const operations::Find&) const;
  WriteResult Execute(const operations::InsertOne&);
  WriteResult Execute(const operations::InsertMany&);
  WriteResult Execute(const operations::ReplaceOne&);
  WriteResult Execute(const operations::Update&);
  WriteResult Execute(const operations::Delete&);
  WriteResult Execute(const operations::FindAndModify&);
  WriteResult Execute(const operations::FindAndRemove&);
  WriteResult Execute(operations::Bulk&&);
  Cursor Execute(const operations::Aggregate&);
  void Execute(const operations::Drop&);
  /// @}
 private:
  std::shared_ptr<impl::CollectionImpl> impl_;
};

template <typename... Options>
size_t Collection::Count(formats::bson::Document filter,
                         Options&&... options) const {
  operations::Count count_op(std::move(filter));
  (count_op.SetOption(std::forward<Options>(options)), ...);
  return Execute(count_op);
}

template <typename... Options>
size_t Collection::CountApprox(Options&&... options) const {
  operations::CountApprox count_approx_op;
  (count_approx_op.SetOption(std::forward<Options>(options)), ...);
  return Execute(count_approx_op);
}

namespace impl {

template <typename Option, typename... Options>
using HasOptionHelper =
    std::disjunction<std::is_same<std::decay_t<Options>, Option>...>;

template <typename Option, typename... Options>
static constexpr bool kHasOption = HasOptionHelper<Option, Options...>::value;

}  // namespace impl

template <typename... Options>
Cursor Collection::Find(formats::bson::Document filter,
                        Options&&... options) const {
  operations::Find find_op(std::move(filter));
  (find_op.SetOption(std::forward<Options>(options)), ...);
  return Execute(find_op);
}

template <typename... Options>
std::optional<formats::bson::Document> Collection::FindOne(
    formats::bson::Document filter, Options&&... options) const {
  static_assert(
      !(std::is_same<std::decay_t<Options>, options::Limit>::value || ...),
      "Limit option cannot be used in FindOne");
  auto cursor = Find(std::move(filter), options::Limit{1},
                     std::forward<Options>(options)...);
  if (cursor.begin() == cursor.end()) return {};
  return *cursor.begin();
}

template <typename... Options>
WriteResult Collection::InsertOne(formats::bson::Document document,
                                  Options&&... options) {
  operations::InsertOne insert_op(std::move(document));
  (insert_op.SetOption(std::forward<Options>(options)), ...);
  return Execute(insert_op);
}

template <typename... Options>
WriteResult Collection::InsertMany(
    std::vector<formats::bson::Document> documents, Options&&... options) {
  operations::InsertMany insert_op(std::move(documents));
  (insert_op.SetOption(std::forward<Options>(options)), ...);
  return Execute(insert_op);
}

template <typename... Options>
WriteResult Collection::ReplaceOne(formats::bson::Document selector,
                                   formats::bson::Document replacement,
                                   Options&&... options) {
  operations::ReplaceOne replace_op(std::move(selector),
                                    std::move(replacement));
  (replace_op.SetOption(std::forward<Options>(options)), ...);
  return Execute(replace_op);
}

template <typename... Options>
WriteResult Collection::UpdateOne(formats::bson::Document selector,
                                  formats::bson::Document update,
                                  Options&&... options) {
  operations::Update update_op(operations::Update::Mode::kSingle,
                               std::move(selector), std::move(update));
  (update_op.SetOption(std::forward<Options>(options)), ...);
  return Execute(update_op);
}

template <typename... Options>
WriteResult Collection::UpdateMany(formats::bson::Document selector,
                                   formats::bson::Document update,
                                   Options&&... options) {
  operations::Update update_op(operations::Update::Mode::kMulti,
                               std::move(selector), std::move(update));
  (update_op.SetOption(std::forward<Options>(options)), ...);
  return Execute(update_op);
}

template <typename... Options>
WriteResult Collection::DeleteOne(formats::bson::Document selector,
                                  Options&&... options) {
  operations::Delete delete_op(operations::Delete::Mode::kSingle,
                               std::move(selector));
  (delete_op.SetOption(std::forward<Options>(options)), ...);
  return Execute(delete_op);
}

template <typename... Options>
WriteResult Collection::DeleteMany(formats::bson::Document selector,
                                   Options&&... options) {
  operations::Delete delete_op(operations::Delete::Mode::kMulti,
                               std::move(selector));
  (delete_op.SetOption(std::forward<Options>(options)), ...);
  return Execute(delete_op);
}

template <typename... Options>
WriteResult Collection::FindAndModify(formats::bson::Document query,
                                      const formats::bson::Document& update,
                                      Options&&... options) {
  operations::FindAndModify fam_op(std::move(query), update);
  (fam_op.SetOption(std::forward<Options>(options)), ...);
  return Execute(fam_op);
}

template <typename... Options>
WriteResult Collection::FindAndRemove(formats::bson::Document query,
                                      Options&&... options) {
  operations::FindAndRemove fam_op(std::move(query));
  (fam_op.SetOption(std::forward<Options>(options)), ...);
  return Execute(fam_op);
}

template <typename... Options>
void Collection::Drop(Options&&... options) {
  operations::Drop drop_op;
  (drop_op.SetOption(std::forward<Options>(options)), ...);
  return Execute(drop_op);
}

template <typename... Options>
operations::Bulk Collection::MakeOrderedBulk(Options&&... options) {
  operations::Bulk bulk(operations::Bulk::Mode::kOrdered);
  (bulk.SetOption(std::forward<Options>(options)), ...);
  return bulk;
}

template <typename... Options>
operations::Bulk Collection::MakeUnorderedBulk(Options&&... options) {
  operations::Bulk bulk(operations::Bulk::Mode::kUnordered);
  (bulk.SetOption(std::forward<Options>(options)), ...);
  return bulk;
}

template <typename... Options>
Cursor Collection::Aggregate(formats::bson::Value pipeline,
                             Options&&... options) {
  operations::Aggregate aggregate(std::move(pipeline));
  (aggregate.SetOption(std::forward<Options>(options)), ...);
  return Execute(aggregate);
}

}  // namespace storages::mongo

USERVER_NAMESPACE_END
