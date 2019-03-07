#include <storages/mongo_ng/operations.hpp>

#include <chrono>
#include <limits>

#include <bson/bson.h>
#include <mongoc/mongoc.h>

#include <formats/bson/value_builder.hpp>
#include <storages/mongo_ng/exception.hpp>
#include <utils/assert.hpp>

#include <storages/mongo_ng/operations_impl.hpp>

namespace storages::mongo_ng::operations {
namespace {

formats::bson::impl::BsonBuilder& GetBuilder(
    boost::optional<formats::bson::impl::BsonBuilder>& optional_builder) {
  if (!optional_builder) {
    optional_builder.emplace();
  }
  return *optional_builder;
}

mongoc_read_mode_t ToNativeReadMode(options::ReadPreference::Mode mode) {
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
}

const char* ToNativeReadConcernLevel(options::ReadConcern level) {
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
}

impl::ReadPrefsPtr MakeReadPrefs(options::ReadPreference::Mode mode) {
  return impl::ReadPrefsPtr(mongoc_read_prefs_new(ToNativeReadMode(mode)));
}

impl::ReadPrefsPtr MakeReadPrefs(const options::ReadPreference& option) {
  auto read_prefs = MakeReadPrefs(option.GetMode());
  for (const auto& tag : option.GetTags()) {
    mongoc_read_prefs_add_tag(read_prefs.get(), tag.GetBson().get());
  }
  if (!mongoc_read_prefs_is_valid(read_prefs.get())) {
    throw InvalidQueryArgumentException(
        "Provided read preference is not valid");
  }
  return read_prefs;
}

formats::bson::impl::BsonHolder MakeSortBson(const options::Sort& sort) {
  // order is significant here, so we build BSON directly
  formats::bson::impl::BsonBuilder builder;
  for (const auto& [field, direction] : sort.GetOrder()) {
    builder.Append(field,
                   direction == options::Sort::Direction::kAscending ? 1 : -1);
  }
  return builder.Extract();
}

void AppendReadConcern(formats::bson::impl::BsonBuilder& builder,
                       options::ReadConcern level) {
  impl::ReadConcernPtr read_concern(mongoc_read_concern_new());
  const char* native_level = ToNativeReadConcernLevel(level);
  if (!mongoc_read_concern_set_level(read_concern.get(), native_level)) {
    throw MongoException("Cannot set read concern '") << native_level << '\'';
  }

  if (!mongoc_read_concern_append(read_concern.get(), builder.Get())) {
    throw MongoException("Cannot append read concern option");
  }
}

void AppendWriteConcern(formats::bson::impl::BsonBuilder& builder,
                        options::WriteConcern::Level level) {
  impl::WriteConcernPtr write_concern(mongoc_write_concern_new());
  switch (level) {
    case options::WriteConcern::kMajority:
      mongoc_write_concern_set_wmajority(
          write_concern.get(),
          std::chrono::duration_cast<std::chrono::milliseconds>(
              options::WriteConcern::kDefaultMajorityTimeout)
              .count());
      break;
    case options::WriteConcern::kUnacknowledged:
      mongoc_write_concern_set_w(write_concern.get(), 0);
      break;
  }
  if (!mongoc_write_concern_append(write_concern.get(), builder.Get())) {
    throw MongoException("Cannot append write concern option");
  }
}

void AppendWriteConcern(formats::bson::impl::BsonBuilder& builder,
                        const options::WriteConcern& wc_option) {
  if (wc_option.NodesCount() > std::numeric_limits<int32_t>::max()) {
    throw InvalidQueryArgumentException("Value ")
        << wc_option.NodesCount()
        << " of write concern nodes count is too high";
  }
  auto timeout_ms = wc_option.Timeout().count();
  if (timeout_ms > std::numeric_limits<int32_t>::max()) {
    throw InvalidQueryArgumentException("Value ")
        << timeout_ms << "ms of write concern timeout is too high";
  }

  impl::WriteConcernPtr write_concern(mongoc_write_concern_new());

  if (wc_option.IsMajority()) {
    mongoc_write_concern_set_wmajority(write_concern.get(), timeout_ms);
  } else {
    if (!wc_option.Tag().empty()) {
      mongoc_write_concern_set_wtag(write_concern.get(),
                                    wc_option.Tag().c_str());
    } else {
      mongoc_write_concern_set_w(write_concern.get(), wc_option.NodesCount());
    }
    mongoc_write_concern_set_wtimeout(write_concern.get(), timeout_ms);
  }

  if (wc_option.Journal()) {
    mongoc_write_concern_set_journal(write_concern.get(), *wc_option.Journal());
  }

  if (!mongoc_write_concern_append(write_concern.get(), builder.Get())) {
    throw MongoException("Cannot append write concern option");
  }
}

void AppendUint64Option(formats::bson::impl::BsonBuilder& builder,
                        const std::string& name, uint64_t value) {
  if (value > std::numeric_limits<int64_t>::max()) {
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

void AppendUpsert(formats::bson::impl::BsonBuilder& builder) {
  static const std::string kOptionName = "upsert";
  builder.Append(kOptionName, true);
}

void EnableFlag(const impl::FindAndModifyOptsPtr& fam_options,
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

void Count::SetOption(const options::ReadPreference& read_prefs) {
  impl_->read_prefs = MakeReadPrefs(read_prefs);
}

void Count::SetOption(options::ReadPreference::Mode mode) {
  impl_->read_prefs = MakeReadPrefs(mode);
}

void Count::SetOption(options::ReadConcern level) {
  AppendReadConcern(GetBuilder(impl_->options), level);
}

void Count::SetOption(options::Skip skip) {
  AppendSkip(GetBuilder(impl_->options), skip);
}

void Count::SetOption(options::Limit limit) {
  AppendLimit(GetBuilder(impl_->options), limit);
}

CountApprox::CountApprox() = default;
CountApprox::~CountApprox() = default;

void CountApprox::SetOption(const options::ReadPreference& read_prefs) {
  impl_->read_prefs = MakeReadPrefs(read_prefs);
}

void CountApprox::SetOption(options::ReadPreference::Mode mode) {
  impl_->read_prefs = MakeReadPrefs(mode);
}

void CountApprox::SetOption(options::ReadConcern level) {
  AppendReadConcern(GetBuilder(impl_->options), level);
}

void CountApprox::SetOption(options::Skip skip) {
  AppendSkip(GetBuilder(impl_->options), skip);
}

void CountApprox::SetOption(options::Limit limit) {
  AppendLimit(GetBuilder(impl_->options), limit);
}

Find::Find(formats::bson::Document filter) : impl_(std::move(filter)) {}
Find::~Find() = default;

void Find::SetOption(const options::ReadPreference& read_prefs) {
  impl_->read_prefs = MakeReadPrefs(read_prefs);
}

void Find::SetOption(options::ReadPreference::Mode mode) {
  impl_->read_prefs = MakeReadPrefs(mode);
}

void Find::SetOption(options::ReadConcern level) {
  AppendReadConcern(GetBuilder(impl_->options), level);
}

void Find::SetOption(options::Skip skip) {
  AppendSkip(GetBuilder(impl_->options), skip);
}

void Find::SetOption(options::Limit limit) {
  AppendLimit(GetBuilder(impl_->options), limit);
}

void Find::SetOption(options::Projection projection) {
  if (projection.IsEmpty()) return;

  static const std::string kOptionName = "projection";
  GetBuilder(impl_->options)
      .Append(kOptionName, std::move(projection).Extract());
}

void Find::SetOption(const options::Sort& sort) {
  if (sort.GetOrder().empty()) return;

  static const std::string kOptionName = "sort";
  GetBuilder(impl_->options).Append(kOptionName, MakeSortBson(sort));
}

void Find::SetOption(const options::Hint& hint) {
  static const std::string kOptionName = "hint";
  GetBuilder(impl_->options).Append(kOptionName, hint.Value());
}

void Find::SetOption(options::AllowPartialResults) {
  static const std::string kOptionName = "allowPartialResults";
  GetBuilder(impl_->options).Append(kOptionName, true);
}

void Find::SetOption(options::Tailable) {
  static const std::string kTailable = "tailable";
  static const std::string kAwaitData = "awaitData";
  static const std::string kNoCursorTimeout = "noCursorTimeout";

  for (const auto& option : {kTailable, kAwaitData, kNoCursorTimeout}) {
    GetBuilder(impl_->options).Append(option, true);
  }
}

void Find::SetOption(const options::Comment& comment) {
  static const std::string kOptionName = "comment";
  GetBuilder(impl_->options).Append(kOptionName, comment.Value());
}

void Find::SetOption(const options::MaxServerTime& max_server_time) {
  static const std::string kOptionName = "maxTimeMS";
  GetBuilder(impl_->options)
      .Append(kOptionName, max_server_time.Value().count());
}

InsertOne::InsertOne(formats::bson::Document document)
    : impl_(std::move(document)) {}

InsertOne::~InsertOne() = default;

void InsertOne::SetOption(options::WriteConcern::Level level) {
  AppendWriteConcern(GetBuilder(impl_->options), level);
}

void InsertOne::SetOption(const options::WriteConcern& write_concern) {
  AppendWriteConcern(GetBuilder(impl_->options), write_concern);
}

void InsertOne::SetOption(options::SuppressServerExceptions) {
  impl_->should_throw = false;
}

InsertMany::InsertMany() = default;

InsertMany::InsertMany(std::vector<formats::bson::Document> documents_)
    : impl_(std::move(documents_)) {}

InsertMany::~InsertMany() = default;

void InsertMany::Append(formats::bson::Document document) {
  impl_->documents.push_back(std::move(document));
}

void InsertMany::SetOption(options::Unordered) {
  static const std::string kOptionName = "ordered";
  GetBuilder(impl_->options).Append(kOptionName, false);
}

void InsertMany::SetOption(options::WriteConcern::Level level) {
  AppendWriteConcern(GetBuilder(impl_->options), level);
}

void InsertMany::SetOption(const options::WriteConcern& write_concern) {
  AppendWriteConcern(GetBuilder(impl_->options), write_concern);
}

void InsertMany::SetOption(options::SuppressServerExceptions) {
  impl_->should_throw = false;
}

ReplaceOne::ReplaceOne(formats::bson::Document selector,
                       formats::bson::Document replacement)
    : impl_(std::move(selector), std::move(replacement)) {}

ReplaceOne::~ReplaceOne() = default;

void ReplaceOne::SetOption(options::Upsert) {
  AppendUpsert(GetBuilder(impl_->options));
}

void ReplaceOne::SetOption(options::WriteConcern::Level level) {
  AppendWriteConcern(GetBuilder(impl_->options), level);
}

void ReplaceOne::SetOption(const options::WriteConcern& write_concern) {
  AppendWriteConcern(GetBuilder(impl_->options), write_concern);
}

void ReplaceOne::SetOption(options::SuppressServerExceptions) {
  impl_->should_throw = false;
}

Update::Update(Mode mode, formats::bson::Document selector,
               formats::bson::Document update)
    : impl_(mode, std::move(selector), std::move(update)) {}

Update::~Update() = default;

void Update::SetOption(options::Upsert) {
  AppendUpsert(GetBuilder(impl_->options));
}

void Update::SetOption(options::WriteConcern::Level level) {
  AppendWriteConcern(GetBuilder(impl_->options), level);
}

void Update::SetOption(const options::WriteConcern& write_concern) {
  AppendWriteConcern(GetBuilder(impl_->options), write_concern);
}

void Update::SetOption(options::SuppressServerExceptions) {
  impl_->should_throw = false;
}

Delete::Delete(Mode mode, formats::bson::Document selector)
    : impl_(mode, std::move(selector)) {}

Delete::~Delete() = default;

void Delete::SetOption(options::WriteConcern::Level level) {
  AppendWriteConcern(GetBuilder(impl_->options), level);
}

void Delete::SetOption(const options::WriteConcern& write_concern) {
  AppendWriteConcern(GetBuilder(impl_->options), write_concern);
}

void Delete::SetOption(options::SuppressServerExceptions) {
  impl_->should_throw = false;
}

FindAndModify::FindAndModify(formats::bson::Document query,
                             const formats::bson::Document& update)
    : impl_(std::move(query)) {
  impl_->options.reset(mongoc_find_and_modify_opts_new());
  if (!mongoc_find_and_modify_opts_set_update(impl_->options.get(),
                                              update.GetBson().get())) {
    throw MongoException("Cannot set update document");
  }
}

FindAndModify::~FindAndModify() = default;

void FindAndModify::SetOption(options::Upsert) {
  EnableFlag(impl_->options, MONGOC_FIND_AND_MODIFY_UPSERT);
}

void FindAndModify::SetOption(options::ReturnNew) {
  EnableFlag(impl_->options, MONGOC_FIND_AND_MODIFY_RETURN_NEW);
}

void FindAndModify::SetOption(const options::Sort& sort) {
  if (!mongoc_find_and_modify_opts_set_sort(impl_->options.get(),
                                            MakeSortBson(sort).get())) {
    throw MongoException("Cannot set sort");
  }
}

void FindAndModify::SetOption(options::Projection projection) {
  const auto projection_doc = std::move(projection).Extract();
  if (!mongoc_find_and_modify_opts_set_fields(impl_->options.get(),
                                              projection_doc.GetBson().get())) {
    throw MongoException("Cannot set projection");
  }
}

void FindAndModify::SetOption(options::WriteConcern::Level level) {
  formats::bson::impl::BsonBuilder wc_builder;
  AppendWriteConcern(wc_builder, level);
  if (!mongoc_find_and_modify_opts_append(impl_->options.get(),
                                          wc_builder.Extract().get())) {
    throw MongoException("Cannot set write concern");
  }
}

void FindAndModify::SetOption(const options::WriteConcern& write_concern) {
  formats::bson::impl::BsonBuilder wc_builder;
  AppendWriteConcern(wc_builder, write_concern);
  if (!mongoc_find_and_modify_opts_append(impl_->options.get(),
                                          wc_builder.Extract().get())) {
    throw MongoException("Cannot set write concern");
  }
}

void FindAndModify::SetOption(const options::MaxServerTime& max_server_time) {
  auto value_ms = max_server_time.Value().count();
  if (value_ms < 0 || value_ms > std::numeric_limits<uint32_t>::max()) {
    throw InvalidQueryArgumentException("Max server time of ")
        << value_ms << "ms is out of bounds";
  }
  if (!mongoc_find_and_modify_opts_set_max_time_ms(impl_->options.get(),
                                                   value_ms)) {
    throw MongoException("Cannot set max server time");
  }
}

FindAndRemove::FindAndRemove(formats::bson::Document query)
    : impl_(std::move(query)) {
  impl_->options.reset(mongoc_find_and_modify_opts_new());
  EnableFlag(impl_->options, MONGOC_FIND_AND_MODIFY_REMOVE);
}

FindAndRemove::~FindAndRemove() = default;

void FindAndRemove::SetOption(const options::Sort& sort) {
  if (!mongoc_find_and_modify_opts_set_sort(impl_->options.get(),
                                            MakeSortBson(sort).get())) {
    throw MongoException("Cannot set sort");
  }
}

void FindAndRemove::SetOption(options::Projection projection) {
  const auto projection_doc = std::move(projection).Extract();
  if (!mongoc_find_and_modify_opts_set_fields(impl_->options.get(),
                                              projection_doc.GetBson().get())) {
    throw MongoException("Cannot set projection");
  }
}

void FindAndRemove::SetOption(options::WriteConcern::Level level) {
  formats::bson::impl::BsonBuilder wc_builder;
  AppendWriteConcern(wc_builder, level);
  if (!mongoc_find_and_modify_opts_append(impl_->options.get(),
                                          wc_builder.Extract().get())) {
    throw MongoException("Cannot set write concern");
  }
}

void FindAndRemove::SetOption(const options::WriteConcern& write_concern) {
  formats::bson::impl::BsonBuilder wc_builder;
  AppendWriteConcern(wc_builder, write_concern);
  if (!mongoc_find_and_modify_opts_append(impl_->options.get(),
                                          wc_builder.Extract().get())) {
    throw MongoException("Cannot set write concern");
  }
}

void FindAndRemove::SetOption(const options::MaxServerTime& max_server_time) {
  auto value_ms = max_server_time.Value().count();
  if (value_ms < 0 || value_ms > std::numeric_limits<uint32_t>::max()) {
    throw InvalidQueryArgumentException("Max server time of ")
        << value_ms << "ms is out of bounds";
  }
  if (!mongoc_find_and_modify_opts_set_max_time_ms(impl_->options.get(),
                                                   value_ms)) {
    throw MongoException("Cannot set max server time");
  }
}

}  // namespace storages::mongo_ng::operations
