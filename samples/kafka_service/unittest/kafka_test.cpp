#include <userver/kafka/utest/kafka_fixture.hpp>

#include <userver/utest/using_namespace_userver.hpp>

#include <consume.hpp>
#include <produce.hpp>

class KafkaServiceTest : public kafka::utest::KafkaCluster {};

UTEST_F(KafkaServiceTest, Produce) {
  auto producer = MakeProducer("kafka-producer");
  EXPECT_EQ(
      kafka_sample::Produce(
          producer, kafka_sample::RequestMessage{"test-topic-1", "test-key",
                                                 "test-message"}),
      kafka_sample::SendStatus::kSuccess);
}
