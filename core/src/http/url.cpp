#include <http/url.hpp>

namespace http {

/* Encode/Decode is ported from FastCgi daemon with minor changes:
 * https://github.yandex-team.ru/InfraComponents/fastcgi-daemon2/blob/master/library/util.cpp
 */

std::string UrlEncode(utils::string_view input_string) {
  std::string result;
  result.reserve(3 * input_string.size());

  for (char symbol : input_string) {
    if (isalnum(symbol)) {
      result.append(1, symbol);
      continue;
    }
    switch (symbol) {
      case '-':
      case '_':
      case '.':
      case '!':
      case '~':
      case '*':
      case '(':
      case ')':
      case '\'':
        result.append(1, symbol);
        break;
      default:
        char bytes[4] = {'%', 0, 0, 0};
        bytes[1] = (symbol & 0xF0) / 16;
        bytes[1] += (bytes[1] > 9) ? 'A' - 10 : '0';
        bytes[2] = symbol & 0x0F;
        bytes[2] += (bytes[2] > 9) ? 'A' - 10 : '0';
        result.append(bytes, sizeof(bytes) - 1);
        break;
    }
  }
  return result;
}

std::string UrlDecode(utils::string_view range) {
  std::string result;
  for (const char *i = range.begin(), *end = range.end(); i != end; ++i) {
    switch (*i) {
      case '+':
        result.append(1, ' ');
        break;
      case '%':
        if (std::distance(i, end) > 2) {
          int digit;
          char f = *(i + 1), s = *(i + 2);
          digit = (f >= 'A' ? ((f & 0xDF) - 'A') + 10 : (f - '0')) * 16;
          digit += (s >= 'A') ? ((s & 0xDF) - 'A') + 10 : (s - '0');
          result.append(1, static_cast<char>(digit));
          i += 2;
        } else {
          result.append(1, '%');
        }
        break;
      default:
        result.append(1, (*i));
        break;
    }
  }

  return result;
}

namespace {

template <typename T>
std::string DoMakeQuery(T begin, T end) {
  std::string result;
  bool first = true;
  for (auto it = begin; it != end; ++it) {
    if (!first) result += '&';
    first = false;
    result += UrlEncode(it->first) + '=' + UrlEncode(it->second);
  }
  return result;
}

template <typename T>
std::string MakeUrl(utils::string_view path, T begin, T end) {
  return std::string(path) + '?' + DoMakeQuery(begin, end);
}

}  // namespace

std::string MakeUrl(utils::string_view path, const Args& query_args) {
  return MakeUrl(path, query_args.begin(), query_args.end());
}

std::string MakeUrl(
    utils::string_view path,
    std::initializer_list<std::pair<utils::string_view, utils::string_view>>
        query_args) {
  return MakeUrl(path, query_args.begin(), query_args.end());
}

std::string MakeQuery(const Args& query_args) {
  return DoMakeQuery(query_args.begin(), query_args.end());
}

std::string MakeQuery(
    std::initializer_list<std::pair<utils::string_view, utils::string_view>>
        query_args) {
  return DoMakeQuery(query_args.begin(), query_args.end());
}

}  // namespace http
