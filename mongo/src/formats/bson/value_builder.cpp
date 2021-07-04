#include <userver/formats/bson/value_builder.hpp>

#include <limits>

#include <fmt/format.h>

#include <formats/bson/value_impl.hpp>
#include <formats/bson/wrappers.hpp>
#include <formats/common/validations.hpp>
#include <userver/formats/bson/exception.hpp>
#include <userver/utils/assert.hpp>

namespace formats::bson {
namespace {

int64_t ToInt64(uint64_t value) {
  if (value > static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
    throw BsonException(fmt::format("Value {} is too large for BSON", value));
  }
  return static_cast<int64_t>(value);
}

}  // namespace

ValueBuilder::ValueBuilder() : ValueBuilder(Type::kNull) {}

ValueBuilder::ValueBuilder(ValueBuilder::Type type) {
  switch (type) {
    case Type::kNull:
      impl_ = std::make_shared<impl::ValueImpl>(nullptr);
      break;

    case Type::kArray:
      impl_ = std::make_shared<impl::ValueImpl>(
          impl::MutableBson().Extract(), impl::ValueImpl::DocumentKind::kArray);
      break;

    case Type::kObject:
      impl_ = std::make_shared<impl::ValueImpl>(
          impl::MutableBson().Extract(),
          impl::ValueImpl::DocumentKind::kDocument);
      break;
  }
}

ValueBuilder::ValueBuilder(impl::ValueImplPtr impl) : impl_(std::move(impl)) {}

ValueBuilder::ValueBuilder(const ValueBuilder& other) { *this = other; }

// NOLINTNEXTLINE(performance-noexcept-move-constructor)
ValueBuilder::ValueBuilder(ValueBuilder&& other) { *this = std::move(other); }

ValueBuilder& ValueBuilder::operator=(const ValueBuilder& other) {
  if (this == &other) return *this;

  Assign(other.impl_);
  return *this;
}

// NOLINTNEXTLINE(performance-noexcept-move-constructor)
ValueBuilder& ValueBuilder::operator=(ValueBuilder&& other) {
  if (other.impl_.use_count() == 1) {
    Assign(std::move(other.impl_));
  } else {
    Assign(other.impl_);
  }
  return *this;
}

ValueBuilder::ValueBuilder(const formats::bson::Value& value) {
  value.CheckNotMissing();
  Assign(value.impl_);
}

ValueBuilder::ValueBuilder(const Document& document)
    : ValueBuilder(static_cast<const Value&>(document)) {}

ValueBuilder::ValueBuilder(formats::bson::Value&& value) {
  value.CheckNotMissing();
  if (value.impl_.use_count() == 1) {
    Assign(std::move(value.impl_));
  } else {
    Assign(value.impl_);
  }
}

ValueBuilder::ValueBuilder(Document&& document)
    : ValueBuilder(static_cast<Value&&>(document)) {}

ValueBuilder::ValueBuilder(std::nullptr_t) : ValueBuilder() {}

ValueBuilder::ValueBuilder(bool value)
    : impl_(std::make_shared<impl::ValueImpl>(value)) {}

ValueBuilder::ValueBuilder(int32_t value)
    : impl_(std::make_shared<impl::ValueImpl>(value)) {}

ValueBuilder::ValueBuilder(uint32_t value)
    : impl_(std::make_shared<impl::ValueImpl>(int64_t{value})) {}

ValueBuilder::ValueBuilder(int64_t value)
    : impl_(std::make_shared<impl::ValueImpl>(value)) {}

ValueBuilder::ValueBuilder(uint64_t value)
    : impl_(std::make_shared<impl::ValueImpl>(ToInt64(value))) {}

ValueBuilder::ValueBuilder(double value)
    : impl_(std::make_shared<impl::ValueImpl>(
          formats::common::ValidateFloat<BsonException>(value))) {}

ValueBuilder::ValueBuilder(const char* value)
    : ValueBuilder(std::string(value)) {}

ValueBuilder::ValueBuilder(std::string value)
    : impl_(std::make_shared<impl::ValueImpl>(std::move(value))) {}

ValueBuilder::ValueBuilder(std::string_view value)
    : ValueBuilder(std::string(value)) {}

ValueBuilder::ValueBuilder(const std::chrono::system_clock::time_point& value)
    : impl_(std::make_shared<impl::ValueImpl>(value)) {}

ValueBuilder::ValueBuilder(const Oid& value)
    : impl_(std::make_shared<impl::ValueImpl>(value)) {}

ValueBuilder::ValueBuilder(Binary value)
    : impl_(std::make_shared<impl::ValueImpl>(std::move(value))) {}

ValueBuilder::ValueBuilder(const Decimal128& value)
    : impl_(std::make_shared<impl::ValueImpl>(value)) {}

ValueBuilder::ValueBuilder(MinKey value)
    : impl_(std::make_shared<impl::ValueImpl>(value)) {}

ValueBuilder::ValueBuilder(MaxKey value)
    : impl_(std::make_shared<impl::ValueImpl>(value)) {}

ValueBuilder::ValueBuilder(const Timestamp& value)
    : impl_(std::make_shared<impl::ValueImpl>(value)) {}

#ifdef _LIBCPP_VERSION
ValueBuilder::ValueBuilder(long value)
#else
ValueBuilder::ValueBuilder(long long value)
#endif
    : ValueBuilder(int64_t{value}) {
}

#ifdef _LIBCPP_VERSION
ValueBuilder::ValueBuilder(unsigned long value)
#else
ValueBuilder::ValueBuilder(unsigned long long value)
#endif
    : ValueBuilder(uint64_t{value}) {
}

ValueBuilder ValueBuilder::operator[](const std::string& name) {
  return ValueBuilder(impl_->GetOrInsert(name));
}

ValueBuilder ValueBuilder::operator[](uint32_t index) {
  return ValueBuilder((*impl_)[index]);
}

void ValueBuilder::Remove(const std::string& key) { impl_->Remove(key); }

ValueBuilder::iterator ValueBuilder::begin() {
  return {*impl_, impl_->Begin()};
}

ValueBuilder::iterator ValueBuilder::end() { return {*impl_, impl_->End()}; }

bool ValueBuilder::IsEmpty() const { return impl_->IsEmpty(); }

uint32_t ValueBuilder::GetSize() const { return impl_->GetSize(); }

bool ValueBuilder::HasMember(const std::string& name) const {
  return impl_->HasMember(name);
}

void ValueBuilder::Resize(uint32_t size) { impl_->Resize(size); }

void ValueBuilder::PushBack(ValueBuilder&& elem) {
  impl_->PushBack(std::move(elem.impl_));
}

Value ValueBuilder::ExtractValue() {
  impl_->SyncBsonValue();
  return Value(
      std::exchange(impl_, std::make_shared<impl::ValueImpl>(nullptr)));
}

// Do not replace existing pointer as it may be a part of parsed value somewhere
void ValueBuilder::Assign(const impl::ValueImplPtr& impl) {
  if (impl_) {
    *impl_ = *impl;
  } else {
    impl_ = std::make_shared<impl::ValueImpl>(*impl);
  }
}

// Do not replace existing pointer as it may be a part of parsed value somewhere
void ValueBuilder::Assign(impl::ValueImplPtr&& impl) {
  if (impl_) {
    *impl_ = std::move(*impl);
  } else {
    impl_ = std::move(impl);
  }
}

}  // namespace formats::bson
