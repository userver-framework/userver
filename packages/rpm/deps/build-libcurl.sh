#!/bin/bash

VERSION=7.86.0
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

# download librdkafka source RPM
SPEC_FILE=$(rpm -E %_specdir)/libcurl-$VERSION-$VERSIONSSV.spec

SOURCES_DIR=$(rpm -E %_sourcedir)
cp libcurl/0101-curl-7.32.0-multilib.patch $SOURCES_DIR/
cp libcurl/0102-curl-7.84.0-test3026.patch $SOURCES_DIR/

cat << 'EOF' > $SPEC_FILE
Summary: A utility for getting files from remote servers (FTP, HTTP, and others)
Name: curl
Version: %{_version}
Release: 1%{?dist}
License: MIT
Source0: https://curl.se/download/%{name}-%{version}.tar.xz
Source1: https://curl.se/download/%{name}-%{version}.tar.xz.asc
# The curl download page ( https://curl.se/download.html ) links
# to Daniel's address page https://daniel.haxx.se/address.html for the GPG Key,
# which points to the GPG key as of April 7th 2016 of https://daniel.haxx.se/mykey.asc
Source2: https://daniel.haxx.se/mykey.asc
 
# patch making libcurl multilib ready
Patch101: 0101-curl-7.32.0-multilib.patch

# test3026: disable valgrind
Patch102: 0102-curl-7.84.0-test3026.patch
 
Provides: curl-full = %{version}-%{release}
Provides: webclient
URL: https://curl.se/
BuildRequires: automake
BuildRequires: brotli-devel
BuildRequires: coreutils
BuildRequires: gcc
BuildRequires: groff
BuildRequires: krb5-devel
BuildRequires: libidn2-devel
BuildRequires: libnghttp2-devel
BuildRequires: libpsl-devel
BuildRequires: libssh-devel
BuildRequires: libtool
BuildRequires: make
BuildRequires: openldap-devel
BuildRequires: openssh-clients
BuildRequires: openssh-server
BuildRequires: openssl-devel
BuildRequires: perl-interpreter
BuildRequires: pkgconfig
BuildRequires: python3
BuildRequires: python3-devel
BuildRequires: sed
BuildRequires: zlib-devel
 
# For gpg verification of source tarball
BuildRequires: gnupg2
 
# needed to compress content of tool_hugehelp.c after changing curl.1 man page
BuildRequires: perl(IO::Compress::Gzip)
 
# needed for generation of shell completions
BuildRequires: perl(Getopt::Long)
BuildRequires: perl(Pod::Usage)
BuildRequires: perl(strict)
BuildRequires: perl(warnings)
 
# gnutls-serv is used by the upstream test-suite
BuildRequires: gnutls-utils
 
# hostname(1) is used by the test-suite but it is missing in armv7hl buildroot
BuildRequires: hostname
 
# nghttpx (an HTTP/2 proxy) is used by the upstream test-suite
BuildRequires: libnghttp2
 
# perl modules used in the test suite
BuildRequires: perl(Cwd)
BuildRequires: perl(Digest::MD5)
BuildRequires: perl(Digest::SHA)
BuildRequires: perl(Exporter)
BuildRequires: perl(File::Basename)
BuildRequires: perl(File::Copy)
BuildRequires: perl(File::Spec)
BuildRequires: perl(IPC::Open2)
BuildRequires: perl(MIME::Base64)
BuildRequires: perl(Time::Local)
BuildRequires: perl(Time::HiRes)
BuildRequires: perl(vars)
 
%if 0%{?fedora}
# needed for upstream test 1451
BuildRequires: python3-impacket
%endif
 
# The test-suite runs automatically through valgrind if valgrind is available
# on the system.  By not installing valgrind into mock's chroot, we disable
# this feature for production builds on architectures where valgrind is known
# to be less reliable, in order to avoid unnecessary build failures (see RHBZ
# #810992, #816175, and #886891).  Nevertheless developers are free to install
# valgrind manually to improve test coverage on any architecture.
%ifarch x86_64
BuildRequires: valgrind
%endif
 
# stunnel is used by upstream tests but it does not seem to work reliably
# on s390x and occasionally breaks some tests (mainly 1561 and 1562)
%ifnarch s390x
BuildRequires: stunnel
%endif
 
