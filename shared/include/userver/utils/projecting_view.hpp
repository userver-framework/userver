#pragma once

/// @file utils/projecting_view.hpp
/// @brief @copybrief utils::ProjectingView

#include <iterator>
#include <type_traits>

namespace utils {

namespace impl {

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

}  // namespace impl

/// @brief Iterator that applies projection on each dereference.
///
/// Consider using utils::MakeKeysView, utils::MakeValuesView or
/// utils::ProjectingView
template <class BaseIterator, class Projection>
class ProjectingIterator : Projection {
 public:
  using iterator_category = typename std::forward_iterator_tag;
  using value_type = typename BaseIterator::value_type;
  using difference_type = typename BaseIterator::difference_type;

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
class ProjectingView : Projection {
 public:
  using BaseConstIterator = typename Container::const_iterator;
  using BaseIterator =
      std::conditional_t<std::is_const<Container>::value, BaseConstIterator,
                         typename Container::iterator>;

  /// Stores a reference to the container
  explicit ProjectingView(Container& container) noexcept
      : container_(container) {}

  /// Stores a reference to the container and move initializes Projection
  ProjectingView(Container& container, Projection proj) noexcept
      : Projection(std::move(proj)), container_(container) {}

  /// @returns utils::ProjectingIterator to the cbegin of referenced container
  auto cbegin() const {
    return ProjectingIterator<BaseConstIterator, Projection>{
        container_.cbegin(), static_cast<const Projection&>(*this)};
  }

  /// @returns utils::ProjectingIterator to the cend of referenced container
  auto cend() const {
    return ProjectingIterator<BaseConstIterator, Projection>{
        container_.cend(), static_cast<const Projection&>(*this)};
  }

  /// @returns utils::ProjectingIterator to the begin of referenced container
  auto begin() {
    return ProjectingIterator<BaseIterator, Projection>{
        container_.begin(), static_cast<const Projection&>(*this)};
  }

  /// @returns utils::ProjectingIterator to the end of referenced container
  auto end() {
    return ProjectingIterator<BaseIterator, Projection>{
        container_.end(), static_cast<const Projection&>(*this)};
  }

  /// @returns utils::ProjectingIterator to the cbegin of referenced container
  auto begin() const { return cbegin(); }

  /// @returns utils::ProjectingIterator to the cend of referenced container
  auto end() const { return cend(); }

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
ProjectingView<const Container, impl::first> MakeKeysView(const Container& c) {
  return ProjectingView<const Container, impl::first>{c};
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
ProjectingView<Container, impl::second> MakeValuesView(Container& c) {
  return ProjectingView<Container, impl::second>{c};
}

}  // namespace utils
