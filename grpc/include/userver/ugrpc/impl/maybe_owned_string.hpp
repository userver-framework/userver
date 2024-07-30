#pragma once

#include <string>
#include <string_view>
#include <utility>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

class MaybeOwnedString final {
 public:
  struct Ref {};

  MaybeOwnedString() = default;

  explicit MaybeOwnedString(std::string&& string)
      : storage_(std::move(string)), view_(storage_) {}

  MaybeOwnedString(Ref, std::string_view string) : view_(string) {}

  MaybeOwnedString(MaybeOwnedString&& other) noexcept {
    *this = std::move(other);
  }

  MaybeOwnedString& operator=(MaybeOwnedString&& other) noexcept {
    if (this == &other) return *this;
    if (other.view_.data() == other.storage_.data()) {
      storage_ = std::move(other.storage_);
      view_ = storage_;
    } else {
      storage_.clear();
      view_ = other.view_;
    }
    return *this;
  }

  std::string_view Get() const noexcept { return view_; }

 private:
  std::string storage_;
  std::string_view view_;
};

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