# using an older version of libcurl could result in CURLE_UNKNOWN_OPTION
Requires: libcurl%{?_isa} >= %{version}-%{release}
 
# require at least the version of libpsl that we were built against,
# to ensure that we have the necessary symbols available (#1631804)
%global libpsl_version %(pkg-config --modversion libpsl 2>/dev/null || echo 0)
 
# require at least the version of libssh that we were built against,
# to ensure that we have the necessary symbols available (#525002, #642796)
%global libssh_version %(pkg-config --modversion libssh 2>/dev/null || echo 0)
 
# require at least the version of openssl-libs that we were built against,
# to ensure that we have the necessary symbols available (#1462184, #1462211)
# (we need to translate 3.0.0-alpha16 -> 3.0.0-0.alpha16 and 3.0.0-beta1 -> 3.0.0-0.beta1 though)
%global openssl_version %({ pkg-config --modversion openssl 2>/dev/null || echo 0;} | sed 's|-|-0.|')
 
%description
curl is a command line tool for transferring data with URL syntax, supporting
FTP, FTPS, HTTP, HTTPS, SCP, SFTP, TFTP, TELNET, DICT, LDAP, LDAPS, FILE, IMAP,
SMTP, POP3 and RTSP.  curl supports SSL certificates, HTTP POST, HTTP PUT, FTP
uploading, HTTP form based upload, proxies, cookies, user+password
authentication (Basic, Digest, NTLM, Negotiate, kerberos...), file transfer
resume, proxy tunneling and a busload of other useful tricks. 
 
%package -n libcurl
Summary: A library for getting files from web servers
Requires: libpsl%{?_isa} >= %{libpsl_version}
Requires: libssh%{?_isa} >= %{libssh_version}
Requires: openssl-libs%{?_isa} >= 1:%{openssl_version}
Provides: libcurl-full = %{version}-%{release}
Provides: libcurl-full%{?_isa} = %{version}-%{release}
 
%description -n libcurl
libcurl is a free and easy-to-use client-side URL transfer library, supporting
FTP, FTPS, HTTP, HTTPS, SCP, SFTP, TFTP, TELNET, DICT, LDAP, LDAPS, FILE, IMAP,
SMTP, POP3 and RTSP. libcurl supports SSL certificates, HTTP POST, HTTP PUT,
FTP uploading, HTTP form based upload, proxies, cookies, user+password
authentication (Basic, Digest, NTLM, Negotiate, Kerberos4), file transfer
resume, http proxy tunneling and more.
 
%package -n libcurl-devel
Summary: Files needed for building applications with libcurl
Requires: libcurl%{?_isa} = %{version}-%{release}
 
Provides: curl-devel = %{version}-%{release}
Provides: curl-devel%{?_isa} = %{version}-%{release}
Obsoletes: curl-devel < %{version}-%{release}
 
%description -n libcurl-devel
The libcurl-devel package includes header files and libraries necessary for
developing programs which use the libcurl library. It contains the API
documentation of the library, too.
 
%package -n curl-minimal
Summary: Conservatively configured build of curl for minimal installations
Provides: curl = %{version}-%{release}
Conflicts: curl
Suggests: libcurl-minimal
RemovePathPostfixes: .minimal
 
# using an older version of libcurl could result in CURLE_UNKNOWN_OPTION
Requires: libcurl%{?_isa} >= %{version}-%{release}
 
%description -n curl-minimal
This is a replacement of the 'curl' package for minimal installations.  It
comes with a limited set of features compared to the 'curl' package.  On the
other hand, the package is smaller and requires fewer run-time dependencies to
be installed.
 
%package -n libcurl-minimal
Summary: Conservatively configured build of libcurl for minimal installations
Requires: openssl-libs%{?_isa} >= 1:%{openssl_version}
Provides: libcurl = %{version}-%{release}
Provides: libcurl%{?_isa} = %{version}-%{release}
Conflicts: libcurl%{?_isa}
RemovePathPostfixes: .minimal
# needed for RemovePathPostfixes to work with shared libraries
%undefine __brp_ldconfig
 
%description -n libcurl-minimal
This is a replacement of the 'libcurl' package for minimal installations.  It
comes with a limited set of features compared to the 'libcurl' package.  On the
other hand, the package is smaller and requires fewer run-time dependencies to
be installed.
 
%prep
%{gpgverify} --keyring='%{SOURCE2}' --signature='%{SOURCE1}' --data='%{SOURCE0}'
%setup -q

