#pragma once

/// @file userver/dump/operations_file.hpp
/// @brief File-based dump Reader and Writer implementations

#include <filesystem>
#include <utility>

#include <userver/fs/blocking/c_file.hpp>
#include <userver/fs/blocking/impl/permissions.hpp>
#include <userver/utils/cpu_relax.hpp>
#include <userver/utils/fast_pimpl.hpp>

#include <userver/dump/factory.hpp>
#include <userver/dump/operations.hpp>

USERVER_NAMESPACE_BEGIN

namespace dump {

/// A handle to a dump file. File operations block the thread.
class FileWriter final : public Writer {
public:
    /// @brief Creates a new dump file and opens it
    /// @param perms permissions to set, add or remove
    /// @param options how to apply permissions when finalizing the dump
    /// @note With `std::filesystem::perm_options::remove`, the temporary file starts
    /// with mode `0600` (subject to umask), then @ref dump::FileWriter::Finish removes the specified bits.
    /// @throws `Error` on a filesystem error
    explicit FileWriter(
        std::string path,
        std::filesystem::perms perms,
        tracing::ScopeTime& scope,
        std::filesystem::perm_options options = std::filesystem::perm_options::replace
    );

    /// @overload
    /// Accepts `boost::filesystem::perms`.
    /// @deprecated use the std::filesystem overload
    template <fs::blocking::impl::LegacyPermissions Permissions>
    explicit FileWriter(std::string path, Permissions perms, tracing::ScopeTime& scope)
        : dump::FileWriter(
              std::move(path),
              fs::blocking::impl::ToStdPermissions(perms),
              scope,
              fs::blocking::impl::ToStdPermOptions(perms)
          )
    {}

    void Finish() override;

private:
    void WriteRaw(std::string_view data) override;

    fs::blocking::CFile file_;
    std::string final_path_;
    std::string path_;
    std::filesystem::perms perms_;
    std::filesystem::perm_options options_;
    utils::StreamingCpuRelax cpu_relax_;
};

/// A handle to a dump file. File operations block the thread.
class FileReader final : public Reader {
public:
    /// @brief Opens an existing dump file
    /// @throws `Error` on a filesystem error
    explicit FileReader(std::string path);

    void Finish() override;

private:
    std::string_view ReadRaw(std::size_t max_size) override;

    void BackUp(std::size_t size) override;

    fs::blocking::CFile file_;
    std::string path_;
    std::string curr_chunk_;
};

class FileOperationsFactory final : public OperationsFactory {
public:
    /// @brief Creates a factory that writes dumps with the specified permissions.
    /// @param options how to apply permissions when finalizing each dump; see @ref dump::FileWriter
    explicit FileOperationsFactory(
        std::filesystem::perms perms,
        std::filesystem::perm_options options = std::filesystem::perm_options::replace
    );

    /// @overload
    /// Accepts `boost::filesystem::perms`.
    /// @deprecated use the std::filesystem overload
    template <fs::blocking::impl::LegacyPermissions Permissions>
    explicit FileOperationsFactory(Permissions perms)
        : dump::FileOperationsFactory(
              fs::blocking::impl::ToStdPermissions(perms),
              fs::blocking::impl::ToStdPermOptions(perms)
          )
    {}

    std::unique_ptr<Reader> CreateReader(std::string full_path) override;

    std::unique_ptr<Writer> CreateWriter(std::string full_path, tracing::ScopeTime& scope) override;

private:
    const std::filesystem::perms perms_;
    const std::filesystem::perm_options options_;
};

}  // namespace dump

USERVER_NAMESPACE_END
