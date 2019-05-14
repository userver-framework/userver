#include <storages/mongo/operations.hpp>

#include <mongoc/mongoc.h>

#include <formats/bson/bson_builder.hpp>
#include <formats/bson/value_builder.hpp>
#include <storages/mongo/exception.hpp>
#include <utils/assert.hpp>
#include <utils/graphite.hpp>

#include <storages/mongo/operations_common.hpp>
#include <storages/mongo/operations_impl.hpp>
#include <storages/mongo/wrappers.hpp>

namespace storages::mongo::operations {
namespace {

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
  if (!mongoc_write_concern_append(impl::MakeWriteConcern(level).get(),
                                   builder.Get())) {
    throw MongoException("Cannot append write concern option");
  }
}

void AppendWriteConcern(formats::bson::impl::BsonBuilder& builder,
                        const options::WriteConcern& wc_option) {
  if (!mongoc_write_concern_append(impl::MakeWriteConcern(wc_option).get(),
                                   builder.Get())) {
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

std::string MakeReadPrefsDescription(const options::ReadPreference::Mode mode) {
  switch (mode) {
    case options::ReadPreference::Mode::kPrimary:
      return kDefaultReadPrefDesc;
    case options::ReadPreference::Mode::kSecondary:
      return "secondary";
    case options::ReadPreference::Mode::kPrimaryPreferred:
      return "primary-preferred";
    case options::ReadPreference::Mode::kSecondaryPreferred:
      return "secondary-preferred";
    case options::ReadPreference::Mode::kNearest:
      return "nearest";
  }
}

std::string MakeReadPrefsDescription(
    const options::ReadPreference& read_prefs) {
  std::string result = MakeReadPrefsDescription(read_prefs.GetMode());
  if (!read_prefs.GetTags().empty()) result += "-tagged";
  return result;
}

std::string MakeWriteConcernDescription(options::WriteConcern::Level level) {
  switch (level) {
    case options::WriteConcern::Level::kMajority:
      return "majority";
    case options::WriteConcern::Level::kUnacknowledged:
      return "unacknowledged";
  }
}

std::string MakeWriteConcernDescription(
    const options::WriteConcern& write_concern) {
  std::string result;
  if (write_concern.IsMajority()) {
    result =
        MakeWriteConcernDescription(options::WriteConcern::Level::kMajority);
    if (write_concern.Timeout() !=
        options::WriteConcern::kDefaultMajorityTimeout) {
      result += "-timeout";
    }
  } else {
    if (!write_concern.Tag().empty()) {
      result = utils::graphite::EscapeName(write_concern.Tag());
    } else if (!write_concern.NodesCount()) {
      result = MakeWriteConcernDescription(
          options::WriteConcern::Level::kUnacknowledged);
    } else if (write_concern.NodesCount() == 1) {
      result = kDefaultWriteConcernDesc;
    } else {
      result = "w" + std::to_string(write_concern.NodesCount());
    }
    if (write_concern.Timeout().count()) {
      result += "-timeout";
    }
  }
  if (write_concern.Journal()) {
    result += "-j";
    result += std::to_string(*write_concern.Journal());
  }
  return result;
}

}  // namespace

const std::string kDefaultReadPrefDesc = "primary";
const std::string kDefaultWriteConcernDesc = "default";

Count::Count(formats::bson::Document filter) : impl_(std::move(filter)) {}
Count::~Count() = default;

Count::Count(const Count& other) : impl_(other.impl_->filter) {
  impl_->read_prefs_desc = other.impl_->read_prefs_desc;
  impl_->read_prefs.reset(
      mongoc_read_prefs_copy(other.impl_->read_prefs.get()));
  impl_->options = other.impl_->options;

  if (!impl_->filter.GetSize()) {
    impl_->use_new_count = false;
  }
}

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
Count::Count(Count&&) noexcept = default;
// NOLINTNEXTLINE(cppcoreguidelines-c-copy-assignment-signature)
Count& Count::operator=(const Count& rhs) { return *this = Count(rhs); }
// NOLINTNEXTLINE(performance-noexcept-move-constructor)
Count& Count::operator=(Count&&) = default;

void Count::SetOption(const options::ReadPreference& read_prefs) {
  impl_->read_prefs = MakeReadPrefs(read_prefs);
  impl_->read_prefs_desc = MakeReadPrefsDescription(read_prefs);
}

void Count::SetOption(options::ReadPreference::Mode mode) {
  impl_->read_prefs = MakeReadPrefs(mode);
  impl_->read_prefs_desc = MakeReadPrefsDescription(mode);
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

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
CountApprox::CountApprox() = default;
CountApprox::~CountApprox() = default;

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
CountApprox::CountApprox(const CountApprox& other) {
  impl_->read_prefs_desc = other.impl_->read_prefs_desc;
  impl_->read_prefs.reset(
      mongoc_read_prefs_copy(other.impl_->read_prefs.get()));
  impl_->options = other.impl_->options;
}

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
CountApprox::CountApprox(CountApprox&&) noexcept = default;

CountApprox& CountApprox::operator=(const CountApprox& rhs) {
  // NOLINTNEXTLINE(cppcoreguidelines-c-copy-assignment-signature)
  return *this = CountApprox(rhs);
}

// NOLINTNEXTLINE(performance-noexcept-move-constructor)
CountApprox& CountApprox::operator=(CountApprox&&) = default;

void CountApprox::SetOption(const options::ReadPreference& read_prefs) {
  impl_->read_prefs = MakeReadPrefs(read_prefs);
  impl_->read_prefs_desc = MakeReadPrefsDescription(read_prefs);
}

void CountApprox::SetOption(options::ReadPreference::Mode mode) {
  impl_->read_prefs = MakeReadPrefs(mode);
  impl_->read_prefs_desc = MakeReadPrefsDescription(mode);
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

Find::Find(formats::bson::Document filter) : impl_(std::move(filter)) {}
Find::~Find() = default;

Find::Find(const Find& other) : impl_(other.impl_->filter) {
  impl_->read_prefs_desc = other.impl_->read_prefs_desc;
  impl_->read_prefs.reset(
      mongoc_read_prefs_copy(other.impl_->read_prefs.get()));
  impl_->options = other.impl_->options;
}

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
Find::Find(Find&&) noexcept = default;
// NOLINTNEXTLINE(cppcoreguidelines-c-copy-assignment-signature)
Find& Find::operator=(const Find& rhs) { return *this = Find(rhs); }
// NOLINTNEXTLINE(performance-noexcept-move-constructor)
Find& Find::operator=(Find&&) = default;

void Find::SetOption(const options::ReadPreference& read_prefs) {
  impl_->read_prefs = MakeReadPrefs(read_prefs);
  impl_->read_prefs_desc = MakeReadPrefsDescription(read_prefs);
}

void Find::SetOption(options::ReadPreference::Mode mode) {
  impl_->read_prefs = MakeReadPrefs(mode);
  impl_->read_prefs_desc = MakeReadPrefsDescription(mode);
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
  if (projection.IsEmpty()) return;

  static const std::string kOptionName = "projection";
  impl::EnsureBuilder(impl_->options)
      .Append(kOptionName, std::move(projection).Extract());
}

void Find::SetOption(const options::Sort& sort) {
  if (sort.GetOrder().empty()) return;

  static const std::string kOptionName = "sort";
  impl::EnsureBuilder(impl_->options).Append(kOptionName, MakeSortBson(sort));
}

void Find::SetOption(const options::Hint& hint) {
  static const std::string kOptionName = "hint";
  impl::EnsureBuilder(impl_->options).Append(kOptionName, hint.Value());
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
  static const std::string kOptionName = "comment";
  impl::EnsureBuilder(impl_->options).Append(kOptionName, comment.Value());
}

void Find::SetOption(const options::MaxServerTime& max_server_time) {
  static const std::string kOptionName = "maxTimeMS";
  impl::EnsureBuilder(impl_->options)
      .Append(kOptionName, max_server_time.Value().count());
}

InsertOne::InsertOne(formats::bson::Document document)
    : impl_(std::move(document)) {}

InsertOne::~InsertOne() = default;

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
InsertOne::InsertOne(const InsertOne&) = default;
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
InsertOne::InsertOne(InsertOne&&) noexcept = default;
InsertOne& InsertOne::operator=(const InsertOne&) = default;
// NOLINTNEXTLINE(performance-noexcept-move-constructor)
InsertOne& InsertOne::operator=(InsertOne&&) = default;

void InsertOne::SetOption(options::WriteConcern::Level level) {
  AppendWriteConcern(impl::EnsureBuilder(impl_->options), level);
  impl_->write_concern_desc = MakeWriteConcernDescription(level);
}

void InsertOne::SetOption(const options::WriteConcern& write_concern) {
  AppendWriteConcern(impl::EnsureBuilder(impl_->options), write_concern);
  impl_->write_concern_desc = MakeWriteConcernDescription(write_concern);
}

void InsertOne::SetOption(options::SuppressServerExceptions) {
  impl_->should_throw = false;
}

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
InsertMany::InsertMany() = default;

InsertMany::InsertMany(std::vector<formats::bson::Document> documents_)
    : impl_(std::move(documents_)) {}

InsertMany::~InsertMany() = default;

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
InsertMany::InsertMany(const InsertMany&) = default;
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
InsertMany::InsertMany(InsertMany&&) noexcept = default;
InsertMany& InsertMany::operator=(const InsertMany&) = default;
// NOLINTNEXTLINE(performance-noexcept-move-constructor)
InsertMany& InsertMany::operator=(InsertMany&&) = default;

void InsertMany::Append(formats::bson::Document document) {
  impl_->documents.push_back(std::move(document));
}

void InsertMany::SetOption(options::Unordered) {
  static const std::string kOptionName = "ordered";
  impl::EnsureBuilder(impl_->options).Append(kOptionName, false);
}

void InsertMany::SetOption(options::WriteConcern::Level level) {
  AppendWriteConcern(impl::EnsureBuilder(impl_->options), level);
  impl_->write_concern_desc = MakeWriteConcernDescription(level);
}

void InsertMany::SetOption(const options::WriteConcern& write_concern) {
  AppendWriteConcern(impl::EnsureBuilder(impl_->options), write_concern);
  impl_->write_concern_desc = MakeWriteConcernDescription(write_concern);
}

void InsertMany::SetOption(options::SuppressServerExceptions) {
  impl_->should_throw = false;
}

ReplaceOne::ReplaceOne(formats::bson::Document selector,
                       formats::bson::Document replacement)
    : impl_(std::move(selector), std::move(replacement)) {}

ReplaceOne::~ReplaceOne() = default;

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
ReplaceOne::ReplaceOne(const ReplaceOne&) = default;
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
ReplaceOne::ReplaceOne(ReplaceOne&&) noexcept = default;
ReplaceOne& ReplaceOne::operator=(const ReplaceOne&) = default;
// NOLINTNEXTLINE(performance-noexcept-move-constructor)
ReplaceOne& ReplaceOne::operator=(ReplaceOne&&) = default;

void ReplaceOne::SetOption(options::Upsert) {
  impl::AppendUpsert(impl::EnsureBuilder(impl_->options));
}

void ReplaceOne::SetOption(options::WriteConcern::Level level) {
  AppendWriteConcern(impl::EnsureBuilder(impl_->options), level);
  impl_->write_concern_desc = MakeWriteConcernDescription(level);
}

void ReplaceOne::SetOption(const options::WriteConcern& write_concern) {
  AppendWriteConcern(impl::EnsureBuilder(impl_->options), write_concern);
  impl_->write_concern_desc = MakeWriteConcernDescription(write_concern);
}

void ReplaceOne::SetOption(options::SuppressServerExceptions) {
  impl_->should_throw = false;
}

Update::Update(Mode mode, formats::bson::Document selector,
               formats::bson::Document update)
    : impl_(mode, std::move(selector), std::move(update)) {}

Update::~Update() = default;

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
Update::Update(const Update&) = default;
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
Update::Update(Update&&) noexcept = default;
Update& Update::operator=(const Update&) = default;
// NOLINTNEXTLINE(performance-noexcept-move-constructor)
Update& Update::operator=(Update&&) = default;

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
  impl_->write_concern_desc = MakeWriteConcernDescription(level);
}

void Update::SetOption(const options::WriteConcern& write_concern) {
  AppendWriteConcern(impl::EnsureBuilder(impl_->options), write_concern);
  impl_->write_concern_desc = MakeWriteConcernDescription(write_concern);
}

void Update::SetOption(options::SuppressServerExceptions) {
  impl_->should_throw = false;
}

Delete::Delete(Mode mode, formats::bson::Document selector)
    : impl_(mode, std::move(selector)) {}

Delete::~Delete() = default;

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
Delete::Delete(const Delete&) = default;
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
Delete::Delete(Delete&&) noexcept = default;
Delete& Delete::operator=(const Delete&) = default;
// NOLINTNEXTLINE(performance-noexcept-move-constructor)
Delete& Delete::operator=(Delete&&) = default;

void Delete::SetOption(options::WriteConcern::Level level) {
  AppendWriteConcern(impl::EnsureBuilder(impl_->options), level);
  impl_->write_concern_desc = MakeWriteConcernDescription(level);
}

void Delete::SetOption(const options::WriteConcern& write_concern) {
  AppendWriteConcern(impl::EnsureBuilder(impl_->options), write_concern);
  impl_->write_concern_desc = MakeWriteConcernDescription(write_concern);
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

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
FindAndModify::FindAndModify(FindAndModify&&) noexcept = default;
// NOLINTNEXTLINE(performance-noexcept-move-constructor)
FindAndModify& FindAndModify::operator=(FindAndModify&&) = default;

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
  impl_->write_concern_desc = MakeWriteConcernDescription(level);
}

void FindAndModify::SetOption(const options::WriteConcern& write_concern) {
  formats::bson::impl::BsonBuilder wc_builder;
  AppendWriteConcern(wc_builder, write_concern);
  if (!mongoc_find_and_modify_opts_append(impl_->options.get(),
                                          wc_builder.Extract().get())) {
    throw MongoException("Cannot set write concern");
  }
  impl_->write_concern_desc = MakeWriteConcernDescription(write_concern);
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

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
FindAndRemove::FindAndRemove(FindAndRemove&&) noexcept = default;
// NOLINTNEXTLINE(performance-noexcept-move-constructor)
FindAndRemove& FindAndRemove::operator=(FindAndRemove&&) = default;

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
  impl_->write_concern_desc = MakeWriteConcernDescription(level);
}

void FindAndRemove::SetOption(const options::WriteConcern& write_concern) {
  formats::bson::impl::BsonBuilder wc_builder;
  AppendWriteConcern(wc_builder, write_concern);
  if (!mongoc_find_and_modify_opts_append(impl_->options.get(),
                                          wc_builder.Extract().get())) {
    throw MongoException("Cannot set write concern");
  }
  impl_->write_concern_desc = MakeWriteConcernDescription(write_concern);
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

}  // namespace storages::mongo::operations
