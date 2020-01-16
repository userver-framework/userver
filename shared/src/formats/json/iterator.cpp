#include <formats/json/iterator.hpp>

#include <formats/json/exception.hpp>
#include <formats/json/value.hpp>
#include <formats/json/value_builder.hpp>
#include "json_tree.hpp"

#include <rapidjson/document.h>

namespace formats {
namespace json {

template <typename iter_traits>
Iterator<iter_traits>::Iterator(const NativeValuePtr& root,
                                const impl::Value* container, int iter,
                                int depth)
    : root_(root),
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
      container_(const_cast<impl::Value*>(container)),
      iter_(iter),
      depth_(depth),
      valid_(false) {}

template <typename iter_traits>
Iterator<iter_traits>::Iterator(const Iterator<iter_traits>& other)
    : root_(other.root_),
      container_(other.container_),
      iter_(other.iter_),
      depth_(other.depth_),
      valid_(false) {}

template <typename iter_traits>
Iterator<iter_traits>::Iterator(Iterator<iter_traits>&& other) noexcept
    : root_(std::move(other.root_)),
      container_(other.container_),
      iter_(other.iter_),
      depth_(other.depth_),
      valid_(false) {}

template <typename iter_traits>
Iterator<iter_traits>& Iterator<iter_traits>::operator=(
    const Iterator<iter_traits>& other) {
  root_ = other.root_;
  container_ = other.container_;
  iter_ = other.iter_;
  depth_ = other.depth_;
  valid_ = false;
  return *this;
}

template <typename iter_traits>
Iterator<iter_traits>& Iterator<iter_traits>::operator=(
    Iterator<iter_traits>&& other) noexcept {
  root_ = std::move(other.root_);
  container_ = other.container_;
  iter_ = other.iter_;
  depth_ = other.depth_;
  valid_ = false;
  return *this;
}

template <typename iter_traits>
Iterator<iter_traits>::~Iterator() = default;

template <typename iter_traits>
Iterator<iter_traits> Iterator<iter_traits>::operator++(int) {
  valid_ = false;
  return Iterator<iter_traits>(root_, container_, iter_++, depth_);
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
  switch (container_->GetType()) {
    case rapidjson::kObjectType: {
      const auto& key = container_->MemberBegin()[iter_].name;
      return std::string(key.GetString(),
                         key.GetString() + key.GetStringLength());
    }
    default:
      throw TypeMismatchException(
          container_->GetType(), rapidjson::kObjectType,
          impl::MakePath(root_.get(), container_, depth_));
  }
}

template <typename iter_traits>
uint32_t Iterator<iter_traits>::GetIndex() const {
  switch (container_->GetType()) {
    case rapidjson::kArrayType:
      return iter_;
    default:
      throw TypeMismatchException(
          container_->GetType(), rapidjson::kArrayType,
          impl::MakePath(root_.get(), container_, depth_));
  }
}

template <typename iter_traits>
void Iterator<iter_traits>::UpdateValue() const {
  if (std::exchange(valid_, true)) {
    return;
  }

  switch (container_->GetType()) {
    case rapidjson::kArrayType:
      value_.SetNonRoot(root_, container_->Begin()[iter_], depth_ + 1);
      break;
    case rapidjson::kObjectType:
      value_.SetNonRoot(root_, container_->MemberBegin()[iter_].value,
                        depth_ + 1);
      break;
    default:;
  }
}

// Explicit instantiation
template class Iterator<Value::IterTraits>;
template class Iterator<ValueBuilder::IterTraits>;

}  // namespace json
}  // namespace formats
