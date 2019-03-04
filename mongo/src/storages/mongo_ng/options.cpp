#include <storages/mongo_ng/options.hpp>

#include <storages/mongo_ng/exception.hpp>
#include <utils/text.hpp>

namespace storages::mongo_ng::options {

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

Projection::Projection()
    : builder_(formats::bson::ValueBuilder::Type::kDocument) {}

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

}  // namespace storages::mongo_ng::options
