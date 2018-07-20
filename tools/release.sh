#!/bin/bash
set -e

export LANG="C"

PROJECT_ARCH=`dpkg-architecture | grep DEB_BUILD_ARCH= | sed -e 's/.*=//'`

[[ -z "$DEBFULLNAME" ]] && { echo "DEBFULLNAME variable is not set"; exit 1; }
[[ -z "$DEBEMAIL" ]] && { echo "DEBEMAIL variable is not set"; exit 1; }

git fetch origin
git checkout master && git pull origin master
git checkout develop && git pull origin develop
git fetch origin --tags

BUILD_NUMBER="${BUILD_NUMBER:-`head debian/changelog -n1 | gawk -F'[ .()]' '{ print $3 "." $4 "." (strtonum($5) + 1) }'`}"
SOURCE_PACKAGE_NAME="${SOURCE_PACKAGE_NAME:-yandex-taxi-userver}"

echo "BUILD_NUMBER=$BUILD_NUMBER"
echo "SOURCE_PACKAGE_NAME=$SOURCE_PACKAGE_NAME"

git checkout -b release/$BUILD_NUMBER develop

LAST_RELEASE=`git describe --tags $(git rev-list --tags --max-count=1)`
LOG=$(git log --no-merges --format='  * %an | %s' $LAST_RELEASE..HEAD)

if [ -n "$LOG" ]
then
	mv debian/changelog debian/changelog.old
	echo "$SOURCE_PACKAGE_NAME ($BUILD_NUMBER) unstable; urgency=medium" > debian/changelog
	echo "" >> debian/changelog
	echo "$LOG" >> debian/changelog
	echo "" >> debian/changelog
	echo " -- $DEBFULLNAME <$DEBEMAIL>  $(date "+%a, %d %b %Y %T %z")" >> debian/changelog
	echo "" >> debian/changelog
	cat debian/changelog.old >> debian/changelog
	rm debian/changelog.old
	git commit -a -m "edit changelog $BUILD_NUMBER"

	git checkout master
	git merge --no-ff release/$BUILD_NUMBER --no-edit
	git tag -a $BUILD_NUMBER -m $BUILD_NUMBER
	git checkout develop
	git merge --no-ff release/$BUILD_NUMBER --no-edit
	git branch -d release/$BUILD_NUMBER

	git checkout master && git push origin master
	git checkout develop && git push origin develop
	git push origin --tags
else
	BUILD_NUMBER=$(dpkg-parsechangelog --show-field version)
fi

make clean
export DEB_BUILD_OPTIONS="parallel=$(nproc) $DEB_BUILD_OPTIONS"
debuild -e CC=clang-5.0 -e CXX=clang++-5.0

cd ..
dupload --nomail --to=yandex-taxi-xenial $SOURCE_PACKAGE_NAME\_$BUILD_NUMBER\_$PROJECT_ARCH.changes

#if [ -n "$CREATE_TICKET" ]
#then
#	cd $PROJECT_DIR
#	debticket -n --auth ea4dd09cb1046b72fa2b75c42916e91e
#fi
