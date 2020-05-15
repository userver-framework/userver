#pragma once

#include <optional>

#include <bson/bson.h>

#include <formats/bson/bson_builder.hpp>
#include <storages/mongo/options.hpp>
#include <storages/mongo/wrappers.hpp>

namespace storages::mongo::impl {

formats::bson::impl::BsonBuilder& EnsureBuilder(
    std::optional<formats::bson::impl::BsonBuilder>&);

const bson_t* GetNative(const std::optional<formats::bson::impl::BsonBuilder>&);

WriteConcernPtr MakeWriteConcern(options::WriteConcern::Level);
WriteConcernPtr MakeWriteConcern(const options::WriteConcern&);

void AppendUpsert(formats::bson::impl::BsonBuilder&);

}  // namespace storages::mongo::impl
