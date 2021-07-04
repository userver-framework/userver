#include <userver/utils/graphite.hpp>

#include <cstring>

/* Based on
 * https://wiki.yandex-team.ru/solomon/mega-graphite/metrics/#pravilavalidaciiimen
 */

namespace utils::graphite {

namespace {

/* ^[-_0-9a-zA-Z.:=/\[\]()"?]+$ */
bool IsPrintable(char c) {
  if (strchr("-_:=/[]()\"?", c)) return true;
  if ('a' <= c && c <= 'z') return true;
  if ('A' <= c && c <= 'Z') return true;
  if (isdigit(c)) return true;

  return false;
}

}  // namespace

std::string EscapeName(const std::string& s) {
  std::string result;
  result.reserve(s.size());
  for (auto c : s) {
    if (IsPrintable(c))
      result.push_back(c);
    else
      result.push_back('_');
  }
  return result;
}

}  // namespace utils::graphite
