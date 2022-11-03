#!/bin/bash

VERSION=$1
VERSIONSSV=ssv2

# script require 'sudo rpm' for install RPM packages
# and access to mirror.yandex.ru repository

# create build/RPMS folder - all built packages will be duplicated here
RES_TMP=build/TMP/
RES_RPMS=build/RPMS/
rm -rf "$RES_TMP"

mkdir -p $RES_TMP
mkdir -p $RES_RPMS

# download and install packages required for build
sudo yum -y install spectool yum-utils rpmdevtools redhat-rpm-config rpm-build autoconf automake libtool \
  glib2-devel cmake gcc-c++ epel-rpm-macros \
  || \
  { echo "can't install base packages" >&2 ; exit 1 ; }

sudo yum -y install libzstd-devel zlib-devel || \
  { echo "can't install base packages" >&2 ; exit 1 ; }

# create folders for RPM build environment
mkdir -vp  `rpm -E '%_tmppath %_rpmdir %_builddir %_sourcedir %_specdir %_srcrpmdir %_rpmdir/%_arch'`

#cp /home/ykuznetsov/rocksdb-6.5.2.tar.gz "`rpm -E %_sourcedir`"/

BIN_RPM_FOLDER=$(rpm -E '%_rpmdir/%_arch')

# download librdkafka source RPM
SPEC_FILE=$(rpm -E %_specdir)/gtest-$VERSION-$VERSIONSSV.spec

cat << 'EOF' > $SPEC_FILE
Name:    gtest
Version: %{_version}
Release: ssv2%{?dist}
Summary: Google C++ testing framework
Group:   Development/Libraries/C and C++
License: BSD and ASL2.0
URL:     https://github.com/google/googletest
Source0: https://github.com/google/googletest/archive/refs/tags/release-%{version}.tar.gz
BuildRequires: cmake
BuildRequires: gcc-c++

BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{_version}-%{release}-XXXXXX)
%define __product protobuf
%define __cmake_build_dir build
%description

This package contains development files for gtest.

%package -n %{name}-devel
Summary: Google C++ testing framework
Group:   Development/Libraries/C and C++
Requires: %{name} = %{version}
%description -n %{name}-devel

This package contains development files for gtest.

%prep
%setup -q -n googletest-release-%{_version}

%build
mkdir build
pushd build
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_SHARED_LIBS=ON -DCMAKE_SKIP_BUILD_RPATH=TRUE -Dgtest_build_tests=OFF ..
make -j
popd

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}
echo "Install to %{buildroot}"

pushd build
DESTDIR=%{buildroot}/ make install INSTALL_PATH=%{buildroot}/
popd

%clean
rm -rf %{buildroot}
%post   -n %{name} -p /sbin/ldconfig
%postun -n %{name} -p /sbin/ldconfig

%files -n %{name}
%defattr(444,root,root)
%{_libdir}/libgmock.so.*
%{_libdir}/libgtest.so.*
%{_libdir}/libgmock_main.so.*
%{_libdir}/libgtest_main.so.*

%files -n %{name}-devel
%{_libdir}/libgtest.so
%{_libdir}/libgtest_main.so
%{_libdir}/libgmock.so
%{_libdir}/libgmock_main.so
%{_includedir}/gtest
%{_includedir}/gmock
%{_libdir}/cmake/GTest
%{_libdir}/pkgconfig/*.pc
EOF

$SUDO_PREFIX yum-builddep -y "$SPEC_FILE" || \
  { echo "can't install build requirements" >&2 ; exit 1 ; }

spectool --force -g -R --define "_version $VERSION" "$SPEC_FILE" || \
  { echo "can't download sources" >&2 ; exit 1 ; }

rpmbuild --force -ba --define "_version $VERSION" "$SPEC_FILE" || \
  { echo "can't build RPM" >&2 ; exit 1 ; }

# install
cp $BIN_RPM_FOLDER/gtest-*$VERSION-*.rpm $RES_RPMS/

