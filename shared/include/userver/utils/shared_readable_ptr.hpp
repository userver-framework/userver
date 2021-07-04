#pragma once

/// @file userver/utils/shared_readable_ptr.hpp
/// @brief @copybrief utils::SharedReadablePtr

#include <memory>
#include <type_traits>

namespace utils {

/// @ingroup userver_containers
///
/// @brief std::shared_ptr<const T> wrapper that makes sure that the pointer
/// is stored before dereferencing. Protects from dangling references:
/// @code
///   // BAD! Result of `config_.Get()` may be destroyed after the invocation.
///   const auto& config = config_.Get()->Get<taxi_config::TaxiConfig>();
/// @endcode
template <typename T>
class SharedReadablePtr final {
  static_assert(!std::is_const_v<T>,
                "SharedReadablePtr already adds `const` to `T`");
  static_assert(!std::is_reference_v<T>,
                "SharedReadablePtr does not work with references");

 public:
  using Base = std::shared_ptr<const T>;

  SharedReadablePtr(const SharedReadablePtr& ptr) = default;
  SharedReadablePtr(SharedReadablePtr&& ptr) noexcept = default;

  SharedReadablePtr& operator=(const SharedReadablePtr& ptr) = default;
  SharedReadablePtr& operator=(SharedReadablePtr&& ptr) noexcept = default;

  SharedReadablePtr(const Base& ptr) noexcept : base_(ptr) {}

  SharedReadablePtr(Base&& ptr) noexcept : base_(std::move(ptr)) {}

  SharedReadablePtr& operator=(const Base& ptr) {
    base_ = ptr;
    return *this;
  }

  SharedReadablePtr& operator=(Base&& ptr) noexcept {
    base_ = std::move(ptr);
    return *this;
  }

  const T& operator*() const& noexcept { return *base_; }

  const T& operator*() && { ReportMisuse(); }

  const T* operator->() const& noexcept { return base_.get(); }

  const T* operator->() && { ReportMisuse(); }

  operator const Base&() const& noexcept { return base_; }

  operator const Base&() && { ReportMisuse(); }

  explicit operator bool() const noexcept { return !!base_; }

 private:
  [[noreturn]] static void ReportMisuse() {
    static_assert(!sizeof(T), "keep the pointer before using, please");
  }

  Base base_;
};

}  // namespace utils
