#pragma once

#include <boost/filesystem/operations.hpp>

#include <cache/dump/factory.hpp>
#include <cache/dump/operations.hpp>
#include <utils/fast_pimpl.hpp>
#include <utils/strong_typedef.hpp>

namespace cache::dump {

using SecretKey = utils::NonLoggable<class SecretKeyTag, std::string>;

class EncryptedWriter final : public Writer {
 public:
  /// @brief Creates a new dump file and opens it
  /// @throws `Error` on a filesystem error
  EncryptedWriter(std::string_view filename, const SecretKey& secret_key,
                  boost::filesystem::perms);

  ~EncryptedWriter() override;

  void Finish() override;

 private:
  void WriteRaw(std::string_view data) override;

  struct Impl;
  utils::FastPimpl<Impl, 536, 8> impl_;
};

class EncryptedReader final : public Reader {
 public:
  /// @brief Opens an existing dump file
  /// @throws `Error` on a filesystem error
  EncryptedReader(std::string_view filename, const SecretKey& key);

  ~EncryptedReader() override;

  void Finish() override;

 private:
  std::string_view ReadRaw(std::size_t size) override;

  struct Impl;
  utils::FastPimpl<Impl, 576, 8> impl_;
};

class EncryptedOperationsFactory final : public OperationsFactory {
 public:
  EncryptedOperationsFactory(SecretKey&& secret_key,
                             boost::filesystem::perms perms);

  std::unique_ptr<Reader> CreateReader(std::string full_path) override;

  std::unique_ptr<Writer> CreateWriter(std::string full_path) override;

 private:
  const SecretKey secret_key_;
  const boost::filesystem::perms perms_;
};

}  // namespace cache::dump
