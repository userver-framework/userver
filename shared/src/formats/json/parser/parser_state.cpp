#include <formats/json/parser/parser_state.hpp>

#include <variant>
#include <vector>

#include <boost/container/small_vector.hpp>

#include <rapidjson/error/en.h>
#include <rapidjson/reader.h>

#include <formats/json/parser/base_parser.hpp>
#include <formats/json/parser/parser_handler.hpp>
#include <utils/assert.hpp>

namespace formats::json::parser {

struct ParserState::Impl {
  struct StackItem final {
    BaseParser* parser;

    // document path item, e.g. "field" for object field name
    // or 1 for array item number
    std::variant<size_t, std::string_view> path_item;
  };

  // JSON in handlers is often 2-5 items in depth
  boost::container::small_vector<StackItem, 16> stack;
  // TODO: manage path using std::visit in step
};

ParserState::ParserState() : impl_() {}

ParserState::~ParserState() = default;

void ParserState::PushParserNoKey(BaseParser& parser) {
  PushParser(parser, "");
}

void ParserState::PushParser(BaseParser& parser, std::string_view key) {
  parser.SetState(*this);
  impl_->stack.push_back({&parser, key});
}

void ParserState::PushParser(BaseParser& parser, size_t key) {
  parser.SetState(*this);
  impl_->stack.push_back({&parser, key});
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
            GetCurrentPath(),
            rapidjson::GetParseError_En(reader.GetParseErrorCode()),
        };
      }
    }
  } catch (const ParseError&) {
    throw;
  } catch (const std::exception& e) {
    throw ParseError{
        is.Tell(),
        GetCurrentPath(),
        e.what(),
    };
  }

  if (!stack.empty()) {
    throw ParseError(is.Tell(), "", "data is expected after the end of file");
  }
}

namespace {
struct PathItemVisitor final {
  std::string operator()(std::string_view sw) { return std::string{sw}; }
  std::string operator()(size_t i) { return fmt::format("[{}]", i); }
};
}  // namespace

std::string ParserState::GetCurrentPath() const {
  std::string path;
  for (const auto& [_parser, item] : impl_->stack) {
    if (!path.empty()) path += '.';
    path += std::visit(PathItemVisitor(), item);
  }
  return path;
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
