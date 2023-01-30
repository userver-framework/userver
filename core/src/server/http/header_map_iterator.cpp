#include <userver/server/http/header_map_iterator.hpp>

#include <server/http/header_map_impl/map.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

HeaderMapIterator::HeaderMapIterator() = default;
HeaderMapIterator::HeaderMapIterator(UnderlyingIterator it)
    : it_{it} {}
HeaderMapIterator::~HeaderMapIterator() = default;

HeaderMapIterator::HeaderMapIterator(const HeaderMapIterator& other) = default;
HeaderMapIterator::HeaderMapIterator(HeaderMapIterator&& other) noexcept
    : it_{other.it_} {}

HeaderMapIterator& HeaderMapIterator::operator=(
    const HeaderMapIterator& other) {
  if (this != &other) {
    it_ = other.it_;
    current_value_.reset();
  }

  return *this;
}
HeaderMapIterator& HeaderMapIterator::operator=(
    HeaderMapIterator&& other) noexcept {
  return *this = other;
}

HeaderMapIterator& HeaderMapIterator::operator++() {
  ++it_;
  current_value_.reset();

  return *this;
}
HeaderMapIterator HeaderMapIterator::operator++(int) {
  HeaderMapIterator copy{*this};
  ++*this;
  return copy;
}

HeaderMapIterator::reference HeaderMapIterator::operator*() {
  UpdateCurrentValue();
  return *current_value_;
}

HeaderMapIterator::const_reference HeaderMapIterator::operator*() const {
  UpdateCurrentValue();
  return *current_value_;
}

HeaderMapIterator::pointer HeaderMapIterator::operator->() {
  UpdateCurrentValue();
  return &*current_value_;
}

HeaderMapIterator::const_pointer HeaderMapIterator::operator->() const {
  UpdateCurrentValue();
  return &*current_value_;
}

bool HeaderMapIterator::operator==(const HeaderMapIterator& other) const {
  return it_ == other.it_;
}

bool HeaderMapIterator::operator!=(const HeaderMapIterator& other) const {
  return it_ != other.it_;
}

void HeaderMapIterator::UpdateCurrentValue() const {
  if (!current_value_.has_value()) {
    auto& value = *it_;
    current_value_.emplace(EntryProxy{value.header_name, value.header_value});
  }
}

// ------------------------------- ConstIterator -------------------------------

HeaderMapConstIterator::HeaderMapConstIterator() = default;
HeaderMapConstIterator::HeaderMapConstIterator(UnderlyingIterator it)
    : it_{it} {}
HeaderMapConstIterator::~HeaderMapConstIterator() = default;

HeaderMapConstIterator::HeaderMapConstIterator(
    const HeaderMapConstIterator& other) = default;
HeaderMapConstIterator::HeaderMapConstIterator(
    HeaderMapConstIterator&& other) noexcept
    : it_{other.it_} {}

HeaderMapConstIterator& HeaderMapConstIterator::operator=(
    const HeaderMapConstIterator& other) {
  if (this != &other) {
    it_ = other.it_;
    current_value_.reset();
  }

  return *this;
}
HeaderMapConstIterator& HeaderMapConstIterator::operator=(
    HeaderMapConstIterator&& other) noexcept {
  return *this = other;
}

HeaderMapConstIterator& HeaderMapConstIterator::operator++() {
  ++it_;
  current_value_.reset();

  return *this;
}
HeaderMapConstIterator HeaderMapConstIterator::operator++(int) {
  HeaderMapConstIterator copy{*this};
  ++*this;
  return copy;
}

HeaderMapConstIterator::reference HeaderMapConstIterator::operator*() {
  UpdateCurrentValue();
  return *current_value_;
}

HeaderMapConstIterator::const_reference HeaderMapConstIterator::operator*()
    const {
  UpdateCurrentValue();
  return *current_value_;
}

HeaderMapConstIterator::pointer HeaderMapConstIterator::operator->() {
  UpdateCurrentValue();
  return &*current_value_;
}

HeaderMapConstIterator::const_pointer HeaderMapConstIterator::operator->()
    const {
  UpdateCurrentValue();
  return &*current_value_;
}

bool HeaderMapConstIterator::operator==(
    const HeaderMapConstIterator& other) const {
  return it_ == other.it_;
}

bool HeaderMapConstIterator::operator!=(
    const HeaderMapConstIterator& other) const {
  return it_ != other.it_;
}

void HeaderMapConstIterator::UpdateCurrentValue() const {
  if (!current_value_.has_value()) {
    const auto& value = *it_;
    current_value_.emplace(
        ConstEntryProxy{value.header_name, value.header_value});
  }
}

}  // namespace server::http

USERVER_NAMESPACE_END
