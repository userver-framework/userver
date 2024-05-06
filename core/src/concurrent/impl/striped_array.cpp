#include <concurrent/impl/striped_array.hpp>

#ifdef USERVER_IMPL_HAS_RSEQ

#include <memory>  // for std::uninitialized_fill_n

#include <userver/compiler/impl/constexpr.hpp>
#include <userver/compiler/impl/lsan.hpp>
#include <userver/concurrent/impl/intrusive_stack.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

struct StripedArrayNode final {
  explicit StripedArrayNode(std::intptr_t* array) : array(array) {}

  SinglyLinkedHook<StripedArrayNode> hook{};
  std::intptr_t* const array;
};

namespace {

// IntrusiveStack degrades performance under contention.
// Suppose that StripedArray's are not created-destroyed very often.
using StripedArrayStorage =
    IntrusiveStack<StripedArrayNode, MemberHook<&StripedArrayNode::hook>>;
USERVER_IMPL_CONSTINIT StripedArrayStorage striped_array_storage;

// To avoid static destruction order fiasco.
static_assert(std::is_trivially_destructible_v<StripedArrayStorage>);

StripedArrayNode& AcquireStripedArrayNode() {
  // Optimistic path: if 'pool' is not empty, take from it.
  if (auto* const node = striped_array_storage.TryPop()) {
    return *node;
  }

  static_assert(alignof(StripedArrayNode) == sizeof(std::intptr_t));
  // So that 'arrays' are aligned automatically.
  static_assert(kDestructiveInterferenceSize % sizeof(StripedArrayNode) == 0);
  static_assert(StripedArray::kStride * sizeof(std::intptr_t) ==
                kDestructiveInterferenceSize);

  const auto rseq_array_size = GetRseqArraySize();

  // Fused allocation:
  // 1. nodes: StripedArrayNode[kStride]
  // 2. arrays: rseq_array_size x std::intptr_t[kStride]
  auto* const buffer = static_cast<std::byte*>(::operator new(
      StripedArray::kStride * sizeof(StripedArrayNode) +
          rseq_array_size * StripedArray::kStride * sizeof(std::intptr_t),
      std::align_val_t{kDestructiveInterferenceSize}));

  // During static destruction, the nodes in striped_array_storage are
  // intentionally leaked, otherwise it's impossible to allow creating
  // StripedArray instances with static storage.
  compiler::impl::LsanIgnoreObject(buffer);

  auto* const nodes = reinterpret_cast<StripedArrayNode*>(buffer);
  auto* const arrays = reinterpret_cast<std::intptr_t*>(
      buffer + StripedArray::kStride * sizeof(StripedArrayNode));

  // Each node owns a strided range within 'arrays'.
  for (std::size_t i = 1; i < StripedArray::kStride; ++i) {
    striped_array_storage.Push(*::new (&nodes[i]) StripedArrayNode(arrays + i));
  }
  auto& head_node = *::new (&nodes[0]) StripedArrayNode(arrays + 0);
  return head_node;
}

void ReleaseStripedArrayNode(StripedArrayNode& node) noexcept {
  static_assert(noexcept(striped_array_storage.Push(node)));
  return striped_array_storage.Push(node);
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
