#include <userver/formats/json/value_builder.hpp>

#include <type_traits>
#include <utility>

#include <rapidjson/allocators.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <userver/formats/common/validations.hpp>
#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/datetime_light.hpp>

#include <formats/json/impl/exttypes.hpp>
#include <formats/json/impl/types_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json {

namespace {

rapidjson::Type ToNativeType(Type type) {
    switch (type) {
        case Type::kNull:
            return rapidjson::kNullType;
        case Type::kArray:
            return rapidjson::kArrayType;
        case Type::kObject:
            return rapidjson::kObjectType;
        default:
            UASSERT_MSG(false, "No mapping from formats::json::Type to rapidjson::Type found");
            return rapidjson::kNullType;
    }
}

::rapidjson::CrtAllocator g_allocator;

bool HasMembers(const impl::Value& value) {
    return (value.IsArray() && !value.Empty()) || (value.IsObject() && !value.ObjectEmpty());
}

template <typename T>
T ValidateArithmetic(T value) {
    if constexpr (std::is_floating_point_v<T>) {
        return formats::common::ValidateFloat<Exception>(value);
    } else {
        return value;
    }
}

}  // namespace

ValueBuilder::ValueBuilder(Type type)
    : value_(
          type == Type::kNull
              ? impl::VersionedValuePtr::SharedNull()
              : impl::VersionedValuePtr::Create(ToNativeType(type))
      )
{}

ValueBuilder::ValueBuilder(const ValueBuilder& other)
    : ValueBuilder(*other.value_)
{}

// NOLINTNEXTLINE(cppcoreguidelines-noexcept-move-operations, performance-noexcept-move-constructor)
ValueBuilder::ValueBuilder(ValueBuilder&& other)
    : value_(GetMovedWrapper(other))
{}

impl::VersionedValuePtr ValueBuilder::CreateString(std::string_view value) {
    auto root = impl::VersionedValuePtr::Create(::rapidjson::Type::kNullType);
    root->SetString(rapidjson::StringRef(value.data(), value.size()), g_allocator);
    return root;
}

template <typename T>
requires impl::IsNormalizedArithmetic<T>
impl::VersionedValuePtr ValueBuilder::CreateArithmetic(T value) {
    return impl::VersionedValuePtr::Create(ValidateArithmetic(value));
}

ValueBuilder& ValueBuilder::operator=(const ValueBuilder& other) {
    if (this != &other) {
        AssignNative(impl::Value{std::as_const(other.value_)->GetNative(), g_allocator});
    }
    return *this;
}

ValueBuilder& ValueBuilder::operator=(ValueBuilder&& other) {
    if (this == &other) {
        return *this;
    }
    if (value_.IsUniqueRoot() && other.value_.IsUniqueRoot()) {
        value_ = std::exchange(other.value_, impl::MutableValueWrapper{});
    } else {
        impl::Value native;
        if (value_->holder_.Get() == other.value_->holder_.Get()) {
            Copy(native, other);
        } else {
            Move(native, std::move(other));
        }
        AssignNative(std::move(native));
    }
    return *this;
}

ValueBuilder::ValueBuilder(const formats::json::Value& other)
    : value_([&other] {
          const auto& native = other.GetNative();
          return native.IsNull()
                     ? impl::VersionedValuePtr::SharedNull()
                     : impl::VersionedValuePtr::Create(native, g_allocator);
      }())
{}

ValueBuilder::ValueBuilder(formats::json::Value&& other)
    : value_(GetMovedValueRoot(other))
{}

impl::VersionedValuePtr ValueBuilder::GetMovedValueRoot(Value& value) {
    const auto& native = std::as_const(value).GetNative();
    if (native.IsNull()) {
        return impl::VersionedValuePtr::SharedNull();
    }
    if (!value.IsUniqueReference()) {
        return impl::VersionedValuePtr::Create(native, g_allocator);
    }
    if (value.IsRoot()) {
        auto reset = Value{};
        auto root = std::move(value.holder_);
        value = std::move(reset);
        return root;
    }
    return impl::VersionedValuePtr::Create(std::move(value.GetNative()));
}

impl::MutableValueWrapper ValueBuilder::GetMovedWrapper(ValueBuilder& other) {
    if (other.value_.IsUniqueRoot()) {
        return std::exchange(other.value_, impl::MutableValueWrapper{});
    }
    auto root = impl::VersionedValuePtr::Create(::rapidjson::Type::kNullType);
    Move(*root, std::move(other));
    return impl::MutableValueWrapper{std::move(root)};
}

