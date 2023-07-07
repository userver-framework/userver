#include <userver/utils/not_null.hpp>
#include <userver/utils/shared_readable_ptr.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

TEST(NotNull, FromPtrConstructor) {
  int sample = 179;
  auto ptr = std::make_shared<int>(sample);
  utils::NotNull<std::shared_ptr<int>> ref(ptr);
  EXPECT_EQ(ref.GetBase(), ptr);

  utils::NotNull<std::shared_ptr<int>> moved_ref(std::move(ptr));
  EXPECT_EQ(*moved_ref, sample);
}

TEST(NotNull, ConvertConstructor) {
  int sample = 179;
  const auto ptr = std::make_shared<int>(sample);
  utils::NotNull<utils::SharedReadablePtr<int>> readable_ref(ptr);
  EXPECT_EQ(readable_ref.GetBase(), ptr);

  utils::NotNull<std::shared_ptr<int>> ref(ptr);

  utils::NotNull<utils::SharedReadablePtr<int>> ref_copy(ref);
  EXPECT_EQ(ref_copy.GetBase(), ptr);

  utils::NotNull<utils::SharedReadablePtr<int>> ref_move(std::move(ref));
  EXPECT_EQ(*ref_move, sample);
}

TEST(NotNull, CopyAndMoveConstructor) {
  int sample = 179;
  auto ptr = std::make_shared<int>(sample);
  utils::NotNull<std::shared_ptr<int>> ref(ptr);

  auto copy_ref(ref);
  EXPECT_EQ(ref.GetBase(), copy_ref.GetBase());

  auto move_ref(std::move(ref));
  EXPECT_EQ(*move_ref, sample);
}

TEST(NotNull, CopyAndMoveOperator) {
  int sample = 179;
  auto ptr = std::make_shared<int>(sample);
  utils::NotNull<std::shared_ptr<int>> ref(ptr);

  auto copy_ref = ref;
  EXPECT_EQ(ref.GetBase(), copy_ref.GetBase());

  auto move_ref = std::move(ref);
  EXPECT_EQ(*move_ref, sample);
}

TEST(NotNull, Operator) {
  int sample = 179;
  auto first_ptr = std::make_shared<int>(sample);
  auto second_ptr = std::make_shared<int>(sample);

  utils::NotNull<std::shared_ptr<int>> first_ref(first_ptr);
  utils::NotNull<std::shared_ptr<int>> second_ref(second_ptr);

  utils::NotNull<std::shared_ptr<int>> equal_to_first_ref(first_ptr);

  EXPECT_TRUE(first_ref != second_ref);
  EXPECT_FALSE(first_ref == second_ref);

  EXPECT_TRUE(first_ref == equal_to_first_ref);
  EXPECT_FALSE(first_ref != equal_to_first_ref);
}

TEST(NotNull, Get) {
  std::vector<int> sample_vector{1, 2, 3};
  auto ptr = std::make_shared<std::vector<int>>(sample_vector);
  utils::NotNull<std::shared_ptr<std::vector<int>>> ref(ptr);

  EXPECT_EQ(*ref, sample_vector);
  EXPECT_EQ(ref.GetBase(), ptr);
  EXPECT_EQ(ref->size(), sample_vector.size());
}

TEST(NotNull, Pointer) {
  int sample = 179;
  utils::NotNull ref(&sample);
  EXPECT_EQ(ref.GetBase(), &sample);
}

TEST(NotNull, ConvertibleFromReference) {
  int sample = 179;
  utils::NotNull<int*> ref = sample;
  EXPECT_EQ(ref.GetBase(), &sample);
}

TEST(NotNull, SharedRef) {
  int sample = 179;
  auto ref = utils::MakeSharedRef<int>(sample);
  EXPECT_EQ(*ref, sample);
}

TEST(NotNull, UniqueRef) {
  int sample = 179;
  auto ref = utils::MakeUniqueRef<int>(sample);
  EXPECT_EQ(*ref, sample);
}

TEST(NotNull, BoolConversion) {
  EXPECT_FALSE((std::is_convertible_v<utils::NotNull<int*>, bool>));
  EXPECT_FALSE((std::is_constructible_v<bool, utils::NotNull<int*>>));
}

USERVER_NAMESPACE_END
