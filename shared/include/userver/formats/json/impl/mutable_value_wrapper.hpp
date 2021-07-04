#pragma once

#include <memory>
#include <string>
#include <variant>
#include <vector>

#include <userver/formats/json/impl/types.hpp>

#include <userver/utils/fast_pimpl.hpp>

namespace formats::json {
class Value;
}  // namespace formats::json

namespace formats::json::impl {

class MutableValueWrapper {
 public:
  MutableValueWrapper() noexcept;
  explicit MutableValueWrapper(VersionedValuePtr root);
  ~MutableValueWrapper();

  MutableValueWrapper(const MutableValueWrapper&);
  MutableValueWrapper(MutableValueWrapper&&) noexcept;
  MutableValueWrapper& operator=(const MutableValueWrapper&);
  MutableValueWrapper& operator=(MutableValueWrapper&&) noexcept;

  MutableValueWrapper WrapMember(std::string&& element,
                                 const Value& member) const;
  MutableValueWrapper WrapElement(size_t index) const;

  const formats::json::Value& operator*() const;
  formats::json::Value& operator*();
  const formats::json::Value* operator->() const;
  formats::json::Value* operator->();

  std::string GetPath() const;
  formats::json::Value ExtractValue() &&;

  void OnMembersChange();

 private:
  struct JsonPath;
  struct Impl;

  MutableValueWrapper(std::shared_ptr<JsonPath> path, VersionedValuePtr root,
                      const Value& member, int depth);

  void EnsureCurrent() const;

  // MAC_COMPAT
#ifdef _LIBCPP_VERSION
  static constexpr size_t kSize = 80;
  static constexpr size_t kAlignment = 8;
#else
  static constexpr size_t kSize = 88;
  static constexpr size_t kAlignment = 8;
#endif
  utils::FastPimpl<Impl, kSize, kAlignment, true> impl_;
};

}  // namespace formats::json::impl
