#include <userver/storages/etcd/response.hpp>

#include <etcd/api/etcdserverpb/rpc.pb.h>
#include <etcd/api/mvccpb/kv.pb.h>

USERVER_NAMESPACE_BEGIN


namespace storages::etcd {

Response::Response() = default;
Response::Response(Response&&) = default;
Response::Response(const Response&) = default;
Response& Response::operator=(Response&&) = default;
Response& Response::operator=(const Response&) = default;
Response::~Response() = default;

Response::Response(ResponseNative&& native_response)
    : response_(native_response)
{}

Response::iterator::iterator(std::size_t index, const Response& response)
    : index_(index), response_(response)
{}

Response::iterator& Response::iterator::operator++() {
  ++index_;
  return *this;
}

Response::iterator Response::iterator::operator++(int) {
  return iterator(index_++, response_);
}

bool Response::iterator::operator==(Response::iterator other) {
  return &response_ == &other.response_ && index_ == other.index_;
}

bool Response::iterator::operator!=(Response::iterator other) {
  return &response_ != &other.response_ || index_ != other.index_;
}

Response::iterator::reference Response::iterator::operator*() {
  return response_[index_];
}

std::unique_ptr<KeyValue> Response::iterator::operator->() {
  return std::make_unique<KeyValue>(response_[index_]);
}

std::size_t Response::size() const {
  return response_->kvs_size();
}

Response::iterator Response::begin() {
  return iterator(0, *this);
}

Response::iterator Response::end() {
  return iterator(response_->kvs_size(), *this);
}

KeyValue Response::operator[](std::size_t index) const {
  return KeyValue(response_->kvs(index));
}

}  // namespace storages::etcd

USERVER_NAMESPACE_END