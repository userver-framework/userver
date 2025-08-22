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
        : start_time(std::chrono::steady_clock::now()),
          form_data_args(impl::kZeroAllocationBucketCount, request_args.hash_function()),
          path_args_by_name_index(impl::kZeroAllocationBucketCount, request_args.hash_function()),
          headers(impl::kBucketCount),
          cookies(impl::kZeroAllocationBucketCount, request_args.hash_function()),
          response(http_request, data_accounter, start_time, cookies.hash_function()) {}

    std::chrono::steady_clock::time_point start_time;
    std::chrono::steady_clock::time_point task_create_time;
    std::chrono::steady_clock::time_point task_start_time;
    std::chrono::steady_clock::time_point response_notify_time;
    std::chrono::steady_clock::time_point start_send_response_time;
    std::chrono::steady_clock::time_point finish_send_response_time;

    HttpMethod method{HttpMethod::kUnknown};
    int http_major{1};
    int http_minor{1};
    std::string url;
    std::string request_path;
    std::string request_body;
    utils::impl::TransparentMap<std::string, std::vector<std::string>, utils::StrCaseHash> request_args;
    utils::impl::TransparentMap<std::string, std::vector<FormDataArg>, utils::StrCaseHash> form_data_args;
    std::vector<std::string> path_args;
    utils::impl::TransparentMap<std::string, size_t, utils::StrCaseHash> path_args_by_name_index;
    HeadersMap headers;
    CookiesMap cookies;
    bool is_final{false};
#ifndef NDEBUG
    mutable bool args_referenced{false};
#endif
    // TODO
    mutable UpgradeCallback upgrade_websocket_cb;

    mutable HttpResponse response;
    engine::io::Sockaddr remote_address;
    engine::TaskProcessor* task_processor{nullptr};
    const handlers::HttpHandlerBase* handler{nullptr};
    handlers::HttpRequestStatistics* request_statistics{nullptr};
};

}  // namespace server::http

USERVER_NAMESPACE_END
