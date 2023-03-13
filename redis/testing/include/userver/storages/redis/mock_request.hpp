#pragma once

#include <userver/utest/utest.hpp>

#include <deque>
#include <memory>
#include <string>

#include <userver/storages/redis/impl/base.hpp>
#include <userver/storages/redis/impl/exception.hpp>
#include <userver/storages/redis/impl/request.hpp>
#include <userver/storages/redis/reply_types.hpp>
#include <userver/storages/redis/request.hpp>
#include <userver/utils/assert.hpp>

#include <userver/storages/redis/request_data_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {
namespace impl {

template <typename Result, typename ReplyType>
class MockRequestData final : public RequestDataBase<ReplyType> {
 public:
  explicit MockRequestData(ReplyType&& reply) : reply_(std::move(reply)) {}

  void Wait() override {}

  ReplyType Get(const std::string& /*request_description*/) override {
    return std::move(reply_);
  }

  ReplyPtr GetRaw() override {
    UASSERT_MSG(false, "not supported in mocked request");
    return nullptr;
  }

 private:
  ReplyType reply_;
};

template <typename Result>
class MockRequestData<Result, void> final : public RequestDataBase<void> {
 public:
  MockRequestData() = default;

  void Wait() override {}

  void Get(const std::string& /*request_description*/) override {}

  ReplyPtr GetRaw() override {
    UASSERT_MSG(false, "not supported in mocked request");
    return nullptr;
  }
};

template <typename Result, typename ReplyType>
class MockRequestDataTimeout final : public RequestDataBase<ReplyType> {
 public:
  MockRequestDataTimeout() = default;

  void Wait() override {}

  ReplyType Get(const std::string& request_description) override {
    throw USERVER_NAMESPACE::redis::RequestFailedException(
        request_description,
        USERVER_NAMESPACE::redis::ReplyStatus::kTimeoutError);
  }

  ReplyPtr GetRaw() override {
    UASSERT_MSG(false, "not supported in mocked request");
    return nullptr;
  }
};

template <ScanTag scan_tag>
class MockRequestScanData : public RequestScanDataBase<scan_tag> {
 public:
  using ReplyElem = typename ScanReplyElem<scan_tag>::type;

  template <typename Data>
  explicit MockRequestScanData(const Data& data)
      : MockRequestScanData(data.begin(), data.end()) {}

  template <typename It>
  explicit MockRequestScanData(It begin, It end) : data_(begin, end) {}

  ReplyElem Get() override {
    if (Eof())
      throw RequestScan::GetAfterEofException("Trying to Get() after eof");
    auto result = std::move(data_.front());
    data_.pop_front();
    return result;
  }

  ReplyElem& Current() override {
    if (Eof())
      throw RequestScan::GetAfterEofException(
          "Trying to call Current() after eof");
    return data_.front();
  }

  bool Eof() override { return data_.empty(); }

 private:
  std::deque<ReplyElem> data_;
};

template <typename Result, typename ReplyType = Result>
Request<Result, ReplyType> CreateMockRequest(
    Result&& result, Request<Result, ReplyType>* /* for ADL */) {
  return Request<Result, ReplyType>(
      std::make_unique<MockRequestData<Result, ReplyType>>(
          std::forward<Result>(result)));
}

template <typename Result, typename ReplyType = Result>
Request<Result, ReplyType> CreateMockRequestVoid(
    Request<Result, ReplyType>* /* for ADL */) {
  static_assert(std::is_same<ReplyType, void>::value, "ReplyType must be void");
  return Request<Result, ReplyType>(
      std::make_unique<MockRequestData<Result, ReplyType>>());
}

template <typename Result, typename ReplyType = Result>
Request<Result, ReplyType> CreateMockRequestTimeout(
    Request<Result, ReplyType>* /* for ADL */) {
  return Request<Result, ReplyType>(
      std::make_unique<MockRequestDataTimeout<Result, ReplyType>>());
}

template <typename T>
struct ReplyTypeHelper {};

template <typename Result, typename ReplyType>
struct ReplyTypeHelper<Request<Result, ReplyType>> {
  using ExtractedReplyType = ReplyType;
};

template <typename Request>
using ExtractReplyType = typename ReplyTypeHelper<Request>::ExtractedReplyType;

}  // namespace impl

template <typename Request>
Request CreateMockRequest(impl::ExtractReplyType<Request> reply) {
  Request* tmp = nullptr;
  return impl::CreateMockRequest(std::move(reply), tmp);
}

template <typename Request>
Request CreateMockRequest() {
  static_assert(std::is_same<impl::ExtractReplyType<Request>, void>::value,
                "you must specify the reply for this request");
  Request* tmp = nullptr;
  return impl::CreateMockRequestVoid(tmp);
}

template <typename Request>
Request CreateMockRequestTimeout() {
  Request* tmp = nullptr;
  return impl::CreateMockRequestTimeout(tmp);
}

template <ScanTag scan_tag>
ScanRequest<scan_tag> CreateMockRequestScan(
    const std::vector<typename ScanReplyElem<scan_tag>::type>& reply_data) {
  return ScanRequest<scan_tag>(
      std::make_unique<impl::MockRequestScanData<scan_tag>>(reply_data));
}

}  // namespace storages::redis

USERVER_NAMESPACE_END
