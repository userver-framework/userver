#pragma once

#include <string_view>
#include <variant>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <boost/container/small_vector.hpp>

#include <formats/json/impl/types_impl.hpp>
#include <userver/utils/overloaded.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json {

enum class ObjectProcessing { kNone, kInplaceSorting };

template <ObjectProcessing Processing>
struct ValueTypes {};

template <>
struct ValueTypes<ObjectProcessing::kInplaceSorting> {
    using Member = impl::Value::Member;
    using Value = impl::Value;

    using MemberIterator = impl::Value::MemberIterator;
    using ArrayIterator = impl::Value::ValueIterator;
};

template <>
struct ValueTypes<ObjectProcessing::kNone> {
    using Member = const impl::Value::Member;
    using Value = const impl::Value;

    using MemberIterator = impl::Value::ConstMemberIterator;
    using ArrayIterator = impl::Value::ConstValueIterator;
};

namespace impl {

template <typename Iter>
struct SubRange {
    Iter begin;
    Iter end;
};

template <ObjectProcessing Processing>
class StackCell final {
public:
    using Member = typename ValueTypes<Processing>::Member;
    using MemberIterator = typename ValueTypes<Processing>::MemberIterator;

    using Value = typename ValueTypes<Processing>::Value;
    using ArrayIterator = typename ValueTypes<Processing>::ArrayIterator;

    StackCell(MemberIterator cur_it, MemberIterator end)
        : sub_range_(SubRange<MemberIterator>{cur_it, end}),
          size_(static_cast<std::size_t>(std::distance(cur_it, end))) {}

    StackCell(ArrayIterator cur_it, ArrayIterator end)
        : sub_range_(SubRange<ArrayIterator>{cur_it, end}),
          size_(static_cast<std::size_t>(std::distance(cur_it, end))) {}

    template <typename Func1, typename Func2>
    decltype(auto) Visit(Func1 accepts_member_sub_range, Func2 accepts_array_sub_range) const {
        return std::visit(utils::Overloaded{accepts_member_sub_range, accepts_array_sub_range}, sub_range_);
    }

    void Next() {
        std::visit([](auto& arg) { ++arg.begin; }, sub_range_);
    }

    bool IsEnd() const {
        return Visit(
            [](const SubRange<MemberIterator>& member_sub_range) -> bool {
                return member_sub_range.begin == member_sub_range.end;
            },
            [](const SubRange<ArrayIterator>& array_sub_range) -> bool {
                return array_sub_range.begin == array_sub_range.end;
            }
        );
    }

    std::size_t GetSize() const noexcept { return size_; }

private:
    std::variant<SubRange<MemberIterator>, SubRange<ArrayIterator>> sub_range_;
    std::size_t size_;
};

template <ObjectProcessing Processing>
using Stack = boost::container::small_vector<StackCell<Processing>, 20>;

template <typename Handler, typename Cell>
bool WriteEnd(Handler& handler, const Cell& cell) {
    using MemberIterator = typename Cell::MemberIterator;
    using ArrayIterator = typename Cell::ArrayIterator;

    return cell.Visit(
        [&handler, size = cell.GetSize()](const SubRange<MemberIterator>&) mutable -> bool {
            return handler.EndObject(size);
        },
        [&handler, size = cell.GetSize()](const SubRange<ArrayIterator>&) mutable -> bool {
            return handler.EndArray(size);
        }
    );
}

void InplaceSortObjectChildren(impl::Value& value);

template <ObjectProcessing Processing>
void EmplaceObject(Stack<Processing>& stack, typename ValueTypes<Processing>::Value& value) {
    if constexpr (Processing == ObjectProcessing::kInplaceSorting) {
        InplaceSortObjectChildren(value);
    }
    stack.emplace_back(value.MemberBegin(), value.MemberEnd());
}

template <ObjectProcessing Processing>
void EmplaceArray(Stack<Processing>& stack, typename ValueTypes<Processing>::Value& value) {
    stack.emplace_back(value.Begin(), value.End());
}

template <ObjectProcessing Processing, typename Handler>
bool WriteStartAndEnterValue(Stack<Processing>& stack, Handler& handler) {
    using Value = typename ValueTypes<Processing>::Value;
    using MemberIterator = typename ValueTypes<Processing>::MemberIterator;
    using ArrayIterator = typename ValueTypes<Processing>::ArrayIterator;

    bool was_written = true;
    Value& value = stack.back().Visit(
        [&was_written, &handler](const SubRange<MemberIterator>& member_sub_range) mutable -> Value& {
            was_written = handler.Key(
                member_sub_range.begin->name.GetString(),
                member_sub_range.begin->name.GetStringLength(),
                /*copy=*/true
            );
            return member_sub_range.begin->value;
        },
        [](const SubRange<ArrayIterator>& array_sub_range) mutable -> Value& { return *array_sub_range.begin; }
    );

    if (!was_written) {
        return false;
    }

    if (value.IsObject()) {
        was_written = handler.StartObject();
        EmplaceObject(stack, value);
    } else if (value.IsArray()) {
        was_written = handler.StartArray();
        EmplaceArray(stack, value);
    } else {
        was_written = value.Accept(handler);
        stack.back().Next();
    }
    return was_written;
}

}  // namespace impl

template <ObjectProcessing Processing, typename Handler>
bool AcceptNoRecursion(typename ValueTypes<Processing>::Value& process_value, Handler& handler) {
    using ArrayIterator = typename ValueTypes<Processing>::ArrayIterator;

    impl::Stack<Processing> stack;

    stack.emplace_back(static_cast<ArrayIterator>(&process_value), static_cast<ArrayIterator>(&process_value + 1));

    while (true) {
        if (stack.back().IsEnd()) {
            if (stack.size() == 1) {
                return true;
            }
            if (!impl::WriteEnd(handler, stack.back())) {
                return false;
            }
            stack.pop_back();

            stack.back().Next();
        } else {
            if (!impl::WriteStartAndEnterValue(stack, handler)) {
                return false;
            }
        }
    }
}

template <typename Handler>
bool AcceptNoRecursion(const impl::Value& process_value, Handler& handler) {
    return AcceptNoRecursion<ObjectProcessing::kNone>(process_value, handler);
}

}  // namespace formats::json

USERVER_NAMESPACE_END
