#include "slugify.hpp"
#include "errors.hpp"
#include "make_error.hpp"


namespace real_medium::utils::slug {

std::string Slugify(const std::string& str) {
  std::string slug = str;

  // Преобразование всех символов в нижний регистр
  std::transform(slug.begin(), slug.end(), slug.begin(), ::tolower);

  // Замена пробелов и специальных символов на дефисы
  for (char& c : slug)
    if (!std::isalnum(c))
      c = '-';
  // Удаление повторяющихся дефисов
  slug.erase(std::unique(slug.begin(), slug.end(), [](char a, char b) { return a == '-' && b == '-'; }), slug.end());

  // Удаление дефисов в начале и конце строки
  if (!slug.empty() && slug.front() == '-')
    slug.erase(slug.begin());

  if (!slug.empty() && slug.back() == '-')
    slug.pop_back();

  if(slug.empty())
    throw std::runtime_error("slufigy failed");

  return slug;
}

}  // namespace real_medium::slug
