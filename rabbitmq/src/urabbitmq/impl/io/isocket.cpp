#include "isocket.hpp"

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl::io {

NonSecureSocket::NonSecureSocket(engine::io::Socket&& socket)
    : socket_{std::move(socket)} {
  socket_.SetNotAwaitable();
}

NonSecureSocket::~NonSecureSocket() = default;

size_t NonSecureSocket::Write(const void* buff, size_t size) {
  return socket_.SendSome(buff, size, {});
}

size_t NonSecureSocket::Read(void* buff, size_t size) {
  return socket_.RecvSome(buff, size, {});
}

int NonSecureSocket::GetFd() const { return socket_.Fd(); }

SecureSocket::SecureSocket(engine::io::Socket&& socket)
    // TODO : deadline
    : fd_{socket.Fd()},
      socket_{
          engine::io::TlsWrapper::StartTlsClient(std::move(socket), "", {})} {
  socket_.SetNotAwaitable();
}

SecureSocket::~SecureSocket() = default;

size_t SecureSocket::Write(const void* buff, size_t size) {
  return socket_.SendSome(buff, size, {});
}

size_t SecureSocket::Read(void* buff, size_t size) {
  return socket_.RecvSome(buff, size, {});
}

int SecureSocket::GetFd() const { return fd_; }

}  // namespace urabbitmq::impl::io

USERVER_NAMESPACE_END
