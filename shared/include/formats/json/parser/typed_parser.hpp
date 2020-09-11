#pragma once

#include <optional>

#include <formats/json/parser/base_parser.hpp>
#include <utils/assert.hpp>

namespace formats::json::parser {

template <typename T>
class Subscriber {
 public:
  virtual ~Subscriber() = default;

  virtual void OnSend(T&&) = 0;
};

template <typename T>
class SubscriberSink final : public Subscriber<T> {
 public:
  SubscriberSink(T& data) : data_(data) {}

  void OnSend(T&& value) override { data_ = std::move(value); }

 private:
  T& data_;
};

template <typename T>
class SubscriberSinkOptional final : public Subscriber<T>,
                                     public Subscriber<std::optional<T>> {
 public:
  SubscriberSinkOptional(std::optional<T>& data) : data_(data) {}

  void OnSend(T&& value) override { data_ = std::move(value); }

  void OnSend(std::optional<T>&& value) override { data_ = std::move(value); }

 private:
  std::optional<T>& data_;
};

template <typename T>
class TypedParser : public BaseParser {
 public:
  void Subscribe(Subscriber<T>& subscriber) { subscriber_ = &subscriber; }

  using ResultType = T;

  virtual void Reset(){};

 protected:
  void SetResult(T&& value) {
    parser_state_->PopMe(*this);
    if (subscriber_) subscriber_->OnSend(std::move(value));
  }

  Subscriber<T>* subscriber_{nullptr};
};

template <typename T, typename Parser>
T ParseToType(std::string_view input) {
  T result{};
  Parser parser;
  parser.Reset();
  SubscriberSink<T> sink(result);
  parser.Subscribe(sink);

  ParserState state;
  state.PushParser(parser);
  state.ProcessInput(input);

  return result;
}

}  // namespace formats::json::parser
