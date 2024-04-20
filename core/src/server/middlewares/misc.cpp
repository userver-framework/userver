#include <server/middlewares/misc.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::middlewares::misc {

std::string_view CutTrailingSlash(
    const std::string& meta_type,
    handlers::UrlTrailingSlashOption trailing_slash) {
  std::string_view meta_type_view{meta_type};
  if (trailing_slash == handlers::UrlTrailingSlashOption::kBoth &&
      meta_type_view.size() > 1 && meta_type_view.back() == '/') {
    meta_type_view.remove_suffix(1);
  }

  return meta_type_view;
}

}  // namespace server::middlewares::misc

USERVER_NAMESPACE_END
