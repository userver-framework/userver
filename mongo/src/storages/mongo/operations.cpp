#include <userver/storages/mongo/operations.hpp>

#include <mongoc/mongoc.h>

#include <userver/formats/bson/bson_builder.hpp>
#include <userver/formats/bson/value_builder.hpp>
#include <userver/storages/mongo/exception.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/graphite.hpp>

#include <storages/mongo/cdriver/wrappers.hpp>
#include <storages/mongo/operations_common.hpp>
#include <storages/mongo/operations_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::operations {
namespace {

mongoc_read_mode_t ToCDriverReadMode(options::ReadPreference::Mode mode) {
  using Mode = options::ReadPreference::Mode;

  switch (mode) {
    case Mode::kPrimary:
      return MONGOC_READ_PRIMARY;
    case Mode::kSecondary:
      return MONGOC_READ_SECONDARY;
    case Mode::kPrimaryPreferred:
      return MONGOC_READ_PRIMARY_PREFERRED;
    case Mode::kSecondaryPreferred:
      return MONGOC_READ_SECONDARY_PREFERRED;
    case Mode::kNearest:
      return MONGOC_READ_NEAREST;
  }

  UINVARIANT(false, "Unexpected ReadPreference::Mode");
}

const char* ToCDriverReadConcernLevel(options::ReadConcern level) {
  using Level = options::ReadConcern;

  switch (level) {
    case Level::kLocal:
      return MONGOC_READ_CONCERN_LEVEL_LOCAL;
    case Level::kMajority:
      return MONGOC_READ_CONCERN_LEVEL_MAJORITY;
    case Level::kLinearizable:
      return MONGOC_READ_CONCERN_LEVEL_LINEARIZABLE;
    case Level::kAvailable:
      return MONGOC_READ_CONCERN_LEVEL_AVAILABLE;
  }

  UINVARIANT(false, "Unexpected ReadConcern");
}

impl::cdriver::ReadPrefsPtr MakeCDriverReadPrefs(
    options::ReadPreference::Mode mode) {
  return impl::cdriver::ReadPrefsPtr(ToCDriverReadMode(mode));
}

impl::cdriver::ReadPrefsPtr MakeCDriverReadPrefs(
    const options::ReadPreference& option) {
  auto read_prefs = MakeCDriverReadPrefs(option.GetMode());

  const std::optional<std::chrono::seconds> max_staleness =
      option.GetMaxStaleness();
  if (max_staleness) {
    mongoc_read_prefs_set_max_staleness_seconds(read_prefs.Get(),
                                                max_staleness->count());
  }

  for (const auto& tag : option.GetTags()) {
    const bson_t* native_tag_bson_ptr = tag.GetBson().get();
    mongoc_read_prefs_add_tag(read_prefs.Get(), native_tag_bson_ptr);
  }
  if (!mongoc_read_prefs_is_valid(read_prefs.Get())) {
    throw InvalidQueryArgumentException(
        "Provided read preference is not valid");
  }
  return read_prefs;
}

void AppendReadConcern(formats::bson::impl::BsonBuilder& builder,
                       options::ReadConcern level) {
  const impl::cdriver::ReadConcernPtr read_concern(mongoc_read_concern_new());
  const char* native_level = ToCDriverReadConcernLevel(level);
  if (!mongoc_read_concern_set_level(read_concern.get(), native_level)) {
    throw MongoException("Cannot set read concern '") << native_level << '\'';
  }

  if (!mongoc_read_concern_append(read_concern.get(), builder.Get())) {
    throw MongoException("Cannot append read concern option");
  }
}

void AppendWriteConcern(formats::bson::impl::BsonBuilder& builder,
                        options::WriteConcern::Level level) {
  if (!mongoc_write_concern_append(impl::MakeCDriverWriteConcern(level).get(),
                                   builder.Get())) {
    throw MongoException("Cannot append write concern option");
  }
}

void AppendWriteConcern(formats::bson::impl::BsonBuilder& builder,
                        const options::WriteConcern& wc_option) {
  if (!mongoc_write_concern_append(
          impl::MakeCDriverWriteConcern(wc_option).get(), builder.Get())) {
    throw MongoException("Cannot append write concern option");
  }
}

void AppendUint64Option(formats::bson::impl::BsonBuilder& builder,
                        const std::string& name, std::uint64_t value) {
  if (value >
      static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
    throw InvalidQueryArgumentException("Value ")
        << value << " of '" << name << "' is too high";
  }
  builder.Append(name, value);
}

void AppendSkip(formats::bson::impl::BsonBuilder& builder, options::Skip skip) {
  if (!skip.Value()) return;

  static const std::string kOptionName = "skip";
  AppendUint64Option(builder, kOptionName, skip.Value());
}

void AppendLimit(formats::bson::impl::BsonBuilder& builder,
                 options::Limit limit) {
  if (!limit.Value()) return;

  static const std::string kOptionName = "limit";
  AppendUint64Option(builder, kOptionName, limit.Value());
}

void AppendHint(formats::bson::impl::BsonBuilder& builder,
                const options::Hint& hint) {
  static const std::string kOptionName = "hint";
  builder.Append(kOptionName, hint.Value());
}

void EnableFlag(const impl::cdriver::FindAndModifyOptsPtr& fam_options,
                mongoc_find_and_modify_flags_t new_flag) {
  UASSERT(!!fam_options);
  const auto old_flags =
      mongoc_find_and_modify_opts_get_flags(fam_options.get());
  if (!mongoc_find_and_modify_opts_set_flags(
          fam_options.get(),
          static_cast<mongoc_find_and_modify_flags_t>(old_flags | new_flag))) {
    throw MongoException("Cannot set FAM flag ") << new_flag;
  }
}

}  // namespace

