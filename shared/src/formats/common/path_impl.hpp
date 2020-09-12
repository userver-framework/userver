#pragma once

#include <string>
#include <string_view>

namespace formats::common::impl {

static constexpr char kPathSeparator = '.';
static constexpr char kPathRoot[] = "/";

void AppendPath(std::string& path, std::string_view key);
void AppendPath(std::string& path, std::size_t index);

std::string MakeChildPath(const std::string& parent, std::string_view key);
std::string MakeChildPath(const std::string& parent, std::size_t index);
}  // namespace formats::common::impl
