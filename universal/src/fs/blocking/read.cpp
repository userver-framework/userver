#include <userver/fs/blocking/read.hpp>
#include <userver/fs/path_utils.hpp>
#include <userver/utils/assert.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>

USERVER_NAMESPACE_BEGIN

namespace fs::blocking {

namespace {

bool IsHiddenFile(const std::filesystem::path& path) {
    auto name = path.filename().native();
    UASSERT(!name.empty());
    return name != ".." && name != "." && name[0] == '.';
}

}  // namespace

std::string ReadFileContents(utils::zstring_view path) {
    const std::ifstream ifs(path.c_str());
    if (!ifs) {
        throw std::runtime_error(fmt::format("Error opening '{}'", path));
    }

    std::ostringstream buffer;
    buffer << ifs.rdbuf();
    return buffer.str();
}

bool FileExists(utils::zstring_view path) { return std::filesystem::exists(path.c_str()); }

std::filesystem::file_type GetFileType(utils::zstring_view path) {
    return std::filesystem::status(path.c_str()).type();
}

FileInfoWithDataMap ReadRecursiveFilesInfoWithData(
    utils::zstring_view path,
    utils::Flags<SettingsReadFile> flags,
    std::size_t max_size_to_cache
) {
    FileInfoWithDataMap data{};
    for (auto it = std::filesystem::recursive_directory_iterator(path.c_str());
         it != std::filesystem::recursive_directory_iterator();
         ++it)
    {
        // only files
        if (it->status().type() != std::filesystem::file_type::regular) {
            continue;
        }
        if ((flags & SettingsReadFile::kSkipHidden) && IsHiddenFile(it->path())) {
            continue;
        }
        FileInfoWithData info{};
        info.extension = it->path().extension().string();
        if (info.extension.empty() && IsHiddenFile(it->path())) {
            info.extension = it->path().filename().string();
        }
        const auto file_size = std::filesystem::file_size(it->path());
        if (file_size > max_size_to_cache) {
            info.data_or_path = std::filesystem::absolute(it->path());
        } else {
            info.data_or_path = ReadFileContents(it->path().string());
        }
        data[GetLexicallyRelative(it->path().string(), path)] = std::make_shared<const FileInfoWithData>(std::move(info)
        );
    }
    return data;
}

}  // namespace fs::blocking

USERVER_NAMESPACE_END
