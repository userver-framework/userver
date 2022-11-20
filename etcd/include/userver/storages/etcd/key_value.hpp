#pragma once

#include <memory>
#include <string>
#include <vector>
#include <algorithm>

#include "response_fwd.hpp"

#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::etcd {

class KeyValue {
 public:
    KeyValue();
    KeyValue(KeyValue&&);
    KeyValue& operator=(KeyValue&&);
    ~KeyValue();

  std::pair<const std::string&, const std::string&> Pair();
  const std::string& GetKey();
  const std::string& GetValue();

 private:
    using KeyValueNative = mvccpb::KeyValue;
    static constexpr std::size_t kImplSize = 72;
    static constexpr std::size_t kImplAlign = 8;
    utils::FastPimpl<KeyValueNative, kImplSize, kImplAlign> key_value_;
};

}

USERVER_NAMESPACE_END