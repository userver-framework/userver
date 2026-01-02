#include <userver/fs/read.hpp>

#include <userver/engine/async.hpp>
#include <userver/fs/blocking/read.hpp>

USERVER_NAMESPACE_BEGIN

namespace fs {

std::string ReadFileContents(engine::TaskProcessor& async_tp, const std::string& path) {
    return engine::AsyncNoSpan(async_tp, &fs::blocking::ReadFileContents, path).Get();
}

FileInfoWithDataMap ReadRecursiveFilesInfoWithData(
    engine::TaskProcessor& async_tp,
    const std::string& path,
    SettingReadFileFlags flags
) {
    return engine::AsyncNoSpan(async_tp, &fs::blocking::ReadRecursiveFilesInfoWithData, std::cref(path), flags).Get();
}

bool FileExists(engine::TaskProcessor& async_tp, const std::string& path) {
    return engine::AsyncNoSpan(async_tp, &fs::blocking::FileExists, path).Get();
}

}  // namespace fs

USERVER_NAMESPACE_END
