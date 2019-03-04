#include <storages/mongo_ng/operations.hpp>

#include <limits>

#include <bson/bson.h>
#include <mongoc/mongoc.h>

#include <formats/bson/value_builder.hpp>
#include <storages/mongo_ng/exception.hpp>

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

void AppendReadConcern(formats::bson::impl::BsonBuilder& builder,
                       options::ReadConcern level) {
  impl::ReadConcernPtr read_concern(mongoc_read_concern_new());
  const char* native_level = ToNativeReadConcernLevel(level);
  if (!mongoc_read_concern_set_level(read_concern.get(), native_level)) {
    throw MongoException("Cannot set read concern '") << native_level << '\'';
  }

  if (!mongoc_read_concern_append(read_concern.get(), builder.Get())) {
    throw MongoException("Cannot write read concern option");
  }
}

void AppendUint64Option(formats::bson::impl::BsonBuilder& builder,
                        const std::string& name, uint64_t value) {
  if (value > std::numeric_limits<int64_t>::max()) {
    throw InvalidQueryArgumentException("Value")
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
  // order is significant here, so we build BSON directly
  formats::bson::impl::SubdocBson subdoc(GetBuilder(impl_->options).Get(),
                                         kOptionName.c_str(),
                                         kOptionName.size());
  for (const auto& [field, direction] : sort.GetOrder()) {
    bson_append_int32(
        subdoc.Get(), field.c_str(), field.size(),
        direction == options::Sort::Direction::kAscending ? 1 : -1);
  }
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

void Find::SetOption(options::BatchSize batch_size) {
  if (!batch_size.Value()) return;

  static const std::string kOptionName = "batchSize";
  AppendUint64Option(GetBuilder(impl_->options), kOptionName,
                     batch_size.Value());
}

void Find::SetOption(const options::MaxServerTime& max_server_time) {
  static const std::string kOptionName = "maxTimeMS";
  GetBuilder(impl_->options)
      .Append(kOptionName, max_server_time.Value().count());
}

}  // namespace storages::mongo_ng::operations
