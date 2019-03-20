#include <formats/yaml/iterator.hpp>

#include <formats/yaml/exception.hpp>
#include <formats/yaml/value.hpp>
#include <formats/yaml/value_builder.hpp>

#include <yaml-cpp/yaml.h>

namespace formats {
namespace yaml {

template <typename iter_traits>
Iterator<iter_traits>::Iterator(const typename iter_traits::native_iter& iter,
                                int index, const formats::yaml::Path& path)
    : iter_pimpl_(iter), path_(path), index_(index), valid_(false) {}

template <typename iter_traits>
Iterator<iter_traits>::Iterator(const Iterator<iter_traits>& other)
    : iter_pimpl_(other.iter_pimpl_),
      path_(other.path_),
      index_(other.index_),
      valid_(false) {}

template <typename iter_traits>
Iterator<iter_traits>::Iterator(Iterator<iter_traits>&& other) noexcept
    : iter_pimpl_(std::move(other.iter_pimpl_)),
      path_(std::move(other.path_)),
      index_(other.index_),
      valid_(false) {}

template <typename iter_traits>
Iterator<iter_traits>& Iterator<iter_traits>::operator=(
    const Iterator<iter_traits>& other) {
  iter_pimpl_ = other.iter_pimpl_;
  path_ = other.path_;
  index_ = other.index_;
  valid_ = false;
  return *this;
}

template <typename iter_traits>
Iterator<iter_traits>& Iterator<iter_traits>::operator=(
    Iterator<iter_traits>&& other) noexcept {
  iter_pimpl_ = std::move(other.iter_pimpl_);
  path_ = std::move(other.path_);
  index_ = other.index_;
  valid_ = false;
  return *this;
}

template <typename iter_traits>
Iterator<iter_traits>::~Iterator() = default;

template <typename iter_traits>
Iterator<iter_traits> Iterator<iter_traits>::operator++(int) {
  valid_ = false;
  const auto index_copy = index_;
  if (index_ != -1) {
    ++index_;
  }
  return Iterator<iter_traits>((*iter_pimpl_)++, index_copy, path_);
}

template <typename iter_traits>
Iterator<iter_traits>& Iterator<iter_traits>::operator++() {
  valid_ = false;
  ++(*iter_pimpl_);
  if (index_ != -1) {
    ++index_;
  }
  return *this;
}

template <typename iter_traits>
typename Iterator<iter_traits>::reference Iterator<iter_traits>::operator*()
    const {
  UpdateValue();
  return value_;
}

template <typename iter_traits>
typename Iterator<iter_traits>::pointer Iterator<iter_traits>::operator->()
    const {
  UpdateValue();
  return &value_;
}

template <typename iter_traits>
bool Iterator<iter_traits>::operator==(
    const Iterator<iter_traits>& other) const {
  return *iter_pimpl_ == *other.iter_pimpl_;
}

template <typename iter_traits>
bool Iterator<iter_traits>::operator!=(
    const Iterator<iter_traits>& other) const {
  return *iter_pimpl_ != *other.iter_pimpl_;
}

template <typename iter_traits>
std::string Iterator<iter_traits>::GetName() const {
  if (index_ != -1) {
    throw TypeMismatchException(Type::kArray, Type::kObject,
                                PathToString(path_));
  }
  return (*iter_pimpl_)->first.Scalar();
}

template <typename iter_traits>
uint32_t Iterator<iter_traits>::GetIndex() const {
  if (index_ == -1) {
    throw TypeMismatchException(Type::kObject, Type::kArray,
                                PathToString(path_));
  }
  return index_;
}

template <typename iter_traits>
void Iterator<iter_traits>::UpdateValue() const {
  if (std::exchange(valid_, true)) {
    return;
  }

  if (index_ == -1) {
    value_.SetNonRoot((*iter_pimpl_)->second, path_, GetName());
  } else {
    value_.SetNonRoot(**iter_pimpl_, path_, index_);
  }
}

// Explicit instantiation
template class Iterator<Value::IterTraits>;
template class Iterator<ValueBuilder::IterTraits>;

}  // namespace yaml
}  // namespace formats
