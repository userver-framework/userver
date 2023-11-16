## Dynamic config

Dynamic config is a system of options that can be changed at runtime without
restarting the service.

For schemas of dynamic configs used by userver itself, see
@ref scripts/docs/en/schemas/dynamic_configs.md

For an information on how to write a service that distributes dynamic configs
see @ref scripts/docs/en/userver/tutorial/config_service.md.


### Adding and using your own dynamic configs

Dynamic config values could be obtained via the dynamic_config::Source client
that could be retrieved from components::DynamicConfig:

@snippet components/component_sample_test.cpp  Sample user component source


To get a specific value you need a parser for it. For example, here's how you
could parse and get the `SAMPLE_INTEGER_FROM_RUNTIME_CONFIG` option:

@snippet components/component_sample_test.cpp  Sample user component runtime config source

If utils::DaemonMain is used, then the dynamic configs of the service could
be printed by passing `--print-dynamic-config-defaults` command line option.


### Why your service needs Dynamic configs

Dynamic configs are an essential part of a reliable service with high
availability. The configs could be used as an emergency switch for new
functionality, selector for experiments, limits/timeouts/log-level setup,
proxy setup and so forth.

See @ref scripts/docs/en/userver/tutorial/production_service.md setup example.


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref rabbitmq_driver | @ref scripts/docs/en/schemas/dynamic_configs.md ⇨
@htmlonly </div> @endhtmlonly
