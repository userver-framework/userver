#include "gtest/gtest.h"
#include "slugify.hpp"

namespace real_medium::utils::slug {

TEST(SlugifyTest, TestEmpty) {
  EXPECT_EQ(Slugify(""), "");
}

TEST(SlugifyTest, TestEnglish1) {
  EXPECT_EQ(Slugify("abc"), "abc");
}

TEST(SlugifyTest, TestEnglish2) {
  EXPECT_EQ(Slugify("ab`c"), "abc");
}

TEST(SlugifyTest, TestEnglish3) {
  EXPECT_EQ(Slugify("ab%c"), "abc");
}

TEST(SlugifyTest, TestEnglish4) {
  EXPECT_EQ(Slugify("ab  c"), "ab-c");
}

TEST(SlugifyTest, TestEnglish5) {
  EXPECT_EQ(Slugify("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~:/?#[]@!$&'()*+,;=")
                , "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz0123456789");
}

TEST(SlugifyTest, TestRussian) {
  EXPECT_EQ(Slugify("Заголовок статьи"), "zagolovok-stat'i");
}

TEST(SlugifyTest, TestGerman) {
  EXPECT_EQ(Slugify("Artikelüberschrift"), "artikeluberschrift");
}

TEST(SlugifyTest, TestFrench) {
  EXPECT_EQ(Slugify("le titre de l'article"), "le-titre-de-larticle");
}

TEST(SlugifyTest, TestSpanish) {
  EXPECT_EQ(Slugify("título del artículo"), "titulo-del-articulo");
}

TEST(SlugifyTest, TestChineese) {
  EXPECT_EQ(Slugify("文章標題"), "wen-zhang-biao-ti");
}
}
