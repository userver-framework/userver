#include <userver/formats/json/parser/parser_state.hpp>

#include <variant>
#include <vector>

#include <boost/container/small_vector.hpp>

#include <fmt/format.h>
#include <rapidjson/error/en.h>
#include <rapidjson/reader.h>

#include <userver/formats/common/path.hpp>
#include <userver/formats/json/parser/base_parser.hpp>
#include <userver/formats/json/parser/parser_handler.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json::parser {

namespace {

std::string ToLimited(std::string_view sw) {
  if (sw.size() > 128)
    return std::string(sw.substr(0, 128)) + "... (truncated)";
  else
    return std::string{sw};
}

}  // namespace

struct ParserState::Impl {
  struct StackItem final {
    BaseParser* parser;
  };

  // JSON in handlers is often 2-5 items in depth
  boost::container::small_vector<StackItem, 16> stack;

  void PushParser(BaseParser& parser, ParserState& parser_state);

  [[nodiscard]] std::string GetPath() const;
};

void ParserState::Impl::PushParser(BaseParser& parser,
                                   ParserState& parser_state) {
  parser.SetState(parser_state);
  stack.push_back({&parser});
}

std::string ParserState::Impl::GetPath() const {
  std::string result;

  for (const auto& item : stack) {
    const auto str = item.parser->GetPathItem();
    if (str.empty()) continue;

    if (!result.empty()) {
      result += '.';
    }
    result += str;
  }

  return result;
}

ParserState::ParserState() = default;

ParserState::~ParserState() = default;

void ParserState::PushParser(BaseParser& parser) {
  impl_->PushParser(parser, *this);
}

void ParserState::ProcessInput(std::string_view sw) {
  rapidjson::Reader reader;
  rapidjson::MemoryStream is(sw.data(), sw.size());
  reader.IterativeParseInit();

  auto& stack = impl_->stack;

  size_t pos = 0;
  try {
    while (!reader.IterativeParseComplete()) {
      if (stack.empty()) {
        throw InternalParseError("Symbols after end of document");
      }

      UASSERT(stack.back().parser);
      ParserHandler handler(*stack.back().parser);

      pos = is.Tell();
      reader.IterativeParseNext<rapidjson::kParseDefaultFlags>(is, handler);

      if (reader.HasParseError()) {
        throw ParseError{
            reader.GetErrorOffset(),
            impl_->GetPath(),
            rapidjson::GetParseError_En(reader.GetParseErrorCode()),
        };
      }
    }
  } catch (const ParseError&) {
    throw;
  } catch (const std::exception& e) {
    auto cur_pos = is.Tell();
    auto msg = (cur_pos == pos)
                   ? ""
                   : fmt::format(", the latest token was {}",
                                 ToLimited(sw.substr(pos, cur_pos - pos)));
    throw ParseError{
        cur_pos,
        impl_->GetPath(),
        e.what() + msg,
    };
  }

  if (!stack.empty()) {
    throw ParseError(is.Tell(), "", "data is expected after the end of file");
  }
}

BaseParser& ParserState::GetTopParser() const {
  UASSERT(!impl_->stack.empty());
  return *impl_->stack.back().parser;
}

void ParserState::PopMe([[maybe_unused]] BaseParser& parser) {
  UASSERT(!impl_->stack.empty());
  UASSERT(&parser == impl_->stack.back().parser);

  impl_->stack.pop_back();
}

}  // namespace formats::json::parser

USERVER_NAMESPACE_END
