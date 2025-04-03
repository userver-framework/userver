#pragma once

#include <userver/server/http/http_request.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

namespace impl {

constexpr size_t kBucketCount = 16;

constexpr size_t kZeroAllocationBucketCount = 0;

}  // namespace impl

struct HttpRequest::Impl {
    // Use hash_function() magic to pass out the same RNG seed among all
    // unordered_maps because we don't need different seeds and want to avoid its
    // overhead.
    Impl(HttpRequest& http_request, request::ResponseDataAccounter& data_accounter)
        : start_time_(std::chrono::steady_clock::now()),
          form_data_args_(impl::kZeroAllocationBucketCount, request_args_.hash_function()),
          path_args_by_name_index_(impl::kZeroAllocationBucketCount, request_args_.hash_function()),
          headers_(impl::kBucketCount),
          cookies_(impl::kZeroAllocationBucketCount, request_args_.hash_function()),
          response_(http_request, data_accounter, start_time_, cookies_.hash_function()) {}

    std::chrono::steady_clock::time_point start_time_;
    std::chrono::steady_clock::time_point task_create_time_;
    std::chrono::steady_clock::time_point task_start_time_;
    std::chrono::steady_clock::time_point response_notify_time_;
    std::chrono::steady_clock::time_point start_send_response_time_;
    std::chrono::steady_clock::time_point finish_send_response_time_;

    HttpMethod method_{HttpMethod::kUnknown};
    int http_major_{1};
    int http_minor_{1};
    std::string url_;
    std::string request_path_;
    std::string request_body_;
    utils::impl::TransparentMap<std::string, std::vector<std::string>, utils::StrCaseHash> request_args_;
    utils::impl::TransparentMap<std::string, std::vector<FormDataArg>, utils::StrCaseHash> form_data_args_;
    std::vector<std::string> path_args_;
    utils::impl::TransparentMap<std::string, size_t, utils::StrCaseHash> path_args_by_name_index_;
    HeadersMap headers_;
    CookiesMap cookies_;
    bool is_final_{false};
#ifndef NDEBUG
    mutable bool args_referenced_{false};
#endif
    // TODO
    mutable UpgradeCallback upgrade_websocket_cb_;

    mutable HttpResponse response_;
    engine::io::Sockaddr remote_address_;
    engine::TaskProcessor* task_processor_{nullptr};
    const handlers::HttpHandlerBase* handler_{nullptr};
    handlers::HttpRequestStatistics* request_statistics_{nullptr};
};

}  // namespace server::http

USERVER_NAMESPACE_END
