#pragma once

/// @file userver/engine/io/tls_wrapper.hpp
/// @brief TLS socket wrappers

#include <string>
#include <vector>

#include <userver/crypto/certificate.hpp>
#include <userver/crypto/private_key.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/io/common.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::io {

/// Class for TLS communications over a Socket.
///
/// Not thread safe.
///
/// Usage example:
/// @snippet src/engine/io/tls_wrapper_test.cpp TLS wrapper usage
class [[nodiscard]] TlsWrapper final : public RwBase {
 public:
  /// Starts a TLS client on an opened socket
  static TlsWrapper StartTlsClient(Socket&& socket,
                                   const std::string& server_name,
                                   Deadline deadline);

  /// Starts a TLS server on an opened socket
  static TlsWrapper StartTlsServer(
      Socket&& socket, const crypto::Certificate& cert,
      const crypto::PrivateKey& key, Deadline deadline,
      const std::vector<crypto::Certificate>& cert_authorities = {});

  ~TlsWrapper() override;

  TlsWrapper(const TlsWrapper&) = delete;
  TlsWrapper(TlsWrapper&&) noexcept;

  /// Whether the socket is valid.
  explicit operator bool() const { return IsValid(); }

  /// Whether the socket is valid.
  bool IsValid() const override;

  /// Suspends current task until the socket has data available.
  [[nodiscard]] bool WaitReadable(Deadline) override;

  /// Suspends current task until the socket can accept more data.
  [[nodiscard]] bool WaitWriteable(Deadline) override;

  /// @brief Receives at least one byte from the socket.
  /// @returns 0 if connection is closed on one side and no data could be
  /// received any more, received bytes count otherwise.
  [[nodiscard]] size_t RecvSome(void* buf, size_t len, Deadline deadline);

  /// @brief Receives exactly len bytes from the socket.
  /// @note Can return less than len if socket is closed by peer.
  [[nodiscard]] size_t RecvAll(void* buf, size_t len, Deadline deadline);

  /// @brief Sends exactly len bytes to the socket.
  /// @note Can return less than len if socket is closed by peer.
  [[nodiscard]] size_t SendAll(const void* buf, size_t len, Deadline deadline);

  /// @brief Finishes TLS session and returns the socket.
  /// @warning Wrapper becomes invalid on entry and can only be used to retry
  ///   socket extraction if interrupted.
  [[nodiscard]] Socket StopTls(Deadline deadline);

  /// @brief Receives at least one byte from the socket.
  /// @returns 0 if connection is closed on one side and no data could be
  /// received any more, received bytes count otherwise.
  [[nodiscard]] size_t ReadSome(void* buf, size_t len,
                                Deadline deadline) override {
    return RecvSome(buf, len, deadline);
  }

  /// @brief Receives exactly len bytes from the socket.
  /// @note Can return less than len if socket is closed by peer.
  [[nodiscard]] size_t ReadAll(void* buf, size_t len,
                               Deadline deadline) override {
    return RecvAll(buf, len, deadline);
  }

  /// @brief Writes exactly len bytes to the socket.
  /// @note Can return less than len if socket is closed by peer.
  [[nodiscard]] size_t WriteAll(const void* buf, size_t len,
                                Deadline deadline) override {
    return SendAll(buf, len, deadline);
  }

 private:
  explicit TlsWrapper(Socket&&);

  class Impl;
  constexpr static size_t kSize = 296;
  constexpr static size_t kAlignment = 8;
  utils::FastPimpl<Impl, kSize, kAlignment> impl_;
};

}  // namespace engine::io

USERVER_NAMESPACE_END