# upstream patches
 
# Fedora patches
%patch101 -p1
%patch102 -p1
 
# disable test 1112 (#565305), test 1455 (occasionally fails with 'bind failed
# with errno 98: Address already in use' in Koji environment), and test 1801
# <https://github.com/bagder/curl/commit/21e82bd6#commitcomment-12226582>
printf "1112\n1455\n1184\n1801\n" >> tests/data/DISABLED
 
# disable test 1319 on ppc64 (server times out)
%ifarch ppc64
echo "1319" >> tests/data/DISABLED
%endif
 
# disable tests 320..322 on ppc64le where it started to hang/fail
%ifarch ppc64le
printf "320\n321\n322\n" >> tests/data/DISABLED
%endif
 
# temporarily disable tests 582 and 1452 on s390x (client times out)
%ifarch s390x
printf "582\n1452\n" >> tests/data/DISABLED
%endif
 
# temporarily disable tests 702 703 716 on armv7hl (#1829180)
%ifarch armv7hl
printf "702\n703\n716\n" >> tests/data/DISABLED
%endif
 
# temporarily disable tests 300{0,1} on x86_64 (stunnel clashes with itself)
%ifarch x86_64
printf "3000\n3001\n" >> tests/data/DISABLED
%endif
 
# test3026: avoid pthread_create() failure due to resource exhaustion on i386
%ifarch %{ix86}
sed -e 's|NUM_THREADS 1000$|NUM_THREADS 256|' \
    -i tests/libtest/lib3026.c
%endif
 
# adapt test 323 for updated OpenSSL
sed -e 's|^35$|35,52|' -i tests/data/test323
 
# use localhost6 instead of ip6-localhost in the curl test-suite
(
    # avoid glob expansion in the trace output of `bash -x`
    { set +x; } 2>/dev/null
    cmd="sed -e 's|ip6-localhost|localhost6|' -i tests/data/test[0-9]*"
    printf "+ %s\n" "$cmd" >&2
    eval "$cmd"
)
 
# regenerate the configure script and Makefile.in files
autoreconf -fiv
 
%build
mkdir build-{full,minimal}
export common_configure_opts="          \
    --cache-file=../config.cache        \
    --disable-static                    \
    --enable-hsts                       \
    --enable-ipv6                       \
    --enable-symbol-hiding              \
    --enable-threaded-resolver          \
    --without-zstd                      \
    --with-gssapi                       \
    --with-libidn2                      \
    --with-nghttp2                      \
    --with-ssl --with-ca-bundle=%{_sysconfdir}/pki/tls/certs/ca-bundle.crt"
 
%global _configure ../configure
 
# configure minimal build
(
    cd build-minimal
    %configure $common_configure_opts   \
        --disable-dict                  \
        --disable-gopher                \
        --disable-imap                  \
        --disable-ldap                  \
        --disable-ldaps                 \
        --disable-manual                \
        --disable-mqtt                  \
        --disable-ntlm                  \
        --disable-ntlm-wb               \
        --disable-pop3                  \
        --disable-rtsp                  \
        --disable-smb                   \
        --disable-smtp                  \
        --disable-telnet                \
        --disable-tftp                  \
        --disable-tls-srp               \
        --without-brotli                \
        --without-libpsl                \
        --without-libssh
)
 
# configure full build
(
    cd build-full
    %configure $common_configure_opts   \
        --enable-dict                   \
        --enable-gopher                 \
        --enable-imap                   \
        --enable-ldap                   \
        --enable-ldaps                  \
        --enable-manual                 \
        --enable-mqtt                   \
        --enable-ntlm                   \
        --enable-ntlm-wb                \
        --enable-pop3                   \
        --enable-rtsp                   \
        --enable-smb                    \
        --enable-smtp                   \
        --enable-telnet                 \
        --enable-tftp                   \
        --enable-tls-srp                \
        --with-brotli                   \
        --with-libpsl                   \
        --with-libssh
)
 
# avoid using rpath
sed -e 's/^runpath_var=.*/runpath_var=/' \
    -e 's/^hardcode_libdir_flag_spec=".*"$/hardcode_libdir_flag_spec=""/' \
    -i build-{full,minimal}/libtool
 
%make_build V=1 -C build-minimal
%make_build V=1 -C build-full
 
