#pragma once

#include <string>

#include <userver/formats/json/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

void SetSubField(formats::json::ValueBuilder& object, const std::string& path,
                 formats::json::ValueBuilder value);

}

USERVER_NAMESPACE_END
