#pragma once

#include <atomic>

#include <rapidjson/document.h>

#include <userver/formats/json/impl/types.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json::impl {

struct VersionedValuePtr::Data {
  template <typename... Args>
  explicit Data(Args&&... args) : native(std::forward<Args>(args)...) {}

  // rapidjson has somewhat broken class hierarchy, we have to do this manually
  // https://github.com/Tencent/rapidjson/issues/387
  explicit Data(Document&&);

  ~Data();

  // native rapidjson value
  Value native;

  // version of internal rapidjson structures (member arrays)
  // used in ValueBuilder to avoid UAF, ignored in read-only Value
  std::atomic<size_t> version{0};
};

template <typename... Args>
VersionedValuePtr VersionedValuePtr::Create(Args&&... args) {
  return VersionedValuePtr{std::make_shared<Data>(std::forward<Args>(args)...)};
}

}  // namespace formats::json::impl

USERVER_NAMESPACE_END
