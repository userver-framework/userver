#pragma once

/// @file userver/utils/small_string.hpp
/// @brief @copybrief utils::SmallString

#include <cstddef>
#include <functional>
#include <stdexcept>
#include <string>

#include <boost/container/small_vector.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// @ingroup userver_universal userver_containers
///
/// @brief An alternative to std::string with a custom SSO (small string
/// optimization) container size. Unlike std::string, SmallString is not
/// null-terminated thus it has no c_str(), data() returns a not null-terminated
/// buffer.
template <std::size_t N>
class SmallString final {
  using Container = boost::container::small_vector<char, N>;

 public:
  /// @brief Create empty string.
  SmallString() = default;

  /// @brief Create a string from another one.
  SmallString(const SmallString<N>&) = default;

  /// @brief Create a string from another one.
  explicit SmallString(SmallString<N>&&) noexcept = default;

  /// @brief Create a string from std::string_view.
  explicit SmallString(std::string_view sv);

  /// @brief Assign the value of other string_view to this string.
  SmallString& operator=(std::string_view sv);

  /// @brief Assign the value of other string to this string.
  SmallString& operator=(const SmallString&) = default;

  /// @brief Assign the value of other string to this string.
  SmallString& operator=(SmallString&&) noexcept = default;

  /// @brief Convert string to a std::string_view.
  operator std::string_view() const;

  /// @brief Read-only subscript access to the data contained in the string.
  const char& operator[](std::size_t pos) const;

  /// @brief Subscript access to the data contained in the string.
  char& operator[](std::size_t pos);

  /// @brief Provides access to the data contained in the string.
  const char& at(std::size_t pos) const;

  /// @brief Provides access to the data contained in the string.
  char& at(std::size_t pos);

  using iterator = typename Container::iterator;

  using const_iterator = typename Container::const_iterator;

  iterator begin() noexcept;
  const_iterator begin() const noexcept;

  iterator end() noexcept;
  const_iterator end() const noexcept;

  /// @brief Get string size.
  std::size_t size() const noexcept;

  /// @brief Get pointer to data.
  /// @warning The buffer is not null-terminated.
  const char* data() const noexcept;

  /// @brief Get pointer to data.
  /// @warning The buffer is not null-terminated.
  char* data() noexcept;

  /// @brief Resize the string. If its length is increased,
  /// fill new chars with %c.
  void resize(std::size_t n, char c);

  /// @brief Resize the string. Use op to write into the string and replace a
  /// sequence of characters
  template <class Operation>
  void resize_and_overwrite(std::size_t size, Operation op);

  /// @brief Shrink the string's size to fit all contents
  void shrink_to_fit();

  /// @brief Get current capacity.
  std::size_t capacity() const noexcept;

  /// @brief Reserve to %n bytes.
  void reserve(std::size_t n);

  /// @brief Clear the string.
  void clear() noexcept;

  /// @brief Is the string empty?
  bool empty() const noexcept;

  /// @brief Get a reference to the first character.
  char& front();

  /// @brief Get a reference to the first character.
  const char& front() const;

  /// @brief Get a reference to the last character.
  char& back();

  /// @brief Get a reference to the last character.
  const char& back() const;

  /// @brief Append a character to the string.
  void push_back(char c);

  /// @brief Append contents of a string_view to the string.
  void append(std::string_view str);

  /// @brief Remove the last character from the string.
  void pop_back();

 private:
  boost::container::small_vector<char, N> data_;
};

template <std::size_t N>
SmallString<N>::SmallString(std::string_view sv)
    : data_(sv.begin(), sv.end()) {}

template <std::size_t N>
SmallString<N>::operator std::string_view() const {
  return std::string_view{data_.data(), data_.size()};
}

template <std::size_t N>
SmallString<N>& SmallString<N>::operator=(std::string_view sv) {
  data_ = {sv.begin(), sv.end()};
  return *this;
}

template <std::size_t N>
bool operator==(const SmallString<N>& str, std::string_view sv) {
  return std::string_view{str} == sv;
}

