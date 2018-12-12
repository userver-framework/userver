#pragma once

#include <string>

namespace blocking {
namespace fs {

/* Read contents of file in fs. On error throws an exception. */
std::string ReadFileContents(const std::string& path);

}  // namespace fs
}  // namespace blocking
