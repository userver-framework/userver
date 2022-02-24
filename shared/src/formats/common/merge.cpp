#include <userver/formats/common/merge.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::common {

void Merge(formats::json::ValueBuilder& original,
           const formats::json::Value& patch) {
  if (patch.IsObject() && original.IsObject() && !original.IsEmpty()) {
    for (const auto& [elem_key, elem_value] : Items(patch)) {
      auto next_origin = original[elem_key];
      Merge(next_origin, elem_value);
    }
  } else if (!patch.IsMissing()) {
    original = patch;
  }
}

}  // namespace formats::common

USERVER_NAMESPACE_END
