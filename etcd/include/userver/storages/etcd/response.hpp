#pragma once

#include <memory>
#include <string>
#include <vector>
#include <algorithm>

#include <userver/utils/fast_pimpl.hpp>

#include "client_fwd.hpp"
#include "key_value.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::etcd {

class Response {
 public:
  using ResponseNative = etcdserverpb::RangeResponse;

  class iterator : public std::iterator<std::input_iterator_tag, KeyValue, KeyValue, KeyValue*, KeyValue> {
   public:
    explicit iterator(std::size_t index, const Response& response);
    iterator& operator++();
    iterator operator++(int);
    bool operator==(iterator other);
    bool operator!=(iterator other);
    reference operator*();
    std::unique_ptr<KeyValue> operator->();
   private:
    std::size_t index_;
    const Response& response_;
  };

  Response();
  Response(Response&&);
  Response(const Response&);
  Response(ResponseNative&& native_response);
  Response& operator=(Response&&);
  Response& operator=(const Response&);
  ~Response();

  std::size_t size() const;
  iterator begin();
  iterator end();
  KeyValue operator[](std::size_t index) const;

 private:
  static constexpr std::size_t kImplSize = 64;
  static constexpr std::size_t kImplAlign = 8;
  utils::FastPimpl<ResponseNative, kImplSize, kImplAlign> response_;

};

}

USERVER_NAMESPACE_END