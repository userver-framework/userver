#pragma once

#include <bson/bson.h>

#include <boost/optional.hpp>

#include <formats/bson/bson_builder.hpp>
#include <storages/mongo/options.hpp>
#include <storages/mongo/wrappers.hpp>

namespace storages::mongo::impl {

formats::bson::impl::BsonBuilder& EnsureBuilder(
    boost::optional<formats::bson::impl::BsonBuilder>&);

const bson_t* GetNative(
    const boost::optional<formats::bson::impl::BsonBuilder>&);

WriteConcernPtr MakeWriteConcern(options::WriteConcern::Level);
WriteConcernPtr MakeWriteConcern(const options::WriteConcern&);

void AppendUpsert(formats::bson::impl::BsonBuilder&);

}  // namespace storages::mongo::impl
