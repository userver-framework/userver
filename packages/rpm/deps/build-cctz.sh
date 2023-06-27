#!/bin/bash

VERSION=2.3
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
SPEC_FILE=$(rpm -E %_specdir)/cctz-$VERSION-$VERSIONSSV.spec

cat << 'EOF' > $SPEC_FILE
Name:    cctz
Version: %{_version}
Release: ssv1%{?dist}
Summary: cctz give C++ programmers all the necessary tools for computing with dates, times, and time zones in a simple and correct manner.
Group:   Development/Libraries/C and C++
License: ASL2.0
URL:     https://github.com/google/cctz
Source0: https://github.com/google/cctz/archive/refs/tags/v%{version}.tar.gz
BuildRequires: cmake
BuildRequires: gcc
BuildRequires: gcc-c++

BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{_version}-%{release}-XXXXXX)

%description

cctz give C++ programmers all the necessary tools for computing with dates, times, and time zones in a simple and correct manner.

%package -n %{name}-devel
Summary: cctz give C++ programmers all the necessary tools for computing with dates, times, and time zones in a simple and correct manner.
Group:   Development/Libraries/C and C++
Requires: %{name} = %{version}
%description -n %{name}-devel

cctz give C++ programmers all the necessary tools for computing with dates, times, and time zones in a simple and correct manner.

%prep
%autosetup -p1

%build
%cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON

%cmake_build

%install
%cmake_install

%clean
rm -rf %{buildroot}
%post   -n %{name} -p /sbin/ldconfig
%postun -n %{name} -p /sbin/ldconfig

%files -n %{name}
%{_bindir}/time_tool
%{_libdir}/lib%{name}.so

%files -n %{name}-devel

%{_includedir}/%{name}
%{_libdir}/cmake/%{name}

EOF

$SUDO_PREFIX yum-builddep -y "$SPEC_FILE" || \
  { echo "can't install build requirements" >&2 ; exit 1 ; }

spectool --force -g -R --define "_version $VERSION" "$SPEC_FILE" || \
  { echo "can't download sources" >&2 ; exit 1 ; }

rpmbuild --force -ba --define "_version $VERSION" "$SPEC_FILE" || \
  { echo "can't build RPM" >&2 ; exit 1 ; }

# install
cp $BIN_RPM_FOLDER/cctz-*$VERSION-*.rpm $RES_RPMS/

