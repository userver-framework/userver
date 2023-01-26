#include <concurrent/impl/intrusive_mpsc_queue.hpp>

#include <compiler/relax_cpu.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/fast_scope_guard.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

namespace {

using NodePtr = IntrusiveMpscQueueImpl::NodePtr;
using NodeRef = IntrusiveMpscQueueImpl::NodeRef;

NodeRef BlockThreadUntilNotNull(std::atomic<NodePtr>& next) noexcept {
  NodePtr next_ptr = next.load(std::memory_order_acquire);
  compiler::RelaxCpu relax;
  while (next_ptr == nullptr) {
    relax();
    next_ptr = next.load(std::memory_order_acquire);
  }
  return *next_ptr;
}

}  // namespace

void IntrusiveMpscQueueImpl::Push(NodeRef node) noexcept {
  UASSERT(GetNext(node).load(std::memory_order_relaxed) == nullptr);
  const NodeRef prev = head_->exchange(node, std::memory_order_acq_rel);

  // If the consumer reads 'prev' right here (between 'xchg' and 'mov' on
  // x86), the consumer will be momentarily blocked.

  GetNext(prev).store(node, std::memory_order_release);
}

IntrusiveMpscQueueImpl::NodePtr IntrusiveMpscQueueImpl::TryPop() noexcept {
  UASSERT_MSG(!is_consuming_.exchange(true),
              "Multiple concurrent consumers detected");
  const utils::FastScopeGuard guard([this]() noexcept {
    UASSERT_MSG(is_consuming_.exchange(false),
                "Multiple concurrent consumers detected");
  });

  NodeRef tail = tail_;
  NodePtr next = GetNext(tail).load(std::memory_order_acquire);

  if (tail == &stub_) {
    if (next == nullptr) {
      // An addition to the base algorithm. We check if the queue is really
      // empty, or if a Push is in process. We do this because other nodes may
      // have already been pushed after the blocking node.
      if (tail == head_->load(std::memory_order_acquire)) {
        // The queue is logically empty.
        return nullptr;
      }

      // A node is being pushed after the stub_ node.
      next = BlockThreadUntilNotNull(GetNext(tail));
    }

    // The queue is no longer empty, discard the stub_ node.
    GetNext(stub_).store(nullptr, std::memory_order_relaxed);
    tail_ = *next;
    tail = *next;
    next = GetNext(tail).load(std::memory_order_acquire);
  }

  if (next != nullptr) {
    // Happy path: there are more nodes after 'tail', pop 'tail'.
    tail_ = *next;
    GetNext(tail).store(nullptr, std::memory_order_relaxed);
    return tail;
  }

  // There seems to be no nodes after 'tail'. To remove it, we first have to
  // push 'stub_' so that the node list is never empty (otherwise it would be
  // hard to correct both head_ and tail_ in Push).
  NodeRef head = head_->load(std::memory_order_acquire);
  if (head == tail &&
      head_->compare_exchange_strong(head, stub_, std::memory_order_release,
                                     std::memory_order_relaxed)) {
    tail_ = stub_;
    UASSERT(GetNext(tail).load(std::memory_order_relaxed) == nullptr);
    return tail;
  }

  // A node is actually being pushed after 'tail', 'tail.next' has just not been
  // corrected yet.
  next = BlockThreadUntilNotNull(GetNext(tail));

  tail_ = *next;
  GetNext(tail).store(nullptr, std::memory_order_relaxed);
  return tail;
}

std::atomic<NodePtr>& IntrusiveMpscQueueImpl::GetNext(NodeRef node) noexcept {
  return node->singly_linked_hook.next_;
}

}  // namespace concurrent::impl

USERVER_NAMESPACE_END
