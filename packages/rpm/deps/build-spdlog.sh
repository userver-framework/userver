#!/bin/bash

VERSION=$1
VERSIONSSV=ssv1

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
SPEC_FILE=$(rpm -E %_specdir)/spdlog-$VERSION-$VERSIONSSV.spec

cat << 'EOF' > $SPEC_FILE
Name:    spdlog
Version: %{_version}
Release: ssv2%{?dist}
Summary: Super fast C++ logging library
Group:   Development/Libraries/C and C++
License: BSD-2-Clause
URL:     https://github.com/gabime/spdlog
Source0: https://github.com/gabime/spdlog/archive/refs/tags/v%{_version}.tar.gz
BuildRequires: cmake
BuildRequires: gcc-c++

BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{_version}-%{release}-XXXXXX)
%define __product protobuf
%define __cmake_build_dir build
%description

This is a packaged version of the gabime/spdlog C++ logging library available at Github.

%package -n %{name}-devel
Summary: Super fast C++ logging library
Group:   Development/Libraries/C and C++
Requires: %{name} = %{version}
%description -n %{name}-devel

This is a packaged version of the gabime/spdlog C++ logging library available at Github.

%prep
%setup -q -n spdlog-%{_version}

%build
mkdir build-shared
pushd build-shared
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DSPDLOG_INSTALL:BOOL=ON -DSPDLOG_BUILD_SHARED:BOOL=ON -DCMAKE_INSTALL_PREFIX=/usr ..
make -j
popd

mkdir build-static
pushd build-static
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DSPDLOG_INSTALL:BOOL=ON -DSPDLOG_BUILD_SHARED:BOOL=OFF -DCMAKE_INSTALL_PREFIX=/usr ..
make -j
popd

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}
echo "Install to %{buildroot}"

pushd build-shared
DESTDIR=%{buildroot}/ make install INSTALL_PATH=%{buildroot}/
popd

pushd build-static
DESTDIR=%{buildroot}/ make install INSTALL_PATH=%{buildroot}/
popd

#mv %{buildroot}/usr/lib %{buildroot}/usr/lib64

%clean
rm -rf %{buildroot}
%post   -n %{name} -p /sbin/ldconfig
%postun -n %{name} -p /sbin/ldconfig

%files -n %{name}
%{_libdir}/libspdlog.so.1*

%files -n %{name}-devel
%{_libdir}/lib%{name}.so
%{_libdir}/lib%{name}.a
%{_includedir}/spdlog
%{_libdir}/cmake/spdlog
%{_libdir}/pkgconfig/*.pc
EOF

$SUDO_PREFIX yum-builddep -y "$SPEC_FILE" || \
  { echo "can't install build requirements" >&2 ; exit 1 ; }

spectool --force -g -R --define "_version $VERSION" "$SPEC_FILE" || \
  { echo "can't download sources" >&2 ; exit 1 ; }

rpmbuild --force -ba --define "_version $VERSION" "$SPEC_FILE" || \
  { echo "can't build RPM" >&2 ; exit 1 ; }

# install
cp $BIN_RPM_FOLDER/spdlog*.rpm $RES_RPMS/

