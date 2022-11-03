Name:    userver
Version: %{_version}
Release: ssv1%{?dist}
Summary: userver framework
Group:   Development/Libraries/C and C++
License: Apache License 2.0
URL:     https://github.com/userver-framework/userver
Source0: https://github.com/yoori/userver/archive/refs/tags/%{_version}.tar.gz
BuildRequires: cmake
BuildRequires: python36 python3-libs
BuildRequires: gcc-c++
BuildRequires: cctz-devel gtest-devel spdlog-devel
Requires: cctz gtest spdlog
BuildRequires: libnghttp2-devel libidn2-devel brotli-devel c-ares-devel
Requires: libnghttp2 libidn2 brotli c-ares
BuildRequires: libcurl-devel libbson-devel
Requires: libcurl libbson
BuildRequires: mongo-c-driver-libs mongo-c-driver
Requires: mongo-c-driver-libs mongo-c-driver
BuildRequires: fmt-devel
Requires: fmt
BuildRequires: libssh-devel
Requires: libssh
BuildRequires: hiredis-devel
Requires: hiredis

BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{_version}-%{release}-XXXXXX)

%description

Open source asynchronous framework with a rich set of abstractions for fast and comfortable creation of C++ microservices, services and utilities.

%package -n %{name}-devel
Summary: userver framework
Group:   Development/Libraries/C and C++
Requires: %{name} = %{version}
Requires: cctz-devel gtest-devel spdlog-devel
Requires: libnghttp2-devel libidn2-devel brotli-devel c-ares-devel
Requires: libcurl-devel libbson-devel
Requires: mongo-c-driver-libs mongo-c-driver
Requires: fmt-devel
Requires: libssh-devel
Requires: hiredis-devel

%description -n %{name}-devel

Open source asynchronous framework with a rich set of abstractions for fast and comfortable creation of C++ microservices, services and utilities.

%prep
%setup -q -n userver-%{_version}

%build
rm -rf build
mkdir build
pushd build
export PYTHONPATH=$PYTHONPATH:/usr/local/lib/python3.6/site-packages
cmake -DUSERVER_FEATURE_PATCH_LIBPQ=0 -DCMAKE_BUILD_TYPE=Release -DUSERVER_FEATURE_STACKTRACE=0 \
  -DPython3_EXECUTABLE=/usr/bin/python3 \
  -DUSERVER_BUILD_SHARED_LIBS:BOOL=ON -DCMAKE_INSTALL_PREFIX:PATH=/usr \
  -DCMAKE_INSTALL_LIBDIR=lib64 ..
make -j
popd

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}
echo "Install to %{buildroot}"

pushd build
DESTDIR=%{buildroot}/ make install INSTALL_PATH=%{buildroot}/
# move third party installations into lib64
cp -r %{buildroot}/usr/lib %{buildroot}/usr/lib64
rm -rf %{buildroot}/usr/lib
popd

%clean
rm -rf %{buildroot}

%files -n %{name}
%{_libdir}/lib*.so

%files -n %{name}-devel
%{_includedir}/
%{_libdir}/lib%{name}.a
%{_libdir}/cmake/amqpcppConfig.cmake
%{_libdir}/cmake/amqpcppConfig-release.cmake
%{_libdir}/pkgconfig/*.pc
