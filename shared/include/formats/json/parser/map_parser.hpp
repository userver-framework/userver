#pragma once

#include <formats/json/parser/typed_parser.hpp>

namespace formats::json::parser {

template <typename Map, typename ValueParser>
class MapParser final : public TypedParser<Map> {
 public:
  using Value = typename Map::mapped_type;

  explicit MapParser(ValueParser& value_parser) : value_parser_(value_parser) {}

  void Reset(Map& result) override {
    this->state_ = State::kStart;
    TypedParser<Map>::Reset(result);
  }

  void StartObject() override {
    switch (state_) {
      case State::kStart:
        this->state_ = State::kInside;
        break;

      case State::kInside:
        this->Throw("{");
    }
  }

  void Key(std::string_view key) override {
    if (state_ != State::kInside) this->Throw("object");

    auto& value = (*this->result_)[std::string{key}];
    this->value_parser_.Reset(value);
    this->parser_state_->PushParser(this->value_parser_, key);
  }

  void EndObject() override {
    if (state_ == State::kInside) {
      this->parser_state_->PopMe(*this);
      return;
    }
    // impossible?
    this->Throw("}");
  }

  std::string Expected() const override {
    switch (state_) {
      case State::kInside:
        return "string";

      case State::kStart:
        return "object";
    }
  }

 private:
  enum class State {
    kStart,
    kInside,
  };
  State state_;
  Value value_;
  ValueParser& value_parser_;
};

}  // namespace formats::json::parser
