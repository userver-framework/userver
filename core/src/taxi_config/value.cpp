#include <taxi_config/value.hpp>

#include <type_traits>

#include <formats/json/serialize.hpp>
#include <formats/json/value_builder.hpp>
#include <logging/logger.hpp>

namespace taxi_config {

formats::json::Value DocsMap::Get(const std::string& name) const {
  const auto& mongo_it = docs_.find(name);
  if (mongo_it == docs_.end())
    throw std::runtime_error("Can't find doc for " + name);

  const auto& doc = mongo_it->second;
  auto value = doc["v"];
  if (value.IsMissing())
    throw std::runtime_error("Mongo config have no element 'v' for " + name);

  requested_names_.insert(name);

  return value;
}

void DocsMap::Set(std::string name, formats::json::Value obj) {
  auto it = docs_.find(name);
  if (it != docs_.end()) {
    it->second = obj;
  } else {
    docs_.emplace(std::move(name), std::move(obj));
  }
}

void DocsMap::Parse(const std::string& json_string, bool empty_ok) {
  auto json = formats::json::FromString(json_string);

  if (!empty_ok && json.GetSize() == 0)
    throw std::runtime_error("DocsMap::Parse failed: json is empty");

  for (auto it = json.begin(); it != json.end(); ++it) {
    auto name = it.GetName();
    formats::json::ValueBuilder builder;

    /* Use fake [name] magic to pass the json path into DocsMap
     * to ease debugging of bad default value
     */
    builder[name]["v"] = *it;
    Set(name, builder.ExtractValue()[name]);
  }
}

size_t DocsMap::Size() const { return docs_.size(); }

void DocsMap::MergeFromOther(DocsMap&& other) {
  // TODO: do docs_.merge(other) after we get C++17 std lib
  for (auto& it : other.docs_) {
    std::string name = it.first;
    formats::json::Value value = it.second;

    auto this_it = docs_.find(name);
    if (this_it == docs_.end()) {
      docs_.emplace(std::move(name), std::move(value));
    } else {
      this_it->second = value;
    }
  }
}

std::vector<std::string> DocsMap::GetRequestedNames() const {
  return std::vector<std::string>(requested_names_.begin(),
                                  requested_names_.end());
}

std::string DocsMap::AsJsonString() const {
  formats::json::ValueBuilder body_builder(formats::json::Type::kObject);

  for (auto& it : docs_) {
    body_builder[it.first] = it.second["v"];
  }

  return formats::json::ToString(body_builder.ExtractValue());
}

const std::string kValueDictDefaultName = "__default__";

}  // namespace taxi_config
