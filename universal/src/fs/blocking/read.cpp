#include <userver/fs/blocking/read.hpp>
#include <userver/fs/path_utils.hpp>
#include <userver/utils/assert.hpp>

#include <fstream>
#include <sstream>

#include <boost/filesystem/directory.hpp>
#include <boost/filesystem/operations.hpp>

USERVER_NAMESPACE_BEGIN

namespace fs::blocking {

namespace {

bool IsHiddenFile(const boost::filesystem::path& path) {
    auto name = path.filename().native();
    UASSERT(!name.empty());
    return name != ".." && name != "." && name[0] == '.';
}

}  // namespace

std::string ReadFileContents(const std::string& path) {
    const std::ifstream ifs(path);
    if (!ifs) {
        throw std::runtime_error("Error opening '" + path + '\'');
    }

    std::ostringstream buffer;
    buffer << ifs.rdbuf();
    return buffer.str();
}

bool FileExists(const std::string& path) { return boost::filesystem::exists(path); }

boost::filesystem::file_type GetFileType(const std::string& path) { return boost::filesystem::status(path).type(); }

FileInfoWithDataMap ReadRecursiveFilesInfoWithData(const std::string& path, utils::Flags<SettingsReadFile> flags) {
    FileInfoWithDataMap data{};
    for (auto it = boost::filesystem::recursive_directory_iterator(path);
         it != boost::filesystem::recursive_directory_iterator();
         ++it)
    {
        // only files
        if (it->status().type() != boost::filesystem::regular_file) {
            continue;
        }
        if ((flags & SettingsReadFile::kSkipHidden) && IsHiddenFile(it->path())) {
            continue;
        }
        FileInfoWithData info{};
        info.extension = it->path().extension().string();
        info.data = ReadFileContents(it->path().string());
        data[GetLexicallyRelative(it->path().string(), path)] = std::make_shared<const FileInfoWithData>(std::move(info)
        );
    }
    return data;
}

}  // namespace fs::blocking

USERVER_NAMESPACE_END
