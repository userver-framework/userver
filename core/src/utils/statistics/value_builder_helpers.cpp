#include <utils/statistics/value_builder_helpers.hpp>

#include <userver/formats/common/utils.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

namespace {

std::string MakeConflictMessage(const formats::json::ValueBuilder& original,
                                const formats::json::ValueBuilder& patch) {
  const auto original_value =
      formats::json::ValueBuilder{original}.ExtractValue();
  const auto patch_value = formats::json::ValueBuilder{patch}.ExtractValue();
  return fmt::format("Conflicting metrics at '{}', original='{}', patch='{}'",
                     original.GetPath(), original_value, patch_value);
}

void CheckedMerge(formats::json::ValueBuilder& original,
                  formats::json::ValueBuilder&& patch) {
  if (patch.IsObject() && original.IsObject()) {
    for (const auto& [elem_key, elem_value] : Items(patch)) {
      auto next_origin = original[elem_key];
      CheckedMerge(next_origin, std::move(elem_value));
    }
  } else if (patch.IsNull()) {
    return;  // do nothing
  } else {
    UASSERT_MSG(original.IsNull() ||  // TODO: remove IsNull()
                    formats::json::ValueBuilder{original}.ExtractValue() ==
                        formats::json::ValueBuilder{patch}.ExtractValue(),
                MakeConflictMessage(original, patch));
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

std::optional<std::string> FindNonNumberMetricPath(
    const formats::json::Value& json) {
  for (const auto& [name, value] : Items(json)) {
    if (name == "$meta") {
      continue;
    }
    if (value.IsObject()) {
      auto path = FindNonNumberMetricPath(value);
      if (path.has_value()) {
        return path;
      }
    } else if (value.IsInt() || value.IsInt64() || value.IsUInt64() ||
               value.IsDouble() || value.IsNull()) {  // TODO: remove IsNull()
      continue;
    } else {
      return value.GetPath();
    }
  }

  return std::nullopt;
}

bool AreAllMetricsNumbers(const formats::json::Value& json) {
  const auto path = FindNonNumberMetricPath(json);
  UASSERT_MSG(!path.has_value(),
              "Some metrics are not numbers, path: " + *path + ". Value: " +
                  ToString(formats::common::GetAtPath(
                      json, formats::common::SplitPathString(*path))));
  return true;
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
