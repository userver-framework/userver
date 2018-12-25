#pragma once

#include <string>

#include <boost/filesystem/operations.hpp>
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
                                   std::string contents,
                                   boost::filesystem::perms perms);

void Chmod(engine::TaskProcessor& async_tp, const std::string& path,
           boost::filesystem::perms perms);

}  // namespace fs
}  // namespace async
