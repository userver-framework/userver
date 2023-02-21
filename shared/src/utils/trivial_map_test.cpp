#include <userver/utils/trivial_map.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

/// [sample string bimap]
constexpr utils::TrivialBiMap kToInt = [](auto selector) {
  return selector()
      .Case("zero", 0)
      .Case("one", 1)
      .Case("two", 2)
      .Case("three", 3)
      .Case("four", 4);
};

TEST(TrivialBiMap, StringBasic) {
  EXPECT_FALSE(kToInt.TryFind(42));

  EXPECT_EQ(kToInt.TryFind("one").value(), 1);
  EXPECT_EQ(kToInt.TryFind(2).value(), "two");

  EXPECT_EQ(kToInt.TryFind("ten").value_or(-1), -1);
}
/// [sample string bimap]

TEST(TrivialBiMap, StringBasicDescribe) {
  EXPECT_EQ(kToInt.Describe(),
            "('zero', '0'), ('one', '1'), ('two', '2'), ('three', '3'), "
            "('four', '4')");
  EXPECT_EQ(kToInt.DescribeFirst(), "'zero', 'one', 'two', 'three', 'four'");
}

/// [sample bidir bimap]
enum class Colors { kRed, kOrange, kYellow, kGreen, kBlue, kViolet };
enum ThirdPartyColor { kGreen, kBlue, kViolet, kRed, kOrange, kYellow };

constexpr utils::TrivialBiMap kColorSwitch = [](auto selector) {
  return selector()
      .Case(ThirdPartyColor::kRed, Colors::kRed)
      .Case(ThirdPartyColor::kOrange, Colors::kOrange)
      .Case(ThirdPartyColor::kYellow, Colors::kYellow)
      .Case(ThirdPartyColor::kGreen, Colors::kGreen)
      .Case(ThirdPartyColor::kBlue, Colors::kBlue)
      .Case(ThirdPartyColor::kViolet, Colors::kViolet);
};

TEST(TrivialBiMap, EnumToEnum) {
  EXPECT_EQ(kColorSwitch.TryFind(ThirdPartyColor::kRed).value(), Colors::kRed);
  EXPECT_EQ(kColorSwitch.TryFind(ThirdPartyColor::kBlue).value(),
            Colors::kBlue);

  EXPECT_EQ(kColorSwitch.TryFind(Colors::kGreen).value(),
            ThirdPartyColor::kGreen);
  EXPECT_EQ(kColorSwitch.TryFind(Colors::kOrange).value(),
            ThirdPartyColor::kOrange);
}
/// [sample bidir bimap]

/// [sample contains switch]
constexpr utils::TrivialSet kKnownLanguages = [](auto selector) {
  return selector()
      .Case("c++")
      .Case("python")
      .Case("javascript")
      .Case("kotlin");
};

TEST(TrivialBiMap, Contains) {
  EXPECT_TRUE(kKnownLanguages.ContainsICase("C++"));
  EXPECT_TRUE(kKnownLanguages.ContainsICase("Javascript"));
  EXPECT_TRUE(kKnownLanguages.Contains("kotlin"));
  EXPECT_FALSE(kKnownLanguages.ContainsICase("HTML"));

  EXPECT_EQ(kKnownLanguages.Describe(),
            "'c++', 'python', 'javascript', 'kotlin'");
}
/// [sample contains switch]

TEST(TrivialBiMap, StaticConstexprLocalType) {
  struct IntsPair {
    int x;
    int y;
  };
  static constexpr utils::TrivialBiMap kKnownTwos = [](auto selector) {
    return selector()
        .Case(10, IntsPair{1, 0})
        .Case(11, IntsPair{1, 1})
        .Case(20, IntsPair{2, 0})
        .Case(21, IntsPair{2, 1})
        .Case(22, IntsPair{1, 1});
  };

  EXPECT_EQ(kKnownTwos.TryFind(10)->x, 1);
  EXPECT_EQ(kKnownTwos.TryFind(10)->y, 0);

  EXPECT_EQ(kKnownTwos.TryFind(21)->x, 2);
  EXPECT_EQ(kKnownTwos.TryFind(21)->y, 1);
}

