#include <gtest/gtest.h>

#include <formats/json/parser/parser.hpp>

// TODO: move to utest/*
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

using namespace formats::json::parser;

TEST(JsonStringParser, Int64) {
  std::string input{"12345"};

  int64_t result{0};
  IntParser int_parser;
  int_parser.Reset(result);

  ParserState state;
  state.PushParserNoKey(int_parser);
  state.ProcessInput(input);

  EXPECT_EQ(result, 12345);
}

TEST(JsonStringParser, Int64Overflow) {
  std::string input{std::to_string(-1ULL)};

  EXPECT_THROW_TEXT((ParseToType<int64_t, IntParser>(input)), ParseError,
                    "Parse error at pos 20, path '': bad "
                    "numeric conversion: positive overflow");
}

TEST(JsonStringParser, Int64MinMax) {
  std::string input{"12345"};

  int64_t result{0};
  auto validator = MakeValidator<int64_t>([](int64_t value) {
    if (value > 5) {
      throw InternalParseError(
          fmt::format("max value ({}) < value (({})", 5, value));
    }
  });
  IntParser int_parser(validator);
  int_parser.Reset(result);

  ParserState state;
  state.PushParserNoKey(int_parser);
  EXPECT_THROW_TEXT(
      state.ProcessInput(input), ParseError,
      "Parse error at pos 5, path '': max value (5) < value ((12345)");
}

class EmptyObjectParser final : public BaseParser {
 protected:
  void StartObject() override {}

  void EndObject() override { parser_state_->PopMe(*this); }

  std::string Expected() const override { return "'}'"; }
};

TEST(JsonStringParser, EmptyObject) {
  std::string input{"{}"};

  EmptyObjectParser obj_parser;

  ParserState state;
  state.PushParserNoKey(obj_parser);
  EXPECT_NO_THROW(state.ProcessInput(input));
}

TEST(JsonStringParser, EmptyObjectKey) {
  std::string input{"{\"key\":1}"};

  EmptyObjectParser obj_parser;

  ParserState state;
  state.PushParserNoKey(obj_parser);
  EXPECT_THROW_TEXT(state.ProcessInput(input), ParseError,
                    "Parse error at pos 6, path '': '}' was "
                    "expected, but field 'key' found");
}

struct IntObject final {
  int64_t field;
};

bool operator==(const IntObject& lh, const IntObject& rh) {
  return lh.field == rh.field;
}

class IntObjectParser final : public BaseParser {
 public:
  void Reset(IntObject& obj) {
    result_ = &obj;
    has_field_ = false;
  }

 protected:
  void StartObject() override {}

  void Key(std::string_view key) override {
    if (key == "field") {
      has_field_ = true;
      field_parser_.Reset(result_->field);
      parser_state_->PushParser(field_parser_, key);
    } else {
      throw InternalParseError("Bad field for IntObject ('" + std::string(key) +
                               ")");
    }
  }

  void EndObject() override {
    if (!has_field_) {
      throw InternalParseError("Missing required field 'field'");
    }
    parser_state_->PopMe(*this);
  }

  // Note: not strictly correct
  std::string Expected() const override { return "'{'"; }

 private:
  IntParser field_parser_;
  IntObject* result_{nullptr};
  bool has_field_;
};

TEST(JsonStringParser, IntObject) {
  std::string input("{\"field\": 234}");
  EXPECT_EQ((ParseToType<IntObject, IntObjectParser>(input)), IntObject({234}));
}

TEST(JsonStringParser, IntObjectNoField) {
  std::string input("{}");

  EXPECT_THROW_TEXT(
      (ParseToType<IntObject, IntObjectParser>(input)), ParseError,
      "Parse error at pos 1, path '': Missing required field 'field'");
}

TEST(JsonStringParser, ArrayIntObjectNoField) {
  std::string input("[{}]");

  IntObjectParser obj_parser;
  ArrayParser<IntObject, IntObjectParser> array_parser(obj_parser);

  std::vector<IntObject> result;
  array_parser.Reset(result);
  ParserState state;
  state.PushParserNoKey(array_parser);

  EXPECT_THROW_TEXT(
      state.ProcessInput(input), ParseError,
      "Parse error at pos 2, path '[0]': Missing required field 'field'");
}

TEST(JsonStringParser, ArrayInt) {
  std::string input("[1,2,3]");
  std::vector<int64_t> result{};

  IntParser int_parser;
  ArrayParser<int64_t, IntParser> parser(int_parser);
  parser.Reset(result);

  ParserState state;
  state.PushParserNoKey(parser);
  state.ProcessInput(input);
  EXPECT_EQ(result, (std::vector<int64_t>{1, 2, 3}));
}

TEST(JsonStringParser, ArrayIntLimit) {
  IntParser int_parser;
  auto validator = MakeValidator<std::vector<int64_t>>([](auto value) {
    const auto size = value.size();
    if (size > 2 || size < 1) {
      throw InternalParseError("");
    }
  });
  ArrayParser<int64_t, IntParser> parser(int_parser, validator);

  for (int i = 0; i < 4; i++) {
    std::string input{"["};
    for (int j = 0; j < i; j++) {
      if (j > 0) input += ',';
      input += "1";
    }
    input += "]";

    std::vector<int64_t> result{};
    parser.Reset(result);

    ParserState state;
    state.PushParserNoKey(parser);
    if (i == 0 || i == 3) {
      EXPECT_THROW(state.ProcessInput(input), ParseError) << "length " << i;
    } else {
      EXPECT_NO_THROW(state.ProcessInput(input));
      EXPECT_EQ(result.size(), i);
      for (size_t j = 0; j < result.size(); j++)
        EXPECT_EQ(result[j], 1) << "item " << j;
    }
  }
}

TEST(JsonStringParser, ArrayArrayInt) {
  std::string input("[[1],[],[2,3,4]]");
  std::vector<std::vector<int64_t>> result{};

  IntParser int_parser;
  using Subparser = ArrayParser<int64_t, IntParser>;
  Subparser subparser(int_parser);
  ArrayParser<std::vector<int64_t>, Subparser> parser(subparser);
  parser.Reset(result);

  ParserState state;
  state.PushParserNoKey(parser);
  state.ProcessInput(input);
  EXPECT_EQ(result, (std::vector<std::vector<int64_t>>{{1}, {}, {2, 3, 4}}));
}

TEST(JsonStringParser, Enum) {
  enum class Enum {
    kFirst,
    kSecond,
  };

  Enum enum_result;
  std::string result;

  auto validator =
      MakeValidator<std::string>([&enum_result](const auto& value) {
        if (value == "first")
          enum_result = Enum::kFirst;
        else if (value == "second")
          enum_result = Enum::kSecond;
        else
          throw InternalParseError("");
      });
  StringParser parser(validator);
  parser.Reset(result);

  ParserState state;
  state.PushParserNoKey(parser);

  std::string input("\"first\"");
  state.ProcessInput(input);
  EXPECT_EQ(result, "first");
  EXPECT_EQ(enum_result, Enum::kFirst);
}

enum class TestEnum {
  kFirst,
  kSecond,
};

TestEnum Parse(const std::string&, ::formats::parse::To<TestEnum>) {
  return TestEnum::kFirst;
}

TEST(JsonStringParser, EnumConverter) {
  TestEnum enum_result;

  ParserConverter<TestEnum, StringParser> converter;
  converter.Reset(enum_result);

  ParserState state;
  state.PushParserNoKey(converter);

  std::string input("\"first\"");
  state.ProcessInput(input);
  EXPECT_EQ(enum_result, TestEnum::kFirst);
}
