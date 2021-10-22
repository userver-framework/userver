#include <userver/formats/json/iterator.hpp>

#include <rapidjson/document.h>

#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>

#include <formats/json/impl/exttypes.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json {
namespace {

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
template <typename Traits>
Iterator<Traits>::Iterator(ContainerType container, int pos)
    : Iterator(std::move(container), GetValue(container).GetExtendedType(),
               pos) {}

template <typename Traits>
Iterator<Traits>::Iterator(ContainerType&& container, int type,
                           int pos) noexcept
    : container_(std::move(container)), type_(type), pos_(pos) {}

template <typename Traits>
Iterator<Traits>::Iterator(const Iterator<Traits>& other)
    : container_(other.container_), type_(other.type_), pos_(other.pos_) {}

template <typename Traits>
Iterator<Traits>::Iterator(Iterator<Traits>&& other) noexcept
    : container_(std::move(other.container_)),
      type_(other.type_),
      pos_(other.pos_) {}

template <typename Traits>
Iterator<Traits>& Iterator<Traits>::operator=(const Iterator<Traits>& other) {
  if (this == &other) return *this;

  container_ = other.container_;
  type_ = other.type_;
  pos_ = other.pos_;
  current_.reset();
  return *this;
}

template <typename Traits>
Iterator<Traits>& Iterator<Traits>::operator=(
    Iterator<Traits>&& other) noexcept {
  container_ = std::move(other.container_);
  type_ = other.type_;
  pos_ = other.pos_;
  current_.reset();
  return *this;
}

template <typename Traits>
Iterator<Traits>::~Iterator() = default;

template <typename Traits>
Iterator<Traits> Iterator<Traits>::operator++(int) {
  current_.reset();
  return Iterator<Traits>(ContainerType{container_}, type_, pos_++);
}

template <typename Traits>
Iterator<Traits>& Iterator<Traits>::operator++() {
  current_.reset();
  ++pos_;
  return *this;
}

template <typename Traits>
typename Iterator<Traits>::reference Iterator<Traits>::operator*() const {
  UpdateValue();
  return *current_;
}

template <typename Traits>
typename Iterator<Traits>::pointer Iterator<Traits>::operator->() const {
  return &**this;
}

template <typename Traits>
bool Iterator<Traits>::operator==(const Iterator<Traits>& other) const {
  return pos_ == other.pos_;
}

template <typename Traits>
bool Iterator<Traits>::operator!=(const Iterator<Traits>& other) const {
  return pos_ != other.pos_;
}

template <typename Traits>
std::string Iterator<Traits>::GetName() const {
  if (type_ == impl::Type::objectValue) {
    const auto& key = GetValue(container_).value_ptr_->MemberBegin()[pos_].name;
    return std::string(key.GetString(),
                       key.GetString() + key.GetStringLength());
  }
  throw TypeMismatchException(type_, impl::Type::objectValue,
                              GetValue(container_).GetPath());
}

template <typename Traits>
size_t Iterator<Traits>::GetIndex() const {
  if (type_ == impl::Type::arrayValue) {
    return pos_;
  }
  throw TypeMismatchException(type_, impl::Type::arrayValue,
                              GetValue(container_).GetPath());
}

template <>
void Iterator<Value::IterTraits>::UpdateValue() const {
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
void Iterator<ValueBuilder::IterTraits>::UpdateValue() const {
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

// Explicit instantiation
template class Iterator<Value::IterTraits>;
template class Iterator<ValueBuilder::IterTraits>;

}  // namespace formats::json

USERVER_NAMESPACE_END
