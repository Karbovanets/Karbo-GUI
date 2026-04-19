#!/bin/bash
set -e

APP=Karbo-GUI
APPDIR=AppDir

# Download tools
if ! test -f linuxdeploy-x86_64.AppImage; then
    wget -q https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
fi

if ! test -f linuxdeploy-plugin-qt-x86_64.AppImage; then
    wget -q https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
fi

chmod +x *.AppImage

# Clean AppDir
rm -rf $APPDIR
mkdir -p $APPDIR/usr/bin
mkdir -p $APPDIR/usr/share/icons/hicolor/256x256/apps
mkdir -p $APPDIR/usr/share/applications

# Copy binary
cp ../build/release/$APP $APPDIR/usr/bin/

# Copy desktop file (generated at build time from src/karbo-gui.desktop.in)
cp ../build/release/karbo-gui.desktop $APPDIR/usr/share/applications/karbowanec.desktop

# Copy icon (the .desktop Icon= field is "karbowanec")
cp ../src/images/Karbovanets.png \
   $APPDIR/usr/share/icons/hicolor/256x256/apps/karbowanec.png

# Build AppImage
./linuxdeploy-x86_64.AppImage \
  --appdir $APPDIR \
  --executable $APPDIR/usr/bin/$APP \
  --desktop-file $APPDIR/usr/share/applications/karbowanec.desktop \
  --icon-file $APPDIR/usr/share/icons/hicolor/256x256/apps/karbowanec.png \
  --plugin qt \
  --output appimage
