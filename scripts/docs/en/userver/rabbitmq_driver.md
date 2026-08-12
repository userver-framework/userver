# RabbitMQ (AMQP 0-9-1)

**Quality:** @ref QUALITY_TIERS "Golden Tier".

🐙 **userver** provides access to RabbitMQ servers via
components::RabbitMQ. The uRabbitMQ driver is asynchronous, it suspends
current coroutine for carrying out network I/O.

@section rabbitmq_feature Features
- Publishing messages;
- Consuming messages;
- Creating Exchanges, Queues and Bindings;
- Transport level security;
- Connections pooling;
- End-to-end logging for messages in publish->consume chain.

@section rabbitmq_info More information
- For configuration see components::RabbitMQ
- For cluster operations see urabbitmq::Client, urabbitmq::AdminChannel,
  urabbitmq::Channel, urabbitmq::ReliableChannel
- For consumers support see urabbitmq::ConsumerBase and
  urabbitmq::ConsumerComponentBase

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/grpc/server_middlewares.md |
@ref scripts/docs/en/userver/dynamic_config.md ⇨
@htmlonly </div> @endhtmlonly
