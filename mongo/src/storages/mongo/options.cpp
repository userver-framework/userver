#include <userver/storages/mongo/options.hpp>

#include <userver/formats/bson/inline.hpp>
#include <userver/formats/bson/value_builder.hpp>
#include <userver/storages/mongo/exception.hpp>
#include <userver/utils/text.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::options {

ReadPreference::ReadPreference(Mode mode) : mode_(mode) {}

ReadPreference::ReadPreference(Mode mode,
                               std::vector<formats::bson::Document> tags)
    : mode_(mode), tags_(std::move(tags)) {}

ReadPreference::Mode ReadPreference::GetMode() const { return mode_; }

std::optional<std::chrono::seconds> ReadPreference::GetMaxStaleness() const {
  return max_staleness_;
}

const std::vector<formats::bson::Document>& ReadPreference::GetTags() const {
  return tags_;
}

ReadPreference& ReadPreference::SetMaxStaleness(
    std::optional<std::chrono::seconds> max_staleness) {
  // https://github.com/mongodb/specifications/blob/master/source/max-staleness/max-staleness.rst#smallest-allowed-value-for-maxstalenessseconds
  constexpr static std::chrono::seconds kSmallestMaxStaleness{90};

  if (max_staleness && *max_staleness < kSmallestMaxStaleness) {
    throw InvalidQueryArgumentException("Invalid max staleness value ")
        << max_staleness->count() << "s, must be at least "
        << kSmallestMaxStaleness.count() << 's';
  }

  max_staleness_ = max_staleness;
  return *this;
}

ReadPreference& ReadPreference::AddTag(formats::bson::Document tag) {
  tags_.push_back(std::move(tag));
  return *this;
}

WriteConcern::WriteConcern(Level level)
    : nodes_count_(0), is_majority_(false), timeout_(0) {
  switch (level) {
    case Level::kMajority:
      is_majority_ = true;
      timeout_ = kDefaultMajorityTimeout;
      break;

    case Level::kUnacknowledged:;  // already set up
  }
}

WriteConcern::WriteConcern(size_t nodes_count)
    : nodes_count_(nodes_count), is_majority_(false), timeout_(0) {}

WriteConcern::WriteConcern(std::string tag)
    : nodes_count_(0), is_majority_(false), tag_(std::move(tag)), timeout_(0) {
  if (!utils::text::IsCString(tag_)) {
    throw InvalidQueryArgumentException("Invalid write concern tag");
  }
}

bool WriteConcern::IsMajority() const { return is_majority_; }
size_t WriteConcern::NodesCount() const { return nodes_count_; }
const std::string& WriteConcern::Tag() const { return tag_; }
std::optional<bool> WriteConcern::Journal() const { return journal_; }

const std::chrono::milliseconds& WriteConcern::Timeout() const {
  return timeout_;
}

WriteConcern& WriteConcern::SetTimeout(std::chrono::milliseconds timeout) {
  timeout_ = timeout;
  return *this;
}

WriteConcern& WriteConcern::SetJournal(bool value) {
  journal_ = value;
  return *this;
}

Projection::Projection(
    std::initializer_list<std::string_view> fields_to_include) {
  for (const auto& field : fields_to_include) Include(field);
}

Projection& Projection::Include(std::string_view field) {
  projection_builder_.Append(field, true);
  return *this;
}

Projection& Projection::Exclude(std::string_view field) {
  projection_builder_.Append(field, false);
  return *this;
}

Projection& Projection::Slice(std::string_view field, int32_t limit,
                              int32_t skip) {
  static const std::string kSliceOp = "$slice";
  formats::bson::Value slice;
  if (!skip) {
    slice = formats::bson::MakeDoc(kSliceOp, limit);
  } else {
    if (limit < 0) {
      throw InvalidQueryArgumentException("Cannot use negative slice limit ")
          << limit << " with nonzero skip " << skip << " in projection";
    }
    slice =
        formats::bson::MakeDoc(kSliceOp, formats::bson::MakeArray(skip, limit));
  }

  projection_builder_.Append(field, slice);
  return *this;
}

Projection& Projection::ElemMatch(std::string_view field,
                                  const formats::bson::Document& pred) {
  static const std::string kElemMatchOp = "$elemMatch";
  projection_builder_.Append(field, formats::bson::MakeDoc(kElemMatchOp, pred));
  return *this;
}

const bson_t* Projection::GetProjectionBson() const {
  return projection_builder_.Get();
}

Sort::Sort(
    std::initializer_list<std::pair<std::string_view, Direction>> order) {
  for (const auto& [field, direction] : order) By(field, direction);
}

Sort& Sort::By(std::string_view field, Direction direction) {
  sort_builder_.Append(
      field, direction == options::Sort::Direction::kAscending ? 1 : -1);
  return *this;
}

const bson_t* Sort::GetSortBson() const { return sort_builder_.Get(); }

Hint::Hint(std::string index_name)
    : value_(
          formats::bson::ValueBuilder(std::move(index_name)).ExtractValue()) {}

Hint::Hint(formats::bson::Document index_spec)
    : value_(std::move(index_spec)) {}

const formats::bson::Value& Hint::Value() const { return value_; }

Comment::Comment(std::string value) : value_(std::move(value)) {
  if (!utils::text::IsUtf8(value_)) {
    throw InvalidQueryArgumentException(
        "Provided comment is not a valid UTF-8");
  }
}

const std::string& Comment::Value() const { return value_; }

}  // namespace storages::mongo::options

USERVER_NAMESPACE_END
