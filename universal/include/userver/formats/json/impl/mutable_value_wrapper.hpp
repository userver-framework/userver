#pragma once

#include <memory>
#include <string>

#include <userver/formats/json/impl/types.hpp>

#include <userver/compiler/select.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json {
class Value;
}  // namespace formats::json

namespace formats::json::impl {

class MutableValueWrapper {
 public:
  MutableValueWrapper();
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

  static constexpr std::size_t kSize = compiler::SelectSize()  //
                                           .ForLibCpp32(48)
                                           .ForLibCpp64(96)
                                           .ForLibStdCpp64(104)
                                           .ForLibStdCpp32(60);
  static constexpr std::size_t kAlignment = alignof(void*);
  utils::FastPimpl<Impl, kSize, kAlignment> impl_;
};

}  // namespace formats::json::impl

USERVER_NAMESPACE_END
