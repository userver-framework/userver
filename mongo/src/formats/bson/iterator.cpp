#include <formats/bson/iterator.hpp>

#include <formats/bson/exception.hpp>
#include <formats/bson/value.hpp>
#include <formats/bson/value_builder.hpp>
#include <formats/bson/value_impl.hpp>

namespace formats {
namespace bson {

template <typename ValueType>
Iterator<ValueType>::ArrowProxy::ArrowProxy(ValueType value)
    : value_(std::move(value)) {}

template <typename ValueType>
Iterator<ValueType>::Iterator(impl::ValueImpl& iterable, NativeIter it)
    : iterable_(&iterable), it_(std::move(it)) {}

template <typename ValueType>
Iterator<ValueType> Iterator<ValueType>::operator++(int) {
  Iterator<ValueType> tmp(*this);
  ++*this;
  return tmp;
}

template <typename ValueType>
Iterator<ValueType>& Iterator<ValueType>::operator++() {
  boost::apply_visitor([](auto& it) { ++it; }, it_);
  return *this;
}

template <typename ValueType>
ValueType Iterator<ValueType>::operator*() const {
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
  return ValueType(boost::apply_visitor(Visitor{}, it_));
}

template <typename ValueType>
typename Iterator<ValueType>::ArrowProxy Iterator<ValueType>::operator->()
    const {
  return ArrowProxy(**this);
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
  return boost::apply_visitor(Visitor(*iterable_), it_);
}

template <typename ValueType>
uint32_t Iterator<ValueType>::GetIndex() const {
  class Visitor {
   public:
    Visitor(impl::ValueImpl& iterable) : iterable_(iterable) {}

    uint32_t operator()(impl::ParsedArray::const_iterator it) const {
      return it -
             boost::get<impl::ParsedArray::const_iterator>(iterable_.Begin());
    }

    uint32_t operator()(impl::ParsedDocument::const_iterator) const {
      throw TypeMismatchException(BSON_TYPE_DOCUMENT, BSON_TYPE_ARRAY,
                                  iterable_.GetPath());
    }

   private:
    impl::ValueImpl& iterable_;
  };
  return boost::apply_visitor(Visitor(*iterable_), it_);
}

// Template instantiations
template class Iterator<Value>;
template class Iterator<ValueBuilder>;

}  // namespace bson
}  // namespace formats
