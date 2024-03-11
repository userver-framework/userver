#include "json_tree.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>

#include <rapidjson/document.h>

#include <userver/formats/common/path.hpp>
#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/impl/types.hpp>

USERVER_NAMESPACE_BEGIN

namespace {
const std::string kInvalidPathStr = "<not-part-of-json-tree>";

using Value = formats::json::impl::Value;
using Member = formats::json::impl::Value::MemberIterator::value_type;
using TreeIterFrame = formats::json::impl::TreeIterFrame;

/// Use pointer arithmetic to get `rapidjson::GenericMember` out of `Value`
const Member* MemberFromValue(const Value* val) {
  return reinterpret_cast<const Member*>(reinterpret_cast<const char*>(val) -
                                         offsetof(Member, value));
}

constexpr std::size_t kLookupFailed = std::numeric_limits<std::size_t>::max();

/// Given beginning and length of an array, check if `member` pointer belongs to
/// it and return its index or -1 on failure
template <typename TValue>
std::size_t FastLookup(const TValue* member, const TValue* first,
                       std::size_t size) {
  return member >= first && member < first + size ? member - first
                                                  : kLookupFailed;
}

/// Return `true` if member had been found in `container` and update `stack` so
/// `member`'s path can be calculated
template <typename TValue, typename TValidateAddress>
bool ProcessContainer(formats::json::impl::TreeStack& stack,
                      const Value* container, const TValue* member,
                      int node_depth, const TValue* first, std::size_t size,
                      const TValidateAddress& extra_check) {
  const int depth = stack.size();
  if (depth == node_depth) {
    const std::size_t index = FastLookup(member, first, size);
    if (index != kLookupFailed && extra_check(first + index)) {
      stack.emplace_back(container, index + 1);
      return true;
    }
  } else if (depth < node_depth) {
    stack.emplace_back(container);
  }
  return false;
}
}  // namespace

namespace formats::json::impl {

std::string MakePath(const Value* root, const Value* node, int node_depth) {
  TreeStack stack;
  const Value* value = root;

  if (value == node || value == nullptr) return formats::common::kPathRoot;

  stack.reserve(node_depth + 1);
  stack.emplace_back();  // fake "top" frame to avoid extra checks for an empty
                         // stack inside walker loop
  for (;;) {
    stack.back().Advance();

    if (value->IsObject() && value->MemberCount() > 0) {
      if (ProcessContainer(
              stack, value, MemberFromValue(node), node_depth,
              &*value->MemberBegin(), value->MemberCount(),
              [node](const Member* m) { return &m->value == node; }))
        break;
    } else if (value->IsArray() && value->Size() > 0) {
      if (ProcessContainer(stack, value, node, node_depth, &*value->Begin(),
                           value->Size(),
                           [node](const Value* v) { return v == node; }))
        break;
    }

    while (!stack.back().HasMoreElements()) {
      stack.pop_back();
      if (stack.empty()) return kInvalidPathStr;
    }

    value = stack.back().CurrentValue();
  }

  return ExtractPath(stack);
}

std::string ExtractPath(const TreeStack& stack) {
  std::string path;
  for (size_t depth = 1; depth < stack.size(); depth++) {
    const auto& frame = stack[depth];
    // because index in stack has already been Advance'd
    const auto idx = frame.CurrentIndex() - 1;
    if (frame.container()->IsObject()) {
      const Value& name = frame.container()->MemberBegin()[idx].name;
      common::AppendPath(
          path, std::string_view{name.GetString(), name.GetStringLength()});
    } else if (frame.container()->IsArray()) {
      common::AppendPath(path, idx);
    }
  }

  return path.empty() ? common::kPathRoot : path;
}
}  // namespace formats::json::impl

USERVER_NAMESPACE_END
