#include <storages/postgres/io/type_mapping.hpp>

#include <algorithm>
#include <set>
#include <unordered_map>
#include <vector>

#include <logging/log.hpp>
#include <storages/postgres/exceptions.hpp>
#include <utils/algo.hpp>

namespace storages::postgres::io {

namespace {

using ParserList = std::unordered_multimap<PredefinedOids, std::string>;
using OidToOid = std::unordered_map<PredefinedOids, PredefinedOids>;

char const* const kVoidLiteral = "void";

ParserList& Parsers() {
  static ParserList parsers_{{PredefinedOids::kVoid, kVoidLiteral}};
  return parsers_;
}

OidToOid& ArrayToElement() {
  static OidToOid map_;
  return map_;
}

TypeBufferCategory& TypeCategories() {
  static TypeBufferCategory cats_{
      {static_cast<Oid>(PredefinedOids::kVoid), BufferCategory::kVoid}};
  return cats_;
}

const std::unordered_map<BufferCategory, std::string, BufferCategoryHash>
    kBufferCategoryToString{
        {BufferCategory::kKeepCategory, "parent buffer category"},
        {BufferCategory::kNoParser, "no parser"},
        {BufferCategory::kVoid, "void result"},
        {BufferCategory::kPlainBuffer, "plain buffer"},
        {BufferCategory::kArrayBuffer, "array buffer"},
        {BufferCategory::kCompositeBuffer, "composite buffer"},
        {BufferCategory::kRangeBuffer, "range buffer"},
    };

}  // namespace

const std::string& ToString(BufferCategory val) {
  if (auto f = kBufferCategoryToString.find(val);
      f != kBufferCategoryToString.end()) {
    return f->second;
  }
  throw LogicError("Invalid buffer category value " +
                   std::to_string(static_cast<int>(val)));
}

BufferCategory GetTypeBufferCategory(const TypeBufferCategory& categories,
                                     Oid type_oid) {
  auto cat = io::GetBufferCategory(static_cast<io::PredefinedOids>(type_oid));
  return cat != BufferCategory::kNoParser
             ? cat
             : ::utils::FindOrDefault(categories, type_oid,
                                      BufferCategory::kNoParser);
}

namespace detail {

RegisterPredefinedOidParser RegisterPredefinedOidParser::Register(
    PredefinedOids type_oid, PredefinedOids array_oid, BufferCategory category,
    std::string cpp_name) {
  ArrayToElement().insert(std::make_pair(array_oid, type_oid));
  // No use registering no category for a type - this is default.
  if (category != BufferCategory::kNoParser) {
    TypeCategories().insert(
        std::make_pair(static_cast<Oid>(type_oid), category));
    TypeCategories().insert(std::make_pair(static_cast<Oid>(array_oid),
                                           BufferCategory::kArrayBuffer));
  }
  Parsers().emplace(type_oid, cpp_name);
  Parsers().emplace(array_oid, std::move(cpp_name));
  return {};
}

}  // namespace detail

bool HasParser(PredefinedOids type_oid) {
  return Parsers().find(type_oid) != Parsers().end();
}

bool MappedToSameType(PredefinedOids lhs, PredefinedOids rhs) {
  auto lhs_range = Parsers().equal_range(lhs);
  if (lhs_range.first == lhs_range.second) {
    return false;
  }
  auto rhs_range = Parsers().equal_range(rhs);
  if (rhs_range.first == rhs_range.second) {
    return false;
  }
  std::set<std::string> lhs_types;
  std::transform(lhs_range.first, lhs_range.second,
                 std::inserter(lhs_types, lhs_types.begin()),
                 [](const auto& pair) { return pair.second; });
  for (auto p = rhs_range.first; p != rhs_range.second; ++p) {
    if (lhs_types.count(p->second)) {
      return true;
    }
  }
  return false;
}

BufferCategory GetBufferCategory(PredefinedOids oid) {
  const auto& cats = TypeCategories();
  if (auto f = cats.find(static_cast<Oid>(oid)); f != cats.end()) {
    return f->second;
  }
  return BufferCategory::kNoParser;
}

PredefinedOids GetArrayElementOid(PredefinedOids array_oid) {
  if (auto f = ArrayToElement().find(array_oid); f != ArrayToElement().end()) {
    return f->second;
  }
  return PredefinedOids::kInvalid;
}

void LogRegisteredTypes() {
  for (const auto& [pg_name, cpp_name] : Parsers()) {
    LOG_DEBUG() << fmt::format("pg type mapping: oid='{}' cpp='{}'", pg_name,
                               cpp_name);
  }
}

}  // namespace storages::postgres::io
