#include <formats/json/parser/parser_json.hpp>

#include <rapidjson/document.h>

#include <formats/json/types.hpp>

namespace formats::json::parser {

struct JsonValueParser::Impl {
  ::formats::json::impl::Document raw_value_;
  size_t level_{0};
};

// Explicit ctr to silence clang-tidy
JsonValueParser::JsonValueParser() : impl_{} {}

JsonValueParser::~JsonValueParser() = default;

void JsonValueParser::Reset(Value& result) {
  *impl_ = {};
  TypedParser<Value>::Reset(result);
}

void JsonValueParser::Null() {
  if (!impl_->raw_value_.Null()) Throw(Expected());
  MaybePopSelf();
}

void JsonValueParser::Bool(bool value) {
  if (!impl_->raw_value_.Bool(value)) Throw(Expected());
  MaybePopSelf();
}

void JsonValueParser::Int64(int64_t value) {
  if (!impl_->raw_value_.Int64(value)) Throw(Expected());
  MaybePopSelf();
}

void JsonValueParser::Uint64(uint64_t value) {
  if (!impl_->raw_value_.Uint64(value)) Throw(Expected());
  MaybePopSelf();
}

void JsonValueParser::Double(double value) {
  if (!impl_->raw_value_.Double(value)) Throw(Expected());
  MaybePopSelf();
}

void JsonValueParser::String(std::string_view sw) {
  if (!impl_->raw_value_.String(sw.data(), sw.size(), true)) Throw(Expected());
  MaybePopSelf();
}

void JsonValueParser::StartObject() {
  if (!impl_->raw_value_.StartObject()) Throw(Expected());
  impl_->level_++;
}

void JsonValueParser::Key(std::string_view key) {
  if (!impl_->raw_value_.Key(key.data(), key.size(), true)) Throw(Expected());
}

void JsonValueParser::EndObject(size_t members) {
  // rapidjson stores malloc'ed data in ptr inside of union
  // and manages it manually, which is not visible to clang-tidy
  // NOLINTNEXTLINE(clang-analyzer-unix.Malloc)
  if (!impl_->raw_value_.EndObject(members)) Throw(Expected());

  impl_->level_--;
  MaybePopSelf();
}

void JsonValueParser::StartArray() {
  if (!impl_->raw_value_.StartArray()) Throw(Expected());
  impl_->level_++;
}

void JsonValueParser::EndArray(size_t members) {
  // rapidjson stores malloc'ed data in ptr inside of union
  // and manages it manually, which is not visible to clang-tidy
  // NOLINTNEXTLINE(clang-analyzer-unix.Malloc)
  if (!impl_->raw_value_.EndArray(members)) Throw(Expected());

  impl_->level_--;
  MaybePopSelf();
}

std::string JsonValueParser::Expected() const { return "anything"; }

void JsonValueParser::MaybePopSelf() {
  if (impl_->level_ == 0) {
    auto generator = [](const auto&) { return true; };
    impl_->raw_value_.Populate(generator);

    auto root = std::make_shared<impl::Value>();
    root->Swap(impl_->raw_value_);
    this->SetResult(Value{std::move(root)});
  }
}

}  // namespace formats::json::parser
