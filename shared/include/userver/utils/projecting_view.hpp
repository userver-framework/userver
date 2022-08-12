#pragma once

/// @file userver/utils/projecting_view.hpp
/// @brief @copybrief utils::ProjectingView

#include <iterator>
#include <type_traits>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace impl {

struct first {
  template <class T>
  auto& operator()(T& value) const noexcept {
    return value.first;
  }
};

struct second {
  template <class T>
  auto& operator()(T& value) const noexcept {
    return value.second;
  }
};

template <class BaseIterator, class Projection>
class ProjectingIterator final {
  using BaseDifferenceType =
      typename std::iterator_traits<BaseIterator>::difference_type;
  using BaseReferenceType =
      typename std::iterator_traits<BaseIterator>::reference;
  using BaseIteratorCategory =
      typename std::iterator_traits<BaseIterator>::iterator_category;

 public:
  using difference_type = BaseDifferenceType;
  using reference = std::invoke_result_t<Projection&, BaseReferenceType>;
  using value_type = std::remove_reference_t<reference>;
  using iterator_category = std::conditional_t<
      std::is_reference_v<reference> &&
          !std::is_same_v<BaseIteratorCategory, std::input_iterator_tag>,
      std::forward_iterator_tag, std::input_iterator_tag>;

  ProjectingIterator() = default;

  ProjectingIterator(const BaseIterator& it, Projection& proj)
      : it_(it), proj_(&proj) {}

  ProjectingIterator(const ProjectingIterator&) = default;
  ProjectingIterator(ProjectingIterator&&) noexcept = default;
  ProjectingIterator& operator=(const ProjectingIterator&) = default;
  ProjectingIterator& operator=(ProjectingIterator&&) noexcept = default;

  bool operator==(const ProjectingIterator& other) const {
    return it_ == other.it_;
  }
  bool operator!=(const ProjectingIterator& other) const {
    return it_ != other.it_;
  }

  ProjectingIterator& operator++() {
    ++it_;
    return *this;
  }

  ProjectingIterator operator++(int) {
    return ProjectingIterator{it_++, proj_};
  }

  decltype(auto) operator*() const { return (*proj_)(*it_); }
  decltype(auto) operator*() { return (*proj_)(*it_); }

 private:
  BaseIterator it_{};
  Projection* proj_{nullptr};
};

}  // namespace impl

/// @ingroup userver_containers
///
/// @brief View that applies projection on iterator dereference.
///
/// Example:
/// @code
/// std::map<int, char> cont{
///     {1, '1'},
///     {2, '2'},
/// };
/// auto proj = utils::ProjectingView(
///     cont, [](auto& v) -> char& { return v.second; });
/// assert(*proj.begin() == 1);
/// assert(*++proj.begin() == 2);
///
/// *proj.begin() = 'Y';
/// assert(*proj.begin() == 'Y');
/// assert(cont[1] == 'Y');
/// @endcode
template <class Container, class Projection>
class ProjectingView : private Projection {
  using BaseIterator = decltype(std::begin(std::declval<Container&>()));

 public:
  using iterator = impl::ProjectingIterator<BaseIterator, Projection>;
  using const_iterator =
      impl::ProjectingIterator<BaseIterator, const Projection>;

  ProjectingView(Container& container, const Projection& proj)
      : Projection(proj), container_(container) {}

  ProjectingView(Container& container, Projection&& proj)
      : Projection(std::move(proj)), container_(container) {}

  const_iterator begin() const { return {std::begin(container_), *this}; }
  const_iterator end() const { return {std::end(container_), *this}; }
  const_iterator cbegin() const { return begin(); }
  const_iterator cend() const { return end(); }

  iterator begin() { return {std::begin(container_), *this}; }
  iterator end() { return {std::end(container_), *this}; }

  auto size() const { return std::size(container_); }

 private:
  Container& container_;
};

/// Helper function that returns a view of references to keys, close to the
/// std::ranges::views::keys functionality.
///
/// Example:
/// @code
/// std::map<int, char> cont{
///     {1, '1'},
///     {2, '2'},
/// };
/// auto proj = utils::MakeKeysView(cont);
/// assert(*proj.begin() == 1);
/// assert(*++proj.begin() == 2);
/// @endcode
template <class Container>
auto MakeKeysView(const Container& c) {
  return ProjectingView{c, impl::first{}};
}

/// Helper function that returns a view of references to values, close to the
/// std::ranges::views::values functionality.
///
/// Example:
/// @code
/// std::map<int, char> cont{
///     {1, '1'},
///     {2, '2'},
/// };
/// auto proj = utils::MakeValuesView(cont);
/// assert(*proj.begin() == '1');
/// assert(*++proj.begin() == '2');
///
/// *proj.begin() = 'Y';
/// assert(*proj.begin() == 'Y');
/// assert(cont[1] == 'Y');
/// @endcode
template <class Container>
auto MakeValuesView(Container& c) {
  return ProjectingView{c, impl::second{}};
}

}  // namespace utils

USERVER_NAMESPACE_END
