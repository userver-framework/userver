#include <formats/json/iterator.hpp>

#include <formats/json/exception.hpp>
#include <formats/json/value.hpp>
#include <formats/json/value_builder.hpp>

namespace formats {
namespace json {

template <typename iter_traits>
Iterator<iter_traits>::Iterator(const NativeValuePtr& root,
                                const typename iter_traits::native_iter& iter,
                                const formats::json::Path& path)
    : root_(root), iter_(iter), path_(path), valid_(false) {}

template <typename iter_traits>
Iterator<iter_traits>::Iterator(const Iterator<iter_traits>& other)
    : root_(other.root_),
      iter_(other.iter_),
      path_(other.path_),
      valid_(false) {}

template <typename iter_traits>
Iterator<iter_traits>::Iterator(Iterator<iter_traits>&& other) noexcept
    : root_(std::move(other.root_)),
      iter_(std::move(other.iter_)),
      path_(std::move(other.path_)),
      valid_(false) {}

template <typename iter_traits>
Iterator<iter_traits>& Iterator<iter_traits>::operator=(
    const Iterator<iter_traits>& other) {
  root_ = other.root_;
  iter_ = other.iter_;
  path_ = other.path_;
  valid_ = false;
  return *this;
}

template <typename iter_traits>
Iterator<iter_traits>& Iterator<iter_traits>::operator=(
    Iterator<iter_traits>&& other) noexcept {
  root_ = std::move(other.root_);
  iter_ = std::move(other.iter_);
  path_ = std::move(other.path_);
  valid_ = false;
  return *this;
}

template <typename iter_traits>
Iterator<iter_traits> Iterator<iter_traits>::operator++(int) {
  valid_ = false;
  return Iterator<iter_traits>(root_, iter_++, path_);
}

template <typename iter_traits>
Iterator<iter_traits>& Iterator<iter_traits>::operator++() {
  valid_ = false;
  ++iter_;
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
  return iter_ == other.iter_;
}

template <typename iter_traits>
bool Iterator<iter_traits>::operator!=(
    const Iterator<iter_traits>& other) const {
  return iter_ != other.iter_;
}

template <typename iter_traits>
std::string Iterator<iter_traits>::GetName() const {
  const auto& name = iter_.name();
  if (name.empty()) {
    ThrowTypeMismatch(Json::objectValue);
  }
  return name;
}

template <typename iter_traits>
uint32_t Iterator<iter_traits>::GetIndex() const {
  const auto index = iter_.index();
  if (static_cast<Json::Int>(index) == -1) {
    ThrowTypeMismatch(Json::arrayValue);
  }
  return index;
}

template <typename iter_traits>
void Iterator<iter_traits>::ThrowTypeMismatch(Json::ValueType expected) const {
  UpdateValue();
  throw TypeMismatchException(value_.Get(), expected, value_.GetPath());
}

template <typename iter_traits>
void Iterator<iter_traits>::UpdateValue() const {
  if (std::exchange(valid_, true)) {
    return;
  }

  const std::string& key = iter_.name();
  if (!key.empty()) {
    value_.Set(root_, *iter_, path_, key);
  } else {
    value_.Set(root_, *iter_, path_, iter_.index());
  }
}

// Explicit instantiation
template class Iterator<Value::IterTraits>;
template class Iterator<ValueBuilder::IterTraits>;

}  // namespace json
}  // namespace formats
