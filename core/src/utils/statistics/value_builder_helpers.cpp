#include <utils/statistics/value_builder_helpers.hpp>

#include <boost/algorithm/string/join.hpp>

#include <userver/formats/common/utils.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

namespace {

void CheckedMerge(formats::json::ValueBuilder& original,
                  formats::json::ValueBuilder&& patch) {
  if (patch.IsObject() && original.IsObject()) {
    for (const auto& [elem_key, elem_value] : Items(patch)) {
      auto next_origin = original[elem_key];
      CheckedMerge(next_origin, std::move(elem_value));
    }
  } else {
    UASSERT_MSG(original.IsNull(), fmt::format("Conflicting metrics at '{}'",
                                               patch.ExtractValue().GetPath()));
    original = std::move(patch);
  }
}

}  // namespace

void SetSubField(formats::json::ValueBuilder& object,
                 std::vector<std::string>&& path,
                 formats::json::ValueBuilder&& value) {
  if (path.empty()) {
    CheckedMerge(object, std::move(value));
  } else {
    auto child = formats::common::GetAtPath(object, std::move(path));
    CheckedMerge(child, std::move(value));
  }
}

std::string JoinPath(const std::vector<std::string>& path) {
  return boost::algorithm::join(path, ".");
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
