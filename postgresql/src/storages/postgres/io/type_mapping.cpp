#include <userver/storages/postgres/io/type_mapping.hpp>

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <fmt/format.h>
#include <boost/functional/hash.hpp>

#include <userver/logging/log.hpp>
#include <userver/storages/postgres/exceptions.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/underlying_value.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::io {

namespace {

struct PredefinedOidsPair {
  PredefinedOidsPair(PredefinedOids first_, PredefinedOids second_)
      : first(std::min(first_, second_)), second(std::max(first_, second_)) {}

  PredefinedOids first;
  PredefinedOids second;
};

bool operator==(const PredefinedOidsPair& lhs, const PredefinedOidsPair& rhs) {
  return lhs.first == rhs.first && lhs.second == rhs.second;
}

struct PredefinedOidsPairHash {
  size_t operator()(const PredefinedOidsPair& value) const {
    size_t result = 0;
    boost::hash_combine(result,
                        USERVER_NAMESPACE::utils::UnderlyingValue(value.first));
    boost::hash_combine(
        result, USERVER_NAMESPACE::utils::UnderlyingValue(value.second));
    return result;
  }
};

class ParserOidsRegistry {
 public:
  bool HasParser(PredefinedOids oid) const {
    return registered_oids_.find(oid) != registered_oids_.end();
  }

  bool AreMappedToSameType(PredefinedOids first, PredefinedOids second) const {
    if (first == second) return true;
    return shared_parser_oids_.find(PredefinedOidsPair{first, second}) !=
           shared_parser_oids_.end();
  }

  void LogRegisteredTypes() const {
    for (const auto& [cpp_name, oid] : oids_by_type_) {
      LOG_DEBUG() << fmt::format("pg type mapping: oid='{}' cpp='{}'",
                                 static_cast<int>(oid), cpp_name);
    }
  }

  void Add(std::string&& cpp_name, PredefinedOids oid) {
    registered_oids_.insert(oid);
    const auto shared_oids = oids_by_type_.equal_range(cpp_name);
    for (auto it = shared_oids.first; it != shared_oids.second; ++it) {
      shared_parser_oids_.emplace(it->second, oid);
    }
    oids_by_type_.emplace(std::move(cpp_name), oid);
  }

 private:
  std::unordered_set<PredefinedOids> registered_oids_;
  std::unordered_set<PredefinedOidsPair, PredefinedOidsPairHash>
      shared_parser_oids_;
  std::unordered_multimap<std::string, PredefinedOids> oids_by_type_;
};

auto& Registry() {
  static auto registry_ = [] {
    ParserOidsRegistry registry;
    registry.Add("void", PredefinedOids::kVoid);
    return registry;
  }();
  return registry_;
}

auto& ArrayToElement() {
  static std::unordered_map<PredefinedOids, PredefinedOids> element_by_array_;
  return element_by_array_;
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
  throw LogicError(fmt::format("Invalid buffer category value {}",
                               USERVER_NAMESPACE::utils::UnderlyingValue(val)));
}

BufferCategory GetTypeBufferCategory(const TypeBufferCategory& categories,
                                     Oid type_oid) {
  auto cat = io::GetBufferCategory(static_cast<io::PredefinedOids>(type_oid));
  return cat != BufferCategory::kNoParser
             ? cat
             : USERVER_NAMESPACE::utils::FindOrDefault(
                   categories, type_oid, BufferCategory::kNoParser);
}

namespace detail {

RegisterPredefinedOidParser RegisterPredefinedOidParser::Register(
    PredefinedOids type_oid, PredefinedOids array_oid, BufferCategory category,
    std::string cpp_name) {
  ArrayToElement().emplace(array_oid, type_oid);
  // No use registering no category for a type - this is default.
  if (category != BufferCategory::kNoParser) {
    TypeCategories().emplace(static_cast<Oid>(type_oid), category);
    TypeCategories().emplace(static_cast<Oid>(array_oid),
                             BufferCategory::kArrayBuffer);
  }
  Registry().Add(fmt::format("{}[]", cpp_name), array_oid);
  Registry().Add(std::move(cpp_name), type_oid);
  return {};
}

}  // namespace detail

bool HasParser(PredefinedOids type_oid) {
  return Registry().HasParser(type_oid);
}

bool MappedToSameType(PredefinedOids lhs, PredefinedOids rhs) {
  return Registry().AreMappedToSameType(lhs, rhs);
}

BufferCategory GetBufferCategory(PredefinedOids oid) {
  return USERVER_NAMESPACE::utils::FindOrDefault(
      TypeCategories(), static_cast<Oid>(oid), BufferCategory::kNoParser);
}

PredefinedOids GetArrayElementOid(PredefinedOids array_oid) {
  return USERVER_NAMESPACE::utils::FindOrDefault(ArrayToElement(), array_oid,
                                                 PredefinedOids::kInvalid);
}

void LogRegisteredTypes() { Registry().LogRegisteredTypes(); }

}  // namespace storages::postgres::io

USERVER_NAMESPACE_END
