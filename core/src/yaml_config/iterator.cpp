#include <yaml_config/iterator.hpp>

#include <utils/assert.hpp>
#include <yaml_config/yaml_config.hpp>

namespace yaml_config {
template <typename iter_traits>
Iterator<iter_traits>::Iterator(const Iterator<iter_traits>& other)
    : container_(other.container_), it_(other.it_) {
  current_.reset();
}

template <typename iter_traits>
Iterator<iter_traits>::Iterator(Iterator<iter_traits>&& other) noexcept
    : container_(std::exchange(other.container_, nullptr)),
      it_(std::move(other.it_)) {
  current_.reset();
}

template <typename iter_traits>
Iterator<iter_traits>& Iterator<iter_traits>::operator=(
    const Iterator<iter_traits>& other) {
  it_ = other.it_;
  container_ = other.container_;
  current_.reset();

  return *this;
}

template <typename iter_traits>
Iterator<iter_traits>& Iterator<iter_traits>::operator=(
    Iterator<iter_traits>&& other) noexcept {
  it_ = std::move(other.it_);
  container_ = std::exchange(other.container_, nullptr);
  current_.reset();

  return *this;
}

template <typename iter_traits>
void Iterator<iter_traits>::UpdateValue() const {
  UASSERT(container_ != nullptr);
  if (current_) return;

  if (it_.GetIteratorType() == formats::common::Type::kArray) {
    current_ = (*container_)[it_.GetIndex()];
  } else {
    UASSERT(it_.GetIteratorType() == formats::common::Type::kObject);
    current_ = (*container_)[it_.GetName()];
  }
}

// Explicit instantiation
template class Iterator<YamlConfig::IterTraits>;

}  // namespace yaml_config
