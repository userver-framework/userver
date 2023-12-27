#include <userver/utils/text.hpp>

#include <algorithm>
#include <unordered_map>

#include <boost/algorithm/string.hpp>
#include <boost/locale.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include <userver/engine/shared_mutex.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::text {

namespace {

const std::string kLocaleArabic = "ar";
const std::string kLocaleArabicNumbersLatn = "ar@numbers=latn";

}  // namespace

std::string Format(double value, const std::string& locale, int ndigits,
                   bool is_fixed) {
  std::stringstream res;

  // localization of arabic numerals is broken, fallback to latin (0-9)
  if (locale == kLocaleArabic) {
    // see: https://sites.google.com/site/icuprojectuserguide/locale
    res.imbue(GetLocale(kLocaleArabicNumbersLatn));
  } else {
    res.imbue(GetLocale(locale));
  }

  if (is_fixed) res.setf(std::ios::fixed, std::ios::floatfield);
  res.precision(ndigits);

  res << boost::locale::as::number << value;
  return res.str();
}

std::string ToLower(std::string_view str, const std::string& locale) {
  return boost::locale::to_lower(str.data(), str.data() + str.size(),
                                 GetLocale(locale));
}

std::string ToUpper(std::string_view str, const std::string& locale) {
  return boost::locale::to_upper(str.data(), str.data() + str.size(),
                                 GetLocale(locale));
}

std::string Capitalize(std::string_view str, const std::string& locale) {
  return boost::locale::to_title(str.data(), str.data() + str.size(),
                                 GetLocale(locale));
}

const std::locale& GetLocale(const std::string& name) {
  static engine::SharedMutex m;
  using locales_map_t = std::unordered_map<std::string, std::locale>;
  static locales_map_t locales;
  {
    std::shared_lock read_lock(m);
    auto it = static_cast<const locales_map_t&>(locales).find(name);
    if (it != locales.cend()) {
      return it->second;
    }
  }

  boost::locale::generator gen;
  std::locale loc = gen(name);
  {
    std::unique_lock write_lock(m);
    return locales.emplace(name, std::move(loc)).first->second;
  }
}

}  // namespace utils::text

USERVER_NAMESPACE_END
