#pragma once

#include <boost/optional.hpp>

#include <formats/bson/bson_builder.hpp>
#include <storages/mongo_ng/operations.hpp>

#include <formats/bson/wrappers.hpp>
#include <storages/mongo_ng/wrappers.hpp>

namespace storages::mongo_ng::operations {

class Count::Impl {
 public:
  Impl(formats::bson::Document filter_) : filter(std::move(filter_)) {}

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
  Impl(formats::bson::Document filter_) : filter(std::move(filter_)) {}

  formats::bson::Document filter;
  impl::ReadPrefsPtr read_prefs;
  boost::optional<formats::bson::impl::BsonBuilder> options;
};

}  // namespace storages::mongo_ng::operations