void ValueBuilder::AssignNative(impl::Value native) {
    auto& current = *value_;
    if (native.IsNull() && current.holder_.IsSharedNull()) {
        return;
    }
    if (HasMembers(std::as_const(current).GetNative())) {
        value_.OnMembersChange();
    }
    current.GetNative() = std::move(native);
}

void ValueBuilder::AssignNull() { AssignNative(impl::Value{}); }

void ValueBuilder::AssignString(std::string_view value) {
    impl::Value native;
    native.SetString(rapidjson::StringRef(value.data(), value.size()), g_allocator);
    AssignNative(std::move(native));
}

void ValueBuilder::AssignValue(const Value& value) { AssignNative(impl::Value{value.GetNative(), g_allocator}); }

void ValueBuilder::AssignValue(Value&& value) { AssignNative(std::move(value).MoveNativeOrCopy(g_allocator)); }

template <typename T>
requires impl::IsNormalizedArithmetic<T>
void ValueBuilder::AssignArithmetic(T value) {
    AssignNative(impl::Value{ValidateArithmetic(value)});
}

ValueBuilder::ValueBuilder(EmplaceEnabler, impl::MutableValueWrapper value) noexcept : ValueBuilder(std::move(value)) {}

ValueBuilder::ValueBuilder(impl::MutableValueWrapper value) noexcept : value_(std::move(value)) {}

ValueBuilder::ValueBuilder(common::TransferTag, ValueBuilder&& value) noexcept : value_(std::move(value.value_)) {}

impl::VersionedValuePtr ValueBuilder::GetSerializedRoot(Value value) {
    if (!value.IsUniqueReference()) {
        const auto& native = std::as_const(value).GetNative();
        return native.IsNull()
                   ? impl::VersionedValuePtr::SharedNull()
                   : impl::VersionedValuePtr::Create(native, g_allocator);
    }
    if (value.IsRoot()) {
        return std::move(value.holder_);
    }
    const auto& native = std::as_const(value).GetNative();
    if (native.IsNull()) {
        return impl::VersionedValuePtr::SharedNull();
    }
    return impl::VersionedValuePtr::Create(std::move(value.GetNative()));
}

ValueBuilder ValueBuilder::operator[](std::string key) {
    auto& member = AddMember(key, CheckMemberExists::kYes);
    return ValueBuilder{value_.WrapMember(std::move(key), member)};
}

ValueBuilder ValueBuilder::operator[](std::size_t index) {
    value_->CheckInBounds(index);
    return ValueBuilder{value_.WrapElement(index)};
}

void ValueBuilder::EmplaceNocheck(std::string_view key, ValueBuilder value) {
    Move(AddMember(key, CheckMemberExists::kNo), std::move(value));
}

void ValueBuilder::EmplaceNocheck(std::string_view key, const formats::json::Value& value) {
    AddMember(key, CheckMemberExists::kNo, [&value] { return impl::Value{value.GetNative(), g_allocator}; });
}

void ValueBuilder::EmplaceNocheck(std::string_view key, formats::json::Value&& value) {
    AddMember(key, CheckMemberExists::kNo, [&value] { return std::move(value).MoveNativeOrCopy(g_allocator); });
}

void ValueBuilder::EmplaceNocheck(std::string_view key, std::nullptr_t) { AddMember(key, CheckMemberExists::kNo); }

void ValueBuilder::EmplaceNocheck(std::string_view key, const std::string& value) {
    EmplaceNocheck(key, std::string_view{value});
}

void ValueBuilder::EmplaceNocheck(std::string_view key, std::string_view value) {
    AddMember(key, CheckMemberExists::kNo, [value] {
        impl::Value member;
        member.SetString(rapidjson::StringRef(value.data(), value.size()), g_allocator);
        return member;
    });
}

template <typename T>
requires impl::IsNormalizedArithmetic<T>
void ValueBuilder::EmplaceNocheckArithmetic(std::string_view key, T value) {
    const auto validated = ValidateArithmetic(value);
    AddMember(key, CheckMemberExists::kNo, [validated] { return impl::Value{validated}; });
}

void ValueBuilder::Remove(std::string_view key) {
    value_->CheckObject();
    if (value_->GetNative().RemoveMember(impl::MakeJsonStringViewValue(key))) {
        value_.OnMembersChange();
    }
}

ValueBuilder::iterator ValueBuilder::begin() {
    value_->CheckObjectOrArrayOrNull();
    return iterator{value_, 0};
}

ValueBuilder::iterator ValueBuilder::end() {
    value_->CheckObjectOrArrayOrNull();
    return iterator{value_, static_cast<int>(GetSize())};
}

bool ValueBuilder::IsEmpty() const { return value_->IsEmpty(); }

bool ValueBuilder::IsNull() const noexcept { return value_->IsNull(); }

