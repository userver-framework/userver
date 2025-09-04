# Distro package maintainers

## Maintainer's role

We develop userver framework and take care of our users. We want userver to be available on as much platforms, OSes and
distros as possible. It is quite difficult to maintain such complex product as userver working in any environment. It
is a difficult engineer problem, but also it is often difficult or impossible to test userver on the target platform
because of absent software/hardware.

That's why there are userver distro package maintainers! That's the community role which is responsible for maintaining
userver buildable and working in a specific environment.

A maintainer should (where possible):
1) keep build dependencies list in @ref scripts/docs/en/deps/ for the distro up-to-date
2) watch for new distro version releases and create new dependency files in @ref scripts/docs/en/deps soon after release
3) cleanup EOL (end of life) distro versions dependency files
4) maintain CI setup in @ref .github/workflows in "green" state

The maintainer is a community role which is not paid, but is a honorable one. You will get a "distro maintainer" label
in official userver telegram channels.


## Call for maintainers

Currently we're looking for maintainers for the following OSes/distros:

1) MacOS
2) Fedora
3) Debian
4) ArchLinux
5) Gentoo
6) FreeBSD
7) Conan package

The list is not complete, you may suggest your own distro to maintain.


## Current maintainer list

Active maintainers so far:
- Ubuntu - userver team


## Contacts

If you feel you're ready to be a maintainer, please contact us by any of the following methods:
- telegram https://t.me/userver_ru (in Russian)
- telegram https://t.me/userver_en (in English)

Then please make a pull request to [github](https://github.com/userver-framework/userver/) which changes this page and
adds you to the "Current maintainer list" section.


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/roadmap_and_changelog.md | @ref scripts/docs/en/userver/faq.md ⇨
@htmlonly </div> @endhtmlonly
