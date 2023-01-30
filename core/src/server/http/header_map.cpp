#include <userver/server/http/header_map.hpp>

#include <server/http/header_map_impl/header_name.hpp>
#include <server/http/header_map_impl/map.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

HeaderMap::HeaderMap() = default;

HeaderMap::~HeaderMap() = default;

HeaderMap::HeaderMap(const HeaderMap& other) = default;

HeaderMap::HeaderMap(HeaderMap&& other) noexcept = default;

std::size_t HeaderMap::size() const noexcept { return impl_->Size(); }

bool HeaderMap::empty() const noexcept { return impl_->Empty(); }

void HeaderMap::clear() { impl_->Clear(); }

HeaderMapIterator HeaderMap::find(const std::string& key) {
  const auto do_find = [this, &key] {
    if (header_map_impl::IsLowerCase(key)) {
      return impl_->Find(key);
    } else {
      const auto lowercase = header_map_impl::ToLowerCase(key);
      return impl_->Find(lowercase);
    }
  };

  return HeaderMapIterator{do_find()};
}

HeaderMapConstIterator HeaderMap::find(const std::string& key) const {
  const auto do_find = [this, &key] {
    if (header_map_impl::IsLowerCase(key)) {
      return impl_->Find(key);
    } else {
      const auto lowercase = header_map_impl::ToLowerCase(key);
      return impl_->Find(lowercase);
    }
  };

  return HeaderMapConstIterator{do_find()};
}

HeaderMapIterator HeaderMap::find(SpecialHeader key) noexcept {
  return HeaderMapIterator{impl_->Find(key)};
}

HeaderMapConstIterator HeaderMap::find(SpecialHeader key) const noexcept {
  return HeaderMapConstIterator{impl_->Find(key)};
}

void HeaderMap::Insert(std::string key, std::string value) {
  if (header_map_impl::IsLowerCase(key)) {
    impl_->Insert(std::move(key), std::move(value), false);
  } else {
    impl_->Insert(header_map_impl::ToLowerCase(key), std::move(value), false);
  }
}

void HeaderMap::Insert(SpecialHeader key, std::string value) {
  impl_->Insert(key, std::move(value), false);
}

void HeaderMap::InsertOrAppend(std::string key, std::string value) {
  if (header_map_impl::IsLowerCase(key)) {
    impl_->Insert(std::move(key), std::move(value), true);
  } else {
    impl_->Insert(header_map_impl::ToLowerCase(key), std::move(value), true);
  }
}

void HeaderMap::InsertOrAppend(SpecialHeader key, std::string value) {
  impl_->Insert(key, std::move(value), true);
}

void HeaderMap::Erase(const std::string& key) {
  if (header_map_impl::IsLowerCase(key)) {
    impl_->Erase(key);
  } else {
    const auto lowercase = header_map_impl::ToLowerCase(key);
    impl_->Erase(lowercase);
  }
}

void HeaderMap::Erase(SpecialHeader key) { impl_->Erase(key); }

HeaderMapIterator HeaderMap::begin() noexcept {
  return HeaderMapIterator{impl_->Begin()};
}

HeaderMapConstIterator HeaderMap::begin() const noexcept { return cbegin(); }

HeaderMapConstIterator HeaderMap::cbegin() const noexcept {
  return HeaderMapConstIterator{impl_->Begin()};
}

HeaderMapIterator HeaderMap::end() noexcept {
  return HeaderMapIterator{impl_->End()};
}

HeaderMapConstIterator HeaderMap::end() const noexcept { return cend(); }

HeaderMapConstIterator HeaderMap::cend() const noexcept {
  return HeaderMapConstIterator{impl_->End()};
}

}  // namespace server::http

USERVER_NAMESPACE_END
