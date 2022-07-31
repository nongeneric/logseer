#!/bin/bash

# point LINUXDEPLOY to linuxdeploy-x86_64.AppImage
# point SRC to the source directory
# put linuxdeploy-plugin-qt-x86_64.AppImage next to it

set -e

rm -rf /tmp/AppDir
rm -f *.AppImage

ninja logseer
${LINUXDEPLOY} --plugin qt \
    --appdir=/tmp/AppDir \
    -d ${SRC}/logseer.desktop \
    -e gui/logseer \
    -i ${SRC}/gui/resources/logseer.svg \
    --output appimage

LIBC=`/usr/lib64/libc.so.6 | head -n1 | perl -pe 's/.*?((\d+\.\d+)+).*/$1/g'`
VERSION=`grep "project(" -i $SRC/CMakeLists.txt | awk -F " " '{print $3}'`
mv logseer-x86_64.AppImage logseer-${VERSION}.x86_64.glibc_${LIBC}.AppImage
