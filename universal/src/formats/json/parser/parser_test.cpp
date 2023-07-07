#include <gtest/gtest.h>

#include <unordered_map>

#include <userver/formats/json/parser/parser.hpp>
#include <userver/formats/json/serialize.hpp>

// TODO: move to utest/*
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define EXPECT_THROW_TEXT(code, exception_type, exc_text)                    \
  try {                                                                      \
    code;                                                                    \
    FAIL() << "expected exception " #exception_type ", but not thrown";      \
  } catch (const exception_type& e) {                                        \
    EXPECT_EQ(e.what(), std::string{exc_text}) << "wrong exception message"; \
  } catch (const std::exception& e) {                                        \
    FAIL() << "wrong exception type, expected " #exception_type ", but got " \
           << typeid(e).name();                                              \
  }

USERVER_NAMESPACE_BEGIN
namespace fjp = formats::json::parser;

TEST(JsonStringParser, Int64) {
  std::string input{"12345"};

  int64_t result{0};
  fjp::Int64Parser int_parser;
  fjp::SubscriberSink<int64_t> sink(result);
  int_parser.Subscribe(sink);
  int_parser.Reset();

  fjp::ParserState state;
  state.PushParser(int_parser);
  state.ProcessInput(input);

  EXPECT_EQ(result, 12345);

  EXPECT_EQ((fjp::ParseToType<int, fjp::IntParser>("3.0")), 3);

  EXPECT_EQ((fjp::ParseToType<int, fjp::IntParser>("0.0")), 0);
  EXPECT_EQ((fjp::ParseToType<int, fjp::IntParser>("0")), 0);

  EXPECT_EQ((fjp::ParseToType<int, fjp::IntParser>("-3.0")), -3);
  EXPECT_EQ((fjp::ParseToType<int, fjp::IntParser>("-3")), -3);
  EXPECT_EQ((fjp::ParseToType<int, fjp::IntParser>("-1192.0")), -1192);

  EXPECT_THROW_TEXT((fjp::ParseToType<int, fjp::IntParser>("3.01")),
                    fjp::ParseError,
                    "Parse error at pos 4, path '': integer was expected, but "
                    "double found, the latest token was 3.01");
}

TEST(JsonStringParser, Double) {
  EXPECT_DOUBLE_EQ((fjp::ParseToType<double, fjp::DoubleParser>("1.23")), 1.23);
  EXPECT_DOUBLE_EQ((fjp::ParseToType<double, fjp::DoubleParser>("-20")), -20.0);
  EXPECT_DOUBLE_EQ((fjp::ParseToType<double, fjp::DoubleParser>("0")), 0);
  EXPECT_DOUBLE_EQ((fjp::ParseToType<double, fjp::DoubleParser>("123.456")),
                   123.456);

  EXPECT_THROW_TEXT((fjp::ParseToType<double, fjp::DoubleParser>("123.456a")),
                    fjp::ParseError,
                    "Parse error at pos 7, path '': The document root must not "
                    "be followed by other values.");
  EXPECT_THROW_TEXT(
      (fjp::ParseToType<double, fjp::DoubleParser>("[]")), fjp::ParseError,
      "Parse error at pos 0, path '': number was expected, but array found");
  EXPECT_THROW_TEXT(
      (fjp::ParseToType<double, fjp::DoubleParser>("{}")), fjp::ParseError,
      "Parse error at pos 0, path '': number was expected, but object found");
}

TEST(JsonStringParser, Int64Overflow) {
  std::string input{std::to_string(-1ULL)};

  EXPECT_THROW_TEXT((fjp::ParseToType<int64_t, fjp::Int64Parser>(input)),
                    fjp::ParseError,
                    "Parse error at pos 20, path '': bad "
                    "numeric conversion: positive overflow, the latest token "
                    "was 18446744073709551615");
}

class EmptyObjectParser final : public fjp::BaseParser {
 protected:
  void StartObject() override {}

  void EndObject() override { parser_state_->PopMe(*this); }

  std::string Expected() const override { return "'}'"; }
  std::string GetPathItem() const override { return {}; }
};

TEST(JsonStringParser, EmptyObject) {
  std::string input{"{}"};

  EmptyObjectParser obj_parser;

  fjp::ParserState state;
  state.PushParser(obj_parser);
  EXPECT_NO_THROW(state.ProcessInput(input));
}