%check
# compile upstream test-cases
#%make_build V=1 -C build-minimal/tests
#%make_build V=1 -C build-full/tests
 
# relax crypto policy for the test-suite to make it pass again (#1610888)
#export OPENSSL_SYSTEM_CIPHERS_OVERRIDE=XXX
#export OPENSSL_CONF=
 
# make runtests.pl work for out-of-tree builds
#export srcdir=../../tests
 
# prevent valgrind from being extremely slow (#1662656)
# https://fedoraproject.org/wiki/Changes/DebuginfodByDefault
#unset DEBUGINFOD_URLS
 
# run the upstream test-suite for both curl-minimal and curl-full
#for size in minimal full; do (
#    cd build-${size}
 
    # we have to override LD_LIBRARY_PATH because we eliminated rpath
#    export LD_LIBRARY_PATH="${PWD}/lib/.libs"
 
#    cd tests
#    perl -I../../tests ../../tests/runtests.pl -a -p -v '!flaky'
#)
#done
 
 
%install
# install and rename the library that will be packaged as libcurl-minimal
%make_install -C build-minimal/lib
rm -f ${RPM_BUILD_ROOT}%{_libdir}/libcurl.{la,so}
for i in ${RPM_BUILD_ROOT}%{_libdir}/*; do
    mv -v $i $i.minimal
done
 
# install and rename the executable that will be packaged as curl-minimal
%make_install -C build-minimal/src
mv -v ${RPM_BUILD_ROOT}%{_bindir}/curl{,.minimal}
 
# install libcurl.m4
install -d $RPM_BUILD_ROOT%{_datadir}/aclocal
install -m 644 docs/libcurl/libcurl.m4 $RPM_BUILD_ROOT%{_datadir}/aclocal
 
# install the executable and library that will be packaged as curl and libcurl
cd build-full
%make_install
# install zsh completion for curl
# (we have to override LD_LIBRARY_PATH because we eliminated rpath)
LD_LIBRARY_PATH="$RPM_BUILD_ROOT%{_libdir}:$LD_LIBRARY_PATH" \
    %make_install -C scripts
 
# do not install /usr/share/fish/completions/curl.fish which is also installed
# by fish-3.0.2-1.module_f31+3716+57207597 and would trigger a conflict
rm -rf ${RPM_BUILD_ROOT}%{_datadir}/fish
 
rm -f ${RPM_BUILD_ROOT}%{_libdir}/libcurl.la
 
%ldconfig_scriptlets -n libcurl
 
%ldconfig_scriptlets -n libcurl-minimal
 
%files
%doc CHANGES
%doc README
%doc docs/BUGS.md
%doc docs/FAQ
%doc docs/FEATURES.md
%doc docs/TODO
%doc docs/TheArtOfHttpScripting.md
%{_bindir}/curl
%{_mandir}/man1/curl.1*
%{_datadir}/zsh
 
%files -n libcurl
%license COPYING
%{_libdir}/libcurl.so.4
%{_libdir}/libcurl.so.4.[0-9].[0-9]
 
%files -n libcurl-devel
%doc docs/examples/*.c docs/examples/Makefile.example docs/INTERNALS.md
%doc docs/CONTRIBUTE.md docs/libcurl/ABI.md
%{_bindir}/curl-config*
%{_includedir}/curl
%{_libdir}/*.so
%{_libdir}/pkgconfig/*.pc
%{_mandir}/man1/curl-config.1*
%{_mandir}/man3/*
%{_datadir}/aclocal/libcurl.m4
 
%files -n curl-minimal
%{_bindir}/curl.minimal
%{_mandir}/man1/curl.1*
 
%files -n libcurl-minimal
%license COPYING
%{_libdir}/libcurl.so.4.minimal
%{_libdir}/libcurl.so.4.[0-9].[0-9].minimal
EOF

$SUDO_PREFIX yum-builddep -y "$SPEC_FILE" || \
  { echo "can't install build requirements" >&2 ; exit 1 ; }

spectool --force -g -R --define "_version $VERSION" "$SPEC_FILE" || \
  { echo "can't download sources" >&2 ; exit 1 ; }

rpmbuild --force -ba --define "_version $VERSION" "$SPEC_FILE" || \
  { echo "can't build RPM" >&2 ; exit 1 ; }

# install
cp $BIN_RPM_FOLDER/libcurl*.rpm $RES_RPMS/

