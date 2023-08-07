# About the BETA state

At the moment the userver framework is in the "beta" state. That state is meant
to show that the framework is in state of migration from in-house to a fully
open source development:
* some of the CI checks are still migrating to the github CI
* Docker images are being refactored to not rely on internal infrastructure
* documentation requires more examples and clarifications
* some integrations for the popular tools|services are being added

Moreover, as there's no release, there's no
@ref scripts/docs/en/userver/development/stability.md "API stability" yet.


## Is userver ready for production use?

We are successfully using userver for ages for our in-house development for
many high-load high-availability services.

Make an experiment to make sure that the current functionality suits your
needs. If there's something missing, please fill a
[feature request at github](https://github.com/userver-framework/userver/issues).


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/deploy_env.md | @ref scripts/docs/en/userver/roadmap_and_changelog.md ⇨
@htmlonly </div> @endhtmlonly