TEST(JsonStringParser, EmptyObjectKey) {
  std::string input{"{\"key\":1}"};

  EmptyObjectParser obj_parser;

  fjp::ParserState state;
  state.PushParser(obj_parser);
  EXPECT_THROW_TEXT(
      state.ProcessInput(input), fjp::ParseError,
      "Parse error at pos 6, path '': '}' was "
      "expected, but field 'key' found, the latest token was \"key\"");
}

struct IntObject final {
  int64_t field{0};
};

bool operator==(const IntObject& lh, const IntObject& rh) {
  return lh.field == rh.field;
}

class IntObjectParser final : public fjp::TypedParser<IntObject> {
 public:
  void Reset() override { has_field_ = false; }

 protected:
  void StartObject() override {}

  void Key(std::string_view key) override {
    if (key == "field") {
      key_ = key;
      has_field_ = true;
      field_parser_.Reset();
      field_parser_.Subscribe(field_sink_);
      parser_state_->PushParser(field_parser_);
    } else {
      throw fjp::InternalParseError("Bad field for IntObject ('" +
                                    std::string(key) + ")");
    }
  }

  void EndObject() override {
    if (!has_field_) {
      throw fjp::InternalParseError("Missing required field 'field'");
    }
    SetResult(std::move(result_));
  }

  // Note: not strictly correct
  std::string Expected() const override { return "'{'"; }

  std::string GetPathItem() const override { return key_; }

 private:
  IntObject result_;
  fjp::Int64Parser field_parser_;
  std::string key_;
  fjp::SubscriberSink<int64_t> field_sink_{result_.field};
  bool has_field_{false};
};

TEST(JsonStringParser, IntObject) {
  std::string input("{\"field\": 234}");
  EXPECT_EQ((fjp::ParseToType<IntObject, IntObjectParser>(input)),
            IntObject({234}));
}

TEST(JsonStringParser, IntObjectNoField) {
  std::string input("{}");

  EXPECT_THROW_TEXT(
      (fjp::ParseToType<IntObject, IntObjectParser>(input)), fjp::ParseError,
      "Parse error at pos 1, path '': Missing required field 'field'");
}

TEST(JsonStringParser, ArrayIntObjectNoField) {
  std::string input("[{}]");

  IntObjectParser obj_parser;
  fjp::ArrayParser<IntObject, IntObjectParser> array_parser(obj_parser);

  std::vector<IntObject> result;
  fjp::SubscriberSink<decltype(result)> sink(result);
  array_parser.Reset();
  array_parser.Subscribe(sink);
  fjp::ParserState state;
  state.PushParser(array_parser);

  EXPECT_THROW_TEXT(
      state.ProcessInput(input), fjp::ParseError,
      "Parse error at pos 2, path '[0]': Missing required field 'field'");
}

TEST(JsonStringParser, ArrayIntErrorMsg) {
  fjp::IntParser obj_parser;
  fjp::ArrayParser<int, fjp::IntParser> array_parser(obj_parser);

  std::vector<int> result;
  fjp::SubscriberSink<decltype(result)> sink(result);
  array_parser.Reset();
  array_parser.Subscribe(sink);
  fjp::ParserState state;
  state.PushParser(array_parser);

  EXPECT_THROW_TEXT(state.ProcessInput("1"), fjp::ParseError,
                    "Parse error at pos 1, path '': array was expected, but "
                    "integer found, the latest token was 1");
}

TEST(JsonStringParser, ArrayInt) {
  std::string input("[1,2,3]");
  std::vector<int64_t> result{};

  fjp::Int64Parser int_parser;
  fjp::ArrayParser<int64_t, fjp::Int64Parser> parser(int_parser);
  fjp::SubscriberSink<decltype(result)> sink(result);
  parser.Reset();
  parser.Subscribe(sink);

  fjp::ParserState state;
  state.PushParser(parser);
  state.ProcessInput(input);
  EXPECT_EQ(result, (std::vector<int64_t>{1, 2, 3}));
}

TEST(JsonStringParser, ArrayArrayInt) {
  std::string input("[[1],[],[2,3,4]]");
  std::vector<std::vector<int64_t>> result{};

  fjp::Int64Parser int_parser;
  using Subparser = fjp::ArrayParser<int64_t, fjp::Int64Parser>;
  Subparser subparser(int_parser);
  fjp::ArrayParser<std::vector<int64_t>, Subparser> parser(subparser);
  fjp::SubscriberSink<decltype(result)> sink(result);
  parser.Reset();
  parser.Subscribe(sink);

  fjp::ParserState state;
  state.PushParser(parser);
  state.ProcessInput(input);
  EXPECT_EQ(result, (std::vector<std::vector<int64_t>>{{1}, {}, {2, 3, 4}}));
}

