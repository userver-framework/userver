#include <userver/dynamic_config/value.hpp>

#include <stdexcept>

#include <fmt/format.h>

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <utils/internal_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config {

formats::json::Value DocsMap::Get(std::string_view name) const {
  const auto it = utils::impl::FindTransparent(docs_, name);
  if (it == docs_.end()) {
    throw std::runtime_error(fmt::format("Can't find doc for '{}'", name));
  }

  const auto used_it = utils::impl::FindTransparent(configs_to_be_used_, name);

  if (used_it != configs_to_be_used_.end()) {
    configs_to_be_used_.erase(used_it);
  }

  return it->second;
}

bool DocsMap::Has(std::string_view name) const {
  return utils::impl::FindTransparent(docs_, name) != docs_.end();
}

void DocsMap::Set(std::string name, formats::json::Value obj) {
  utils::impl::TransparentInsertOrAssign(docs_, std::move(name),
                                         std::move(obj));
}

void DocsMap::Parse(const std::string& json_string, bool empty_ok) {
  const auto json = formats::json::FromString(json_string);
  json.CheckObject();

  if (!empty_ok && json.GetSize() == 0)
    throw std::runtime_error("DocsMap::Parse failed: json is empty");

  for (const auto& [name, value] : Items(json)) {
    Set(name, value);
  }
}

size_t DocsMap::Size() const { return docs_.size(); }

void DocsMap::MergeOrAssign(DocsMap&& source) {
  auto new_docs = std::move(source.docs_);
  new_docs.merge(std::move(docs_));
  docs_ = std::move(new_docs);
}

void DocsMap::MergeMissing(const DocsMap& source) {
  docs_.insert(source.docs_.begin(), source.docs_.end());
}

std::unordered_set<std::string> DocsMap::GetNames() const {
  std::unordered_set<std::string> names;
  for (const auto& [k, v] : docs_) names.insert(k);
  return names;
}

formats::json::Value DocsMap::AsJson() const {
  return formats::json::ValueBuilder{docs_}.ExtractValue();
}

bool DocsMap::AreContentsEqual(const DocsMap& other) const {
  return docs_ == other.docs_;
}

void DocsMap::SetConfigsExpectedToBeUsed(
    utils::impl::TransparentSet<std::string> configs, utils::InternalTag) {
  configs_to_be_used_ = std::move(configs);
}

const utils::impl::TransparentSet<std::string>&
DocsMap::GetConfigsExpectedToBeUsed(utils::InternalTag) const {
  return configs_to_be_used_;
}

const std::string kValueDictDefaultName = "__default__";

namespace impl {

[[noreturn]] void ThrowNoValueException(std::string_view dict_name,
                                        std::string_view key) {
  throw std::runtime_error(
      fmt::format("no value for '{}' in dict '{}'", key, dict_name));
}

}  // namespace impl

}  // namespace dynamic_config

USERVER_NAMESPACE_END
