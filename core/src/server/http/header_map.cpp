#include <userver/server/http/header_map.hpp>

#include <server/http/header_map_impl/header_name.hpp>
#include <server/http/header_map_impl/map.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

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

void HeaderMap::clear() {
  impl_->Clear();
}

HeaderMap::Iterator HeaderMap::find(const std::string& key) const {
  const auto lowercase = header_map_impl::ToLowerCase(key);
  return Iterator{Iterator::UnderlyingIteratorImpl{impl_->Find(lowercase)}};
}

utils::CheckedPtr<std::string> HeaderMap::FindPrepared(
    std::string_view key) const noexcept {
  UASSERT(header_map_impl::IsLowerCase(key));

  const auto it = impl_->Find(key);
  return utils::MakeCheckedPtr(it == impl_->End() ? nullptr
                                                  : &it->header_value);
}

void HeaderMap::Insert(std::string key, std::string value) {
  return InsertPrepared(header_map_impl::ToLowerCase(key), std::move(value));
}

void HeaderMap::InsertPrepared(std::string key, std::string value) {
  UASSERT(header_map_impl::IsLowerCase(key));

  return impl_->Insert(std::move(key), std::move(value));
}

void HeaderMap::Erase(const std::string& key) {
  const auto lowercase = header_map_impl::ToLowerCase(key);

  ErasePrepared(lowercase);
}

void HeaderMap::ErasePrepared(std::string_view key) {
  UASSERT(header_map_impl::IsLowerCase(key));

  impl_->Erase(key);
}

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
