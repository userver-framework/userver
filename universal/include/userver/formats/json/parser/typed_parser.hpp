#pragma once

#include <optional>

#include <userver/formats/json/parser/base_parser.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

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

/// @brief Main base class for SAX parsers
///
/// There are two main groups of SAX parser classes:
/// - typed parser
/// - proxy parser
///
/// TypedParser derivative is a parser that handles JSON tokens by itself.
/// It implements methods of BaseParser for handling specific tokens (e.g.
/// Null(), StartArray(), Double()). Usually parser implements only 1-2 methods
/// of BaseParser for handling a fixed set of JSON tokens and leaves default
/// implementation for the rest (iow, treat other JSON tokens as a parse error).
///
/// Parser usually maintains its state as a field(s). Parse methods are called
/// when a specific input token is read, and implementation updates the parser
/// state. When finished, the parser calls SetResult() with the cooked value. It
/// pops current parser from the parser stack and signals the subscriber with
/// the result.
///
/// TypedParser may delegate part of its job to subparsers. It is very common to
/// define a parser for an object/array and reuse field parsers for JSON object
/// fields parsing. A subparser is usually implemented as a field of a parser
/// class. When such parser wants to start subobject parsing, it pushes the
/// subparser onto the stack (and maybe calls some of its parse methods).
///
/// E.g.:
///
/// ~~~~~~~~~~~~~~{.cpp}
/// void SomeParser::Double(double d) {
///   subparser_->Reset();
///   parser_state_->PushParser(subparser_.GetParser());
///   subparser_->Double(d);
/// }
/// ~~~~~~~~~~~~~~
///
/// You may also implement a proxy parser. It derives neither from TypedParser
/// nor any other userver's parser class. A proxy parser is a class that
/// delegates the whole job of input token handling to subparser(s), but somehow
/// mutates the result (e.g. converts or validates it) - proxies the result. It
/// doesn't implement any JSON token handling methods by itself. Usually proxy
/// parser stores a subparser as a field and maybe stores some housekeeping
/// settings for result handling.
///
/// Duck typing is used in proxy parsers to negate virtual methods overhead.
/// A proxy parser must implement the following methods:
///
/// ~~~~~~~~~{.cpp}
/// class ProxyParser final
///   : public formats::json::parser::Subscriber<Subparser::ResultType> {
///  public:
///   // Define result type that will be passed to OnSend() of a subscriber
///   using ResultType = Result;
///
///   ProxyParser() {
///     // Proxy parser wants to be called when the subparser
///     // signals with the result
///     subparser.Subscribe(*this);
///   }
///
///   // Reset() of proxy parser MUST call Reset() of subparsers in contrast to
///   // a typed parser as a proxy parser doesn't control pushing of subparser
///   // onto the stack.
///   void Reset() {
///     subparser_.Reset();
///   }
///
///   void Subscribe(formats::json::parser::Subscriber<Result>& subscriber) {
///     subscriber_ = &subscriber;
///   }
///
///   // The core method of proxy parser. It converts/filters the result value
///   // and signals the (maybe mutated) result further to its subscriber.
///   void OnSend(Subparser::ResultType&& result) override {
///     if (subscriber_) subscriber_->OnSend(Result(std::move(result)));
///   }
///
///   // Returns a typed parser that is responsible for actual JSON parsing.
///   auto& GetParser() { return subparser_.GetParser(); }
///
///  private:
///   Subparser subparser_;
///   Subscriber* subscriber_{nullptr};
/// }
/// ~~~~~~~~~
///
template <typename T>
class TypedParser : public BaseParser {
 public:
  void Subscribe(Subscriber<T>& subscriber) { subscriber_ = &subscriber; }

  using ResultType = T;

  /// Resets parser's internal state.
  /// It should not call Reset() of subparsers (if any).
  /// Subparsers' Reset() should be called just before pushing it onto the
  /// stack.
  virtual void Reset(){};

  /// Returns an actual parser.
  /// It is commonly used in PushParser() to identify typed parser
  /// of a proxy parser.
  TypedParser<T>& GetParser() { return *this; }

 protected:
  void SetResult(T&& value) {
    parser_state_->PopMe(*this);
    if (subscriber_) subscriber_->OnSend(std::move(value));
  }

 private:
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

USERVER_NAMESPACE_END
