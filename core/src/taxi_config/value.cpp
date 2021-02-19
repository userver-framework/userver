#include <taxi_config/value.hpp>

#include <type_traits>

#include <formats/json/serialize.hpp>
#include <formats/json/value_builder.hpp>
#include <logging/logger.hpp>

namespace taxi_config {

formats::json::Value DocsMap::Get(const std::string& name) const {
  const auto it = docs_.find(name);
  if (it == docs_.end()) {
    throw std::runtime_error("Can't find doc for '" + name + "'");
  }

  requested_names_.insert(name);
  return it->second;
}

void DocsMap::Set(std::string name, formats::json::Value obj) {
  // TODO: remove that logic
  if (obj.IsObject() && obj.GetSize() == 1 && !obj["v"].IsMissing()) {
    obj = obj["v"];
  }

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

  for (const auto& [name, value] : Items(json)) {
    formats::json::ValueBuilder builder;

    /* Use fake [name] magic to pass the json path into DocsMap
     * to ease debugging of bad default value
     */
    builder[name] = value;
    Set(name, builder.ExtractValue()[name]);
  }
}

size_t DocsMap::Size() const { return docs_.size(); }

void DocsMap::MergeFromOther(DocsMap&& other) {
  auto new_docs = std::move(other.docs_);
  new_docs.merge(std::move(docs_));
  docs_ = std::move(new_docs);
}

const std::unordered_set<std::string>& DocsMap::GetRequestedNames() const {
  return requested_names_;
}

std::unordered_set<std::string> DocsMap::GetNames() const {
  std::unordered_set<std::string> names;
  for (const auto& [k, v] : docs_) names.insert(k);
  return names;
}

std::string DocsMap::AsJsonString() const {
  formats::json::ValueBuilder body_builder(formats::json::Type::kObject);

  for (auto& it : docs_) {
    body_builder[it.first] = it.second;
  }

  return formats::json::ToString(body_builder.ExtractValue());
}

const std::string kValueDictDefaultName = "__default__";

}  // namespace taxi_config
