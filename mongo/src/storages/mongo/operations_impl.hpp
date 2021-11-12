#pragma once

#include <optional>
#include <string>
#include <vector>

#include <userver/formats/bson/bson_builder.hpp>
#include <userver/formats/bson/document.hpp>
#include <userver/formats/bson/exception.hpp>
#include <userver/formats/bson/value.hpp>
#include <userver/storages/mongo/bulk.hpp>
#include <userver/storages/mongo/operations.hpp>

#include <storages/mongo/cdriver/wrappers.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::operations {

extern const std::string kDefaultReadPrefDesc;
extern const std::string kDefaultWriteConcernDesc;

class Count::Impl {
 public:
  explicit Impl(formats::bson::Document filter_) : filter(std::move(filter_)) {}

  formats::bson::Document filter;
  std::string read_prefs_desc{kDefaultReadPrefDesc};
  impl::cdriver::ReadPrefsPtr read_prefs;
  std::optional<formats::bson::impl::BsonBuilder> options;
  bool use_new_count{true};
};

class CountApprox::Impl {
 public:
  std::string read_prefs_desc{kDefaultReadPrefDesc};
  impl::cdriver::ReadPrefsPtr read_prefs;
  std::optional<formats::bson::impl::BsonBuilder> options;
};

class Find::Impl {
 public:
  explicit Impl(formats::bson::Document filter_) : filter(std::move(filter_)) {}

  formats::bson::Document filter;
  std::string read_prefs_desc{kDefaultReadPrefDesc};
  impl::cdriver::ReadPrefsPtr read_prefs;
  std::optional<formats::bson::impl::BsonBuilder> options;
  bool has_comment_option{false};
  bool has_max_server_time_option{false};
};

class InsertOne::Impl {
 public:
  explicit Impl(formats::bson::Document&& document_)
      : document(std::move(document_)) {}

  formats::bson::Document document;
  std::string write_concern_desc{kDefaultWriteConcernDesc};
  std::optional<formats::bson::impl::BsonBuilder> options;
  bool should_throw{true};
};

class InsertMany::Impl {
 public:
  Impl() = default;

  explicit Impl(std::vector<formats::bson::Document>&& documents_)
      : documents(std::move(documents_)) {}

  std::vector<formats::bson::Document> documents;
  std::string write_concern_desc{kDefaultWriteConcernDesc};
  std::optional<formats::bson::impl::BsonBuilder> options;
  bool should_throw{true};
};

class ReplaceOne::Impl {
 public:
  Impl(formats::bson::Document&& selector_,
       formats::bson::Document&& replacement_)
      : selector(std::move(selector_)), replacement(std::move(replacement_)) {}

  formats::bson::Document selector;
  formats::bson::Document replacement;
  std::string write_concern_desc{kDefaultWriteConcernDesc};
  std::optional<formats::bson::impl::BsonBuilder> options;
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
  bool should_retry_dupkey{false};
  formats::bson::Document selector;
  formats::bson::Document update;
  std::string write_concern_desc{kDefaultWriteConcernDesc};
  std::optional<formats::bson::impl::BsonBuilder> options;
};

class Delete::Impl {
 public:
  Impl(Mode mode_, formats::bson::Document&& selector_)
      : mode(mode_), selector(std::move(selector_)) {}

  Mode mode;
  bool should_throw{true};  // moved here for size optimization
  formats::bson::Document selector;
  std::string write_concern_desc{kDefaultWriteConcernDesc};
  std::optional<formats::bson::impl::BsonBuilder> options;
};

class FindAndModify::Impl {
 public:
  explicit Impl(formats::bson::Document&& query_) : query(std::move(query_)) {}

  formats::bson::Document query;
  std::string write_concern_desc{kDefaultWriteConcernDesc};
  impl::cdriver::FindAndModifyOptsPtr options;
  bool should_retry_dupkey{false};
  bool has_max_server_time_option{false};
};

class FindAndRemove::Impl {
 public:
  explicit Impl(formats::bson::Document&& query_) : query(std::move(query_)) {}

  formats::bson::Document query;
  std::string write_concern_desc{kDefaultWriteConcernDesc};
  impl::cdriver::FindAndModifyOptsPtr options;
  bool has_max_server_time_option{false};
};

class Bulk::Impl {
 public:
  explicit Impl(Mode mode_) : mode(mode_) {}

  impl::cdriver::BulkOperationPtr bulk;
  std::string write_concern_desc{kDefaultWriteConcernDesc};
  Mode mode;
  bool should_throw{true};
};

class Aggregate::Impl {
 public:
  explicit Impl(formats::bson::Value pipeline_)
      : pipeline(std::move(pipeline_)) {}

  formats::bson::Value pipeline;
  std::string read_prefs_desc{kDefaultReadPrefDesc};
  impl::cdriver::ReadPrefsPtr read_prefs;
  std::string write_concern_desc{kDefaultWriteConcernDesc};
  std::optional<formats::bson::impl::BsonBuilder> options;
  bool has_comment_option{false};
  bool has_max_server_time_option{false};
};

void AppendComment(formats::bson::impl::BsonBuilder& builder,
                   bool& has_comment_option, const options::Comment& comment);

void AppendMaxServerTime(formats::bson::impl::BsonBuilder& builder,
                         bool& has_max_server_time_option,
                         const options::MaxServerTime& max_server_time);

}  // namespace storages::mongo::operations

USERVER_NAMESPACE_END
