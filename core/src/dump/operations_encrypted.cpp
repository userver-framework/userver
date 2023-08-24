#include <userver/dump/operations_encrypted.hpp>

#include <fmt/format.h>
#include <boost/filesystem/operations.hpp>

#include <cryptopp/files.h>
#include <cryptopp/gcm.h>
#include <cryptopp/modes.h>
#include <cryptopp/osrng.h>

#include <userver/fs/blocking/write.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/cpu_relax.hpp>

USERVER_NAMESPACE_BEGIN

namespace dump {

namespace {
constexpr std::size_t kCheckTimeAfterBytes{16 * 1024};
}

using Encryption = ::CryptoPP::GCM<::CryptoPP::AES>::Encryption;
using Decryption = ::CryptoPP::GCM<::CryptoPP::AES>::Decryption;

using IV = utils::NonLoggable<class IvTag, std::string>;

struct EncryptedWriter::Impl {
  std::string filename;
  Encryption encryption;
  std::unique_ptr<::CryptoPP::AuthenticatedEncryptionFilter> filter;
  utils::StreamingCpuRelax cpu_relax_;

  Impl(std::string&& filename, tracing::ScopeTime* scope)
      : filename(std::move(filename)),
        cpu_relax_(kCheckTimeAfterBytes, scope) {}

  std::string GetTempFilename() const { return filename + ".tmp"; }
};

constexpr std::size_t kIvSize = ::CryptoPP::AES::BLOCKSIZE;

constexpr std::size_t kMinPumpSize = 32768;

template <typename T>
inline const unsigned char* GetBytes(const T& data) {
  return reinterpret_cast<const unsigned char*>(data.GetUnderlying().data());
}

IV GenerateIv() {
  unsigned char iv[kIvSize];

  thread_local ::CryptoPP::AutoSeededRandomPool pool;
  pool.GenerateBlock(iv, kIvSize);

  return IV(iv, iv + kIvSize);
}

EncryptedWriter::EncryptedWriter(std::string filename,
                                 const SecretKey& secret_key,
                                 boost::filesystem::perms perms,
                                 tracing::ScopeTime& scope)
    : impl_(std::move(filename), &scope) {
  auto iv = GenerateIv();
  impl_->encryption.SetKeyWithIV(GetBytes(secret_key),
                                 secret_key.GetUnderlying().size(),
                                 GetBytes(iv), iv.size());

  // Create empty file with the passed perms and reopen it
  // because CryptoPP is not able to pass perms
  const auto& temp_filename = impl_->GetTempFilename();
  fs::blocking::RewriteFileContents(temp_filename, "");
  auto sink = std::make_unique<::CryptoPP::FileSink>(temp_filename.c_str());
  fs::blocking::Chmod(temp_filename, perms);

  // 1. IV
  sink->Put(reinterpret_cast<unsigned char*>(iv.GetUnderlying().data()),
            iv.size());

  impl_->filter = std::make_unique<::CryptoPP::AuthenticatedEncryptionFilter>(
      impl_->encryption, sink.release());
}

EncryptedWriter::~EncryptedWriter() = default;

void EncryptedWriter::WriteRaw(std::string_view data) {
  // 2. Data
  impl_->filter->Put(reinterpret_cast<const unsigned char*>(data.data()),
                     data.size());
  impl_->cpu_relax_.Relax(data.size());
}

void EncryptedWriter::Finish() {
  const bool hard_flush = true;
  impl_->filter->MessageEnd();
  impl_->filter->Flush(hard_flush);

  fs::blocking::Rename(impl_->GetTempFilename(), impl_->filename);
}

struct EncryptedReader::Impl {
  std::string filename;
  Decryption decryption;
  std::unique_ptr<::CryptoPP::FileSource> file;

  size_t next_skip{0};  // how many bytes in `raw` should be skipped
  std::string raw;
};

EncryptedReader::EncryptedReader(std::string filename,
                                 const SecretKey& secret_key) {
  impl_->filename = filename;

  bool pump_all = false;

  IV iv;
  impl_->file = std::make_unique<::CryptoPP::FileSource>(
      impl_->filename.c_str(), pump_all,
      new ::CryptoPP::StringSink(iv.GetUnderlying()));

  auto& file = *impl_->file;

  file.Pump(kIvSize);
  if (iv.size() != kIvSize) {
    throw Error(fmt::format(
        "Unexpected end-of-file while trying to read IV from encrypted dump "
        "file \"{}\": requested-size={}",
        impl_->filename, kIvSize));
  }
  file.Detach();

  impl_->decryption.SetKeyWithIV(GetBytes(secret_key), secret_key.size(),
                                 GetBytes(iv), kIvSize);

  file.Attach(new ::CryptoPP::AuthenticatedDecryptionFilter(
      impl_->decryption, new ::CryptoPP::StringSink(impl_->raw)));
}

EncryptedReader::~EncryptedReader() = default;

std::string_view EncryptedReader::ReadRaw(std::size_t max_size) {
  auto& raw = impl_->raw;

  UASSERT(raw.size() >= impl_->next_skip);

  if (raw.size() - impl_->next_skip >= max_size) {
    // There are enough bytes in `raw`, just return it
    auto skip = impl_->next_skip;
    impl_->next_skip += max_size;
    return {impl_->raw.data() + skip, max_size};
  }

  // Not enough bytes in `raw`, move it and read the rest (or until EOF)

  if (impl_->next_skip) {
    // Remove previously read data chunk
    raw.erase(0, impl_->next_skip);
  }

  // If raw doesn't contain enough bytes, read it
  while (impl_->raw.size() < max_size) {
    auto pump_size = std::max(max_size, kMinPumpSize);
    impl_->file->Pump(pump_size);
    if (impl_->file->GetStream()->eof()) {
      impl_->file->PumpAll();
      break;
    }
  }

  const auto result_size = std::min(impl_->raw.size(), max_size);
  impl_->next_skip = result_size;
  return {impl_->raw.data(), result_size};
}

void EncryptedReader::Finish() {
  if (impl_->file->GetStream()->eof()) return;

  impl_->raw.clear();
  impl_->file->Pump(1);
  if (impl_->raw.empty() && impl_->file->GetStream()->eof()) return;

  throw Error(fmt::format(
      "Unexpected extra data at the end of encrypted dump file \"{}\"",
      impl_->filename));
}

EncryptedOperationsFactory::EncryptedOperationsFactory(
    SecretKey&& secret_key, boost::filesystem::perms perms)
    : secret_key_(std::move(secret_key)), perms_(perms) {}

std::unique_ptr<Reader> EncryptedOperationsFactory::CreateReader(
    std::string full_path) {
  return std::make_unique<EncryptedReader>(std::move(full_path), secret_key_);
}

std::unique_ptr<Writer> EncryptedOperationsFactory::CreateWriter(
    std::string full_path, tracing::ScopeTime& scope) {
  return std::make_unique<EncryptedWriter>(std::move(full_path), secret_key_,
                                           perms_, scope);
}

}  // namespace dump

USERVER_NAMESPACE_END
