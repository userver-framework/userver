#pragma once

/// @file userver/dump/operations_encrypted.hpp
/// @brief Encrypted dump Reader and Writer implementations

#include <filesystem>
#include <utility>

#include <userver/dump/factory.hpp>
#include <userver/dump/operations.hpp>
#include <userver/fs/blocking/impl/permissions.hpp>
#include <userver/utils/fast_pimpl.hpp>
#include <userver/utils/strong_typedef.hpp>

USERVER_NAMESPACE_BEGIN

namespace dump {

using SecretKey = utils::NonLoggable<class SecretKeyTag, std::string>;

class EncryptedWriter final : public Writer {
public:
    /// @brief Creates a new dump file and opens it
    /// @param options how to apply permissions to the temporary dump file
    /// @throws `Error` on a filesystem error
    EncryptedWriter(
        std::string filename,
        const SecretKey& secret_key,
        std::filesystem::perms perms,
        tracing::ScopeTime& scope,
        std::filesystem::perm_options options = std::filesystem::perm_options::replace
    );

    /// @overload
    /// Accepts `boost::filesystem::perms`.
    /// @deprecated use the std::filesystem overload
    template <fs::blocking::impl::LegacyPermissions Permissions>
    EncryptedWriter(std::string filename, const SecretKey& secret_key, Permissions perms, tracing::ScopeTime& scope)
        : dump::EncryptedWriter(
              std::move(filename),
              secret_key,
              fs::blocking::impl::ToStdPermissions(perms),
              scope,
              fs::blocking::impl::ToStdPermOptions(perms)
          )
    {}

    ~EncryptedWriter() override;

    void Finish() override;

private:
    void WriteRaw(std::string_view data) override;

    struct Impl;
    utils::FastPimpl<Impl, 632, 8> impl_;
};

class EncryptedReader final : public Reader {
public:
    /// @brief Opens an existing dump file
    /// @throws `Error` on a filesystem error
    EncryptedReader(std::string filename, const SecretKey& key);

    ~EncryptedReader() override;

    void Finish() override;

private:
    std::string_view ReadRaw(std::size_t max_size) override;

    void BackUp(std::size_t size) override;

    struct Impl;
    utils::FastPimpl<Impl, 600, 8> impl_;
};

class EncryptedOperationsFactory final : public OperationsFactory {
public:
    /// @brief Creates a factory that encrypts dumps with the specified key and writes them with the specified
    /// permissions.
    /// @param options how to apply permissions to each temporary dump file
    EncryptedOperationsFactory(
        SecretKey&& secret_key,
        std::filesystem::perms perms,
        std::filesystem::perm_options options = std::filesystem::perm_options::replace
    );

    /// @overload
    /// Accepts `boost::filesystem::perms`.
    /// @deprecated use the std::filesystem overload
    template <fs::blocking::impl::LegacyPermissions Permissions>
    EncryptedOperationsFactory(SecretKey&& secret_key, Permissions perms)
        : dump::EncryptedOperationsFactory(
              std::move(secret_key),
              fs::blocking::impl::ToStdPermissions(perms),
              fs::blocking::impl::ToStdPermOptions(perms)
          )
    {}

    std::unique_ptr<Reader> CreateReader(std::string full_path) override;

    std::unique_ptr<Writer> CreateWriter(std::string full_path, tracing::ScopeTime& scope) override;

private:
    const SecretKey secret_key_;
    const std::filesystem::perms perms_;
    const std::filesystem::perm_options options_;
};

}  // namespace dump

USERVER_NAMESPACE_END