Count::Count(formats::bson::Document filter) : impl_(std::move(filter)) {}
Count::~Count() = default;

Count::Count(const Count& other) = default;
Count::Count(Count&&) noexcept = default;
Count& Count::operator=(const Count& rhs) = default;
Count& Count::operator=(Count&&) noexcept = default;

void Count::SetOption(const options::ReadPreference& read_prefs) {
  impl_->read_prefs = MakeCDriverReadPrefs(read_prefs);
}

void Count::SetOption(options::ReadPreference::Mode mode) {
  impl_->read_prefs = MakeCDriverReadPrefs(mode);
}

void Count::SetOption(options::ReadConcern level) {
  AppendReadConcern(impl::EnsureBuilder(impl_->options), level);
}

void Count::SetOption(options::Skip skip) {
  AppendSkip(impl::EnsureBuilder(impl_->options), skip);
}

void Count::SetOption(options::Limit limit) {
  AppendLimit(impl::EnsureBuilder(impl_->options), limit);
}

void Count::SetOption(options::ForceCountImpl count_impl) {
  impl_->use_new_count = (count_impl == options::ForceCountImpl::kAggregate);
}

void Count::SetOption(const options::MaxServerTime& max_server_time) {
  AppendMaxServerTime(impl_->max_server_time, max_server_time);
}

CountApprox::CountApprox() = default;
CountApprox::~CountApprox() = default;

CountApprox::CountApprox(const CountApprox& other) = default;
CountApprox::CountApprox(CountApprox&&) noexcept = default;
CountApprox& CountApprox::operator=(const CountApprox& rhs) = default;
CountApprox& CountApprox::operator=(CountApprox&&) noexcept = default;

void CountApprox::SetOption(const options::ReadPreference& read_prefs) {
  impl_->read_prefs = MakeCDriverReadPrefs(read_prefs);
}

void CountApprox::SetOption(options::ReadPreference::Mode mode) {
  impl_->read_prefs = MakeCDriverReadPrefs(mode);
}

void CountApprox::SetOption(options::ReadConcern level) {
  AppendReadConcern(impl::EnsureBuilder(impl_->options), level);
}

