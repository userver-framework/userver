#include <yaml_config/iterator.hpp>

#include <utils/assert.hpp>
#include <yaml_config/parse.hpp>
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

  const auto& full_path = container_->FullPath();
  const auto& vars = container_->ConfigVarsPtr();

  if (it_.GetIteratorType() == formats::common::Type::kArray) {
    const size_t index = it_.GetIndex();
    auto result_opt = yaml_config::ParseOptional<value_type>(
        container_->Yaml(), index, full_path, vars);
    if (result_opt) {
      current_ = std::move(*result_opt);
    } else {
      // To get proper missing value
      current_ =
          value_type(formats::yaml::Value()[impl::PathAppend(full_path, index)],
                     impl::PathAppend(full_path, index), vars);
    }
  } else {
    UASSERT(it_.GetIteratorType() == formats::common::Type::kObject);
    const auto& key = it_.GetName();
    auto result_opt = yaml_config::ParseOptional<value_type>(
        container_->Yaml(), key, full_path, vars);
    if (result_opt) {
      current_ = std::move(*result_opt);
    } else {
      // To get proper missing value
      current_ =
          value_type(formats::yaml::Value()[impl::PathAppend(full_path, key)],
                     impl::PathAppend(full_path, key), vars);
    }
  }
}

// Explicit instantiation
template class Iterator<YamlConfig::IterTraits>;

}  // namespace yaml_config
