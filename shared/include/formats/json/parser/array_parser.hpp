#pragma once

#include <formats/json/parser/typed_parser.hpp>

namespace formats::json::parser {

// Parser for array -> vector/set/unordered_set
template <typename Item, typename ItemParser,
          typename Array = std::vector<Item>>
class ArrayParser final : public TypedParser<Array> {
 public:
  explicit ArrayParser(
      ItemParser& item_parser,
      const BaseValidator<Array>& validator = kEmptyValidator<Array>)
      : TypedParser<Array>(validator), item_parser_(item_parser) {}

  void Reset(Array& result) {
    index_ = 0;
    state_ = State::kStart;
    TypedParser<Array>::Reset(result);

    /*
     * Heuristics:
     * STL impls have a small initial capacity of vector.
     * It leads to multiple reallocations during inserts.
     * We intentionally give up some maybe-unused space to lower realloc count.
     */
    result.reserve(16);
  }

 protected:
  void StartArray() override {
    if (state_ == State::kStart) {
      state_ = State::kInside;
    } else {
      PushParser();
      Parser().StartArray();
    }
  }
  void EndArray() override {
    if (state_ == State::kInside) {
      this->PopAndValidate();
      return;
    }
    // impossible?
    this->Throw("end array");
  }

  void Int64(int64_t i) override {
    PushParser();
    Parser().Int64(i);
  }
  void Uint64(uint64_t i) override {
    PushParser();
    Parser().Uint64(i);
  }
  void Null() override {
    PushParser();
    Parser().Null();
  }
  void Bool(bool b) override {
    PushParser();
    Parser().Bool(b);
  }
  void Double(double d) {
    PushParser();
    Parser().Double(d);
  }
  void String(std::string_view sw) override {
    PushParser();
    Parser().String(sw);
  }
  void StartObject() override {
    PushParser();
    Parser().StartObject();
  }

  void PushParser() {
    if (state_ != State::kInside) this->Throw("array");

    // TODO: non-vector
    item_parser_.Reset(*this->result_->emplace(this->result_->end()));
    this->parser_state_->PushParser(item_parser_, index_++);
  }

  std::string Expected() const override { return "array"; }

  BaseParser& Parser() { return item_parser_; }

 private:
  ItemParser& item_parser_;
  std::optional<size_t> min_items_, max_items_;

  enum class State {
    kStart,
    kInside,
  };
  size_t index_;
  State state_{State::kStart};
};

}  // namespace formats::json::parser
