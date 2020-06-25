#include <formats/json/parser/parser_state.hpp>

#include <variant>
#include <vector>

#include <boost/container/small_vector.hpp>

#include <rapidjson/error/en.h>
#include <rapidjson/reader.h>

#include <formats/json/parser/base_parser.hpp>
#include <formats/json/parser/parser_handler.hpp>
#include <logging/log.hpp>
#include <utils/assert.hpp>

namespace formats::json::parser {

struct ParserState::Impl {
  struct StackItem final {
    BaseParser* parser;

    size_t prev_stack_path_size;
  };

  // JSON in handlers is often 2-5 items in depth
  boost::container::small_vector<StackItem, 16> stack;

  // document path items, e.g. "field" for object field name
  // or "[2]" for array item number
  std::string stack_path;

  void PushParser(BaseParser& parser, std::string_view key,
                  ParserState& parser_state);
};

void ParserState::Impl::PushParser(BaseParser& parser, std::string_view key,
                                   ParserState& parser_state) {
  parser.SetState(parser_state);
  auto prev_stack_path_size = stack_path.size();

  if (!stack_path.empty()) {
    stack_path += '.';
  }
  if (!key.empty()) {
    stack_path += key;
  }
  stack.push_back({&parser, prev_stack_path_size});
}

ParserState::ParserState() : impl_() {
  // expectation of "common" path length
  impl_->stack_path.reserve(64);
}

ParserState::~ParserState() = default;

void ParserState::PushParserNoKey(BaseParser& parser) {
  PushParser(parser, std::string_view{""});
}

void ParserState::PushParser(BaseParser& parser, std::string_view key) {
  impl_->PushParser(parser, key, *this);
}

void ParserState::PushParser(BaseParser& parser, size_t key) {
  impl_->PushParser(parser, fmt::format("[{}]", key), *this);
}

void ParserState::ProcessInput(std::string_view sw) {
  rapidjson::Reader reader;
  rapidjson::MemoryStream is(sw.data(), sw.size());
  reader.IterativeParseInit();

  auto& stack = impl_->stack;

  try {
    while (!reader.IterativeParseComplete()) {
      if (stack.empty()) {
        throw InternalParseError("Symbols after end of document");
      }

      UASSERT(stack.back().parser);
      ParserHandler handler(*stack.back().parser);
      reader.IterativeParseNext<rapidjson::kParseDefaultFlags>(is, handler);

      if (reader.HasParseError()) {
        throw ParseError{
            reader.GetErrorOffset(),
            impl_->stack_path,
            rapidjson::GetParseError_En(reader.GetParseErrorCode()),
        };
      }
    }
  } catch (const ParseError&) {
    throw;
  } catch (const std::exception& e) {
    throw ParseError{
        is.Tell(),
        impl_->stack_path,
        e.what(),
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

  const auto& top = impl_->stack.back();
  impl_->stack_path.resize(top.prev_stack_path_size);

  impl_->stack.pop_back();
}

}  // namespace formats::json::parser
