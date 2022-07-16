#pragma once

#include <cstddef>

#include <userver/engine/io/socket.hpp>
#include <userver/engine/io/tls_wrapper.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl::io {

class ISocket {
 public:
  virtual ~ISocket() = default;

  virtual size_t Write(const void* buff, size_t size) = 0;

  virtual size_t Read(void* buff, size_t size) = 0;

  virtual int GetFd() const = 0;
};

class NonSecureSocket final : public ISocket {
 public:
  NonSecureSocket(engine::io::Socket&& socket);
  ~NonSecureSocket() override;

  size_t Write(const void* buff, size_t size) override;

  size_t Read(void* buff, size_t size) override;

  int GetFd() const override;

 private:
  engine::io::Socket socket_;
};

class SecureSocket final : public ISocket {
 public:
  SecureSocket(engine::io::Socket&& socket, engine::Deadline deadline);
  ~SecureSocket() override;

  size_t Write(const void* buff, size_t size) override;

  size_t Read(void* buff, size_t size) override;

  int GetFd() const override;

 private:
  const int fd_;
  engine::io::TlsWrapper socket_;
};

}  // namespace urabbitmq::impl::io

USERVER_NAMESPACE_END