void CountApprox::SetOption(options::Skip skip) {
  AppendSkip(impl::EnsureBuilder(impl_->options), skip);
}

void CountApprox::SetOption(options::Limit limit) {
  AppendLimit(impl::EnsureBuilder(impl_->options), limit);
}

void CountApprox::SetOption(const options::MaxServerTime& max_server_time) {
  AppendMaxServerTime(impl_->max_server_time, max_server_time);
}

Find::Find(formats::bson::Document filter) : impl_(std::move(filter)) {}
Find::~Find() = default;

Find::Find(const Find& other) = default;
Find::Find(Find&&) noexcept = default;
Find& Find::operator=(const Find& rhs) = default;
Find& Find::operator=(Find&&) noexcept = default;

void Find::SetOption(const options::ReadPreference& read_prefs) {
  impl_->read_prefs = MakeCDriverReadPrefs(read_prefs);
}

void Find::SetOption(options::ReadPreference::Mode mode) {
  impl_->read_prefs = MakeCDriverReadPrefs(mode);
}

void Find::SetOption(options::ReadConcern level) {
  AppendReadConcern(impl::EnsureBuilder(impl_->options), level);
}

void Find::SetOption(options::Skip skip) {
  AppendSkip(impl::EnsureBuilder(impl_->options), skip);
}

void Find::SetOption(options::Limit limit) {
  AppendLimit(impl::EnsureBuilder(impl_->options), limit);
}

void Find::SetOption(options::Projection projection) {
  const bson_t* projection_bson = projection.GetProjectionBson();
  if (bson_empty0(projection_bson)) return;

  static const std::string kOptionName = "projection";
  impl::EnsureBuilder(impl_->options).Append(kOptionName, projection_bson);
}

void Find::SetOption(const options::Sort& sort) {
  const bson_t* sort_bson = sort.GetSortBson();
  if (bson_empty0(sort_bson)) return;

  static const std::string kOptionName = "sort";
  impl::EnsureBuilder(impl_->options).Append(kOptionName, sort_bson);
}

void Find::SetOption(const options::Hint& hint) {
  AppendHint(impl::EnsureBuilder(impl_->options), hint);
}

void Find::SetOption(options::AllowPartialResults) {
  static const std::string kOptionName = "allowPartialResults";
  impl::EnsureBuilder(impl_->options).Append(kOptionName, true);
}

void Find::SetOption(options::Tailable) {
  static const std::string kTailable = "tailable";
  static const std::string kAwaitData = "awaitData";
  static const std::string kNoCursorTimeout = "noCursorTimeout";

  for (const auto& option : {kTailable, kAwaitData, kNoCursorTimeout}) {
    impl::EnsureBuilder(impl_->options).Append(option, true);
  }
}

void Find::SetOption(const options::Comment& comment) {
  AppendComment(impl::EnsureBuilder(impl_->options), impl_->has_comment_option,
                comment);
}

void Find::SetOption(const options::MaxServerTime& max_server_time) {
  AppendMaxServerTime(impl_->max_server_time, max_server_time);
}

InsertOne::InsertOne(formats::bson::Document document)
    : impl_(std::move(document)) {}

InsertOne::~InsertOne() = default;

InsertOne::InsertOne(const InsertOne&) = default;
InsertOne::InsertOne(InsertOne&&) noexcept = default;
InsertOne& InsertOne::operator=(const InsertOne&) = default;
InsertOne& InsertOne::operator=(InsertOne&&) noexcept = default;

void InsertOne::SetOption(options::WriteConcern::Level level) {
  AppendWriteConcern(impl::EnsureBuilder(impl_->options), level);
}

void InsertOne::SetOption(const options::WriteConcern& write_concern) {
  AppendWriteConcern(impl::EnsureBuilder(impl_->options), write_concern);
}

void InsertOne::SetOption(options::SuppressServerExceptions) {
  impl_->should_throw = false;
}

InsertMany::InsertMany() = default;

