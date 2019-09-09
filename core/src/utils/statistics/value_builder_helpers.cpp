#include <utils/statistics/value_builder_helpers.hpp>

#include <list>

#include <boost/algorithm/string.hpp>

namespace utils::statistics {

void UpdateFields(formats::json::ValueBuilder& object,
                  formats::json::ValueBuilder value) {
  for (auto it = value.begin(); it != value.end(); ++it) {
    const auto& name = it.GetName();
    object[name] = value[name];
  }
}

void SetSubField(formats::json::ValueBuilder& object,
                 std::list<std::string> fields,
                 formats::json::ValueBuilder value) {
  if (fields.empty()) {
    UpdateFields(object, std::move(value));
  } else {
    const auto field = std::move(fields.front());
    fields.pop_front();
    if (field.empty())
      SetSubField(object, std::move(fields), std::move(value));
    else {
      auto subobject = object[field];
      SetSubField(subobject, std::move(fields), std::move(value));
    }
  }
}

void SetSubField(formats::json::ValueBuilder& object, const std::string& path,
                 formats::json::ValueBuilder value) {
  std::list<std::string> fields;
  boost::split(fields, path, [](char c) { return c == '.'; });
  SetSubField(object, std::move(fields), std::move(value));
}

}  // namespace utils::statistics
