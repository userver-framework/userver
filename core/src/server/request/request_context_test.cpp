#include <userver/server/request/request_context.hpp>

#include <userver/utest/assert_macros.hpp>

#include <memory>

USERVER_NAMESPACE_BEGIN

constexpr std::string_view kKey = "key";
constexpr std::string_view kOtherKey = "other_key";

TEST(RequestContext, UserData) {
  server::request::RequestContext context;
  EXPECT_FALSE(context.GetUserDataOptional<int>());

  context.SetUserData(42);
  ASSERT_TRUE(context.GetUserDataOptional<int>());
  UEXPECT_THROW(context.GetUserDataOptional<long long>(), std::bad_cast);
  EXPECT_EQ(context.GetUserData<int>(), 42);
  EXPECT_EQ(*context.GetUserDataOptional<int>(), 42);

  context.EraseUserData();
  EXPECT_FALSE(context.GetUserDataOptional<int>());

  context.EmplaceUserData<long long>(42);
  ASSERT_TRUE(context.GetUserDataOptional<long long>());
  UEXPECT_THROW(context.GetUserDataOptional<int>(), std::bad_cast);
  EXPECT_EQ(context.GetUserData<long long>(), 42);
  EXPECT_EQ(*context.GetUserDataOptional<long long>(), 42);

  UEXPECT_THROW(context.SetUserData<long>(2), std::runtime_error);
  EXPECT_EQ(context.GetUserData<long long>(), 42);

  context.EraseUserData();
  context.SetUserData<long>(2);
  ASSERT_TRUE(context.GetUserDataOptional<long>());
  UEXPECT_THROW(context.GetUserDataOptional<int>(), std::bad_cast);
  EXPECT_EQ(context.GetUserData<long>(), 2);
  EXPECT_EQ(*context.GetUserDataOptional<long>(), 2);
}

TEST(RequestContext, ConstUserData) {
  server::request::RequestContext context;
  context.SetUserData<int>(42);

  ASSERT_TRUE(context.GetUserDataOptional<const int&>());
  ASSERT_TRUE(context.GetUserDataOptional<const int>());
  ASSERT_TRUE(context.GetUserDataOptional<int>());

  EXPECT_EQ(context.GetUserData<const int&>(), 42);
  EXPECT_EQ(context.GetUserData<const int>(), 42);
  EXPECT_EQ(context.GetUserData<int>(), 42);

  EXPECT_EQ(*context.GetUserDataOptional<const int&>(), 42);
  EXPECT_EQ(*context.GetUserDataOptional<const int>(), 42);
  EXPECT_EQ(*context.GetUserDataOptional<int>(), 42);
}

TEST(RequestContext, UserDataReference) {
  server::request::RequestContext context;

  int value = 42;

  context.SetUserData(value);
  ASSERT_TRUE(context.GetUserDataOptional<const int&>());
  ASSERT_TRUE(context.GetUserDataOptional<int&>());
  ASSERT_TRUE(context.GetUserDataOptional<int>());

  value = 84;
  EXPECT_EQ(context.GetUserData<const int&>(), 42);
  EXPECT_EQ(context.GetUserData<const int>(), 42);
  EXPECT_EQ(context.GetUserData<int>(), 42);
  EXPECT_EQ(value, 84);
}

TEST(RequestContext, UserMoveOnlyByConst) {
  server::request::RequestContext context;

  auto value = std::make_unique<int>(42);

  context.SetUserData(std::move(value));
  EXPECT_EQ(*context.GetUserData<std::unique_ptr<int>>(), 42);
}

TEST(RequestContext, Data) {
  server::request::RequestContext context;
  EXPECT_FALSE(context.GetDataOptional<int>(kKey));
  EXPECT_FALSE(context.GetDataOptional<int>(kOtherKey));

  context.SetData(std::string{kKey}, 42);
  ASSERT_TRUE(context.GetDataOptional<int>(kKey));
  UEXPECT_THROW(context.GetDataOptional<long long>(kKey), std::bad_cast);
  EXPECT_FALSE(context.GetDataOptional<int>(kOtherKey));
  EXPECT_EQ(context.GetData<int>(kKey), 42);
  EXPECT_EQ(*context.GetDataOptional<int>(kKey), 42);

  context.EmplaceData<long long>(std::string{kOtherKey}, 1);
  ASSERT_TRUE(context.GetDataOptional<long long>(kOtherKey));
  UEXPECT_THROW(context.GetDataOptional<int>(kOtherKey), std::bad_cast);
  EXPECT_EQ(context.GetData<long long>(kOtherKey), 1);
  EXPECT_EQ(*context.GetDataOptional<long long>(kOtherKey), 1);
  EXPECT_EQ(context.GetData<int>(kKey), 42);

  UEXPECT_THROW(context.SetData<long long>(std::string{kKey}, 2),
                std::runtime_error);
  EXPECT_EQ(context.GetData<int>(kKey), 42);
  EXPECT_EQ(context.GetData<long long>(kOtherKey), 1);

  context.EraseData(kKey);
  EXPECT_FALSE(context.GetDataOptional<int>(kKey));
  ASSERT_TRUE(context.GetDataOptional<long long>(kOtherKey));
  EXPECT_EQ(*context.GetDataOptional<long long>(kOtherKey), 1);

  context.SetData<long long>(std::string{kKey}, 2);
  EXPECT_EQ(context.GetData<long long>(kOtherKey), 1);
  EXPECT_EQ(context.GetData<long long>(kKey), 2);
}

TEST(RequestContext, ConstData) {
  server::request::RequestContext context;
  context.SetData<int>(std::string{kKey}, 42);

  ASSERT_TRUE(context.GetDataOptional<const int&>(kKey));
  ASSERT_TRUE(context.GetDataOptional<const int>(kKey));
  ASSERT_TRUE(context.GetDataOptional<int>(kKey));

  EXPECT_EQ(context.GetData<const int&>(kKey), 42);
  EXPECT_EQ(context.GetData<const int>(kKey), 42);
  EXPECT_EQ(context.GetData<int>(kKey), 42);

  EXPECT_EQ(*context.GetDataOptional<const int&>(kKey), 42);
  EXPECT_EQ(*context.GetDataOptional<const int>(kKey), 42);
  EXPECT_EQ(*context.GetDataOptional<int>(kKey), 42);
}

TEST(RequestContext, DataReference) {
  server::request::RequestContext context;

  int value = 42;

  context.SetData<int>(std::string{kKey}, value);
  ASSERT_TRUE(context.GetDataOptional<const int&>(kKey));
  ASSERT_TRUE(context.GetDataOptional<int&>(kKey));
  ASSERT_TRUE(context.GetDataOptional<int>(kKey));

  value = 84;
  EXPECT_EQ(context.GetData<const int&>(kKey), 42);
  EXPECT_EQ(context.GetData<const int>(kKey), 42);
  EXPECT_EQ(context.GetData<int>(kKey), 42);
  EXPECT_EQ(value, 84);
}

TEST(RequestContext, MoveOnlyByConst) {
  server::request::RequestContext context;

  auto value = std::make_unique<int>(42);

  context.SetData(std::string{kKey}, std::move(value));
  EXPECT_EQ(*context.GetData<std::unique_ptr<int>>(kKey), 42);
}

USERVER_NAMESPACE_END
