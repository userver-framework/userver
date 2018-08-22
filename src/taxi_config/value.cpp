#include <taxi_config/value.hpp>

#include <type_traits>

#include <mongo/db/json.h>

namespace taxi_config {
namespace impl {

template <>
std::string MongoCast<std::string>(const ::mongo::BSONElement& elem) {
  return storages::mongo::ToString(elem);
}

template <>
std::unordered_set<std::string> MongoCast<std::unordered_set<std::string>>(
    const ::mongo::BSONElement& elem) {
  std::unordered_set<std::string> response;
  for (const auto& subitem : storages::mongo::ToArray(elem)) {
    response.emplace(storages::mongo::ToString(subitem));
  }
  return response;
}

template <>
bool MongoCast<bool>(const ::mongo::BSONElement& elem) {
  return storages::mongo::ToBool(elem);
}

}  // namespace impl

mongo::BSONElement DocsMap::Get(const std::string& name) const {
  const auto& mongo_it = docs_.find(name);
  if (mongo_it == docs_.end())
    throw std::runtime_error("Can't find doc for " + name);

  const auto& doc = mongo_it->second;
  if (!doc.hasElement("v"))
    throw std::runtime_error("Mongo config have no element 'v' for " + name);

  auto element = doc["v"];
  if (!element.ok())
    throw std::runtime_error("Mongo element is not ok for " + name);

  return element;
}

void DocsMap::Set(std::string name, mongo::BSONObj obj) {
  docs_[std::move(name)] = std::move(obj);
}

void DocsMap::Parse(const std::string& json) {
  const mongo::BSONObj bson = mongo::fromjson(json);

  if (!bson.isValid())
    throw std::runtime_error("DocsMap::Parse failed: invalid BSON");

  if (bson.isEmpty())
    throw std::runtime_error("DocsMap::Parse failed: BSON is empty");

  std::vector<mongo::BSONElement> elements;
  bson.elems(elements);

  if (elements.empty())
    throw std::runtime_error("DocsMap::Parse failed: elements are empty");

  for (auto const& element : elements) {
    mongo::BSONObjBuilder builder;
    builder.appendAs(element, "v");
    Set(element.fieldName(), builder.obj());
  }
}

}  // namespace taxi_config
