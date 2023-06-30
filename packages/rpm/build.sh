#!/bin/bash

VERSION=$1
VERSIONSSV=ssv1

# script require 'sudo rpm' for install RPM packages
# and access to mirror.yandex.ru repository

# create build/RPMS folder - all built packages will be duplicated here
SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
RES_TMP=build/TMP/
RES_RPMS=build/RPMS/
rm -rf "$RES_TMP"

mkdir -p $RES_TMP
mkdir -p $RES_RPMS

# download and install packages required for build
sudo yum -y install spectool yum-utils rpmdevtools redhat-rpm-config rpm-build \
  epel-rpm-macros \
  || \
  { echo "can't install base packages" >&2 ; exit 1 ; }

# create folders for RPM build environment
mkdir -vp  `rpm -E '%_tmppath %_rpmdir %_builddir %_sourcedir %_specdir %_srcrpmdir %_rpmdir/%_arch'`

BIN_RPM_FOLDER=$(rpm -E '%_rpmdir/%_arch')

SPEC_FILE=$(rpm -E %_specdir)/userver-$VERSION-$VERSIONSSV.spec

# download spec file
#curl 'https://raw.githubusercontent.com/yoori/userver/1.0.0/packages/rpm/userver.spec' >"$SPEC_FILE"
cp $SCRIPTPATH/userver.spec "$SPEC_FILE"

$SUDO_PREFIX yum-builddep -y "$SPEC_FILE" || \
  { echo "can't install build requirements" >&2 ; exit 1 ; }

spectool --force -g -R --define "_version $VERSION" "$SPEC_FILE" || \
  { echo "can't download sources" >&2 ; exit 1 ; }

export PATH=/opt/rh/gcc-toolset-11/root/usr/bin/:$PATH

rpmbuild --force -ba --define "_version $VERSION" "$SPEC_FILE" || \
  { echo "can't build RPM" >&2 ; exit 1 ; }

# install
#cp $BIN_RPM_FOLDER/spdlog*.rpm $RES_RPMS/

