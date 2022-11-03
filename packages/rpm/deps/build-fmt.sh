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
SPEC_FILE=$(rpm -E %_specdir)/fmt-$VERSION-$VERSIONSSV.spec

cat << 'EOF' > $SPEC_FILE
Name:    fmt
Version: %{_version}
Release: ssv1%{?dist}
Summary: Small, safe and fast formatting library for C++
Group:   Development/Libraries/C and C++
License: BSD and ASL2.0
URL:     https://github.com/fmtlib/fmt
Source0: https://github.com/fmtlib/fmt/archive/refs/tags/%{version}.tar.gz
BuildRequires: cmake
BuildRequires: gcc
BuildRequires: gcc-c++
BuildRequires: ninja-build

BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{_version}-%{release}-XXXXXX)

%description

C++ Format is an open-source formatting library for C++. It can be used as a
safe alternative to printf or as a fast alternative to IOStreams.

%package -n %{name}-devel
Summary: Small, safe and fast formatting library for C++
Group:   Development/Libraries/C and C++
Requires: %{name} = %{version}
%description -n %{name}-devel

C++ Format is an open-source formatting library for C++. It can be used as a
safe alternative to printf or as a fast alternative to IOStreams.

%prep
%autosetup -p1

%build
%cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON \
  -DFMT_CMAKE_DIR:STRING=%{_libdir}/cmake/%{name} \
  -DFMT_LIB_DIR:STRING=%{_libdir}

%cmake_build

%install
%cmake_install

%clean
rm -rf %{buildroot}
%post   -n %{name} -p /sbin/ldconfig
%postun -n %{name} -p /sbin/ldconfig

%files -n %{name}
%license LICENSE.rst
%doc ChangeLog.rst README.rst

%{_libdir}/lib%{name}.so.9*

%files -n %{name}-devel

%{_includedir}/%{name}
%{_libdir}/lib%{name}.so
%{_libdir}/cmake/%{name}
%{_libdir}/pkgconfig/%{name}.pc

EOF

$SUDO_PREFIX yum-builddep -y "$SPEC_FILE" || \
  { echo "can't install build requirements" >&2 ; exit 1 ; }

spectool --force -g -R --define "_version $VERSION" "$SPEC_FILE" || \
  { echo "can't download sources" >&2 ; exit 1 ; }

rpmbuild --force -ba --define "_version $VERSION" "$SPEC_FILE" || \
  { echo "can't build RPM" >&2 ; exit 1 ; }

# install
cp $BIN_RPM_FOLDER/fmt-*$VERSION-*.rpm $RES_RPMS/

