#!/bin/bash

# point LINUXDEPLOY to linuxdeploy-x86_64.AppImage
# put linuxdeploy-plugin-qt-x86_64.AppImage next to it

set -e

SRC=../logseer/src

rm -rf /tmp/AppDir
rm -f *.AppImage

ninja logseer
${LINUXDEPLOY} --plugin qt \
    --appdir=/tmp/AppDir \
    -d ${SRC}/logseer.desktop \
    -e gui/logseer \
    -i ${SRC}/gui/resources/logseer.svg \
    --output appimage

LIBC=`/usr/lib64/libc.so.6 | grep "release version" | awk -F " " '{print $NF}'`
VERSION=`grep "project(" -i $SRC/CMakeLists.txt | awk -F " " '{print $3}'`
mv logseer-x86_64.AppImage logseer-${VERSION}.x86_64.glibc_${LIBC::-1}.AppImage
