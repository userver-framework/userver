#pragma once

#include <optional>
#include <string>
#include <unordered_set>

#include <userver/formats/json/value.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/default_dict.hpp>
#include <userver/utils/impl/transparent_hash.hpp>
#include <userver/utils/internal_tag_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config {

class DocsMap final {
 public:
  /* Returns config item or throws an exception if key is missing */
  formats::json::Value Get(std::string_view name) const;

  bool Has(std::string_view name) const;
  void Set(std::string name, formats::json::Value);
  void Parse(std::string_view json_string, bool empty_ok);
  void Parse(formats::json::Value json, bool empty_ok);
  void Remove(const std::string& name);
  size_t Size() const;

  void MergeOrAssign(DocsMap&& source);
  void MergeMissing(const DocsMap& source);

  std::unordered_set<std::string> GetNames() const;
  formats::json::Value AsJson() const;
  bool AreContentsEqual(const DocsMap& other) const;

  /// @cond
  // For internal use only
  // Set of configs expected to be used is automatically updated when
  // configs are retrieved with 'Get' method.
  void SetConfigsExpectedToBeUsed(
      utils::impl::TransparentSet<std::string> configs, utils::InternalTag);

  const utils::impl::TransparentSet<std::string>& GetConfigsExpectedToBeUsed(
      utils::InternalTag) const;
  /// @endcond

 private:
  utils::impl::TransparentMap<std::string, formats::json::Value> docs_;
  mutable utils::impl::TransparentSet<std::string> configs_to_be_used_;
};

template <typename ValueType>
using ValueDict = utils::DefaultDict<ValueType>;

inline constexpr auto kValueDictDefaultName = utils::kDefaultDictDefaultName;

}  // namespace dynamic_config

USERVER_NAMESPACE_END
