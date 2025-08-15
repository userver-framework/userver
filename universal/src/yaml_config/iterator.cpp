#include <userver/yaml_config/iterator.hpp>

#include <userver/utils/assert.hpp>
#include <userver/utils/text_light.hpp>
#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace yaml_config {

namespace {

std::string_view RemoveInternalSuffix(std::string_view key) noexcept {
    static constexpr std::string_view kInternalSuffixes[] = {
        "#env",
        "#file",
        "#fallback",
    };

    for (const auto suffix : kInternalSuffixes) {
        if (utils::text::EndsWith(key, suffix)) {
            return key.substr(0, key.size() - suffix.size());
        }
    }
    return key;
}

}  // namespace

template <typename IterTraits>
Iterator<IterTraits>::Iterator(const Iterator<IterTraits>& other) : container_(other.container_), it_(other.it_) {
    current_.reset();
}

template <typename IterTraits>
Iterator<IterTraits>::Iterator(Iterator<IterTraits>&& other) noexcept
    : container_(std::exchange(other.container_, nullptr)), it_(std::move(other.it_)) {
    current_.reset();
}

template <typename IterTraits>
Iterator<IterTraits>& Iterator<IterTraits>::operator=(const Iterator<IterTraits>& other) {
    if (this == &other) return *this;

    it_ = other.it_;
    container_ = other.container_;
    current_.reset();

    return *this;
}

template <typename IterTraits>
Iterator<IterTraits>& Iterator<IterTraits>::operator=(Iterator<IterTraits>&& other) noexcept {
    it_ = std::move(other.it_);
    container_ = std::exchange(other.container_, nullptr);
    current_.reset();

    return *this;
}

template <typename IterTraits>
Iterator<IterTraits> Iterator<IterTraits>::operator++(int) {
    auto it_copy = it_;
    IncrementInternalIterator();
    return Iterator{*container_, std::move(it_copy)};
}

template <typename IterTraits>
Iterator<IterTraits>& Iterator<IterTraits>::operator++() {
    IncrementInternalIterator();
    return *this;
}

template <typename IterTraits>
std::string Iterator<IterTraits>::GetName() const {
    return std::string{yaml_config::RemoveInternalSuffix(it_.GetName())};
}

template <typename IterTraits>
void Iterator<IterTraits>::UpdateValue() const {
    UASSERT(container_ != nullptr);
    if (current_) return;

    if (it_.GetIteratorType() == formats::common::Type::kArray) {
        current_ = (*container_)[it_.GetIndex()];
    } else {
        UASSERT(it_.GetIteratorType() == formats::common::Type::kObject);
        current_ = (*container_)[yaml_config::RemoveInternalSuffix(it_.GetName())];
    }
}

template <typename IterTraits>
void Iterator<IterTraits>::IncrementInternalIterator() {
    current_.reset();

    if (it_.GetIteratorType() != formats::common::Type::kObject) {
        ++it_;
        return;
    }

    const auto initial_name_raw = it_.GetName();
    const auto initial_name = yaml_config::RemoveInternalSuffix(initial_name_raw);

    UASSERT(container_);
    const auto end = container_->end().it_;

    ++it_;
    for (; it_ != end; ++it_) {
        UASSERT(it_.GetIteratorType() == formats::common::Type::kObject);
        const auto new_name_raw = it_.GetName();
        const auto new_name = yaml_config::RemoveInternalSuffix(new_name_raw);

        if (initial_name != new_name) {
            break;
        }
    }
}

// Explicit instantiation
template class Iterator<YamlConfig::IterTraits>;

}  // namespace yaml_config

USERVER_NAMESPACE_END
