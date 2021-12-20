#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include <grpcpp/channel.h>
#include <grpcpp/security/credentials.h>
#include <grpcpp/support/channel_arguments.h>

#include <userver/concurrent/variable.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

class ChannelCache final {
 public:
  ChannelCache(std::shared_ptr<grpc::ChannelCredentials>&& credentials,
               const grpc::ChannelArguments& channel_args);

  ~ChannelCache();

  class Token;

  // The grpc::Channel is kept in cache as long as some Token pointing to it is
  // alive.
  Token Get(const std::string& endpoint);

 private:
  struct CountedChannel final {
    CountedChannel(const std::string& endpoint,
                   const std::shared_ptr<grpc::ChannelCredentials>& credentials,
                   const grpc::ChannelArguments& channel_args);

    std::shared_ptr<::grpc::Channel> channel;
    std::uint64_t counter{0};
  };

  using Map = std::unordered_map<std::string, CountedChannel>;

  const std::shared_ptr<grpc::ChannelCredentials> credentials_;
  const grpc::ChannelArguments channel_args_;
  concurrent::Variable<Map> channels_;
};

class ChannelCache::Token final {
 public:
  Token() noexcept = default;

  // Must be constructed with 'cache.channels_' under lock
  Token(ChannelCache& cache, const std::string& endpoint,
        CountedChannel& counted_channel) noexcept;

  Token(Token&&) noexcept;
  Token& operator=(Token&&) noexcept;
  ~Token();

  const std::shared_ptr<::grpc::Channel>& GetChannel() const noexcept;

 private:
  ChannelCache* cache_{nullptr};
  const std::string* endpoint_{nullptr};
  CountedChannel* counted_channel_{nullptr};
};

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
