#pragma once

/// @file userver/fs/blocking/file_descriptor.hpp
/// @brief @copybrief fs::blocking::FileDescriptor

#include <filesystem>
#include <span>
#include <string_view>

#include <userver/fs/blocking/impl/permissions.hpp>
#include <userver/utils/zstring_view.hpp>

#include <userver/fs/blocking/open_mode.hpp>

extern "C" {
struct iovec;
}

USERVER_NAMESPACE_BEGIN

namespace fs::blocking {

/// @ingroup userver_universal userver_containers
///
/// @brief A file descriptor wrapper
/// @details The file is closed in the destructor
/// @note The operations on the file are blocking and not thread-safe
class FileDescriptor final {
public:
    /// @brief Open a file using `::open`
    /// @throws std::runtime_error
    static FileDescriptor Open(
        utils::zstring_view path,
        OpenMode flags,
        std::filesystem::perms perms = std::filesystem::perms::owner_read | std::filesystem::perms::owner_write
    );

    /// @overload
    /// Accepts `boost::filesystem::perms`.
    /// @deprecated use the std::filesystem overload
    template <impl::LegacyPermissions Permissions>
    static FileDescriptor Open(utils::zstring_view path, OpenMode flags, Permissions perms) {
        return fs::blocking::FileDescriptor::Open(path, flags, impl::ToStdPermissions(perms));
    }

    /// @brief Open a directory node
    /// @note The only valid operation for such a `FileDescriptor` is `FSync`.
    /// @throws std::runtime_error
    static FileDescriptor OpenDirectory(utils::zstring_view path);

    /// @brief Use the file descriptor directly
    static FileDescriptor AdoptFd(int fd) noexcept;

    /// @brief Duplicate the file descriptor
    static FileDescriptor DupFd(int fd) noexcept;

    /// @brief Default constructor
    /// @note The file descriptor is not open
    FileDescriptor();

    FileDescriptor(FileDescriptor&& other) noexcept;
    FileDescriptor& operator=(FileDescriptor&& other) noexcept;
    ~FileDescriptor();

    /// @brief Checks if the file is open
    /// @note Operations can only be performed on an open `FileDescriptor`.
    bool IsOpen() const noexcept;

    /// @brief Closes the file manually
    /// @throws std::runtime_error
    void Close() &&;

    /// Returns the native file handle
    int GetNative() const noexcept;

    /// Passes the ownership of the file descriptor to the caller
    int Release() &&;

    /// @brief Writes data to the file
    /// @warning Unless `FSync` is called, there is no guarantee the data is stored on disk safely.
    /// @throws std::runtime_error
    void Write(std::string_view contents);

    /// @brief Writes data to the file, has no IOV_MAX limits on contents.size()
    /// @warning Unless `FSync` is called, there is no guarantee the data is stored on disk safely.
    /// @throws std::runtime_error
    void Write(std::span<const struct iovec> contents);

    /// @brief Reads data from the file at current offset
    /// @returns The amount of bytes actually acquired, which can be equal to `max_size`, or less on end-of-file
    /// @throws std::runtime_error
    std::size_t Read(std::span<char> buffer);

    /// @brief Sets the file read/write offset from the beginning of the file
    /// @throws std::runtime_error
    void Seek(std::size_t offset_in_bytes);

    /// @brief Makes sure the written data is actually stored on disk
    /// @throws std::runtime_error
    void FSync();

    /// @brief Fetches the file size
    /// @throws std::runtime_error
    std::size_t GetSize() const;

private:
    explicit FileDescriptor(int fd);

    friend class TempFile;

    int fd_;
};

}  // namespace fs::blocking

USERVER_NAMESPACE_END