TEST(TrivialBiMap, StaticConstexprContainsLocalType) {
  struct IntsPair {
    int x;
    int y;
    bool operator==(IntsPair other) const {
      return x == other.x && y == other.y;
    }
  };
  static constexpr utils::TrivialSet kKnownTwos = [](auto selector) {
    return selector()
        .Case(IntsPair{1, 0})
        .Case(IntsPair{1, 1})
        .Case(IntsPair{2, 0})
        .Case(IntsPair{2, 1})
        .Case(IntsPair{1, 1});
  };

  EXPECT_TRUE(kKnownTwos.Contains(IntsPair{2, 0}));
  EXPECT_FALSE(kKnownTwos.Contains(IntsPair{9, 0}));
}

TEST(TrivialBiMap, StringToString) {
  static constexpr utils::TrivialBiMap kEnglishToGerman = [](auto selector) {
    return selector()
        .Case("zero", "null")
        .Case("one", "eins")
        .Case("two", "zwei")
        .Case("three", "drei");
  };

  EXPECT_EQ(*kEnglishToGerman.TryFindByFirst("zero"), "null");
  EXPECT_EQ(*kEnglishToGerman.TryFindICaseByFirst("ZeRo"), "null");
  EXPECT_EQ(*kEnglishToGerman.TryFindICaseByFirst("three"), "drei");
  EXPECT_EQ(*kEnglishToGerman.TryFindICaseByFirst("Three"), "drei");

  EXPECT_EQ(*kEnglishToGerman.TryFindBySecond("null"), "zero");
  EXPECT_EQ(*kEnglishToGerman.TryFindICaseBySecond("NULL"), "zero");
  EXPECT_EQ(*kEnglishToGerman.TryFindICaseBySecond("DrEi"), "three");
  EXPECT_EQ(*kEnglishToGerman.TryFindICaseBySecond("Drei"), "three");
}

TEST(TrivialBiMap, FindICaseBySecond) {
  static constexpr utils::TrivialBiMap kNumToGerman = [](auto selector) {
    return selector().Case(0, "null").Case(1, "eins").Case(2, "zwei").Case(
        3, "drei");
  };

  EXPECT_EQ(*kNumToGerman.TryFind("null"), 0);
  EXPECT_EQ(*kNumToGerman.TryFindICase("NULL"), 0);
  EXPECT_EQ(*kNumToGerman.TryFindICase("DrEi"), 3);
  EXPECT_EQ(*kNumToGerman.TryFindICase("Drei"), 3);
}

constexpr utils::TrivialBiMap kICaseCheck = [](auto selector) {
  return selector()
      .Case("qwertyuiop[]asdfghjkl;'zxcvbnm,./`1234567890-=+_)(*&^%$#@!~==1", 1)
      .Case("qwertyuiop[]asdfghjkl;'zxcvbnm,./`1234567890-=+_)(*&^%$#@!~==2", 2)
      .Case("qwertyuiop[]asdfghjkl;'zxcvbnm,./`1234567890-=+_)(*&^%$#@!~==3", 3)
      .Case("qwertyuiop[]asdfghjkl;'zxcvbnm,./`1234567890-=+_)(*&^%$#@!~==4", 4)
      .Case("qwertyuiop[]asdfghjkl;'zxcvbnm,./`1234567890-=+_)(*&^%$#@!~==5", 5)
      .Case("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f", 6)
      .Case("\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f",
            7)
      .Case("\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8a\x8b\x8c\x8d\x8e\x8f",
            8)
      .Case("\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9a\x9b\x9c\x9d\x9e\x9f",
            9)
      .Case("\xa0\xa1\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9\xaa\xab\xac\xad\xae\xaf",
            10)
      .Case("\xb0\xb1\xb2\xb3\xb4\xb5\xb6\xb7\xb8\xb9\xba\xbb\xbc\xbd\xbe\xbf",
            11)
      .Case("\xc0\xc1\xc2\xc3\xc4\xc5\xc6\xc7\xc8\xc9\xca\xcb\xcc\xcd\xce\xcf",
            12)
      .Case("\xd0\xd1\xd2\xd3\xd4\xd5\xd6\xd7\xd8\xd9\xda\xdb\xdc\xdd\xde\xdf",
            13)
      .Case("\xe0\xe1\xe2\xe3\xe4\xe5\xe6\xe7\xe8\xe9\xea\xeb\xec\xed\xee\xef",
            14)
      .Case("\xf0\xf1\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfd\xfe\xff",
            15);
};

