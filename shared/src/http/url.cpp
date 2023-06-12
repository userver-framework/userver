#include <userver/http/url.hpp>

#include <array>

USERVER_NAMESPACE_BEGIN

namespace http {

namespace {

const std::string_view kSchemaSeparator = "://";

void UrlEncodeTo(std::string_view input_string, std::string& result) {
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
        std::array<char, 3> bytes = {'%', 0, 0};
        bytes[1] = (symbol & 0xF0) / 16;
        bytes[1] += (bytes[1] > 9) ? 'A' - 10 : '0';
        bytes[2] = symbol & 0x0F;
        bytes[2] += (bytes[2] > 9) ? 'A' - 10 : '0';
        result.append(bytes.data(), bytes.size());
        break;
    }
  }
}

}  // namespace

std::string UrlEncode(std::string_view input_string) {
  std::string result;
  result.reserve(3 * input_string.size());

  UrlEncodeTo(input_string, result);
  return result;
}

std::string UrlDecode(std::string_view range) {
  std::string result;
  result.reserve(range.size() / 3);

  for (const char *i = range.begin(), *end = range.end(); i != end; ++i) {
    switch (*i) {
      case '+':
        result.append(1, ' ');
        break;
      case '%':
        if (std::distance(i, end) > 2) {
          char f = *(i + 1);
          char s = *(i + 2);
          int digit = (f >= 'A' ? ((f & 0xDF) - 'A') + 10 : (f - '0')) * 16;
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
std::size_t GetInitialQueryCapacity(T begin, T end) {
  std::size_t capacity = 1;
  for (auto it = begin; it != end; ++it) {
    // Maximal query result size is 3 * input.size. Coefficient 3 / 2 guarantee
    // no more than one reallocation.
    capacity += 1 + (it->first.size() + it->second.size()) * 3 / 2 + 1;
  }
  return capacity;
}

template <typename T>
void DoMakeQueryTo(T begin, T end, std::string& result) {
  bool first = true;
  for (auto it = begin; it != end; ++it) {
    if (!first) {
      result.append(1, '&');
    } else {
      first = false;
    }
    UrlEncodeTo(it->first, result);
    result.append(1, '=');
    UrlEncodeTo(it->second, result);
  }
}

template <typename T>
std::string DoMakeQuery(T begin, T end) {
  std::string result;
  result.reserve(GetInitialQueryCapacity(begin, end));

  DoMakeQueryTo(begin, end, result);
  return result;
}

template <typename T>
std::string MakeUrl(std::string_view path, T begin, T end) {
  std::string result;
  result.reserve(path.size() + 1 + GetInitialQueryCapacity(begin, end));

  result.append(path);
  result.append(1, '?');
  DoMakeQueryTo(begin, end, result);
  return result;
}

}  // namespace

std::string MakeUrl(std::string_view path, const Args& query_args) {
  return MakeUrl(path, query_args.begin(), query_args.end());
}

std::string MakeUrl(
    std::string_view path,
    const std::unordered_map<std::string, std::string>& query_args) {
  return MakeUrl(path, query_args.begin(), query_args.end());
}

std::string MakeUrl(std::string_view path, const Args& query_args,
                    MultiArgs query_multiargs) {
  for (const auto& [key, value] : query_args) {
    query_multiargs.insert({key, value});
  }
  return MakeUrl(path, query_multiargs.begin(), query_multiargs.end());
}

std::string MakeUrl(
    std::string_view path,
    std::initializer_list<std::pair<std::string_view, std::string_view>>
        query_args) {
  return MakeUrl(path, query_args.begin(), query_args.end());
}

std::string MakeQuery(const Args& query_args) {
  return DoMakeQuery(query_args.begin(), query_args.end());
}

std::string MakeQuery(
    const std::unordered_map<std::string, std::string>& query_args) {
  return DoMakeQuery(query_args.begin(), query_args.end());
}

std::string MakeQuery(
    std::initializer_list<std::pair<std::string_view, std::string_view>>
        query_args) {
  return DoMakeQuery(query_args.begin(), query_args.end());
}

std::string ExtractMetaTypeFromUrl(const std::string& url) {
  auto pos = url.find('?');
  if (pos == std::string::npos) return url;

  return url.substr(0, pos);
}

std::string ExtractPath(std::string_view url) {
  auto pos = url.find(kSchemaSeparator);
  auto tmp = (pos == std::string::npos)
                 ? url
                 : url.substr(pos + kSchemaSeparator.size());

  auto slash_pos = tmp.find('/');
  if (slash_pos == std::string::npos) return "";
  return std::string(tmp.substr(slash_pos));
}

std::string ExtractHostname(std::string_view url) {
  // Drop "schema://"
  auto pos = url.find(kSchemaSeparator);
  auto tmp = (pos == std::string::npos)
                 ? url
                 : url.substr(pos + kSchemaSeparator.size());

  // Drop /.*
  auto slash_pos = tmp.find('/');
  if (slash_pos != std::string::npos) {
    tmp = tmp.substr(0, slash_pos);
  }

  auto userinfo_pos = tmp.rfind('@');
  if (userinfo_pos != std::string::npos) {
    tmp = tmp.substr(userinfo_pos + 1);
  }

  auto bracket_close_pos = tmp.find(']');
  if (bracket_close_pos != std::string::npos) {
    // IPv6 address
    tmp = tmp.substr(0, bracket_close_pos + 1);
  } else {
    // DNS name or IPv4 address
    auto port_pos = tmp.find(':');
    if (port_pos != std::string::npos) {
      tmp = tmp.substr(0, port_pos);
    }
  }

  return std::string{tmp};
}

}  // namespace http

USERVER_NAMESPACE_END
