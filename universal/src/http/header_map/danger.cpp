#include <http/header_map/danger.hpp>

#include <userver/http/common_headers.hpp>

#include <userver/utils/assert.hpp>
#include <userver/utils/rand.hpp>
#include <userver/utils/str_icase.hpp>

USERVER_NAMESPACE_BEGIN

namespace http::headers::header_map {

std::size_t Danger::HashKey(std::string_view key) const noexcept {
  if (!IsRed()) {
    return UnsafeHash(key);
  }

  return SafeHash(key);
}

std::size_t Danger::HashKey(const PredefinedHeader& header) const noexcept {
  if (!IsRed()) {
    return header.hash;
  }

  return SafeHash(header.name);
}

bool Danger::IsYellow() const noexcept { return state_ == State::kYellow; }
bool Danger::IsRed() const noexcept { return state_ == State::kRed; }

void Danger::ToGreen() noexcept {
  UASSERT(IsYellow());

  state_ = State::kGreen;
}

void Danger::ToYellow() noexcept {
  if (state_ == State::kGreen) {
    state_ = State::kYellow;
  }
}

void Danger::ToRed() noexcept {
  UASSERT(IsYellow());

  state_ = State::kRed;

  const auto init_key = [](std::uint64_t& k) {
    do {
      k = std::uniform_int_distribution<std::uint64_t>{}(
          utils::DefaultRandom());
    } while (k == 0);
  };
  init_key(k0_);
  init_key(k1_);
}

std::size_t Danger::SafeHash(std::string_view key) const noexcept {
  UASSERT(k0_ != 0 && k1_ != 0);

  return utils::StrIcaseHash{utils::HashSeed{k0_, k1_}}(key);
}

std::size_t Danger::UnsafeHash(std::string_view key) noexcept {
  return http::headers::impl::UnsafeConstexprHasher{}(key);
}

}  // namespace http::headers::header_map

USERVER_NAMESPACE_END
