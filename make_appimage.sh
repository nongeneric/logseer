#!/bin/bash

# point LINUXDEPLOY to linuxdeploy-x86_64.AppImage
# put linuxdeploy-plugin-qt-x86_64.AppImage next to it

set -e

SRC=../logseer

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
mv logseer-x86_64.AppImage logseer-x86_64.glibc_${LIBC::-1}.AppImage
