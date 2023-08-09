#include "make_error.hpp"
#include <userver/formats/json/value_builder.hpp>

namespace real_medium::utils::error {
userver::formats::json::Value MakeError(std::string_view field_name,
                                        std::string_view message) {
  userver::formats::json::ValueBuilder error_builder;
  error_builder["errors"][field_name.data()].PushBack(message);
  return error_builder.ExtractValue();
}
}  // namespace real_medium::utils