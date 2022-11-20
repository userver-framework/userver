#pragma once

#include <userver/utils/fast_pimpl.hpp>

#include <memory>
#include <string>
#include <vector>
#include <algorithm>

#include "response_fwd.hpp"
#include "key_value.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::etcd {

class Response {
 public:
  class iterator : public std::iterator<std::input_iterator_tag, const KeyValue, const KeyValue, const KeyValue*, const KeyValue&> {
    public:
        explicit iterator(std::size_t index, const Response& response);
        iterator& operator++();
        iterator operator++(int);
        bool operator==(iterator other);
        bool operator!=(iterator other);
        reference operator*();
    private:
      std::size_t index_;
      const Response& response_;
  };

  Response();
  Response(Response&&);

  Response& operator=(Response&&);
  ~Response();
  
  iterator begin();
  iterator end();
  const KeyValue& operator[](std::size_t index) const;

 private:
    using ResponseNative = etcdserverpb::RangeResponse;
    static constexpr std::size_t kImplSize = 64;
    static constexpr std::size_t kImplAlign = 8;
    utils::FastPimpl<ResponseNative, kImplSize, kImplAlign> response_;

};

}

USERVER_NAMESPACE_END