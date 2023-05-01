#pragma once
#include <userver/engine/io/exception.hpp>
#include <userver/utils/impl/span.hpp>

namespace userver {

inline void SendExactly(engine::io::WritableBase* writable,
                        utils::impl::Span<const std::byte> data,
                        engine::Deadline deadline) {
  if (writable->WriteAll(data.data(), data.size(), deadline) != data.size())
    throw(userver::engine::io::IoException()
          << "Socket closed during transfer ");
}

inline void RecvExactly(engine::io::ReadableBase* readable,
                        utils::impl::Span<std::byte> buffer,
                        engine::Deadline deadline) {
  if (readable->ReadAll(buffer.data(), buffer.size(), deadline) !=
      buffer.size())
    throw(engine::io::IoException() << "Socket closed during transfer ");
}

template <class T>
utils::impl::Span<std::byte> as_writable_bytes(
    utils::impl::Span<T> s) noexcept {
  return utils::impl::Span<std::byte>{reinterpret_cast<std::byte*>(s.begin()),
                                      reinterpret_cast<std::byte*>(s.end())};
}

template <class T>
utils::impl::Span<const std::byte> as_bytes(utils::impl::Span<T> s) noexcept {
  return utils::impl::Span<const std::byte>{
      reinterpret_cast<const std::byte*>(s.begin()),
      reinterpret_cast<const std::byte*>(s.end())};
}

template <class T>
utils::impl::Span<T> make_span(T* ptr, size_t count) {
  return utils::impl::Span<T>(ptr, ptr + count);
}


};  // namespace userver
