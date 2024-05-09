#include <userver/kafka/consumer_scope.hpp>

#include <kafka/impl/consumer.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka {

ConsumerScope::ConsumerScope(impl::Consumer& consumer) noexcept
    : consumer_(consumer) {}

ConsumerScope::~ConsumerScope() { Stop(); }

void ConsumerScope::Start(Callback callback) {
  consumer_.StartMessageProcessing(std::move(callback));
}

void ConsumerScope::Stop() noexcept { consumer_.Stop(); }

void ConsumerScope::AsyncCommit() { consumer_.AsyncCommit(); }

}  // namespace kafka

USERVER_NAMESPACE_END
