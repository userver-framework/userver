#include <formats/json/serialize_to.hpp>

#include <utils/datetime.hpp>

namespace formats::json {

formats::json::Value SerializeToJson(std::chrono::system_clock::time_point tp) {
  formats::json::ValueBuilder builder =
      utils::datetime::Timestring(tp, "UTC", utils::datetime::kRfc3339Format);
  return builder.ExtractValue();
}

}  // namespace formats::json
