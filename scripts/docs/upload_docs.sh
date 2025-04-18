#!/bin/sh

if [ -z "$BUILD_DIR" ]; then
    echo "!!! Set BUILD_DIR environment variable to cmake build directory with docs already built."
    echo "!!! See userver/scripts/docs/README.md"
    exit 2
fi

cd "$BUILD_DIR"
rm -rf userver-framework.github.io
git clone git@github.com:userver-framework/userver-framework.github.io.git
cd userver-framework.github.io

git checkout --orphan tmp-develop  # Create a temporary branch
git rm -rf --quiet .
cp -a "$BUILD_DIR/docs/html/." .
git add .  # Add all files and commit them
git commit --quiet -m 'Docs update'
git branch -D develop  # Delete the develop branch
git branch -m develop  # Rename the current branch to develop

echo "Please open $PWD in any local browser, e.g. VSCode LiveServer, and verify that docs are OK."
echo "You are about to force-push userver docs, replacing the contents of https://userver.tech"
echo -n "Are you sure? [yn] "
read REPLY
echo
if [ "$REPLY" = "y" ]; then
    git push -f origin develop  # Force push develop branch to Git server
fi
