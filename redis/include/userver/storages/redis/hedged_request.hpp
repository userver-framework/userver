#pragma once

/// @file userver/storages/redis/hedged_request.hpp
/// @brief
/// Classes and functions for performing hedged requests.
///
/// Use MakeHedgedRedisRequest method to perform hedged redis request.
///
/// Example(Sync):
/// auto result =
///     ::redis::MakeHedgedRedisRequest<storages::redis::RequestHGet>(
///         redis_client_shared_ptr,
///         &storages::redis::Client::Hget,
///         redis_cc, hedging_settings,
///         key, field);
/// Example(Async):
/// auto future =
///     ::redis::MakeHedgedRedisRequestAsync<storages::redis::RequestHGet>(
///         redis_client_shared_ptr,
///         &storages::redis::Client::Hget,
///         redis_cc, hedging_settings,
///         key, field);
/// auto result = future.Get();
///

#include <optional>

#include <userver/storages/redis/client.hpp>
#include <userver/storages/redis/command_control.hpp>
#include <userver/utils/hedged_request.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {
namespace impl {

template <typename RedisRequestType>
struct RedisRequestStrategy {
public:
    using RequestType = RedisRequestType;
    using ReplyType = typename RequestType::Reply;
    using GenF = std::function<std::optional<RequestType>(int)>;

    explicit RedisRequestStrategy(GenF gen_callback) : gen_callback_(std::move(gen_callback)) {}

    RedisRequestStrategy(RedisRequestStrategy&& other) noexcept = default;

    /// @{
    /// Methods needed by HedgingStrategy
    std::optional<RequestType> Create(size_t try_count) { return gen_callback_(try_count); }
    std::optional<std::chrono::milliseconds> ProcessReply(RequestType&& request) {
        reply_ = std::move(request).Get();
        return std::nullopt;
    }
    std::optional<ReplyType> ExtractReply() { return std::move(reply_); }
    void Finish(RequestType&&) {}
    /// @}

private:
    GenF gen_callback_;
    std::optional<ReplyType> reply_;
};

}  // namespace impl

template <typename RedisRequestType>
using HedgedRedisRequest = utils::hedging::HedgedRequestFuture<impl::RedisRequestStrategy<RedisRequestType>>;

template <
    typename RedisRequestType,
    typename... Args,
    typename M = RedisRequestType (storages::redis::Client::*)(Args..., const redis::CommandControl&)>
HedgedRedisRequest<RedisRequestType> MakeHedgedRedisRequestAsync(
    std::shared_ptr<storages::redis::Client> redis,
    M method,
    const redis::CommandControl& cc,
    utils::hedging::HedgingSettings hedging_settings,
    Args... args
) {
    auto gen_request = [redis, method, cc{std::move(cc)}, args_tuple{std::tuple(std::move(args)...)}](int try_count
                       ) mutable -> std::optional<RedisRequestType> {
        cc.retry_counter = try_count;
        cc.max_retries = 1;  ///< We do retries ourselves

        return std::apply(
            [redis, method, cc](auto&&... args) { return (redis.get()->*method)(args..., cc); }, args_tuple
        );
    };
    return utils::hedging::HedgeRequestAsync(
        impl::RedisRequestStrategy<RedisRequestType>(std::move(gen_request)), hedging_settings
    );
}

template <typename RedisRequestType>
using HedgedRedisRequest = utils::hedging::HedgedRequestFuture<impl::RedisRequestStrategy<RedisRequestType>>;

template <
    typename RedisRequestType,
    typename... Args,
    typename M = RedisRequestType (storages::redis::Client::*)(Args..., const redis::CommandControl&)>
std::optional<typename RedisRequestType::Reply> MakeHedgedRedisRequest(
    std::shared_ptr<storages::redis::Client> redis,
    M method,
    const redis::CommandControl& cc,
    utils::hedging::HedgingSettings hedging_settings,
    Args... args
) {
    auto gen_request = [redis, method, cc{std::move(cc)}, args_tuple{std::tuple(std::move(args)...)}](int try_count
                       ) mutable -> std::optional<RedisRequestType> {
        cc.retry_counter = try_count;
        cc.max_retries = 1;  ///< We do retries ourselves

        return std::apply(
            [redis, method, cc](auto&&... args) { return (redis.get()->*method)(args..., cc); }, args_tuple
        );
    };
    return utils::hedging::HedgeRequest(
        impl::RedisRequestStrategy<RedisRequestType>(std::move(gen_request)), hedging_settings
    );
}

/// Same as MakeHedgedRedisRequest but accepting a vector of argument tuples
/// instead of a single set of arguments. And return vector of replies
/// corresponding to input args.
template <
    typename RedisRequestType,
    typename... Args,
    typename M = RedisRequestType (storages::redis::Client::*)(Args..., const redis::CommandControl&)>
std::vector<std::optional<typename RedisRequestType::Reply>> MakeBulkHedgedRedisRequest(
    std::shared_ptr<storages::redis::Client> redis,
    M method,
    const redis::CommandControl& cc,
    utils::hedging::HedgingSettings hedging_settings,
    std::vector<std::tuple<Args...>> args
) {
    std::vector<redis::impl::RedisRequestStrategy<RedisRequestType>> strategies;
    strategies.reserve(args.size());

    for (auto&& arg : args) {
        auto gen_request = [redis, method, cc{std::move(cc)}, args_tuple{std::move(arg)}](int try_count
                           ) mutable -> std::optional<RedisRequestType> {
            cc.retry_counter = try_count;
            cc.max_retries = 1;  ///< We do retries ourselves

            return std::apply(
                [redis, method, cc](auto&&... args) { return (redis.get()->*method)(args..., cc); }, args_tuple
            );
        };
        strategies.emplace_back(std::move(gen_request));
    }

    return utils::hedging::HedgeRequestsBulk(std::move(strategies), hedging_settings);
}

/// Same as MakeHedgedRedisRequestAsync but accepting a vector of argument
/// tuples instead of a single set of arguments. And return vector of replies
/// corresponding to input args.
template <
    typename RedisRequestType,
    typename... Args,
    typename M = RedisRequestType (storages::redis::Client::*)(Args..., const redis::CommandControl&)>
utils::hedging::HedgedRequestBulkFuture<redis::impl::RedisRequestStrategy<RedisRequestType>>
MakeBulkHedgedRedisRequestAsync(
    std::shared_ptr<storages::redis::Client> redis,
    M method,
    const redis::CommandControl& cc,
    utils::hedging::HedgingSettings hedging_settings,
    std::vector<std::tuple<Args...>> args
) {
    using RequestStrategy = redis::impl::RedisRequestStrategy<RedisRequestType>;
    std::vector<RequestStrategy> strategies;
    strategies.reserve(args.size());

    for (auto&& arg : args) {
        auto gen_request = [redis, method, cc{std::move(cc)}, args_tuple{std::move(arg)}](int try_count
                           ) mutable -> std::optional<RedisRequestType> {
            cc.retry_counter = try_count;
            cc.max_retries = 1;  ///< We do retries ourselves

            return std::apply(
                [redis, method, cc](auto&&... args) { return (redis.get()->*method)(args..., cc); }, args_tuple
            );
        };
        strategies.emplace_back(std::move(gen_request));
    }

    return utils::hedging::HedgeRequestsBulkAsync<RequestStrategy>(std::move(strategies), hedging_settings);
}

}  // namespace redis

USERVER_NAMESPACE_END
