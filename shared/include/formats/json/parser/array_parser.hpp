#pragma once

#include <formats/json/parser/typed_parser.hpp>
#include <utils/meta.hpp>

namespace formats::json::parser {

// Parser for array -> vector/set/unordered_set
template <typename Item, typename ItemParser,
          typename Array = std::vector<Item>>
class ArrayParser final : public TypedParser<Array>, public Subscriber<Item> {
 public:
  explicit ArrayParser(ItemParser& item_parser) : item_parser_(item_parser) {
    this->item_parser_.Subscribe(*this);
  }

  void Reset() override {
    index_ = 0;
    state_ = State::kStart;
    storage_.clear();

    if constexpr (meta::kIsVector<Array>) {
      /*
       * Heuristics:
       * STL impls have a small initial capacity of vector.
       * It leads to multiple reallocations during inserts.
       * We intentionally give up some maybe-unused space to lower realloc
       * count.
       */
      storage_.reserve(16);
    }
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
      this->SetResult(std::move(storage_));
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
  void Double(double d) override {
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

  std::string Expected() const override { return "array"; }

  BaseParser& Parser() { return item_parser_; }

 protected:
  void PushParser() {
    if (state_ != State::kInside) this->Throw("array");

    this->item_parser_.Reset();
    this->parser_state_->PushParser(item_parser_, index_++);
  }

  void OnSend(Item&& item) override {
    if constexpr (!meta::kIsVector<Array>) {
      this->storage_.insert(std::move(item));
    } else {
      this->storage_.push_back(std::move(item));
    }
  }

 private:
  ItemParser& item_parser_;
  std::optional<size_t> min_items_, max_items_;

  enum class State {
    kStart,
    kInside,
  };
  size_t index_{0};
  State state_{State::kStart};
  Array storage_;
};

}  // namespace formats::json::parser