InsertMany::InsertMany(std::vector<formats::bson::Document> documents_)
    : impl_(std::move(documents_)) {}

InsertMany::~InsertMany() = default;

InsertMany::InsertMany(const InsertMany&) = default;
InsertMany::InsertMany(InsertMany&&) noexcept = default;
InsertMany& InsertMany::operator=(const InsertMany&) = default;
InsertMany& InsertMany::operator=(InsertMany&&) noexcept = default;

void InsertMany::Append(formats::bson::Document document) {
  impl_->documents.push_back(std::move(document));
}

void InsertMany::SetOption(options::Unordered) {
  static const std::string kOptionName = "ordered";
  impl::EnsureBuilder(impl_->options).Append(kOptionName, false);
}

void InsertMany::SetOption(options::WriteConcern::Level level) {
  AppendWriteConcern(impl::EnsureBuilder(impl_->options), level);
}

void InsertMany::SetOption(const options::WriteConcern& write_concern) {
  AppendWriteConcern(impl::EnsureBuilder(impl_->options), write_concern);
}

void InsertMany::SetOption(options::SuppressServerExceptions) {
  impl_->should_throw = false;
}

ReplaceOne::ReplaceOne(formats::bson::Document selector,
                       formats::bson::Document replacement)
    : impl_(std::move(selector), std::move(replacement)) {}

ReplaceOne::~ReplaceOne() = default;

ReplaceOne::ReplaceOne(const ReplaceOne&) = default;
ReplaceOne::ReplaceOne(ReplaceOne&&) noexcept = default;
ReplaceOne& ReplaceOne::operator=(const ReplaceOne&) = default;
ReplaceOne& ReplaceOne::operator=(ReplaceOne&&) noexcept = default;

void ReplaceOne::SetOption(options::Upsert) {
  impl::AppendUpsert(impl::EnsureBuilder(impl_->options));
}

void ReplaceOne::SetOption(options::WriteConcern::Level level) {
  AppendWriteConcern(impl::EnsureBuilder(impl_->options), level);
}

void ReplaceOne::SetOption(const options::WriteConcern& write_concern) {
  AppendWriteConcern(impl::EnsureBuilder(impl_->options), write_concern);
}

void ReplaceOne::SetOption(options::SuppressServerExceptions) {
  impl_->should_throw = false;
}

Update::Update(Mode mode, formats::bson::Document selector,
               formats::bson::Document update)
    : impl_(mode, std::move(selector), std::move(update)) {}

Update::~Update() = default;

Update::Update(const Update&) = default;
Update::Update(Update&&) noexcept = default;
Update& Update::operator=(const Update&) = default;
Update& Update::operator=(Update&&) noexcept = default;

void Update::SetOption(options::Upsert) {
  impl::AppendUpsert(impl::EnsureBuilder(impl_->options));
}

void Update::SetOption(options::RetryDuplicateKey) {
  if (impl_->mode != Mode::kSingle) {
    throw InvalidQueryArgumentException(
        "Duplicate key errors cannot be retried for multi-document operations");
  }
  impl_->should_retry_dupkey = true;
}

void Update::SetOption(options::WriteConcern::Level level) {
  AppendWriteConcern(impl::EnsureBuilder(impl_->options), level);
}

void Update::SetOption(const options::WriteConcern& write_concern) {
  AppendWriteConcern(impl::EnsureBuilder(impl_->options), write_concern);
}

void Update::SetOption(options::SuppressServerExceptions) {
  impl_->should_throw = false;
}

Delete::Delete(Mode mode, formats::bson::Document selector)
    : impl_(mode, std::move(selector)) {}

Delete::~Delete() = default;

Delete::Delete(const Delete&) = default;
Delete::Delete(Delete&&) noexcept = default;
Delete& Delete::operator=(const Delete&) = default;
Delete& Delete::operator=(Delete&&) noexcept = default;

void Delete::SetOption(options::WriteConcern::Level level) {
  AppendWriteConcern(impl::EnsureBuilder(impl_->options), level);
}