bool ValueBuilder::IsBool() const noexcept { return value_->IsBool(); }

bool ValueBuilder::IsInt() const noexcept { return value_->IsInt(); }

bool ValueBuilder::IsInt64() const noexcept { return value_->IsInt64(); }

bool ValueBuilder::IsUInt64() const noexcept { return value_->IsUInt64(); }

bool ValueBuilder::IsDouble() const noexcept { return value_->IsDouble(); }

bool ValueBuilder::IsString() const noexcept { return value_->IsString(); }

bool ValueBuilder::IsArray() const noexcept { return value_->IsArray(); }

bool ValueBuilder::IsObject() const noexcept { return value_->IsObject(); }

std::size_t ValueBuilder::GetSize() const { return value_->GetSize(); }

std::size_t ValueBuilder::GetCapacity() const {
    const auto& native = value_->GetNative();
    if (native.IsArray()) {
        return native.Capacity();
    }
    if (native.IsObject()) {
        return native.MemberCapacity();
    }
    throw TypeMismatchException(value_->GetExtendedType(), impl::kArrayValue, value_->GetPath());
}

bool ValueBuilder::HasMember(std::string_view key) const { return value_->HasMember(key); }

std::string ValueBuilder::GetPath() const { return value_->GetPath(); }

void ValueBuilder::Reserve(std::size_t capacity) {
    auto& native = value_->GetNative();
    std::size_t old_capacity = 0;
    if (native.IsArray()) {
        old_capacity = native.Capacity();
        native.Reserve(capacity, g_allocator);
    } else if (native.IsObject()) {
        old_capacity = native.MemberCapacity();
        native.MemberReserve(capacity, g_allocator);
    } else {
        throw TypeMismatchException(value_->GetExtendedType(), impl::kArrayValue, value_->GetPath());
    }

    if (old_capacity && capacity > old_capacity) {
        value_.OnMembersChange();
    }
}

void ValueBuilder::Resize(std::size_t size) {
    value_->CheckArrayOrNull();
    auto& native = value_->GetNative();

    if (native.IsNull()) {
        native.SetArray();
    }

    const auto old_capacity = native.Capacity();

    if (size > old_capacity) {
        native.Reserve(size, g_allocator);
        if (old_capacity) {
            value_.OnMembersChange();
        }
    }
    for (size_t curr_size = native.Size(); curr_size > size; --curr_size) {
        native.PopBack();
    }
    for (size_t curr_size = native.Size(); curr_size < size; ++curr_size) {
        native.PushBack(impl::Value{}, g_allocator);
    }
}

template <typename MakeValue>
USERVER_IMPL_NODEBUG_INLINE_FUNC inline void ValueBuilder::PushBackNative(MakeValue&& make_value) {
    auto& value = *value_;
    value.CheckArrayOrNull();
    auto& native = value.GetNative();
    const auto old_capacity = native.IsNull() ? 0 : native.Capacity();
    {
        auto&& member = std::forward<MakeValue>(make_value)();
        if (native.IsNull()) {
            native.SetArray();
        }
        native.PushBack(std::move(member), g_allocator);
    }
    if (old_capacity && old_capacity != native.Capacity()) {
        value_.OnMembersChange();
    }
}

void ValueBuilder::PushBack(std::nullptr_t) {
    PushBackNative([] { return impl::Value{}; });
}

void ValueBuilder::PushBack(const std::string& value) { PushBack(std::string_view{value}); }

void ValueBuilder::PushBack(std::string_view value) {
    PushBackNative([value] {
        impl::Value native_value;
        native_value.SetString(rapidjson::StringRef(value.data(), value.size()), g_allocator);
        return native_value;
    });
}

template <typename T>
requires impl::IsNormalizedArithmetic<T>
void ValueBuilder::PushBackArithmetic(T value) {
    const auto validated = ValidateArithmetic(value);
    PushBackNative([validated] { return impl::Value{validated}; });
}

void ValueBuilder::CheckSingleElementInitializerList(std::size_t size) {
    if (size != 1) {
        throw Exception(
            "ValueBuilder insertion accepts exactly one element in a braced initializer; "
            "call PushBack or EmplaceNocheck separately for each value"
        );
    }
}

void ValueBuilder::PushBack(ValueBuilder&& bld) {
    PushBackNative([this, &bld] {
        impl::Value member;
        if (value_->holder_.Get() == bld.value_->holder_.Get()) {
            Copy(member, bld);
        } else {
            Move(member, std::move(bld));
        }
        return member;
    });
}

void ValueBuilder::PushBack(const formats::json::Value& value) {
    PushBackNative([&value] { return impl::Value{value.GetNative(), g_allocator}; });
}

