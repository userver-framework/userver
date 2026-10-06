#include <storages/mongo/collection_impl.hpp>

#include <userver/storages/mongo/exception.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/text.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl {

CollectionImpl::CollectionImpl(std::string collection_name)
    : collection_name_(std::move(collection_name))
{
    if (!utils::text::IsCString(collection_name_)) {
        throw MongoException("Invalid collection name: '" + collection_name_ + '\'');
    }
}

const std::string& CollectionImpl::GetCollectionName() const { return collection_name_; }

tracing::Span CollectionImpl::MakeSpan(std::string&& name) const {
    tracing::Span span(std::move(name));
    span.AddTag(tracing::kDatabaseType, tracing::kDatabaseMongoType);
    span.AddTag(tracing::kDatabaseCollection, collection_name_);
    return span;
}

}  // namespace storages::mongo::impl

USERVER_NAMESPACE_END
