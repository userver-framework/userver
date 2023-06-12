#pragma once

#include <iterator>
#include <type_traits>

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

struct first {
  template <class T>
  auto operator()(T& value) const noexcept -> decltype(value.first)& {
    return value.first;
  }
};

struct second {
  template <class T>
  auto operator()(T& value) const noexcept -> decltype(value.second)& {
    return value.second;
  }
};

template <class BaseIterator, class Projection>
class ProjectingIterator : Projection {
 public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = std::decay_t<std::invoke_result_t<
      Projection, typename std::iterator_traits<BaseIterator>::value_type&>>;
  using difference_type =
      typename std::iterator_traits<BaseIterator>::difference_type;
  using reference = value_type&;
  using pointer = value_type*;

  ProjectingIterator() = default;
  explicit ProjectingIterator(BaseIterator it) : it_(std::move(it)) {}
  ProjectingIterator(BaseIterator it, Projection proj)
      : Projection(std::move(proj)), it_(std::move(it)) {}
  ProjectingIterator(const ProjectingIterator&) = default;
  ProjectingIterator(ProjectingIterator&&) noexcept = default;
  ProjectingIterator& operator=(const ProjectingIterator&) = default;
  ProjectingIterator& operator=(ProjectingIterator&&) noexcept = default;

  bool operator==(ProjectingIterator other) const { return it_ == other.it_; }
  bool operator!=(ProjectingIterator other) const { return it_ != other.it_; }

  ProjectingIterator& operator++() {
    ++it_;
    return *this;
  }

  ProjectingIterator operator++(int) { return ProjectingIterator{it_++}; }

  decltype(auto) operator*() const { return Projection::operator()(*it_); }

 private:
  BaseIterator it_;
};

// A backport of std::ranges::transform_view.
template <class Container, class Projection>
class ProjectingView : private Projection {
 public:
  using BaseConstIterator = typename Container::const_iterator;
  using BaseIterator =
      std::conditional_t<std::is_const_v<Container>, BaseConstIterator,
                         typename Container::iterator>;

  using const_iterator = ProjectingIterator<BaseConstIterator, Projection>;
  using value_type = typename const_iterator::value_type;

  explicit ProjectingView(Container& container) noexcept
      : container_(container) {}

  ProjectingView(Container& container, Projection proj) noexcept
      : Projection(std::move(proj)), container_(container) {}

  auto cbegin() const {
    return ProjectingIterator<BaseConstIterator, Projection>{
        container_.cbegin(), static_cast<const Projection&>(*this)};
  }

  auto cend() const {
    return ProjectingIterator<BaseConstIterator, Projection>{
        container_.cend(), static_cast<const Projection&>(*this)};
  }

  auto begin() {
    return ProjectingIterator<BaseIterator, Projection>{
        container_.begin(), static_cast<const Projection&>(*this)};
  }

  auto end() {
    return ProjectingIterator<BaseIterator, Projection>{
        container_.end(), static_cast<const Projection&>(*this)};
  }

  auto begin() const { return cbegin(); }
  auto end() const { return cend(); }

 private:
  Container& container_;
};

// A backport of std::ranges::views::keys.
template <class Container>
ProjectingView<const Container, impl::first> MakeKeysView(const Container& c) {
  return ProjectingView<const Container, impl::first>{c};
}

// A backport of std::ranges::views::values.
template <class Container>
ProjectingView<Container, impl::second> MakeValuesView(Container& c) {
  return ProjectingView<Container, impl::second>{c};
}

}  // namespace utils::impl

USERVER_NAMESPACE_END
