#include <userver/storages/redis/mock_subscribe_client.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::test {

/// Here, we test that our Mock is correct
TEST(MockSubscribeClientTest, Basic) {
  std::shared_ptr<MockSubscribeClient> client_mock =
      std::make_shared<MockSubscribeClient>();

  using testing::_;

  {
    testing::InSequence seq;
    EXPECT_CALL(*client_mock, Subscribe("test_subscribe", _, _))
        .Times(1)
        .WillRepeatedly([](std::string, SubscriptionToken::OnMessageCb,
                           const USERVER_NAMESPACE::redis::CommandControl&) {
          return SubscriptionToken{};
        });

    EXPECT_CALL(*client_mock, Psubscribe("test_psubscribe", _, _))
        .Times(1)
        .WillRepeatedly([](std::string, SubscriptionToken::OnPmessageCb,
                           const USERVER_NAMESPACE::redis::CommandControl&) {
          return SubscriptionToken{};
        });
  }
  // check that wrong calls aren't there
  EXPECT_CALL(*client_mock, Subscribe("test_psubscribe", _, _)).Times(0);
  EXPECT_CALL(*client_mock, Psubscribe("test_subscribe", _, _)).Times(0);

  client_mock->Subscribe("test_subscribe", {}, {});
  client_mock->Psubscribe("test_psubscribe", {}, {});
}

/// Here, we test that our Mock is correct
TEST(MockSubscribeClientTest, SubscriptionToken) {
  //! [SbTknExmpl1]
  // Create a mocked subscribe client
  std::shared_ptr<MockSubscribeClient> client_mock =
      std::make_shared<MockSubscribeClient>();
  // We need to create a mocked token explicitly, to be able to set
  // expectaitons on it.
  std::unique_ptr<MockSubscriptionTokenImpl> token_mock =
      std::make_unique<MockSubscriptionTokenImpl>();

  using testing::_;

  // Set expectation on mock. We expect that Unsubscribe() will
  // be called strictly once
  EXPECT_CALL(*token_mock, Unsubscribe).Times(1);
  // Here is how to return a mocked token from Subscribe method.
  // Beware that after this line of code, token_mock will be nullptr!
  EXPECT_CALL(*client_mock, Subscribe("test_subscribe", _, _))
      .Times(1)
      .WillOnce(testing::Return(::testing::ByMove(SubscriptionToken{
          std::unique_ptr<SubscriptionTokenImplBase>{token_mock.release()}})));

  // check that wrong calls aren't there
  EXPECT_CALL(*client_mock, Psubscribe("test_subscribe", _, _)).Times(0);

  // Call Subscribe, get token. Inside this token there should be
  // our token_mock impl
  auto token = client_mock->Subscribe("test_subscribe", {}, {});
  // Can't get access to it, but we can check that something is actually
  // inside the token.
  EXPECT_FALSE(token.IsEmpty());
  token.Unsubscribe();
  //! [SbTknExmpl1]
}

}  // namespace storages::redis::test

USERVER_NAMESPACE_END
