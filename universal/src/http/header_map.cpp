#include <userver/http/header_map.hpp>

#include <http/header_map/map.hpp>

#include <userver/formats/json/value.hpp>
#include <userver/formats/parse/common_containers.hpp>

USERVER_NAMESPACE_BEGIN

namespace http::headers {

namespace {

std::string& DoAt(HeaderMap::Iterator it, HeaderMap::Iterator end,
                  std::string_view key) {
  if (it == end) {
    // we can afford to construct a string here instead of
    // just throwing not-so-meaningful "at"
    throw std::out_of_range{std::string{key}};
  }

  return it->second;
}

const std::string& DoAt(HeaderMap::ConstIterator it,
                        HeaderMap::ConstIterator end, std::string_view key) {
  if (it == end) {
    // we can afford to construct a string here instead of
    // just throwing not-so-meaningful "at"
    throw std::out_of_range{std::string{key}};
  }

  return it->second;
}

}  // namespace

HeaderMap::HeaderMap() = default;

HeaderMap::HeaderMap(
    std::initializer_list<std::pair<std::string, std::string>> headers) {
  reserve(headers.size());

  for (const auto& [name, value] : headers) {
    emplace(name, value);
  }
}

HeaderMap::HeaderMap(
    std::initializer_list<std::pair<PredefinedHeader, std::string>> headers) {
  reserve(headers.size());

  for (const auto& [name, value] : headers) {
    emplace(name, value);
  }
}

HeaderMap::HeaderMap(std::size_t capacity) : HeaderMap{} { reserve(capacity); }

HeaderMap::~HeaderMap() = default;

HeaderMap::HeaderMap(const HeaderMap& other) = default;

// NOLINTNEXTLINE(hicpp-use-equals-default,modernize-use-equals-default)
HeaderMap::HeaderMap(HeaderMap&& other) noexcept
    : impl_{std::move(other.impl_)} {}

HeaderMap& HeaderMap::operator=(const HeaderMap& other) = default;

// NOLINTNEXTLINE(hicpp-use-equals-default,modernize-use-equals-default)
HeaderMap& HeaderMap::operator=(HeaderMap&& other) noexcept {
  impl_ = std::move(other.impl_);

  return *this;
}

void HeaderMap::reserve(std::size_t capacity) { impl_->Reserve(capacity); }

std::size_t HeaderMap::size() const noexcept { return impl_->Size(); }

bool HeaderMap::empty() const noexcept { return impl_->Empty(); }

void HeaderMap::clear() { impl_->Clear(); }

std::size_t HeaderMap::count(std::string_view key) const noexcept {
  return contains(key) ? 1 : 0;
}

std::size_t HeaderMap::count(const PredefinedHeader& key) const noexcept {
  return contains(key) ? 1 : 0;
}

bool HeaderMap::contains(std::string_view key) const noexcept {
  return find(key) != end();
}

bool HeaderMap::contains(const PredefinedHeader& key) const noexcept {
  return find(key) != end();
}

std::string& HeaderMap::operator[](std::string&& key) {
  return impl_
      ->InsertOrModify(header_map::MaybeOwnedKey{key}, "",
                       header_map::Map::InsertOrModifyOccupiedAction::kNoop)
      ->Get()
      .second;
}

std::string& HeaderMap::operator[](std::string_view key) {
  return impl_
      ->InsertOrModify(header_map::MaybeOwnedKey{key}, "",
                       header_map::Map::InsertOrModifyOccupiedAction::kNoop)
      ->Get()
      .second;
}

std::string& HeaderMap::operator[](const PredefinedHeader& key) {
  return impl_
      ->InsertOrModify(key, "",
                       header_map::Map::InsertOrModifyOccupiedAction::kNoop)
      ->Get()
      .second;
}

HeaderMap::Iterator HeaderMap::find(std::string_view key) noexcept {
  return HeaderMap::Iterator{impl_->Find(key)};
}

HeaderMap::ConstIterator HeaderMap::find(std::string_view key) const noexcept {
  return HeaderMap::ConstIterator{impl_->Find(key)};
}

HeaderMap::Iterator HeaderMap::find(const PredefinedHeader& key) noexcept {
  return HeaderMap::Iterator{impl_->Find(key)};
}

HeaderMap::ConstIterator HeaderMap::find(const PredefinedHeader& key) const
    noexcept {
  return HeaderMap::ConstIterator{impl_->Find(key)};
}

void HeaderMap::insert(const std::pair<std::string, std::string>& kvp) {
  emplace(std::string_view{kvp.first}, kvp.second);
}

void HeaderMap::insert(std::pair<std::string, std::string>&& kvp) {
  emplace(std::move(kvp.first), std::move(kvp.second));
}

void HeaderMap::insert_or_assign(std::string key, std::string value) {
  impl_->InsertOrModify(
      header_map::MaybeOwnedKey{key}, std::move(value),
      header_map::Map::InsertOrModifyOccupiedAction::kReplace);
}

void HeaderMap::insert_or_assign(const PredefinedHeader& key,
                                 std::string value) {
  impl_->InsertOrModify(
      key, std::move(value),
      header_map::Map::InsertOrModifyOccupiedAction::kReplace);
}

void HeaderMap::InsertOrAppend(std::string key, std::string value) {
  impl_->InsertOrModify(header_map::MaybeOwnedKey{key}, std::move(value),
                        header_map::Map::InsertOrModifyOccupiedAction::kAppend);
}

void HeaderMap::InsertOrAppend(const PredefinedHeader& key, std::string value) {
  impl_->InsertOrModify(key, std::move(value),
                        header_map::Map::InsertOrModifyOccupiedAction::kAppend);
}

HeaderMap::Iterator HeaderMap::erase(HeaderMap::Iterator it) {
  return HeaderMap::Iterator{impl_->Erase(it->first)};
}

HeaderMap::Iterator HeaderMap::erase(HeaderMap::ConstIterator it) {
  return HeaderMap::Iterator{impl_->Erase(it->first)};
}

HeaderMap::Iterator HeaderMap::erase(std::string_view key) {
  return HeaderMap::Iterator{impl_->Erase(key)};
}

HeaderMap::Iterator HeaderMap::erase(const PredefinedHeader& key) {
  return HeaderMap::Iterator{impl_->Erase(key)};
}

std::string& HeaderMap::at(std::string_view key) {
  auto it = find(key);

  return DoAt(it, end(), key);
}

std::string& HeaderMap::at(const PredefinedHeader& key) {
  auto it = find(key);

  return DoAt(it, end(), key);
}

const std::string& HeaderMap::at(std::string_view key) const {
  const auto it = find(key);

  return DoAt(it, cend(), key);
}

const std::string& HeaderMap::at(const PredefinedHeader& key) const {
  const auto it = find(key);

  return DoAt(it, cend(), key);
}

HeaderMap::Iterator HeaderMap::begin() noexcept {
  return HeaderMap::Iterator{impl_->Begin()};
}

HeaderMap::ConstIterator HeaderMap::begin() const noexcept { return cbegin(); }

HeaderMap::ConstIterator HeaderMap::cbegin() const noexcept {
  return HeaderMap::ConstIterator{impl_->Begin()};
}

HeaderMap::Iterator HeaderMap::end() noexcept {
  return HeaderMap::Iterator{impl_->End()};
}

HeaderMap::ConstIterator HeaderMap::end() const noexcept { return cend(); }

HeaderMap::ConstIterator HeaderMap::cend() const noexcept {
  return HeaderMap::ConstIterator{impl_->End()};
}

bool HeaderMap::operator==(const HeaderMap& other) const noexcept {
  return *impl_ == *other.impl_;
}

void HeaderMap::OutputInHttpFormat(std::string& buffer) const {
  impl_->OutputInHttpFormat(buffer);
}

// ----------------------------------------------------------------------------

HeaderMap::Iterator::Iterator() = default;
HeaderMap::Iterator::Iterator(UnderlyingIterator it) : it_{it} {}
HeaderMap::Iterator::~Iterator() = default;

HeaderMap::Iterator::Iterator(const HeaderMap::Iterator& other) = default;
// NOLINTNEXTLINE(hicpp-use-equals-default,modernize-use-equals-default)
HeaderMap::Iterator::Iterator(HeaderMap::Iterator&& other) noexcept
    : it_{other.it_} {}

HeaderMap::Iterator& HeaderMap::Iterator::operator=(
    const HeaderMap::Iterator& other) = default;
// NOLINTNEXTLINE(hicpp-use-equals-default,modernize-use-equals-default)
HeaderMap::Iterator& HeaderMap::Iterator::operator=(
    HeaderMap::Iterator&& other) noexcept {
  it_ = other.it_;

  return *this;
}

HeaderMap::Iterator& HeaderMap::Iterator::operator++() {
  ++it_;

  return *this;
}
HeaderMap::Iterator HeaderMap::Iterator::operator++(int) {
  HeaderMap::Iterator copy{*this};
  ++*this;
  return copy;
}

HeaderMap::Iterator::reference HeaderMap::Iterator::operator*() {
  return it_->Get();
}

HeaderMap::Iterator::const_reference HeaderMap::Iterator::operator*() const {
  return it_->Get();
}

HeaderMap::Iterator::pointer HeaderMap::Iterator::operator->() {
  return &it_->Get();
}

HeaderMap::Iterator::const_pointer HeaderMap::Iterator::operator->() const {
  return &it_->Get();
}

bool HeaderMap::Iterator::operator==(const HeaderMap::Iterator& other) const {
  return it_ == other.it_;
}

bool HeaderMap::Iterator::operator!=(const Iterator& other) const {
  return it_ != other.it_;
}

bool HeaderMap::Iterator::operator==(const ConstIterator& other) const {
  return it_ == other.it_;
}

// ------------------------------- ConstIterator -------------------------------

HeaderMap::ConstIterator::ConstIterator() = default;
HeaderMap::ConstIterator::ConstIterator(UnderlyingIterator it) : it_{it} {}
HeaderMap::ConstIterator::~ConstIterator() = default;

HeaderMap::ConstIterator::ConstIterator(const ConstIterator& other) = default;
// NOLINTNEXTLINE(hicpp-use-equals-default,modernize-use-equals-default)
HeaderMap::ConstIterator::ConstIterator(ConstIterator&& other) noexcept
    : it_{other.it_} {}

HeaderMap::ConstIterator& HeaderMap::ConstIterator::operator=(
    const ConstIterator& other) = default;
// NOLINTNEXTLINE(hicpp-use-equals-default,modernize-use-equals-default)
HeaderMap::ConstIterator& HeaderMap::ConstIterator::operator=(
    ConstIterator&& other) noexcept {
  it_ = other.it_;

  return *this;
}

HeaderMap::ConstIterator& HeaderMap::ConstIterator::operator++() {
  ++it_;

  return *this;
}
HeaderMap::ConstIterator HeaderMap::ConstIterator::operator++(int) {
  ConstIterator copy{*this};
  ++*this;
  return copy;
}

HeaderMap::ConstIterator::reference HeaderMap::ConstIterator::operator*() {
  return it_->Get();
}

HeaderMap::ConstIterator::const_reference HeaderMap::ConstIterator::operator*()
    const {
  return it_->Get();
}

HeaderMap::ConstIterator::pointer HeaderMap::ConstIterator::operator->() {
  return &it_->Get();
}

HeaderMap::ConstIterator::const_pointer HeaderMap::ConstIterator::operator->()
    const {
  return &it_->Get();
}

bool HeaderMap::ConstIterator::operator==(const ConstIterator& other) const {
  return it_ == other.it_;
}

bool HeaderMap::ConstIterator::operator!=(const ConstIterator& other) const {
  return it_ != other.it_;
}

bool HeaderMap::ConstIterator::operator==(
    const HeaderMap::Iterator& other) const {
  return it_ == other.it_;
}

}  // namespace http::headers

USERVER_NAMESPACE_END
