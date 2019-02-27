#include <formats/bson/value_builder.hpp>

#include <boost/numeric/conversion/cast.hpp>

#include <formats/bson/value_impl.hpp>
#include <utils/assert.hpp>

namespace formats::bson {

ValueBuilder::ValueBuilder()
    : impl_(std::make_shared<impl::ValueImpl>(nullptr)) {}

ValueBuilder::ValueBuilder(impl::ValueImplPtr impl) : impl_(std::move(impl)) {}

ValueBuilder::ValueBuilder(const ValueBuilder& other) { *this = other; }

ValueBuilder::ValueBuilder(ValueBuilder&& other) { *this = std::move(other); }

ValueBuilder& ValueBuilder::operator=(const ValueBuilder& other) {
  Assign(other.impl_);
  return *this;
}

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

ValueBuilder::ValueBuilder(formats::bson::Value&& value) {
  value.CheckNotMissing();
  if (value.impl_.use_count() == 1) {
    Assign(std::move(value.impl_));
  } else {
    Assign(value.impl_);
  }
}

ValueBuilder::ValueBuilder(std::nullptr_t) : ValueBuilder() {}
ValueBuilder::ValueBuilder(bool value)
    : impl_(std::make_shared<impl::ValueImpl>(value)) {}
ValueBuilder::ValueBuilder(int32_t value)
    : impl_(std::make_shared<impl::ValueImpl>(value)) {}
ValueBuilder::ValueBuilder(int64_t value)
    : impl_(std::make_shared<impl::ValueImpl>(value)) {}
ValueBuilder::ValueBuilder(uint64_t value)
    : impl_(std::make_shared<impl::ValueImpl>(
          boost::numeric_cast<int64_t>(value))) {}
ValueBuilder::ValueBuilder(double value)
    : impl_(std::make_shared<impl::ValueImpl>(value)) {}
ValueBuilder::ValueBuilder(const char* value)
    : ValueBuilder(std::string(value)) {}
ValueBuilder::ValueBuilder(std::string value)
    : impl_(std::make_shared<impl::ValueImpl>(std::move(value))) {}
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

ValueBuilder::iterator ValueBuilder::begin() {
  return {*impl_, impl_->Begin()};
}

ValueBuilder::iterator ValueBuilder::end() { return {*impl_, impl_->End()}; }

uint32_t ValueBuilder::GetSize() const { return impl_->GetSize(); }

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
