# Releases, Trunk-based Development and Pull Requests

Information on production usage of userver source code and guarantees.


## Is userver ready for production use?

Yes. We are successfully using userver for ages for
many high-load high-availability services in production.

Make an experiment to make sure that the current functionality suits your
needs. If there's something missing, please fill a
[feature request at github](https://github.com/userver-framework/userver/issues).


## Trunk-based Development

🐙 userver follows the Trunk-Based Development model. In practice it means that
all the revisions from the `develop` branch work fine and all of them have been
**in production** at some point.

Any big new feature is enabled by default **only** after
it goes through unit tests, functional tests, experiments in different
environments and different stages of production adoption.

This development strategy provides fast delivery and adoption of new features.


## Releases and Release model

The above model may not fit if stability of interfaces is
required. Examples of such development cases are: making prebuilt packeges of
the framework for package managers or OS distros, release based development
model in the company, prototyping.

To satisfy the need in stability, the framework also provides releases -
snapshots of the `develop` sources with
@ref scripts/docs/en/userver/development/stability.md "API stability" guarantee.

Releases do not evolve, all the new functionality and bugfixes go to
develop branch of git to become a new release some day.


## Issue, feature and pull requests (PR)

We do our best to implement feature requests and review the pull requests.
However, we prioritize our in-house feature requests and, alas, our resources
are limited.

Please, see @ref CONTRIBUTING.md before making a PR or posting code!

To report an issue or provide a pull request use the github interface
https://github.com/userver-framework/.

There is a queue of simple tickets [for first-time contributors](https://github.com/userver-framework/userver/issues?q=is%3Aissue+is%3Aopen+label%3A%22good+first+issue%22+label%3A%22help+wanted%22)
and a queue of more complex features [for experienced contributors](https://github.com/userver-framework/userver/issues?q=is%3Aissue+is%3Aopen+label%3A%22big%22+label%3A%22help+wanted%22).
If you're planning to contribute soon or starting a work on a big feature,
please write a comment about it into the issue. That way we could remove the
issue from a queue and avoid contention on features.

Bugfixes and new features are not ported to older releases. To get the
functionality you have to update to a new release with a fix.


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/deploy_env.md | @ref scripts/docs/en/userver/roadmap_and_changelog.md ⇨
@htmlonly </div> @endhtmlonly

