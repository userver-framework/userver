Name:    userver
Version: %{_version}
Release: ssv1%{?dist}
Summary: userver framework
Group:   Development/Libraries/C and C++
License: Apache License 2.0
URL:     https://github.com/userver-framework/userver
Source0: https://github.com/yoori/userver/archive/refs/tags/v%{_version}.tar.gz
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
BuildRequires: rapidjson-devel
BuildRequires: moodycamel-concurrentqueue-devel
BuildRequires: compiler-rt
BuildRequires: boost-devel >= 1.66.0
BuildRequires: amqp-cpp-devel
BuildRequires: clickhouse-cpp-devel

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
Requires: clickhouse-cpp
Requires: amqp-cpp
#Requires: boost-stacktrace >= 1.66.0

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
  -DCMAKE_INSTALL_LIBDIR=lib64 -DUSERVER_DOWNLOAD_PACKAGES:BOOL=OFF ..
make -j
popd

echo ">>>> Third party used from GIT: "
ls -1 third_party/ | grep -v -E '^(boost_stacktrace|compiler-rt|moodycamel|pfr|rapidjson)$'
echo "<<<<"

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}
echo "Install to %{buildroot}"

pushd build
DESTDIR=%{buildroot}/ make install INSTALL_PATH=%{buildroot}/
# move third party installations into lib64
#cp -r %{buildroot}/usr/lib %{buildroot}/usr/lib64/
#rm -rf %{buildroot}/usr/lib

# TODO cmake config install
#mv %{buildroot}/usr/cmake %{buildroot}/usr/lib64/
#rm -rf %{buildroot}/usr/cmake
popd

%clean
rm -rf %{buildroot}

%files -n %{name}
%{_bindir}/userver/
%{_libdir}/lib*.so

%files -n %{name}-devel
%{_includedir}/
#{_libdir}/lib*.a
#{_libdir}/cmake/*.cmake
