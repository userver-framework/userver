# Issues, feature and pull requests, releases

## Release model

Userver follows the Trunk-Based Development model. In practice it means that
you could take a random revision of userver from `develop` branch and it would
work fine. Moreover, in our code-base each revision
of userver is tested on all the existing services,
updated quite often, up to multiple times per day. 

The above model may not fit all the projects. Because of that, we do releases - 
a snapshot of the trunk sources.

See @ref md_en_userver_development_stability "info on versioning and stability" 

Releases do not evolve, all the new functionality and bugfixes go to trunk
(develop branch of git) and become a new release.

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
⇦ @ref md_en_userver_development_stability | @ref md_en_userver_driver_guide ⇨
@htmlonly </div> @endhtmlonly
