#include <concurrent/impl/striped_array.hpp>

#ifdef USERVER_IMPL_HAS_RSEQ

#include <memory>  // for std::uninitialized_fill_n

#include <concurrent/impl/intrusive_stack.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

struct StripedArrayNode final {
  explicit StripedArrayNode(std::intptr_t* array) : array(array) {}

  SinglyLinkedHook<StripedArrayNode> hook;
  std::intptr_t* const array;
};

namespace {

bool IsHeadNode(StripedArrayNode& node) {
  return reinterpret_cast<std::uintptr_t>(node.array) %
             kDestructiveInterferenceSize ==
         0;
}

struct StripedArrayStorage final {
  // IntrusiveStack degrades performance under contention.
  // Suppose that StripedArray's are not created-destroyed very often.
  using Pool =
      IntrusiveStack<StripedArrayNode, MemberHook<&StripedArrayNode::hook>>;

  StripedArrayStorage() = default;

  ~StripedArrayStorage() {
    Pool nodes_to_delete;
    while (auto* const node = pool.TryPop()) {
      if (IsHeadNode(*node)) {
        nodes_to_delete.Push(*node);
      }
    }
    nodes_to_delete.DisposeUnsafe([](StripedArrayNode& node) {
      ::operator delete(&node, std::align_val_t{kDestructiveInterferenceSize});
    });
  }

  Pool pool;
};

StripedArrayStorage striped_array_storage;

StripedArrayNode& AcquireStripedArrayNode() {
  // If a StripedArray is created with static storage duration, its init and
  // deinit order may clash with 'striped_array_storage'.
  utils::impl::AssertStaticRegistrationFinished();

  // Optimistic path: if 'pool' is not empty, take from it.
  if (auto* const node = striped_array_storage.pool.TryPop()) {
    return *node;
  }

  static_assert(alignof(StripedArrayNode) == sizeof(std::intptr_t));
  // So that 'arrays' are aligned automatically.
  static_assert(kDestructiveInterferenceSize % sizeof(StripedArrayNode) == 0);
  static_assert(StripedArray::kStride * sizeof(std::intptr_t) ==
                kDestructiveInterferenceSize);

  const auto rseq_array_size = GetRseqArraySize();
  auto& pool = striped_array_storage.pool;

  // Fused allocation:
  // 1. nodes: StripedArrayNode[kStride]
  // 2. arrays: rseq_array_size x std::intptr_t[kStride]
  auto* const buffer = static_cast<std::byte*>(::operator new(
      StripedArray::kStride * sizeof(StripedArrayNode) +
          rseq_array_size * StripedArray::kStride * sizeof(std::intptr_t),
      std::align_val_t{kDestructiveInterferenceSize}));

  auto* const nodes = reinterpret_cast<StripedArrayNode*>(buffer);
  auto* const arrays = reinterpret_cast<std::intptr_t*>(
      buffer + StripedArray::kStride * sizeof(StripedArrayNode));

  // Each node owns a strided range within 'arrays'.
  for (std::size_t i = 1; i < StripedArray::kStride; ++i) {
    pool.Push(*::new (&nodes[i]) StripedArrayNode(arrays + i));
  }
  auto& head_node = *::new (&nodes[0]) StripedArrayNode(arrays + 0);
  UASSERT(IsHeadNode(head_node));
  return head_node;
}

void ReleaseStripedArrayNode(StripedArrayNode& node) noexcept {
  // All the memory for StripedArray's is only deallocated during static
  // destruction.
  static_assert(noexcept(striped_array_storage.pool.Push(node)));
  return striped_array_storage.pool.Push(node);
}

}  // namespace

StripedArray::StripedArray()
    : node_(AcquireStripedArrayNode()), array_(node_.array) {
  const auto elements_view = Elements();
  // This will induce some cache line flushing for neighbor StripedArray's.
  // Suppose that StripedArray's are not created very often.
  std::uninitialized_fill(elements_view.begin(), elements_view.end(), 0);
}

StripedArray::~StripedArray() { ReleaseStripedArrayNode(node_); }

}  // namespace concurrent::impl

USERVER_NAMESPACE_END

#endif
