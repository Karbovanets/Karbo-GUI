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

LightStyle::LightStyle() : Style("light", "Light") {

}

QString LightStyle::fontColorGeneral() const {
  return "#000000";
}

QString LightStyle::backgroundColor() const {
  return "#ffffff";
}

QString LightStyle::backgroundColorAlt() const {
  return "#eff3fa";
}

QString LightStyle::backgroundColorAlternate() const {
  return "#f3f4f6";
}

QString LightStyle::backgroundColorDeep() const {
  return "#ffffff";
}

QString LightStyle::backgroundColorGray() const {
  return "#f4f4f4";
}

QString LightStyle::statusBarBackgroundColor() const {
  return "#cadef7";
}

QString LightStyle::statusBarFontColor() const {
  return "#2a4268";
}

QString LightStyle::headerBackgroundColor() const {
  return "#EDF4FC";
}

QString LightStyle::headerBorderColor() const {
  return "#dddddd";
}

QString LightStyle::addressFontColor() const {
  return "#2a4268";
}

QString LightStyle::balanceFontColor() const {
  return "#2a4268";
}

QString LightStyle::toolButtonBackgroundColorNormal() const {
  return "#dfecfc";
}

QString LightStyle::toolButtonBackgroundColorHover() const {
  return "#c1d5ed";
}

QString LightStyle::toolButtonBackgroundColorPressed() const {
  return "#00a0e3";
}

QString LightStyle::toolButtonFontColorNormal() const {
  return "#2e4469";
}

QString LightStyle::toolButtonFontColorDisabled() const {
  return "#8c949e";
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
  return QPixmap(QString(":icons/total_balance"));
}

QPixmap LightStyle::getConnectedIcon() const {
  return QPixmap(QString(":icons/light/connected"));
}

QPixmap LightStyle::getDisconnectedIcon() const {
  return QPixmap(QString(":icons/light/disconnected"));
}

QPixmap LightStyle::getEncryptedIcon() const {
  return QPixmap(QString(":icons/light/encrypted"));
}

QPixmap LightStyle::getNotEncryptedIcon() const {
  return QPixmap(QString(":icons/light/decrypted"));
}

QPixmap LightStyle::getSyncedIcon() const {
  return QPixmap(QString(":icons/light/synced"));
}

}
