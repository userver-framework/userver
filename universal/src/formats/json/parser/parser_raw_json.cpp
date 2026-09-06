#include <userver/formats/json/parser/parser_raw_json.hpp>

#include <string>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <userver/formats/json/serialize.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json::parser {

struct JsonRawStringParser::Impl {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer{buffer};
    std::size_t level{0};

    void Reset() {
        buffer.Clear();
        writer.Reset(buffer);
        level = 0;
    }
};

JsonRawStringParser::JsonRawStringParser() = default;

JsonRawStringParser::~JsonRawStringParser() = default;

void JsonRawStringParser::Reset() { impl_->Reset(); }

void JsonRawStringParser::Null() {
    if (!impl_->writer.Null()) [[unlikely]] {
        Throw(Expected());
    }
    MaybeSetResult();
}

void JsonRawStringParser::Bool(bool value) {
    if (!impl_->writer.Bool(value)) [[unlikely]] {
        Throw(Expected());
    }
    MaybeSetResult();
}

void JsonRawStringParser::Int64(std::int64_t value) {
    if (!impl_->writer.Int64(value)) [[unlikely]] {
        Throw(Expected());
    }
    MaybeSetResult();
}

void JsonRawStringParser::Uint64(std::uint64_t value) {
    if (!impl_->writer.Uint64(value)) [[unlikely]] {
        Throw(Expected());
    }
    MaybeSetResult();
}

void JsonRawStringParser::Double(double value) {
    if (!impl_->writer.Double(value)) [[unlikely]] {
        Throw(Expected());
    }
    MaybeSetResult();
}

void JsonRawStringParser::String(std::string_view value) {
    if (!impl_->writer.String(value.data(), value.size())) [[unlikely]] {
        Throw(Expected());
    }
    MaybeSetResult();
}

void JsonRawStringParser::StartObject() {
    if (!impl_->writer.StartObject()) [[unlikely]] {
        Throw(Expected());
    }
    if (impl_->level++ > kDepthParseLimit) {
        throw InternalParseError("Exceeded maximum allowed JSON depth of: " + std::to_string(kDepthParseLimit));
    }
}

void JsonRawStringParser::Key(std::string_view key) {
    if (!impl_->writer.Key(key.data(), key.size())) [[unlikely]] {
        Throw(Expected());
    }
}

void JsonRawStringParser::EndObject() {
    if (!impl_->writer.EndObject()) [[unlikely]] {
        Throw(Expected());
    }
    --impl_->level;
    MaybeSetResult();
}

void JsonRawStringParser::StartArray() {
    if (!impl_->writer.StartArray()) [[unlikely]] {
        Throw(Expected());
    }
    if (impl_->level++ > kDepthParseLimit) {
        throw InternalParseError("Exceeded maximum allowed JSON depth of: " + std::to_string(kDepthParseLimit));
    }
}

void JsonRawStringParser::EndArray() {
    if (!impl_->writer.EndArray()) [[unlikely]] {
        Throw(Expected());
    }
    --impl_->level;
    MaybeSetResult();
}

std::string JsonRawStringParser::Expected() const { return "anything"; }

void JsonRawStringParser::MaybeSetResult() {
    if (impl_->level == 0) {
        SetResult(RawString{std::string{impl_->buffer.GetString(), impl_->buffer.GetLength()}});
    }
}

}  // namespace formats::json::parser

USERVER_NAMESPACE_END
