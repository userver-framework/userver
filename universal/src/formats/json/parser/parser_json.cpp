#include <userver/formats/json/parser/parser_json.hpp>

#include <rapidjson/allocators.h>
#include <rapidjson/document.h>

#include <formats/json/impl/types_impl.hpp>

#include <userver/formats/json/serialize.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json::parser {

namespace {
::rapidjson::CrtAllocator g_allocator;
}  // namespace

struct JsonValueParser::Impl {
    json::impl::Document raw_value{&g_allocator};
    size_t level{0};

    ~Impl() {
        auto generator = [](const auto&) { return false; };
        // This forces GenericDocument to clean up its stack,
        // and doesn't do anything other than that.
        raw_value.Populate(generator);
    }
};

JsonValueParser::JsonValueParser() = default;

JsonValueParser::~JsonValueParser() = default;

void JsonValueParser::Null() {
    if (!impl_->raw_value.Null()) Throw(Expected());
    MaybePopSelf();
}

void JsonValueParser::Bool(bool value) {
    if (!impl_->raw_value.Bool(value)) Throw(Expected());
    MaybePopSelf();
}

void JsonValueParser::Int64(int64_t value) {
    if (!impl_->raw_value.Int64(value)) Throw(Expected());
    MaybePopSelf();
}

void JsonValueParser::Uint64(uint64_t value) {
    if (!impl_->raw_value.Uint64(value)) Throw(Expected());
    MaybePopSelf();
}

void JsonValueParser::Double(double value) {
    if (!impl_->raw_value.Double(value)) Throw(Expected());
    MaybePopSelf();
}

void JsonValueParser::String(std::string_view sw) {
    if (!impl_->raw_value.String(sw.data(), sw.size(), true)) Throw(Expected());
    MaybePopSelf();
}

void JsonValueParser::StartObject() {
    if (!impl_->raw_value.StartObject()) Throw(Expected());
    if (impl_->level++ > kDepthParseLimit)
        throw InternalParseError("Exceeded maximum allowed JSON depth of: " + std::to_string(kDepthParseLimit));
}

void JsonValueParser::Key(std::string_view key) {
    if (!impl_->raw_value.Key(key.data(), key.size(), true)) Throw(Expected());
}

void JsonValueParser::EndObject(size_t members) {
    if (!impl_->raw_value.EndObject(members)) Throw(Expected());

    impl_->level--;
    MaybePopSelf();
}

void JsonValueParser::StartArray() {
    if (!impl_->raw_value.StartArray()) Throw(Expected());
    if (impl_->level++ > kDepthParseLimit)
        throw InternalParseError("Exceeded maximum allowed JSON depth of: " + std::to_string(kDepthParseLimit));
}

void JsonValueParser::EndArray(size_t members) {
    if (!impl_->raw_value.EndArray(members)) Throw(Expected());

    impl_->level--;
    MaybePopSelf();
}

std::string JsonValueParser::Expected() const { return "anything"; }

void JsonValueParser::MaybePopSelf() {
    if (impl_->level == 0) {
        auto generator = [](const auto&) { return true; };
        impl_->raw_value.Populate(generator);

        this->SetResult(Value{json::impl::VersionedValuePtr::Create(std::move(impl_->raw_value))});
    }
}

}  // namespace formats::json::parser

USERVER_NAMESPACE_END
