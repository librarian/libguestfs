#!/bin/bash -
# Important: Read 'README-PACKAGING' first.

set -e

if [ ! -d debian -o ! -f BUGS ]; then
    echo "You must run this script from the top level directory."
    exit 1
fi

# Find current version.
version=`head -1 debian/changelog | grep -o '[0-9]*\.[0-9]*\.[0-9]*'`

git rm -f --ignore-unmatch debian/patches/*.patch debian/patches/series

# Get patches since this version.
git format-patch -o debian/patches $version

# Remove non-patches.
pushd debian/patches
rm *NOT-PATCH*

# Make series file.
ls -1 *.patch > series

# Add everything to git.
git add -f *.patch series

popd

echo "copy-patches.sh: Updated debian/patches directory:"
ls -l debian/patches

echo "Now you probably want to commit this to git, but make sure"
echo "your commit message has 'NOT PATCH' in the title, else the"
echo "universe will go into a recursive loop."
