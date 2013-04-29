#!/bin/bash -
# Important: Read 'README-PACKAGING' first.

set -e

if [ ! -d debian -o ! -f BUGS ]; then
    echo "You must run this script from the top level directory."
    exit 1
fi

rm -rf ../tmp.mksrc

mkdir ../tmp.mksrc

# Find current version.
version=`head -1 debian/changelog | grep -o '[0-9]*\.[0-9]*\.[0-9]*'`

zcat ../libguestfs_$version.orig.tar.gz | tar -x -C ../tmp.mksrc
git archive ubuntu debian/ | tar -x -C ../tmp.mksrc/libguestfs-$version

pushd ../tmp.mksrc

mv libguestfs-$version/{*,.[a-z]*} .
rmdir libguestfs-$version

./autogen.sh

debuild -i -uc -us -S

popd

rm -rf ../tmp.mksrc