#pragma once

#include <cstddef>

#include <userver/engine/io/socket.hpp>
#include <userver/engine/io/tls_wrapper.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl::io {

class ISocket {
 public:
  virtual ~ISocket();

  virtual void SendAll(const void* buff, size_t size,
                       engine::Deadline deadline) = 0;

  virtual size_t RecvSome(void* buff, size_t size) = 0;
};

class NonSecureSocket final : public ISocket {
 public:
  NonSecureSocket(engine::io::Socket&& socket);
  ~NonSecureSocket() override;

  void SendAll(const void* buff, size_t size,
               engine::Deadline deadline) override;

  size_t RecvSome(void* buff, size_t size) override;

 private:
  engine::io::Socket socket_;
};

class SecureSocket final : public ISocket {
 public:
  SecureSocket(engine::io::Socket&& socket, engine::Deadline deadline);
  ~SecureSocket() override;

  void SendAll(const void* buff, size_t size,
               engine::Deadline deadline) override;

  size_t RecvSome(void* buff, size_t size) override;

 private:
  engine::io::TlsWrapper socket_;
};

}  // namespace urabbitmq::impl::io

USERVER_NAMESPACE_END
