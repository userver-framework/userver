#pragma once

#include <userver/utest/utest.hpp>

#include <deque>
#include <memory>
#include <string>

#include <userver/storages/redis/base.hpp>
#include <userver/storages/redis/exception.hpp>
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

    ReplyType Get(const std::string& /*request_description*/) override { return std::move(reply_); }

    ReplyPtr GetRaw() override {
        UASSERT_MSG(false, "not supported in mocked request");
        return nullptr;
    }

    engine::impl::ContextAccessor* TryGetContextAccessor() noexcept override {
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

    engine::impl::ContextAccessor* TryGetContextAccessor() noexcept override {
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
        throw RequestFailedException(request_description, ReplyStatus::kTimeoutError);
    }

    ReplyPtr GetRaw() override {
        UASSERT_MSG(false, "not supported in mocked request");
        return nullptr;
    }

    engine::impl::ContextAccessor* TryGetContextAccessor() noexcept override {
        UASSERT_MSG(false, "not supported in mocked request");
        return nullptr;
    }
};

template <ScanTag scan_tag>
class MockRequestScanData : public RequestScanDataBase<scan_tag> {
public:
    using ReplyElem = typename ScanReplyElem<scan_tag>::type;

    template <typename Data>
    explicit MockRequestScanData(const Data& data) : MockRequestScanData(data.begin(), data.end()) {}

    template <typename It>
    explicit MockRequestScanData(It begin, It end) : data_(begin, end) {}

    ReplyElem Get() override {
        if (Eof()) throw RequestScan::GetAfterEofException("Trying to Get() after eof");
        auto result = std::move(data_.front());
        data_.pop_front();
        return result;
    }

    ReplyElem& Current() override {
        if (Eof()) throw RequestScan::GetAfterEofException("Trying to call Current() after eof");
        return data_.front();
    }

    bool Eof() override { return data_.empty(); }

private:
    std::deque<ReplyElem> data_;
};

template <typename T>
struct ReplyTypeHelper {};

template <typename Result, typename ReplyType>
struct ReplyTypeHelper<storages::redis::Request<Result, ReplyType>> {
    using ExtractedReplyType = ReplyType;
};

}  // namespace impl

template <typename Request>
Request CreateMockRequest(typename Request::Reply reply) {
    return Request(
        std::make_unique<impl::MockRequestData<typename Request::Result, typename Request::Reply>>(std::move(reply))
    );
}

template <typename Request>
Request CreateMockRequest() {
    static_assert(std::is_same_v<typename Request::Reply, void>, "you must specify the reply for this request");
    return Request(std::make_unique<impl::MockRequestData<typename Request::Result, void>>());
}

template <typename Request>
Request CreateMockRequestTimeout() {
    return Request(std::make_unique<impl::MockRequestDataTimeout<typename Request::Result, typename Request::Reply>>());
}

template <ScanTag scan_tag>
ScanRequest<scan_tag> CreateMockRequestScan(const std::vector<typename ScanReplyElem<scan_tag>::type>& reply_data) {
    return ScanRequest<scan_tag>(std::make_unique<impl::MockRequestScanData<scan_tag>>(reply_data));
}

}  // namespace storages::redis

USERVER_NAMESPACE_END
