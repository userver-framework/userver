#include <formats/json/iterator.hpp>

#include <formats/json/exception.hpp>
#include <formats/json/value.hpp>
#include <formats/json/value_builder.hpp>

#include <json/value.h>

namespace formats {
namespace json {

template <typename iter_traits>
Iterator<iter_traits>::Iterator(const NativeValuePtr& root,
                                const typename iter_traits::native_iter& iter,
                                const formats::json::Path& path)
    : root_(root), iter_pimpl_(iter), path_(path), valid_(false) {}

template <typename iter_traits>
Iterator<iter_traits>::Iterator(const Iterator<iter_traits>& other)
    : root_(other.root_),
      iter_pimpl_(other.iter_pimpl_),
      path_(other.path_),
      valid_(false) {}

template <typename iter_traits>
Iterator<iter_traits>::Iterator(Iterator<iter_traits>&& other) noexcept
    : root_(std::move(other.root_)),
      iter_pimpl_(std::move(other.iter_pimpl_)),
      path_(std::move(other.path_)),
      valid_(false) {}

template <typename iter_traits>
Iterator<iter_traits>& Iterator<iter_traits>::operator=(
    const Iterator<iter_traits>& other) {
  root_ = other.root_;
  iter_pimpl_ = other.iter_pimpl_;
  path_ = other.path_;
  valid_ = false;
  return *this;
}

template <typename iter_traits>
Iterator<iter_traits>& Iterator<iter_traits>::operator=(
    Iterator<iter_traits>&& other) noexcept {
  root_ = std::move(other.root_);
  iter_pimpl_ = std::move(other.iter_pimpl_);
  path_ = std::move(other.path_);
  valid_ = false;
  return *this;
}

template <typename iter_traits>
Iterator<iter_traits>::~Iterator() = default;

template <typename iter_traits>
Iterator<iter_traits> Iterator<iter_traits>::operator++(int) {
  valid_ = false;
  return Iterator<iter_traits>(root_, (*iter_pimpl_)++, path_);
}

template <typename iter_traits>
Iterator<iter_traits>& Iterator<iter_traits>::operator++() {
  valid_ = false;
  ++(*iter_pimpl_);
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
  const auto& name = iter_pimpl_->name();
  if (name.empty()) {
    throw TypeMismatchException(Json::ValueType::arrayValue,
                                Json::ValueType::objectValue, path_.ToString());
  }
  return name;
}

template <typename iter_traits>
uint32_t Iterator<iter_traits>::GetIndex() const {
  const auto index = iter_pimpl_->index();
  if (static_cast<Json::Int>(index) == -1) {
    throw TypeMismatchException(Json::ValueType::objectValue,
                                Json::ValueType::arrayValue, path_.ToString());
  }
  return index;
}

template <typename iter_traits>
void Iterator<iter_traits>::UpdateValue() const {
  if (std::exchange(valid_, true)) {
    return;
  }

  const std::string& key = iter_pimpl_->name();
  if (!key.empty()) {
    value_.SetNonRoot(root_, **iter_pimpl_, path_, key);
  } else {
    value_.SetNonRoot(root_, **iter_pimpl_, path_, iter_pimpl_->index());
  }
}

// Explicit instantiation
template class Iterator<Value::IterTraits>;
template class Iterator<ValueBuilder::IterTraits>;

}  // namespace json
}  // namespace formats
