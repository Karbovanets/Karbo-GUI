// Copyright (c) 2015-2017, The Bytecoin developers
// Copyright (c) 2017-2018, The Karbo developers
//
// This file is part of Karbo.
//
// Karbovanets is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Karbovanets is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Karbovanets.  If not, see <http://www.gnu.org/licenses/>.

#include "LightStyle.h"

namespace WalletGui {

namespace {
QPixmap renderIcon(const QString& _path, int _size) {
  return QIcon(_path).pixmap(_size, _size);
}
}

LightStyle::LightStyle() : Style("light", "Light") {

}

QString LightStyle::statusBarBackgroundColor() const {
  return "#CADEF7";
}

QString LightStyle::statusBarFontColor() const {
  return "#2A4268";
}

QString LightStyle::headerBackgroundColor() const {
  return "#EDF4FC";
}

QString LightStyle::headerBorderColor() const {
  return "#dddddd";
}

QString LightStyle::addressFontColor() const {
  return "#2A4268";
}

QString LightStyle::balanceFontColor() const {
  return "#2A4268";
}

QString LightStyle::toolButtonBackgroundColorNormal() const {
  return "#DFECFC";
}

QString LightStyle::toolButtonBackgroundColorHover() const {
  return "#C1D5ED";
}

QString LightStyle::toolButtonBackgroundColorPressed() const {
  return "#00A0E3";
}

QString LightStyle::toolButtonFontColorNormal() const {
  return "#2E4469";
}

QString LightStyle::toolButtonFontColorDisabled() const {
  return "#8C949E";
}

QString LightStyle::toolBarBorderColor() const {
  return "#dddddd";
}

QString LightStyle::getWalletSyncGifFile() const {
  return QString(":icons/light/wallet-sync");
}

QPixmap LightStyle::getLogoPixmap() const {
  return QPixmap(QString(":icons/light/logo"));
}

QPixmap LightStyle::getBalanceIcon() const {
  return renderIcon(QString(":icons/total_balance"), 32);
}

QPixmap LightStyle::getConnectedIcon() const {
  return renderIcon(QString(":icons/light/connected"), 20);
}

QPixmap LightStyle::getDisconnectedIcon() const {
  return renderIcon(QString(":icons/light/disconnected"), 20);
}

QPixmap LightStyle::getEncryptedIcon() const {
  return renderIcon(QString(":icons/light/encrypted"), 20);
}

QPixmap LightStyle::getNotEncryptedIcon() const {
  return renderIcon(QString(":icons/light/decrypted"), 20);
}

QPixmap LightStyle::getSyncedIcon() const {
  return renderIcon(QString(":icons/light/synced"), 20);
}

}
