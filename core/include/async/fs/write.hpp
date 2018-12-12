#pragma once

#include <string>

#include <engine/task/task_processor_fwd.hpp>

namespace async {
namespace fs {

void RewriteFileContents(engine::TaskProcessor& async_tp,
                         const std::string& path, std::string contents);

void Rename(engine::TaskProcessor& async_tp, const std::string& source,
            const std::string& destination);

/* Write contents to temp file in the same directory,
 * then atomically replaces the destination */
void RewriteFileContentsAtomically(engine::TaskProcessor& async_tp,
                                   const std::string& path,
                                   std::string contents);

}  // namespace fs
}  // namespace async
