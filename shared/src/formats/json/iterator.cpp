#include <userver/formats/json/iterator.hpp>

#include <rapidjson/document.h>

#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>

#include <formats/json/impl/exttypes.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json {
namespace {

using IteratorDirection = common::IteratorDirection;

const formats::json::Value& GetValue(const formats::json::Value& container) {
  return container;
}

formats::json::Value& GetValue(formats::json::Value& container) {
  return container;
}

const formats::json::Value& GetValue(
    const impl::MutableValueWrapper& container) {
  return *container;
}

formats::json::Value& GetValue(impl::MutableValueWrapper& container) {
  return *container;
}

}  // namespace

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
template <typename Traits, IteratorDirection Direction>
Iterator<Traits, Direction>::Iterator(ContainerType container, int pos)
    : Iterator(std::move(container), GetValue(container).GetExtendedType(),
               pos) {}

template <typename Traits, IteratorDirection Direction>
Iterator<Traits, Direction>::Iterator(ContainerType&& container, int type,
                                      int pos) noexcept
    : container_(std::move(container)), type_(type), pos_(pos) {}

template <typename Traits, IteratorDirection Direction>
Iterator<Traits, Direction>::Iterator(const Iterator<Traits, Direction>& other)
    : container_(other.container_), type_(other.type_), pos_(other.pos_) {}

template <typename Traits, IteratorDirection Direction>
Iterator<Traits, Direction>::Iterator(
    Iterator<Traits, Direction>&& other) noexcept
    : container_(std::move(other.container_)),
      type_(other.type_),
      pos_(other.pos_) {}

template <typename Traits, IteratorDirection Direction>
Iterator<Traits, Direction>& Iterator<Traits, Direction>::operator=(
    const Iterator<Traits, Direction>& other) {
  if (this == &other) return *this;

  container_ = other.container_;
  type_ = other.type_;
  pos_ = other.pos_;
  current_.reset();
  return *this;
}

template <typename Traits, IteratorDirection Direction>
Iterator<Traits, Direction>& Iterator<Traits, Direction>::operator=(
    Iterator<Traits, Direction>&& other) noexcept {
  container_ = std::move(other.container_);
  type_ = other.type_;
  pos_ = other.pos_;
  current_.reset();
  return *this;
}

template <typename Traits, IteratorDirection Direction>
Iterator<Traits, Direction>::~Iterator() = default;

template <typename Traits, IteratorDirection Direction>
Iterator<Traits, Direction> Iterator<Traits, Direction>::operator++(int) {
  current_.reset();
  const auto prev_pos = pos_;
  pos_ += static_cast<int>(Direction);
  return Iterator<Traits, Direction>(ContainerType{container_}, type_,
                                     prev_pos);
}

template <typename Traits, IteratorDirection Direction>
Iterator<Traits, Direction>& Iterator<Traits, Direction>::operator++() {
  current_.reset();
  pos_ += static_cast<int>(Direction);
  return *this;
}

template <typename Traits, IteratorDirection Direction>
typename Iterator<Traits, Direction>::reference
Iterator<Traits, Direction>::operator*() const {
  UpdateValue();
  return *current_;
}

template <typename Traits, IteratorDirection Direction>
typename Iterator<Traits, Direction>::pointer
Iterator<Traits, Direction>::operator->() const {
  return &**this;
}

template <typename Traits, IteratorDirection Direction>
bool Iterator<Traits, Direction>::operator==(
    const Iterator<Traits, Direction>& other) const {
  return pos_ == other.pos_;
}

template <typename Traits, IteratorDirection Direction>
bool Iterator<Traits, Direction>::operator!=(
    const Iterator<Traits, Direction>& other) const {
  return pos_ != other.pos_;
}

template <typename Traits, IteratorDirection Direction>
std::string Iterator<Traits, Direction>::GetNameImpl() const {
  if (type_ == impl::Type::objectValue) {
    const auto& key = GetValue(container_).value_ptr_->MemberBegin()[pos_].name;
    return std::string(key.GetString(),
                       key.GetString() + key.GetStringLength());
  }
  throw TypeMismatchException(type_, impl::Type::objectValue,
                              GetValue(container_).GetPath());
}

template <typename Traits, IteratorDirection Direction>
size_t Iterator<Traits, Direction>::GetIndex() const {
  if (type_ == impl::Type::arrayValue) {
    return pos_;
  }
  throw TypeMismatchException(type_, impl::Type::arrayValue,
                              GetValue(container_).GetPath());
}

template <>
void Iterator<Value::IterTraits, IteratorDirection::kForward>::UpdateValue()
    const {
  if (current_) return;

  if (type_ == impl::Type::arrayValue) {
    current_.emplace(Value::EmplaceEnabler{}, container_.root_,
                     container_.value_ptr_->Begin()[pos_],
                     container_.depth_ + 1);
  } else {
    current_.emplace(Value::EmplaceEnabler{}, container_.root_,
                     container_.value_ptr_->MemberBegin()[pos_].value,
                     container_.depth_ + 1);
  }
}

template <>
void Iterator<Value::IterTraits, IteratorDirection::kReverse>::UpdateValue()
    const {
  if (current_) return;

  if (type_ == impl::Type::arrayValue) {
    current_.emplace(Value::EmplaceEnabler{}, container_.root_,
                     container_.value_ptr_->Begin()[pos_],
                     container_.depth_ + 1);
  } else {
    throw TypeMismatchException(type_, impl::Type::arrayValue,
                                GetValue(container_).GetPath());
  }
}

template <>
void Iterator<ValueBuilder::IterTraits,
              IteratorDirection::kForward>::UpdateValue() const {
  if (current_) return;

  if (type_ == impl::Type::arrayValue) {
    current_.emplace(ValueBuilder::EmplaceEnabler{},
                     container_.WrapElement(pos_));
  } else {
    current_.emplace(
        ValueBuilder::EmplaceEnabler{},
        container_.WrapMember(
            GetName(), container_->value_ptr_->MemberBegin()[pos_].value));
  }
}

template <>
void Iterator<ValueBuilder::IterTraits,
              IteratorDirection::kReverse>::UpdateValue() const {
  if (current_) return;

  if (type_ == impl::Type::arrayValue) {
    current_.emplace(ValueBuilder::EmplaceEnabler{},
                     container_.WrapElement(pos_));
  } else {
    throw TypeMismatchException(type_, impl::Type::arrayValue,
                                GetValue(container_).GetPath());
  }
}

// Explicit instantiation
template class Iterator<Value::IterTraits, IteratorDirection::kForward>;
template class Iterator<Value::IterTraits, IteratorDirection::kReverse>;
template class Iterator<ValueBuilder::IterTraits, IteratorDirection::kForward>;
template class Iterator<ValueBuilder::IterTraits, IteratorDirection::kReverse>;

}  // namespace formats::json

USERVER_NAMESPACE_END
