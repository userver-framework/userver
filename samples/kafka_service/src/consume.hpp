#pragma once

#include <userver/kafka/message.hpp>

#include <userver/utest/using_namespace_userver.hpp>

namespace kafka_sample {

void Consume(kafka::MessageBatchView messages);

}  // namespace kafka_sample
