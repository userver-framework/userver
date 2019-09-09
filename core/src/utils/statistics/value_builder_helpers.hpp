#pragma once

#include <string>

#include <formats/json/value_builder.hpp>

namespace utils::statistics {

void SetSubField(formats::json::ValueBuilder& object, const std::string& path,
                 formats::json::ValueBuilder value);

}
