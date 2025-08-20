#pragma once

#include <type_traits>
#include <utility>

#include <userver/proto-structs/impl/traits_light.hpp>
#include <userver/proto-structs/type_mapping.hpp>

namespace google::protobuf {
class FieldDescriptor;
class Message;
}  // namespace google::protobuf

USERVER_NAMESPACE_BEGIN

namespace proto_structs::impl {

class FieldAccessor {
public:
    FieldAccessor(const ::google::protobuf::Message& message, int field_number) noexcept
        : message_(message), field_number_(field_number) {}

    const ::google::protobuf::Message& GetMessage() const noexcept { return message_; }
    int GetFieldNumber() const noexcept { return field_number_; }

    const ::google::protobuf::FieldDescriptor* GetFieldDescriptor() const noexcept;

private:
    const ::google::protobuf::Message& message_;
    int field_number_;
};

template <proto_structs::traits::ProtoMessage TMessage, typename TReturn>
class FieldGetter : public FieldAccessor {
public:
    using Message = TMessage;
    using ReturnType = TReturn;
    using GetFunc = ReturnType (Message::*)() const;

    FieldGetter(const Message& message, int field_number, GetFunc get_func) noexcept
        : FieldAccessor(message, field_number), message_(message), get_func_(get_func) {}

    const Message& GetMessage() const noexcept { return message_; }
    ReturnType GetValue() const { return (message_.*get_func_)(); }

private:
    const Message& message_;
    GetFunc get_func_;
};

template <proto_structs::traits::ProtoMessage TMessage, typename TReturn>
class FieldGetterWithPresence final : public FieldGetter<TMessage, TReturn> {
public:
    using Base = FieldGetter<TMessage, TReturn>;
    using typename Base::GetFunc;
    using typename Base::Message;
    using HasFunc = bool (Message::*)() const;

    FieldGetterWithPresence(const Message& message, int field_number, GetFunc get_func, HasFunc has_func) noexcept
        : Base(message, field_number, get_func), has_func_(has_func) {}

    bool HasValue() const { return (Base::GetMessage().*has_func_)(); }

private:
    HasFunc has_func_;
};

template <proto_structs::traits::ProtoMessage TMessage>
class FieldSetter : public FieldAccessor {
public:
    using Message = TMessage;
    using ClearFunc = void (Message::*)();

    FieldSetter(Message& message, int field_number, ClearFunc clear_func) noexcept
        : FieldAccessor(message, field_number), message_(message), clear_func_(clear_func) {}

    Message& GetMessage() const noexcept { return message_; }
    void ClearValue() const { (message_.*clear_func_)(); }

private:
    Message& message_;
    ClearFunc clear_func_;
};

template <proto_structs::traits::ProtoMessage TMessage, typename TArg>
class FieldSetterWithArg : public FieldSetter<TMessage> {
public:
    using Base = FieldSetter<TMessage>;
    using typename Base::Message;
    using ArgType = TArg;
    using typename Base::ClearFunc;
    using SetFunc = void (Message::*)(ArgType);

    FieldSetterWithArg(Message& message, int field_number, SetFunc set_func, ClearFunc clear_func)
        : Base(message, field_number, clear_func), set_func_(set_func) {}

    void SetValue(ArgType value) const { (Base::GetMessage().*set_func_)(value); }

private:
    SetFunc set_func_;
};

template <proto_structs::traits::ProtoMessage TMessage, typename TReturn>
class FieldSetterWithMutable : public FieldSetter<TMessage> {
public:
    using Base = FieldSetter<TMessage>;
    using ReturnType = TReturn;
    using typename Base::ClearFunc;
    using typename Base::Message;
    using GetMutableFunc = TReturn (Message::*)();

    FieldSetterWithMutable(Message& message, int field_number, GetMutableFunc get_mutable_func, ClearFunc clear_func)
        : Base(message, field_number, clear_func), get_mutable_func_(get_mutable_func) {}

    ReturnType GetMutableValue() const { (Base::GetMessage().*get_mutable_func_)(); }

private:
    GetMutableFunc get_mutable_func_;
};

template <proto_structs::traits::ProtoMessage TMessage, typename TReturn>
auto CreateFieldGetter(const TMessage& message, int field_number, TReturn (TMessage::*get_func)() const) noexcept {
    return FieldGetter<TMessage, TReturn>(message, field_number, get_func);
}

template <proto_structs::traits::ProtoMessage TMessage, typename TReturn>
auto CreateFieldGetter(
    const TMessage& message,
    int field_number,
    TReturn (TMessage::*get_func)() const,
    bool (TMessage::*has_func)() const noexcept
) {
    return FieldGetterWithPresence<TMessage, TReturn>(message, field_number, get_func, has_func);
}

template <proto_structs::traits::ProtoMessage TMessage, typename TArg>
auto CreateFieldSetter(
    TMessage& message,
    int field_number,
    void (TMessage::*set_func)(TArg),
    void (TMessage::*clear_func)()
) noexcept {
    return FieldSetterWithArg<TMessage, TArg>(message, field_number, set_func, clear_func);
}

template <proto_structs::traits::ProtoMessage TMessage, typename TReturn>
auto CreateFieldSetter(
    TMessage& message,
    int field_number,
    TReturn (TMessage::*get_mutable_func)(),
    void (TMessage::*clear_func)()
) noexcept {
    return FieldSetterWithMutable<TMessage, TReturn>(message, field_number, get_mutable_func, clear_func);
}

namespace traits {

template <typename T>
concept FieldGetter = InheritsFromInstantiation<FieldGetter, T>;

template <typename T>
concept FieldGetterWithPresence = InheritsFromInstantiation<FieldGetterWithPresence, T>;

template <typename T>
concept FieldSetter = InheritsFromInstantiation<FieldSetter, T>;

}  // namespace traits

}  // namespace proto_structs::impl

USERVER_NAMESPACE_END
