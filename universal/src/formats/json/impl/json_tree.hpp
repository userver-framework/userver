#pragma once

#include <vector>

#include <rapidjson/document.h>
#include <boost/container/small_vector.hpp>

#include <userver/formats/json/impl/types.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json::impl {
constexpr std::size_t kInitialStackDepth = 32;

/// Artificial "stack frame" for tree iterator
class TreeIterFrame {
 public:
  /// special "empty" frame
  TreeIterFrame() = default;

  /// frame pointing to non-first element of object
  TreeIterFrame(const Value* container, std::size_t index)
      : container_{container}, current_index_{index} {
    if (container->IsArray())
      end_ = container->Size();
    else if (container->IsObject())
      end_ = container->MemberCount();
    else {
      UASSERT_MSG(false, "TreeIterFrame called on non-container element");
      end_ = 1;  // do our best to not crash with -DNDEBUG
    }
  }

  /// initialize new frame from rapidjson value
  TreeIterFrame(const Value* container) : TreeIterFrame(container, 0) {}

  /// Returns currently "selected" value from frame.
  /// For non-containers always returns container
  const Value* CurrentValue() {
    UASSERT(current_index_ < end_);
    if (container_->IsArray()) {
      return &container_->Begin()[current_index_];
    } else if (container_->IsObject()) {
      return &container_->MemberBegin()[current_index_].value;
    }
    UASSERT_MSG(false,
                "container_ was an Object or an Array and now it is not, "
                "possible memory corruption");
    return container_;  // do not crash with -DNDEBUG
  }

  void Advance() { current_index_++; }

  bool HasMoreElements() const { return current_index_ < end_; }

  std::size_t CurrentIndex() const { return current_index_; }
  const Value* container() const { return container_; }

 private:
  /// pointer to container element being iterated
  const Value* container_ = nullptr;
  /// index of currently "selected" element
  std::size_t current_index_ = 0;
  /// cached Size of corresponding container (because alignment would waste 4
  /// bytes anyway)
  std::size_t end_ = 0;
};

using TreeStack =
    boost::container::small_vector<TreeIterFrame, kInitialStackDepth>;

/// Build path to `node` by recursively traversing `root`
std::string MakePath(const Value* root, const Value* node, int node_depth);
/// Transform nodes onto stack into string
std::string ExtractPath(const TreeStack& stack);
}  // namespace formats::json::impl

USERVER_NAMESPACE_END
