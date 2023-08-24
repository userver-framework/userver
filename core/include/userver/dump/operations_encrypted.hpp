#pragma once

#include <boost/filesystem/operations.hpp>

#include <userver/dump/factory.hpp>
#include <userver/dump/operations.hpp>
#include <userver/utils/fast_pimpl.hpp>
#include <userver/utils/strong_typedef.hpp>

USERVER_NAMESPACE_BEGIN

namespace dump {

using SecretKey = utils::NonLoggable<class SecretKeyTag, std::string>;

class EncryptedWriter final : public Writer {
 public:
  /// @brief Creates a new dump file and opens it
  /// @throws `Error` on a filesystem error
  EncryptedWriter(std::string filename, const SecretKey& secret_key,
                  boost::filesystem::perms, tracing::ScopeTime& scope);

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

  struct Impl;
  utils::FastPimpl<Impl, 600, 8> impl_;
};

class EncryptedOperationsFactory final : public OperationsFactory {
 public:
  EncryptedOperationsFactory(SecretKey&& secret_key,
                             boost::filesystem::perms perms);

  std::unique_ptr<Reader> CreateReader(std::string full_path) override;

  std::unique_ptr<Writer> CreateWriter(std::string full_path,
                                       tracing::ScopeTime& scope) override;

 private:
  const SecretKey secret_key_;
  const boost::filesystem::perms perms_;
};

}  // namespace dump

USERVER_NAMESPACE_END
