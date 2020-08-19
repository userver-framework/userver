#pragma once

#include <formats/json/parser/typed_parser.hpp>
#include <utils/meta.hpp>

namespace formats::json::parser {

namespace impl {

template <typename Item>
struct ItemStorageNonVector {
  Item item;
};

template <typename Item>
struct ItemStorageVector {};

template <typename Item, typename Array>
struct ItemStorage
    : public std::conditional_t<
          meta::is_vector<Array>::value && !std::is_same<Item, bool>::value,
          ItemStorageVector<Item>, ItemStorageNonVector<Item>> {};

}  // namespace impl

// Parser for array -> vector/set/unordered_set
template <typename Item, typename ItemParser,
          typename Array = std::vector<Item>>
class ArrayParser final : public TypedParser<Array>, public Subscriber {
 public:
  explicit ArrayParser(ItemParser& item_parser) : item_parser_(item_parser) {}

  void Reset(Array& result) override {
    index_ = 0;
    state_ = State::kStart;
    TypedParser<Array>::Reset(result);

    if constexpr (meta::is_vector<Array>::value) {
      /*
       * Heuristics:
       * STL impls have a small initial capacity of vector.
       * It leads to multiple reallocations during inserts.
       * We intentionally give up some maybe-unused space to lower realloc
       * count.
       */
      result.reserve(16);
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
      this->parser_state_->PopMe(*this);
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

  void PushParser() {
    if (state_ != State::kInside) this->Throw("array");

    if constexpr (meta::is_vector<Array>::value &&
                  !std::is_same<Item, bool>::value) {
      this->item_parser_.Reset(*this->result_->emplace(this->result_->end()));
    } else {
      this->item_storage_.item = {};
      this->item_parser_.Subscribe(*this);
      this->item_parser_.Reset(this->item_storage_.item);
    }
    this->parser_state_->PushParser(item_parser_, index_++);
  }

  std::string Expected() const override { return "array"; }

  BaseParser& Parser() { return item_parser_; }

 protected:
  void OnSend() override {
    if constexpr (!meta::is_vector<Array>::value) {
      this->result_->insert(std::move(this->item_storage_.item));
    } else if constexpr (std::is_same<Item, bool>::value) {
      this->result_->push_back(std::move(this->item_storage_.item));
    }
  }

 private:
  ItemParser& item_parser_;
  std::optional<size_t> min_items_, max_items_;

  enum class State {
    kStart,
    kInside,
  };
  size_t index_;
  State state_{State::kStart};
  // TODO: C++20 [[no_unique_address]]
  impl::ItemStorage<Item, Array> item_storage_;
};

}  // namespace formats::json::parser
