#pragma once

#include <optional>

#include <userver/formats/bson/bson_builder.hpp>
#include <userver/formats/bson/document.hpp>
#include <userver/storages/mongo/bulk_ops.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::bulk_ops {

class InsertOne::Impl {
public:
    explicit Impl(formats::bson::Document&& document)
        : document(std::move(document))
    {}

    formats::bson::Document document;
};

class ReplaceOne::Impl {
public:
    Impl(formats::bson::Document&& selector, formats::bson::Document&& replacement)
        : selector(std::move(selector)),
          replacement(std::move(replacement))
    {}

    formats::bson::Document selector;
    formats::bson::Document replacement;
    std::optional<formats::bson::impl::BsonBuilder> options;
};

class Update::Impl {
public:
    Impl(Mode mode, formats::bson::Document&& selector, formats::bson::Document&& update)
        : mode(mode),
          selector(std::move(selector)),
          update(std::move(update))
    {}

    Mode mode;
    formats::bson::Document selector;
    formats::bson::Document update;
    std::optional<formats::bson::impl::BsonBuilder> options;
};

class Delete::Impl {
public:
    Impl(Mode mode, formats::bson::Document&& selector)
        : mode(mode),
          selector(std::move(selector))
    {}

    Mode mode;
    formats::bson::Document selector;
    std::optional<formats::bson::impl::BsonBuilder> options;
};

}  // namespace storages::mongo::bulk_ops

USERVER_NAMESPACE_END
