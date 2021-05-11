#pragma once

#include <optional>

#include <bson/bson.h>

#include <formats/bson/bson_builder.hpp>
#include <storages/mongo/cdriver/wrappers.hpp>
#include <storages/mongo/options.hpp>

namespace storages::mongo::impl {

formats::bson::impl::BsonBuilder& EnsureBuilder(
    std::optional<formats::bson::impl::BsonBuilder>&);

const bson_t* GetNative(const std::optional<formats::bson::impl::BsonBuilder>&);

cdriver::WriteConcernPtr MakeCDriverWriteConcern(options::WriteConcern::Level);
cdriver::WriteConcernPtr MakeCDriverWriteConcern(const options::WriteConcern&);

void AppendUpsert(formats::bson::impl::BsonBuilder&);

}  // namespace storages::mongo::impl