template <std::size_t N>
bool operator==(std::string_view sv, const SmallString<N>& str) {
  return std::string_view{str} == sv;
}

template <std::size_t N>
bool operator==(const SmallString<N>& str1, const SmallString<N>& str2) {
  return std::string_view{str1} == std::string_view{str2};
}

template <std::size_t N>
bool operator!=(const SmallString<N>& str1, const SmallString<N>& str2) {
  return !(str1 == str2);
}

template <std::size_t N>
const char& SmallString<N>::operator[](std::size_t pos) const {
  return data_[pos];
}

template <std::size_t N>
char& SmallString<N>::operator[](std::size_t pos) {
  return data_[pos];
}

template <std::size_t N>
const char& SmallString<N>::at(std::size_t pos) const {
  if (size() <= pos) throw std::out_of_range("at");
  return data_[pos];
}

template <std::size_t N>
char& SmallString<N>::at(std::size_t pos) {
  if (size() <= pos) throw std::out_of_range("at");
  return data_[pos];
}

template <std::size_t N>
typename SmallString<N>::iterator SmallString<N>::begin() noexcept {
  return {data_.begin()};
}

template <std::size_t N>
typename SmallString<N>::const_iterator SmallString<N>::begin() const noexcept {
  return {data_.begin()};
}

template <std::size_t N>
typename SmallString<N>::iterator SmallString<N>::end() noexcept {
  return {data_.end()};
}

template <std::size_t N>
typename SmallString<N>::const_iterator SmallString<N>::end() const noexcept {
  return {data_.end()};
}

template <std::size_t N>
std::size_t SmallString<N>::size() const noexcept {
  return data_.size();
}

template <std::size_t N>
const char* SmallString<N>::data() const noexcept {
  return data_.data();
}

template <std::size_t N>
char* SmallString<N>::data() noexcept {
  return data_.data();
}

template <std::size_t N>
bool SmallString<N>::empty() const noexcept {
  return data_.empty();
}

template <std::size_t N>
char& SmallString<N>::front() {
  return data_.front();
}

template <std::size_t N>
const char& SmallString<N>::front() const {
  return data_.front();
}

template <std::size_t N>
char& SmallString<N>::back() {
  return data_.back();
}

template <std::size_t N>
const char& SmallString<N>::back() const {
  return data_.back();
}

template <std::size_t N>
void SmallString<N>::push_back(char c) {
  data_.push_back(c);
}

template <std::size_t N>
void SmallString<N>::append(std::string_view str) {
  std::size_t old_size = data_.size();
  data_.insert(data_.begin() + old_size, str.begin(), str.end());
}

template <std::size_t N>
void SmallString<N>::pop_back() {
  data_.pop_back();
}

template <std::size_t N>
void SmallString<N>::resize(std::size_t n, char c) {
  data_.resize(n, c);
}

template <std::size_t N>
template <class Operation>
void SmallString<N>::resize_and_overwrite(std::size_t size, Operation op) {
  data_.resize(size, boost::container::default_init);
  data_.resize(std::move(op)(data_.data(), size),
               boost::container::default_init);
  UASSERT(data_.size() <= size);
}

template <std::size_t N>
void SmallString<N>::shrink_to_fit() {
  data_.shrink_to_fit();
}

template <std::size_t N>
std::size_t SmallString<N>::capacity() const noexcept {
  return data_.capacity();
}

template <std::size_t N>
void SmallString<N>::reserve(std::size_t n) {
  return data_.reserve(n);
}

template <std::size_t N>
void SmallString<N>::clear() noexcept {
  data_.clear();
}

}  // namespace utils

USERVER_NAMESPACE_END

template <std::size_t N>
struct std::hash<USERVER_NAMESPACE::utils::SmallString<N>> {
  std::size_t operator()(
      const USERVER_NAMESPACE::utils::SmallString<N>& s) const noexcept {
    return std::hash<std::string_view>{}(std::string_view{s});
  }
};
