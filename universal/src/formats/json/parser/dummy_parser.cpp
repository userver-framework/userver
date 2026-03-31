#include <userver/formats/json/parser/dummy_parser.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json::parser {

DummyParser::DummyParser() = default;

DummyParser::~DummyParser() = default;

void DummyParser::Null() { MaybePopSelf(); }

void DummyParser::Bool(bool) { MaybePopSelf(); }

void DummyParser::Int64(int64_t) { MaybePopSelf(); }

void DummyParser::Uint64(uint64_t) { MaybePopSelf(); }

void DummyParser::Double(double) { MaybePopSelf(); }

void DummyParser::String(std::string_view) { MaybePopSelf(); }

void DummyParser::StartObject() {
    level_++;
    MaybePopSelf();
}

void DummyParser::Key(std::string_view) { MaybePopSelf(); }

void DummyParser::EndObject() {
    level_--;
    MaybePopSelf();
}

void DummyParser::StartArray() {
    level_++;
    MaybePopSelf();
}

void DummyParser::EndArray() {
    level_--;
    MaybePopSelf();
}

std::string DummyParser::Expected() const { return "data"; }

void DummyParser::Reset() { level_ = 0; }

DummyParser& DummyParser::GetParser() { return *this; }

void DummyParser::MaybePopSelf() {
    if (level_ == 0) {
        parser_state_->PopMe(*this);
    }
}

}  // namespace formats::json::parser

USERVER_NAMESPACE_END
