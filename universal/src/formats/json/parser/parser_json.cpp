#include <userver/formats/json/parser/parser_json.hpp>

#include <rapidjson/allocators.h>
#include <rapidjson/document.h>

#include <formats/json/impl/types_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json::parser {

namespace {
::rapidjson::CrtAllocator g_allocator;
}  // namespace

struct JsonValueParser::Impl {
  impl::Document raw_value_{&g_allocator};
  size_t level_{0};
};

JsonValueParser::JsonValueParser() = default;

JsonValueParser::~JsonValueParser() = default;

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
  if (!impl_->raw_value_.EndObject(members)) Throw(Expected());

  impl_->level_--;
  MaybePopSelf();
}

void JsonValueParser::StartArray() {
  if (!impl_->raw_value_.StartArray()) Throw(Expected());
  impl_->level_++;
}

void JsonValueParser::EndArray(size_t members) {
  if (!impl_->raw_value_.EndArray(members)) Throw(Expected());

  impl_->level_--;
  MaybePopSelf();
}

std::string JsonValueParser::Expected() const { return "anything"; }

void JsonValueParser::MaybePopSelf() {
  if (impl_->level_ == 0) {
    auto generator = [](const auto&) { return true; };
    impl_->raw_value_.Populate(generator);

    this->SetResult(
        Value{impl::VersionedValuePtr::Create(std::move(impl_->raw_value_))});
  }
}

}  // namespace formats::json::parser

USERVER_NAMESPACE_END
