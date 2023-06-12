#include <userver/formats/bson/iterator.hpp>

#include <formats/bson/value_impl.hpp>
#include <userver/formats/bson/exception.hpp>
#include <userver/formats/bson/value.hpp>
#include <userver/formats/bson/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::bson {

namespace {

using IteratorDirection = common::IteratorDirection;

}  // namespace

template <typename ValueType, IteratorDirection Direction>
Iterator<ValueType, Direction>::Iterator(impl::ValueImpl& iterable,
                                         NativeIter it)
    : iterable_(&iterable), it_(std::move(it)) {}

template <typename ValueType, IteratorDirection Direction>
Iterator<ValueType, Direction>::Iterator(const Iterator& other)
    : iterable_(other.iterable_), it_(other.it_) {}

template <typename ValueType, IteratorDirection Direction>
Iterator<ValueType, Direction>::Iterator(Iterator&& other) noexcept
    : iterable_(other.iterable_), it_(std::move(other.it_)) {}

template <typename ValueType, IteratorDirection Direction>
Iterator<ValueType, Direction>& Iterator<ValueType, Direction>::operator=(
    const Iterator& other) {
  if (this == &other) return *this;

  iterable_ = other.iterable_;
  it_ = other.it_;
  current_.reset();
  return *this;
}

template <typename ValueType, IteratorDirection Direction>
Iterator<ValueType, Direction>& Iterator<ValueType, Direction>::operator=(
    Iterator&& other) noexcept {
  iterable_ = other.iterable_;
  it_ = std::move(other.it_);
  current_.reset();
  return *this;
}

template <typename ValueType, IteratorDirection Direction>
Iterator<ValueType, Direction> Iterator<ValueType, Direction>::operator++(int) {
  Iterator<ValueType, Direction> tmp(*this);
  ++*this;
  return tmp;
}

template <typename ValueType, IteratorDirection Direction>
Iterator<ValueType, Direction>& Iterator<ValueType, Direction>::operator++() {
  current_.reset();
  std::visit([](auto& it) { ++it; }, it_);
  return *this;
}

template <typename ValueType, IteratorDirection Direction>
typename Iterator<ValueType, Direction>::reference
Iterator<ValueType, Direction>::operator*() const {
  UpdateValue();
  return *current_;
}

template <typename ValueType, IteratorDirection Direction>
typename Iterator<ValueType, Direction>::pointer
Iterator<ValueType, Direction>::operator->() const {
  return &**this;
}

template <typename ValueType, IteratorDirection Direction>
bool Iterator<ValueType, Direction>::operator==(const Iterator& rhs) const {
  return it_ == rhs.it_;
}

template <typename ValueType, IteratorDirection Direction>
bool Iterator<ValueType, Direction>::operator!=(const Iterator& rhs) const {
  return it_ != rhs.it_;
}

template <typename ValueType, IteratorDirection Direction>
std::string Iterator<ValueType, Direction>::GetNameImpl() const {
  class Visitor {
   public:
    Visitor(const impl::ValueImpl& iterable) : iterable_(iterable) {}

    std::string operator()(impl::ParsedArray::const_iterator) const {
      throw TypeMismatchException(BSON_TYPE_ARRAY, BSON_TYPE_DOCUMENT,
                                  iterable_.GetPath());
    }

    std::string operator()(impl::ParsedArray::const_reverse_iterator) const {
      throw TypeMismatchException(BSON_TYPE_ARRAY, BSON_TYPE_DOCUMENT,
                                  iterable_.GetPath());
    }

    std::string operator()(impl::ParsedDocument::const_iterator it) const {
      return it->first;
    }

   private:
    const impl::ValueImpl& iterable_;
  };
  return std::visit(Visitor(*iterable_), it_);
}

template <typename ValueType, IteratorDirection Direction>
uint32_t Iterator<ValueType, Direction>::GetIndex() const {
  class Visitor {
   public:
    Visitor(impl::ValueImpl& iterable) : iterable_(iterable) {}

    uint32_t operator()(impl::ParsedArray::const_iterator it) const {
      return it -
             std::get<impl::ParsedArray::const_iterator>(iterable_.Begin());
    }

    uint32_t operator()(impl::ParsedArray::const_reverse_iterator it) const {
      return it - std::get<impl::ParsedArray::const_reverse_iterator>(
                      iterable_.Rbegin());
    }

    uint32_t operator()(impl::ParsedDocument::const_iterator) const {
      throw TypeMismatchException(BSON_TYPE_DOCUMENT, BSON_TYPE_ARRAY,
                                  iterable_.GetPath());
    }

   private:
    impl::ValueImpl& iterable_;
  };
  return std::visit(Visitor(*iterable_), it_);
}

template <typename ValueType, IteratorDirection Direction>
void Iterator<ValueType, Direction>::UpdateValue() const {
  if (current_) return;

  class Visitor {
   public:
    impl::ValueImplPtr operator()(impl::ParsedArray::const_iterator it) const {
      return *it;
    }

    impl::ValueImplPtr operator()(
        impl::ParsedArray::const_reverse_iterator it) const {
      return *it;
    }

    impl::ValueImplPtr operator()(
        impl::ParsedDocument::const_iterator it) const {
      return it->second;
    }
  };
  current_.emplace(std::visit(Visitor{}, it_));
}

// Template instantiations
template class Iterator<const Value, IteratorDirection::kForward>;
template class Iterator<const Value, IteratorDirection::kReverse>;
template class Iterator<ValueBuilder, IteratorDirection::kForward>;
template class Iterator<ValueBuilder, IteratorDirection::kReverse>;

}  // namespace formats::bson

USERVER_NAMESPACE_END
