## Supported Platforms

## Supported Platforms

🐙 **userver** framework supports a wide range of platforms, including but not limited to:

| | |
|--|-----------------------------|
| **OS** | Ubuntu, Debian, Fedora, Arch, Gentoo, macOS |
| **Architectures** | x86, x86_64, AArch64, Arm |
| **Compilers** | GCC-8 and newer, Clang-9 and newer |
| **C++ Standards** | C++17, C++20, C++23 |
| **C++ Standard Libraries** | libstdc++, libc++ |

x86_64 on Ubuntu with clang is additionally tested on multiple
hundreds of in-house services with a wide range of sanitizers and static
analyzers.

Feel free to [submit a feature request](https://github.com/userver-framework/userver/issues)
if your platform is not supported or if you found an issue.


## Helper services and templates

The [userver-framework](https://github.com/userver-framework/) github
organization page contains multiple repositories, including:

* [userver](https://github.com/userver-framework/userver) - the C++
  Asynchronous Framework.
* [service_template](https://github.com/userver-framework/service_template) -
  template of a C++ service that uses userver framework with ready-to-user
  build, test and CI scripts. 
* [uservice-dynconf](https://github.com/userver-framework/uservice-dynconf) -
  the service to control dynamic configs of the other userver-based services.

All the repositories are part of the userver framework,
thus they support the same set of architectures, compilers, operating systems
and C++ standards.

Feel free to ask questions about any parts of the framework at the
[english-speaking](https://t.me/userver_en) or [russian-speaking](https://t.me/userver_ru)
Telegram channels.


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref md_en_userver_framework_comparison | @ref md_en_userver_tutorial_build ⇨
@htmlonly </div> @endhtmlonly
