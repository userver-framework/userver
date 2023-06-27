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

# create folders for RPM build environment
mkdir -vp  `rpm -E '%_tmppath %_rpmdir %_builddir %_sourcedir %_specdir %_srcrpmdir %_rpmdir/%_arch'`

BIN_RPM_FOLDER=$(rpm -E '%_rpmdir/%_arch')

SPEC_FILE=$(rpm -E %_specdir)/amqp-cpp-$VERSION-$VERSIONSSV.spec

cat << 'EOF' > $SPEC_FILE
Name:    amqp-cpp
Version: %{_version}
Release: ssv1%{?dist}
Summary: C++ library for communicating with a RabbitMQ message broker
Group:   Development/Libraries/C and C++
License: Apache License 2.0
URL:     https://github.com/CopernicaMarketingSoftware/AMQP-CPP
Source0: https://github.com/CopernicaMarketingSoftware/AMQP-CPP/archive/refs/tags/v%{version}.tar.gz
BuildRequires: cmake
BuildRequires: gcc
BuildRequires: gcc-c++

BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{_version}-%{release}-XXXXXX)

%description

AMQP-CPP is a C++ library for communicating with a RabbitMQ message broker.
The library can be used to parse incoming data from, and generate frames to, a RabbitMQ server.

%package -n %{name}-devel
Summary: C++ library for communicating with a RabbitMQ message broker
Group:   Development/Libraries/C and C++
Requires: %{name} = %{version}
%description -n %{name}-devel

AMQP-CPP is a C++ library for communicating with a RabbitMQ message broker.
The library can be used to parse incoming data from, and generate frames to, a RabbitMQ server.

%prep
%setup -q -n AMQP-CPP-%{_version}

%build
%cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX:PATH=/usr \
  -DAMQP-CPP_BUILD_SHARED:BOOL=ON \
  -DAMQP-CPP_LINUX_TCP:BOOL=ON \
  -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON

make -j

%install
DESTDIR=%{buildroot}/ make install INSTALL_PATH=%{buildroot}/
mv %{buildroot}/usr/lib %{buildroot}/usr/lib64
mkdir -p %{buildroot}/usr/lib64/cmake
mv %{buildroot}/usr/cmake %{buildroot}/usr/lib64/cmake/amqpcpp
sed -i -r 's|get_filename_component\(_IMPORT_PREFIX "\$\{CMAKE_CURRENT_LIST_FILE\}" PATH\)|set(_IMPORT_PREFIX "/usr/x")|' \
  %{buildroot}/usr/lib64/cmake/amqpcpp/amqpcppConfig.cmake
sed -i -r 's|\{_IMPORT_PREFIX\}/lib/|{_IMPORT_PREFIX}/lib64/|' \
  %{buildroot}/usr/lib64/cmake/amqpcpp/amqpcppConfig-relwithdebinfo.cmake

%clean
rm -rf %{buildroot}

%files -n %{name}
%license LICENSE
%doc README.md

%{_libdir}/libamqpcpp.so.*

%files -n %{name}-devel

%{_includedir}/
%{_libdir}/pkgconfig
%{_libdir}/cmake/amqpcpp
%{_libdir}/libamqpcpp.so

EOF

$SUDO_PREFIX yum-builddep -y "$SPEC_FILE" || \
  { echo "can't install build requirements" >&2 ; exit 1 ; }

spectool --force -g -R --define "_version $VERSION" "$SPEC_FILE" || \
  { echo "can't download sources" >&2 ; exit 1 ; }

rpmbuild --force -ba --define "_version $VERSION" "$SPEC_FILE" || \
  { echo "can't build RPM" >&2 ; exit 1 ; }

# install
cp $BIN_RPM_FOLDER/amqp-cpp-*$VERSION-*.rpm $RES_RPMS/

