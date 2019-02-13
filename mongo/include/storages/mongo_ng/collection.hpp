#pragma once

#include <memory>

#include <formats/bson/document.hpp>

namespace storages::mongo_ng {

namespace impl {
class CollectionImpl;
}  // namespace impl

class Collection {
 public:
  explicit Collection(std::shared_ptr<impl::CollectionImpl>);

  /// @brief Returns an approximated count of all documents in the collection
  /// @note This method uses collection metadata since 4.0 and should be faster
  size_t CountApprox() const;

  /// Returns the number of documents matching the query
  size_t Count(const formats::bson::Document& filter) const;

  void InsertOne(const formats::bson::Document& document);

 private:
  std::shared_ptr<impl::CollectionImpl> impl_;
};

}  // namespace storages::mongo_ng
