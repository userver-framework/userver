#include <userver/formats/yaml/iterator.hpp>

#include <yaml-cpp/yaml.h>

#include <userver/formats/yaml/exception.hpp>
#include <userver/formats/yaml/value.hpp>
#include <userver/formats/yaml/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::yaml {

template <typename IterTraits>
Iterator<IterTraits>::Iterator(const typename IterTraits::native_iter& iter, int index, const formats::yaml::Path& path)
    : iter_pimpl_(iter), path_(path), index_(index) {}

template <typename IterTraits>
Iterator<IterTraits>::Iterator(const Iterator<IterTraits>& other)
    : iter_pimpl_(other.iter_pimpl_), path_(other.path_), index_(other.index_) {}

template <typename IterTraits>
Iterator<IterTraits>::Iterator(Iterator<IterTraits>&& other) noexcept
    : iter_pimpl_(std::move(other.iter_pimpl_)), path_(std::move(other.path_)), index_(other.index_) {}

template <typename IterTraits>
Iterator<IterTraits>& Iterator<IterTraits>::operator=(const Iterator<IterTraits>& other) {
    if (this == &other) return *this;

    iter_pimpl_ = other.iter_pimpl_;
    path_ = other.path_;
    index_ = other.index_;
    current_.reset();
    return *this;
}

template <typename IterTraits>
Iterator<IterTraits>& Iterator<IterTraits>::operator=(Iterator<IterTraits>&& other) noexcept {
    iter_pimpl_ = std::move(other.iter_pimpl_);
    path_ = std::move(other.path_);
    index_ = other.index_;
    current_.reset();
    return *this;
}

template <typename IterTraits>
Iterator<IterTraits>::~Iterator() = default;

template <typename IterTraits>
Iterator<IterTraits> Iterator<IterTraits>::operator++(int) {
    current_.reset();
    const auto index_copy = index_;
    if (index_ != -1) {
        ++index_;
    }
    return Iterator<IterTraits>((*iter_pimpl_)++, index_copy, path_);
}

template <typename IterTraits>
Iterator<IterTraits>& Iterator<IterTraits>::operator++() {
    current_.reset();
    ++(*iter_pimpl_);
    if (index_ != -1) {
        ++index_;
    }
    return *this;
}

template <typename IterTraits>
typename Iterator<IterTraits>::reference Iterator<IterTraits>::operator*() const {
    UpdateValue();
    return *current_;
}

template <typename IterTraits>
typename Iterator<IterTraits>::pointer Iterator<IterTraits>::operator->() const {
    UpdateValue();
    return &**this;
}

template <typename IterTraits>
bool Iterator<IterTraits>::operator==(const Iterator<IterTraits>& other) const {
    return *iter_pimpl_ == *other.iter_pimpl_;
}

template <typename IterTraits>
bool Iterator<IterTraits>::operator!=(const Iterator<IterTraits>& other) const {
    return *iter_pimpl_ != *other.iter_pimpl_;
}

template <typename IterTraits>
std::string Iterator<IterTraits>::GetName() const {
    if (index_ != -1) {
        throw TypeMismatchException(Type::kArray, Type::kObject, path_.ToStringView());
    }
    return (*iter_pimpl_)->first.Scalar();
}

template <typename IterTraits>
uint32_t Iterator<IterTraits>::GetIndex() const {
    if (index_ == -1) {
        throw TypeMismatchException(Type::kObject, Type::kArray, path_.ToStringView());
    }
    return index_;
}

template <typename IterTraits>
Type Iterator<IterTraits>::GetIteratorType() const {
    if (index_ == -1) {
        return Type::kObject;
    } else {
        return Type::kArray;
    }
}

template <typename IterTraits>
void Iterator<IterTraits>::UpdateValue() const {
    if (current_) return;

    if (index_ == -1) {
        current_.emplace(typename value_type::EmplaceEnabler{}, (*iter_pimpl_)->second, path_, GetName());
    } else {
        current_.emplace(typename value_type::EmplaceEnabler{}, **iter_pimpl_, path_, index_);
    }
}

// Explicit instantiation
template class Iterator<Value::IterTraits>;
template class Iterator<ValueBuilder::IterTraits>;

}  // namespace formats::yaml

USERVER_NAMESPACE_END
