#include <userver/server/http/header_map.hpp>

#include <server/http/header_map_impl/header_name.hpp>
#include <server/http/header_map_impl/map.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

namespace {

std::string LowerCase(std::string&& key) {
  if (header_map_impl::IsLowerCase(key)) {
    return std::move(key);
  }

  return header_map_impl::ToLowerCase(key);
}

}  // namespace

struct HeaderMap::Iterator::UnderlyingIterator final {
  UnderlyingIterator() = default;
  explicit UnderlyingIterator(header_map_impl::Map::Iterator it)
      : underlying_iterator{it} {}

  header_map_impl::Map::Iterator underlying_iterator{};
};

HeaderMap::HeaderMap() = default;

HeaderMap::~HeaderMap() = default;

HeaderMap::HeaderMap(const HeaderMap& other) = default;

HeaderMap::HeaderMap(HeaderMap&& other) noexcept = default;

std::size_t HeaderMap::size() const noexcept { return impl_->Size(); }

bool HeaderMap::empty() const noexcept { return impl_->Empty(); }

void HeaderMap::clear() { impl_->Clear(); }

HeaderMap::Iterator HeaderMap::find(const std::string& key) const {
  const auto do_find = [this, &key] {
    if (header_map_impl::IsLowerCase(key)) {
      return impl_->Find(key);
    } else {
      const auto lowercase = header_map_impl::ToLowerCase(key);
      return impl_->Find(lowercase);
    }
  };

  return Iterator{Iterator::UnderlyingIteratorImpl{do_find()}};
}

HeaderMap::Iterator HeaderMap::find(SpecialHeader key) const noexcept {
  return Iterator{Iterator::UnderlyingIteratorImpl{impl_->Find(key.name)}};
}

void HeaderMap::Insert(std::string key, std::string value) {
  impl_->Insert(LowerCase(std::move(key)), std::move(value), false);
}

void HeaderMap::Insert(SpecialHeader key, std::string value) {
  impl_->Insert(std::string{key.name}, std::move(value), false);
}

void HeaderMap::InsertOrAppend(std::string key, std::string value) {
  impl_->Insert(LowerCase(std::move(key)), std::move(value), true);
}

void HeaderMap::InsertOrAppend(SpecialHeader key, std::string value) {
  impl_->Insert(std::string{key.name}, std::move(value), true);
}

void HeaderMap::Erase(const std::string& key) {
  if (header_map_impl::IsLowerCase(key)) {
    impl_->Erase(key);
  } else {
    const auto lowercase = header_map_impl::ToLowerCase(key);
    impl_->Erase(lowercase);
  }
}

void HeaderMap::Erase(SpecialHeader key) { impl_->Erase(key.name); }

HeaderMap::Iterator::Iterator() = default;
HeaderMap::Iterator::Iterator(UnderlyingIteratorImpl it)
    : it_{it->underlying_iterator} {}
HeaderMap::Iterator::~Iterator() = default;

HeaderMap::Iterator::Iterator(const Iterator& other) = default;
HeaderMap::Iterator::Iterator(Iterator&& other) noexcept
    : it_{other.it_->underlying_iterator} {}
HeaderMap::Iterator& HeaderMap::Iterator::operator=(const Iterator& other) {
  if (this != &other) {
    it_ = other.it_;
    current_value_.reset();
  }

  return *this;
}
HeaderMap::Iterator& HeaderMap::Iterator::operator=(Iterator&& other) noexcept {
  return *this = other;
}

HeaderMap::Iterator& HeaderMap::Iterator::operator++() {
  ++it_->underlying_iterator;
  current_value_.reset();

  return *this;
}
HeaderMap::Iterator HeaderMap::Iterator::operator++(int) {
  HeaderMap::Iterator copy{*this};
  ++*this;
  return copy;
}

HeaderMap::Iterator::reference HeaderMap::Iterator::operator*() {
  UpdateCurrentValue();
  return *current_value_;
}

HeaderMap::Iterator::const_reference HeaderMap::Iterator::operator*() const {
  UpdateCurrentValue();
  return *current_value_;
}

HeaderMap::Iterator::pointer HeaderMap::Iterator::operator->() {
  UpdateCurrentValue();
  return &*current_value_;
}

HeaderMap::Iterator::const_pointer HeaderMap::Iterator::operator->() const {
  UpdateCurrentValue();
  return &*current_value_;
}

bool HeaderMap::Iterator::operator==(const Iterator& other) const {
  return it_->underlying_iterator == other.it_->underlying_iterator;
}

bool HeaderMap::Iterator::operator!=(const Iterator& other) const {
  return it_->underlying_iterator != other.it_->underlying_iterator;
}

void HeaderMap::Iterator::UpdateCurrentValue() const {
  if (!current_value_.has_value()) {
    auto& value = *it_->underlying_iterator;
    current_value_.emplace(EntryProxy{value.header_name, value.header_value});
  }
}

HeaderMap::Iterator HeaderMap::begin() {
  return {Iterator::UnderlyingIteratorImpl{impl_->Begin()}};
}

HeaderMap::Iterator HeaderMap::begin() const {
  return {Iterator::UnderlyingIteratorImpl{impl_->Begin()}};
}

HeaderMap::Iterator HeaderMap::end() {
  return {Iterator::UnderlyingIteratorImpl{impl_->End()}};
}

HeaderMap::Iterator HeaderMap::end() const {
  return {Iterator::UnderlyingIteratorImpl{impl_->End()}};
}

}  // namespace server::http

USERVER_NAMESPACE_END