void ValueBuilder::PushBack(formats::json::Value&& value) {
    PushBackNative([&value] { return std::move(value).MoveNativeOrCopy(g_allocator); });
}

formats::json::Value ValueBuilder::ExtractValue() {
    if (!value_->IsRoot()) {
        throw Exception("Extract should be called only from the root builder");
    }
    return std::move(value_).ExtractValue();
}

void ValueBuilder::Copy(impl::Value& to, const ValueBuilder& from) {
    to.CopyFrom(std::as_const(from.value_)->GetNative(), g_allocator);
}

void ValueBuilder::Move(impl::Value& to, ValueBuilder&& from) {
    if (!from.value_.IsRoot()) {
        Copy(to, from);
        return;
    }
    auto& value = *from.value_;
    const auto& native = std::as_const(value).GetNative();
    if (native.IsNull()) {
        to.SetNull();
        return;
    }
    if (HasMembers(native)) {
        from.value_.OnMembersChange();
    }
    to = std::move(value.GetNative());
}

template <typename MakeValue>
USERVER_IMPL_NODEBUG_INLINE_FUNC inline impl::Value& ValueBuilder::AddMember(
    std::string_view key,
    CheckMemberExists check_exists,
    MakeValue&& make_value
) {
    value_->CheckObjectOrNull();
    auto& native = value_->GetNative();

    if (check_exists == CheckMemberExists::kYes && native.IsObject()) {
        auto it = native.FindMember(impl::MakeJsonStringViewValue(key));
        if (it != native.MemberEnd()) {
            return it->value;
        }
    }

    impl::Value native_key(key.data(), key.size(), g_allocator);
    auto&& member = std::forward<MakeValue>(make_value)();
    if (native.IsNull()) {
        native.SetObject();
    }
    // notify wrapper when members capacity (and thus location) changes
    const auto old_capacity = native.MemberCapacity();
    native.AddMember(std::move(native_key), std::move(member), g_allocator);
    if (old_capacity && old_capacity != native.MemberCapacity()) {
        value_.OnMembersChange();
    }

    return std::prev(native.MemberEnd())->value;
}

impl::Value& ValueBuilder::AddMember(std::string_view key, CheckMemberExists check_exists) {
    return AddMember(key, check_exists, [] { return impl::Value{}; });
}

Value Serialize(std::chrono::system_clock::time_point tp, formats::serialize::To<Value>) {
    json::ValueBuilder builder = utils::datetime::UtcTimestring(tp, utils::datetime::kRfc3339Format);
    return builder.ExtractValue();
}

template impl::VersionedValuePtr ValueBuilder::CreateArithmetic(bool);
template impl::VersionedValuePtr ValueBuilder::CreateArithmetic(int);
template impl::VersionedValuePtr ValueBuilder::CreateArithmetic(unsigned int);
template impl::VersionedValuePtr ValueBuilder::CreateArithmetic(std::int64_t);
template impl::VersionedValuePtr ValueBuilder::CreateArithmetic(std::uint64_t);
template impl::VersionedValuePtr ValueBuilder::CreateArithmetic(float);
template impl::VersionedValuePtr ValueBuilder::CreateArithmetic(double);

template void ValueBuilder::AssignArithmetic(bool);
template void ValueBuilder::AssignArithmetic(int);
template void ValueBuilder::AssignArithmetic(unsigned int);
template void ValueBuilder::AssignArithmetic(std::int64_t);
template void ValueBuilder::AssignArithmetic(std::uint64_t);
template void ValueBuilder::AssignArithmetic(float);
template void ValueBuilder::AssignArithmetic(double);

template void ValueBuilder::EmplaceNocheckArithmetic(std::string_view, bool);
template void ValueBuilder::EmplaceNocheckArithmetic(std::string_view, int);
template void ValueBuilder::EmplaceNocheckArithmetic(std::string_view, unsigned int);
template void ValueBuilder::EmplaceNocheckArithmetic(std::string_view, std::int64_t);
template void ValueBuilder::EmplaceNocheckArithmetic(std::string_view, std::uint64_t);
template void ValueBuilder::EmplaceNocheckArithmetic(std::string_view, float);
template void ValueBuilder::EmplaceNocheckArithmetic(std::string_view, double);

template void ValueBuilder::PushBackArithmetic(bool);
template void ValueBuilder::PushBackArithmetic(int);
template void ValueBuilder::PushBackArithmetic(unsigned int);
template void ValueBuilder::PushBackArithmetic(std::int64_t);
template void ValueBuilder::PushBackArithmetic(std::uint64_t);
template void ValueBuilder::PushBackArithmetic(float);
template void ValueBuilder::PushBackArithmetic(double);

}  // namespace formats::json

USERVER_NAMESPACE_END
