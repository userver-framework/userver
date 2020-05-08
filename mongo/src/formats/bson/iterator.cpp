#include <formats/bson/iterator.hpp>

#include <formats/bson/exception.hpp>
#include <formats/bson/value.hpp>
#include <formats/bson/value_builder.hpp>
#include <formats/bson/value_impl.hpp>

namespace formats::bson {

template <typename ValueType>
Iterator<ValueType>::Iterator(impl::ValueImpl& iterable, NativeIter it)
    : iterable_(&iterable), it_(std::move(it)) {}

template <typename ValueType>
Iterator<ValueType>::Iterator(const Iterator& other)
    : iterable_(other.iterable_), it_(other.it_) {}

template <typename ValueType>
Iterator<ValueType>::Iterator(Iterator&& other) noexcept
    : iterable_(other.iterable_), it_(std::move(other.it_)) {}

template <typename ValueType>
Iterator<ValueType>& Iterator<ValueType>::operator=(const Iterator& other) {
  iterable_ = other.iterable_;
  it_ = other.it_;
  current_.reset();
  return *this;
}

template <typename ValueType>
Iterator<ValueType>& Iterator<ValueType>::operator=(Iterator&& other) noexcept {
  iterable_ = other.iterable_;
  it_ = std::move(other.it_);
  current_.reset();
  return *this;
}

template <typename ValueType>
Iterator<ValueType> Iterator<ValueType>::operator++(int) {
  Iterator<ValueType> tmp(*this);
  ++*this;
  return tmp;
}

template <typename ValueType>
Iterator<ValueType>& Iterator<ValueType>::operator++() {
  current_.reset();
  std::visit([](auto& it) { ++it; }, it_);
  return *this;
}

template <typename ValueType>
typename Iterator<ValueType>::reference Iterator<ValueType>::operator*() const {
  UpdateValue();
  return *current_;
}

template <typename ValueType>
typename Iterator<ValueType>::pointer Iterator<ValueType>::operator->() const {
  return &**this;
}

template <typename ValueType>
bool Iterator<ValueType>::operator==(const Iterator& rhs) const {
  return it_ == rhs.it_;
}

template <typename ValueType>
bool Iterator<ValueType>::operator!=(const Iterator& rhs) const {
  return it_ != rhs.it_;
}

template <typename ValueType>
std::string Iterator<ValueType>::GetName() const {
  class Visitor {
   public:
    Visitor(const impl::ValueImpl& iterable) : iterable_(iterable) {}

    std::string operator()(impl::ParsedArray::const_iterator) const {
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

template <typename ValueType>
uint32_t Iterator<ValueType>::GetIndex() const {
  class Visitor {
   public:
    Visitor(impl::ValueImpl& iterable) : iterable_(iterable) {}

    uint32_t operator()(impl::ParsedArray::const_iterator it) const {
      return it -
             std::get<impl::ParsedArray::const_iterator>(iterable_.Begin());
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

template <typename ValueType>
void Iterator<ValueType>::UpdateValue() const {
  if (current_) return;

  class Visitor {
   public:
    impl::ValueImplPtr operator()(impl::ParsedArray::const_iterator it) const {
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
template class Iterator<const Value>;
template class Iterator<ValueBuilder>;

}  // namespace formats::bson
