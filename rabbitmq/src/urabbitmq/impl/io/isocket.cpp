#include "isocket.hpp"

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl::io {

ISocket::~ISocket() = default;

NonSecureSocket::NonSecureSocket(engine::io::Socket&& socket)
    : socket_{std::move(socket)} {}

NonSecureSocket::~NonSecureSocket() { socket_.Close(); }

void NonSecureSocket::SendAll(const void* buff, size_t size,
                              engine::Deadline deadline) {
  if (socket_.SendAll(buff, size, deadline) != size) {
    throw std::runtime_error{"Connection reset by peer"};
  }
}

size_t NonSecureSocket::RecvSome(void* buff, size_t size) {
  return socket_.RecvSome(buff, size, {});
}

SecureSocket::SecureSocket(engine::io::Socket&& socket,
                           engine::Deadline deadline)
    : socket_{engine::io::TlsWrapper::StartTlsClient(std::move(socket), "",
                                                     deadline)} {}

SecureSocket::~SecureSocket() = default;

void SecureSocket::SendAll(const void* buff, size_t size,
                           engine::Deadline deadline) {
  if (socket_.SendAll(buff, size, deadline) != size) {
    throw std::runtime_error{"Connection reset by peer"};
  }
}

size_t SecureSocket::RecvSome(void* buff, size_t size) {
  return socket_.RecvSome(buff, size, {});
}

}  // namespace urabbitmq::impl::io

USERVER_NAMESPACE_END