TEST(JsonStringParser, ArrayBool) {
  std::string input{"[true, false, true]"};
  std::vector<bool> result;

  fjp::BoolParser bool_parser;
  fjp::ArrayParser<bool, fjp::BoolParser> parser(bool_parser);
  fjp::SubscriberSink<decltype(result)> sink(result);
  parser.Reset();
  parser.Subscribe(sink);

  fjp::ParserState state;
  state.PushParser(parser);
  state.ProcessInput(input);
  EXPECT_EQ(result, (std::vector<bool>{true, false, true}));
}

template <class T>
struct JsonStringParserMap : public ::testing::Test {
  using Map = T;
};

using Maps = ::testing::Types<std::map<std::string, int>,
                              std::unordered_map<std::string, int>>;
TYPED_TEST_SUITE(JsonStringParserMap, Maps);

TYPED_TEST(JsonStringParserMap, Map) {
  using Map = typename TestFixture::Map;

  fjp::IntParser int_parser;
  fjp::MapParser<Map, fjp::IntParser> parser{int_parser};

  Map result;
  fjp::SubscriberSink<decltype(result)> sink(result);
  parser.Reset();
  parser.Subscribe(sink);
  fjp::ParserState state;
  state.PushParser(parser);
  state.ProcessInput(R"({"key": 1, "other": 3})");
  EXPECT_EQ(result, (Map{{"key", 1}, {"other", 3}}));
}

TYPED_TEST(JsonStringParserMap, Empty) {
  using Map = typename TestFixture::Map;

  fjp::IntParser int_parser;
  fjp::MapParser<Map, fjp::IntParser> parser{int_parser};

  Map result;
  fjp::SubscriberSink<decltype(result)> sink(result);
  parser.Reset();
  parser.Subscribe(sink);
  fjp::ParserState state;
  state.PushParser(parser);
  state.ProcessInput(R"({})");
  EXPECT_EQ(result, (Map{}));
}

TYPED_TEST(JsonStringParserMap, Invalid) {
  using Map = typename TestFixture::Map;

  fjp::IntParser int_parser;
  fjp::MapParser<Map, fjp::IntParser> parser{int_parser};

  Map result;
  fjp::SubscriberSink<decltype(result)> sink(result);
  parser.Reset();
  parser.Subscribe(sink);
  fjp::ParserState state;
  state.PushParser(parser);

  EXPECT_THROW_TEXT(state.ProcessInput(R"(123)"), fjp::ParseError,
                    "Parse error at pos 3, path '': object was expected, but "
                    "integer found, the latest token was 123");

  EXPECT_THROW_TEXT(
      state.ProcessInput(R"({{"key": 1}})"), fjp::ParseError,
      "Parse error at pos 1, path '': Missing a name for object member.");

  EXPECT_THROW_TEXT(state.ProcessInput(R"(}{)"), fjp::ParseError,
                    "Parse error at pos 0, path '': The document is empty.");
}

TEST(JsonStringParser, JsonValue) {
  std::string inputs[] = {
      R"([1, "123", "", -2, 3.5, {"key": 1, "other": {"key2": 2}}, {}])",
      R"({})",
  };
  for (const auto& input : inputs) {
    auto value_str = formats::json::FromString(input);
    auto value_sax =
        fjp::ParseToType<formats::json::Value, fjp::JsonValueParser>(input);
    EXPECT_EQ(value_str, value_sax) << "input: " + input + ", str='" +
                                           ToString(value_str) + "', sax='" +
                                           ToString(value_sax) + "'";
  }
}

TEST(JsonStringParser, JsonValueBad) {
  std::string inputs[] = {
      R"({)",         //
      R"()",          //
      R"({}})",       //
      R"(})",         //
      R"({"key")",    //
      R"({"key)",     //
      R"({"key":1)",  //
      R"([)",         //
      R"(1 2)",       //
  };
  for (const auto& input : inputs) {
    EXPECT_THROW(
        (fjp::ParseToType<formats::json::Value, fjp::JsonValueParser>(input)),
        fjp::ParseError);
  }
}

TEST(JsonStringParser, BomSymbol) {
  std::string input =
      "{\r\n\"track_id\": \"0000436301831\",\r\n\"service\": "
      "\"boxberry\",\r\n\"status\": \"pickedup\"\r\n}";
  auto value_str = formats::json::FromString(input);
  auto value_sax =
      fjp::ParseToType<formats::json::Value, fjp::JsonValueParser>(input);
  EXPECT_EQ(value_str, value_sax);
}

USERVER_NAMESPACE_END
