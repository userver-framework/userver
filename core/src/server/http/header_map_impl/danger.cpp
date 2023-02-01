#include <server/http/header_map_impl/danger.hpp>

#include <userver/utils/assert.hpp>
#include <userver/utils/rand.hpp>
#include <userver/utils/str_icase.hpp>

#include <server/http/header_map_impl/header_name.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http::header_map_impl {

std::size_t Danger::HashKey(std::string_view key) const noexcept {
  if (!IsRed()) {
    return UnsafeHash(key);
  }

  return SafeHash(key);
}

std::size_t Danger::HashKey(http::SpecialHeader header) const noexcept {
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

  do {
    hash_seed_ =
        std::uniform_int_distribution<std::size_t>{}(utils::DefaultRandom());
  } while (hash_seed_ == 0);
}

std::size_t Danger::SafeHash(std::string_view key) const noexcept {
  UASSERT(hash_seed_ != 0);
  UASSERT(IsLowerCase(key));

  // TODO : better hashing
  return utils::StrCaseHash{hash_seed_}(key);
}

std::size_t Danger::UnsafeHash(std::string_view key) noexcept {
  UASSERT(IsLowerCase(key));

  // TODO : is MurMur good enough with power of 2 modulo?
  return RuntimeHasher{}(key.data(), key.size());
}

}  // namespace server::http::header_map_impl

USERVER_NAMESPACE_END