void Delete::SetOption(const options::WriteConcern& write_concern) {
  AppendWriteConcern(impl::EnsureBuilder(impl_->options), write_concern);
}

void Delete::SetOption(options::SuppressServerExceptions) {
  impl_->should_throw = false;
}

FindAndModify::FindAndModify(formats::bson::Document query,
                             const formats::bson::Document& update)
    : impl_(std::move(query)) {
  impl_->options.reset(mongoc_find_and_modify_opts_new());
  const bson_t* native_update_bson_ptr = update.GetBson().get();
  if (!mongoc_find_and_modify_opts_set_update(impl_->options.get(),
                                              native_update_bson_ptr)) {
    throw MongoException("Cannot set update document");
  }
}

FindAndModify::~FindAndModify() = default;

FindAndModify::FindAndModify(FindAndModify&&) noexcept = default;
FindAndModify& FindAndModify::operator=(FindAndModify&&) noexcept = default;

void FindAndModify::SetOption(options::ReturnNew) {
  EnableFlag(impl_->options, MONGOC_FIND_AND_MODIFY_RETURN_NEW);
}

void FindAndModify::SetOption(options::Upsert) {
  EnableFlag(impl_->options, MONGOC_FIND_AND_MODIFY_UPSERT);
}

void FindAndModify::SetOption(options::RetryDuplicateKey) {
  impl_->should_retry_dupkey = true;
}

void FindAndModify::SetOption(const options::Sort& sort) {
  if (!mongoc_find_and_modify_opts_set_sort(impl_->options.get(),
                                            sort.GetSortBson())) {
    throw MongoException("Cannot set sort");
  }
}

void FindAndModify::SetOption(options::Projection projection) {
  if (!mongoc_find_and_modify_opts_set_fields(impl_->options.get(),
                                              projection.GetProjectionBson())) {
    throw MongoException("Cannot set projection");
  }
}

void FindAndModify::SetOption(options::WriteConcern::Level level) {
  formats::bson::impl::BsonBuilder wc_builder;
  AppendWriteConcern(wc_builder, level);
  const auto wc_bson = wc_builder.Extract();
  const bson_t* native_wc_bson_ptr = wc_bson.get();
  if (!mongoc_find_and_modify_opts_append(impl_->options.get(),
                                          native_wc_bson_ptr)) {
    throw MongoException("Cannot set write concern");
  }
}

void FindAndModify::SetOption(const options::WriteConcern& write_concern) {
  formats::bson::impl::BsonBuilder wc_builder;
  AppendWriteConcern(wc_builder, write_concern);
  const auto wc_bson = wc_builder.Extract();
  const bson_t* native_wc_bson_ptr = wc_bson.get();
  if (!mongoc_find_and_modify_opts_append(impl_->options.get(),
                                          native_wc_bson_ptr)) {
    throw MongoException("Cannot set write concern");
  }
}

void FindAndModify::SetOption(const options::MaxServerTime& max_server_time) {
  AppendMaxServerTime(impl_->max_server_time, max_server_time);
}

FindAndRemove::FindAndRemove(formats::bson::Document query)
    : impl_(std::move(query)) {
  impl_->options.reset(mongoc_find_and_modify_opts_new());
  EnableFlag(impl_->options, MONGOC_FIND_AND_MODIFY_REMOVE);
}

FindAndRemove::~FindAndRemove() = default;

FindAndRemove::FindAndRemove(FindAndRemove&&) noexcept = default;
FindAndRemove& FindAndRemove::operator=(FindAndRemove&&) noexcept = default;

void FindAndRemove::SetOption(const options::Sort& sort) {
  if (!mongoc_find_and_modify_opts_set_sort(impl_->options.get(),
                                            sort.GetSortBson())) {
    throw MongoException("Cannot set sort");
  }
}

