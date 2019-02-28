#pragma once

#include <memory>
#include <type_traits>

#include <boost/optional.hpp>

#include <formats/bson/document.hpp>
#include <storages/mongo_ng/cursor.hpp>
#include <storages/mongo_ng/operations.hpp>

namespace storages::mongo_ng {

namespace impl {
class CollectionImpl;
}  // namespace impl

class Collection {
 public:
  explicit Collection(std::shared_ptr<impl::CollectionImpl>);

  /// Returns the number of documents matching the query
  template <typename... Options>
  size_t Count(formats::bson::Document filter, Options&&... options) const;

  /// @brief Returns an approximated count of all documents in the collection
  /// @note This method uses collection metadata since 4.0 and should be faster
  template <typename... Options>
  size_t CountApprox(Options&&... options) const;

  /// Performs a query on the collection
  template <typename... Options>
  Cursor Find(formats::bson::Document filter, Options&&... options) const;

  /// Retrieves a single document from the collection
  template <typename... Options>
  boost::optional<formats::bson::Document> FindOne(
      formats::bson::Document filter, Options&&... options) const;

  /// Inserts a single document into the collection
  void InsertOne(const formats::bson::Document& document);

  /// @name Prepared operation executors
  /// @{
  size_t Execute(const operations::Count&) const;
  size_t Execute(const operations::CountApprox&) const;
  Cursor Execute(const operations::Find&) const;
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

template <typename... Options>
Cursor Collection::Find(formats::bson::Document filter,
                        Options&&... options) const {
  operations::Find find_op(std::move(filter));
  (find_op.SetOption(std::forward<Options>(options)), ...);
  return Execute(find_op);
}

template <typename... Options>
boost::optional<formats::bson::Document> Collection::FindOne(
    formats::bson::Document filter, Options&&... options) const {
  static_assert(
      !(std::is_same<std::decay_t<Options>, options::Limit>::value || ...),
      "Limit option cannot be used in FindOne");
  auto cursor = Find(std::move(filter), options::Limit{1},
                     std::forward<Options>(options)...);
  if (cursor.begin() == cursor.end()) return {};
  return *cursor.begin();
}

}  // namespace storages::mongo_ng
