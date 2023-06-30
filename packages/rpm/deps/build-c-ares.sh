#!/bin/bash

VERSION=1.18.1
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

BIN_RPM_FOLDER=$(rpm -E '%_rpmdir/%_arch')

# download source RPM
curl 'http://c-ares.haxx.se/license.html' >$(rpm -E '%_sourcedir')/LICENSE

SPEC_FILE=$(rpm -E %_specdir)/c-ares-$VERSION-$VERSIONSSV.spec

cat << 'EOF' > $SPEC_FILE
Summary: A library that performs asynchronous DNS operations
Name: c-ares
Version: %{_version}
Release: 4%{?dist}
License: MIT
Group: System Environment/Libraries
URL: http://c-ares.haxx.se/
Source0: http://c-ares.haxx.se/download/%{name}-%{version}.tar.gz
# The license can be obtained at http://c-ares.haxx.se/license.html
Source1: LICENSE
#Patch0: 0001-Use-RPM-compiler-options.patch
#Patch1: c-ares-1.10.0-multilib.patch
 
BuildRequires: autoconf
BuildRequires: automake
BuildRequires: libtool
 
%description
c-ares is a C library that performs DNS requests and name resolves 
asynchronously. c-ares is a fork of the library named 'ares', written 
by Greg Hudson at MIT.
 
%package devel
Summary: Development files for c-ares
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}
Requires: pkgconfig
 
%description devel
This package contains the header files and libraries needed to
compile applications or shared objects that use c-ares.
 
%prep
%setup -q
#%patch0 -p1 -b .optflags
#%patch1 -p1 -b .multilib
 
cp %{SOURCE1} .
f=CHANGES ; iconv -f iso-8859-1 -t utf-8 $f -o $f.utf8 ; mv $f.utf8 $f
 
%build
autoreconf -if
%configure --enable-shared --disable-static --disable-dependency-tracking --enable-debug CFLAGS="-fPIC"
%{__make} %{?_smp_mflags} VERBOSE=1
 
%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install
rm -f $RPM_BUILD_ROOT/%{_libdir}/libcares.la
 
%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig
 
%files
%defattr(-, root, root)
%doc README.cares CHANGES NEWS LICENSE
%{_libdir}/*.so.*
 
%files devel
%defattr(-, root, root, 0755)
%{_includedir}/ares_nameser.h
%{_includedir}/ares.h
%{_includedir}/ares_build.h
%{_includedir}/ares_dns.h
%{_includedir}/ares_rules.h
%{_includedir}/ares_version.h
%{_libdir}/*.so
%{_libdir}/pkgconfig/libcares.pc
%{_mandir}/man3/ares_*
EOF

$SUDO_PREFIX yum-builddep -y "$SPEC_FILE" || \
  { echo "can't install build requirements" >&2 ; exit 1 ; }

spectool --force -g -R --define "_version $VERSION" "$SPEC_FILE" || \
  { echo "can't download sources" >&2 ; exit 1 ; }

rpmbuild --force -ba --define "_version $VERSION" "$SPEC_FILE" || \
  { echo "can't build RPM" >&2 ; exit 1 ; }

# install
cp $BIN_RPM_FOLDER/c-ares*.rpm $RES_RPMS/