TEST(TrivialBiMap, StringICase) {
  EXPECT_EQ(
      kICaseCheck
          .TryFindICase(
              "qwertyuiop[]asdfghjkl;'zxcvbnm,./`1234567890-=+_)(*&^%$#@!~==5")
          .value(),
      5);
  EXPECT_EQ(
      kICaseCheck
          .TryFindICase(
              "QWERTYUIOP[]ASDFGHJKL;'ZXCVBNM,./`1234567890-=+_)(*&^%$#@!~==5")
          .value(),
      5);
  EXPECT_EQ(
      kICaseCheck
          .TryFindICase(
              "QwErTYUiOP[]ASDFGhJKL;'ZXCVBnM,./`1234567890-=+_)(*&^%$#@!~==5")
          .value(),
      5);

  EXPECT_FALSE(kICaseCheck.TryFind(
      "QWERTYUIOP[]ASDFGHJKL;'ZXCVBNM,./`1234567890-=+_)(*&^%$#@!~==5"));
  EXPECT_FALSE(kICaseCheck.TryFind(
      "QwErTYUiOP[]ASDFGhJKL;'ZXCVBnM,./`1234567890-=+_)(*&^%$#@!~==5"));

  EXPECT_EQ(
      kICaseCheck
          .TryFindICase(
              "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f")
          .value(),
      6);
  EXPECT_EQ(kICaseCheck
                .TryFindICase("\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b"
                              "\x1c\x1d\x1e\x1f")
                .value(),
            7);
  EXPECT_EQ(kICaseCheck
                .TryFindICase("\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8a\x8b"
                              "\x8c\x8d\x8e\x8f")
                .value(),
            8);
  EXPECT_EQ(kICaseCheck
                .TryFindICase("\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9a\x9b"
                              "\x9c\x9d\x9e\x9f")
                .value(),
            9);
  EXPECT_EQ(kICaseCheck
                .TryFindICase("\xa0\xa1\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9\xaa\xab"
                              "\xac\xad\xae\xaf")
                .value(),
            10);
  EXPECT_EQ(kICaseCheck
                .TryFindICase("\xb0\xb1\xb2\xb3\xb4\xb5\xb6\xb7\xb8\xb9\xba\xbb"
                              "\xbc\xbd\xbe\xbf")
                .value(),
            11);
  EXPECT_EQ(kICaseCheck
                .TryFindICase("\xc0\xc1\xc2\xc3\xc4\xc5\xc6\xc7\xc8\xc9\xca\xcb"
                              "\xcc\xcd\xce\xcf")
                .value(),
            12);
  EXPECT_EQ(kICaseCheck
                .TryFindICase("\xd0\xd1\xd2\xd3\xd4\xd5\xd6\xd7\xd8\xd9\xda\xdb"
                              "\xdc\xdd\xde\xdf")
                .value(),
            13);
  EXPECT_EQ(kICaseCheck
                .TryFindICase("\xe0\xe1\xe2\xe3\xe4\xe5\xe6\xe7\xe8\xe9\xea\xeb"
                              "\xec\xed\xee\xef")
                .value(),
            14);
  EXPECT_EQ(kICaseCheck
                .TryFindICase("\xf0\xf1\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb"
                              "\xfc\xfd\xfe\xff")
                .value(),
            15);

  // This attempts to test the
  // Case("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f", 6)
  // by changing the first byte on +16
  EXPECT_FALSE(kICaseCheck.TryFindICase(
      "\x11\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"));
  EXPECT_FALSE(kICaseCheck.TryFindICase(
      "\x21\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"));
  EXPECT_FALSE(kICaseCheck.TryFindICase(
      "\x31\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"));
  EXPECT_FALSE(kICaseCheck.TryFindICase(
      "\x41\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"));
  EXPECT_FALSE(kICaseCheck.TryFindICase(
      "\x51\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"));
  EXPECT_FALSE(kICaseCheck.TryFindICase(
      "\x61\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"));
  EXPECT_FALSE(kICaseCheck.TryFindICase(
      "\x71\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"));
  EXPECT_FALSE(kICaseCheck.TryFindICase(
      "\x81\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"));
  EXPECT_FALSE(kICaseCheck.TryFindICase(
      "\x91\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"));
  EXPECT_FALSE(kICaseCheck.TryFindICase(
      "\xa1\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"));
  EXPECT_FALSE(kICaseCheck.TryFindICase(
      "\xb1\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"));
  EXPECT_FALSE(kICaseCheck.TryFindICase(
      "\xc1\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"));
  EXPECT_FALSE(kICaseCheck.TryFindICase(
      "\xd1\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"));
  EXPECT_FALSE(kICaseCheck.TryFindICase(
      "\xe1\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"));
  EXPECT_FALSE(kICaseCheck.TryFindICase(
      "\xf1\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"));

  // This attempts to test the
  // Case("\xf0\xf1\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfd\xfe\xff",
  // 15) by changing the second byte on +16
  EXPECT_FALSE(kICaseCheck.TryFindICase(
      "\xf0\x01\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfd\xfe\xff"));
  EXPECT_FALSE(kICaseCheck.TryFindICase(
      "\xf0\x11\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfd\xfe\xff"));
  EXPECT_FALSE(kICaseCheck.TryFindICase(
      "\xf0\x21\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfd\xfe\xff"));
  EXPECT_FALSE(kICaseCheck.TryFindICase(
      "\xf0\x31\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfd\xfe\xff"));
  EXPECT_FALSE(kICaseCheck.TryFindICase(
      "\xf0\x41\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfd\xfe\xff"));
  EXPECT_FALSE(kICaseCheck.TryFindICase(
      "\xf0\x51\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfd\xfe\xff"));
  EXPECT_FALSE(kICaseCheck.TryFindICase(
      "\xf0\x61\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfd\xfe\xff"));
  EXPECT_FALSE(kICaseCheck.TryFindICase(
      "\xf0\x71\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfd\xfe\xff"));
  EXPECT_FALSE(kICaseCheck.TryFindICase(
      "\xf0\x81\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfd\xfe\xff"));
  EXPECT_FALSE(kICaseCheck.TryFindICase(
      "\xf0\x91\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfd\xfe\xff"));
  EXPECT_FALSE(kICaseCheck.TryFindICase(
      "\xf0\xa1\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfd\xfe\xff"));
  EXPECT_FALSE(kICaseCheck.TryFindICase(
      "\xf0\xb1\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfd\xfe\xff"));
  EXPECT_FALSE(kICaseCheck.TryFindICase(
      "\xf0\xc1\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfd\xfe\xff"));
  EXPECT_FALSE(kICaseCheck.TryFindICase(
      "\xf0\xd1\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfd\xfe\xff"));
  EXPECT_FALSE(kICaseCheck.TryFindICase(
      "\xf0\xe1\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfd\xfe\xff"));
}

USERVER_NAMESPACE_END
