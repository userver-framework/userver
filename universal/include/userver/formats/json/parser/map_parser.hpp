#pragma once

#include <userver/formats/json/parser/typed_parser.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json::parser {

template <typename Map, typename ValueParser>
class MapParser final : public TypedParser<Map>,
                        public Subscriber<typename Map::mapped_type> {
 public:
  using Value = typename Map::mapped_type;

  explicit MapParser(ValueParser& value_parser) : value_parser_(value_parser) {}

  void Reset() override { this->state_ = State::kStart; }

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

    key_ = key;
    this->value_parser_.Reset();
    this->value_parser_.Subscribe(*this);
    this->parser_state_->PushParser(this->value_parser_.GetParser());
  }

  void EndObject() override {
    if (state_ == State::kInside) {
      this->SetResult(std::move(result_));
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

    UINVARIANT(false, "Unexpected parser state");
  }

 private:
  void OnSend(Value&& value) override {
    this->result_.emplace(std::move(key_), std::move(value));
  }

  std::string GetPathItem() const override { return key_; }

  enum class State {
    kStart,
    kInside,
  };
  State state_;
  std::string key_;
  Map result_;
  ValueParser& value_parser_;
};

}  // namespace formats::json::parser

USERVER_NAMESPACE_END
