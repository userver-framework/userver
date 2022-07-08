#include "multipart_form_data_parser.hpp"

#include <boost/algorithm/string/predicate.hpp>

#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

#include <array>

USERVER_NAMESPACE_BEGIN

namespace server::http {

namespace {

const std::string kMultipartFormData = "multipart/form-data";

const char kCr = '\r';
const char kLf = '\n';

const std::string kOwsChars = " \t";

[[nodiscard]] std::string_view LtrimOws(std::string_view str) {
  const auto first_pchar_pos = str.find_first_not_of(kOwsChars);
  str.remove_prefix(
      first_pchar_pos == std::string_view::npos ? str.size() : first_pchar_pos);

  return str;
}

[[nodiscard]] std::string_view RtrimOws(std::string_view str) {
  const auto last_pchar_pos = str.find_last_not_of(kOwsChars);
  str.remove_suffix(str.size() - (last_pchar_pos == std::string_view::npos
                                      ? 0
                                      : last_pchar_pos + 1));

  return str;
}

[[nodiscard]] std::string_view TrimOws(std::string_view str) {
  return RtrimOws(LtrimOws(str));
}

[[nodiscard]] bool SkipSymbol(std::string_view& str, char ch) {
  if (str.empty() || str.front() != ch) {
    LOG_WARNING() << "Symbol #" << int{ch} << " (" << ch
                  << ") expected, but not found in request body";
    return false;
  }
  str.remove_prefix(1);
  return true;
}

[[nodiscard]] bool SkipCrLf(std::string_view& str, std::string_view crlf) {
  for (char c : crlf) {
    if (!SkipSymbol(str, c)) return false;
  }
  return true;
}

bool IEquals(std::string_view l, std::string_view r) {
  if (l.size() != r.size()) return false;
  for (size_t pos = 0; pos < l.size(); pos++) {
    if (tolower(l[pos]) != tolower(r[pos])) return false;
  }
  return true;
}

[[nodiscard]] bool SkipDoubleHyphen(std::string_view& str) {
  return SkipSymbol(str, '-') && !!SkipSymbol(str, '-');  // !!-for clang-tidy
}

bool IsWsp(char c) { return c == ' ' || c == '\t'; }

void SkipOptionalSpaces(std::string_view& str) {
  while (!str.empty() && IsWsp(str.front())) str.remove_prefix(1);
}

std::string_view ReadToken(std::string_view& str) {
  // `token` from https://tools.ietf.org/html/rfc7230#section-3.2.6
  static const std::array<bool, 256> kAllowed = []() {
    std::array<bool, 256> res{};
    res.fill(false);
    for (int i = 33; i < 127; i++) {
      res[i] = true;
    }
    for (char c : "()<>@,;:\\\"/[]?={}") {
      res[static_cast<uint8_t>(c)] = false;
    }
    return res;
  }();

  size_t pos = 0;
  while (pos < str.size() && kAllowed[static_cast<uint8_t>(str[pos])]) ++pos;
  std::string_view res = str.substr(0, pos);
  str.remove_prefix(pos);
  return res;
}

std::string_view ReadHeaderValue(std::string_view& str) {
  // `field-value` from https://tools.ietf.org/html/rfc7230#section-3.2
  // `obs-fold` is not supported now
  static const std::array<bool, 256> kAllowed = []() {
    std::array<bool, 256> res{};
    res.fill(false);
    res[static_cast<uint8_t>('\t')] = true;
    for (int i = 32; i < 256; i++) {
      res[i] = true;
    }
    res[127] = false;
    return res;
  }();

  size_t pos = 0;
  while (pos < str.size() && kAllowed[static_cast<uint8_t>(str[pos])]) ++pos;

  auto header_value = TrimOws(str.substr(0, pos));
  str.remove_prefix(pos);
  return header_value;
}

std::string_view AutoDetectCrLf(std::string_view str, std::string_view dflt) {
  // String `str` should start with a line break.
  // Detect if single '\r' or single '\n' is used instead of "\r\n" for line
  // breaks.
  if (!str.empty() && (str.front() == kCr || str.front() == kLf)) {
    if (str.front() == kLf || (str.size() <= 1 || str[1] != kLf)) {
      return str.substr(0, 1);
    }
  }
  return dflt;
}

struct FormDataArgInfo {
  std::string name;
  FormDataArg arg;
};

bool ParseHeaderParamValue(std::string_view& str, std::string* value) {
  if (value) *value = {};
  SkipOptionalSpaces(str);
  if (!str.empty() && str.front() == '"') {
    str.remove_prefix(1);
    while (!str.empty()) {
      if (str.front() == '"') {
        str.remove_prefix(1);
        return true;
      }
      if (str.front() == '\\') {
        str.remove_prefix(1);
        if (str.empty()) break;
      }
      char c = str.front();
      str.remove_prefix(1);
      if (value) *value += c;
    }
    LOG_WARNING() << "Can't parse header param value: closing '\"' not found";
    return false;
  } else {
    auto token = ReadToken(str);
    if (token.empty()) {
      LOG_WARNING() << "Can't parse header param value: token not found";
      return false;
    }
    if (value) *value = token;
    return true;
  }
}

bool ParseContentDisposition(std::string_view content_disposition,
                             FormDataArgInfo& arg_info) {
  static const std::string_view kFormData = "form-data";
  static const std::string_view kName = "name";
  static const std::string_view kFilename = "filename";

  auto str = content_disposition;
  auto value = ReadToken(str);
  if (!IEquals(value, kFormData)) {
    LOG_WARNING() << "Bad Content-Disposition: '" << kFormData << "' expected '"
                  << value << "' found";
    return false;
  }
  SkipOptionalSpaces(str);
  while (!str.empty()) {
    if (!SkipSymbol(str, ';')) return false;
    SkipOptionalSpaces(str);
    auto param_name = ReadToken(str);
    if (!param_name.empty()) {
      SkipOptionalSpaces(str);
      if (!SkipSymbol(str, '=')) return false;
      bool is_name = IEquals(param_name, kName);
      bool is_filename = IEquals(param_name, kFilename);
      bool need_header_value = is_name || is_filename;
      std::string value;
      if (!ParseHeaderParamValue(str, need_header_value ? &value : nullptr))
        return false;
      SkipOptionalSpaces(str);
      if (is_name) {
        arg_info.name = std::move(value);
      } else if (is_filename) {
        arg_info.arg.filename = std::move(value);
      }
    } else {
      LOG_WARNING() << "Bad Content-Disposition: empty attribute name";
      return false;
    }
  }

  if (arg_info.name.empty()) {
    LOG_WARNING() << "No name found in Content-Disposition";
    return false;
  }
  return true;
}

bool ProcessMultipartFormDataHeader(std::string_view name,
                                    std::string_view value,
                                    FormDataArgInfo& arg_info) {
  static const std::string kContentDisposition = "Content-Disposition";
  static const std::string kContentType = "Content-Type";

  LOG_TRACE() << "Process header: name=" << name << ", value=" << value;
  if (IEquals(name, kContentDisposition)) {
    arg_info.arg.content_disposition = value;
    if (!ParseContentDisposition(arg_info.arg.content_disposition, arg_info))
      return false;
  } else if (IEquals(name, kContentType)) {
    arg_info.arg.content_type = value;
  }
  // just ignore other headers

  return true;
}

bool ParseMultipartFormDataHeaders(std::string_view& body,
                                   FormDataArgInfo& arg_info,
                                   std::string_view crlf) {
  while (!body.empty()) {
    if (boost::starts_with(body, crlf)) break;

    auto header_name = ReadToken(body);
    if (header_name.empty()) {
      LOG_WARNING()
          << "Can't parse multipart header: header-name token not found";
      return false;
    }
    if (!SkipSymbol(body, ':')) return false;

    LOG_TRACE() << "header_name_parsed, unparsed body part=" << body;
    auto header_value = ReadHeaderValue(body);
    if (!SkipCrLf(body, crlf)) return false;

    if (!ProcessMultipartFormDataHeader(header_name, header_value, arg_info)) {
      LOG_WARNING() << "Can't parse multipart header name";
      return false;
    }
  }
  if (body.empty()) {
    LOG_WARNING() << "Unexpected request body end";
    return false;
  }
  return SkipCrLf(body, crlf);
}

size_t FindBoundaryEnd(std::string_view body, std::string_view boundary,
                       std::string_view crlf) {
  size_t pos = 0;
  while (pos < body.size()) {
    if (body[pos] == crlf.front()) {
      if (pos + crlf.size() < body.size() &&
          body.substr(pos, crlf.size()) == crlf) {
        pos += crlf.size();
        if (pos < body.size() && body[pos] == '-') {
          ++pos;
          if (pos < body.size() && body[pos] == '-') {
            ++pos;
            size_t idx = 0;
            while (idx < boundary.size() && pos + idx < body.size() &&
                   body[pos + idx] == boundary[idx])
              ++idx;
            pos += idx;
            if (idx == boundary.size()) return pos;
          }
        }
      } else
        ++pos;
    } else
      ++pos;
  }
  return std::string_view::npos;
}

bool ParseMultipartFormDataValue(std::string_view& body,
                                 std::string_view boundary,
                                 FormDataArgInfo&& arg_info,
                                 std::optional<std::string>& charset,
                                 FormDataArgs& form_data_args,
                                 std::string_view crlf) {
  static const std::string kCharset = "_charset_";

  if (arg_info.arg.content_disposition.empty()) {
    LOG_WARNING() << "Missing Content-Disposition header";
    return false;
  }

  size_t pos = FindBoundaryEnd(body, boundary, crlf);
  if (pos == std::string_view::npos) {
    LOG_WARNING() << "Unexpected end of form-data part value";
    return false;
  }
  arg_info.arg.value = body.substr(0, pos - boundary.size() - 2 - crlf.size());
  if (arg_info.name == kCharset) {
    charset = arg_info.arg.value;
  } else {
    LOG_TRACE() << "add arg_info.name=" << arg_info.name
                << ", arg_info.arg=" << arg_info.arg.ToDebugString();
    form_data_args[arg_info.name].push_back(std::move(arg_info.arg));
  }
  body.remove_prefix(pos);
  SkipOptionalSpaces(body);
  return true;
}

bool ParseMultipartFormDataBody(std::string_view body,
                                std::string_view boundary,
                                std::string default_charset,
                                FormDataArgs& form_data_args,
                                bool strict_cr_lf) {
  LOG_TRACE() << "body=" << body << ", body.size()=" << body.size();
  std::string_view crlf = "\r\n";
  if (boundary.size() + 2 <= body.size() && body[0] == '-' && body[1] == '-' &&
      body.substr(2, boundary.size()) == boundary) {
    body.remove_prefix(2 + boundary.size());
    if (!strict_cr_lf) crlf = AutoDetectCrLf(body, crlf);
  } else {
    while (!body.empty() && body.front() != kCr && body.front() != kLf)
      body.remove_prefix(1);
    if (!strict_cr_lf) crlf = AutoDetectCrLf(body, crlf);
    size_t pos = FindBoundaryEnd(body, boundary, crlf);
    if (pos == std::string_view::npos) {
      LOG_WARNING() << "Unexpected request body end";
      return false;
    }
    body.remove_prefix(pos);
  }
  SkipOptionalSpaces(body);

  std::optional<std::string> charset;
  if (!default_charset.empty()) charset = std::move(default_charset);

  LOG_TRACE() << "crlf=" << crlf;

  while (!body.empty()) {
    if (body.front() == '-') {
      if (!SkipDoubleHyphen(body)) return false;
      // https://datatracker.ietf.org/doc/html/rfc2046#section-5.1.1
      SkipOptionalSpaces(body);
      if (!body.empty()) {
        if (!SkipCrLf(body, crlf)) return false;
      }
      if (!body.empty()) {
        LOG_INFO() << "Extra (" << body.size()
                   << ") characters in request body";
      }
      for (auto& [name, args] : form_data_args) {
        UASSERT(!args.empty());
        for (auto& arg : args) {
          if (charset) {
            arg.default_charset = charset;
          }
          LOG_TRACE() << "form_data arg, name: " << name
                      << ", arg: " << arg.ToDebugString();
        }
      }
      return true;
    }
    if (!SkipCrLf(body, crlf)) return false;

    FormDataArgInfo arg_info;

    if (!ParseMultipartFormDataHeaders(body, arg_info, crlf)) return false;
    LOG_TRACE() << "ParseMultipartFormDataHeaders finished, body=" << body
                << ", body.size()=" << body.size();
    if (!ParseMultipartFormDataValue(body, boundary, std::move(arg_info),
                                     charset, form_data_args, crlf)) {
      return false;
    }
  }
  LOG_WARNING() << "Unexpected request body end";
  return false;
}

}  // namespace

bool IsMultipartFormDataContentType(std::string_view content_type) {
  if (!IEquals(content_type.substr(0, kMultipartFormData.size()),
               kMultipartFormData))
    return false;
  if (content_type.size() == kMultipartFormData.size()) return true;
  switch (content_type[kMultipartFormData.size()]) {
    case ';':
    case ' ':
    case '\t':
      return true;
  }
  return false;
}

bool ParseMultipartFormData(const std::string& content_type,
                            std::string_view body, FormDataArgs& form_data_args,
                            bool strict_cr_lf) {
  static const std::string kBoundary = "boundary";
  static const std::string kCharset = "charset";
  static const std::string kBoundaryNotFound =
      "'boundary' parameter of multipart/form-data not found";

  if (!IsMultipartFormDataContentType(content_type)) {
    LOG_WARNING() << "Content type is not 'multipart/form-data'";
    return false;
  }
  std::string_view unparsed = content_type;
  unparsed.remove_prefix(kMultipartFormData.size());
  SkipOptionalSpaces(unparsed);

  std::string boundary;
  std::string charset;
  while (!unparsed.empty()) {
    if (!SkipSymbol(unparsed, ';')) return false;
    SkipOptionalSpaces(unparsed);
    auto param_name = ReadToken(unparsed);
    if (!param_name.empty()) {
      SkipOptionalSpaces(unparsed);
      if (!SkipSymbol(unparsed, '=')) return false;
      bool is_boundary = IEquals(param_name, kBoundary);
      bool is_charset = IEquals(param_name, kCharset);
      bool need_header_value = is_boundary || is_charset;
      std::string value;
      if (!ParseHeaderParamValue(unparsed,
                                 need_header_value ? &value : nullptr))
        return false;
      SkipOptionalSpaces(unparsed);
      if (is_boundary) {
        boundary = std::move(value);
      } else if (is_charset) {
        charset = std::move(value);
      }
      // else ignore other parameters
    } else {
      LOG_WARNING() << "Bad Content-Type: empty attribute name";
      return false;
    }
  }

  if (boundary.empty()) {
    LOG_WARNING() << kBoundaryNotFound;
    return false;
  }

  return ParseMultipartFormDataBody(body, boundary, std::move(charset),
                                    form_data_args, strict_cr_lf);
}

}  // namespace server::http

USERVER_NAMESPACE_END
