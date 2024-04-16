#include "are_equal.hpp"

#include <algorithm>

#include <rapidjson/document.h>
#include <boost/container/small_vector.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using Value = formats::json::impl::Value;

std::string_view AsStringView(const Value& value) {
  return {value.GetString(), value.GetStringLength()};
}

bool PartiallyOrderedStringCmp(std::string_view lhs, std::string_view rhs) {
  const auto lhs_size = lhs.size();
  const auto rhs_size = rhs.size();

  return std::tie(lhs_size, lhs) < std::tie(rhs_size, rhs);
}

class ParallelTraverseQueue final {
 public:
  using ValuePtr = const Value*;

  std::pair<ValuePtr, ValuePtr> Pop() noexcept {
    UASSERT(start_index_ < buffer_.size());
    return buffer_[start_index_++];
  }
  void Push(ValuePtr lhr, ValuePtr rhs) { buffer_.emplace_back(lhr, rhs); }
  bool Empty() const noexcept { return start_index_ == buffer_.size(); }

 private:
  boost::container::small_vector<std::pair<ValuePtr, ValuePtr>, 32> buffer_;
  std::size_t start_index_ = 0;
};

enum class SurfaceCheckResult { kPass, kNeedsDeeperCheck, kDeny };

SurfaceCheckResult SurfaceCheck(const Value* lhs, const Value* rhs) {
  if (lhs == rhs) {
    return SurfaceCheckResult::kPass;
  }

  if (lhs == nullptr || rhs == nullptr || lhs->GetType() != rhs->GetType()) {
    return SurfaceCheckResult::kDeny;
  }

  if (lhs->IsObject()) {
    if (lhs->MemberCount() != rhs->MemberCount()) {
      return SurfaceCheckResult::kDeny;
    }
    return SurfaceCheckResult::kNeedsDeeperCheck;
  }

  if (lhs->IsArray()) {
    if (lhs->Size() != rhs->Size()) {
      return SurfaceCheckResult::kDeny;
    }
    return SurfaceCheckResult::kNeedsDeeperCheck;
  }

  if (*lhs == *rhs) {
    return SurfaceCheckResult::kPass;
  }
  return SurfaceCheckResult::kDeny;
}

bool AreNotEqualObjectsThin(const Value* lhs, const Value* rhs,
                            ParallelTraverseQueue& traverse) {
  // both lhs and rhs are expected to be objects with the same size

  const auto size = lhs->MemberCount();
  const auto l_members = lhs->MemberBegin();
  const auto r_members = rhs->MemberBegin();
  for (std::size_t i = 0; i < size; ++i) {
    const auto& l_member = l_members[i];
    const auto& l_name = AsStringView(l_member.name);
    std::size_t j = 0;
    for (; j < size; ++j) {
      const auto& r_member = r_members[j];
      if (l_name != AsStringView(r_member.name)) {
        continue;
      }

      auto surf = SurfaceCheck(&l_member.value, &r_member.value);
      if (surf == SurfaceCheckResult::kPass) {
        break;
      }
      if (surf == SurfaceCheckResult::kDeny) {
        return true;
      }
      traverse.Push(&l_member.value, &r_member.value);
      break;
    }

    if (j == size) return true;
  }

  return false;
}

bool AreNotEqualObjects(const Value* lhs, const Value* rhs,
                        ParallelTraverseQueue& traverse) {
  // both lhs and rhs are expected to be objects with the same size

  const auto size = lhs->MemberCount();
  if (size <= 32) {
    return AreNotEqualObjectsThin(lhs, rhs, traverse);
  }

  boost::container::small_vector<std::pair<std::string_view, const Value*>, 64>
      lhs_object_kvs;

  const auto l_members = lhs->MemberBegin();
  for (std::size_t i = 0; i < size; ++i) {
    lhs_object_kvs.emplace_back(AsStringView(l_members[i].name),
                                &l_members[i].value);
  }
  std::sort(lhs_object_kvs.begin(), lhs_object_kvs.end(),
            [](const auto& l, const auto& r) {
              return PartiallyOrderedStringCmp(l.first, r.first);
            });

  const auto r_members = rhs->MemberBegin();
  for (std::size_t i = 0; i < size; ++i) {
    const auto r_name = AsStringView(r_members[i].name);
    const auto matching_lhs_it =
        std::lower_bound(lhs_object_kvs.begin(), lhs_object_kvs.end(), r_name,
                         [](const auto& check, const auto& key) {
                           return PartiallyOrderedStringCmp(check.first, key);
                         });
    if (matching_lhs_it == lhs_object_kvs.end() ||
        matching_lhs_it->first != r_name) {
      return true;
    }

    auto surf = SurfaceCheck(matching_lhs_it->second, &r_members[i].value);
    if (surf == SurfaceCheckResult::kPass) {
      continue;
    }
    if (surf == SurfaceCheckResult::kDeny) {
      return true;
    }
    traverse.Push(matching_lhs_it->second, &r_members[i].value);
  }

  return false;
}

}  // namespace

namespace formats::json::impl {

bool AreEqual(const Value* lhs_root, const Value* rhs_root) {
  auto root_surf = SurfaceCheck(lhs_root, rhs_root);
  if (root_surf == SurfaceCheckResult::kPass) {
    return true;
  }
  if (root_surf == SurfaceCheckResult::kDeny) {
    return false;
  }

  ParallelTraverseQueue traverse;
  traverse.Push(lhs_root, rhs_root);

  while (!traverse.Empty()) {
    const auto [lhs, rhs] =
        traverse.Pop();  // Guaranteed to have the same object or array type and
                         // the same size
    if (lhs->IsArray()) {
      const auto& l_arr = *lhs;
      const auto& r_arr = *rhs;
      for (std::size_t i = 0; i < lhs->Size(); ++i) {
        auto surf = SurfaceCheck(&l_arr[i], &r_arr[i]);
        if (surf == SurfaceCheckResult::kPass) {
          continue;
        }
        if (surf == SurfaceCheckResult::kDeny) {
          return false;
        }
        traverse.Push(&l_arr[i], &r_arr[i]);
      }
    } else if (AreNotEqualObjects(lhs, rhs, traverse)) {
      return false;
    }
  }

  return true;
}

}  // namespace formats::json::impl

USERVER_NAMESPACE_END
