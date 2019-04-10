#pragma once

#include <vector>

#include <boost/optional.hpp>

#include <formats/bson/bson_builder.hpp>
#include <formats/bson/document.hpp>
#include <storages/mongo/bulk.hpp>
#include <storages/mongo/operations.hpp>

#include <storages/mongo/wrappers.hpp>

namespace storages::mongo::operations {

class Count::Impl {
 public:
  explicit Impl(formats::bson::Document filter_) : filter(std::move(filter_)) {}

  formats::bson::Document filter;
  impl::ReadPrefsPtr read_prefs;
  boost::optional<formats::bson::impl::BsonBuilder> options;
};

class CountApprox::Impl {
 public:
  impl::ReadPrefsPtr read_prefs;
  boost::optional<formats::bson::impl::BsonBuilder> options;
};

class Find::Impl {
 public:
  explicit Impl(formats::bson::Document filter_) : filter(std::move(filter_)) {}

  formats::bson::Document filter;
  impl::ReadPrefsPtr read_prefs;
  boost::optional<formats::bson::impl::BsonBuilder> options;
};

class InsertOne::Impl {
 public:
  explicit Impl(formats::bson::Document&& document_)
      : document(std::move(document_)) {}

  formats::bson::Document document;
  boost::optional<formats::bson::impl::BsonBuilder> options;
  bool should_throw{true};
};

class InsertMany::Impl {
 public:
  Impl() = default;

  explicit Impl(std::vector<formats::bson::Document>&& documents_)
      : documents(std::move(documents_)) {}

  std::vector<formats::bson::Document> documents;
  boost::optional<formats::bson::impl::BsonBuilder> options;
  bool should_throw{true};
};

class ReplaceOne::Impl {
 public:
  Impl(formats::bson::Document&& selector_,
       formats::bson::Document&& replacement_)
      : selector(std::move(selector_)), replacement(std::move(replacement_)) {}

  formats::bson::Document selector;
  formats::bson::Document replacement;
  boost::optional<formats::bson::impl::BsonBuilder> options;
  bool should_throw{true};
};

class Update::Impl {
 public:
  Impl(Mode mode_, formats::bson::Document&& selector_,
       formats::bson::Document update_)
      : mode(mode_),
        selector(std::move(selector_)),
        update(std::move(update_)) {}

  Mode mode;
  bool should_throw{true};  // moved here for size optimization
  formats::bson::Document selector;
  formats::bson::Document update;
  boost::optional<formats::bson::impl::BsonBuilder> options;
};

class Delete::Impl {
 public:
  Impl(Mode mode_, formats::bson::Document&& selector_)
      : mode(mode_), selector(std::move(selector_)) {}

  Mode mode;
  bool should_throw{true};  // moved here for size optimization
  formats::bson::Document selector;
  boost::optional<formats::bson::impl::BsonBuilder> options;
};

class FindAndModify::Impl {
 public:
  explicit Impl(formats::bson::Document&& query_) : query(std::move(query_)) {}

  formats::bson::Document query;
  impl::FindAndModifyOptsPtr options;
};

class FindAndRemove::Impl {
 public:
  explicit Impl(formats::bson::Document&& query_) : query(std::move(query_)) {}

  formats::bson::Document query;
  impl::FindAndModifyOptsPtr options;
};

class Bulk::Impl {
 public:
  explicit Impl(Mode mode_) : mode(mode_) {}

  impl::BulkOperationPtr bulk;
  Mode mode;
  bool should_throw{true};
};

}  // namespace storages::mongo::operations
