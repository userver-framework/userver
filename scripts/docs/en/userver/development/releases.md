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

To report an issue or provide a pull request use the github interface.

Bugfixes and new features are not ported to older releases. To get the
functionality you have to update to a new release with a fix.
