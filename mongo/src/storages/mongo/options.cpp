#include <storages/mongo/options.hpp>

#include <storages/mongo/exception.hpp>
#include <utils/text.hpp>

namespace storages::mongo::options {

ReadPreference::ReadPreference(Mode mode) : mode_(mode) {}

ReadPreference::ReadPreference(Mode mode,
                               std::vector<formats::bson::Document> tags)
    : mode_(mode), tags_(std::move(tags)) {}

ReadPreference::Mode ReadPreference::GetMode() const { return mode_; }

const std::vector<formats::bson::Document>& ReadPreference::GetTags() const {
  return tags_;
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
const boost::optional<bool>& WriteConcern::Journal() const { return journal_; }

const std::chrono::milliseconds& WriteConcern::Timeout() const {
  return timeout_;
}

WriteConcern& WriteConcern::SetTimeout(std::chrono::milliseconds timeout) {
  timeout_ = std::move(timeout);
  return *this;
}

WriteConcern& WriteConcern::SetJournal(bool value) {
  journal_ = value;
  return *this;
}

Projection::Projection()
    : builder_(formats::bson::ValueBuilder::Type::kObject) {}

Projection::Projection(std::initializer_list<std::string> fields_to_include)
    : Projection() {
  for (const auto& field : fields_to_include) Include(field);
}

Projection::Projection(formats::bson::Document doc)
    : builder_(std::move(doc)) {}

Projection& Projection::Include(const std::string& field) {
  builder_[field] = true;
  return *this;
}

Projection& Projection::Exclude(const std::string& field) {
  builder_[field] = false;
  return *this;
}

Projection& Projection::Slice(const std::string& field, int32_t limit,
                              int32_t skip) {
  static const std::string kSliceOp = "$slice";
  formats::bson::ValueBuilder slice;
  if (!skip) {
    slice[kSliceOp] = limit;
  } else {
    if (limit < 0) {
      throw InvalidQueryArgumentException("Cannot use negative slice limit ")
          << limit << " with nonzero skip " << skip << " in projection";
    }
    slice[kSliceOp].PushBack(skip);
    slice[kSliceOp].PushBack(limit);
  }

  builder_[field] = std::move(slice);
  return *this;
}

Projection& Projection::ElemMatch(const std::string& field,
                                  formats::bson::Document pred) {
  static const std::string kElemMatchOp = "$elemMatch";
  formats::bson::ValueBuilder elem_match;
  elem_match[kElemMatchOp] = std::move(pred);

  builder_[field] = std::move(elem_match);
  return *this;
}

bool Projection::IsEmpty() const { return builder_.GetSize() == 0; }

formats::bson::Document Projection::Extract() && {
  return builder_.ExtractValue();
}

Sort::Sort(Order order) : order_(std::move(order)) {}

Sort::Sort(std::initializer_list<Order::value_type> order) {
  for (auto& [field, direction] : order) By(std::move(field), direction);
}

Sort& Sort::By(std::string field, Direction direction) {
  order_.emplace_back(std::move(field), direction);
  return *this;
}

const Sort::Order& Sort::GetOrder() const { return order_; }

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
