#pragma once

#include <vector>

#include <boost/optional.hpp>

#include <formats/bson/bson_builder.hpp>
#include <storages/mongo_ng/operations.hpp>

#include <formats/bson/wrappers.hpp>
#include <storages/mongo_ng/wrappers.hpp>

namespace storages::mongo_ng::operations {

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
  explicit Impl(formats::bson::Document document_)
      : document(std::move(document_)), should_throw(true) {}

  formats::bson::Document document;
  boost::optional<formats::bson::impl::BsonBuilder> options;
  bool should_throw;
};

class InsertMany::Impl {
 public:
  Impl() : should_throw(true) {}

  explicit Impl(std::vector<formats::bson::Document> documents_)
      : documents(std::move(documents_)), should_throw(true) {}

  std::vector<formats::bson::Document> documents;
  boost::optional<formats::bson::impl::BsonBuilder> options;
  bool should_throw;
};

class ReplaceOne::Impl {
 public:
  Impl(formats::bson::Document selector_, formats::bson::Document replacement_)
      : selector(std::move(selector_)),
        replacement(std::move(replacement_)),
        should_throw(true) {}

  formats::bson::Document selector;
  formats::bson::Document replacement;
  boost::optional<formats::bson::impl::BsonBuilder> options;
  bool should_throw;
};

class Update::Impl {
 public:
  Impl(Mode mode_, formats::bson::Document selector_,
       formats::bson::Document update_)
      : mode(mode_),
        should_throw(true),
        selector(std::move(selector_)),
        update(std::move(update_)) {}

  Mode mode;
  bool should_throw;  // moved here for size optimization
  formats::bson::Document selector;
  formats::bson::Document update;
  boost::optional<formats::bson::impl::BsonBuilder> options;
};

class Delete::Impl {
 public:
  Impl(Mode mode_, formats::bson::Document selector_)
      : mode(mode_), should_throw(true), selector(std::move(selector_)) {}

  Mode mode;
  bool should_throw;  // moved here for size optimization
  formats::bson::Document selector;
  boost::optional<formats::bson::impl::BsonBuilder> options;
};

class FindAndModify::Impl {
 public:
  explicit Impl(formats::bson::Document query_) : query(std::move(query_)) {}

  formats::bson::Document query;
  impl::FindAndModifyOptsPtr options;
};

class FindAndRemove::Impl {
 public:
  explicit Impl(formats::bson::Document query_) : query(std::move(query_)) {}

  formats::bson::Document query;
  impl::FindAndModifyOptsPtr options;
};

}  // namespace storages::mongo_ng::operations
