#include <userver/fs/blocking/write.hpp>

#include <sys/stat.h>
#include <cerrno>
#include <system_error>

#include <fmt/format.h>

#include <userver/fs/blocking/file_descriptor.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/boost_uuid4.hpp>
#include <userver/utils/fast_scope_guard.hpp>

USERVER_NAMESPACE_BEGIN

namespace fs::blocking {

namespace {

void CreateDirectory(const char* path, std::filesystem::perms perms) {
    if (::mkdir(path, static_cast<::mode_t>(perms)) == -1) {
        const auto code = errno;
        if (code != EEXIST) {
            throw std::system_error(
                code,
                std::generic_category(),
                fmt::format("Error while creating directory \"{}\"", path)
            );
        }
    }
}

}  // namespace

void CreateDirectories(std::string_view path, std::filesystem::perms perms) {
    UASSERT(!path.empty());

    std::string mutable_path;
    mutable_path.reserve(path.size() + 2);
    mutable_path.append(path);
    mutable_path.push_back('/');
    mutable_path.push_back('\0');

    for (char* p = mutable_path.data() + 1; *p != '\0'; ++p) {
        if (*p == '/') {
            *p = '\0';  // temporarily truncate
            CreateDirectory(mutable_path.data(), perms);
            *p = '/';
        }
    }
}

void CreateDirectories(std::string_view path) {
    using std::filesystem::perms;
    const auto
        perms0755 = perms::owner_all | perms::group_read | perms::group_exec | perms::others_read | perms::others_exec;
    CreateDirectories(path, perms0755);
}

void RewriteFileContents(utils::zstring_view path, std::string_view contents) {
    constexpr OpenMode flags{OpenFlag::kWrite, OpenFlag::kCreateIfNotExists, OpenFlag::kTruncate};
    auto fd = FileDescriptor::Open(path, flags);

    fd.Write(contents);
    std::move(fd).Close();
}

void RewriteFileContentsFSync(utils::zstring_view path, std::string_view contents) {
    constexpr OpenMode flags{OpenFlag::kWrite, OpenFlag::kCreateIfNotExists, OpenFlag::kTruncate};
    auto fd = FileDescriptor::Open(path, flags);
    fd.Write(contents);
    fd.FSync();

    std::move(fd).Close();
}

void SyncDirectoryContents(utils::zstring_view path) {
    auto fd = FileDescriptor::OpenDirectory(path);
    fd.FSync();
    std::move(fd).Close();
}

void Rename(utils::zstring_view source, utils::zstring_view destination) {
    std::filesystem::rename(source.c_str(), destination.c_str());
}

void RewriteFileContentsAtomically(
    utils::zstring_view path,
    std::string_view contents,
    std::filesystem::perms perms,
    std::filesystem::perm_options options
) {
    const auto tmp_path = fmt::format("{}{}.tmp", path, utils::generators::GenerateBoostUuid());
    const std::filesystem::path filesystem_tmp_path{tmp_path};
    const auto directory = filesystem_tmp_path.parent_path().string();

    utils::FastScopeGuard guard{[&filesystem_tmp_path]() noexcept {
        std::error_code ignore_error;
        std::filesystem::remove(filesystem_tmp_path, ignore_error);
    }};
    fs::blocking::RewriteFileContentsFSync(tmp_path, contents);
    fs::blocking::Chmod(tmp_path, perms, options);
    fs::blocking::Rename(tmp_path, path);
    guard.Release();

    fs::blocking::SyncDirectoryContents(directory);
}

void Chmod(utils::zstring_view path, std::filesystem::perms perms, std::filesystem::perm_options options) {
    UINVARIANT(!std::filesystem::is_symlink(path.c_str()), "Changing permissions on a symlink is ambiguous");
    std::filesystem::permissions(path.c_str(), perms, options);
}

bool RemoveSingleFile(utils::zstring_view path) { return std::filesystem::remove(path.c_str()); }

}  // namespace fs::blocking

USERVER_NAMESPACE_END
