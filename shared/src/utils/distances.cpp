#include <utils/distances.hpp>

#include <algorithm>
#include <vector>

#include <fmt/format.h>

#include <userver/utils/impl/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace impl {

template <typename RandomIt>
std::size_t GetLevenshteinDistance(RandomIt begin1, RandomIt end1,
                                   RandomIt begin2, RandomIt end2) {
  const std::size_t line_size = end1 - begin1 + 1;
  std::vector<std::size_t> lines(2 * line_size);
  Span prev_line(lines.data(), lines.data() + line_size);
  Span cur_line(lines.data() + line_size, lines.data() + 2 * line_size);
  for (std::size_t i = 0; i < prev_line.size(); ++i) {
    prev_line[i] = i;
  }
  for (std::size_t line_index = 1;
       line_index <= static_cast<std::size_t>(end2 - begin2); ++line_index) {
    cur_line[0] = line_index;
    for (std::size_t col = 1; col < line_size; ++col) {
      cur_line[col] = std::min(cur_line[col - 1] + 1, prev_line[col] + 1);
      cur_line[col] = std::min(
          cur_line[col], prev_line[col - 1] + (*(begin1 + col - 1) !=
                                               *(begin2 + line_index - 1)));
    }
    std::swap(prev_line, cur_line);
  }
  return prev_line[line_size - 1];
}

template <typename RandomIt>
std::size_t GetDamerauLevenshteinDistance(RandomIt begin1, RandomIt end1,
                                          RandomIt begin2, RandomIt end2) {
  const std::size_t line_size = end1 - begin1 + 1;
  std::vector<std::size_t> lines(3 * line_size);

  Span first_prev_line(lines.data(), lines.data() + line_size);
  Span second_prev_line(lines.data() + line_size, lines.data() + 2 * line_size);
  Span cur_line(lines.data() + 2 * line_size, lines.data() + 3 * line_size);
  for (std::size_t i = 0; i < line_size; ++i) {
    first_prev_line[i] = i;
  }
  for (std::size_t line_index = 1;
       line_index <= static_cast<std::size_t>(end2 - begin2); ++line_index) {
    cur_line[0] = line_index;
    for (std::size_t col = 1; col < line_size; ++col) {
      cur_line[col] = std::min(cur_line[col - 1] + 1, first_prev_line[col] + 1);
      cur_line[col] = std::min(cur_line[col], first_prev_line[col - 1] +
                                                  (*(begin1 + col - 1) !=
                                                   *(begin2 + line_index - 1)));
      if (line_index >= 2 && col >= 2 &&
          *(begin1 + col - 1) == *(begin2 + line_index - 2) &&
          *(begin1 + col - 2) == *(begin2 + line_index - 1)) {
        cur_line[col] = std::min(cur_line[col], second_prev_line[col - 2] + 1);
      }
    }
    std::swap(first_prev_line, second_prev_line);
    std::swap(cur_line, first_prev_line);
  }
  return first_prev_line[line_size - 1];
}

}  // namespace impl

std::size_t GetLevenshteinDistance(std::string_view view1,
                                   std::string_view view2) {
  return impl::GetLevenshteinDistance(view1.begin(), view1.end(), view2.begin(),
                                      view2.end());
}

std::size_t GetDamerauLevenshteinDistance(std::string_view view1,
                                          std::string_view view2) {
  return impl::GetDamerauLevenshteinDistance(view1.begin(), view1.end(),
                                             view2.begin(), view2.end());
}

std::string SuggestNameErrorMsg(std::optional<std::string_view> suggest_name) {
  if (suggest_name.has_value()) {
    return fmt::format(" Perhaps you meant '{}' ?", suggest_name.value());
  }
  return "";
}

}  // namespace utils

USERVER_NAMESPACE_END
