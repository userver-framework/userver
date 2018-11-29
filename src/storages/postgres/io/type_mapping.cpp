#include <storages/postgres/io/type_mapping.hpp>

#include <unordered_map>
#include <vector>

namespace storages::postgres::io {

namespace {

using ParserList = std::unordered_multimap<PredefinedOids, std::string>;

ParserList& BinaryParsers() {
  static ParserList parsers_;
  return parsers_;
}

ParserList& TextParsers() {
  static ParserList parsers_;
  return parsers_;
}

}  // namespace

namespace detail {

RegisterPredefinedOidParser RegisterPredefinedOidParser::Register(
    PredefinedOids type_oid, PredefinedOids array_oid, std::string&& cpp_name,
    bool text_parser, bool bin_parser) {
  auto both = text_parser && bin_parser;
  if (both) {
    TextParsers().insert(std::make_pair(type_oid, cpp_name));
    TextParsers().insert(std::make_pair(array_oid, cpp_name));
    BinaryParsers().insert(std::make_pair(type_oid, cpp_name));
    BinaryParsers().insert(std::make_pair(array_oid, std::move(cpp_name)));
  } else if (text_parser) {
    TextParsers().insert(std::make_pair(type_oid, cpp_name));
    TextParsers().insert(std::make_pair(array_oid, std::move(cpp_name)));
  } else if (bin_parser) {
    BinaryParsers().insert(std::make_pair(type_oid, cpp_name));
    BinaryParsers().insert(std::make_pair(array_oid, std::move(cpp_name)));
  }
  return {};
}

}  // namespace detail

bool HasTextParser(PredefinedOids type_oid) {
  return TextParsers().count(type_oid) > 0;
}

bool HasBinaryParser(PredefinedOids type_oid) {
  return BinaryParsers().count(type_oid) > 0;
}

}  // namespace storages::postgres::io
