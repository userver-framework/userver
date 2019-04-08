#pragma once

#include <boost/optional.hpp>

#include <formats/bson/bson_builder.hpp>
#include <formats/bson/document.hpp>
#include <storages/mongo/bulk_ops.hpp>

namespace storages::mongo::bulk_ops {

class InsertOne::Impl {
 public:
  explicit Impl(formats::bson::Document&& document_)
      : document(std::move(document_)) {}

  formats::bson::Document document;
};

class ReplaceOne::Impl {
 public:
  Impl(formats::bson::Document&& selector_,
       formats::bson::Document&& replacement_)
      : selector(std::move(selector_)), replacement(std::move(replacement_)) {}

  formats::bson::Document selector;
  formats::bson::Document replacement;
  boost::optional<formats::bson::impl::BsonBuilder> options;
};

class Update::Impl {
 public:
  Impl(Mode mode_, formats::bson::Document&& selector_,
       formats::bson::Document&& update_)
      : mode(mode_),
        selector(std::move(selector_)),
        update(std::move(update_)) {}

  Mode mode;
  formats::bson::Document selector;
  formats::bson::Document update;
  boost::optional<formats::bson::impl::BsonBuilder> options;
};

class Delete::Impl {
 public:
  Impl(Mode mode_, formats::bson::Document&& selector_)
      : mode(mode_), selector(std::move(selector_)) {}

  Mode mode;
  formats::bson::Document selector;
};

}  // namespace storages::mongo::bulk_ops
