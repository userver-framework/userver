#pragma once

#include <memory>
#include <string>
#include <vector>
#include <algorithm>

#include <userver/utils/fast_pimpl.hpp>

#include "client_fwd.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::etcd {

class KeyValue {
 public:
  using KeyValueNative = mvccpb::KeyValue;

  KeyValue();
  KeyValue(KeyValue&&);
  KeyValue(const KeyValue&);
  KeyValue(const KeyValueNative&);
  KeyValue& operator=(KeyValue&&);
  KeyValue& operator=(const KeyValue&);
  ~KeyValue();

  std::pair<const std::string&, const std::string&> Pair();
  const std::string& GetKey() const;
  const std::string& GetValue() const;

 private:
  static constexpr std::size_t kImplSize = 72;
  static constexpr std::size_t kImplAlign = 8;
  utils::FastPimpl<KeyValueNative, kImplSize, kImplAlign> key_value_;
};

}

USERVER_NAMESPACE_END