#pragma once

#include <string>
#include <vector>

#include <userver/formats/json/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

void SetSubField(formats::json::ValueBuilder& object,
                 std::vector<std::string>&& path,
                 formats::json::ValueBuilder&& value);

std::string JoinPath(const std::vector<std::string>& path);

}  // namespace utils::statistics

USERVER_NAMESPACE_END
