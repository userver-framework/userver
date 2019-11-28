#pragma once

#include <string>

namespace formats::common::impl {

static constexpr char kPathSeparator = '.';
static constexpr char kPathRoot[] = "/";

std::string MakeChildPath(const std::string& parent, const std::string& key);
std::string MakeChildPath(const std::string& parent, std::size_t index);
}  // namespace formats::common::impl
