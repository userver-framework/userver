#include <userver/ugrpc/client/impl/channel_cache.hpp>

#include <utility>

#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

ChannelCache::Token::Token(ChannelCache& cache, const std::string& endpoint,
                           CountedChannel& counted_channel) noexcept
    : cache_(&cache), endpoint_(&endpoint), counted_channel_(&counted_channel) {
  ++counted_channel.counter;
}

ChannelCache::Token::Token(Token&& other) noexcept
    : cache_(std::exchange(other.cache_, nullptr)),
      endpoint_(std::exchange(other.endpoint_, nullptr)),
      counted_channel_(std::exchange(other.counted_channel_, nullptr)) {}

ChannelCache::Token& ChannelCache::Token::operator=(Token&& other) noexcept {
  std::swap(cache_, other.cache_);
  std::swap(endpoint_, other.endpoint_);
  std::swap(counted_channel_, other.counted_channel_);
  return *this;
}

ChannelCache::Token::~Token() {
  if (!cache_) return;
  UASSERT(endpoint_);
  UASSERT(counted_channel_);

  auto channels = cache_->channels_.Lock();
  if (--counted_channel_->counter == 0) {
    channels->erase(*endpoint_);
  }
}

const std::shared_ptr<grpc::Channel>& ChannelCache::Token::GetChannel() const
    noexcept {
  UASSERT(counted_channel_);
  return counted_channel_->channel;
}

ChannelCache::CountedChannel::CountedChannel(
    const std::string& endpoint,
    const std::shared_ptr<grpc::ChannelCredentials>& credentials,
    const grpc::ChannelArguments& channel_args)
    : channel(grpc::CreateCustomChannel(endpoint, credentials, channel_args)) {}

ChannelCache::ChannelCache(
    std::shared_ptr<grpc::ChannelCredentials>&& credentials,
    const grpc::ChannelArguments& channel_args)
    : credentials_(std::move(credentials)), channel_args_(channel_args) {}

ChannelCache::~ChannelCache() = default;

ChannelCache::Token ChannelCache::Get(const std::string& endpoint) {
  auto channels = channels_.Lock();
  const auto [it, _] =
      channels->try_emplace(endpoint, endpoint, credentials_, channel_args_);
  return Token(*this, it->first, it->second);
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
