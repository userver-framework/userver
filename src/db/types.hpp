#pragma once

#include <string_view>

namespace real_medium::sql::types {

inline constexpr std::string_view kProfile{"real_medium.profile"};
inline constexpr std::string_view kUser{"real_medium.user"};

inline constexpr std::string_view kFullArticleInfo{"real_medium.full_article_info"};
inline constexpr std::string_view kTaggedArticleWithProfile{"real_medium.tagged_article_with_author_profile"};

}  // namespace real_medium::sql::types