void FindAndRemove::SetOption(options::Projection projection) {
  if (!mongoc_find_and_modify_opts_set_fields(impl_->options.get(),
                                              projection.GetProjectionBson())) {
    throw MongoException("Cannot set projection");
  }
}

void FindAndRemove::SetOption(options::WriteConcern::Level level) {
  formats::bson::impl::BsonBuilder wc_builder;
  AppendWriteConcern(wc_builder, level);
  const auto wc_bson = wc_builder.Extract();
  const bson_t* native_wc_bson_ptr = wc_bson.get();
  if (!mongoc_find_and_modify_opts_append(impl_->options.get(),
                                          native_wc_bson_ptr)) {
    throw MongoException("Cannot set write concern");
  }
}

void FindAndRemove::SetOption(const options::WriteConcern& write_concern) {
  formats::bson::impl::BsonBuilder wc_builder;
  AppendWriteConcern(wc_builder, write_concern);
  const auto wc_bson = wc_builder.Extract();
  const bson_t* native_wc_bson_ptr = wc_bson.get();
  if (!mongoc_find_and_modify_opts_append(impl_->options.get(),
                                          native_wc_bson_ptr)) {
    throw MongoException("Cannot set write concern");
  }
}

void FindAndRemove::SetOption(const options::MaxServerTime& max_server_time) {
  AppendMaxServerTime(impl_->max_server_time, max_server_time);
}

Aggregate::Aggregate(formats::bson::Value pipeline)
    : impl_(std::move(pipeline)) {
  if (!impl_->pipeline.IsArray()) {
    throw InvalidQueryArgumentException("Aggregation pipeline is not an array");
  }
  if (impl_->pipeline.IsEmpty()) {
    throw InvalidQueryArgumentException(
        "Provided aggregation pipeline is empty");
  }
}

Aggregate::~Aggregate() = default;

Aggregate::Aggregate(const Aggregate& other) = default;
Aggregate::Aggregate(Aggregate&&) noexcept = default;
Aggregate& Aggregate::operator=(const Aggregate& rhs) = default;
Aggregate& Aggregate::operator=(Aggregate&&) noexcept = default;

void Aggregate::SetOption(const options::ReadPreference& read_prefs) {
  impl_->read_prefs = MakeCDriverReadPrefs(read_prefs);
}

void Aggregate::SetOption(options::ReadPreference::Mode mode) {
  impl_->read_prefs = MakeCDriverReadPrefs(mode);
}

void Aggregate::SetOption(options::ReadConcern level) {
  AppendReadConcern(impl::EnsureBuilder(impl_->options), level);
}

void Aggregate::SetOption(const options::WriteConcern& write_concern) {
  AppendWriteConcern(impl::EnsureBuilder(impl_->options), write_concern);
}

void Aggregate::SetOption(options::WriteConcern::Level level) {
  AppendWriteConcern(impl::EnsureBuilder(impl_->options), level);
}

void Aggregate::SetOption(const options::Hint& hint) {
  AppendHint(impl::EnsureBuilder(impl_->options), hint);
}

void Aggregate::SetOption(const options::Comment& comment) {
  AppendComment(impl::EnsureBuilder(impl_->options), impl_->has_comment_option,
                comment);
}

void Aggregate::SetOption(const options::MaxServerTime& max_server_time) {
  AppendMaxServerTime(impl_->max_server_time, max_server_time);
}

Drop::Drop() = default;
Drop::~Drop() = default;

Drop::Drop(const Drop&) = default;
Drop::Drop(Drop&&) noexcept = default;
Drop& Drop::operator=(const Drop&) = default;
Drop& Drop::operator=(Drop&&) noexcept = default;

void Drop::SetOption(options::WriteConcern::Level level) {
  AppendWriteConcern(impl::EnsureBuilder(impl_->options), level);
}

void Drop::SetOption(const options::WriteConcern& write_concern) {
  AppendWriteConcern(impl::EnsureBuilder(impl_->options), write_concern);
}

}  // namespace storages::mongo::operations

USERVER_NAMESPACE_END
