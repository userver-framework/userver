#pragma once

#include <atomic>
#include <cstdint>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

template <typename T>
class TaggedPtr final {
  static_assert(sizeof(std::uintptr_t) <= sizeof(std::uint64_t));
  static constexpr std::uint64_t kTagShift = 48;

 public:
  using Tag = std::uint16_t;

  constexpr /*implicit*/ TaggedPtr(std::nullptr_t) noexcept : impl_(0) {}

  TaggedPtr(T* ptr, Tag tag)
      : impl_(reinterpret_cast<std::uintptr_t>(ptr) |
              (std::uint64_t{tag} << kTagShift)) {
    UASSERT(!(reinterpret_cast<std::uintptr_t>(ptr) & 0xffff'0000'0000'0000));
  }

  T* GetDataPtr() const noexcept {
    return reinterpret_cast<T*>(static_cast<std::uintptr_t>(
        impl_ & ((std::uint64_t{1} << kTagShift) - 1)));
  }

  Tag GetTag() const noexcept { return static_cast<Tag>(impl_ >> kTagShift); }

  Tag GetNextTag() const noexcept { return static_cast<Tag>(GetTag() + 1); }

 private:
  std::uint64_t impl_;
};

}  // namespace concurrent::impl

USERVER_NAMESPACE_END
