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
#include <storages/mongo/stats.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::operations {

extern const std::string kDefaultReadPrefDesc;
extern const std::string kDefaultWriteConcernDesc;
inline constexpr std::chrono::milliseconds kNoMaxServerTime{0};

stats::OpType ToStatsOpType(operations::Update::Mode);

stats::OpType ToStatsOpType(operations::Delete::Mode);

class Count::Impl {
 public:
  explicit Impl(formats::bson::Document filter_) : filter(std::move(filter_)) {}

  formats::bson::Document filter;
  stats::OperationKey op_key{stats::OpType::kCount};
  impl::cdriver::ReadPrefsPtr read_prefs;
  std::optional<formats::bson::impl::BsonBuilder> options;
  bool use_new_count{true};
  std::chrono::milliseconds max_server_time{kNoMaxServerTime};
};

class CountApprox::Impl {
 public:
  stats::OperationKey op_key{stats::OpType::kCountApprox};
  impl::cdriver::ReadPrefsPtr read_prefs;
  std::optional<formats::bson::impl::BsonBuilder> options;
  std::chrono::milliseconds max_server_time{kNoMaxServerTime};
};

class Find::Impl {
 public:
  explicit Impl(formats::bson::Document filter_) : filter(std::move(filter_)) {}

  formats::bson::Document filter;
  stats::OperationKey op_key{stats::OpType::kFind};
  impl::cdriver::ReadPrefsPtr read_prefs;
  std::optional<formats::bson::impl::BsonBuilder> options;
  bool has_comment_option{false};
  std::chrono::milliseconds max_server_time{kNoMaxServerTime};
};

class InsertOne::Impl {
 public:
  explicit Impl(formats::bson::Document&& document_)
      : document(std::move(document_)) {}

  formats::bson::Document document;
  stats::OperationKey op_key{stats::OpType::kInsertOne};
  std::optional<formats::bson::impl::BsonBuilder> options;
  bool should_throw{true};
};

class InsertMany::Impl {
 public:
  Impl() = default;

  explicit Impl(std::vector<formats::bson::Document>&& documents_)
      : documents(std::move(documents_)) {}

  std::vector<formats::bson::Document> documents;
  stats::OperationKey op_key{stats::OpType::kInsertMany};
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
  stats::OperationKey op_key{stats::OpType::kReplaceOne};
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
  stats::OperationKey op_key{ToStatsOpType(mode)};
  std::optional<formats::bson::impl::BsonBuilder> options;
};

class Delete::Impl {
 public:
  Impl(Mode mode_, formats::bson::Document&& selector_)
      : mode(mode_), selector(std::move(selector_)) {}

  Mode mode;
  bool should_throw{true};  // moved here for size optimization
  formats::bson::Document selector;
  stats::OperationKey op_key{ToStatsOpType(mode)};
  std::optional<formats::bson::impl::BsonBuilder> options;
};

class FindAndModify::Impl {
 public:
  explicit Impl(formats::bson::Document&& query_) : query(std::move(query_)) {}

  formats::bson::Document query;
  stats::OperationKey op_key{stats::OpType::kFindAndModify};
  impl::cdriver::FindAndModifyOptsPtr options;
  bool should_retry_dupkey{false};
  std::chrono::milliseconds max_server_time{kNoMaxServerTime};
};

class FindAndRemove::Impl {
 public:
  explicit Impl(formats::bson::Document&& query_) : query(std::move(query_)) {}

  formats::bson::Document query;
  stats::OperationKey op_key{stats::OpType::kFindAndRemove};
  impl::cdriver::FindAndModifyOptsPtr options;
  std::chrono::milliseconds max_server_time{kNoMaxServerTime};
};

class Bulk::Impl {
 public:
  explicit Impl(Mode mode_) : mode(mode_) {}

  impl::cdriver::BulkOperationPtr bulk;
  stats::OperationKey op_key{stats::OpType::kBulk};
  Mode mode;
  bool should_throw{true};
};

class Aggregate::Impl {
 public:
  explicit Impl(formats::bson::Value pipeline_)
      : pipeline(std::move(pipeline_)) {}

  formats::bson::Value pipeline;
  impl::cdriver::ReadPrefsPtr read_prefs;
  stats::OperationKey op_key{stats::OpType::kAggregate};
  std::optional<formats::bson::impl::BsonBuilder> options;
  bool has_comment_option{false};
  std::chrono::milliseconds max_server_time{kNoMaxServerTime};
};

class Drop::Impl {
 public:
  Impl() = default;

  std::optional<formats::bson::impl::BsonBuilder> options;
  stats::OperationKey op_key{stats::OpType::kDrop};
};

void AppendComment(formats::bson::impl::BsonBuilder& builder,
                   bool& has_comment_option, const options::Comment& comment);

void AppendMaxServerTime(std::chrono::milliseconds& destination,
                         const options::MaxServerTime& max_server_time);

/// @throws InvalidQueryArgumentException on invalid @a max_server_time
void VerifyMaxServerTime(std::chrono::milliseconds& max_server_time);

}  // namespace storages::mongo::operations

USERVER_NAMESPACE_END
