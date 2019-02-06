#pragma once

#include <string>

#include <engine/task/task_processor_fwd.hpp>

namespace async {
namespace fs {

std::string ReadFileContents(engine::TaskProcessor& async_tp,
                             const std::string& path);
bool FileExists(engine::TaskProcessor& async_tp, const std::string& path);

}  // namespace fs
}  // namespace async
