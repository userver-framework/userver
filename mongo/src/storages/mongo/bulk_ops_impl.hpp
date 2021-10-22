#pragma once

#include <optional>

#include <userver/formats/bson/bson_builder.hpp>
#include <userver/formats/bson/document.hpp>
#include <userver/storages/mongo/bulk_ops.hpp>

USERVER_NAMESPACE_BEGIN

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
  std::optional<formats::bson::impl::BsonBuilder> options;
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
  std::optional<formats::bson::impl::BsonBuilder> options;
};

class Delete::Impl {
 public:
  Impl(Mode mode_, formats::bson::Document&& selector_)
      : mode(mode_), selector(std::move(selector_)) {}

  Mode mode;
  formats::bson::Document selector;
};

}  // namespace storages::mongo::bulk_ops

USERVER_NAMESPACE_END
