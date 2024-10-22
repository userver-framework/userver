#pragma once

#include <memory>

#include <grpcpp/completion_queue.h>

#include <userver/utils/fixed_array.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

class QueueRunner;

class CompletionQueuePoolBase {
public:
    CompletionQueuePoolBase(CompletionQueuePoolBase&&) = delete;
    CompletionQueuePoolBase& operator=(CompletionQueuePoolBase&&) = delete;

    std::size_t GetSize() const { return queues_.size(); }

    grpc::CompletionQueue& GetQueue(std::size_t idx) { return *queues_[idx]; }

    grpc::CompletionQueue& NextQueue();

protected:
    explicit CompletionQueuePoolBase(utils::FixedArray<std::unique_ptr<grpc::CompletionQueue>> queues);

    // protected to prevent destruction via pointer to base.
    ~CompletionQueuePoolBase();

private:
    utils::FixedArray<std::unique_ptr<grpc::CompletionQueue>> queues_;
    utils::FixedArray<QueueRunner> queue_runners_;
};

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
