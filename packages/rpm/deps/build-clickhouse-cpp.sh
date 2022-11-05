#!/bin/bash

VERSION=$1
VERSIONSSV=ssv2
SCRIPTDIR=$(dirname "$0")

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

# create folders for RPM build environment
mkdir -vp  `rpm -E '%_tmppath %_rpmdir %_builddir %_sourcedir %_specdir %_srcrpmdir %_rpmdir/%_arch'`

BIN_RPM_FOLDER=$(rpm -E '%_rpmdir/%_arch')

SPEC_FILE=$(rpm -E %_specdir)/clickhouse-cpp-$VERSION-$VERSIONSSV.spec

cat << 'EOF' > $SPEC_FILE
Name:    clickhouse-cpp
Version: %{_version}
Release: ssv1%{?dist}
Summary: Clickhouse C++ client library
Group:   Development/Libraries/C and C++
License: Apache License 2.0
URL:     https://github.com/ClickHouse/clickhouse-cpp
Source0: https://github.com/yoori/clickhouse-cpp/archive/refs/tags/v%{version}.tar.gz
#Source0: https://github.com/ClickHouse/clickhouse-cpp/archive/refs/tags/v{version}.tar.gz
BuildRequires: cmake
BuildRequires: gcc
BuildRequires: gcc-c++

BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{_version}-%{release}-XXXXXX)

%description

Clickhouse C++ client library

%package -n %{name}-devel
Summary: Clickhouse C++ client library
Group:   Development/Libraries/C and C++
Requires: %{name} = %{version}
%description -n %{name}-devel

Clickhouse C++ client library

%prep
%setup -q -n clickhouse-cpp-%{_version}

%build
%cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX:PATH=/usr \
  -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON -DCMAKE_INSTALL_LIBDIR=%{_lib}

make -j

%install
DESTDIR=%{buildroot}/ make install INSTALL_PATH=%{buildroot}/
#mv %{buildroot}/usr/lib %{buildroot}/usr/lib64

%clean
rm -rf %{buildroot}

%files -n %{name}
%license LICENSE
%doc README.md

%{_libdir}/libclickhouse-cpp-lib.so

%files -n %{name}-devel

%{_includedir}/clickhouse
%{_includedir}/clickhouse-cpp-deps
%{_libdir}/libclickhouse-cpp-lib-static.a
%{_libdir}/clickhouse-cpp-deps
%{_libdir}/cmake
%{_libdir}/pkgconfig

EOF

$SUDO_PREFIX yum-builddep -y "$SPEC_FILE" || \
  { echo "can't install build requirements" >&2 ; exit 1 ; }

spectool --force -g -R --define "_version $VERSION" "$SPEC_FILE" || \
  { echo "can't download sources" >&2 ; exit 1 ; }

rpmbuild --force -ba --define "_version $VERSION" "$SPEC_FILE" || \
  { echo "can't build RPM" >&2 ; exit 1 ; }

# install
cp $BIN_RPM_FOLDER/clickhouse-cpp-*$VERSION-*.rpm $RES_RPMS/

