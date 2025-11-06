#include <etcd/client_impl.hpp>

#include <iostream>
#include <memory>
#include <string>

#include <fmt/format.h>

#include <userver/clients/http/streamed_response.hpp>
#include <userver/crypto/base64.hpp>
#include <userver/dynamic_config/value.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/etcd/exceptions.hpp>
#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/http/status_code.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/rand.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include <schemas/etcd.hpp>

#include <etcd/etcd_responses.hpp>

USERVER_NAMESPACE_BEGIN

namespace etcd {

namespace {

const std::uint32_t kMinRetryStatusCode = 500;
const std::uint32_t kMaxRetryStatusCode = 599;

const std::uint32_t kMinGoodStatusCode = 200;
const std::uint32_t kMaxGoodStatusCode = 299;

const std::string kKeyPrefix = "/etcd/";

KeyValueState ConvertRawKeyValueState(const etcd_schemas::RawKeyValueState& raw_key_value_state) {
    KeyValueState key_value_state;
    key_value_state.key = raw_key_value_state.key.GetUnderlying().substr(kKeyPrefix.size());
    key_value_state.value = raw_key_value_state.value.GetUnderlying();
    key_value_state.version = std::stoi(raw_key_value_state.version);
    return key_value_state;
}

std::string BuildPutUrl(std::string_view service_url) { return fmt::format("{}/v3/kv/put", service_url); }

std::string BuildPutData(std::string_view key, std::string_view value) {
    const auto etcd_key = fmt::format("{}{}", kKeyPrefix, key);
    return formats::json::ToString(formats::json::MakeObject(
        "key", crypto::base64::Base64Encode(etcd_key), "value", crypto::base64::Base64Encode(value)
    ));
}

std::string BuildRangeUrl(std::string_view service_url) { return fmt::format("{}/v3/kv/range", service_url); }

std::string BuildRangeData(std::string_view key, const std::optional<std::string_view> maybe_range_end = std::nullopt) {
    const auto etcd_key = fmt::format("{}{}", kKeyPrefix, key);
    if (!maybe_range_end.has_value()) {
        return formats::json::ToString(formats::json::MakeObject("key", crypto::base64::Base64Encode(etcd_key)));
    }
    const auto etcd_range_end = fmt::format("{}{}", kKeyPrefix, maybe_range_end.value());
    return formats::json::ToString(formats::json::MakeObject(
        "key", crypto::base64::Base64Encode(etcd_key), "range_end", crypto::base64::Base64Encode(etcd_range_end)
    ));
}

std::string BuildDeleteUrl(std::string_view service_url) { return fmt::format("{}/v3/kv/deleterange", service_url); }

std::string BuildDeleteData(std::string_view key) {
    const auto etcd_key = fmt::format("{}{}", kKeyPrefix, key);
    return formats::json::ToString(formats::json::MakeObject("key", crypto::base64::Base64Encode(etcd_key)));
}

std::string BuildWatchUrl(std::string_view service_url) { return fmt::format("{}/v3/watch", service_url); }

std::string BuildWatchData(std::string_view key) {
    const auto etcd_key = fmt::format("{}{}", kKeyPrefix, key);
    return formats::json::ToString(formats::json::MakeObject(
        "create_request", formats::json::MakeObject("key", crypto::base64::Base64Encode(etcd_key))
    ));
}

bool ShouldRetry(const http::StatusCode status_code) {
    return kMinRetryStatusCode <= status_code && status_code <= kMaxRetryStatusCode;
}

void CheckResponseStatusCode(const http::StatusCode status_code, std::string_view body) {
    if (status_code < kMinGoodStatusCode || kMaxGoodStatusCode < status_code) {
        throw EtcdRequestError(fmt::format("Got bad status code from etcd: {}, body: {}", status_code, body));
    }
}

}  // namespace

namespace impl {

ClientImpl::ClientImpl(clients::http::Client& http_client, ClientSettings settings)
    : http_client_(http_client), settings_(settings) {}

void ClientImpl::Put(std::string_view key, std::string_view value) {
    PerformEtcdRequest(BuildPutUrl, BuildPutData(key, value));
}

void ClientImpl::Delete(std::string_view key) { PerformEtcdRequest(BuildDeleteUrl, BuildDeleteData(key)); }

std::optional<std::string> ClientImpl::Get(std::string_view key) {
    auto response = PerformEtcdRequest(BuildRangeUrl, BuildRangeData(key));

    const auto maybe_range_response = formats::json::FromString(response.body()).As<etcd_schemas::EtcdRangeResponse>();
    if (!maybe_range_response.kvs.has_value()) {
        return std::nullopt;
    }
    for (const auto& raw_key_value_state : maybe_range_response.kvs.value()) {
        const auto key_value_state = ConvertRawKeyValueState(raw_key_value_state);
        if (key_value_state.key == key) {
            return key_value_state.value;
        }
    }
    return std::nullopt;
}

std::vector<KeyValueState> ClientImpl::Range(std::string_view key_prefix) {
    const auto response =
        PerformEtcdRequest(BuildRangeUrl, BuildRangeData(key_prefix, fmt::format("{}\xFF", key_prefix)));

    const auto maybe_range_response = formats::json::FromString(response.body()).As<etcd_schemas::EtcdRangeResponse>();
    if (!maybe_range_response.kvs.has_value()) {
        return {};
    }

    std::vector<KeyValueState> range_result;
    range_result.reserve(maybe_range_response.kvs.value().size());
    for (const auto& raw_key_value_state : maybe_range_response.kvs.value()) {
        const auto key_value_state = ConvertRawKeyValueState(raw_key_value_state);
        range_result.push_back(key_value_state);
    }
    return range_result;
}

WatchListener ClientImpl::StartWatch(std::string_view key) {
    auto queue = concurrent::SpscQueue<KeyValueState>::Create();

    auto watch_queues_ptr = watch_queues_.Lock();
    watch_queues_ptr->push_back(queue);

    bts_.AsyncDetach("watch task", [string_key = std::string(key), producer = queue->GetProducer(), this]() mutable {
        this->WatchKeyChanges(string_key, std::move(producer));
    });

    return WatchListener{queue->GetConsumer()};
}

clients::http::Response
ClientImpl::PerformEtcdRequest(const std::function<std::string(std::string_view)>& url_builder, std::string_view data) {
    auto endpoints = settings_.endpoints;
    utils::Shuffle(endpoints);

    std::optional<clients::http::Response> maybe_response;
    for (const auto& endpoint : endpoints) {
        const auto response_ptr = http_client_.CreateRequest()
                                      .post(url_builder(endpoint), std::string{data})
                                      .retry(settings_.attempts)
                                      .timeout(settings_.request_timeout_ms.count())
                                      .perform();
        if (response_ptr == nullptr) {
            LOG_WARNING() << "Perform request returns nullptr";
            continue;
        }
        maybe_response = *(response_ptr);
        const auto& response = maybe_response.value();
        if (!ShouldRetry(response.status_code())) {
            CheckResponseStatusCode(response.status_code(), response.body());
            return response;
        }
    }
    LOG_ERROR() << "Request was not successful";
    if (maybe_response.has_value()) {
        throw EtcdRequestError(
            fmt::format("Failed to get Ok response from etcd with error: {}", maybe_response.value().body())
        );
    } else {
        throw EtcdRequestError(
            fmt::format("Failed to get streamed response, number of etcd endpoints: {}", endpoints.size())
        );
    }
}

clients::http::StreamedResponse ClientImpl::PerformStreamedEtcdRequest(
    const std::function<std::string(std::string_view)>& url_builder,
    std::string_view data
) {
    auto endpoints = settings_.endpoints;
    utils::Shuffle(endpoints);

    std::optional<clients::http::StreamedResponse> maybe_streamed_response;
    for (const auto& endpoint : endpoints) {
        const auto queue = concurrent::StringStreamQueue::Create();
        maybe_streamed_response = http_client_.CreateRequest()
                                      .post(url_builder(endpoint), std::string{data})
                                      .retry(settings_.attempts)
                                      .timeout(settings_.watch_timeout_ms.count())
                                      .async_perform_stream_body(queue);
        auto& streamed_response = maybe_streamed_response.value();
        if (!ShouldRetry(streamed_response.StatusCode())) {
            CheckResponseStatusCode(streamed_response.StatusCode(), "There is no body in stream responses");
            return std::move(streamed_response);
        }
    }
    if (maybe_streamed_response.has_value()) {
        throw EtcdError(fmt::format(
            "Failed to get Ok response from etcd with status code: {}", maybe_streamed_response.value().StatusCode()
        ));
    } else {
        throw EtcdError(fmt::format("Failed to get streamed response, number of etcd endpoints: {}", endpoints.size()));
    }
}

void ClientImpl::WatchKeyChanges(const std::string key, concurrent::SpscQueue<KeyValueState>::Producer producer) {
    const auto watch_data = BuildWatchData(key);

    while (true) {
        LOG_DEBUG() << "Start whatching key changes";
        auto stream_response = PerformStreamedEtcdRequest(BuildWatchUrl, watch_data);
        std::string body_part;
        while (stream_response.ReadChunk(body_part, engine::Deadline())) {
            EtcdWatchResponse etcd_watch_response;
            try {
                etcd_watch_response = formats::json::FromString(body_part).As<EtcdWatchResponse>();
            } catch (const EtcdWatchResponseParseError& error) {
                LOG_DEBUG() << "Couldnot parse etcd response: " << error;
                continue;
            }
            for (const auto& raw_key_value_state : etcd_watch_response.raw_key_value_states) {
                auto key_value_state = ConvertRawKeyValueState(raw_key_value_state);
                if (!producer.Push(std::move(key_value_state))) {
                    LOG_ERROR() << "Could not push to queue, aborting task";
                    return;
                };
            }
        }
        LOG_ERROR() << "Could not read chunk from stream response";
    }
}

}  // namespace impl

}  // namespace etcd

USERVER_NAMESPACE_END
